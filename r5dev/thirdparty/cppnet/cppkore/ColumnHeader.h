#pragma once

#include <cstdint>
#include "StringBase.h"
#include "HorizontalAlignment.h"

namespace Forms
{
	// Externally defined so we don't conflict
	class ListView;

	// Displays a single column header in a ListView control.
	class ColumnHeader
	{
	public:
		ColumnHeader();
		ColumnHeader(const string& Text);
		ColumnHeader(const string& Text, int32_t Width);
		ColumnHeader(const string& Text, int32_t Width, HorizontalAlignment Alignment);
		~ColumnHeader() = default;

		// The index of this column.
		int32_t Index() const;

		// The index of this column as it is displayed.
		int32_t DisplayIndex();
		// The index of this column as it is displayed.
		void SetDisplayIndex(int32_t Value);

		// Sets the display index without reflowing others.
		void SetDisplayIndexInternal(int32_t Value);

		// The text displayed in the column header.
		const string& Text() const;
		// The text displayed in the column header.
		void SetText(const string& Value);

		// The width of the column in pixels.
		int32_t Width() const;
		// The width of the column in pixels.
		void SetWidth(int32_t Value);

		// The horizontal alignment of the text contained in this column.
		HorizontalAlignment TextAlign() const;
		// The horizontal alignment of the text contained in this column.
		void SetTextAlign(HorizontalAlignment Value);

		// Returns the ListView control that this column is displayed in.  May be null.
		ListView* GetListView();
		// Sets the ListView control that this column is displayed in.
		void SetListView(ListView* Owner);

		// Custom equality operator
		bool operator==(const ColumnHeader& Rhs);

	private:
		// Internal cached properties
		ListView* _OwnerListView;

		// Width and index
		int32_t _Width;
		int32_t _IndexInternal;

		// Text to display
		string _Text;
		HorizontalAlignment _TextAlign;

		// Set the display indices of the ListView columns.
		void SetDisplayIndices(const std::unique_ptr<int32_t[]>& Cols, int32_t Count);
	};
}