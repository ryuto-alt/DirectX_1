#include "CollisionEvent.h"

using namespace DirectX;

CollisionEvent::CollisionEvent()
    : m_objectA(-1)
    , m_objectB(-1)
    , m_contactPoint(0.0f, 0.0f, 0.0f)
    , m_contactNormal(0.0f, 0.0f, 0.0f)
    , m_impulse(0.0f)
    , m_isColliding(false)
{
}

CollisionEvent::~CollisionEvent()
{
}

void CollisionEvent::SetObjectIDs(PhysicsObjectID objA, PhysicsObjectID objB)
{
    m_objectA = objA;
    m_objectB = objB;
    m_isColliding = true;
}

PhysicsObjectID CollisionEvent::GetObjectA() const
{
    return m_objectA;
}

PhysicsObjectID CollisionEvent::GetObjectB() const
{
    return m_objectB;
}

XMFLOAT3 CollisionEvent::GetContactPoint() const
{
    return m_contactPoint;
}

XMFLOAT3 CollisionEvent::GetContactNormal() const
{
    return m_contactNormal;
}

float CollisionEvent::GetImpulse() const
{
    return m_impulse;
}

void CollisionEvent::SetContactPoint(const XMFLOAT3& point)
{
    m_contactPoint = point;
}

void CollisionEvent::SetContactNormal(const XMFLOAT3& normal)
{
    m_contactNormal = normal;
}

void CollisionEvent::SetImpulse(float impulse)
{
    m_impulse = impulse;
}

bool CollisionEvent::IsColliding() const
{
    return m_isColliding;
}