#pragma once

#include "DirectXMath.h"

namespace Sendai
{
class Camera
{
  public:
    Camera(DirectX::XMFLOAT3 Position);
    VOID Update(FLOAT ElapsedSeconds);
    VOID OnKeyDown(WPARAM Key);
    VOID OnKeyUp(WPARAM Key);
    VOID Reset();

    inline DirectX::XMMATRIX GetViewMatrix() const
    {
        const DirectX::XMVECTOR EyePos = DirectX::XMLoadFloat3(&_Position);
        const DirectX::XMVECTOR EyeDir = DirectX::XMLoadFloat3(&_LookDirection);
        const DirectX::XMVECTOR UpDir = DirectX::XMLoadFloat3(&_UpDirection);

        return DirectX::XMMatrixLookToLH(EyePos, EyeDir, UpDir);
    }

    inline DirectX::XMMATRIX GetProjectionMatrix(float fovAngleY, float aspectRatio, float nearZ, float farZ) const
    {
        return DirectX::XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
    }

    inline DirectX::XMFLOAT3 GetPosition() const
    {
        return _Position;
    }

  private:
    DirectX::XMFLOAT3 _InitialPosition;
    DirectX::XMFLOAT3 _Position;
    FLOAT _Yaw;
    FLOAT _Pitch;
    DirectX::XMFLOAT3 _LookDirection;
    DirectX::XMFLOAT3 _UpDirection;
    FLOAT _MoveSpeed; // units per second
    FLOAT _TurnSpeed; // radians per second
    FLOAT _SpeedEnhance;

    struct
    {
        BOOL W;
        BOOL A;
        BOOL S;
        BOOL D;

        BOOL LeftArrow;
        BOOL RightArrow;
        BOOL UpArrow;
        BOOL DownArrow;
    } _KeysPressed;
};
} // namespace Sendai