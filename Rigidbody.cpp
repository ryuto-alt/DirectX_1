#include "Rigidbody.h"

using namespace DirectX;

Rigidbody::Rigidbody()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f, 1.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
    , m_angularVelocity(0.0f, 0.0f, 0.0f)
    , m_force(0.0f, 0.0f, 0.0f)
    , m_torque(0.0f, 0.0f, 0.0f)
    , m_mass(1.0f)
    , m_useGravity(true)
    , m_isKinematic(false)
{
}

Rigidbody::~Rigidbody()
{
}

void Rigidbody::Initialize()
{
    // 物理特性の初期化
    m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_rotation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    m_velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_angularVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_force = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_torque = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void Rigidbody::Update(float deltaTime)
{
    if (m_isKinematic)
        return;

    // 重力を適用
    if (m_useGravity) {
        m_force.y -= 9.8f * m_mass * deltaTime;
    }

    // 速度の更新 (F = ma, a = F/m, v = v + a*dt)
    float inverseMass = 1.0f / m_mass;
    m_velocity.x += m_force.x * inverseMass * deltaTime;
    m_velocity.y += m_force.y * inverseMass * deltaTime;
    m_velocity.z += m_force.z * inverseMass * deltaTime;

    // 角速度の更新 (単純化)
    m_angularVelocity.x += m_torque.x * inverseMass * deltaTime;
    m_angularVelocity.y += m_torque.y * inverseMass * deltaTime;
    m_angularVelocity.z += m_torque.z * inverseMass * deltaTime;

    // 位置の更新
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;
    m_position.z += m_velocity.z * deltaTime;

    // 回転の更新 (単純化)
    // 実際の物理演算では四元数を使って正確に計算する必要があります
    // ここでは簡略化のため省略しています

    // 力と回転力をリセット（1フレームのみ適用）
    m_force = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_torque = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

void Rigidbody::SetForce(const XMFLOAT3& force)
{
    m_force = force;
}

void Rigidbody::SetTorque(const XMFLOAT3& torque)
{
    m_torque = torque;
}

XMFLOAT3 Rigidbody::GetForce() const
{
    return m_force;
}

XMFLOAT3 Rigidbody::GetTorque() const
{
    return m_torque;
}

void Rigidbody::SetPosition(const XMFLOAT3& position)
{
    m_position = position;
}

void Rigidbody::SetRotation(const XMFLOAT4& rotation)
{
    m_rotation = rotation;
}

XMFLOAT3 Rigidbody::GetPosition() const
{
    return m_position;
}

XMFLOAT4 Rigidbody::GetRotation() const
{
    return m_rotation;
}

void Rigidbody::SetMass(float mass)
{
    if (mass <= 0.0f)
        mass = 0.001f; // 質量が0以下にならないようにする
    m_mass = mass;
}

void Rigidbody::SetGravity(bool useGravity)
{
    m_useGravity = useGravity;
}

void Rigidbody::SetKinematic(bool isKinematic)
{
    m_isKinematic = isKinematic;
}