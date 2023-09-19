#pragma once

#include "Matrix.h"
#include "Vector3.h"

namespace Assets
{
	using namespace Math;	// We need the math classes

	enum class RenderViewCameraUpAxis : uint32_t
	{
		Y = 0,
		Z = 1
	};

	// Represents a 3D viewport camera with various resize routines.
	class RenderViewCamera
	{
	public:
		RenderViewCamera();
		RenderViewCamera(float Theta, float Phi, float Radius, RenderViewCameraUpAxis UpAxis = RenderViewCameraUpAxis::Y);
		~RenderViewCamera() = default;

		// Rotates the camera about a point in from of it, theta is forward and backward, phi is side to side.
		void Rotate(float Theta, float Phi);
		// Move the camera down the look vector.
		void Zoom(float Distance);
		// Moves the camera within it's local X-Y plane.
		void Pan(float X, float Y);

		// Resets the render camera from the given values.
		void Reset(float Theta, float Phi, float Radius);
		// Sets the up axis properly
		void SetUpAxis(RenderViewCameraUpAxis UpAxis);

		// Re-creates the internal projection matrix based on the input.
		void UpdateProjectionMatrix(float Fov, float ClientWidth, float ClientHeight, float NearClip, float FarClip);
		
		// Gets the position of the camera in Cartesian coordinates.
		Vector3 GetCameraPosition() const;
		// Gets the projection matrix
		Matrix& GetProjectionMatrix();
		// Gets the view matrix
		Matrix& GetViewMatrix();
		// Gets the model matrix
		Matrix& GetModelMatrix();

	private:
		// Internal cached flags
		float _Theta;
		float _Phi;
		float _Radius;
		float _Up;

		// Whether or not we need to update the matrix
		bool _MatrixDirty;

		// Calculates a new view matrix
		void UpdateViewMatrix();

		// Internal helper routine to calculate the position relative to _Target.
		Vector3 ToCartesian() const;

		// The target coordinates
		Vector3 _Target;

		// The view, projection, and model matrix
		Matrix _ViewMatrix;
		Matrix _ProjectionMatrix;
		Matrix _ModelMatrix;
	};
}