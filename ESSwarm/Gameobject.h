#ifndef _GAME_OBJECT_H
#define _GAME_OBJECT_H

//=================================================================
//Base Game Object Class
//=================================================================

#include <directxmath.h>
#include <vector>
#include <string>

using namespace DirectX;

class Camera;
struct ID3D11DeviceContext;
struct GameData;
struct DrawData;

class GameObject
{
public:
	GameObject();
	virtual ~GameObject();

	void Tick(float _time, std::vector<GameObject*> gameobjects);
	void ApplyPhysics(float _time);
	void ApplySwarmBehaviour(std::vector<GameObject*> gameobjects);
	void ApplyForce(XMFLOAT3 _force);
	void CheckNearbyGameobjects(std::vector<GameObject*> gameobjects);
	XMFLOAT3 Separate();
	void BoundingBox();
	XMFLOAT3 Seek(XMFLOAT3 _target);
	float Distance(XMFLOAT3 pos1, XMFLOAT3 pos2);
	void Init();

	//getters
	XMFLOAT3	GetPos() { return m_pos; }
	XMFLOAT3	GetScale() { return m_scale; }
	XMMATRIX	GetWorldMat() { return m_worldMat; }
	float		GetPitch() { return m_pitch; }
	float		GetYaw() { return m_yaw; }
	float		GetRoll() { return m_roll; }
	bool		IsPhysicsOn() { return m_physicsOn; }
	float		GetDrag() { return m_drag; }
	bool		GetIsLeader() { return is_leader; }

	//setters
	void		SetPos(XMFLOAT3 _pos) { m_pos = _pos; }
	void		SetScale(XMFLOAT3 _scale) { m_scale = _scale; }
	void		SetPitch(float _pitch) { m_pitch = _pitch; }
	void		SetYaw(float _yaw) { m_yaw = _yaw; }
	void		SetRoll(float _roll) { m_roll = _roll; }
	void		SetPitchYawRoll(float _pitch, float _yaw, float _roll) { m_pitch = _pitch; m_yaw = _yaw; m_roll = _roll; }
	void		SetPhysicsOn(bool _physics) { m_physicsOn = _physics; }
	void		TogglePhysics() { m_physicsOn = !m_physicsOn; }
	void		SetDrag(float _drag) { m_drag = _drag; }
	void		SetIsLeader(bool _leader) { is_leader = _leader; }
	void		SetLeader(GameObject* _leader) { leader = _leader; }


protected:

	//World transform/matrix of this GO and it components
	XMMATRIX m_worldMat;
	XMMATRIX m_rotMat;
	XMMATRIX m_fudge;
	XMMATRIX m_scaleMat;
	XMMATRIX m_posMat;

	XMFLOAT3 m_pos;	
	XMFLOAT3 m_scale;

	float m_pitch, m_yaw, m_roll;

	bool m_physicsOn = true;
	float m_drag = 0.01f;
	XMFLOAT3 m_vel = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 m_acc = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 target_pos = XMFLOAT3(0.0f, 0.0f, 0.0f);

	bool is_leader = false;
	std::vector<GameObject*> nearby_gameobjects;
	float seperation_dis = 2.0f;
	float nearby_dis = 4.0f;
	float max_speed = 0.6f;
	float max_force = 5.0f;
	float follow_distance = 5.0f;

	GameObject* leader;
};

#endif