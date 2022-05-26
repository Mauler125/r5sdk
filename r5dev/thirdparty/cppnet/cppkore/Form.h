#pragma once

#include <bitset>
#include <memory>
#include <cstdint>
#include "Icon.h"
#include "Control.h"
#include "ContainerControl.h"
#include "DialogResult.h"
#include "CloseReason.h"
#include "FormBorderStyle.h"
#include "FormWindowState.h"
#include "FormStartPosition.h"
#include "FormClosedEventArgs.h"
#include "FormClosingEventArgs.h"
#include "PaintFrameEventArgs.h"

namespace Forms
{
	class Form : public ContainerControl
	{
	public:
		Form();
		virtual ~Form() = default;

		// Gets the border style of the form.
		FormBorderStyle GetFormBorderStyle();
		// Sets the border style of the form.
		void SetFormBorderStyle(FormBorderStyle Value);

		// Gets a value indicating whether a control box is displayed in the
		// caption bar of the form.
		bool ControlBox();
		// Sets a value indicating whether a control box is displayed in the
		// caption bar of the form.
		void SetControlBox(bool Value);

		// Gets the dialog result for the form.
		DialogResult GetDialogResult();
		// Sets the dialog result for the form.
		void SetDialogResult(DialogResult Value);

		// Gets a value indicating whether the maximize button is
		// displayed in the caption bar of the form.
		bool MaximizeBox();
		// Sets a value indicating whether the maximize button is
		// displayed in the caption bar of the form.
		void SetMaximizeBox(bool Value);

		// Gets a value indicating whether the minimize button is displayed in the caption bar of the form.
		bool MinimizeBox();
		// Sets a value indicating whether the minimize button is displayed in the caption bar of the form.
		void SetMinimizeBox(bool Value);

		// If ShowInTaskbar is true then the form will be displayed
		// in the Windows Taskbar.
		bool ShowInTaskbar();
		// If ShowInTaskbar is true then the form will be displayed
		// in the Windows Taskbar.
		void SetShowInTaskbar(bool Value);

		// Gets a value indicating whether an icon is displayed in the
		// caption bar of the form.
		bool ShowIcon();
		// Sets a value indicating whether an icon is displayed in the
		// caption bar of the form.
		void SetShowIcon(bool Value);

		// Gets the icon displayed on this form.
		std::unique_ptr<Drawing::Icon>& Icon();
		// Sets the icon displayed on this form.
		void SetIcon(std::unique_ptr<Drawing::Icon>&& Value);

		// Gets the starting position of the form at run time.
		FormStartPosition StartPosition();
		// Sets the starting position of the form at run time.
		void SetStartPosition(FormStartPosition Value);

		// Gets a value indicating whether the form should be displayed as the top-most
		// form of your application.
		bool TopMost();
		// Sets a value indicating whether the form should be displayed as the top-most
		// form of your application.
		void SetTopMost(bool Value);

		// Gets the form's window state.
		FormWindowState WindowState();
		// Sets the form's window state.
		void SetWindowState(FormWindowState Value);

		// Activates the form and gives it focus.
		void Activate();

		// Checks whether a modal dialog is ready to close.
		bool CheckCloseDialog(bool ClosingOnly);

		// Closes the form.
		void Close();

		// Displays this form as a model dialog box with no owner window. (You MUST clean up the dialog)
		DialogResult ShowDialog();
		// Displays this form as a model dialog with the specified owner. (You MUST clean up the dialog)
		DialogResult ShowDialog(Form* Owner);

		// Gets the form that owns this form.
		Form* Owner();
		// Sets the form that owns this form.
		void SetOwner(Form* Value);

		// Gets whether or not to render a custom form frame. (OnPaintFrame)
		bool UseCustomFrame();
		// Sets whether or not to render a custom form frame. (OnPaintFrame)
		void SetUseCustomFrame(bool Value);

		// Gets whether or not the form is active.
		bool Active();
		// Sets whether or not the form is active.
		void SetActive(bool Value);

		// Gets the opacity value for this form.
		uint8_t Opacity();
		// Sets the opacity value for this form (0-255).
		void SetOpacity(uint8_t Value);

		// Updates the default button based on current selection
		void UpdateDefaultButton();

		// We must define control event bases here
		virtual void OnLoad();
		virtual void OnHandleCreated();
		virtual void OnStyleChanged();
		virtual void OnEnabledChanged();
		virtual void OnActivated();
		virtual void OnDeactivate();
		virtual void OnClosing(const std::unique_ptr<FormClosingEventArgs>& EventArgs);
		virtual void OnFormClosing(const std::unique_ptr<FormClosingEventArgs>& EventArgs);
		virtual void OnClosed(const std::unique_ptr<FormClosedEventArgs>& EventArgs);
		virtual void OnFormClosed(const std::unique_ptr<FormClosedEventArgs>& EventArgs);
		virtual void OnPaintFrame(const std::unique_ptr<PaintFrameEventArgs>& EventArgs);

		// We must define event handlers here
		EventBase<void(*)(Control*)> Load;
		EventBase<void(*)(Control*)> Activated;
		EventBase<void(*)(Control*)> Deactivate;
		EventBase<void(*)(const std::unique_ptr<FormClosingEventArgs>&, Control*)> Closing;
		EventBase<void(*)(const std::unique_ptr<FormClosingEventArgs>&, Control*)> FormClosing;
		EventBase<void(*)(const std::unique_ptr<FormClosedEventArgs>&, Control*)> Closed;
		EventBase<void(*)(const std::unique_ptr<FormClosedEventArgs>&, Control*)> FormClosed;
		EventBase<void(*)(const std::unique_ptr<PaintFrameEventArgs>&, Control*)> PaintFrame;

		// Add a control to this form.
		virtual void AddControl(Control* Ctrl);
		// Used to properly clean up the control.
		virtual void Dispose();
		// Override WndProc for specific form messages.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

		// The owner form, if any
		Form* _Owner;

		// Gets whether or not closing has been called.
		bool CalledClosing();
		// Sets whether or not closing has been called.
		void SetCalledClosing(bool Value);

		// Gets whether or not create control has been called.
		bool CalledCreateControl();
		// Sets whether or not create control has been called.
		void SetCalledCreateControl(bool Value);

		// Gets whether or not make visible has been called.
		bool CalledMakeVisible();
		// Sets whether or not make visible has been called.
		void SetCalledMakeVisible(bool Value);

		// Gets whether or not on load has been called.
		bool CalledOnLoad();
		// Sets whether or not on load has been called.
		void SetCalledOnLoad(bool Value);

		// Gets a value that determines whether the window is closing.
		bool IsClosing();
		// Sets a value that determines whether the window is closing.
		void SetIsClosing(bool Value);

		// Gets a value that determine whether we show the window on create.
		bool ShowWindowOnCreate();
		// Sets a value that determine whether we show the window on create.
		void SetShowWindowOnCreate(bool Value);

		// Internal routine to change control visibility.
		virtual void SetVisibleCore(bool Value);

	private:
		// Internal cached flags
		FormBorderStyle _FormBorderStyle;
		FormWindowState _FormWindowState;
		FormStartPosition _FormStartPosition;
		DialogResult _DialogResult;
		CloseReason _CloseReason;
		uint8_t _Opacity;

		// The default button for this form, if any
		Control* _DefaultButton;

		// Internal shared form flags
		std::bitset<14> _FormStateFlags;

		// Internal icon handle
		std::unique_ptr<Drawing::Icon> _Icon;

		// We must define each window message handler here...
		void WmClose(Message& Msg);
		void WmSysCommand(Message& Msg);
		void WmEraseBkgnd(Message& Msg);
		void WmGetMinMaxInfo(Message& Msg);
		void WmWindowPosChanged(Message& Msg);
		void WmNcPaint(Message& Msg);
		void WmNcActivate(Message& Msg);
		void WmActivate(Message& Msg);
		void WmCreate(Message& Msg);

		// Internal routine to setup the border styles
		void FillInCreateParamsBorderStyles(CreateParams& Cp);
		// Internal routine to setup the window state
		void FillInCreateParamsWindowState(CreateParams& Cp);
		// Internal routine to setup the border icons
		void FillInCreateParamsBorderIcons(CreateParams& Cp);
		// Internal routine to setup the start position
		void FillInCreateParamsStartPosition(CreateParams& Cp);

		// Updates the internal window state
		void UpdateWindowState();
		// This forces the SystemMenu to look like we want
		void AdjustSystemMenu();
		// This forces the SystemMenu to look like we want
		void AdjustSystemMenu(HMENU Menu);

		// Sets up the default button
		void SetDefaultButton(Control* Btn);

		// Centers the form to the parent
		void CenterToParent();
		// Centers the form to the screen
		void CenterToScreen();
	};
}