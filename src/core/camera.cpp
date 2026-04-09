#include "pch.h"

#include "camera.h"
#include <algorithm>

using Sendai::Camera;
using namespace DirectX;

Camera::Camera(XMFLOAT3 Position)
{
    _InitialPosition = Position;
    _Position = Position;
    _Yaw = 2 * XM_PI;
    _Pitch = 0.0f;
    _LookDirection = {0, 0, 0};
    _UpDirection = {0, 1, 0};
    _MoveSpeed = {5.0f};
    _TurnSpeed = XM_PIDIV4;
    _KeysPressed = {};
    _SpeedEnhance = 1.0f;
}

VOID Camera::Reset()
{
    _Position = _InitialPosition;
    _Yaw = 2 * XM_PI;
    _Pitch = 0.0f;
    _LookDirection = {0, 0, 0};
    _SpeedEnhance = 1.0f;
}

void Camera::Update(const FLOAT ElapsedSeconds)
{
    const FLOAT RotateDelta = _TurnSpeed * ElapsedSeconds;

    if (_KeysPressed.LeftArrow)
    {
        _Yaw -= RotateDelta;
    }
    if (_KeysPressed.RightArrow)
    {
        _Yaw += RotateDelta;
    }
    if (_KeysPressed.UpArrow)
    {
        _Pitch += RotateDelta;
    }
    if (_KeysPressed.DownArrow)
    {
        _Pitch -= RotateDelta;
    }

    _Pitch = std::clamp(_Pitch, -DirectX::XM_PIDIV4, DirectX::XM_PIDIV4);

    const FLOAT CosPitch = cosf(_Pitch);
    const FLOAT SinPitch = sinf(_Pitch);
    const FLOAT CosYaw = cosf(_Yaw);
    const FLOAT SinYaw = sinf(_Yaw);

    _LookDirection.x = CosPitch * SinYaw;
    _LookDirection.y = SinPitch;
    _LookDirection.z = CosPitch * CosYaw;

    const XMVECTOR Forward = XMLoadFloat3(&_LookDirection);
    const XMVECTOR Up = XMLoadFloat3(&_UpDirection);
    const XMVECTOR Right = XMVector3Cross(Up, Forward);

    FLOAT RightInput = (_KeysPressed.A ? -1.0f : 0.0f) + (_KeysPressed.D ? 1.0f : 0.0f);
    FLOAT ForwardInput = (_KeysPressed.W ? 1.0f : 0.0f) + (_KeysPressed.S ? -1.0f : 0.0f);

    XMVECTOR Movement = XMVectorScale(Right, RightInput) + XMVectorScale(Forward, ForwardInput);

    if (!XMVector3Equal(Movement, XMVectorZero()))
    {
        Movement = XMVector3Normalize(Movement);

        const FLOAT Magnitude = _SpeedEnhance * _MoveSpeed * ElapsedSeconds;
        const XMVECTOR Velocity = Movement * Magnitude;
        const XMVECTOR CurrentPos = XMLoadFloat3(&_Position);
        XMStoreFloat3(&_Position, CurrentPos + Velocity);
    }
}

VOID Camera::OnKeyDown(WPARAM Key)
{
    switch (Key)
    {
    case 'W':
        _KeysPressed.W = TRUE;
        break;
    case 'A':
        _KeysPressed.A = TRUE;
        break;
    case 'S':
        _KeysPressed.S = TRUE;
        break;
    case 'D':
        _KeysPressed.D = TRUE;
        break;
    case VK_LEFT:
        _KeysPressed.LeftArrow = TRUE;
        break;
    case VK_RIGHT:
        _KeysPressed.RightArrow = TRUE;
        break;
    case VK_UP:
        _KeysPressed.UpArrow = TRUE;
        break;
    case VK_DOWN:
        _KeysPressed.DownArrow = TRUE;
        break;
    case VK_ESCAPE:
        Reset();
        break;
    case VK_SHIFT:
        _SpeedEnhance = 3.0f;
        break;
    }
}

VOID Camera::OnKeyUp(WPARAM Key)
{
    switch (Key)
    {
    case 'W':
        _KeysPressed.W = FALSE;
        break;
    case 'A':
        _KeysPressed.A = FALSE;
        break;
    case 'S':
        _KeysPressed.S = FALSE;
        break;
    case 'D':
        _KeysPressed.D = FALSE;
        break;
    case VK_LEFT:
        _KeysPressed.LeftArrow = FALSE;
        break;
    case VK_RIGHT:
        _KeysPressed.RightArrow = FALSE;
        break;
    case VK_UP:
        _KeysPressed.UpArrow = FALSE;
        break;
    case VK_DOWN:
        _KeysPressed.DownArrow = FALSE;
        break;
    case VK_SHIFT:
        _SpeedEnhance = 1.0f;
        break;
    }
}