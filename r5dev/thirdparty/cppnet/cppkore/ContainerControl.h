#pragma once

#include "Control.h"
#include "AutoScaleMode.h"

namespace Forms
{
	// TODO: Process [*] Key functions, virtual...

	class ContainerControl : public Control
	{
	public:
		ContainerControl();
		virtual ~ContainerControl() = default;

		// Indicates the current active control on the container control.
		Control* ActiveControl();
		// Indicates the current active control on the container control.
		void SetActiveControl(Control* Value);

		// Activates a control
		bool ActivateControl(Control* Value);

		// Represents the DPI or Font setting that the control has been scaled to or designed at.
		Drawing::SizeF AutoScaleDimensions();
		// Represents the DPI or Font setting that the control has been scaled to or designed at.
		void SetAutoScaleDimensions(Drawing::SizeF Size);

		// Gets the current auto scale ratio.
		Drawing::SizeF CurrentAutoScaleDimensions();

		// Determines the scaling mode of this control.
		AutoScaleMode AutoScaleMode();
		// Determines the scaling mode of this control.
		void SetAutoScaleMode(Forms::AutoScaleMode Mode);

		// Override WndProc for specific form messages.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();
		// Make sure we set this so we know if we are a container.
		virtual bool IsContainerControl();

		// Internal routine to active a control.
		bool ActivateControlInternal(Control* Ctrl, bool Originator);
		// Internal routine to set active control.
		bool AssignActiveControlInternal(Control* Ctrl);

		// Internal selection routine
		virtual void Select(bool Directed, bool Forward);
		// Internal routine to scale a control
		virtual void Scale(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl);
		// Internal layout resuming routine
		virtual void OnLayoutResuming(bool PerformLayout);
		// Internal child layout resuming routine
		virtual void OnChildLayoutResuming(Control* Child, bool PerformLayout);

		// Sets focus to the active control.
		void FocusActiveControlInternal();

		// Internal routine to set a control as active.
		void SetActiveControlInternal(Control* Value);

		// Gets the parent form, if any.
		Control* ParentFormInternal();

		// Fetches the auto scale ratio for the given settings.
		Drawing::SizeF AutoScaleFactor();

	private:
		// Internal cached flags
		Control* _ActiveControl;
		Control* _FocusedControl;

		Drawing::SizeF _AutoScaleDimensions;
		Drawing::SizeF _CurrentAutoScaleDimensions;

		Forms::AutoScaleMode _AutoScaleMode;
		bool _ScalingNeededOnLayout;

		// We must define each window message handler here...
		void WmSetFocus(Message& Msg);

		// Internal routine to enable scaling on child controls.
		void LayoutScalingNeeded();
		// Internal routine to set scaling flag.
		void EnableRequiredScaling(Control* Ctrl, bool Enable);

		// Internal routine to determinalistically perform scaling.
		void PerformNeededAutoScaleOnLayout();

		// Performs scaling of this control. Scaling works by scaling all children of this control.
		void PerformAutoScale(bool IncludeBounds, bool ExcludeBounds);

		// Internal routine to calculate the font scale ratio.
		Drawing::SizeF GetFontAutoScaleDimensions();
	};
}