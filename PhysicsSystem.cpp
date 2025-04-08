#include "PhysicsSystem.h"
#include <algorithm>
#include <cassert>

// 静的メンバの初期化
int Collider::s_nextID = 0;

//
// Collider クラスの実装
//
Collider::Collider()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_worldMatrix(DirectX::XMMatrixIdentity())
    , m_category(CollisionCategory::Default)
    , m_mask(CollisionCategory::All)
    , m_enabled(true)
    , m_isTrigger(false)
    , m_userData(nullptr)
    , m_id(s_nextID++)
{
}

void Collider::SetPosition(const DirectX::XMFLOAT3& position)
{
    m_position = position;
    // ワールド行列の更新（スケール、回転、移動の順）
    DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
    DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    m_worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
}

void Collider::SetRotation(const DirectX::XMFLOAT3& rotation)
{
    m_rotation = rotation;
    // ワールド行列の更新（スケール、回転、移動の順）
    DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
    DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    m_worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
}

void Collider::SetScale(const DirectX::XMFLOAT3& scale)
{
    m_scale = scale;
    // ワールド行列の更新（スケール、回転、移動の順）
    DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
    DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
    m_worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
}

void Collider::SetWorldMatrix(const DirectX::XMMATRIX& worldMatrix)
{
    m_worldMatrix = worldMatrix;

    // ワールド行列から位置、回転、スケールを抽出
    DirectX::XMVECTOR scale;
    DirectX::XMVECTOR rotation;
    DirectX::XMVECTOR position;

    DirectX::XMMatrixDecompose(&scale, &rotation, &position, worldMatrix);

    DirectX::XMStoreFloat3(&m_position, position);

    // クォータニオンからオイラー角への変換
    DirectX::XMFLOAT4 rotationQuaternion;
    DirectX::XMStoreFloat4(&rotationQuaternion, rotation);

    // クォータニオンからオイラー角を計算
    float xx = rotationQuaternion.x * rotationQuaternion.x;
    float yy = rotationQuaternion.y * rotationQuaternion.y;
    float zz = rotationQuaternion.z * rotationQuaternion.z;
    float xy = rotationQuaternion.x * rotationQuaternion.y;
    float zw = rotationQuaternion.z * rotationQuaternion.w;
    float zx = rotationQuaternion.z * rotationQuaternion.x;
    float yw = rotationQuaternion.y * rotationQuaternion.w;
    float yz = rotationQuaternion.y * rotationQuaternion.z;
    float xw = rotationQuaternion.x * rotationQuaternion.w;

    m_rotation.x = atan2f(2.0f * (yz + xw), 1.0f - 2.0f * (xx + yy));     // Pitch
    m_rotation.y = asinf(2.0f * (xy + zw));                               // Yaw
    m_rotation.z = atan2f(2.0f * (zx + yw), 1.0f - 2.0f * (yy + zz));     // Roll

    DirectX::XMStoreFloat3(&m_scale, scale);
}

DirectX::XMFLOAT3 Collider::GetPosition() const
{
    return m_position;
}

DirectX::XMFLOAT3 Collider::GetRotation() const
{
    return m_rotation;
}

DirectX::XMFLOAT3 Collider::GetScale() const
{
    return m_scale;
}

DirectX::XMMATRIX Collider::GetWorldMatrix() const
{
    return m_worldMatrix;
}

void Collider::SetCategory(CollisionCategory category)
{
    m_category = category;
}

void Collider::SetMask(CollisionCategory mask)
{
    m_mask = mask;
}

CollisionCategory Collider::GetCategory() const
{
    return m_category;
}

CollisionCategory Collider::GetMask() const
{
    return m_mask;
}

void Collider::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool Collider::IsEnabled() const
{
    return m_enabled;
}

void Collider::SetTrigger(bool isTrigger)
{
    m_isTrigger = isTrigger;
}

bool Collider::IsTrigger() const
{
    return m_isTrigger;
}

void Collider::SetUserData(void* userData)
{
    m_userData = userData;
}

void* Collider::GetUserData() const
{
    return m_userData;
}

int Collider::GetID() const
{
    return m_id;
}

//
// BoxCollider クラスの実装
//
BoxCollider::BoxCollider()
    : Collider()
    , m_halfExtents(0.5f, 0.5f, 0.5f)
{
}

BoxCollider::BoxCollider(const DirectX::XMFLOAT3& halfExtents)
    : Collider()
    , m_halfExtents(halfExtents)
{
}

ColliderType BoxCollider::GetType() const
{
    return ColliderType::Box;
}

void BoxCollider::SetHalfExtents(const DirectX::XMFLOAT3& halfExtents)
{
    m_halfExtents = halfExtents;
}

DirectX::XMFLOAT3 BoxCollider::GetHalfExtents() const
{
    return m_halfExtents;
}

//
// SphereCollider クラスの実装
//
SphereCollider::SphereCollider()
    : Collider()
    , m_radius(0.5f)
{
}

SphereCollider::SphereCollider(float radius)
    : Collider()
    , m_radius(radius)
{
}

ColliderType SphereCollider::GetType() const
{
    return ColliderType::Sphere;
}

void SphereCollider::SetRadius(float radius)
{
    m_radius = radius;
}

float SphereCollider::GetRadius() const
{
    return m_radius;
}

//
// CapsuleCollider クラスの実装
//
CapsuleCollider::CapsuleCollider()
    : Collider()
    , m_radius(0.5f)
    , m_height(2.0f)
{
}

CapsuleCollider::CapsuleCollider(float radius, float height)
    : Collider()
    , m_radius(radius)
    , m_height(height)
{
}

ColliderType CapsuleCollider::GetType() const
{
    return ColliderType::Capsule;
}

void CapsuleCollider::SetRadius(float radius)
{
    m_radius = radius;
}

void CapsuleCollider::SetHeight(float height)
{
    m_height = height;
}

float CapsuleCollider::GetRadius() const
{
    return m_radius;
}

float CapsuleCollider::GetHeight() const
{
    return m_height;
}

//
// MeshCollider クラスの実装
//
MeshCollider::MeshCollider()
    : Collider()
{
}

ColliderType MeshCollider::GetType() const
{
    return ColliderType::Mesh;
}

void MeshCollider::SetVertices(const std::vector<DirectX::XMFLOAT3>& vertices)
{
    m_vertices = vertices;
}

void MeshCollider::SetIndices(const std::vector<int>& indices)
{
    m_indices = indices;
}

const std::vector<DirectX::XMFLOAT3>& MeshCollider::GetVertices() const
{
    return m_vertices;
}

const std::vector<int>& MeshCollider::GetIndices() const
{
    return m_indices;
}

//
// Rigidbody クラスの実装
//
Rigidbody::Rigidbody()
    : m_mass(1.0f)
    , m_friction(0.5f)
    , m_restitution(0.5f)
    , m_linearVelocity(0.0f, 0.0f, 0.0f)
    , m_angularVelocity(0.0f, 0.0f, 0.0f)
    , m_force(0.0f, 0.0f, 0.0f)
    , m_torque(0.0f, 0.0f, 0.0f)
    , m_linearDamping(0.01f)
    , m_angularDamping(0.01f)
    , m_useGravity(true)
    , m_isKinematic(false)
    , m_collider(nullptr)
{
}

void Rigidbody::SetMass(float mass)
{
    m_mass = mass;
}

float Rigidbody::GetMass() const
{
    return m_mass;
}

void Rigidbody::SetFriction(float friction)
{
    m_friction = friction;
}

float Rigidbody::GetFriction() const
{
    return m_friction;
}

void Rigidbody::SetRestitution(float restitution)
{
    m_restitution = restitution;
}

float Rigidbody::GetRestitution() const
{
    return m_restitution;
}

void Rigidbody::SetLinearVelocity(const DirectX::XMFLOAT3& velocity)
{
    m_linearVelocity = velocity;
}

DirectX::XMFLOAT3 Rigidbody::GetLinearVelocity() const
{
    return m_linearVelocity;
}

void Rigidbody::SetAngularVelocity(const DirectX::XMFLOAT3& velocity)
{
    m_angularVelocity = velocity;
}

DirectX::XMFLOAT3 Rigidbody::GetAngularVelocity() const
{
    return m_angularVelocity;
}

void Rigidbody::SetUseGravity(bool useGravity)
{
    m_useGravity = useGravity;
}

bool Rigidbody::UsesGravity() const
{
    return m_useGravity;
}

void Rigidbody::SetKinematic(bool isKinematic)
{
    m_isKinematic = isKinematic;
}

bool Rigidbody::IsKinematic() const
{
    return m_isKinematic;
}

void Rigidbody::ApplyForce(const DirectX::XMFLOAT3& force)
{
    m_force.x += force.x;
    m_force.y += force.y;
    m_force.z += force.z;
}

void Rigidbody::ApplyForceAtPosition(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& position)
{
    // まず力を加える
    ApplyForce(force);

    // トルクを計算
    if (m_collider) {
        DirectX::XMFLOAT3 centerOfMass = m_collider->GetPosition();
        DirectX::XMFLOAT3 relativePos = {
            position.x - centerOfMass.x,
            position.y - centerOfMass.y,
            position.z - centerOfMass.z
        };

        // 外積でトルクを計算
        DirectX::XMFLOAT3 torque = {
            relativePos.y * force.z - relativePos.z * force.y,
            relativePos.z * force.x - relativePos.x * force.z,
            relativePos.x * force.y - relativePos.y * force.x
        };

        ApplyTorque(torque);
    }
}

void Rigidbody::ApplyImpulse(const DirectX::XMFLOAT3& impulse)
{
    // 衝撃（impulse）は力（force）を時間で積分したもの
    // 運動量の変化 = 衝撃 = 質量 × 速度の変化
    // よって、速度の変化 = 衝撃 / 質量

    if (m_mass > 0.0f && !m_isKinematic) {
        float invMass = 1.0f / m_mass;
        m_linearVelocity.x += impulse.x * invMass;
        m_linearVelocity.y += impulse.y * invMass;
        m_linearVelocity.z += impulse.z * invMass;
    }
}

void Rigidbody::ApplyImpulseAtPosition(const DirectX::XMFLOAT3& impulse, const DirectX::XMFLOAT3& position)
{
    // まず衝撃を加える
    ApplyImpulse(impulse);

    // 角運動量の変化を計算
    if (m_collider && m_mass > 0.0f && !m_isKinematic) {
        DirectX::XMFLOAT3 centerOfMass = m_collider->GetPosition();
        DirectX::XMFLOAT3 relativePos = {
            position.x - centerOfMass.x,
            position.y - centerOfMass.y,
            position.z - centerOfMass.z
        };

        // 外積で角運動量を計算
        DirectX::XMFLOAT3 angularImpulse = {
            relativePos.y * impulse.z - relativePos.z * impulse.y,
            relativePos.z * impulse.x - relativePos.x * impulse.z,
            relativePos.x * impulse.y - relativePos.y * impulse.x
        };

        // 慣性テンソルの逆行列で角速度の変化を計算（簡略化のため単位行列を使用）
        float invMomentOfInertia = 1.0f / (m_mass * 0.4f); // 大まかな慣性モーメント
        m_angularVelocity.x += angularImpulse.x * invMomentOfInertia;
        m_angularVelocity.y += angularImpulse.y * invMomentOfInertia;
        m_angularVelocity.z += angularImpulse.z * invMomentOfInertia;
    }
}

void Rigidbody::ApplyTorque(const DirectX::XMFLOAT3& torque)
{
    m_torque.x += torque.x;
    m_torque.y += torque.y;
    m_torque.z += torque.z;
}

void Rigidbody::SetLinearDamping(float damping)
{
    m_linearDamping = damping;
}

float Rigidbody::GetLinearDamping() const
{
    return m_linearDamping;
}

void Rigidbody::SetAngularDamping(float damping)
{
    m_angularDamping = damping;
}

float Rigidbody::GetAngularDamping() const
{
    return m_angularDamping;
}

void Rigidbody::SetCollider(const std::shared_ptr<Collider>& collider)
{
    m_collider = collider;
}

std::shared_ptr<Collider> Rigidbody::GetCollider() const
{
    return m_collider;
}

//
// PhysicsSystem クラスの実装
//
PhysicsSystem::PhysicsSystem()
    : m_gravity(0.0f, -9.81f, 0.0f)
{
}

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

bool PhysicsSystem::Initialize()
{
    m_rigidbodies.clear();
    m_colliders.clear();
    m_collisionListeners.clear();
    m_currentCollisions.clear();
    m_previousCollisions.clear();
    m_collisionEvents.clear();

    return true;
}

void PhysicsSystem::Shutdown()
{
    m_rigidbodies.clear();
    m_colliders.clear();
    m_collisionListeners.clear();
    m_currentCollisions.clear();
    m_previousCollisions.clear();
    m_collisionEvents.clear();
}

void PhysicsSystem::Update(float deltaTime)
{
    // 前フレームの衝突情報を保存
    m_previousCollisions = m_currentCollisions;
    m_currentCollisions.clear();
    m_collisionEvents.clear();

    // 剛体の更新
    for (auto& rb : m_rigidbodies) {
        if (rb->IsKinematic() || !rb->GetCollider() || !rb->GetCollider()->IsEnabled()) {
            continue;
        }

        // 重力の適用
        if (rb->UsesGravity()) {
            rb->ApplyForce({
                m_gravity.x * rb->GetMass(),
                m_gravity.y * rb->GetMass(),
                m_gravity.z * rb->GetMass()
                });
        }

        // 速度の更新
        DirectX::XMFLOAT3 velocity = rb->GetLinearVelocity();
        DirectX::XMFLOAT3 force = rb->GetForce();
        float mass = rb->GetMass();

        if (mass > 0.0f) {
            float invMass = 1.0f / mass;

            // 加速度 = 力 / 質量
            velocity.x += force.x * invMass * deltaTime;
            velocity.y += force.y * invMass * deltaTime;
            velocity.z += force.z * invMass * deltaTime;
        }

        // 線形減衰
        float linearDamping = rb->GetLinearDamping();
        float linearDampingFactor = 1.0f - linearDamping * deltaTime;
        if (linearDampingFactor < 0.0f) linearDampingFactor = 0.0f;

        velocity.x *= linearDampingFactor;
        velocity.y *= linearDampingFactor;
        velocity.z *= linearDampingFactor;

        rb->SetLinearVelocity(velocity);

        // 角速度の更新
        DirectX::XMFLOAT3 angularVelocity = rb->GetAngularVelocity();
        DirectX::XMFLOAT3 torque = rb->GetTorque();

        if (mass > 0.0f) {
            float invMomentOfInertia = 1.0f / (mass * 0.4f); // 大まかな慣性モーメント

            // 角加速度 = トルク / 慣性モーメント
            angularVelocity.x += torque.x * invMomentOfInertia * deltaTime;
            angularVelocity.y += torque.y * invMomentOfInertia * deltaTime;
            angularVelocity.z += torque.z * invMomentOfInertia * deltaTime;
        }

        // 角減衰
        float angularDamping = rb->GetAngularDamping();
        float angularDampingFactor = 1.0f - angularDamping * deltaTime;
        if (angularDampingFactor < 0.0f) angularDampingFactor = 0.0f;

        angularVelocity.x *= angularDampingFactor;
        angularVelocity.y *= angularDampingFactor;
        angularVelocity.z *= angularDampingFactor;

        rb->SetAngularVelocity(angularVelocity);

        // 位置の更新
        DirectX::XMFLOAT3 position = rb->GetCollider()->GetPosition();
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        position.z += velocity.z * deltaTime;

        rb->GetCollider()->SetPosition(position);

        // 回転の更新
        DirectX::XMFLOAT3 rotation = rb->GetCollider()->GetRotation();
        rotation.x += angularVelocity.x * deltaTime;
        rotation.y += angularVelocity.y * deltaTime;
        rotation.z += angularVelocity.z * deltaTime;

        rb->GetCollider()->SetRotation(rotation);

        // 力とトルクをリセット
        rb->SetForce({ 0.0f, 0.0f, 0.0f });
        rb->SetTorque({ 0.0f, 0.0f, 0.0f });
    }

    // 衝突検出
    DetectCollisions();

    // 衝突応答
    ResolveCollisions();

    // 衝突イベントの処理
    ProcessCollisionEvents();
}

void PhysicsSystem::AddRigidbody(const std::shared_ptr<Rigidbody>& rigidbody)
{
    if (rigidbody && rigidbody->GetCollider()) {
        m_rigidbodies.push_back(rigidbody);

        // コライダーもリストに追加
        AddCollider(rigidbody->GetCollider());
    }
}

void PhysicsSystem::RemoveRigidbody(const std::shared_ptr<Rigidbody>& rigidbody)
{
    if (rigidbody) {
        auto it = std::find(m_rigidbodies.begin(), m_rigidbodies.end(), rigidbody);
        if (it != m_rigidbodies.end()) {
            // コライダーもリストから削除
            if (rigidbody->GetCollider()) {
                RemoveCollider(rigidbody->GetCollider());
            }

            m_rigidbodies.erase(it);
        }
    }
}

void PhysicsSystem::AddCollider(const std::shared_ptr<Collider>& collider)
{
    if (collider) {
        auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
        if (it == m_colliders.end()) {
            m_colliders.push_back(collider);
        }
    }
}

void PhysicsSystem::RemoveCollider(const std::shared_ptr<Collider>& collider)
{
    if (collider) {
        auto it = std::find(m_colliders.begin(), m_colliders.end(), collider);
        if (it != m_colliders.end()) {
            m_colliders.erase(it);
        }
    }
}

void PhysicsSystem::AddCollisionListener(const std::string& name, CollisionEventListener listener)
{
    m_collisionListeners[name] = listener;
}

void PhysicsSystem::RemoveCollisionListener(const std::string& name)
{
    auto it = m_collisionListeners.find(name);
    if (it != m_collisionListeners.end()) {
        m_collisionListeners.erase(it);
    }
}

void PhysicsSystem::SetGravity(const DirectX::XMFLOAT3& gravity)
{
    m_gravity = gravity;
}

DirectX::XMFLOAT3 PhysicsSystem::GetGravity() const
{
    return m_gravity;
}

bool PhysicsSystem::Raycast(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction, float maxDistance,
    CollisionInfo& outInfo, CollisionCategory mask)
{
    // 方向ベクトルの正規化
    DirectX::XMVECTOR dirVec = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&direction));
    DirectX::XMFLOAT3 dir;
    DirectX::XMStoreFloat3(&dir, dirVec);

    bool hit = false;
    float closestDistance = maxDistance;

    // すべてのコライダーをチェック
    for (const auto& collider : m_colliders) {
        if (!collider->IsEnabled()) {
            continue;
        }

        // 衝突フィルタリング
        if ((static_cast<int>(collider->GetCategory()) & static_cast<int>(mask)) == 0) {
            continue;
        }

        // コライダーの種類に応じたレイキャスト処理
        bool collision = false;
        float distance = maxDistance;
        DirectX::XMFLOAT3 hitPoint = { 0, 0, 0 };
        DirectX::XMFLOAT3 hitNormal = { 0, 0, 0 };

        if (collider->GetType() == ColliderType::Sphere) {
            // 球とレイの交差判定
            auto sphereCollider = std::static_pointer_cast<SphereCollider>(collider);
            DirectX::XMFLOAT3 sphereCenter = sphereCollider->GetPosition();
            float radius = sphereCollider->GetRadius();

            DirectX::XMVECTOR originVec = DirectX::XMLoadFloat3(&origin);
            DirectX::XMVECTOR centerVec = DirectX::XMLoadFloat3(&sphereCenter);

            // 球の中心からレイへの最短距離を計算
            DirectX::XMVECTOR offsetVec = DirectX::XMVectorSubtract(centerVec, originVec);
            float offsetDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(offsetVec, dirVec));

            // レイが球から離れる方向を向いている場合は交差しない
            if (offsetDot < 0) {
                float sqrDist = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(offsetVec));
                if (sqrDist > radius * radius) {
                    continue;
                }
            }

            // 球の中心からレイへの最短距離の二乗を計算
            DirectX::XMVECTOR projVec = DirectX::XMVectorScale(dirVec, offsetDot);
            DirectX::XMVECTOR closestVec = DirectX::XMVectorAdd(originVec, projVec);
            DirectX::XMVECTOR distVec = DirectX::XMVectorSubtract(centerVec, closestVec);
            float sqrDist = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(distVec));

            // 最短距離が半径より大きい場合は交差しない
            if (sqrDist > radius * radius) {
                continue;
            }

            // レイが球を貫通する長さを計算
            float penetration = sqrtf(radius * radius - sqrDist);

            // 交差点までの距離を計算
            distance = offsetDot - penetration;
            if (distance < 0) {
                // レイの始点が球の内部にある場合
                distance = 0;
            }

            if (distance <= maxDistance) {
                // 交差点と法線を計算
                DirectX::XMVECTOR hitPointVec = DirectX::XMVectorAdd(originVec, DirectX::XMVectorScale(dirVec, distance));
                DirectX::XMStoreFloat3(&hitPoint, hitPointVec);

                DirectX::XMVECTOR hitNormalVec = DirectX::XMVectorSubtract(hitPointVec, centerVec);
                hitNormalVec = DirectX::XMVector3Normalize(hitNormalVec);
                DirectX::XMStoreFloat3(&hitNormal, hitNormalVec);

                collision = true;
            }
        }
        else if (collider->GetType() == ColliderType::Box) {
            // ボックスとレイの交差判定
            auto boxCollider = std::static_pointer_cast<BoxCollider>(collider);
            DirectX::XMFLOAT3 boxCenter = boxCollider->GetPosition();
            DirectX::XMFLOAT3 halfExtents = boxCollider->GetHalfExtents();

            // ボックスのローカル座標系にレイを変換
            DirectX::XMMATRIX worldToLocal = DirectX::XMMatrixInverse(nullptr, boxCollider->GetWorldMatrix());

            DirectX::XMVECTOR originVec = DirectX::XMLoadFloat3(&origin);
            originVec = DirectX::XMVector3Transform(originVec, worldToLocal);

            DirectX::XMVECTOR dirVecLocal = DirectX::XMLoadFloat3(&dir);
            dirVecLocal = DirectX::XMVector3TransformNormal(dirVecLocal, worldToLocal);

            DirectX::XMFLOAT3 originLocal, dirLocal;
            DirectX::XMStoreFloat3(&originLocal, originVec);
            DirectX::XMStoreFloat3(&dirLocal, dirVecLocal);

            // スラブ交差法によるボックスとレイの交差判定
            float tmin = -FLT_MAX;
            float tmax = FLT_MAX;

            DirectX::XMFLOAT3 normal = { 0, 0, 0 };
            int hitAxis = -1;

            // 各軸に対してスラブとの交差判定
            for (int i = 0; i < 3; i++) {
                float axisOrigin = i == 0 ? originLocal.x : (i == 1 ? originLocal.y : originLocal.z);
                float axisDir = i == 0 ? dirLocal.x : (i == 1 ? dirLocal.y : dirLocal.z);
                float axisExtent = i == 0 ? halfExtents.x : (i == 1 ? halfExtents.y : halfExtents.z);

                if (fabsf(axisDir) < FLT_EPSILON) {
                    // レイがこの軸と平行な場合
                    if (axisOrigin < -axisExtent || axisOrigin > axisExtent) {
                        // ボックスの外側でレイがボックスと平行なので交差しない
                        collision = false;
                        break;
                    }
                }
                else {
                    // レイがボックスのスラブと交差する時間を計算
                    float invDir = 1.0f / axisDir;
                    float t1 = (-axisExtent - axisOrigin) * invDir;
                    float t2 = (axisExtent - axisOrigin) * invDir;

                    // 交差区間を更新
                    if (t1 > t2) {
                        float temp = t1;
                        t1 = t2;
                        t2 = temp;
                    }

                    if (t1 > tmin) {
                        tmin = t1;
                        hitAxis = i;
                        normal = { 0, 0, 0 };
                        (&normal.x)[i] = axisDir < 0 ? 1.0f : -1.0f;
                    }

                    if (t2 < tmax) {
                        tmax = t2;
                    }

                    if (tmin > tmax || tmax < 0) {
                        // 交差区間が無効または交差点がレイの後ろ側にあるので交差しない
                        collision = false;
                        break;
                    }
                }
            }

            // 交差区間が有効であればレイはボックスと交差する
            if (tmin <= tmax && tmax >= 0) {
                distance = tmin < 0 ? 0 : tmin; // レイの始点がボックス内部にある場合

                if (distance <= maxDistance) {
                    // 交差点を計算
                    DirectX::XMVECTOR hitPointLocalVec = DirectX::XMVectorAdd(originVec, DirectX::XMVectorScale(dirVecLocal, distance));

                    // ワールド座標に戻す
                    DirectX::XMMATRIX localToWorld = boxCollider->GetWorldMatrix();
                    DirectX::XMVECTOR hitPointVec = DirectX::XMVector3Transform(hitPointLocalVec, localToWorld);
                    DirectX::XMStoreFloat3(&hitPoint, hitPointVec);

                    // 法線をワールド座標系に変換
                    DirectX::XMVECTOR normalVec = DirectX::XMLoadFloat3(&normal);
                    normalVec = DirectX::XMVector3TransformNormal(normalVec, localToWorld);
                    normalVec = DirectX::XMVector3Normalize(normalVec);
                    DirectX::XMStoreFloat3(&hitNormal, normalVec);

                    collision = true;
                }
            }
        }

        // 最も近い交差点を保持
        if (collision && distance < closestDistance) {
            closestDistance = distance;
            hit = true;

            outInfo.colliderA = nullptr; // レイにはコライダーがない
            outInfo.colliderB = collider;
            outInfo.contactPoint = hitPoint;
            outInfo.normal = hitNormal;
            outInfo.penetrationDepth = 0.0f; // レイキャストでは貫通深度は使用しない
        }
    }

    return hit;
}

void PhysicsSystem::DetectCollisions()
{
    // すべてのコライダーペアの衝突検出
    for (size_t i = 0; i < m_colliders.size(); i++) {
        auto& colliderA = m_colliders[i];

        if (!colliderA->IsEnabled()) {
            continue;
        }

        for (size_t j = i + 1; j < m_colliders.size(); j++) {
            auto& colliderB = m_colliders[j];

            if (!colliderB->IsEnabled()) {
                continue;
            }

            // 衝突フィルタリング
            int categoryA = static_cast<int>(colliderA->GetCategory());
            int maskA = static_cast<int>(colliderA->GetMask());
            int categoryB = static_cast<int>(colliderB->GetCategory());
            int maskB = static_cast<int>(colliderB->GetMask());

            if ((categoryA & maskB) == 0 || (categoryB & maskA) == 0) {
                continue;
            }

            // 衝突情報
            CollisionInfo info;

            // 衝突判定
            if (CheckCollision(colliderA, colliderB, info)) {
                // 衝突ペアを作成
                CollisionPair pair;
                pair.colliderIdA = colliderA->GetID();
                pair.colliderIdB = colliderB->GetID();

                // 現在の衝突リストに追加
                m_currentCollisions[pair] = info;

                // 衝突イベントの種類を判断
                auto prevIt = m_previousCollisions.find(pair);
                CollisionEventType eventType;

                if (prevIt == m_previousCollisions.end()) {
                    // 前のフレームでは衝突していなかった
                    eventType = CollisionEventType::Enter;
                }
                else {
                    // 前のフレームでも衝突していた
                    eventType = CollisionEventType::Stay;
                }

                // 衝突イベントを追加
                CollisionEvent event;
                event.type = eventType;
                event.colliderA = colliderA;
                event.colliderB = colliderB;
                event.info = info;

                m_collisionEvents.push_back(event);
            }
        }
    }

    // 前のフレームで衝突していたが、現在のフレームでは衝突していないペアを検出し、Exit イベントを生成
    for (const auto& prevPair : m_previousCollisions) {
        if (m_currentCollisions.find(prevPair.first) == m_currentCollisions.end()) {
            // コライダーを検索
            std::shared_ptr<Collider> colliderA = nullptr;
            std::shared_ptr<Collider> colliderB = nullptr;

            for (const auto& collider : m_colliders) {
                if (collider->GetID() == prevPair.first.colliderIdA) {
                    colliderA = collider;
                }
                else if (collider->GetID() == prevPair.first.colliderIdB) {
                    colliderB = collider;
                }

                if (colliderA && colliderB) {
                    break;
                }
            }

            // 両方のコライダーが見つかった場合のみイベントを生成
            if (colliderA && colliderB) {
                CollisionEvent event;
                event.type = CollisionEventType::Exit;
                event.colliderA = colliderA;
                event.colliderB = colliderB;
                event.info = prevPair.second;

                m_collisionEvents.push_back(event);
            }
        }
    }
}

void PhysicsSystem::ResolveCollisions()
{
    // すべての衝突に対して物理応答を適用
    for (const auto& collisionPair : m_currentCollisions) {
        const CollisionInfo& info = collisionPair.second;

        // コライダーを取得
        std::shared_ptr<Collider> colliderA = info.colliderA;
        std::shared_ptr<Collider> colliderB = info.colliderB;

        if (!colliderA || !colliderB) {
            continue;
        }

        // トリガーの場合は物理応答を行わない
        if (colliderA->IsTrigger() || colliderB->IsTrigger()) {
            continue;
        }

        // 関連する剛体を検索
        std::shared_ptr<Rigidbody> rbA = nullptr;
        std::shared_ptr<Rigidbody> rbB = nullptr;

        for (const auto& rb : m_rigidbodies) {
            if (rb->GetCollider() == colliderA) {
                rbA = rb;
            }
            else if (rb->GetCollider() == colliderB) {
                rbB = rb;
            }

            if (rbA && rbB) {
                break;
            }
        }

        // 両方の剛体が見つかった場合のみ衝突応答を行う
        if (rbA && rbB) {
            // どちらも運動が固定されている場合は衝突応答を行わない
            if (rbA->IsKinematic() && rbB->IsKinematic()) {
                continue;
            }

            // 衝突点での相対速度を計算
            DirectX::XMFLOAT3 velA = rbA->GetLinearVelocity();
            DirectX::XMFLOAT3 velB = rbB->GetLinearVelocity();

            DirectX::XMFLOAT3 relativeVelocity = {
                velA.x - velB.x,
                velA.y - velB.y,
                velA.z - velB.z
            };

            // 法線方向の相対速度を計算
            float normalVelocity =
                relativeVelocity.x * info.normal.x +
                relativeVelocity.y * info.normal.y +
                relativeVelocity.z * info.normal.z;

            // 既に離れようとしている場合は衝突応答を行わない
            if (normalVelocity > 0) {
                continue;
            }

            // 反発係数（はね返り）
            float restitution = std::min(rbA->GetRestitution(), rbB->GetRestitution());

            // 摩擦係数
            float friction = std::sqrt(rbA->GetFriction() * rbB->GetFriction());

            // 質量に基づく衝撃力の配分を計算
            float massA = rbA->IsKinematic() ? FLT_MAX : rbA->GetMass();
            float massB = rbB->IsKinematic() ? FLT_MAX : rbB->GetMass();

            float totalInvMass = (massA > 0.0f ? 1.0f / massA : 0.0f) + (massB > 0.0f ? 1.0f / massB : 0.0f);

            // 衝撃力の大きさを計算
            float impulseMagnitude = -(1.0f + restitution) * normalVelocity / totalInvMass;

            // 法線方向の衝撃力
            DirectX::XMFLOAT3 impulse = {
                info.normal.x * impulseMagnitude,
                info.normal.y * impulseMagnitude,
                info.normal.z * impulseMagnitude
            };

            // 衝撃力を適用
            if (!rbA->IsKinematic()) {
                DirectX::XMFLOAT3 impulseA = {
                    impulse.x / massA,
                    impulse.y / massA,
                    impulse.z / massA
                };

                DirectX::XMFLOAT3 newVelA = {
                    velA.x + impulseA.x,
                    velA.y + impulseA.y,
                    velA.z + impulseA.z
                };

                rbA->SetLinearVelocity(newVelA);
            }

            if (!rbB->IsKinematic()) {
                DirectX::XMFLOAT3 impulseB = {
                    -impulse.x / massB,
                    -impulse.y / massB,
                    -impulse.z / massB
                };

                DirectX::XMFLOAT3 newVelB = {
                    velB.x + impulseB.x,
                    velB.y + impulseB.y,
                    velB.z + impulseB.z
                };

                rbB->SetLinearVelocity(newVelB);
            }

            // 位置の補正（めり込み解消）
            float percent = 0.2f; // 補正係数（0～1）
            float slop = 0.01f;   // 許容する貫通深度

            float correction = std::max(info.penetrationDepth - slop, 0.0f) * percent / totalInvMass;

            DirectX::XMFLOAT3 correctionA = {
                info.normal.x * correction * (massA > 0.0f ? 1.0f / massA : 0.0f),
                info.normal.y * correction * (massA > 0.0f ? 1.0f / massA : 0.0f),
                info.normal.z * correction * (massA > 0.0f ? 1.0f / massA : 0.0f)
            };

            DirectX::XMFLOAT3 correctionB = {
                -info.normal.x * correction * (massB > 0.0f ? 1.0f / massB : 0.0f),
                -info.normal.y * correction * (massB > 0.0f ? 1.0f / massB : 0.0f),
                -info.normal.z * correction * (massB > 0.0f ? 1.0f / massB : 0.0f)
            };

            if (!rbA->IsKinematic()) {
                DirectX::XMFLOAT3 posA = colliderA->GetPosition();
                posA.x += correctionA.x;
                posA.y += correctionA.y;
                posA.z += correctionA.z;
                colliderA->SetPosition(posA);
            }

            if (!rbB->IsKinematic()) {
                DirectX::XMFLOAT3 posB = colliderB->GetPosition();
                posB.x += correctionB.x;
                posB.y += correctionB.y;
                posB.z += correctionB.z;
                colliderB->SetPosition(posB);
            }
        }
    }
}

void PhysicsSystem::ProcessCollisionEvents()
{
    // すべての衝突イベントを処理
    for (const auto& event : m_collisionEvents) {
        // すべてのリスナーに通知
        for (const auto& listener : m_collisionListeners) {
            listener.second(event);
        }
    }
}

bool PhysicsSystem::TestSphereSphere(const SphereCollider& sphereA, const SphereCollider& sphereB, CollisionInfo& outInfo)
{
    DirectX::XMFLOAT3 posA = sphereA.GetPosition();
    DirectX::XMFLOAT3 posB = sphereB.GetPosition();

    // 中心間の距離を計算
    DirectX::XMFLOAT3 delta = {
        posB.x - posA.x,
        posB.y - posA.y,
        posB.z - posA.z
    };

    float distanceSquared =
        delta.x * delta.x +
        delta.y * delta.y +
        delta.z * delta.z;

    float radiusSum = sphereA.GetRadius() + sphereB.GetRadius();

    // 中心間の距離が半径の和より小さければ衝突
    if (distanceSquared < radiusSum * radiusSum) {
        float distance = sqrtf(distanceSquared);

        // 法線を計算（中心から中心への方向）
        DirectX::XMFLOAT3 normal;
        if (distance > 0.0001f) {
            normal.x = delta.x / distance;
            normal.y = delta.y / distance;
            normal.z = delta.z / distance;
        }
        else {
            // 中心がほぼ同じ位置にある場合、Yアップとする
            normal = { 0.0f, 1.0f, 0.0f };
        }

        // 接触点と貫通深度を計算
        float penetrationDepth = radiusSum - distance;

        DirectX::XMFLOAT3 contactPoint = {
            posA.x + normal.x * sphereA.GetRadius(),
            posA.y + normal.y * sphereA.GetRadius(),
            posA.z + normal.z * sphereA.GetRadius()
        };

        // 衝突情報を設定
        outInfo.colliderA = std::make_shared<SphereCollider>(sphereA);
        outInfo.colliderB = std::make_shared<SphereCollider>(sphereB);
        outInfo.contactPoint = contactPoint;
        outInfo.normal = normal;
        outInfo.penetrationDepth = penetrationDepth;

        return true;
    }

    return false;
}

bool PhysicsSystem::TestSphereBox(const SphereCollider& sphere, const BoxCollider& box, CollisionInfo& outInfo)
{
    // 球の中心をボックスのローカル座標系に変換
    DirectX::XMFLOAT3 sphereCenter = sphere.GetPosition();
    DirectX::XMFLOAT3 boxCenter = box.GetPosition();
    DirectX::XMFLOAT3 boxHalfExtents = box.GetHalfExtents();

    // ボックスのワールド行列の逆行列を計算
    DirectX::XMMATRIX worldToLocal = DirectX::XMMatrixInverse(nullptr, box.GetWorldMatrix());

    // 球の中心をボックスのローカル座標系に変換
    DirectX::XMVECTOR sphereCenterVec = DirectX::XMLoadFloat3(&sphereCenter);
    sphereCenterVec = DirectX::XMVector3Transform(sphereCenterVec, worldToLocal);

    DirectX::XMFLOAT3 sphereCenterLocal;
    DirectX::XMStoreFloat3(&sphereCenterLocal, sphereCenterVec);

    // ボックスの最も近い点を計算
    DirectX::XMFLOAT3 closestPoint = {
        std::max(boxCenter.x - boxHalfExtents.x, std::min(sphereCenterLocal.x, boxCenter.x + boxHalfExtents.x)),
        std::max(boxCenter.y - boxHalfExtents.y, std::min(sphereCenterLocal.y, boxCenter.y + boxHalfExtents.y)),
        std::max(boxCenter.z - boxHalfExtents.z, std::min(sphereCenterLocal.z, boxCenter.z + boxHalfExtents.z))
    };

    // 最も近い点と球の中心の距離を計算
    DirectX::XMFLOAT3 delta = {
        closestPoint.x - sphereCenterLocal.x,
        closestPoint.y - sphereCenterLocal.y,
        closestPoint.z - sphereCenterLocal.z
    };

    float distanceSquared =
        delta.x * delta.x +
        delta.y * delta.y +
        delta.z * delta.z;

    float radius = sphere.GetRadius();

    // 距離が半径より小さければ衝突
    if (distanceSquared < radius * radius) {
        float distance = sqrtf(distanceSquared);

        // 法線を計算
        DirectX::XMFLOAT3 normalLocal;
        if (distance > 0.0001f) {
            normalLocal.x = -delta.x / distance;
            normalLocal.y = -delta.y / distance;
            normalLocal.z = -delta.z / distance;
        }
        else {
            // 最も近い点が球の中心とほぼ同じ位置にある場合
            // 球の中心がボックスのどの面に最も近いかを判断
            float minDistance = FLT_MAX;
            int axis = -1;
            int sign = 0;

            for (int i = 0; i < 3; i++) {
                float axisCenter = i == 0 ? sphereCenterLocal.x : (i == 1 ? sphereCenterLocal.y : sphereCenterLocal.z);
                float axisExtent = i == 0 ? boxHalfExtents.x : (i == 1 ? boxHalfExtents.y : boxHalfExtents.z);

                float distToPositiveFace = axisExtent - axisCenter;
                float distToNegativeFace = axisExtent + axisCenter;

                if (distToPositiveFace < minDistance) {
                    minDistance = distToPositiveFace;
                    axis = i;
                    sign = 1;
                }

                if (distToNegativeFace < minDistance) {
                    minDistance = distToNegativeFace;
                    axis = i;
                    sign = -1;
                }
            }

            normalLocal = { 0.0f, 0.0f, 0.0f };
            (&normalLocal.x)[axis] = static_cast<float>(sign);
        }

        // 法線をワールド座標系に変換
        DirectX::XMVECTOR normalLocalVec = DirectX::XMLoadFloat3(&normalLocal);
        DirectX::XMMATRIX localToWorld = box.GetWorldMatrix();
        normalLocalVec = DirectX::XMVector3TransformNormal(normalLocalVec, localToWorld);
        normalLocalVec = DirectX::XMVector3Normalize(normalLocalVec);

        DirectX::XMFLOAT3 normal;
        DirectX::XMStoreFloat3(&normal, normalLocalVec);

        // 接触点を計算
        DirectX::XMFLOAT3 contactPoint = {
            sphereCenter.x - normal.x * radius,
            sphereCenter.y - normal.y * radius,
            sphereCenter.z - normal.z * radius
        };

        // 貫通深度を計算
        float penetrationDepth = radius - distance;

        // 衝突情報を設定
        outInfo.colliderA = std::make_shared<SphereCollider>(sphere);
        outInfo.colliderB = std::make_shared<BoxCollider>(box);
        outInfo.contactPoint = contactPoint;
        outInfo.normal = normal;
        outInfo.penetrationDepth = penetrationDepth;

        return true;
    }

    return false;
}

bool PhysicsSystem::TestBoxBox(const BoxCollider& boxA, const BoxCollider& boxB, CollisionInfo& outInfo)
{
    // 分離軸定理を使用してボックス同士の衝突を判定
    // 実装が複雑になるため、ここでは簡略化されたバージョンとして仮実装
    // 実際のゲームエンジンでは、分離軸定理の完全な実装やGJKアルゴリズムを使用します

    // テスト用：中心間の距離でおおよその判定を行う
    DirectX::XMFLOAT3 posA = boxA.GetPosition();
    DirectX::XMFLOAT3 posB = boxB.GetPosition();
    DirectX::XMFLOAT3 extA = boxA.GetHalfExtents();
    DirectX::XMFLOAT3 extB = boxB.GetHalfExtents();

    DirectX::XMFLOAT3 delta = {
        posB.x - posA.x,
        posB.y - posA.y,
        posB.z - posA.z
    };

    // 各軸での距離と許容距離を計算
    float absX = fabsf(delta.x);
    float absY = fabsf(delta.y);
    float absZ = fabsf(delta.z);

    float allowedX = extA.x + extB.x;
    float allowedY = extA.y + extB.y;
    float allowedZ = extA.z + extB.z;

    // 各軸で許容距離内かチェック
    if (absX <= allowedX && absY <= allowedY && absZ <= allowedZ) {
        // 最も浅い貫通軸を見つける
        float overlapX = allowedX - absX;
        float overlapY = allowedY - absY;
        float overlapZ = allowedZ - absZ;

        DirectX::XMFLOAT3 normal;
        float penetrationDepth;

        if (overlapX < overlapY && overlapX < overlapZ) {
            // X軸での貫通が最小
            normal = { delta.x > 0 ? -1.0f : 1.0f, 0.0f, 0.0f };
            penetrationDepth = overlapX;
        }
        else if (overlapY < overlapZ) {
            // Y軸での貫通が最小
            normal = { 0.0f, delta.y > 0 ? -1.0f : 1.0f, 0.0f };
            penetrationDepth = overlapY;
        }
        else {
            // Z軸での貫通が最小
            normal = { 0.0f, 0.0f, delta.z > 0 ? -1.0f : 1.0f };
            penetrationDepth = overlapZ;
        }

        // 接触点を計算（簡略化：ボックスAの中心から法線方向にextA分移動した点）
        DirectX::XMFLOAT3 contactPoint = {
            posA.x + normal.x * extA.x,
            posA.y + normal.y * extA.y,
            posA.z + normal.z * extA.z
        };

        // 衝突情報を設定
        outInfo.colliderA = std::make_shared<BoxCollider>(boxA);
        outInfo.colliderB = std::make_shared<BoxCollider>(boxB);
        outInfo.contactPoint = contactPoint;
        outInfo.normal = normal;
        outInfo.penetrationDepth = penetrationDepth;

        return true;
    }

    return false;
}

bool PhysicsSystem::TestSphereCapsule(const SphereCollider& sphere, const CapsuleCollider& capsule, CollisionInfo& outInfo)
{
    // 球とカプセルの衝突判定
    DirectX::XMFLOAT3 sphereCenter = sphere.GetPosition();
    DirectX::XMFLOAT3 capsuleCenter = capsule.GetPosition();
    float sphereRadius = sphere.GetRadius();
    float capsuleRadius = capsule.GetRadius();
    float capsuleHalfHeight = capsule.GetHeight() * 0.5f;

    // カプセルの端点（ローカル座標系）
    DirectX::XMFLOAT3 capsulePointA = { 0, -capsuleHalfHeight, 0 };
    DirectX::XMFLOAT3 capsulePointB = { 0, capsuleHalfHeight, 0 };

    // カプセルのワールド行列
    DirectX::XMMATRIX capsuleWorld = capsule.GetWorldMatrix();

    // 端点をワールド座標系に変換
    DirectX::XMVECTOR pointAVec = DirectX::XMLoadFloat3(&capsulePointA);
    DirectX::XMVECTOR pointBVec = DirectX::XMLoadFloat3(&capsulePointB);

    pointAVec = DirectX::XMVector3Transform(pointAVec, capsuleWorld);
    pointBVec = DirectX::XMVector3Transform(pointBVec, capsuleWorld);

    DirectX::XMFLOAT3 worldPointA, worldPointB;
    DirectX::XMStoreFloat3(&worldPointA, pointAVec);
    DirectX::XMStoreFloat3(&worldPointB, pointBVec);

    // 球の中心から線分への最短距離を計算
    DirectX::XMFLOAT3 lineVec = {
        worldPointB.x - worldPointA.x,
        worldPointB.y - worldPointA.y,
        worldPointB.z - worldPointA.z
    };

    DirectX::XMFLOAT3 sphereToCapsuleA = {
        sphereCenter.x - worldPointA.x,
        sphereCenter.y - worldPointA.y,
        sphereCenter.z - worldPointA.z
    };

    // 線分の長さの二乗
    float lineLength2 =
        lineVec.x * lineVec.x +
        lineVec.y * lineVec.y +
        lineVec.z * lineVec.z;

    // 線分上でのパラメータ t（0～1）
    float t = (
        sphereToCapsuleA.x * lineVec.x +
        sphereToCapsuleA.y * lineVec.y +
        sphereToCapsuleA.z * lineVec.z
        ) / lineLength2;

    t = std::max(0.0f, std::min(1.0f, t));

    // 線分上の最近点
    DirectX::XMFLOAT3 closestPoint = {
        worldPointA.x + lineVec.x * t,
        worldPointA.y + lineVec.y * t,
        worldPointA.z + lineVec.z * t
    };

    // 最近点から球の中心への距離
    DirectX::XMFLOAT3 delta = {
        sphereCenter.x - closestPoint.x,
        sphereCenter.y - closestPoint.y,
        sphereCenter.z - closestPoint.z
    };

    float distanceSquared =
        delta.x * delta.x +
        delta.y * delta.y +
        delta.z * delta.z;

    float radiusSum = sphereRadius + capsuleRadius;

    // 距離が半径の和より小さければ衝突
    if (distanceSquared < radiusSum * radiusSum) {
        float distance = sqrtf(distanceSquared);

        // 法線を計算
        DirectX::XMFLOAT3 normal;
        if (distance > 0.0001f) {
            normal.x = delta.x / distance;
            normal.y = delta.y / distance;
            normal.z = delta.z / distance;
        }
        else {
            // 最近点が球の中心とほぼ同じ位置にある場合、Y軸方向とする
            normal = { 0.0f, 1.0f, 0.0f };
        }

        // 接触点を計算
        DirectX::XMFLOAT3 contactPoint = {
            sphereCenter.x - normal.x * sphereRadius,
            sphereCenter.y - normal.y * sphereRadius,
            sphereCenter.z - normal.z * sphereRadius
        };

        // 貫通深度を計算
        float penetrationDepth = radiusSum - distance;

        // 衝突情報を設定
        outInfo.colliderA = std::make_shared<SphereCollider>(sphere);
        outInfo.colliderB = std::make_shared<CapsuleCollider>(capsule);
        outInfo.contactPoint = contactPoint;
        outInfo.normal = normal;
        outInfo.penetrationDepth = penetrationDepth;

        return true;
    }

    return false;
}

bool PhysicsSystem::TestBoxCapsule(const BoxCollider& box, const CapsuleCollider& capsule, CollisionInfo& outInfo)
{
    // ボックスとカプセルの衝突判定は複雑なため、ここでは簡易的な実装を提供します
    // 実際のゲームエンジンでは、GJKアルゴリズムなどを使用します

    // テスト用：カプセルを球で近似して判定
    SphereCollider approxSphere;
    approxSphere.SetPosition(capsule.GetPosition());
    approxSphere.SetRadius(capsule.GetRadius() + capsule.GetHeight() * 0.5f);

    return TestSphereBox(approxSphere, box, outInfo);
}

bool PhysicsSystem::TestCapsuleCapsule(const CapsuleCollider& capsuleA, const CapsuleCollider& capsuleB, CollisionInfo& outInfo)
{
    // カプセル同士の衝突判定も複雑なため、ここでは簡易的な実装を提供します

    // テスト用：両方のカプセルを球で近似して判定
    SphereCollider sphereA;
    sphereA.SetPosition(capsuleA.GetPosition());
    sphereA.SetRadius(capsuleA.GetRadius() + capsuleA.GetHeight() * 0.5f);

    SphereCollider sphereB;
    sphereB.SetPosition(capsuleB.GetPosition());
    sphereB.SetRadius(capsuleB.GetRadius() + capsuleB.GetHeight() * 0.5f);

    return TestSphereSphere(sphereA, sphereB, outInfo);
}

bool PhysicsSystem::CheckCollision(const std::shared_ptr<Collider>& colliderA, const std::shared_ptr<Collider>& colliderB, CollisionInfo& outInfo)
{
    // コライダーの種類によって適切な衝突判定関数を呼び出す
    ColliderType typeA = colliderA->GetType();
    ColliderType typeB = colliderB->GetType();

    outInfo.colliderA = colliderA;
    outInfo.colliderB = colliderB;

    // 球 vs 球
    if (typeA == ColliderType::Sphere && typeB == ColliderType::Sphere) {
        return TestSphereSphere(
            *static_cast<SphereCollider*>(colliderA.get()),
            *static_cast<SphereCollider*>(colliderB.get()),
            outInfo
        );
    }

    // 球 vs ボックス
    if (typeA == ColliderType::Sphere && typeB == ColliderType::Box) {
        return TestSphereBox(
            *static_cast<SphereCollider*>(colliderA.get()),
            *static_cast<BoxCollider*>(colliderB.get()),
            outInfo
        );
    }

    // ボックス vs 球
    if (typeA == ColliderType::Box && typeB == ColliderType::Sphere) {
        bool result = TestSphereBox(
            *static_cast<SphereCollider*>(colliderB.get()),
            *static_cast<BoxCollider*>(colliderA.get()),
            outInfo
        );

        // 法線の向きを逆にする
        if (result) {
            outInfo.normal.x *= -1;
            outInfo.normal.y *= -1;
            outInfo.normal.z *= -1;

            // コライダーの順序を入れ替え
            std::swap(outInfo.colliderA, outInfo.colliderB);
        }

        return result;
    }

    // ボックス vs ボックス
    if (typeA == ColliderType::Box && typeB == ColliderType::Box) {
        return TestBoxBox(
            *static_cast<BoxCollider*>(colliderA.get()),
            *static_cast<BoxCollider*>(colliderB.get()),
            outInfo
        );
    }

    // 球 vs カプセル
    if (typeA == ColliderType::Sphere && typeB == ColliderType::Capsule) {
        return TestSphereCapsule(
            *static_cast<SphereCollider*>(colliderA.get()),
            *static_cast<CapsuleCollider*>(colliderB.get()),
            outInfo
        );
    }

    // カプセル vs 球
    if (typeA == ColliderType::Capsule && typeB == ColliderType::Sphere) {
        bool result = TestSphereCapsule(
            *static_cast<SphereCollider*>(colliderB.get()),
            *static_cast<CapsuleCollider*>(colliderA.get()),
            outInfo
        );

        // 法線の向きを逆にする
        if (result) {
            outInfo.normal.x *= -1;
            outInfo.normal.y *= -1;
            outInfo.normal.z *= -1;

            // コライダーの順序を入れ替え
            std::swap(outInfo.colliderA, outInfo.colliderB);
        }

        return result;
    }

    // ボックス vs カプセル
    if (typeA == ColliderType::Box && typeB == ColliderType::Capsule) {
        return TestBoxCapsule(
            *static_cast<BoxCollider*>(colliderA.get()),
            *static_cast<CapsuleCollider*>(colliderB.get()),
            outInfo
        );
    }

    // カプセル vs ボックス
    if (typeA == ColliderType::Capsule && typeB == ColliderType::Box) {
        bool result = TestBoxCapsule(
            *static_cast<BoxCollider*>(colliderB.get()),
            *static_cast<CapsuleCollider*>(colliderA.get()),
            outInfo
        );

        // 法線の向きを逆にする
        if (result) {
            outInfo.normal.x *= -1;
            outInfo.normal.y *= -1;
            outInfo.normal.z *= -1;

            // コライダーの順序を入れ替え
            std::swap(outInfo.colliderA, outInfo.colliderB);
        }

        return result;
    }

    // カプセル vs カプセル
    if (typeA == ColliderType::Capsule && typeB == ColliderType::Capsule) {
        return TestCapsuleCapsule(
            *static_cast<CapsuleCollider*>(colliderA.get()),
            *static_cast<CapsuleCollider*>(colliderB.get()),
            outInfo
        );
    }

    // メッシュコライダーの判定はここでは実装しません

    return false;
}