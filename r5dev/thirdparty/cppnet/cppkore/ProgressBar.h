#pragma once

#include <cstdint>
#include "Control.h"
#include "FlatStyle.h"
#include "BorderStyle.h"
#include "ContentAlignment.h"
#include "ProgressBarStyle.h"

namespace Forms
{
	// Represents a Windows progress bar control.
	class ProgressBar : public Control
	{
	public:
		ProgressBar();
		virtual ~ProgressBar() = default;

		// Creates a new instance of the specified control with the given parent.
		virtual void CreateControl(Control* Parent = nullptr);

		// Gets the current position of the progress bar.
		uint32_t Value();
		// Sets the current position of the progress bar.
		void SetValue(uint32_t Value);

		// Gets the marquee animation speed of the progress bar.
		uint32_t MarqueeAnimationSpeed();
		// Sets the marquee animation speed of the progress bar.
		void SetMarqueeAnimationSpeed(uint32_t Value);

		// Gets the maximum value of the progress bar.
		uint32_t Maximum();
		// Sets the maximum value of the progress bar.
		void SetMaximum(uint32_t Value);

		// Gets the minimum value of the progress bar.
		uint32_t Minimum();
		// Sets the minimum value of the progress bar.
		void SetMinimum(uint32_t Value);

		// Gets the amount that a call to PerformStep() to increase the progress.
		uint32_t Step();
		// Sets the amount that a call to PerformStep() to increase the progress.
		void SetStep(uint32_t Value);

		// Gets the style of the progress bar. This is can be either Blocks or Continuous.
		ProgressBarStyle ProgressStyle();
		// Sets the style of the progress bar. This is can be either Blocks or Continuous.
		void SetProgressStyle(ProgressBarStyle Value);

		// Advances the current position of the progress bar.
		void Increment(uint32_t Value);
		// Advances the current position of the progress bar by the step count.
		void PerformStep();

		// We must define control event bases here
		virtual void OnHandleCreated();
		virtual void OnBackColorChanged();
		virtual void OnForeColorChanged();

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

	private:
		// Cached flags
		uint32_t _Minimum;
		uint32_t _Maximum;
		uint32_t _Step;
		uint32_t _Value;
		uint32_t _MarqueeSpeed;

		// The style the control displays
		ProgressBarStyle _Style;

		// Starts the marquee animation.
		void StartMarquee();
		// Updates the progress.
		void UpdatePos();
	};
}