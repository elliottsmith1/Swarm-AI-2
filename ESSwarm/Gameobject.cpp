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
	leader = nullptr;
	delete leader;
}

void GameObject::Tick(float _time, std::vector<GameObject*> gameobjects)
{
	ApplySwarmBehaviour(gameobjects);

	ApplyPhysics(_time);
}

void GameObject::ApplyPhysics(float _time)
{
	if (m_physicsOn)
	{
		XMFLOAT3 temp_pos = m_pos;
		XMFLOAT3 temp_vel = m_vel;
		XMFLOAT3 temp_acc = m_acc;

		temp_acc.x -= temp_vel.x;
		temp_acc.y -= temp_vel.y;
		temp_acc.z -= temp_vel.z;

		temp_acc.x *= _time;
		temp_acc.y *= _time;
		temp_acc.z *= _time;

		temp_vel.x += temp_acc.x;
		temp_vel.y += temp_acc.y;
		temp_vel.z += temp_acc.z;

		XMFLOAT3 newVel = temp_vel;

		temp_vel = m_vel;

		temp_vel.x *= _time;
		temp_vel.y *= _time;
		temp_vel.z *= _time;

		temp_pos.x += temp_vel.x;
		temp_pos.y += temp_vel.y;
		temp_pos.z += temp_vel.z;

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

void GameObject::ApplySwarmBehaviour(std::vector<GameObject*> gameobjects)
{
	CheckNearbyGameobjects(gameobjects);

	//ApplyForce(Separate());

	if (leader)
	{
		ApplyForce(Seek(leader->GetPos()));
	}

	if (is_leader)
	{
		m_pos.y += 0.005f;
		m_pos.x += 0.005f;
	}

	//BoundingBox();
}

void GameObject::ApplyForce(XMFLOAT3 _force)
{
	m_acc.x += _force.x;
	m_acc.y += _force.y;
	m_acc.z += _force.z;
}

void GameObject::CheckNearbyGameobjects(std::vector<GameObject*> gameobjects)
{
	nearby_gameobjects.clear();

	//assign any gameobjetcs near enough to local vector
	for (int i = 0; i < gameobjects.size(); i++)
	{
		XMFLOAT3 temp_pos = gameobjects[i]->GetPos();
		temp_pos.x - m_pos.x;
		temp_pos.y - m_pos.y;
		temp_pos.z - m_pos.z;

		if ((temp_pos.x < nearby_dis) && (temp_pos.y < nearby_dis))
		{
			nearby_gameobjects.push_back(gameobjects[i]);
		}
	}
}

XMFLOAT3 GameObject::Separate()
{
	XMFLOAT3 steer = XMFLOAT3(0, 0, 0);
	int count = 0;

	// For every near boid, check if it's too close
	for (int i = 0; i < nearby_gameobjects.size(); i++)
	{
		XMFLOAT3 temp_pos = nearby_gameobjects[i]->GetPos();
		temp_pos.x - m_pos.x;
		temp_pos.y - m_pos.y;
		temp_pos.z - m_pos.z;

		if ((temp_pos.x < seperation_dis) && (temp_pos.y < seperation_dis))
		{
			// Calculate vector pointing away from neighbor
			XMFLOAT3 diff;
			diff.z = m_pos.z - nearby_gameobjects[i]->GetPos().z;
			diff.y = m_pos.y - nearby_gameobjects[i]->GetPos().y;
			diff.x = m_pos.x - nearby_gameobjects[i]->GetPos().x;

			//diff = XMVector3NormalizeEst(diff);

			//factor in how close to other object
			if (temp_pos.x < temp_pos.y)
			{
				diff.x / temp_pos.x;
				diff.y / temp_pos.x;
				diff.z / temp_pos.x;
			}

			else
			{
				diff.x / temp_pos.y;
				diff.y / temp_pos.y;
				diff.z / temp_pos.y;
			}

			steer.x += diff.x;
			steer.y += diff.y;
			steer.z += diff.z;

			count++;            // Keep track of how many
		}		
	}

	// Average -- divide by how many
	if (count > 0)
	{
		steer.x /= count;
		steer.y /= count;
		steer.z /= count;
	}

	//steer = XMVector3Normalize(steer);

	steer.x *= max_speed;
	steer.y *= max_speed;
	steer.z *= max_speed;

	steer.x -= m_vel.x;
	steer.y -= m_vel.y;
	steer.z -= m_vel.z;

	float max_force = 1.0f;

	//steer = XMVector3ClampLength(steer, 0, max_force);

	return steer;
}

void GameObject::BoundingBox()
{
	int Xmin = 0, Xmax = 100, Ymin = 0, Ymax = 100;

	//if out of bounds then bounce back and reverse direction 
	if (m_pos.x < Xmin)
	{
		m_pos.x += 0.1f * max_speed;
	}

	else if (m_pos.x > Xmax)
	{
		m_pos.x -= 0.1f * max_speed;
	}

	if (m_pos.y < Ymin)
	{
		m_pos.y += 0.1f * max_speed;
	}

	else if (m_pos.y > Ymax)
	{
		m_pos.y -= 0.1f * max_speed;
	}
}

XMFLOAT3 GameObject::Seek(XMFLOAT3 _target)
{
	// A vector pointing from the position to the target
	XMFLOAT3 desired = _target;

	desired.x -= m_pos.x;
	desired.z -= m_pos.z;
	desired.y -= m_pos.y;

	// Scale to maximum speed
	//desired = XMVector3Normalize(desired);

	desired.x *= max_speed;
	desired.y *= max_speed;
	desired.z *= max_speed;

	desired.x - m_vel.x;
	desired.y - m_vel.y;
	desired.z - m_vel.z;

	//desired = XMVector3ClampLengthV(steer, Vector3::Zero, Vector3(m_boidData->maxForce, m_boidData->maxForce, m_boidData->maxForce));

	return desired;
}
