#pragma once
#include <DirectXMath.h>

class DXCamera {
public:
    DXCamera(float fov, float aspectRatio, float nearPlane, float farPlane);
    ~DXCamera();

    void SetPosition(const DirectX::XMFLOAT3& position);
    void SetTarget(const DirectX::XMFLOAT3& target);
    void SetUp(const DirectX::XMFLOAT3& up);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;

private:
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_target;
    DirectX::XMFLOAT3 m_up;
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
};