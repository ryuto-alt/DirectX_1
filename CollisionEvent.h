#pragma once

#include <DirectXMath.h>

// PhysicsSystemのオブジェクトID
typedef int PhysicsObjectID;

// 衝突イベントデータを格納する構造体
class CollisionEvent {
public:
    CollisionEvent();
    ~CollisionEvent();

    // オブジェクトID設定
    void SetObjectIDs(PhysicsObjectID objA, PhysicsObjectID objB);
    PhysicsObjectID GetObjectA() const;
    PhysicsObjectID GetObjectB() const;

    // 衝突情報
    DirectX::XMFLOAT3 GetContactPoint() const;
    DirectX::XMFLOAT3 GetContactNormal() const;
    float GetImpulse() const;

    // 衝突情報の設定
    void SetContactPoint(const DirectX::XMFLOAT3& point);
    void SetContactNormal(const DirectX::XMFLOAT3& normal);
    void SetImpulse(float impulse);

    // 衝突しているかどうかの判定
    bool IsColliding() const;

private:
    PhysicsObjectID m_objectA;
    PhysicsObjectID m_objectB;
    DirectX::XMFLOAT3 m_contactPoint;
    DirectX::XMFLOAT3 m_contactNormal;
    float m_impulse;
    bool m_isColliding;
};