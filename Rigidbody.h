#pragma once

#include <DirectXMath.h>

class Rigidbody {
public:
    Rigidbody();
    ~Rigidbody();

    // 初期化と更新
    void Initialize();
    void Update(float deltaTime);

    // 力と回転に関するメソッド（エラーで指摘されていたもの）
    void SetForce(const DirectX::XMFLOAT3& force);
    void SetTorque(const DirectX::XMFLOAT3& torque);
    DirectX::XMFLOAT3 GetForce() const;
    DirectX::XMFLOAT3 GetTorque() const;

    // 位置と回転の設定・取得
    void SetPosition(const DirectX::XMFLOAT3& position);
    void SetRotation(const DirectX::XMFLOAT4& rotation);
    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT4 GetRotation() const;

    // 物理特性の設定
    void SetMass(float mass);
    void SetGravity(bool useGravity);
    void SetKinematic(bool isKinematic);

private:
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT4 m_rotation;
    DirectX::XMFLOAT3 m_velocity;
    DirectX::XMFLOAT3 m_angularVelocity;
    DirectX::XMFLOAT3 m_force;
    DirectX::XMFLOAT3 m_torque;
    float m_mass;
    bool m_useGravity;
    bool m_isKinematic;
};