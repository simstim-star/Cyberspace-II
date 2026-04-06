#include "pch.h"

#include "camera.h"

using Sendai::Camera;

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

void
Camera::Reset()
{
	_Position = _InitialPosition;
	_Yaw = 2 * XM_PI;
	_Pitch = 0.0f;
	_LookDirection = {0, 0, 0};
	_SpeedEnhance = 1.0f;
}

void
Camera::Update(FLOAT ElapsedSeconds)
{
	FLOAT RotateDelta = _TurnSpeed * ElapsedSeconds;

	if (_KeysPressed.LeftArrow) {
		_Yaw -= RotateDelta;
	}
	if (_KeysPressed.RightArrow) {
		_Yaw += RotateDelta;
	}
	if (_KeysPressed.UpArrow) {
		_Pitch += RotateDelta;
	}
	if (_KeysPressed.DownArrow) {
		_Pitch -= RotateDelta;
	}

	_Pitch = min(_Pitch, XM_PIDIV4);
	_Pitch = max(-XM_PIDIV4, _Pitch);

	/*
		Determine the look direction (check notes/xyz_from_yaw_pitch.png)
		 x = cos(pitch) * sin(yaw)
		 y = sin(pitch)
		 z = cos(pitch) * cos(yaw)
	*/
	FLOAT look_b = cosf(_Pitch);
	_LookDirection.x = look_b * sinf(_Yaw);
	_LookDirection.y = sinf(_Pitch);
	_LookDirection.z = look_b * cosf(_Yaw);

	XMVECTOR Forward = XMLoadFloat3(&_LookDirection);
	XMVECTOR Up = XMLoadFloat3(&_UpDirection);
	XMVECTOR Right = XM_VEC3_CROSS(Forward, Up);

	XMVECTOR Movement = XMVectorZero();

	// Process input, which can be [-1,0,1] and will be applied to the direction we want to move
	{
		FLOAT RightInput = 0.f;
		if (_KeysPressed.A) {
			RightInput += 1.0f;
		}
		if (_KeysPressed.D) {
			RightInput -= 1.0f;
		}

		FLOAT ForwardInput = 0.f;
		if (_KeysPressed.W) {
			ForwardInput += 1.0f;
		}
		if (_KeysPressed.S) {
			ForwardInput -= 1.0f;
		}

		XMVECTOR RightScaledByInput = XM_VEC_SCALE(Right, RightInput);
		XMVECTOR ForwardScaledByInput = XM_VEC_SCALE(Forward, ForwardInput);
		Movement = XM_VEC_ADD(Movement, RightScaledByInput);
		Movement = XM_VEC_ADD(Movement, ForwardScaledByInput);
	}

	XMVECTOR Zero = XMVectorZero();
	if (!XM_VEC3_EQ(Movement, Zero)) {
		Movement = XM_VEC3_NORM(Movement);
		Movement = XM_VEC_SCALE(Movement, _SpeedEnhance * _MoveSpeed * ElapsedSeconds);

		XMFLOAT3 Delta;
		XM_STORE_FLOAT3(&Delta, Movement);

		_Position.x += Delta.x;
		_Position.y += Delta.y;
		_Position.z += Delta.z;
	}
}

void
Camera::OnKeyDown(WPARAM Key)
{
	switch (Key) {
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

void
Camera::OnKeyUp(WPARAM Key)
{
	switch (Key) {
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