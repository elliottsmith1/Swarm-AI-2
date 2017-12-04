#include "Gameobject.h"
#include "GameData.h"

GameObject::GameObject()
{
	//set the Gameobject to the origin with no rotation and unit scaling 
	m_pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pitch = 0.0f;
	m_yaw = 0.0f;
	m_roll = 0.0f;
	m_scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	m_worldMat = XMMatrixIdentity();
	m_fudge = XMMatrixIdentity();
}

GameObject::~GameObject()
{

}

void GameObject::Tick(float _time)
{
	XMFLOAT3 temp_pos = m_pos;

	temp_pos.x += 0.005f;
	temp_pos.y += 0.005f;

	m_pos = temp_pos;

	if (m_physicsOn)
	{			
		XMFLOAT3 temp_vel = m_vel;
		XMFLOAT3 temp_acc = m_acc;

		temp_vel.x * m_drag;
		temp_vel.y * m_drag;
		temp_vel.z * m_drag;

		temp_acc.x - temp_vel.x;
		temp_acc.y - temp_vel.y;
		temp_acc.z - temp_vel.z;

		temp_acc.x * _time;
		temp_acc.y * _time;
		temp_acc.z * _time;

		temp_vel.x + temp_acc.x;
		temp_vel.y + temp_acc.y;
		temp_vel.z + temp_acc.z;

		XMFLOAT3 newVel = temp_vel;		

		temp_vel = m_vel;
		XMFLOAT3 temp_pos = m_pos;

		temp_vel.x + temp_pos.x;
		temp_vel.y + temp_pos.y;
		temp_vel.z + temp_pos.z;

		XMFLOAT3 newPos = temp_pos;

		m_vel = newVel;
		m_pos = newPos;
	}

	m_scaleMat = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	m_rotMat = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	m_posMat = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

	m_worldMat = m_fudge * m_scaleMat * m_rotMat * m_posMat;

	m_acc = XMFLOAT3(0.0f, 0.0f, 0.0f);
}