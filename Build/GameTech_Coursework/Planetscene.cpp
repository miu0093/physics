#include "EmptyScene.h"
#include <nclgl\Vector4.h>
#include <ncltech\SceneManager.h>
#include <ncltech\PhysicsEngine.h>
#include <ncltech\CommonUtils.h>
#include <ncltech\DistanceConstraint.h>

using namespace CommonUtils;
EmptyScene::EmptyScene(const std::string& friendly_name)
	:Scene(friendly_name)
	,m_AccumTime(0.0f)
{}

EmptyScene::~EmptyScene() {}

const Vector3 orbitPos = Vector3(1.f, 20.0f, 0.0f);

void EmptyScene::OnInitializeScene() {

Scene::OnInitializeScene();
this->AddGameObject(CommonUtils::BuildCuboidGroundFloor("Ground",
	                              Vector3(0.0f, -1.0f, 0.0f),
	                              Vector3(20.0f, 1.0f, 20.0f), 
	                              true, 
	                              0.0f, 
	                              true, 
	                              false, 
	                              Vector4(1.f, 1.f, 1.0f, 1.0f)));

if (m_Network.Initialize(0))
{
	NCLDebug::Log("Network: Initialized!");

	//Attempt to connect to the server on localhost:1234
	m_pServerConnection = m_Network.ConnectPeer(127, 0, 0, 1, 1234);
	NCLDebug::Log("Network: Attempting to connect to server.");
}

SceneManager::Instance()->GetCamera()->SetPosition(Vector3(15.0f, 30.0f, 25.0f));
SceneManager::Instance()->GetCamera()->SetYaw(25.f);
SceneManager::Instance()->GetCamera()->SetPitch(-18.f);
PhysicsEngine::Instance()->SetDebugDrawFlags(DEBUGDRAW_FLAGS_CONSTRAINT);

//Create Sphere-Sphere Manifold Test
{
	Object* sphere = CommonUtils::BuildSphereObject("1",            //jupiter
		Vector3(1.f, 20.0f, 0.0f),		//Position leading to 0.25 meter overlap between spheres
		10.f,									//Radius
		true,									//Has Physics Object
		0.0f,									//Infinite Mass
		true,									//Has Collision Shape
		false,	//Dragable by the user
		Vector4(1.f, 1.f, 1.f, 1.f));
		//CommonUtils::GenColour(0.f, 1.0f));	//Color

	this->AddGameObject(sphere);
	ballbig = this->FindGameObject("1");
	ballbig->Physics()->SetRestState(true);

	this->AddGameObject(CommonUtils::BuildSphereObject("2",            //small planet
		//Vector3(orbitPos.x, orbitPos.y, orbitPos.z),
		Vector3(20.0f, 20.0f, 0.0f),										//Position
		1.25f,									//Radius
		true,									//Has Physics Object
		0.0f,									//Infinite Mass
		true,									//Has Collision Shape
		false,									//Dragable by the user
		Vector4(1.f, 1.f, 1.f, 1.f)));
		//CommonUtils::GenColour(0.f, 1.0f)));	//Color
	ballsmall = this->FindGameObject("2");
	ballsmall->Physics()->SetRestState(false);
}
//Create Sphere-Cuboid Manifold Test
{
	this->AddGameObject(CommonUtils::BuildCuboidObject("4",         //scoreboard
		Vector3(1.f, 32.f, 0.0f),									//Position
		Vector3(0.1f, 2.0f, 2.f),				//Half dimensions
		true,									//Has Physics Object
		0.0f,									//Infinite Mass
		true,									//Has Collision Shape
		false,									//Dragable by the user
		Vector4(1.f, 1.f, 1.f, 1.f)));
		//CommonUtils::GenColour(0.f, 1.0f)));	//Color

	scoreb = this->FindGameObject("4");
	scoreb->Physics()->SetRestState(false);
	scoreb->Physics()->SetTarget(true);
}

};

void EmptyScene::OnCleanupScene()
{
	Scene::OnCleanupScene();
	enet_peer_disconnect_now(m_pServerConnection, 0);

	//Release network and all associated data/peer connections
	m_Network.Release();
	m_pServerConnection = NULL;
}

void EmptyScene:: OnUpdateScene(float dt) 
{
	Scene::OnUpdateScene(dt);
	m_AccumTime += dt;

	 ballbig = this->FindGameObject("1");
	 ballsmall = this->FindGameObject("2");
	 ball3 = this->FindGameObject("3");
	 scoreb = this->FindGameObject("4");

	ballbig->Physics()->SetAngularVelocity(Vector3(0, 0.2, 0));

	auto callback = std::bind(
		&EmptyScene::ProcessNetworkEvent,	// Function to call
		this,								// Associated class instance
		std::placeholders::_1);				// Where to place the first parameter
	m_Network.ServiceNetwork(dt, callback);

	//Add Debug Information to screen
	uint8_t ip1 = m_pServerConnection->address.host & 0xFF;
	uint8_t ip2 = (m_pServerConnection->address.host >> 8) & 0xFF;
	uint8_t ip3 = (m_pServerConnection->address.host >> 16) & 0xFF;
	uint8_t ip4 = (m_pServerConnection->address.host >> 24) & 0xFF;

	NCLDebug::DrawTextWs(ballbig->Physics()->GetPosition() + Vector3(1.f, 20.0f, 0.0f), 14.f, TEXTALIGN_CENTRE, Vector4(),
		"Peer: %u.%u.%u.%u:%u", ip1, ip2, ip3, ip4, m_pServerConnection->address.port);

	Vector4 status_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	NCLDebug::AddStatusEntry(status_color, "Network Traffic");
	NCLDebug::AddStatusEntry(status_color, "    Incoming: %5.2fKbps", m_Network.m_IncomingKb);
	NCLDebug::AddStatusEntry(status_color, "    Outgoing: %5.2fKbps", m_Network.m_OutgoingKb);
	
	if (ballsmall != NULL)
	{
		ballsmall->Physics()->SetAngularVelocity(Vector3(0, 1, 0));

		ballsmall->Physics()->SetPosition(Vector3(
			orbitPos.x + 30 * cos(DegToRad(m_AccumTime *10.f)* -2.f)*0.75f,
			orbitPos.y,
			orbitPos.z + 30 * sin(DegToRad(m_AccumTime *10.f)* -2.f)*0.75f));
	}

	if (scoreb->Physics()->GetCollision() == true && PhysicsEngine::Instance()->GetScoreBoard()>0) {  //netcode server score state
		m_servergetpoint = true;
	}
	else {
		m_servergetpoint = false;
	}

	if (this->m_servergetpoint == true) {                 //server respond
		char* text_data_point = "You Scored GJ! ";
		ENetPacket* packet = enet_packet_create(text_data_point, strlen(text_data_point) + 1, ENET_PACKET_FLAG_RELIABLE);
		enet_peer_send(m_pServerConnection, 0, packet);
	}

	if (scoreb != NULL) {
		scoreb->Physics()->SetAngularVelocity(Vector3(0, 0.2, 0));
	}
	
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_J)) {         //shoot bullet

		Matrix4 viewMtx = SceneManager::Instance()->GetCamera()->BuildViewMatrix();
		Vector3 viewDir = -Vector3(viewMtx[2], viewMtx[6], viewMtx[10]);

		ostringstream ballshoot;
		ballshoot << "ball" << n;

		Object* baller = CommonUtils::BuildSphereObject(
			ballshoot.str().c_str(),
			SceneManager::Instance()->GetCamera()->GetPosition(),
			0.3f,
			true,
			2.0f,
			true,
			false,
			Vector4(1.f, 0.f, 0.f, 1.f));
		this->AddGameObject(baller);
		baller->Physics()->SetLinearVelocity(viewDir*20.f);
		baller->Physics()->SetRestState(false);
		baller->Physics()->SetBullet(true);

		n++;
	}
		NCLDebug::AddStatusEntry(Vector4(1.f,0.f,0.f,1.0), " Your current score is :%d", currentscore);
		NCLDebug::AddStatusEntry(Vector4(1.f,0.f,0.f,1.0), " Your total score is :%d", totalscore);
		if (scoreb->Physics()->GetCollision() == true && PhysicsEngine::Instance()->GetScoreBoard() > 0) {
			currentscore = PhysicsEngine::Instance()->GetScoreBoard();
			totalscore += currentscore;
		}
}

void EmptyScene::ProcessNetworkEvent(const ENetEvent& evnt)
{
	switch (evnt.type)
	{
		//New connection request or an existing peer accepted our connection request
	case ENET_EVENT_TYPE_CONNECT:
	{
		if (evnt.peer == m_pServerConnection)
		{
			NCLDebug::Log("Network: Successfully connected to server!");

			//Send a 'hello' packet
			char* text_data = "Hellooo!";
			ENetPacket* packet = enet_packet_create(text_data, strlen(text_data) + 1, 0);
			enet_peer_send(m_pServerConnection, 0, packet);
		}
	}
	break;


	//Server has sent us a new packet
	case ENET_EVENT_TYPE_RECEIVE:
	{
		if (evnt.packet->dataLength == sizeof(Vector3))
		{
			Vector3 pos;
			memcpy(&pos, evnt.packet->data, sizeof(Vector3));
			//ballbig->Physics()->SetPosition(pos);
		}
		else
		{
			NCLERROR("Recieved Invalid Network Packet!");
		}

	}
	break;


	//Server has disconnected
	case ENET_EVENT_TYPE_DISCONNECT:
	{
		NCLDebug::Log("Network: Server has disconnected!");
	}
	break;
	}
}