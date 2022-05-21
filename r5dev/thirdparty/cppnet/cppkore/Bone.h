#pragma once

#include <cstdint>
#include "StringBase.h"
#include "MathHelper.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "BoneFlags.h"

namespace Assets
{
	using namespace Math;	// All of the matrices classes are in Math::*

	// A container class that holds 3D bone data.
	class Bone
	{
	public:
		// Initialize a blank 3D bone.
		Bone();
		// Initialize a 3D bone with it's tag name.
		Bone(const string& Name);
		// Initialize a 3D bone with it's tag name, and parent index.
		Bone(const string& Name, int32_t ParentIndex);
		// Initialize a 3D bone with it's tag name, parent index, and transposition matrix.
		Bone(const string& Name, int32_t ParentIndex, Vector3 Position, Quaternion Rotation, BoneFlags Flags = BoneFlags::HasLocalSpaceMatrices);
		// Initialize a 3D bone with it's tag name, parent index, transposition matrix, and scale transform.
		Bone(const string& Name, int32_t ParentIndex, Vector3 Position, Quaternion Rotation, Vector3 Scale, BoneFlags Flags = (BoneFlags::HasLocalSpaceMatrices | BoneFlags::HasScale));
		// Initialize a 3D bone with it's tag name, parent index, local and global transposition matrix, and scale transform.
		Bone(const string& Name, int32_t ParentIndex, Vector3 LocalPosition, Quaternion LocalRotation, Vector3 GlobalPosition, Quaternion GlobalRotation, Vector3 Scale, BoneFlags Flags = (BoneFlags::HasGlobalSpaceMatrices | BoneFlags::HasLocalSpaceMatrices | BoneFlags::HasScale));
		// Destroy all 3D bone resources.
		~Bone() = default;

		// Ensure that our bone is not copied or assigned to for performance reasons.
		Bone(const Bone&) = delete;

		// Gets bone specific flags.
		bool GetFlag(BoneFlags Flag);
		// Sets bone specific flags.
		void SetFlag(BoneFlags Flags, bool Value);

		// Gets the tag name assigned to the bone.
		const string& Name() const;
		// Sets the tag name assigned to the bone.
		void SetName(const string& Value);

		// Gets the parent bone index.
		const int32_t& Parent() const;
		// Sets the parent bone index.
		void SetParent(int32_t Value);

		// Gets the local space position.
		const Vector3& LocalPosition() const;
		// Sets the local space position.
		void SetLocalPosition(Vector3 Value);

		// Gets the local space rotation.
		const Quaternion& LocalRotation() const;
		// Sets the local space rotation.
		void SetLocalRotation(Quaternion Value);

		// Gets the global space position.
		const Vector3& GlobalPosition() const;
		// Sets the global space position.
		void SetGlobalPosition(Vector3 Value);

		// Gets the global space rotation.
		const Quaternion& GlobalRotation() const;
		// Sets the global space rotation.
		void SetGlobalRotation(Quaternion Value);

		// Gets the scale transform.
		const Vector3& Scale() const;
		// Sets the scale transform.
		void SetScale(Vector3 Value);

	private:
		// Internal tag name
		string _Name;

		// Internal parent index
		int32_t _Parent;

		// Internal flags for this bone
		BoneFlags _Flags;

		// Internal 3D matrix information for this bone
		Vector3 _LocalSpacePosition;
		Vector3 _GlobalSpacePosition;
		Quaternion _LocalSpaceRotation;
		Quaternion _GlobalSpaceRotation;
		Vector3 _Scale;
	};
}