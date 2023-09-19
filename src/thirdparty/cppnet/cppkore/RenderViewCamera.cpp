#include "stdafx.h"
#include "RenderViewCamera.h"

namespace Assets
{
	RenderViewCamera::RenderViewCamera()
		: _Theta(0), _Phi(0), _Radius(0), _Up(1), _Target(0, 0, 0), _ViewMatrix(), _ProjectionMatrix(), _MatrixDirty(true)
	{
	}

	RenderViewCamera::RenderViewCamera(float Theta, float Phi, float Radius, RenderViewCameraUpAxis UpAxis)
		: _Theta(Theta), _Phi(Phi), _Radius(Radius), _Up(1), _Target(0, 0, 0), _ViewMatrix(), _ProjectionMatrix(), _MatrixDirty(true)
	{
	}

	void RenderViewCamera::Rotate(float Theta, float Phi)
	{
		this->_MatrixDirty = true;

		if (_Up > 0.f)
			_Theta += Theta;
		else
			_Theta -= Theta;

		_Phi += Phi;

		if (_Phi > (float)MathHelper::PI2)
			_Phi -= (float)MathHelper::PI2;
		else if (_Phi < -(float)MathHelper::PI2)
			_Phi += (float)MathHelper::PI2;

		if ((_Phi > 0 && _Phi < (float)MathHelper::PI) || (_Phi < -(float)MathHelper::PI && _Phi > -(float)MathHelper::PI2))
			_Up = 1.f;
		else
			_Up = -1.f;
	}

	void RenderViewCamera::Zoom(float Distance)
	{
		this->_MatrixDirty = true;

		_Radius -= Distance;

		if (_Radius <= 0.f)
		{
			_Radius = 30.f;
			auto Look = (_Target - this->GetCameraPosition()).GetNormalized();
			_Target = (_Target + (Look * 30.f));
		}
	}

	void RenderViewCamera::Pan(float X, float Y)
	{
		this->_MatrixDirty = true;

		auto Look = (_Target - this->GetCameraPosition()).GetNormalized();
		auto WorldUp = Vector3(0, _Up, 0);

		auto Right = Look.Cross(WorldUp);
		auto Up = Look.Cross(Right);

		_Target = (_Target + ((Right * X) + (Up * Y)));
	}

	void RenderViewCamera::Reset(float Theta, float Phi, float Radius)
	{
		_Theta = Theta;
		_Phi = Phi;
		_Radius = Radius;
		_Up = 1;
		_Target = { 0, 0, 0 };
		_ViewMatrix = {};
		_ProjectionMatrix = {};
		_MatrixDirty = true;
	}

	void RenderViewCamera::SetUpAxis(RenderViewCameraUpAxis UpAxis)
	{
		switch (UpAxis)
		{
		case RenderViewCameraUpAxis::Y:
			_ModelMatrix = Matrix();
			break;
		case RenderViewCameraUpAxis::Z:
			_ModelMatrix = Matrix::CreateFromQuaternion(Quaternion::FromEulerAngles(-90, 0, 0));
			break;
		}
		_MatrixDirty = true;
	}

	void RenderViewCamera::UpdateProjectionMatrix(float Fov, float ClientWidth, float ClientHeight, float NearClip, float FarClip)
	{
		this->_ProjectionMatrix = Matrix::CreatePerspectiveFov(Fov, ClientWidth / ClientHeight, NearClip, FarClip);
	}

	Vector3 RenderViewCamera::GetCameraPosition() const
	{
		return (this->_Target + this->ToCartesian());
	}

	Matrix& RenderViewCamera::GetProjectionMatrix()
	{
		return this->_ProjectionMatrix;
	}

	Matrix& RenderViewCamera::GetViewMatrix()
	{
		if (this->_MatrixDirty)
		{
			this->UpdateViewMatrix();
			this->_MatrixDirty = false;
		}

		return this->_ViewMatrix;
	}

	Matrix& RenderViewCamera::GetModelMatrix()
	{
		return this->_ModelMatrix;
	}

	void RenderViewCamera::UpdateViewMatrix()
	{
		this->_ViewMatrix = Matrix::CreateLookAt(GetCameraPosition(), this->_Target, Vector3(0.f, this->_Up, 0.f));
	}

	Vector3 RenderViewCamera::ToCartesian() const
	{
		return Vector3(
			_Radius * sinf(_Phi) * sinf(_Theta),
			_Radius * cosf(_Phi),
			_Radius * sinf(_Phi) * cosf(_Theta)
		);
	}
}
