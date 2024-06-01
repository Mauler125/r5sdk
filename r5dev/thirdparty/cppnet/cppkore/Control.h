#pragma once

#include <cstdint>
#include <memory>
#include <Windows.h>
#include <CommCtrl.h>
#include "Keys.h"
#include "Font.h"
#include "Action.h"
#include "Message.h"
#include "DropTarget.h"
#include "EventBase.h"
#include "StringBase.h"
#include "ControlTypes.h"
#include "AnchorStyles.h"
#include "CreateParams.h"
#include "MouseButtons.h"
#include "ControlStyles.h"
#include "ControlStates.h"
#include "KeyEventArgs.h"
#include "PaintEventArgs.h"
#include "MouseEventArgs.h"
#include "ControlCollection.h"
#include "BoundsSpecified.h"
#include "InvalidateEventArgs.h"
#include "DragEventArgs.h"
#include "KeyPressEventArgs.h"
#include "HandledMouseEventArgs.h"

// Used for reflection messages
#define WM_REFLECT (WM_USER + 0x1C00)

// Undocumented flags
#define DCX_USESTYLE 0x00010000
#define DCX_NODELETERGN 0x00040000

// Remove built-in macros
#undef SetWindowText
#undef DrawText

namespace Forms
{
	// Defines the base class for controls, which are components
	// with visual representation.
	class Control
	{
	public:
		Control();
		virtual ~Control();

		// Makes the control display by setting the visible property to true.
		virtual void Show();
		// Hides the control by setting the visible property to false.
		virtual void Hide();

		// Creates a new instance of the specified control with the given parent.
		virtual void CreateControl(Control* Parent = nullptr);

		// Attempts to set focus to this control.
		bool Focus();
		// Indicates whether the control has focus.
		bool Focused();
		// Indicates whether the control can receive focus.
		bool CanFocus();
		// Indicates whether the control can be selected.
		bool CanSelect();

		// Indicates whether the control or one of it's children currently has focus.
		bool ContainsFocus();

		// Suspends the control layout functionality.
		void SuspendLayout();
		// Resumes layout functionality.
		void ResumeLayout(bool PerformLayout = true);

		// Indicates whether the control is currently enabled.
		bool Enabled();
		// Indicates whether the control is currently enabled.
		void SetEnabled(bool Value);

		// The AllowDrop property. If AllowDrop is set to true then
		// this control will allow drag and drop operations and events to be used.
		bool AllowDrop();
		// The AllowDrop property. If AllowDrop is set to true then
		// this control will allow drag and drop operations and events to be used.
		void SetAllowDrop(bool Value);

		// Indicates whether the control is visible.
		bool Visible();
		// Indicates whether the control is visible.
		void SetVisible(bool Value);
		
		// This will enable or disable double buffering.
		bool DoubleBuffered();
		// This will enable or disable double buffering.
		void SetDoubleBuffered(bool Value);

		// Indicates whether the control has captured the mouse.
		bool Capture();
		// Indicates whether the control has captured the mouse.
		void SetCapture(bool Value);

		// The current value of the anchor property. The anchor property
		// determines which edges of the control are anchored to the container's
		// edges.
		AnchorStyles Anchor();
		// The current value of the anchor property. The anchor property
		// determines which edges of the control are anchored to the container's
		// edges.
		void SetAnchor(AnchorStyles Value);

		// The tab index of this control.
		uint32_t TabIndex();
		// The tab index of this control.
		void SetTabIndex(uint32_t Value);

		// The location of this control.
		Drawing::Point Location();
		// The location of this control.
		void SetLocation(Drawing::Point Value);

		// The size of the control.
		Drawing::Size Size();
		// The size of the control.
		void SetSize(Drawing::Size Value);

		// The maximum size of the control.
		Drawing::Size MaximumSize();
		// The maximum size of the control.
		void SetMaximumSize(Drawing::Size Value);

		// The minimum size of the control.
		Drawing::Size MinimumSize();
		// The minimum size of the control.
		void SetMinimumSize(Drawing::Size Value);

		// The background color of this control.
		Drawing::Color BackColor();
		// The background color of this control.
		void SetBackColor(Drawing::Color Color);

		// The foreground color of the control.
		Drawing::Color ForeColor();
		// The foreground color of the control.
		void SetForeColor(Drawing::Color Color);

		// Retrieves the current font for this control.
		Drawing::Font* GetFont();
		// Sets the current font for this control.
		void SetFont(Drawing::Font* Font);

		// The parent of this control.
		Control* Parent();
		// The parent of this control.
		void SetParent(Control* Value);

		// The client rect of the control.
		Drawing::Rectangle ClientRectangle();

		// The size of the clientRect.
		Drawing::Size ClientSize();
		// The size of the clientRect.
		void SetClientSize(Drawing::Size Value);

		// Computes the location of the client point in screen coords.
		Drawing::Point PointToScreen(const Drawing::Point& Point);
		// Computes the location of the screen point in client coords.
		Drawing::Point PointToClient(const Drawing::Point& Point);

		// Computes the location of the client rectangle in screen coords.
		Drawing::Rectangle RectangleToScreen(const Drawing::Rectangle& Rect);
		// Computes the location of the screen rectangle in client coords.
		Drawing::Rectangle RectangleToClient(const Drawing::Rectangle& Rect);

		// Gets the current text associated with this control.
		virtual String Text();
		// Sets the current text associated with this control.
		virtual void SetText(const String& Value);

		// Brings this control to the front of the z-order.
		void BringToFront();
		// Sends this control to the back of the z-order.
		void SendToBack();

		// Style flags attached to this control.
		bool GetStyle(ControlStyles Flag);
		// Style flags attached to this control.
		void SetStyle(ControlStyles Flags, bool Value);

		// State flags attached to this control.
		bool GetState(ControlStates Flag);
		// State flags attached to this control.
		void SetState(ControlStates Flags, bool Value);

		// Invalidates the control and causes a paint message to be sent to the control.
		void Invalidate(bool InvalidateChildren = false);

		// Executes a delegate on the thread that owns the control's underlying window handle.
		void Invoke(Action Method);
		// Gets whether or not an invoke is required.
		bool InvokeRequired();

		// Forces the control to paint any currently invalid areas.
		void Update();
		// Forces the control to invalidate and immediately repaint itself and children.
		void Refresh();

		// Returns the native handle of this control
		HWND GetHandle();
		// Returns the type of this control
		ControlTypes GetType();
		// Returns the number of child controls
		uint32_t GetControlCount();

		// Retrieves the form that the control is on.
		Control* FindForm();
		// Retrieves the container control that we have, if any.
		Control* GetContainerControl();
		// Retrieves the next control in the tab order of child controls.
		Control* GetNextControl(Control* Ctrl, bool Forward);

		// Verifies if a control is a child of this control.
		bool Contains(Control* Ctrl);

		// Returns a reference to this controls child collection, if available
		const std::unique_ptr<ControlCollection>& Controls();

		// Activates this control.
		void Select();
		// Selects the next control following Ctrl.
		bool SelectNextControl(Control* Ctrl, bool Forward, bool TabStopOnly, bool Nested, bool Wrap);

		// Updates this control in it's parent's z-order.
		void UpdateZOrder();

		// Invokes the default window procedure associated with this Window. It is
		// an error to call this method when the Handle property is zero.
		void DefWndProc(Message& Msg);

		// We must define control event bases here
		virtual void OnPaint(const std::unique_ptr<PaintEventArgs>& EventArgs);
		virtual void OnPaintBackground(const std::unique_ptr<PaintEventArgs>& EventArgs);
		virtual void OnMouseClick(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnMouseDoubleClick(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnMouseDown(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnMouseMove(const std::unique_ptr<MouseEventArgs>& EventArgs);
		virtual void OnInvalidated(const std::unique_ptr<InvalidateEventArgs>& EventArgs);
		virtual void OnMouseWheel(const std::unique_ptr<HandledMouseEventArgs>& EventArgs);
		virtual void OnKeyPress(const std::unique_ptr<KeyPressEventArgs>& EventArgs);
		virtual void OnKeyUp(const std::unique_ptr<KeyEventArgs>& EventArgs);
		virtual void OnKeyDown(const std::unique_ptr<KeyEventArgs>& EventArgs);
		virtual void OnDragEnter(const std::unique_ptr<DragEventArgs>& EventArgs);
		virtual void OnDragOver(const std::unique_ptr<DragEventArgs>& EventArgs);
		virtual void OnDragDrop(const std::unique_ptr<DragEventArgs>& EventArgs);
		virtual void OnDragLeave();
		virtual void OnFontChanged();
		virtual void OnVisibleChanged();
		virtual void OnHandleCreated();
		virtual void OnHandleDestroyed();
		virtual void OnTextChanged();
		virtual void OnMouseEnter();
		virtual void OnMouseLeave();
		virtual void OnMouseHover();
		virtual void OnLostFocus();
		virtual void OnGotFocus();
		virtual void OnStyleChanged();
		virtual void OnLocationChanged();
		virtual void OnSizeChanged();
		virtual void OnResize();
		virtual void OnClientSizeChanged();
		virtual void OnMouseCaptureChanged();
		virtual void OnBackColorChanged();
		virtual void OnForeColorChanged();
		virtual void OnClick();
		virtual void OnDoubleClick();
		virtual void OnEnabledChanged();

		// We must define event handlers here
		EventBase<void(*)(Control*)> Click;
		EventBase<void(*)(Control*)> DoubleClick;
		EventBase<void(*)(Control*)> MouseEnter;
		EventBase<void(*)(Control*)> MouseLeave;
		EventBase<void(*)(Control*)> MouseHover;
		EventBase<void(*)(Control*)> SizeChanged;
		EventBase<void(*)(Control*)> Resize;
		EventBase<void(*)(Control*)> LostFocus;
		EventBase<void(*)(Control*)> GotFocus;
		EventBase<void(*)(Control*)> TextChanged;
		EventBase<void(*)(Control*)> FontChanged;
		EventBase<void(*)(Control*)> HandleCreated;
		EventBase<void(*)(Control*)> HandleDestroyed;
		EventBase<void(*)(Control*)> EnabledChanged;
		EventBase<void(*)(Control*)> VisibleChanged;
		EventBase<void(*)(Control*)> LocationChanged;
		EventBase<void(*)(Control*)> BackColorChanged;
		EventBase<void(*)(Control*)> ForeColorChanged;
		EventBase<void(*)(Control*)> StyleChanged;
		EventBase<void(*)(Control*)> ClientSizeChanged;
		EventBase<void(*)(Control*)> MouseCaptureChanged;
		EventBase<void(*)(Control*)> DragLeave;
		EventBase<void(*)(const std::unique_ptr<DragEventArgs>&, Control*)> DragEnter;
		EventBase<void(*)(const std::unique_ptr<DragEventArgs>&, Control*)> DragDrop;
		EventBase<void(*)(const std::unique_ptr<DragEventArgs>&, Control*)> DragOver;
		EventBase<void(*)(const std::unique_ptr<KeyEventArgs>&, Control*)> KeyUp;
		EventBase<void(*)(const std::unique_ptr<KeyEventArgs>&, Control*)> KeyDown;
		EventBase<void(*)(const std::unique_ptr<KeyPressEventArgs>&, Control*)> KeyPress;
		EventBase<void(*)(const std::unique_ptr<PaintEventArgs>&, Control*)> Paint;
		EventBase<void(*)(const std::unique_ptr<MouseEventArgs>&, Control*)> MouseUp;
		EventBase<void(*)(const std::unique_ptr<MouseEventArgs>&, Control*)> MouseDown;
		EventBase<void(*)(const std::unique_ptr<MouseEventArgs>&, Control*)> MouseMove;
		EventBase<void(*)(const std::unique_ptr<MouseEventArgs>&, Control*)> MouseClick;
		EventBase<void(*)(const std::unique_ptr<MouseEventArgs>&, Control*)> MouseDoubleClick;
		EventBase<void(*)(const std::unique_ptr<InvalidateEventArgs>&, Control*)> Invalidated;
		EventBase<void(*)(const std::unique_ptr<HandledMouseEventArgs>&, Control*)> MouseWheel;

		// The standard windows message pump for this control.
		virtual void WndProc(Message& Msg);

		// Routine to get the default GDI+ palette for a control.
		static HPALETTE SetUpPalette(HDC Dc, bool Force, bool RealizePalette);
		// Gets the current state of the mouse buttons.
		static MouseButtons GetMouseButtons();
		// Gets the current position of the mouse in screen coordinates.
		static Drawing::Point GetMousePosition();
		// Retrieves the current state of the modifier keys.
		static Keys GetModifierKeys();

		// Determines if the required scaling property is enabled
		bool RequiredScalingEnabled();
		// Determines if the required scaling property is enabled
		void SetRequiredScalingEnabled(bool Value);

		// Routine to scale a control
		virtual void Scale(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl);

	protected:
		// The control handle
		HWND _Handle;
		// The control base windows proc
		LONG_PTR _WndProcBase;

		// The RTTI type of this control
		ControlTypes _RTTI;

		// The parent control if any
		Control* _Parent;

		// A collection of children controls, if we are a container control
		std::unique_ptr<ControlCollection> _Controls;

		// Internal size caching
		uint32_t _X;
		uint32_t _Y;
		uint32_t _Width;
		uint32_t _Height;
		uint32_t _ClientWidth;
		uint32_t _ClientHeight;

		// Internal tab index caching
		uint32_t _TabIndex;

		// Internal min/max size caching
		uint32_t _MaximumWidth;
		uint32_t _MaximumHeight;
		uint32_t _MinimumWidth;
		uint32_t _MinimumHeight;

		// Internal text caching
		String _Text;

		// Contains the anchor information...
		struct AnchorDeltasCache
		{
			float XMoveFrac;
			float YMoveFrac;
			float XSizeFrac;
			float YSizeFrac;
			RECT InitialRect;
			bool InitialRectSet;
		} _AnchorDeltas;

		// Internal layout anchor
		AnchorStyles _Anchor;

		// Control base colors
		Drawing::Color _BackColor;
		Drawing::Color _ForeColor;

		// Control brush, if any
		uintptr_t _BackColorBrush;

		// Control base font
		std::unique_ptr<Drawing::Font> _Font;

		// Updates the bounds of the control based on the handle the control is bound to.
		void UpdateBounds();
		// Updates the bounds of the control based on the bounds passed in.
		void UpdateBounds(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height, uint32_t ClientWidth, uint32_t ClientHeight);
		// Updates the bounds of the control based on the bounds passed in.
		void UpdateBounds(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height);
		// Sets the bounds of the control.
		void SetBounds(uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height);
		// Updates the deltas of the control based on the anchor
		void UpdateDeltas();
		// Updates the initial position based on anchor
		void UpdateInitialPos();

		// Internal selection routine
		virtual void Select(bool Directed, bool Forward);
		// Internal layout resuming routine
		virtual void OnLayoutResuming(bool PerformLayout);
		// Internal child layout resuming routine
		virtual void OnChildLayoutResuming(Control* Child, bool PerformLayout);

		// Gets the current control window style
		uint32_t WindowStyle();
		// Sets the current controls window style
		void SetWindowStyle(uint32_t Value);
		// Gets the current control extended style
		uint32_t WindowExStyle();
		// Sets the current controls extended style
		void SetWindowExStyle(uint32_t Value);

		// Internal routine to calculate client size into size
		Drawing::Size SizeFromClientSize(int32_t Width, int32_t Height);

		// Internal routine to scale a size, based on control limitations
		Drawing::Size ScaleSize(Drawing::Size Start, float X, float Y);
		// Internal routine to scale a control, calculating bounds
		void ScaleControl(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl);
		// Internal routine to scale child controls, calculating bounds
		void ScaleChildControls(Drawing::SizeF IncludedFactor, Drawing::SizeF ExcludedFactor, Control* Ctrl);

		// Internal routine to scale a control by factor
		virtual void ScaleControl(Drawing::SizeF Factor, BoundsSpecified Specified);
		// Internal routine to scale a bounds by a factor
		virtual Drawing::Rectangle GetScaledBounds(Drawing::Rectangle Bounds, Drawing::SizeF Factor, BoundsSpecified Specified);

		// The GDI brush for our background color.
		uintptr_t BackColorBrush();

		// Gets the current text of the Window
		virtual String WindowText();
		// Sets the current text of the Window
		virtual void SetWindowText(const String& Value);

		// Updates the control styles...
		void UpdateStyles();

		// Registers the control drop target.
		void SetAcceptDrops(bool Accept);

		// Resets the mouse leave listeners.
		void ResetMouseEventArgs();

		// Destroys the window handle
		void DestroyHandle();

		// Performs layout of child controls based on their anchors.
		virtual void PerformLayout();
		// Processes a key message.
		virtual bool ProcessKeyMessage(Message& Msg);
		// Previews a key message.
		virtual bool ProcessKeyPreview(Message& Msg);
		// Processes a key message and properly generates key events.
		virtual bool ProcessKeyEventArgs(Message& Msg);

		// Internal routine to get control visibility.
		virtual bool GetVisibleCore();
		// Internal routine to change control visibility.
		virtual void SetVisibleCore(bool Value);

		// Gets the CreateParams for this control instance.
		virtual CreateParams GetCreateParams();

		// An internal routine that is the root window message processor.
		static LRESULT CALLBACK InternalWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

		// Used when the control can house other controls.
		virtual void AddControl(Control* Ctrl);

		// Used to properly clean up the control.
		virtual void Dispose();

		// Sets the text and background colors of the DC, and returns the background HBRUSH.
		virtual uintptr_t InitializeDCForWmCtlColor(HDC Dc, int32_t Message);

		// Custom message index cache
		static uint32_t WM_MOUSEENTER;
		static uint32_t WM_INVOKEUI;

		// GDI+ palette for rendering
		static HPALETTE HalftonePalette;

		// Whether or not we are a container
		virtual bool IsContainerControl();

		// GDI region copying
		static HRGN CreateCopyOfRgn(HRGN InRgn);

		// Constructs a control from a handle
		static Control* FromHandle(HWND hWnd);
		// Constructs a control from a child handle
		static Control* FromChildHandle(HWND hWnd);

	private:
		// The control styles
		ControlStyles _ControlStyles;
		// The control states
		ControlStates _ControlStates;

		// Total count of suspended layout transactions
		uint8_t _LayoutSuspendCount;

		// Whether or not DPI/Font scaling is required
		bool _RequiredScalingEnabled;
		// Required scaling mode
		BoundsSpecified _RequiredScaling;

		// The control drag drop interface
		std::unique_ptr<DropTarget> _DropTarget;

		// We must define each window message handler here...
		void WmMouseDown(Message& Msg, MouseButtons Button, uint32_t Clicks);
		void WmMouseUp(Message& Msg, MouseButtons Button, uint32_t Clicks);
		void WmMouseEnter(Message& Msg);
		void WmMouseLeave(Message& Msg);
		void WmMouseHover(Message& Msg);
		void WmClose(Message& Msg);
		void WmEraseBkgnd(Message& Msg);
		void WmPaint(Message& Msg);
		void WmCreate(Message& Msg);
		void WmShowWindow(Message& Msg);
		void WmMove(Message& Msg);
		void WmParentNotify(Message& Msg);
		void WmCommand(Message& Msg);
		void WmQueryNewPalette(Message& Msg);
		void WmNotify(Message& Msg);
		void WmNotifyFormat(Message& Msg);
		void WmCaptureChanged(Message& Msg);
		void WmCtlColorControl(Message& Msg);
		void WmKillFocus(Message& Msg);
		void WmSetFocus(Message& Msg);
		void WmMouseMove(Message& Msg);
		void WmSetCursor(Message& Msg);
		void WmMouseWheel(Message& Msg);
		void WmKeyChar(Message& Msg);
		void WmWindowPosChanged(Message& Msg);
		void WmInvokeOnUIThread(Message& Msg);

		// Removes pending messages from the message queue.
		void RemovePendingMessages(uint32_t MsgMin, uint32_t MsgMax);

		// Internal routine to update a child's z-order.
		void UpdateChildZOrder(Control* Ctrl);
		// Internal routine to update a childs index in the control array.
		void UpdateChildControlIndex(Control* Ctrl);

		// This is called recursively when visibility is changed for a control.
		void SelectNextIfFocused();

		// Internal routine to find the next available control.
		Control* GetNextSelectableControl(Control* Ctrl, bool Forward, bool TabStopOnly, bool Nested, bool Wrap);
		// Internal routine to get the first child in tab order.
		Control* GetFirstChildcontrolInTabOrder(bool Forward);

		// Internal routine used to reflect messages up from a top level control.
		static bool ReflectMessageInternal(HWND hWnd, Message& Msg);
		// Internal routine to check for a container control
		static bool IsFocusManagingContainerControl(Control* Ctrl);

		// Internal routine to make sure a class is registered
		static String RegisterWndClass(const char* ClassName, DWORD ClassStyle, bool& Subclass);
	};
}