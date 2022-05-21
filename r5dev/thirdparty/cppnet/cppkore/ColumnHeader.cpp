#include "stdafx.h"
#include "ColumnHeader.h"
#include "ListView.h"

namespace Forms
{
	ColumnHeader::ColumnHeader()
		: ColumnHeader("")
	{
	}

	ColumnHeader::ColumnHeader(const string& Text)
		: ColumnHeader(Text, 60)
	{
	}

	ColumnHeader::ColumnHeader(const string& Text, int32_t Width)
		: ColumnHeader(Text, Width, HorizontalAlignment::Left)
	{
	}

	ColumnHeader::ColumnHeader(const string& Text, int32_t Width, HorizontalAlignment Alignment)
		: _Text(Text), _Width(Width), _TextAlign(Alignment), _OwnerListView(nullptr), _IndexInternal(-1)
	{
	}

	int32_t ColumnHeader::Index() const
	{
		if (_OwnerListView != nullptr)
			return _OwnerListView->Columns.IndexOf(*this);

		return -1;
	}

	int32_t ColumnHeader::DisplayIndex()
	{
		return this->_IndexInternal;
	}

	void ColumnHeader::SetDisplayIndex(int32_t Value)
	{
		if (this->_OwnerListView == nullptr)
		{
			this->_IndexInternal = Value;
			return;
		}

		auto LowDI = min(this->_IndexInternal, Value);
		auto HiDI = max(this->_IndexInternal, Value);
		auto ColsOrder = std::make_unique<int32_t[]>(_OwnerListView->Columns.Count());

		bool HdrMovedForward = Value > this->_IndexInternal;
		ColumnHeader* MovedHdr = nullptr;

		for (uint32_t i = 0; i < _OwnerListView->Columns.Count(); i++)
		{
			auto& Hdr = _OwnerListView->Columns[i];
			if (Hdr.DisplayIndex() == _IndexInternal)
				MovedHdr = &Hdr;
			else if (Hdr.DisplayIndex() >= LowDI && Hdr.DisplayIndex() <= HiDI)
				Hdr._IndexInternal -= HdrMovedForward ? 1 : -1;

			if (i != this->Index())
				ColsOrder[Hdr._IndexInternal] = i;
		}

		if (MovedHdr != nullptr)
		{
			MovedHdr->_IndexInternal = Value;
			ColsOrder[MovedHdr->_IndexInternal] = MovedHdr->Index();
		}

		SetDisplayIndices(ColsOrder, _OwnerListView->Columns.Count());
	}

	void ColumnHeader::SetDisplayIndexInternal(int32_t Value)
	{
		this->_IndexInternal = Value;
	}

	const string& ColumnHeader::Text() const
	{
		return this->_Text;
	}

	void ColumnHeader::SetText(const string& Value)
	{
		_Text = Value;

		if (_OwnerListView)
			_OwnerListView->SetColumnInfo(LVCF_TEXT, *this);
	}

	int32_t ColumnHeader::Width() const
	{
		if (_OwnerListView != nullptr && _OwnerListView->GetState(Forms::ControlStates::StateCreated) && _OwnerListView->GetState(ControlStates::StateCreated))
		{
			auto HwndHdr = (HWND)SendMessageA(_OwnerListView->GetHandle(), LVM_GETHEADER, NULL, NULL);
			if (HwndHdr != NULL)
			{
				auto NativeItemCount = (int32_t)SendMessageA(HwndHdr, HDM_GETITEMCOUNT, NULL, NULL);
				auto Idx = Index();
				if (Idx < NativeItemCount)
					return (int32_t)SendMessageA(_OwnerListView->GetHandle(), LVM_GETCOLUMNWIDTH, (WPARAM)Idx, 0);
			}
		}

		return this->_Width;
	}

	void ColumnHeader::SetWidth(int32_t Value)
	{
		_Width = Value;

		if (_OwnerListView)
			_OwnerListView->SetColumnWidth(this->Index(), _Width);
	}

	HorizontalAlignment ColumnHeader::TextAlign() const
	{
		return this->_TextAlign;
	}

	void ColumnHeader::SetTextAlign(HorizontalAlignment Value)
	{
		if (_TextAlign != Value)
		{
			_TextAlign = Value;

			if (_OwnerListView)
			{
				_OwnerListView->SetColumnInfo(LVCF_FMT, *this);
				_OwnerListView->Invalidate();
			}
		}
	}

	ListView* ColumnHeader::GetListView()
	{
		return this->_OwnerListView;
	}

	void ColumnHeader::SetListView(ListView* Owner)
	{
		this->_OwnerListView = Owner;
	}

	bool ColumnHeader::operator==(const ColumnHeader& Rhs)
	{
		return (this->_IndexInternal == Rhs._IndexInternal && this->_Text == Rhs._Text && this->_TextAlign == Rhs._TextAlign);
	}

	void ColumnHeader::SetDisplayIndices(const std::unique_ptr<int32_t[]>& Cols, int32_t Count)
	{
		if (this->_OwnerListView != nullptr && this->_OwnerListView->GetState(ControlStates::StateCreated))
			SendMessageA(this->_OwnerListView->GetHandle(), LVM_SETCOLUMNORDERARRAY, (WPARAM)Count, (LPARAM)Cols.get());
	}
}
