#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>

// 衝突形状の種類
enum class ColliderType {
    Box,
    Sphere,
    Capsule,
    Mesh
};

// 衝突フィルタカテゴリ
enum class CollisionCategory {
    None = 0,
    Default = 1 << 0,
    Player = 1 << 1,
    Enemy = 1 << 2,
    Projectile = 1 << 3,
    Terrain = 1 << 4,
    Trigger = 1 << 5,
    All = 0xFFFFFFFF
};

// 基本コライダークラス
class Collider {
public:
    Collider();
    virtual ~Collider() = default;

    // コライダーの種類を取得
    virtual ColliderType GetType() const = 0;

    // 位置を設定
    void SetPosition(const DirectX::XMFLOAT3& position);

    // 回転を設定
    void SetRotation(const DirectX::XMFLOAT3& rotation);

    // スケールを設定
    void SetScale(const DirectX::XMFLOAT3& scale);

    // ワールド行列を設定
    void SetWorldMatrix(const DirectX::XMMATRIX& worldMatrix);

    // 位置を取得
    DirectX::XMFLOAT3 GetPosition() const;

    // 回転を取得
    DirectX::XMFLOAT3 GetRotation() const;

    // スケールを取得
    DirectX::XMFLOAT3 GetScale() const;

    // ワールド行列を取得
    DirectX::XMMATRIX GetWorldMatrix() const;

    // 衝突カテゴリを設定
    void SetCategory(CollisionCategory category);

    // 衝突マスクを設定
    void SetMask(CollisionCategory mask);

    // 衝突カテゴリを取得
    CollisionCategory GetCategory() const;

    // 衝突マスクを取得
    CollisionCategory GetMask() const;

    // 衝突判定を有効/無効に設定
    void SetEnabled(bool enabled);

    // 衝突判定が有効かどうか取得
    bool IsEnabled() const;

    // トリガーとして設定（衝突はイベントを発生させるが、物理的な反応はしない）
    void SetTrigger(bool isTrigger);

    // トリガーかどうかを取得
    bool IsTrigger() const;

    // ユーザーデータを設定
    void SetUserData(void* userData);

    // ユーザーデータを取得
    void* GetUserData() const;

    // コライダーIDを取得
    int GetID() const;

protected:
    DirectX::XMFLOAT3 m_position;    // 位置
    DirectX::XMFLOAT3 m_rotation;    // 回転（オイラー角、ラジアン）
    DirectX::XMFLOAT3 m_scale;       // スケール
    DirectX::XMMATRIX m_worldMatrix; // ワールド行列
    CollisionCategory m_category;    // 衝突カテゴリ
    CollisionCategory m_mask;        // 衝突マスク
    bool m_enabled;                  // 有効フラグ
    bool m_isTrigger;                // トリガーフラグ
    void* m_userData;                // ユーザーデータ
    int m_id;                        // コライダーID

    static int s_nextID;             // 次のコライダーID
};

// ボックスコライダー
class BoxCollider : public Collider {
public:
    BoxCollider();
    BoxCollider(const DirectX::XMFLOAT3& halfExtents);
    virtual ~BoxCollider() = default;

    // コライダーの種類を取得
    virtual ColliderType GetType() const override;

    // 半分の幅を設定
    void SetHalfExtents(const DirectX::XMFLOAT3& halfExtents);

    // 半分の幅を取得
    DirectX::XMFLOAT3 GetHalfExtents() const;

private:
    DirectX::XMFLOAT3 m_halfExtents; // 半分の幅
};

// 球コライダー
class SphereCollider : public Collider {
public:
    SphereCollider();
    SphereCollider(float radius);
    virtual ~SphereCollider() = default;

    // コライダーの種類を取得
    virtual ColliderType GetType() const override;

    // 半径を設定
    void SetRadius(float radius);

    // 半径を取得
    float GetRadius() const;

private:
    float m_radius; // 半径
};

// カプセルコライダー
class CapsuleCollider : public Collider {
public:
    CapsuleCollider();
    CapsuleCollider(float radius, float height);
    virtual ~CapsuleCollider() = default;

    // コライダーの種類を取得
    virtual ColliderType GetType() const override;

    // 半径を設定
    void SetRadius(float radius);

    // 高さを設定
    void SetHeight(float height);

    // 半径を取得
    float GetRadius() const;

    // 高さを取得
    float GetHeight() const;

private:
    float m_radius; // 半径
    float m_height; // 高さ
};

// メッシュコライダー
class MeshCollider : public Collider {
public:
    MeshCollider();
    virtual ~MeshCollider() = default;

    // コライダーの種類を取得
    virtual ColliderType GetType() const override;

    // 頂点データを設定
    void SetVertices(const std::vector<DirectX::XMFLOAT3>& vertices);

    // インデックスデータを設定
    void SetIndices(const std::vector<int>& indices);

    // 頂点データを取得
    const std::vector<DirectX::XMFLOAT3>& GetVertices() const;

    // インデックスデータを取得
    const std::vector<int>& GetIndices() const;

private:
    std::vector<DirectX::XMFLOAT3> m_vertices; // 頂点データ
    std::vector<int> m_indices;               // インデックスデータ
};

// 剛体クラス
class Rigidbody {
public:
    Rigidbody();
    ~Rigidbody() = default;

    // 質量を設定
    void SetMass(float mass);

    // 質量を取得
    float GetMass() const;

    // 摩擦係数を設定
    void SetFriction(float friction);

    // 摩擦係数を取得
    float GetFriction() const;

    // 反発係数を設定
    void SetRestitution(float restitution);

    // 反発係数を取得
    float GetRestitution() const;

    // 線形速度を設定
    void SetLinearVelocity(const DirectX::XMFLOAT3& velocity);

    // 線形速度を取得
    DirectX::XMFLOAT3 GetLinearVelocity() const;

    // 角速度を設定
    void SetAngularVelocity(const DirectX::XMFLOAT3& velocity);

    // 角速度を取得
    DirectX::XMFLOAT3 GetAngularVelocity() const;

    // 重力の影響を受けるかどうか設定
    void SetUseGravity(bool useGravity);

    // 重力の影響を受けるかどうか取得
    bool UsesGravity() const;

    // 運動を固定するかどうか設定
    void SetKinematic(bool isKinematic);

    // 運動が固定されているかどうか取得
    bool IsKinematic() const;

    // 力を加える
    void ApplyForce(const DirectX::XMFLOAT3& force);

    // 特定の位置に力を加える
    void ApplyForceAtPosition(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& position);

    // 衝撃を加える
    void ApplyImpulse(const DirectX::XMFLOAT3& impulse);

    // 特定の位置に衝撃を加える
    void ApplyImpulseAtPosition(const DirectX::XMFLOAT3& impulse, const DirectX::XMFLOAT3& position);

    // トルクを加える
    void ApplyTorque(const DirectX::XMFLOAT3& torque);

    // 線形運動を減衰させる係数を設定
    void SetLinearDamping(float damping);

    // 線形運動を減衰させる係数を取得
    float GetLinearDamping() const;

    // 角運動を減衰させる係数を設定
    void SetAngularDamping(float damping);

    // 角運動を減衰させる係数を取得
    float GetAngularDamping() const;

    // コライダーを設定
    void SetCollider(const std::shared_ptr<Collider>& collider);

    // コライダーを取得
    std::shared_ptr<Collider> GetCollider() const;

private:
    float m_mass;                           // 質量
    float m_friction;                       // 摩擦係数
    float m_restitution;                    // 反発係数
    DirectX::XMFLOAT3 m_linearVelocity;     // 線形速度
    DirectX::XMFLOAT3 m_angularVelocity;    // 角速度
    DirectX::XMFLOAT3 m_force;              // 合力
    DirectX::XMFLOAT3 m_torque;             // 合トルク
    float m_linearDamping;                  // 線形減衰係数
    float m_angularDamping;                 // 角減衰係数
    bool m_useGravity;                      // 重力の影響を受けるか
    bool m_isKinematic;                     // 運動を固定するか
    std::shared_ptr<Collider> m_collider;   // コライダー
};

// 衝突情報構造体
struct CollisionInfo {
    std::shared_ptr<Collider> colliderA;          // コライダーA
    std::shared_ptr<Collider> colliderB;          // コライダーB
    DirectX::XMFLOAT3 contactPoint;               // 接触点
    DirectX::XMFLOAT3 normal;                     // 衝突法線
    float penetrationDepth;                       // 貫通深度
};

// 衝突イベントの種類
enum class CollisionEventType {
    Enter,  // 衝突開始
    Stay,   // 衝突中
    Exit    // 衝突終了
};

// 衝突イベント構造体
struct CollisionEvent {
    CollisionEventType type;                      // イベントタイプ
    std::shared_ptr<Collider> colliderA;          // コライダーA
    std::shared_ptr<Collider> colliderB;          // コライダーB
    CollisionInfo info;                           // 衝突情報
};

// 衝突イベントリスナー型定義
using CollisionEventListener = std::function<void(const CollisionEvent&)>;

// 物理システムクラス
class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    // 初期化
    bool Initialize();

    // シャットダウン
    void Shutdown();

    // 更新
    void Update(float deltaTime);

    // 剛体を追加
    void AddRigidbody(const std::shared_ptr<Rigidbody>& rigidbody);

    // 剛体を削除
    void RemoveRigidbody(const std::shared_ptr<Rigidbody>& rigidbody);

    // コライダーを追加
    void AddCollider(const std::shared_ptr<Collider>& collider);

    // コライダーを削除
    void RemoveCollider(const std::shared_ptr<Collider>& collider);

    // 衝突イベントリスナーを追加
    void AddCollisionListener(const std::string& name, CollisionEventListener listener);

    // 衝突イベントリスナーを削除
    void RemoveCollisionListener(const std::string& name);

    // 重力を設定
    void SetGravity(const DirectX::XMFLOAT3& gravity);

    // 重力を取得
    DirectX::XMFLOAT3 GetGravity() const;

    // レイキャストによる衝突判定
    bool Raycast(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction, float maxDistance,
        CollisionInfo& outInfo, CollisionCategory mask = static_cast<CollisionCategory>(0xFFFFFFFF));

private:
    // 衝突検出
    void DetectCollisions();

    // 衝突応答
    void ResolveCollisions();

    // 衝突イベントの処理
    void ProcessCollisionEvents();

    // 衝突判定（球 vs 球）
    bool TestSphereSphere(const SphereCollider& sphereA, const SphereCollider& sphereB, CollisionInfo& outInfo);

    // 衝突判定（球 vs ボックス）
    bool TestSphereBox(const SphereCollider& sphere, const BoxCollider& box, CollisionInfo& outInfo);

    // 衝突判定（ボックス vs ボックス）
    bool TestBoxBox(const BoxCollider& boxA, const BoxCollider& boxB, CollisionInfo& outInfo);

    // 衝突判定（球 vs カプセル）
    bool TestSphereCapsule(const SphereCollider& sphere, const CapsuleCollider& capsule, CollisionInfo& outInfo);

    // 衝突判定（ボックス vs カプセル）
    bool TestBoxCapsule(const BoxCollider& box, const CapsuleCollider& capsule, CollisionInfo& outInfo);

    // 衝突判定（カプセル vs カプセル）
    bool TestCapsuleCapsule(const CapsuleCollider& capsuleA, const CapsuleCollider& capsuleB, CollisionInfo& outInfo);

    // コライダー間の衝突をチェック
    bool CheckCollision(const std::shared_ptr<Collider>& colliderA, const std::shared_ptr<Collider>& colliderB, CollisionInfo& outInfo);

    DirectX::XMFLOAT3 m_gravity;                                 // 重力
    std::vector<std::shared_ptr<Rigidbody>> m_rigidbodies;       // 剛体リスト
    std::vector<std::shared_ptr<Collider>> m_colliders;          // コライダーリスト
    std::unordered_map<std::string, CollisionEventListener> m_collisionListeners; // 衝突イベントリスナー

    // 衝突ペア管理
    struct CollisionPair {
        int colliderIdA;
        int colliderIdB;
        bool operator==(const CollisionPair& other) const {
            return (colliderIdA == other.colliderIdA && colliderIdB == other.colliderIdB) ||
                (colliderIdA == other.colliderIdB && colliderIdB == other.colliderIdA);
        }
    };

    struct CollisionPairHash {
        std::size_t operator()(const CollisionPair& pair) const {
            // コライダーIDのペアに対するハッシュ関数
            return std::hash<int>()(pair.colliderIdA) ^ std::hash<int>()(pair.colliderIdB);
        }
    };

    // 現在の衝突ペアと前のフレームの衝突ペア
    std::unordered_map<CollisionPair, CollisionInfo, CollisionPairHash> m_currentCollisions;
    std::unordered_map<CollisionPair, CollisionInfo, CollisionPairHash> m_previousCollisions;

    // 衝突イベントキュー
    std::vector<CollisionEvent> m_collisionEvents;
};