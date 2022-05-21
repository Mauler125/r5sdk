#include "stdafx.h"
#include "Curve.h"

namespace Assets
{
	Curve::Curve()
		: Curve("", CurveProperty::Extra, AnimationCurveMode::Absolute)
	{
	}

	Curve::Curve(const string& Name, CurveProperty Property)
		: Curve(Name, Property, AnimationCurveMode::Absolute)
	{
	}

	Curve::Curve(const string& Name, CurveProperty Property, AnimationCurveMode Mode)
		: Name(Name), Property(Property), Mode(Mode), _IsFrameIntegral(true)
	{
	}

	bool Curve::IsFrameIntegral() const
	{
		return this->_IsFrameIntegral;
	}

	void Curve::SetFrameIntegral(bool Value)
	{
		if (this->_IsFrameIntegral != Value)
		{
			this->_IsFrameIntegral = Value;

			for (auto& Key : Keyframes)
			{
				if (Value)
					Key.Frame.Integer32 = (uint32_t)Key.Frame.Float;
				else
					Key.Frame.Float = (float)Key.Frame.Integer32;
			}
		}
	}

	CurveValue::CurveValue(uint8_t Value)
		: Byte(Value)
	{
	}

	CurveValue::CurveValue(uint32_t Value)
		: Integer32(Value)
	{
	}

	CurveValue::CurveValue(float Value)
		: Float(Value)
	{
	}

	CurveValue::CurveValue(Math::Quaternion Value)
		: Vector4(Value)
	{
	}

	CurveFrame::CurveFrame(uint32_t Value)
		: Integer32(Value)
	{
	}

	CurveFrame::CurveFrame(float Value)
		: Float(Value)
	{
	}

	CurveKeyframe::CurveKeyframe()
		: Frame(0u), Value({0, 0, 0, 0})
	{
	}

	CurveKeyframe::CurveKeyframe(uint32_t Frame, float Value)
		: Frame(Frame), Value(Value)
	{
	}

	CurveKeyframe::CurveKeyframe(float Frame, float Value)
		: Frame(Frame), Value(Value)
	{
	}

	CurveKeyframe::CurveKeyframe(uint32_t Frame, Math::Quaternion Value)
		: Frame(Frame), Value(Value)
	{
	}

	CurveKeyframe::CurveKeyframe(float Frame, Math::Quaternion Value)
		: Frame(Frame), Value(Value)
	{
	}
}
