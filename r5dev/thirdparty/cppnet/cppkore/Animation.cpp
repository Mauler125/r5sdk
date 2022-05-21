#include "stdafx.h"
#include "Animation.h"

namespace Assets
{
	Animation::Animation()
		: Animation(0)
	{
	}

	Animation::Animation(uint32_t BoneCount)
		: Animation(BoneCount, 30.0f)
	{
	}

	Animation::Animation(uint32_t BoneCount, float FrameRate)
		: Bones(BoneCount), Looping(false), FrameRate(FrameRate), Name("error"), TransformSpace(AnimationTransformSpace::Local), RotationInterpolation(AnimationRotationInterpolation::Quaternion)
	{
	}

	List<Curve>& Animation::GetNodeCurves(const string& NodeName)
	{
		if (Curves.ContainsKey(NodeName))
			return Curves[NodeName];

		Curves.Add(NodeName, List<Curve>());
		return Curves[NodeName];
	}

	void Animation::AddNotification(const string& Name, uint32_t Frame)
	{
		if (Notificiations.ContainsKey(Name))
			Notificiations[Name].EmplaceBack(Frame);

		Notificiations.Add(Name, List<uint32_t>());
		Notificiations[Name].EmplaceBack(Frame);
	}

	const uint32_t Animation::FrameCount(bool Legacy) const
	{
		uint32_t Result = 0;

		for (auto& Kvp : Curves)
		{
			for (auto& Curve : Kvp.Value())
			{
				if (Legacy)
				{
					// Used for animation types which do not support non-bone like curves
					// So we only have quaternion rotation, translations, and scales
					if (Curve.Property == CurveProperty::RotateQuaternion || (Curve.Property >= CurveProperty::TranslateX && Curve.Property <= CurveProperty::TranslateZ) || (Curve.Property >= CurveProperty::ScaleX && Curve.Property <= CurveProperty::ScaleZ))
					{
						for (auto& Keyframe : Curve.Keyframes)
							Result = max(Result, Keyframe.Frame.Integer32);
					}
				}
				else
				{
					// Support all curves
					for (auto& Keyframe : Curve.Keyframes)
						Result = max(Result, Keyframe.Frame.Integer32);
				}
			}
		}

		for (auto& NoteTrack : Notificiations)
			for (auto& Note : NoteTrack.Value())
				Result = max(Result, Note);

		// Frame count is the length of the animation in frames
		// Frames start at index 0, so we add one to get the count
		return Result + 1;
	}

	const uint32_t Animation::NotificationCount() const
	{
		uint32_t Result = 0;

		for (auto& NoteTrack : Notificiations)
			Result += NoteTrack.Value().Count();

		return Result;
	}

	void Animation::Scale(float Factor)
	{
		for (auto& Kvp : Curves)
		{
			for (auto& Curve : Kvp.Value())
			{
				// Translation keyframes are scaled here...
				if (Curve.Property >= CurveProperty::TranslateX && Curve.Property <= CurveProperty::TranslateZ)
				{
					for (auto& Keyframe : Curve.Keyframes)
					{
						Keyframe.Value.Float *= Factor;
					}
				}
			}
		}

		for (auto& Bone : Bones)
		{
			if (Bone.GetFlag(BoneFlags::HasLocalSpaceMatrices))
			{
				Bone.SetLocalPosition(Bone.LocalPosition() * Factor);
			}
			if (Bone.GetFlag(BoneFlags::HasGlobalSpaceMatrices))
			{
				Bone.SetGlobalPosition(Bone.GlobalPosition() * Factor);
			}
		}
	}

	void Animation::RemoveEmptyNodes()
	{
		for (auto& Kvp : Curves)
		{
			for (int32_t i = ((int32_t)Kvp.Value().Count() - 1); i >= 0; i--)
			{
				if (Kvp.Value()[i].Keyframes.Count() == 0)
				{
					Kvp.Value().RemoveAt(i);
				}
			}
		}
	}
}
