#include "Camera.h"

namespace DirectxPlayground
{

Camera::Camera(float fovY, float aspect, float nearZ, float farZ)
    : mDirty(false)
{
    XMMATRIX view = XMMatrixIdentity();
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    XMMATRIX vp = view * proj;
    XMStoreFloat4x4(&mProjection, proj);
    XMStoreFloat4x4(&mView, view);
    XMStoreFloat4x4(&mToWorld, view);
    XMStoreFloat4x4(&mViewProjection, vp);
}

void Camera::SetWorldPosition(const XMFLOAT3& pos, bool updateViewAndVP /* = true*/)
{
    mToWorld(3, 0) = pos.x;
    mToWorld(3, 1) = pos.y;
    mToWorld(3, 2) = pos.z;
    if (updateViewAndVP)
    {
        XMMATRIX cameraToWorld = XMLoadFloat4x4(&mToWorld);
        XMVECTOR det = XMMatrixDeterminant(cameraToWorld);
        XMMATRIX newView = XMMatrixInverse(&det, cameraToWorld);
        XMFLOAT4X4 newViewStored;
        XMStoreFloat4x4(&newViewStored, newView);
        SetView(newViewStored, mToWorld, true);
    }
}

}