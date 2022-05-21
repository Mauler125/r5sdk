#pragma once

#include <cstdint>
#include <bitset>
#include "Control.h"
#include "ToolTipIcon.h"
#include "DictionaryBase.h"
#include "PopupEventArgs.h"
#include "DrawToolTipEventArgs.h"

namespace Forms
{
	// Provides a small pop-up window containing a line of text
	// that describes the purpose of a tool or control.
	class ToolTip : public Control
	{
	public:
		ToolTip();
		virtual ~ToolTip() = default;

		// Creates a new instance of the specified control with the given parent.
		virtual void CreateControl(Control* Parent = nullptr);

		// Gets a value indicating whether the Tooltip control is currently active.
		bool Active();
		// Sets a value indicating whether the Tooltip control is currently active.
		void SetActive(bool Value);

		// The time (in milliseconds) that passes before the tooltip appears.
		uint32_t AutomaticDelay();
		// The time (in milliseconds) that passes before the tooltip appears.
		void SetAutomaticDelay(uint32_t Value);

		// The initial delay for the tooltip control.
		uint32_t AutoPopDelay();
		// The initial delay for the tooltip control.
		void SetAutoPopDelay(uint32_t Value);

		// Gets is balloon for the tooltip control.
		bool IsBalloon();
		// Sets is balloon for the tooltip control.
		void SetIsBalloon(bool Value);

		// The initial delay for the tooltip control.
		uint32_t InitialDelay();
		// The initial delay for the tooltip control.
		void SetInitialDelay(uint32_t Value);

		// Indicates whether the tooltip will be drawn by the system or the user.
		bool OwnerDraw();
		// Indicates whether the tooltip will be drawn by the system or the user.
		void SetOwnerDraw(bool Value);

		// The length of time (in milliseconds) that the tooltip takes to reshow.
		uint32_t ReshowDelay();
		// The length of time (in milliseconds) that the tooltip takes to reshow.
		void SetReshowDelay(uint32_t Value);

		// Gets or sets a value indicating whether the tooltip appears when the parent is inactive.
		bool ShowAlways();
		// Gets or sets a value indicating whether the tooltip appears when the parent is inactive.
		void SetShowAlways(bool Value);

		// When set to true, any ampersands in the text property are not displayed.
		bool StripAmpersands();
		// When set to true, any ampersands in the text property are not displayed.
		void SetStripAmpersands(bool Value);

		// Gets or sets an Icon on the tooltip.
		ToolTipIcon GetToolTipIcon();
		// Gets or sets an Icon on the tooltip.
		void SetToolTipIcon(ToolTipIcon Value);

		//  Gets or sets the title of the tooltip.
		string ToolTipTitle();
		//  Gets or sets the title of the tooltip.
		void SetToolTipTitle(const string& Value);

		// When set to true, animations are used when tooltip is shown or hidden.
		bool UseAnimation();
		// When set to true, animations are used when tooltip is shown or hidden.
		void SetUseAnimation(bool Value);

		// When set to true, a fade effect is used when tooltips are shown or hidden.
		bool UseFading();
		// When set to true, a fade effect is used when tooltips are shown or hidden.
		void SetUseFading(bool Value);

		// Associates tooltip text with the specified control.
		void SetToolTip(Control* Ctrl, const string& Caption);

		// Removes all tooltips from the currently setup controls.
		void RemoveAll();
		// Removes a tooltip from the specified control.
		void Remove(Control* Ctrl);

		// We must define control event bases here
		virtual void OnHandleCreated();
		virtual void OnBackColorChanged();
		virtual void OnForeColorChanged();
		virtual void OnPopup(const std::unique_ptr<PopupEventArgs>& EventArgs);
		virtual void OnDraw(const std::unique_ptr<DrawToolTipEventArgs>& EventArgs);

		// We must define event handlers here
		EventBase<void(*)(const std::unique_ptr<PopupEventArgs>&, Control*)> Popup;
		EventBase<void(*)(const std::unique_ptr<DrawToolTipEventArgs>&, Control*)> Draw;

		// Override WndProc for specific button messages.
		virtual void WndProc(Message& Msg);

	protected:
		// Get custom control creation parameters for this instance.
		virtual CreateParams GetCreateParams();

		// Registers all tooltip information.
		void RegisterTooltip(Control* Ctrl);

	private:
		// Internal cached flags
		std::bitset<10> _Flags;
		uint32_t _DelayTimes[4];
		ToolTipIcon _ToolTipIcon;
		string _ToolTipTitle;

		// Internal cache dictionary
		Dictionary<uintptr_t, string> _ControlCache;

		// We must define each window message handler here...
		void WmShow();
		void WmPop();
		void WmMove();
		bool WmWindowPosChanged();
		void WmWindowFromPoint(Message& Msg);

		// Adjusts the other delay values based on the automatic value.
		void AdjustBaseFromAuto();
		// Sets the delay time based on the TTDT values.
		void SetDelayTime(uint32_t Type, uint32_t Time);
		// Gets the delay time based on the TTDT values.
		uint32_t GetDelayTime(uint32_t Type);

		// Returns the HWND of the window that is at the specified point.
		uintptr_t GetWindowFromPoint(Drawing::Point ScreenCoords, bool& Success);

		// Internal routines to aid in the tracking of control creation.
		static void ControlHandleCreated(Control* Sender);
		static void ControlHandleDestroyed(Control* Sender);

		// Internal cached constants
		constexpr static uint32_t DEFAULT_DELAY = 500;
		constexpr static uint32_t RESHOW_RATIO = 5;
		constexpr static uint32_t AUTOPOP_RATIO = 10;

		constexpr static uint32_t XBALLOONOFFSET = 10;
		constexpr static uint32_t YBALLOONOFFSET = 8;
	};
}