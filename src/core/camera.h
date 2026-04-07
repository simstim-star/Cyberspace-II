#pragma once

#include "DirectXMathC.h"

namespace Sendai {
class Camera {
  public:
	Camera(XMFLOAT3 Position);
	void Update(FLOAT ElapsedSeconds);
	void OnKeyDown(WPARAM Key);
	void OnKeyUp(WPARAM Key);
	void Reset();

	inline XMMATRIX
	ViewMatrix() const
	{
		XMVECTOR EyePosition = XMLoadFloat3(&_Position);
		XMVECTOR EyeDirection = XMLoadFloat3(&_LookDirection);
		XMVECTOR UpDirection = XMLoadFloat3(&_UpDirection);
		return XM_MAT_LOOK_TO_LH(EyePosition, EyeDirection, UpDirection);
	}

	inline XMMATRIX
	ProjectionMatrix(FLOAT FOV, FLOAT AspectRatio, FLOAT NearPlane, FLOAT FarPlane) const
	{
		return XMMatrixPerspectiveFovLH(FOV, AspectRatio, NearPlane, FarPlane);
	}

	inline XMFLOAT3
	GetPosition() const
	{
		return _Position;
	}

  private:
	XMFLOAT3 _InitialPosition;
	XMFLOAT3 _Position;
	FLOAT _Yaw;
	FLOAT _Pitch;
	XMFLOAT3 _LookDirection;
	XMFLOAT3 _UpDirection;
	FLOAT _MoveSpeed; // units per second
	FLOAT _TurnSpeed; // radians per second
	FLOAT _SpeedEnhance;

	struct {
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