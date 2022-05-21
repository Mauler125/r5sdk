#include "stdafx.h"
#include "Bone.h"

namespace Assets
{
	Bone::Bone()
		: _Parent(-1), _Flags(BoneFlags::HasLocalSpaceMatrices), _LocalSpacePosition(0, 0, 0), _LocalSpaceRotation(0, 0, 0, 1), _GlobalSpacePosition(0, 0, 0), _GlobalSpaceRotation(0, 0, 0, 1), _Scale(1, 1, 1)
	{
	}

	Bone::Bone(const string& Name)
		: Bone(Name, -1, { 0, 0, 0 }, { 0, 0, 0, 1 }, { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 }, BoneFlags::HasLocalSpaceMatrices)
	{
	}

	Bone::Bone(const string& Name, int32_t ParentIndex)
		: Bone(Name, ParentIndex, { 0, 0, 0 }, { 0, 0, 0, 1 }, { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 }, BoneFlags::HasLocalSpaceMatrices)
	{
	}

	Bone::Bone(const string& Name, int32_t ParentIndex, Vector3 Position, Quaternion Rotation, BoneFlags Flags)
		: Bone(Name, ParentIndex)
	{
		if (((int)Flags & (int)BoneFlags::HasLocalSpaceMatrices) == (int)BoneFlags::HasLocalSpaceMatrices)
		{
			this->_LocalSpacePosition = Position;
			this->_LocalSpaceRotation = Rotation;
		}
		else
		{
			this->_GlobalSpacePosition = Position;
			this->_GlobalSpaceRotation = Rotation;
		}

		this->_Flags = Flags;
	}

	Bone::Bone(const string & Name, int32_t ParentIndex, Vector3 Position, Quaternion Rotation, Vector3 Scale, BoneFlags Flags)
		: Bone(Name, ParentIndex)
	{
		if (((int)Flags & (int)BoneFlags::HasLocalSpaceMatrices) == (int)BoneFlags::HasLocalSpaceMatrices)
		{
			this->_LocalSpacePosition = Position;
			this->_LocalSpaceRotation = Rotation;
		}
		else
		{
			this->_GlobalSpacePosition = Position;
			this->_GlobalSpaceRotation = Rotation;
		}

		this->_Scale = Scale;
		this->_Flags = Flags;
	}

	Bone::Bone(const string& Name, int32_t ParentIndex, Vector3 LocalPosition, Quaternion LocalRotation, Vector3 GlobalPosition, Quaternion GlobalRotation, Vector3 Scale, BoneFlags Flags)
		: _Name(Name), _Parent(ParentIndex), _LocalSpacePosition(LocalPosition), _LocalSpaceRotation(LocalRotation), _GlobalSpacePosition(GlobalPosition), _GlobalSpaceRotation(GlobalRotation), _Scale(Scale), _Flags(Flags)
	{
	}

	bool Bone::GetFlag(BoneFlags Flag)
	{
		return ((int)this->_Flags & (int)Flag) == (int)Flag;
	}

	void Bone::SetFlag(BoneFlags Flags, bool Value)
	{
		this->_Flags = Value ? (BoneFlags)((int)this->_Flags | (int)Flags) : (BoneFlags)((int)this->_Flags & ~(int)Flags);
	}

	const string& Bone::Name() const
	{
		return this->_Name;
	}

	void Bone::SetName(const string& Value)
	{
		this->_Name = Value;
	}

	const int32_t& Bone::Parent() const
	{
		return this->_Parent;
	}

	void Bone::SetParent(int32_t Value)
	{
		this->_Parent = Value;
	}

	const Vector3& Bone::LocalPosition() const
	{
		return this->_LocalSpacePosition;
	}

	void Bone::SetLocalPosition(Vector3 Value)
	{
		this->_LocalSpacePosition = Value;
	}

	const Quaternion& Bone::LocalRotation() const
	{
		return this->_LocalSpaceRotation;
	}

	void Bone::SetLocalRotation(Quaternion Value)
	{
		this->_LocalSpaceRotation = Value;
	}

	const Vector3& Bone::GlobalPosition() const
	{
		return this->_GlobalSpacePosition;
	}

	void Bone::SetGlobalPosition(Vector3 Value)
	{
		this->_GlobalSpacePosition = Value;
	}

	const Quaternion& Bone::GlobalRotation() const
	{
		return this->_GlobalSpaceRotation;
	}

	void Bone::SetGlobalRotation(Quaternion Value)
	{
		this->_GlobalSpaceRotation = Value;
	}

	const Vector3& Bone::Scale() const
	{
		return this->_Scale;
	}

	void Bone::SetScale(Vector3 Value)
	{
		this->_Scale = Value;
	}
}
