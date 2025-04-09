#include "DXCamera.h"

using namespace DirectX;

DXCamera::DXCamera(float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_fov(fov), m_aspectRatio(aspectRatio), m_nearPlane(nearPlane), m_farPlane(farPlane),
    m_position(0.0f, 0.0f, -5.0f), m_target(0.0f, 0.0f, 0.0f), m_up(0.0f, 1.0f, 0.0f)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

DXCamera::~DXCamera()
{
    // Nothing to clean up
}

void DXCamera::SetPosition(const XMFLOAT3& position)
{
    m_position = position;
    UpdateViewMatrix();
}

void DXCamera::SetTarget(const XMFLOAT3& target)
{
    m_target = target;
    UpdateViewMatrix();
}

void DXCamera::SetUp(const XMFLOAT3& up)
{
    m_up = up;
    UpdateViewMatrix();
}

XMMATRIX DXCamera::GetViewMatrix() const
{
    return m_viewMatrix;
}

XMMATRIX DXCamera::GetProjectionMatrix() const
{
    return m_projectionMatrix;
}

void DXCamera::UpdateViewMatrix()
{
    m_viewMatrix = XMMatrixLookAtLH(
        XMLoadFloat3(&m_position),
        XMLoadFloat3(&m_target),
        XMLoadFloat3(&m_up)
    );
}

void DXCamera::UpdateProjectionMatrix()
{
    m_projectionMatrix = XMMatrixPerspectiveFovLH(
        m_fov,
        m_aspectRatio,
        m_nearPlane,
        m_farPlane
    );
}