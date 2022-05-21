#pragma once

#include <memory>
#include <cstdint>
#include "ListBase.h"
#include "DictionaryBase.h"

// The separate anim parts
#include "Bone.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "AnimationTypes.h"
#include "Curve.h"

namespace Assets
{
	// A container class that holds 3D animation data.
	class Animation
	{
	public:
		// Initialize a blank 3D animation without any known counts.
		Animation();
		// Initialize a 3D animation with the given count for bones.
		Animation(uint32_t BoneCount);
		// Initialize a 3D animation with the given bone count and framerate.
		Animation(uint32_t BoneCount, float FrameRate);

		// The name of the animation
		string Name;

		// A collection of 3D bones for this animation. (May or may not represent the actual skeleton)
		List<Bone> Bones;
		// The collection of curves that make up this animation.
		Dictionary<string, List<Curve>> Curves;
		// A collection of notifications that may occur.
		Dictionary<string, List<uint32_t>> Notificiations;

		// Gets a reference to a list of node curves.
		List<Curve>& GetNodeCurves(const string& NodeName);
		// Adds a notification to the animation.
		void AddNotification(const string& Name, uint32_t Frame);

		// Gets the count of frames in the animation.
		const uint32_t FrameCount(bool Legacy = false) const;
		// Gets the count of notifications in the animation.
		const uint32_t NotificationCount() const;

		// Specifies if this animation should loop.
		bool Looping;
		// The framerate of this animation.
		float FrameRate;
		// The transformation space of this animation.
		AnimationTransformSpace TransformSpace;
		// The rotation interpolation mode.
		AnimationRotationInterpolation RotationInterpolation;

		// Scales the animation with the given factor.
		void Scale(float Factor);
		// Removes nodes without keyframes.
		void RemoveEmptyNodes();
	};
}