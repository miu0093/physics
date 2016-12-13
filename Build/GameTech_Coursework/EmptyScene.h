#pragma once
#include <ncltech\NetworkBase.h>
#include <nclgl\Mesh.h>
#include <ncltech\Scene.h>
#include <ncltech\CommonUtils.h>
#include <nclgl\OBJMesh.h>
#include <sstream>



class EmptyScene : public Scene
{
public:
	EmptyScene(const std::string& friendly_name);
	virtual ~EmptyScene();
	int n = 0;
	virtual void OnInitializeScene() override;
	float m_AccumTime;
	virtual void OnUpdateScene(float dt) override;
	virtual void OnCleanupScene() override;
	int    currentscore = 0;
	int    totalscore = 0;
	OBJMesh* m_MeshPlayer;
	void ProcessNetworkEvent(const ENetEvent& evnt);

protected:
	Object* ballbig ;//planet
	Object* ballsmall;//bullet
	Object* ball3;//nothing
	Object* scoreb;//scoreboard
	bool  m_servergetpoint;
	NetworkBase m_Network;
	ENetPeer*	m_pServerConnection;
};
