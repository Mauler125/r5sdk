#include "stdafx.h"
#include "TextBoxBase.h"

namespace Forms
{
	TextBoxBase::TextBoxBase()
		: Control(), _Flags((TextBoxFlags)0), _BorderStyle(BorderStyle::Fixed3D), _MaxLength(32767), _SelectionStart(0), _SelectionLength(0), _DoubleClickFired(false)
	{
		SetFlag(TextBoxFlags::FlagAutoSize | TextBoxFlags::FlagHideSelection | TextBoxFlags::FlagWordWrap | TextBoxFlags::FlagShortcutsEnabled, true);
		// TODO: Re-evaluate after setting auto-size flag properly... SetStyle(ControlStyles::FixedHeight, GetFlag(TextBoxFlags::FlagAutoSize));
		SetStyle(ControlStyles::StandardClick |
			ControlStyles::StandardDoubleClick |
			ControlStyles::UseTextForAccessibility |
			ControlStyles::UserPaint, false);
		
		// Default back color for text based controls
		_BackColor = { 255, 255, 255 };
	}

	void TextBoxBase::OnMouseUp(const std::unique_ptr<MouseEventArgs>& EventArgs)
	{
		auto Pt = this->PointToScreen({ EventArgs->X, EventArgs->Y });

		if (EventArgs->Button == MouseButtons::Left)
		{
			if (WindowFromPoint({ Pt.X, Pt.Y }) == this->_Handle)
			{
				if (!this->_DoubleClickFired)
				{
					OnClick();
					OnMouseClick(EventArgs);
				}
				else
				{
					this->_DoubleClickFired = false;
					OnDoubleClick();
					OnMouseDoubleClick(EventArgs);
				}
			}

			this->_DoubleClickFired = false;
		}

		Control::OnMouseUp(EventArgs);
	}

	void TextBoxBase::OnHideSelectionChanged()
	{
		HideSelectionChanged.RaiseEvent(this);
	}

	void TextBoxBase::OnModifiedChanged()
	{
		ModifiedChanged.RaiseEvent(this);
	}

	void TextBoxBase::OnMultilineChanged()
	{
		MultilineChanged.RaiseEvent(this);
	}

	void TextBoxBase::OnReadOnlyChanged()
	{
		ReadOnlyChanged.RaiseEvent(this);
	}

	void TextBoxBase::OnAcceptsTabChanged()
	{
		AcceptsTabChanged.RaiseEvent(this);
	}

	void TextBoxBase::OnHandleCreated()
	{
		Control::OnHandleCreated();

		if (GetFlag(TextBoxFlags::FlagSetSelectionOnHandleCreated))
		{
			Select(this->_SelectionStart, this->_SelectionLength);
			SetFlag(TextBoxFlags::FlagScrollToCaretOnHandleCreated, false);
		}

		this->UpdateMaxLength();

		if (GetFlag(TextBoxFlags::FlagModified))
			SendMessageA(this->_Handle, EM_SETMODIFY, 1, NULL);

		if (GetFlag(TextBoxFlags::FlagScrollToCaretOnHandleCreated))
		{
			ScrollToCaret();
			SetFlag(TextBoxFlags::FlagScrollToCaretOnHandleCreated, false);
		}
	}

	void TextBoxBase::OnHandleDestroyed()
	{
		SetFlag(TextBoxFlags::FlagModified, this->Modified());
		SetFlag(TextBoxFlags::FlagSetSelectionOnHandleCreated, true);
		GetSelectionStartAndLength(this->_SelectionStart, this->_SelectionLength);

		// Call the base event last
		Control::OnHandleDestroyed();
	}

	BorderStyle TextBoxBase::GetBorderStyle()
	{
		return this->_BorderStyle;
	}

	void TextBoxBase::SetBorderStyle(BorderStyle Value)
	{
		this->_BorderStyle = Value;

		Invalidate();
		UpdateStyles();
	}

	bool TextBoxBase::AcceptsTab()
	{
		return GetFlag(TextBoxFlags::FlagAcceptsTab);
	}

	void TextBoxBase::SetAcceptsTab(bool Value)
	{
		if (GetFlag(TextBoxFlags::FlagAcceptsTab) != Value)
		{
			SetFlag(TextBoxFlags::FlagAcceptsTab, Value);
			OnAcceptsTabChanged();
		}
	}

	bool TextBoxBase::CanUndo()
	{
		return (SendMessageA(this->_Handle, EM_CANUNDO, NULL, NULL) == 0);
	}

	bool TextBoxBase::HideSelection()
	{
		return GetFlag(TextBoxFlags::FlagHideSelection);
	}

	void TextBoxBase::SetHideSelection(bool Value)
	{
		if (GetFlag(TextBoxFlags::FlagHideSelection) != Value)
		{
			SetFlag(TextBoxFlags::FlagHideSelection, Value);

			Invalidate();
			UpdateStyles();

			OnHideSelectionChanged();
		}
	}

	List<string> TextBoxBase::Lines()
	{
		List<string> Result;

		auto Buffer = this->Text();
		uint32_t LineStart = 0;

		while (LineStart < Buffer.Length())
		{
			uint32_t LineEnd = LineStart;
			for (; LineEnd < Buffer.Length(); LineEnd++)
			{
				char c = Buffer[LineEnd];
				if (c == '\r' || c == '\n')
					break;
			}

			Result.EmplaceBack(Buffer.Substring(LineStart, LineEnd - LineStart));

			// Treat "\r", "\r\n", and "\n" as new lines
			if (LineEnd < Buffer.Length() && Buffer[LineEnd] == '\r')
				LineEnd++;
			if (LineEnd < Buffer.Length() && Buffer[LineEnd] == '\n')
				LineEnd++;

			LineStart = LineEnd;
		}

		if (Buffer.Length() > 0 && (Buffer[Buffer.Length() - 1] == '\r' || Buffer[Buffer.Length() - 1] == '\n'))
			Result.EmplaceBack("");

		return Result;
	}

	void TextBoxBase::SetLines(const List<string>& Value)
	{
		string Result = "";

		for (auto& Line : Value)
		{
			Result.Append(Line);
			Result.Append((char*)"\r\n");
		}

		this->SetText(Result);
	}

	uint32_t TextBoxBase::MaxLength()
	{
		return this->_MaxLength;
	}

	void TextBoxBase::SetMaxLength(uint32_t Value)
	{
		if (this->_MaxLength != Value)
		{
			this->_MaxLength = Value;
			UpdateMaxLength();
		}
	}

	string TextBoxBase::Text()
	{
		return Control::Text();
	}

	void TextBoxBase::SetText(const string& Value)
	{
		if (Value != Control::Text())
		{
			Control::SetText(Value);

			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, EM_SETMODIFY, 0, NULL);
		}
	}

	uint32_t TextBoxBase::TextLength()
	{
		if (GetState(ControlStates::StateCreated))
			return (uint32_t)GetWindowTextLengthA(this->_Handle);

		return Text().Length();
	}

	bool TextBoxBase::WordWrap()
	{
		return GetFlag(TextBoxFlags::FlagWordWrap);
	}

	void TextBoxBase::SetWordWrap(bool Value)
	{
		if (WordWrap() != Value)
		{
			SetFlag(TextBoxFlags::FlagWordWrap, Value);
			UpdateStyles();
		}
	}

	void TextBoxBase::AppendText(const string& Text)
	{
		int32_t SelStart, SelLength;
		GetSelectionStartAndLength(SelStart, SelLength);

		int32_t EndOfText = this->GetEndPosition();

		SelectInternal(EndOfText, EndOfText, EndOfText);
		SetSelectedText(Text);

		if (this->_Width == 0 || this->_Height == 0)
			Select(SelStart, SelLength);
	}

	void TextBoxBase::Clear()
	{
		this->SetText("");
	}

	void TextBoxBase::ClearUndo()
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, EM_EMPTYUNDOBUFFER, 0, 0);
	}

	void TextBoxBase::Copy()
	{
		SendMessageA(this->_Handle, WM_COPY, NULL, NULL);
	}

	void TextBoxBase::Cut()
	{
		SendMessageA(this->_Handle, WM_CUT, NULL, NULL);
	}

	void TextBoxBase::Paste()
	{
		SendMessageA(this->_Handle, WM_PASTE, NULL, NULL);
	}

	void TextBoxBase::Undo()
	{
		SendMessageA(this->_Handle, EM_UNDO, NULL, NULL);
	}

	void TextBoxBase::Select(int32_t Start, int32_t Length)
	{
		int TextLen = this->TextLength();

		if (Start > TextLen)
		{
			// Convert to negative length if we're at the end...
			int64_t LongLength = min(0, (int64_t)Length + Start - TextLen);

			if (LongLength < INT_MIN)
				Length = INT_MIN;
			else
				Length = (int32_t)LongLength;

			Start = TextLen;
		}

		SelectInternal(Start, Length, TextLen);
	}

	void TextBoxBase::SelectAll()
	{
		auto Len = this->TextLength();
		SelectInternal(0, Len, Len);
	}

	void TextBoxBase::DeselectAll()
	{
		this->SetSelectionLength(0);
	}

	void TextBoxBase::ScrollToCaret()
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, EM_SCROLLCARET, NULL, NULL);
		else
			SetFlag(TextBoxFlags::FlagScrollToCaretOnHandleCreated, true);
	}

	CreateParams TextBoxBase::GetCreateParams()
	{
		auto Cp = Control::GetCreateParams();
		Cp.ClassName = "EDIT";
		Cp.Style |= ES_AUTOHSCROLL | ES_AUTOVSCROLL;

		if (!GetFlag(TextBoxFlags::FlagHideSelection))
			Cp.Style |= ES_NOHIDESEL;
		if (GetFlag(TextBoxFlags::FlagReadOnly))
			Cp.Style |= ES_READONLY;

		Cp.ExStyle &= (~WS_EX_CLIENTEDGE);
		Cp.Style &= (~WS_BORDER);

		switch (this->_BorderStyle)
		{
		case BorderStyle::Fixed3D:
			Cp.ExStyle |= WS_EX_CLIENTEDGE;
			break;
		case BorderStyle::FixedSingle:
			Cp.Style |= WS_BORDER;
			break;
		}

		if (GetFlag(TextBoxFlags::FlagMultiline))
		{
			Cp.Style |= ES_MULTILINE;

			if (GetFlag(TextBoxFlags::FlagWordWrap))
				Cp.Style &= ~ES_AUTOHSCROLL;
		}

		return Cp;
	}

	void TextBoxBase::SetWindowText(const string& Value)
	{
		// Override to prevent double OnTextChanged events
		if (this->WindowText() != Value)
		{
			SetFlag(TextBoxFlags::FlagCodeUpdateText, true);
			Control::SetWindowText(Value);
			SetFlag(TextBoxFlags::FlagCodeUpdateText, false);
		}
	}

	bool TextBoxBase::CanRaiseTextChangedEvent()
	{
		return true;
	}

	void TextBoxBase::UpdateMaxLength()
	{
		if (GetState(ControlStates::StateCreated))
			SendMessageA(this->_Handle, EM_LIMITTEXT, (WPARAM)this->_MaxLength, NULL);
	}

	void TextBoxBase::SetSelectedTextInternal(const string& Text, bool ClearUndo)
	{
		SendMessageA(this->_Handle, EM_LIMITTEXT, 0, 0);

		if (ClearUndo)
		{
			SendMessageA(this->_Handle, EM_REPLACESEL, 0, (LPARAM)(char*)Text);
			SendMessageA(this->_Handle, EM_SETMODIFY, 0, 0);
			this->ClearUndo();
		}
		else
			SendMessageA(this->_Handle, EM_REPLACESEL, -1, (LPARAM)(char*)Text);

		SendMessageA(this->_Handle, EM_LIMITTEXT, this->_MaxLength, NULL);
	}

	void TextBoxBase::SelectInternal(int32_t Start, int32_t Length, int32_t TextLen)
	{
		if (GetState(ControlStates::StateCreated))
		{
			int32_t St, Ed;
			AdjustSelectionStartAndEnd(Start, Length, St, Ed, TextLen);

			SendMessageA(this->_Handle, EM_SETSEL, (WPARAM)St, (LPARAM)Ed);
		}
		else
		{
			this->_SelectionStart = Start;
			this->_SelectionLength = Length;
			SetFlag(TextBoxFlags::FlagSetSelectionOnHandleCreated, true);
		}
	}

	int32_t TextBoxBase::GetEndPosition()
	{
		return GetState(ControlStates::StateCreated) ? TextLength() + 1 : TextLength();
	}

	void TextBoxBase::GetSelectionStartAndLength(int32_t& Start, int32_t& Length)
	{
		int32_t End = 0;

		if (!GetState(ControlStates::StateCreated))
		{
			AdjustSelectionStartAndEnd(this->_SelectionStart, this->_SelectionLength, Start, End, -1);
			Length = End - Start;
		}
		else
		{
			Start = 0;
			SendMessageA(this->_Handle, EM_GETSEL, (WPARAM)&Start, (LPARAM)&End);

			Start = (int32_t)max(0, Start);
			End = (int32_t)max(0, End);

			Length = End - Start;
		}
	}

	void TextBoxBase::AdjustSelectionStartAndEnd(int32_t SelStart, int32_t SelLength, int32_t& Start, int32_t& End, int32_t TextLen)
	{
		Start = SelStart;
		End = 0;

		if (Start <= -1)
			Start = -1;
		else
		{
			int32_t TextLength;

			if (TextLen >= 0)
				TextLength = TextLen;
			else
				TextLength = this->TextLength();

			if (Start > TextLength)
				Start = TextLength;

			// Easy check for overflow when we add them
			if ((int64_t)((int64_t)Start + (int64_t)SelLength) > INT_MAX)
				End = Start > 0 ? INT_MAX : INT_MIN;
			else
				End = Start + SelLength;

			// End must always be positive and less than total
			if (End < 0)
				End = 0;
			else if (End > TextLength)
				End = TextLength;
		}
	}

	bool TextBoxBase::Modified()
	{
		if (GetState(ControlStates::StateCreated))
		{
			auto CurState = (0 != (int)(long)SendMessageA(this->_Handle, EM_GETMODIFY, NULL, NULL));

			if (GetFlag(TextBoxFlags::FlagModified) != CurState)
			{
				SetFlag(TextBoxFlags::FlagModified, CurState);
				OnModifiedChanged();
			}

			return CurState;
		}

		return GetFlag(TextBoxFlags::FlagModified);
	}

	void TextBoxBase::SetModified(bool Value)
	{
		if (Modified() != Value)
		{
			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, EM_SETMODIFY, Value ? 1 : 0, NULL);

			SetFlag(TextBoxFlags::FlagModified, Value);
			OnModifiedChanged();
		}
	}

	bool TextBoxBase::Multiline()
	{
		return GetFlag(TextBoxFlags::FlagMultiline);
	}

	void TextBoxBase::SetMultiline(bool Value)
	{
		if (GetFlag(TextBoxFlags::FlagMultiline) != Value)
		{
			SetFlag(TextBoxFlags::FlagMultiline, Value);

			UpdateStyles();
			OnMultilineChanged();
		}
	}

	bool TextBoxBase::PasswordProtect()
	{
		return false;
	}

	bool TextBoxBase::ReadOnly()
	{
		return GetFlag(TextBoxFlags::FlagReadOnly);
	}

	void TextBoxBase::SetReadOnly(bool Value)
	{
		if (GetFlag(TextBoxFlags::FlagReadOnly) != Value)
		{
			SetFlag(TextBoxFlags::FlagReadOnly, Value);

			if (GetState(ControlStates::StateCreated))
				SendMessageA(this->_Handle, EM_SETREADONLY, Value ? -1 : 0, NULL);

			OnReadOnlyChanged();
		}
	}

	string TextBoxBase::SelectedText()
	{
		int32_t SelStart, SelLength;
		GetSelectionStartAndLength(SelStart, SelLength);

		return Text().Substring(SelStart, SelLength);
	}

	void TextBoxBase::SetSelectedText(const string& Value)
	{
		SetSelectedTextInternal(Value, true);
	}

	int32_t TextBoxBase::SelectionLength()
	{
		int32_t Length, Junk;
		GetSelectionStartAndLength(Junk, Length);

		return Length;
	}

	void TextBoxBase::SetSelectionLength(int32_t Value)
	{
		int32_t SelStart, SelLength;
		GetSelectionStartAndLength(SelStart, SelLength);

		if (Value != SelLength)
			Select(SelStart, Value);
	}

	int32_t TextBoxBase::SelectionStart()
	{
		int32_t Start, Junk;
		GetSelectionStartAndLength(Start, Junk);

		return Start;
	}

	void TextBoxBase::SetSelectionStart(int32_t Value)
	{
		Select(Value, this->SelectionLength());
	}

	bool TextBoxBase::GetFlag(TextBoxFlags Flag)
	{
		return ((int)this->_Flags & (int)Flag) == (int)Flag;
	}

	void TextBoxBase::SetFlag(TextBoxFlags Flags, bool Value)
	{
		this->_Flags = Value ? (TextBoxFlags)((int)this->_Flags | (int)Flags) : (TextBoxFlags)((int)this->_Flags & ~(int)Flags);
	}

	void TextBoxBase::WmReflectCommand(Message& Msg)
	{
		if (!GetFlag(TextBoxFlags::FlagCodeUpdateText))
		{
			if (HIWORD(Msg.WParam) == EN_CHANGE && CanRaiseTextChangedEvent())
				OnTextChanged();
			else if (HIWORD(Msg.WParam) == EN_UPDATE)
				Modified();
		}
	}

	void TextBoxBase::WmGetDlgCode(Message& Msg)
	{
		Control::WndProc(Msg);

		if (this->AcceptsTab())
			Msg.Result = (uintptr_t)((int)Msg.Result | DLGC_WANTTAB);
		else
			Msg.Result = (uintptr_t)((int)Msg.Result & ~(DLGC_WANTTAB | DLGC_WANTALLKEYS));
	}

	void TextBoxBase::WmSetFont(Message& Msg)
	{
		Control::WndProc(Msg);

		if (!GetFlag(TextBoxFlags::FlagMultiline))
			SendMessageA(this->_Handle, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
	}

	void TextBoxBase::WndProc(Message& Msg)
	{
		switch (Msg.Msg)
		{
		case WM_LBUTTONDBLCLK:
			_DoubleClickFired = true;
			Control::WndProc(Msg);
			break;
		case WM_REFLECT:
			WmReflectCommand(Msg);
			break;
		case WM_GETDLGCODE:
			WmGetDlgCode(Msg);
			break;
		case WM_SETFONT:
			WmSetFont(Msg);
			break;
		default:
			Control::WndProc(Msg);
			break;
		}
	}
}