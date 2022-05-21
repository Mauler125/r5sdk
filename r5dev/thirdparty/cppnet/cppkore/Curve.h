#pragma once

#include <cstdint>
#include "StringBase.h"
#include "ListBase.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "AnimationTypes.h"

namespace Assets
{
	// The property of the node being animated
	enum class CurveProperty
	{
		Extra,
		RotateQuaternion,
		RotateX,
		RotateY,
		RotateZ,
		TranslateX,
		TranslateY,
		TranslateZ,
		ScaleX,
		ScaleY,
		ScaleZ,
		Visibility,
	};

	union CurveFrame
	{
		uint32_t Integer32;
		float Float;

		explicit CurveFrame(uint32_t Value);
		explicit CurveFrame(float Value);
	};

	union CurveValue
	{
		uint8_t Byte;
		uint32_t Integer32;
		float Float;
		Math::Quaternion Vector4;

		explicit CurveValue(uint8_t Value);
		explicit CurveValue(uint32_t Value);
		explicit CurveValue(float Value);
		explicit CurveValue(Math::Quaternion Value);
	};

	// A keyframe is a pair of frame time and value at that specific frame time
	struct CurveKeyframe
	{
		CurveFrame Frame;
		CurveValue Value;

		CurveKeyframe();
		explicit CurveKeyframe(uint32_t Frame, float Value);
		explicit CurveKeyframe(float Frame, float Value);
		explicit CurveKeyframe(uint32_t Frame, Math::Quaternion Value);
		explicit CurveKeyframe(float Frame, Math::Quaternion Value);
	};

	// Represents a 3D animation curve for a specific node->property.
	class Curve
	{
	public:
		Curve();
		Curve(const string& Name, CurveProperty Property);
		Curve(const string& Name, CurveProperty Property, AnimationCurveMode Mode);

		// The node name of this curve.
		string Name;
		// The property of the node for the curve.
		CurveProperty Property;

		// Whether or not the frame is an integer, or floating point value. (Default: true)
		bool IsFrameIntegral() const;
		// Sets whether or not the frame is an integer, or floating point value.
		void SetFrameIntegral(bool Value);

		// A list of keyframes that make up this curve
		List<CurveKeyframe> Keyframes;
		// The mode to apply each curve value using (Default: Absolute)
		AnimationCurveMode Mode;

	private:
		// Internal cached values
		bool _IsFrameIntegral;
	};
}