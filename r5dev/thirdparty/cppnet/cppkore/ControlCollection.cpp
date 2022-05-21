#include "stdafx.h"
#include "ControlCollection.h"
#include "Control.h"

namespace Forms
{
	ControlCollection::~ControlCollection()
	{
		// We own the control as soon as the collection takes ahold of it...
		for (auto& Ctrl : this->_Controls)
			delete Ctrl;
	}

	void ControlCollection::Add(Control* Ctrl)
	{
		this->_Controls.Add(Ctrl);
	}

	void ControlCollection::Remove(Control* Ctrl)
	{
		if (Ctrl == nullptr)
			return;

		this->_Controls.Remove(Ctrl);
		delete Ctrl;
	}

	void ControlCollection::RemoveAt(uint32_t Index)
	{
		if (Index < this->_Controls.Count())
		{
			auto Ptr = this->_Controls[Index];
			this->_Controls.RemoveAt(Index);
			delete Ptr;
		}
	}

	int32_t ControlCollection::IndexOf(Control* Ctrl)
	{
		auto Result = this->_Controls.IndexOf(Ctrl);

		return (Result == List<Control*>::InvalidPosition) ? -1 : (int32_t)Result;
	}

	bool ControlCollection::Contains(Control* Ctrl)
	{
		return IndexOf(Ctrl) > -1;
	}

	void ControlCollection::SetChildIndex(Control* Ctrl, int32_t Index)
	{
		auto CurrentIndex = IndexOf(Ctrl);

		if (CurrentIndex == Index || CurrentIndex == -1)
			return;

		if (Index >= (int32_t)Count() || Index == -1)
		{
			Index = Count() - 1;
		}

		MoveElement(Ctrl, CurrentIndex, Index);
		Ctrl->UpdateZOrder();
	}

	uint32_t ControlCollection::Count()
	{
		return this->_Controls.Count();
	}

	Control* ControlCollection::operator[](size_t Index)
	{
		return (Control*)this->_Controls[Index];
	}

	Control* ControlCollection::begin() const noexcept
	{
		return (Control*)this->_Controls.begin();
	}

	Control* ControlCollection::end() const noexcept
	{
		return (Control*)this->_Controls.end();
	}

	void ControlCollection::MoveElement(Control* Ctrl, int32_t CurrentIndex, int32_t NewIndex)
	{
		int32_t Delta = NewIndex - CurrentIndex;

		switch (Delta)
		{
		case -1:
		case 1:
			this->_Controls[CurrentIndex] = this->_Controls[NewIndex];
			break;

		default:
		{
			int32_t Start = 0;
			int32_t Dest = 0;

			if (Delta > 0)
			{
				Start = CurrentIndex + 1;
				Dest = NewIndex;
			}
			else
			{
				Start = NewIndex;
				Dest = NewIndex + 1;

				Delta = -Delta;
			}

			CopyElement(Start, Dest, Delta);
		}
		break;
		}
		
		this->_Controls[NewIndex] = Ctrl;
	}

	void ControlCollection::CopyElement(int32_t SourceIndex, int32_t DestinationIndex, uint32_t Length)
	{
		if (SourceIndex < DestinationIndex)
		{
			SourceIndex = SourceIndex + Length;
			DestinationIndex = DestinationIndex + Length;

			for (; Length > 0; Length--)
			{
				this->_Controls[--DestinationIndex] = this->_Controls[--SourceIndex];
			}
		}
		else
		{
			for (; Length > 0; Length--)
			{
				this->_Controls[DestinationIndex++] = this->_Controls[SourceIndex++];
			}
		}
	}
}
