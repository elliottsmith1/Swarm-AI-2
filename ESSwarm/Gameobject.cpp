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

	follow_distance = (rand() % 50) + 25;

	m_worldMat = XMMatrixIdentity();
	m_fudge = XMMatrixIdentity();
}

GameObject::~GameObject()
{
	leader = nullptr;
	delete leader;
}

void GameObject::Init()
{
	
}

void GameObject::Tick(float _time, std::vector<GameObject*> gameobjects)
{
	ApplySwarmBehaviour(gameobjects);

	ApplyPhysics(_time);
}

void GameObject::ApplyPhysics(float _time)
{
	m_vel.x += m_acc.x * _time;
	m_vel.y += m_acc.y * _time;

	XMVECTOR temp_vel = XMLoadFloat3(&m_vel);
	temp_vel = XMVector3NormalizeEst(temp_vel);
	temp_vel = XMVector3ClampLength(temp_vel, 0.0f, max_speed);
	XMStoreFloat3(&m_vel, temp_vel);

	m_pos.x += m_vel.x;
	m_pos.y += m_vel.y;	

	m_scaleMat = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
	m_rotMat = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);
	m_posMat = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

	m_worldMat = m_fudge * m_scaleMat * m_rotMat * m_posMat;

	m_vel.x *= m_drag;
	m_vel.y *= m_drag;

	m_acc = XMFLOAT3(0.0f, 0.0f, 0.0f);	
}

void GameObject::ApplySwarmBehaviour(std::vector<GameObject*> gameobjects)
{
	CheckNearbyGameobjects(gameobjects);	

	if (leader)
	{
		target_pos = leader->GetPos();
	}

	float distance = Distance(m_pos, target_pos);	

	if (distance > follow_distance)
	{
		ApplyForce(Seek(target_pos));
	}

	else
	{
		if (is_leader)
		{
			target_pos.x = (rand() % 600) + 1;
			target_pos.y = (rand() % 600) + 1;
		}
	}

	if (!is_leader)
	{
		ApplyForce(Separate());
	}

	else 
	{
		if (follow_distance != 5.0f)
		{
			follow_distance = 5.0f;

			max_speed = 0.7f;
		}
	}

	BoundingBox();
}

void GameObject::ApplyForce(XMFLOAT3 _force)
{
	m_acc.x += _force.x;
	m_acc.y += _force.y;
	m_acc.z += _force.z;

	XMVECTOR temp_acc = XMLoadFloat3(&m_acc);
	temp_acc = XMVector3ClampLength(temp_acc, 0, max_speed);
	XMStoreFloat3(&m_acc, temp_acc);
}

void GameObject::CheckNearbyGameobjects(std::vector<GameObject*> gameobjects)
{
	nearby_gameobjects.clear();

	//assign any gameobjetcs near enough to local vector
	for (int i = 0; i < gameobjects.size(); i++)
	{
		float distance = Distance(m_pos, gameobjects[i]->GetPos());

		if ((distance < nearby_dis) && (distance > 0))
		{
			nearby_gameobjects.push_back(gameobjects[i]);
		}
	}
}

XMFLOAT3 GameObject::Separate()
{
	XMFLOAT3 steer = XMFLOAT3(0, 0, 0);
	float count = 0;

	// For every near boid, check if it's too close
	for (int i = 0; i < nearby_gameobjects.size(); i++)
	{
		float distance = Distance(m_pos, nearby_gameobjects[i]->GetPos());

		if ((distance < seperation_dis) && (distance > 0))
		{
			// Calculate vector pointing away from neighbor
			XMVECTOR pos_vec = XMLoadFloat3(&m_pos);
			XMVECTOR target_pos = XMLoadFloat3(&nearby_gameobjects[i]->GetPos());
			XMVECTOR distance_vec = XMVectorSubtract(pos_vec, target_pos);
			XMFLOAT3 diff = XMFLOAT3(0.0f, 0.0f, 0.0f);

			//vector_sub = XMVector3NormalizeEst(vector_sub);
			XMStoreFloat3(&diff, distance_vec);

			//factor in how close to other object
			diff.x /= distance;
			diff.y /= distance;
			diff.z /= distance;

			//apply movement
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

	XMVECTOR temp_steer = XMLoadFloat3(&steer);
	//temp_steer = XMVector3NormalizeEst(temp_steer);
	XMStoreFloat3(&steer, temp_steer);

	steer.x *= max_speed;
	steer.y *= max_speed;
	steer.z *= max_speed;

	steer.x -= m_vel.x;
	steer.y -= m_vel.y;
	steer.z -= m_vel.z;

	temp_steer = XMLoadFloat3(&steer);
	temp_steer = XMVector3ClampLength(temp_steer, 0, max_force);
	XMStoreFloat3(&steer, temp_steer);

	steer.x *= 10.0f;
	steer.y *= 10.0f;

	return steer;
}

void GameObject::BoundingBox()
{
	float Xmin = -2000.0f, Xmax = 2000.0f, Ymin = -2000.0f, Ymax = 2000.0f, force = 10.0f;
	XMFLOAT3 temp_pos = m_pos;
	bool apply_force = false;

	//if out of bounds then bounce back and reverse direction 
	if (m_pos.x < Xmin)
	{	
		//temp_pos.x += force;
		temp_pos.x *= -1.0f;

		apply_force = true;
	}

	if (m_pos.x > Xmax)
	{
		//temp_pos.x -= force;
		temp_pos.x *= -1.0f;

		apply_force = true;
	}

	if (m_pos.y < Ymin)
	{
		//temp_pos.y += force;
		temp_pos.y *= -1.0f;

		apply_force = true;
	}

	if (m_pos.y > Ymax)
	{
		//temp_pos.y -= force;
		temp_pos.y *= -1.0f;

		apply_force = true;
	}

	if (apply_force)
	{
		ApplyForce(temp_pos);
	}
}

XMFLOAT3 GameObject::Seek(XMFLOAT3 _target)
{
	// A vector pointing from the position to the target

	XMVECTOR temp_des = XMLoadFloat3(&_target);
	XMVECTOR temp_pos = XMLoadFloat3(&m_pos);
	XMVECTOR temp_vel = XMLoadFloat3(&m_vel);

	XMFLOAT3 speed = XMFLOAT3(max_speed, max_speed, max_speed);
	XMVECTOR speed_vec = XMLoadFloat3(&speed);

	temp_des = XMVectorSubtract(temp_des, temp_pos);

	temp_des = XMVector3NormalizeEst(temp_des);

	temp_des = XMVectorMultiply(temp_des, speed_vec);

	temp_des = XMVectorSubtract(temp_des, temp_vel);

	temp_des = XMVector3ClampLength(temp_des, 0, max_force);

	XMFLOAT3 desired = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMStoreFloat3(&desired, temp_des);

	return desired;
}

float GameObject::Distance(XMFLOAT3 pos1, XMFLOAT3 pos2)
{
	float distance = 0.0f;
	XMFLOAT3 v = XMFLOAT3(0.0f, 0.0f, 0.0f);

	v.x = pos1.x - pos2.x;
	v.y = pos1.y - pos2.y;
	v.z = pos1.z - pos2.z;
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}


