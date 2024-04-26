#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "imgui_logger.h"
#include "imgui.h"
#include "imgui_internal.h"

template<class InputIt1, class InputIt2, class BinaryPredicate>
static bool equals(InputIt1 first1, InputIt1 last1,
	InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
	for (; first1 != last1 && first2 != last2; ++first1, ++first2)
	{
		if (!p(*first1, *first2))
			return false;
	}
	return first1 == last1 && first2 == last2;
}

CTextLogger::CTextLogger()
	: m_bAutoScroll(true)
	, m_bScrollToCursor(false)
	, m_bScrollToStart(false)
	, m_bScrolledToStart(false)
	, m_bScrollToBottom(true)
	, m_bScrolledToBottom(false)
	, m_bHandleUserInputs(true)
	, m_bWithinLoggingRect(false)
	, m_nTabSize(4)
	, m_nLeftMargin(0)
	, m_flLineSpacing(1.0f)
	, m_SelectionMode(SelectionMode::Normal)
	, m_flLastClick(-1.0)
	, m_flCursorBlinkerStartTime(-1.0f)
{
	m_Lines.push_back(Line());
}

CTextLogger::~CTextLogger()
{
}

std::string CTextLogger::GetText(const Coordinates& aStart, const Coordinates& aEnd) const
{
	std::string result;

	int lstart = aStart.m_nLine;
	int lend = aEnd.m_nLine;
	int istart = GetCharacterIndex(aStart);
	int iend = GetCharacterIndex(aEnd);
	size_t s = 0;

	for (int i = lstart; i < lend; i++)
		s += m_Lines[i].Length();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= static_cast<int>(m_Lines.size()))
			break;

		const Line& line = m_Lines[lstart];
		if (istart < line.Length())
		{
			result += line.buffer[istart];
			istart++;
		}
		else
		{
			istart = 0;
			++lstart;
		}
	}

	return result;
}

CTextLogger::Coordinates CTextLogger::GetActualLastLineCoordinates() const
{
	return SanitizeCoordinates(Coordinates(GetTotalLines(), 0));
}

CTextLogger::Coordinates CTextLogger::GetActualCursorCoordinates() const
{
	return SanitizeCoordinates(m_State.m_CursorPosition);
}

CTextLogger::Coordinates CTextLogger::SanitizeCoordinates(const Coordinates & aValue) const
{
	int line = aValue.m_nLine;
	int column = aValue.m_nColumn;
	if (line >= static_cast<int>(m_Lines.size()))
	{
		if (m_Lines.empty())
		{
			line = 0;
			column = 0;
		}
		else
		{
			line = static_cast<int>(m_Lines.size() - 1);
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	}
	else
	{
		column = m_Lines.empty() ? 0 : ImMin(column, GetLineMaxColumn(line));
		return Coordinates(line, column);
	}
}

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(CTextLogger::Char c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}

bool UTF8StringValid(const char* pszString)
{
	size_t byteCount = 0;
	CTextLogger::Char currentByte;

	while (*pszString)
	{
		currentByte = static_cast<CTextLogger::Char>(*pszString);
		if (byteCount)
		{
			if ((currentByte & 0xC0) != 0x80)
				return false;

			byteCount--;
		}
		else
		{
			byteCount = UTF8CharLength(currentByte) - 1;
			if (byteCount > 0 && (currentByte & (1 << (7 - byteCount))) == 0)
				return false;
		}
		pszString++;
	}

	return byteCount == 0;
}

void CTextLogger::Advance(Coordinates& aCoordinates) const
{
	if (aCoordinates.m_nLine < static_cast<int>(m_Lines.size()))
	{
		const Line& line = m_Lines[aCoordinates.m_nLine];
		int cindex = GetCharacterIndex(aCoordinates);

		if (cindex + 1 < line.Length())
		{
			int delta = UTF8CharLength(line.buffer[cindex]);
			cindex = ImMin(cindex + delta, static_cast<int>(line.Length() - 1));
		}
		else
		{
			++aCoordinates.m_nLine;
			cindex = 0;
		}
		aCoordinates.m_nColumn = GetCharacterColumn(aCoordinates.m_nLine, cindex);
	}
}

//void CTextLogger::DeleteRange(const Coordinates & aStart, const Coordinates & aEnd)
//{
//	assert(aEnd >= aStart);
//
//	//printf("D(%d.%d)-(%d.%d)\n", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);
//
//	if (aEnd == aStart)
//		return;
//
//	int start = GetCharacterIndex(aStart);
//	int end = GetCharacterIndex(aEnd);
//
//	if (aStart.m_nLine == aEnd.m_nLine)
//	{
//		Line& line = m_Lines[aStart.m_nLine];
//		int n = GetLineMaxColumn(aStart.m_nLine);
//		if (aEnd.m_nColumn >= n)
//			line.erase(line.begin() + start, line.end());
//		else
//			line.erase(line.begin() + start, line.begin() + end);
//	}
//	else
//	{
//		Line& firstLine = m_Lines[aStart.m_nLine];
//		Line& lastLine = m_Lines[aEnd.m_nLine];
//
//		firstLine.erase(firstLine.begin() + start, firstLine.end());
//		lastLine.erase(lastLine.begin(), lastLine.begin() + end);
//
//		if (aStart.m_nLine < aEnd.m_nLine)
//			firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());
//
//		if (aStart.m_nLine < aEnd.m_nLine)
//			RemoveLine(aStart.m_nLine + 1, aEnd.m_nLine + 1);
//	}
//}

void CTextLogger::MarkNewline(Coordinates& aWhere,  int aIndex)
{
	Line& newLine = InsertLine(aWhere.m_nLine + 1);
	Line& line = m_Lines[aWhere.m_nLine];

	if (aIndex < static_cast<int>(m_Lines[aWhere.m_nLine].buffer.size()))
	{
		newLine.buffer.insert(newLine.buffer.begin(), line.buffer.begin() + aIndex, line.buffer.end());
		line.buffer.erase(line.buffer.begin() + aIndex, line.buffer.end());
	}
	else
		line.buffer.push_back('\n');

	++aWhere.m_nLine;
	aWhere.m_nColumn = 0;
}

int CTextLogger::InsertTextAt(Coordinates& aWhere, const char* aValue, const ImU32 aColor)
{
	int cindex = GetCharacterIndex(aWhere);
	int totalLines = 0;

	if (!UTF8StringValid(aValue))
	{
		assert(0);
		aValue = "Invalid UTF-8 string\n";
	}

	while (*aValue != '\0')
	{
		assert(!m_Lines.empty());

		if (*aValue == '\r')
		{
			// skip
			++aValue;
		}
		else if (*aValue == '\n')
		{
			MarkNewline(aWhere, cindex);
			cindex = 0;
			++totalLines;
			++aValue;
		}
		else
		{
			Line& line = m_Lines[aWhere.m_nLine];

			// if the buffer isn't empty, and the next log isn't from the same context, log it as a new line
			if (!line.buffer.empty() && aColor != line.color)
			{
				MarkNewline(aWhere, cindex);
				cindex = 0;
				++totalLines;
				continue;
			}

			size_t d = UTF8CharLength(*aValue);
			while (d-- > 0 && *aValue != '\0')
			{
				if (cindex >= 0 && cindex <= line.Length())
					line.buffer.insert(line.buffer.begin() + cindex++, *aValue++);
				else
					++aValue; // Possibly an invalid character
			}

			line.color = aColor;
			++aWhere.m_nColumn;
		}
	}

	if (!*aValue)
	{
		Line& line = m_Lines[aWhere.m_nLine];
		if (!line.buffer.empty() && cindex >= 0 && cindex <= line.Length())
			line.buffer.insert(line.buffer.begin() + cindex, ' ');
	}

	return totalLines;
}

CTextLogger::Coordinates CTextLogger::ScreenPosToCoordinates(const ImVec2& aPosition) const
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

	int lineNo = ImMax(0, static_cast<int>(ImFloor(local.y / m_CharAdvance.y)));
	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < static_cast<int>(m_Lines.size()))
	{
		const Line& line = m_Lines[lineNo];

		int columnIndex = 0;
		float columnX = 0.0f;

		while (columnIndex < line.Length())
		{
			float columnWidth = 0.0f;

			if (line.buffer[columnIndex] == '\t')
			{
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + ImFloor((1.0f + columnX) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
				columnWidth = newColumnX - oldX;

				if (columnX + columnWidth * 0.5f > local.x)
					break;

				columnX = newColumnX;
				columnCoord = (columnCoord / m_nTabSize) * m_nTabSize + m_nTabSize;
				columnIndex++;
			}
			else
			{
				char buf[7];
				size_t d = UTF8CharLength(line.buffer[columnIndex]);
				size_t i = 0;

				while (i < 6 && d-- > 0 && columnIndex < line.Length())
					buf[i++] = line.buffer[columnIndex++];

				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;

				if (columnX + columnWidth * 0.5f > local.x)
					break;

				columnX += columnWidth;
				columnCoord++;
			}
		}
	}

	return SanitizeCoordinates(Coordinates(lineNo, columnCoord));
}

CTextLogger::Coordinates CTextLogger::FindWordStart(const Coordinates & aFrom) const
{
	Coordinates at = aFrom;
	if (at.m_nLine >= static_cast<int>(m_Lines.size()))
		return at;

	const Line& line = m_Lines[at.m_nLine];
	int cindex = GetCharacterIndex(at);

	if (cindex >= line.Length())
		return at;

	while (cindex > 0 && isspace(line.buffer[cindex]))
		--cindex;

	while (cindex > 0)
	{
		Char c = line.buffer[cindex];
		if ((c & 0xC0) != 0x80)	// not UTF code sequence 10xxxxxx
		{
			if (c <= 32 && isspace(c))
			{
				cindex++;
				break;
			}
		}
		--cindex;
	}

	return Coordinates(at.m_nLine, GetCharacterColumn(at.m_nLine, cindex));
}

CTextLogger::Coordinates CTextLogger::FindWordEnd(const Coordinates & aFrom) const
{
	Coordinates at = aFrom;
	if (at.m_nLine >= static_cast<int>(m_Lines.size()))
		return at;

	const Line& line = m_Lines[at.m_nLine];
	int cindex = GetCharacterIndex(at);

	if (cindex >= line.Length())
		return at;

	bool prevspace = static_cast<bool>(isspace(line.buffer[cindex]));
	while (cindex < line.Length())
	{
		Char c = line.buffer[cindex];
		int d = UTF8CharLength(c);

		if (prevspace != !!isspace(c))
		{
			if (isspace(c))
				while (cindex < line.Length() && !isspace(line.buffer[cindex]))
					++cindex;
			break;
		}
		cindex += d;
	}

	return Coordinates(aFrom.m_nLine, GetCharacterColumn(aFrom.m_nLine, cindex));
}

CTextLogger::Coordinates CTextLogger::FindNextWord(const Coordinates & aFrom) const
{
	Coordinates at = aFrom;
	if (at.m_nLine >= static_cast<int>(m_Lines.size()))
		return at;

	// skip to the next non-word character
	int cindex = GetCharacterIndex(aFrom);
	bool isword = false;
	bool skip = false;

	if (cindex < static_cast<int>(m_Lines[at.m_nLine].buffer.size()))
	{
		const Line& line = m_Lines[at.m_nLine];
		isword = isalnum(line.buffer[cindex]);
		skip = isword;
	}

	while (!isword || skip)
	{
		if (at.m_nLine >= static_cast<int>(m_Lines.size()))
		{
			int l = ImMax(0, static_cast<int>(m_Lines.size() - 1));
			return Coordinates(l, GetLineMaxColumn(l));
		}

		const Line& line = m_Lines[at.m_nLine];
		if (cindex < line.Length())
		{
			isword = isalnum(line.buffer[cindex]);

			if (isword && !skip)
				return Coordinates(at.m_nLine, GetCharacterColumn(at.m_nLine, cindex));

			if (!isword)
				skip = false;

			cindex++;
		}
		else
		{
			cindex = 0;
			++at.m_nLine;
			skip = false;
			isword = false;
		}
	}

	return at;
}

int CTextLogger::GetCharacterIndex(const Coordinates& aCoordinates) const
{
	if (aCoordinates.m_nLine >= static_cast<int>(m_Lines.size()))
		return -1;

	const Line& line = m_Lines[aCoordinates.m_nLine];
	int c = 0;
	int i = 0;

	for (; i < line.Length() && c < aCoordinates.m_nColumn;)
	{
		if (line.buffer[i] == '\t')
			c = (c / m_nTabSize) * m_nTabSize + m_nTabSize;
		else
			++c;
		i += UTF8CharLength(line.buffer[i]);
	}
	return i;
}

int CTextLogger::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= static_cast<int>(m_Lines.size()))
		return 0;

	const Line& line = m_Lines[aLine];
	int col = 0;
	int i = 0;

	while (i < aIndex && i < line.Length())
	{
		Char c = line.buffer[i];
		i += UTF8CharLength(c);
		if (c == '\t')
			col = (col / m_nTabSize) * m_nTabSize + m_nTabSize;
		else
			col++;
	}
	return col;
}

int CTextLogger::GetLineCharacterCount(int aLine) const
{
	if (aLine >= static_cast<int>(m_Lines.size()))
		return 0;

	const Line& line = m_Lines[aLine];
	int c = 0;

	for (size_t i = 0; i < line.Length(); c++)
		i += static_cast<size_t>(UTF8CharLength(m_Lines[aLine].buffer[i]));
	return c;
}

int CTextLogger::GetLineMaxColumn(int aLine) const
{
	if (aLine >= static_cast<int>(m_Lines.size()))
		return 0;

	const Line& line = m_Lines[aLine];
	int col = 0;

	for (size_t i = 0; i < line.Length(); )
	{
		Char c = line.buffer[i];
		if (c == '\t')
			col = (col / m_nTabSize) * m_nTabSize + m_nTabSize;
		else
			col++;
		i += static_cast<size_t>(UTF8CharLength(c));
	}
	return col;
}

bool CTextLogger::IsOnWordBoundary(const Coordinates & aAt) const
{
	if (aAt.m_nLine >= static_cast<int>(m_Lines.size()) || aAt.m_nColumn == 0)
		return true;

	const Line& line = m_Lines[aAt.m_nLine];
	size_t cindex = static_cast<size_t>(GetCharacterIndex(aAt));

	if (cindex >= line.Length())
		return true;

	return isspace(line.buffer[cindex]) != isspace(line.buffer[cindex - 1]);
}

void CTextLogger::RemoveLine(int aStart, int aEnd)
{
	assert(aEnd >= aStart);
	assert(m_Lines.size() > (size_t)(aEnd - aStart));

	m_Lines.erase(m_Lines.begin() + aStart, m_Lines.begin() + aEnd);
	assert(!m_Lines.empty());
}

void CTextLogger::RemoveLine(int aIndex)
{
	assert(m_Lines.size() > 1);

	m_Lines.erase(m_Lines.begin() + aIndex);
	assert(!m_Lines.empty());
}

// TODO[ AMOS ]: rename to InsertBlankLine ?
CTextLogger::Line& CTextLogger::InsertLine(int aIndex)
{
	Line& result = *m_Lines.insert(m_Lines.begin() + aIndex, Line());
	return result;
}

std::string CTextLogger::GetWordUnderCursor() const
{
	const Coordinates c = GetCursorPosition();
	return GetWordAt(c);
}

std::string CTextLogger::GetWordAt(const Coordinates & aCoords) const
{
	const Coordinates start = FindWordStart(aCoords);
	const Coordinates end = FindWordEnd(aCoords);

	std::string r;

	int istart = GetCharacterIndex(start);
	int iend = GetCharacterIndex(end);

	for (int it = istart; it < iend; ++it)
		r.push_back(m_Lines[aCoords.m_nLine].buffer[it]);

	return r;
}

//ImU32 CTextLogger::GetGlyphColor(const Glyph & aGlyph) const
//{
//	ImVec4 color = aGlyph.m_Color;
//	return ImGui::ColorConvertFloat4ToU32(color);
//}

void CTextLogger::HandleKeyboardInputs(bool bHoveredScrollbar, bool bActiveScrollbar)
{
	ImGuiIO& io = ImGui::GetIO();
	bool shift = io.KeyShift;
	bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	bool alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (!bActiveScrollbar && ImGui::IsWindowFocused())
	{
		if (!bHoveredScrollbar && ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
			MoveUp(1, shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown(1, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(1, shift, ctrl);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
			MoveUp(GetPageSize() - 4, shift);
		else if (!alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
			MoveDown(GetPageSize() - 4, shift);
		else if (!alt && ctrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
			MoveTop(shift);
		else if (ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
			MoveBottom(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
			MoveHome(shift);
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
			MoveEnd(shift);
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			SelectAll();
	}
}

void CTextLogger::HandleMouseInputs(bool bHoveredScrollbar, bool bActiveScrollbar)
{
	ImGuiIO& io = ImGui::GetIO();

	bool bShift = io.KeyShift;
	bool bCtrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	bool bAlt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;


	if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
		m_bWithinLoggingRect = true;
	else if (ImGui::IsMouseReleased(0))
		m_bWithinLoggingRect = false;

	if (!bHoveredScrollbar && !bActiveScrollbar && m_bWithinLoggingRect)
	{
		bool click = ImGui::IsMouseClicked(0);

		if (!bShift && !bAlt)
		{
			bool doubleClick = ImGui::IsMouseDoubleClicked(0);

			double t = ImGui::GetTime();
			bool tripleClick = click && !doubleClick && (m_flLastClick != -1.0 && (t - m_flLastClick) < io.MouseDoubleClickTime);

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (!bCtrl)
				{
					m_State.m_CursorPosition = m_InteractiveStart = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					m_SelectionMode = SelectionMode::Line;
					SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);
				}

				m_flLastClick = -1.0;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (!bCtrl)
				{
					m_State.m_CursorPosition = m_InteractiveStart = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());

					// Advance cursor to the end of the selection
					m_InteractiveStart = FindWordStart(m_State.m_CursorPosition);
					m_State.m_CursorPosition = m_InteractiveEnd = FindWordEnd(m_State.m_CursorPosition);

					if (m_SelectionMode == SelectionMode::Line)
						m_SelectionMode = SelectionMode::Normal;
					else
						m_SelectionMode = SelectionMode::Word;
					SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);
				}

				m_flLastClick = ImGui::GetTime();
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				m_State.m_CursorPosition = m_InteractiveStart = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				if (bCtrl)
					m_SelectionMode = SelectionMode::Word;
				else
					m_SelectionMode = SelectionMode::Normal;
				SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);

				m_flLastClick = ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
			{
				m_SelectionMode = SelectionMode::Normal;
				io.WantCaptureMouse = true;

				m_State.m_CursorPosition = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());

				SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);
				EnsureCursorVisible();
			}
		}
		else if (bShift)
		{
			if (click) // Shift select range
			{
				const Coordinates newCursorPos = ScreenPosToCoordinates(ImGui::GetMousePos());

				// Set selection from old cursor pos to new cursor pos
				m_SelectionMode = SelectionMode::Normal;
				SetSelection(m_State.m_CursorPosition, newCursorPos, m_SelectionMode);

				m_InteractiveStart = m_State.m_SelectionStart;
				m_InteractiveEnd = m_State.m_SelectionEnd;
			}
		}
	}
}

void CTextLogger::Render()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	ImGuiWindow* const pWindow = ImGui::GetCurrentWindow();
	const ImGuiStyle& style = ImGui::GetStyle();

	const ImGuiID activeID = ImGui::GetActiveID();
	const ImGuiID hoveredID = ImGui::GetHoveredID();

	const bool bHoveredScrollbar = hoveredID && (hoveredID == ImGui::GetWindowScrollbarID(pWindow, ImGuiAxis_X) || hoveredID == ImGui::GetWindowScrollbarID(pWindow, ImGuiAxis_Y));
	const bool bActiveScrollbar = activeID && (activeID == ImGui::GetWindowScrollbarID(pWindow, ImGuiAxis_X) || activeID == ImGui::GetWindowScrollbarID(pWindow, ImGuiAxis_Y));

	if (m_bHandleUserInputs)
	{
		HandleKeyboardInputs(bHoveredScrollbar, bActiveScrollbar);
		ImGui::PushAllowKeyboardFocus(true);

		HandleMouseInputs(bHoveredScrollbar, bActiveScrollbar);
	}

	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	m_CharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * m_flLineSpacing);

	const ImVec2 contentSize = ImGui::GetWindowContentRegionMax();
	ImDrawList* const drawList = ImGui::GetWindowDrawList();

	const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

	float longest = 0.0f;
	const float scrollY = ImGui::GetScrollY();

	int lineNo = static_cast<int>(ImFloor(scrollY / m_CharAdvance.y));
	const int lineMax = ImMax(0, ImMin(static_cast<int>(m_Lines.size()) - 1, lineNo + static_cast<int>(ImFloor((scrollY + contentSize.y) / m_CharAdvance.y))));

	if (!m_Lines.empty())
	{
		while (lineNo <= lineMax)
		{
			const ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * m_CharAdvance.y);
			const ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x, lineStartScreenPos.y);

			const Line& line = m_Lines[lineNo];
			longest = ImMax(TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);

			Coordinates lineStartCoord(lineNo, 0);
			Coordinates lineEndCoord(lineNo, GetLineMaxColumn(lineNo));

			// Draw selection for the current line
			float sstart = -1.0f;
			float ssend = -1.0f;

			assert(m_State.m_SelectionStart <= m_State.m_SelectionEnd);
			if (m_State.m_SelectionStart <= lineEndCoord)
				sstart = m_State.m_SelectionStart > lineStartCoord ? TextDistanceToLineStart(m_State.m_SelectionStart) : 0.0f;
			if (m_State.m_SelectionEnd > lineStartCoord)
				ssend = TextDistanceToLineStart(m_State.m_SelectionEnd < lineEndCoord ? m_State.m_SelectionEnd : lineEndCoord);

			if (m_State.m_SelectionEnd.m_nLine > lineNo)
				ssend += m_CharAdvance.x;

			if (sstart != -1 && ssend != -1 && sstart < ssend)
			{
				const ImVec2 vstart(lineStartScreenPos.x + sstart, lineStartScreenPos.y);
				const ImVec2 vend(lineStartScreenPos.x + ssend, lineStartScreenPos.y + m_CharAdvance.y);

				drawList->AddRectFilled(vstart, vend, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
			}

			// Render the cursor
			if (m_State.m_CursorPosition.m_nLine == lineNo && ImGui::IsWindowFocused())
			{
				// Initialize the cursor start render time, this is done here
				// as Dear ImGui typically isn't initialized during the
				// construction of this class
				if (m_flCursorBlinkerStartTime == -1.0)
					m_flCursorBlinkerStartTime = ImGui::GetTime();

				const double currTime = ImGui::GetTime();
				const double elapsed = currTime - m_flCursorBlinkerStartTime;

				if (elapsed > 0.4)
				{
					const float width = 1.0f;
					const float cx = TextDistanceToLineStart(m_State.m_CursorPosition);

					const ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
					const ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + m_CharAdvance.y);

					drawList->AddRectFilled(cstart, cend, 0xffe0e0e0);

					if (elapsed > 0.8)
						m_flCursorBlinkerStartTime = currTime;
				}
			}

			if (!line.buffer.empty())
			{
				ImU32 color = line.color;

				if (m_itFilter.IsActive())
				{
					// Make line dark if it isn't found by the filter
					if (!m_itFilter.PassFilter(line.buffer.c_str()))
						color = 0xff605040;
				}

				const ImVec2 newOffset(textScreenPos.x, textScreenPos.y);
				drawList->AddText(newOffset, color, line.buffer.c_str());
			}

			++lineNo;
		}
	}

	// This dummy is here to let Dear ImGui know where the last character of
	// the line had ended, so that it could properly process the horizontal
	// scrollbar
	const float additional = pWindow->ScrollbarY ? style.ScrollbarSize : 0.0f;
	ImGui::Dummy(ImVec2(longest + additional, m_Lines.size() * m_CharAdvance.y));

	m_bScrolledToStart = ImGui::GetScrollX() == 0.0f;

	if (m_bScrollToStart || (m_bAutoScroll && m_bScrolledToStart && !m_bScrollToCursor))
	{
		ImGui::SetScrollHereX(0.0f);
		m_bScrollToStart = false;
	}

	m_bScrolledToBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

	if (m_bScrollToBottom || (m_bAutoScroll && m_bScrolledToBottom && !m_bScrollToCursor))
	{
		ImGui::SetScrollHereY(1.0f);
		m_bScrollToBottom = false;
	}

	m_bScrollToCursor = false;

	if (m_bHandleUserInputs)
		ImGui::PopAllowKeyboardFocus();

	ImGui::PopStyleVar();
}

void CTextLogger::Copy(bool aCopyAll)
{
	if (!aCopyAll && HasSelection())
	{
		ImGui::SetClipboardText(GetSelectedText().c_str());
	}
	else if (!aCopyAll)
	{
		if (!m_Lines.empty())
		{
			const Line& line = m_Lines[GetActualCursorCoordinates().m_nLine];
			ImGui::SetClipboardText(line.buffer.c_str());
		}
	}
	else // Copy all lines to clipboard.
	{
		std::string str;
		for (const Line& line : m_Lines)
		{
			str.append(line.buffer);
		}

		ImGui::SetClipboardText(str.c_str());
	}
}

//void CTextLogger::SetText(const ConLog_t& aText)
//{
//	m_Lines.clear();
//	m_Lines.emplace_back(Line());
//	for (char chr : aText.m_svConLog)
//	{
//		if (chr == '\r') // ignore the carriage return character
//			continue;
//		else if (chr == '\n')
//			m_Lines.emplace_back(Line());
//		else
//			m_Lines.back().emplace_back(Glyph(chr, aText.m_imColor));
//	}
//}

//void CTextLogger::SetTextLines(const std::vector<ConLog_t>& aLines)
//{
//	m_Lines.clear();
//
//	if (aLines.empty())
//	{
//		m_Lines.emplace_back(Line());
//	}
//	else
//	{
//		m_Lines.resize(aLines.size());
//
//		for (size_t i = 0; i < aLines.size(); ++i)
//		{
//			const std::string & aLine = aLines[i].m_svConLog;
//
//			m_Lines[i].reserve(aLine.size());
//			for (size_t j = 0; j < aLine.size(); ++j)
//			{
//				m_Lines[i].emplace_back(Glyph(aLine[j], aLines[i].m_imColor));
//			}
//		}
//	}
//}

void CTextLogger::SetCursorPosition(const Coordinates & aPosition)
{
	if (m_State.m_CursorPosition != aPosition)
	{
		m_State.m_CursorPosition = aPosition;
		EnsureCursorVisible();
	}
}

void CTextLogger::SetSelectionStart(const Coordinates & aPosition)
{
	m_State.m_SelectionStart = SanitizeCoordinates(aPosition);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		ImSwap(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

void CTextLogger::SetSelectionEnd(const Coordinates & aPosition)
{
	m_State.m_SelectionEnd = SanitizeCoordinates(aPosition);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		ImSwap(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

void CTextLogger::SetSelection(const Coordinates & aStart, const Coordinates & aEnd, SelectionMode aMode)
{
	Coordinates oldSelStart = m_State.m_SelectionStart;
	Coordinates oldSelEnd = m_State.m_SelectionEnd;

	m_State.m_SelectionStart = SanitizeCoordinates(aStart);
	m_State.m_SelectionEnd = SanitizeCoordinates(aEnd);

	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		ImSwap(m_State.m_SelectionStart, m_State.m_SelectionEnd);

	switch (aMode)
	{
	case CTextLogger::SelectionMode::Normal:
		break;
	case CTextLogger::SelectionMode::Word:
	{
		m_State.m_SelectionStart = FindWordStart(m_State.m_SelectionStart);
		if (!IsOnWordBoundary(m_State.m_SelectionEnd))
			m_State.m_SelectionEnd = FindWordEnd(FindWordStart(m_State.m_SelectionEnd));
		break;
	}
	case CTextLogger::SelectionMode::Line:
	{
		const int lineNo = m_State.m_SelectionEnd.m_nLine;
		//const size_t lineSize = (size_t)lineNo < m_Lines.size() ? m_Lines[lineNo].size() : 0;
		m_State.m_SelectionStart = Coordinates(m_State.m_SelectionStart.m_nLine, 0);
		m_State.m_SelectionEnd = m_Lines.size() > lineNo + 1 ? Coordinates(lineNo + 1, 0) : Coordinates(lineNo, GetLineMaxColumn(lineNo));
		m_State.m_CursorPosition = m_State.m_SelectionEnd;
		break;
	}
	default:
		break;
	}
}

void CTextLogger::SetTabSize(int aValue)
{
	m_nTabSize = ImMax(0, ImMin(32, aValue));
}

void CTextLogger::InsertText(const char* const text, const ImU32 color)
{
	IM_ASSERT(text);

	if (!*text)
		return;

	Coordinates pos = GetActualLastLineCoordinates();

	const Coordinates& start = ImMin(pos, m_State.m_SelectionStart);
	int totalLines = pos.m_nLine - start.m_nLine;

	totalLines += InsertTextAt(pos, text, color);
}

void CTextLogger::MoveUp(int aAmount, bool aSelect)
{
	const Coordinates oldPos = m_State.m_CursorPosition;
	m_State.m_CursorPosition.m_nLine = ImMax(0, m_State.m_CursorPosition.m_nLine - aAmount);
	if (oldPos != m_State.m_CursorPosition)
	{
		if (aSelect)
		{
			if (oldPos == m_InteractiveStart)
				m_InteractiveStart = m_State.m_CursorPosition;
			else if (oldPos == m_InteractiveEnd)
				m_InteractiveEnd = m_State.m_CursorPosition;
			else
			{
				m_InteractiveStart = m_State.m_CursorPosition;
				m_InteractiveEnd = oldPos;
			}
		}
		else
			m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;

		SetSelection(m_InteractiveStart, m_InteractiveEnd, SelectionMode::Normal);
		EnsureCursorVisible();
	}
}

void CTextLogger::MoveDown(int aAmount, bool aSelect)
{
	assert(m_State.m_CursorPosition.m_nColumn >= 0);
	Coordinates oldPos = m_State.m_CursorPosition;

	m_State.m_CursorPosition.m_nLine = ImMax(0, ImMin(static_cast<int>(m_Lines.size() - 1), m_State.m_CursorPosition.m_nLine + aAmount));

	if (m_State.m_CursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == m_InteractiveEnd)
				m_InteractiveEnd = m_State.m_CursorPosition;
			else if (oldPos == m_InteractiveStart)
				m_InteractiveStart = m_State.m_CursorPosition;
			else
			{
				m_InteractiveStart = oldPos;
				m_InteractiveEnd = m_State.m_CursorPosition;
			}
		}
		else
			m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;

		SetSelection(m_InteractiveStart, m_InteractiveEnd, SelectionMode::Normal);
		EnsureCursorVisible();
	}
}

static bool IsUTFSequence(char c)
{
	return (c & 0xC0) == 0x80;
}

void CTextLogger::MoveLeft(int aAmount, bool aSelect, bool aWordMode)
{
	if (m_Lines.empty())
		return;

	const Coordinates oldPos = m_State.m_CursorPosition;
	m_State.m_CursorPosition = GetActualCursorCoordinates();

	int line = m_State.m_CursorPosition.m_nLine;
	int cindex = GetCharacterIndex(m_State.m_CursorPosition);

	while (aAmount-- > 0)
	{
		if (cindex == 0)
		{
			if (line > 0)
			{
				--line;
				if (static_cast<int>(m_Lines.size()) > line)
					cindex = static_cast<int>(m_Lines[line].buffer.size());
				else
					cindex = 0;
			}
		}
		else
		{
			--cindex;
			if (cindex > 0)
			{
				if (static_cast<int>(m_Lines.size()) > line)
				{
					const Line &lineData = m_Lines[line];

					while (cindex > 0 && IsUTFSequence(lineData.buffer[cindex]))
						--cindex;

					// Skip the newline character.
					if (cindex > 0 && lineData.buffer[cindex] == '\n')
						--cindex;
				}
			}
		}

		m_State.m_CursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));
		if (aWordMode)
		{
			m_State.m_CursorPosition = FindWordStart(m_State.m_CursorPosition);
			cindex = GetCharacterIndex(m_State.m_CursorPosition);
		}
	}

	m_State.m_CursorPosition = Coordinates(line, GetCharacterColumn(line, cindex));

	assert(m_State.m_CursorPosition.m_nColumn >= 0);
	if (aSelect)
	{
		if (oldPos == m_InteractiveStart)
			m_InteractiveStart = m_State.m_CursorPosition;
		else if (oldPos == m_InteractiveEnd)
			m_InteractiveEnd = m_State.m_CursorPosition;
		else
		{
			m_InteractiveStart = m_State.m_CursorPosition;
			m_InteractiveEnd = oldPos;
		}
	}
	else
	{
		if (HasSelection() && !aWordMode)
			m_State.m_CursorPosition = m_InteractiveStart;

		m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
	}

	SetSelection(m_InteractiveStart, m_InteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void CTextLogger::MoveRight(int aAmount, bool aSelect, bool aWordMode)
{
	const Coordinates oldPos = m_State.m_CursorPosition;

	if (m_Lines.empty() || oldPos.m_nLine >= static_cast<int>(m_Lines.size()))
		return;

	int cindex = GetCharacterIndex(m_State.m_CursorPosition);
	while (aAmount-- > 0)
	{
		int lindex = m_State.m_CursorPosition.m_nLine;
		const Line& line = m_Lines[lindex];

		bool isNewLine = false;
		const bool isLastChar = (cindex >= line.Length()-1);

		// If the cursor is at the last character before the newline character,
		// we want to skip the newline character and move to the next line.
		if (isLastChar && !line.buffer.empty())
			isNewLine = line.buffer.back() == '\n';

		if (cindex >= line.Length() || isNewLine)
		{
			if (m_State.m_CursorPosition.m_nLine < static_cast<int>(m_Lines.size()) - 1)
			{
				m_State.m_CursorPosition.m_nLine = ImMax(0, ImMin(static_cast<int>(m_Lines.size()) - 1, m_State.m_CursorPosition.m_nLine + 1));
				m_State.m_CursorPosition.m_nColumn = 0;
			}
			else
				return;
		}
		else
		{
			cindex += UTF8CharLength(line.buffer[cindex]);
			m_State.m_CursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
			if (aWordMode)
				m_State.m_CursorPosition = FindWordEnd(m_State.m_CursorPosition);
		}
	}

	if (aSelect)
	{
		if (oldPos == m_InteractiveEnd)
			m_InteractiveEnd = SanitizeCoordinates(m_State.m_CursorPosition);
		else if (oldPos == m_InteractiveStart)
			m_InteractiveStart = m_State.m_CursorPosition;
		else
		{
			m_InteractiveStart = oldPos;
			m_InteractiveEnd = m_State.m_CursorPosition;
		}
	}
	else
	{
		if (HasSelection() && !aWordMode)
			m_State.m_CursorPosition = m_InteractiveEnd;

		m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
	}
	SetSelection(m_InteractiveStart, m_InteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void CTextLogger::MoveTop(bool aSelect)
{
	const Coordinates oldPos = m_State.m_CursorPosition;
	SetCursorPosition(Coordinates(0, 0));

	if (m_State.m_CursorPosition != oldPos)
	{
		if (aSelect)
		{
			m_InteractiveEnd = oldPos;
			m_InteractiveStart = m_State.m_CursorPosition;
		}
		else
			m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
		SetSelection(m_InteractiveStart, m_InteractiveEnd);
	}
}

void CTextLogger::MoveBottom(bool aSelect)
{
	const Coordinates oldPos = GetCursorPosition();
	const Coordinates newPos = Coordinates(static_cast<int>(m_Lines.size()) - 1, 0);

	SetCursorPosition(newPos);
	if (aSelect)
	{
		m_InteractiveStart = oldPos;
		m_InteractiveEnd = newPos;
	}
	else
		m_InteractiveStart = m_InteractiveEnd = newPos;
	SetSelection(m_InteractiveStart, m_InteractiveEnd);
}

void CTextLogger::MoveHome(bool aSelect)
{
	const Coordinates oldPos = m_State.m_CursorPosition;
	SetCursorPosition(Coordinates(m_State.m_CursorPosition.m_nLine, 0));

	if (m_State.m_CursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == m_InteractiveStart)
				m_InteractiveStart = m_State.m_CursorPosition;
			else if (oldPos == m_InteractiveEnd)
				m_InteractiveEnd = m_State.m_CursorPosition;
			else
			{
				m_InteractiveStart = m_State.m_CursorPosition;
				m_InteractiveEnd = oldPos;
			}
		}
		else
			m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
		SetSelection(m_InteractiveStart, m_InteractiveEnd);
	}
}

void CTextLogger::MoveEnd(bool aSelect)
{
	const Coordinates oldPos = m_State.m_CursorPosition;
	SetCursorPosition(Coordinates(m_State.m_CursorPosition.m_nLine, GetLineMaxColumn(oldPos.m_nLine)));

	if (m_State.m_CursorPosition != oldPos)
	{
		if (aSelect)
		{
			if (oldPos == m_InteractiveEnd)
				m_InteractiveEnd = m_State.m_CursorPosition;
			else if (oldPos == m_InteractiveStart)
				m_InteractiveStart = m_State.m_CursorPosition;
			else
			{
				m_InteractiveStart = oldPos;
				m_InteractiveEnd = m_State.m_CursorPosition;
			}
		}
		else
			m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
		SetSelection(m_InteractiveStart, m_InteractiveEnd);
	}
}

void CTextLogger::SelectWordUnderCursor()
{
	const Coordinates c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}

void CTextLogger::SelectAll()
{
	SetSelection(Coordinates(0, 0), Coordinates(static_cast<int>(m_Lines.size()), 0));
}

bool CTextLogger::HasSelection() const
{
	return m_State.m_SelectionEnd > m_State.m_SelectionStart;
}

void CTextLogger::MoveCoordsInternal(Coordinates& coordOut, int aLines, bool aForward)
{
	assert(aLines > 0);

	if (aLines < 1)
		return;

	if (aForward)
	{
		coordOut.m_nLine += aLines;

		if (coordOut.m_nLine >= static_cast<int>(m_Lines.size()))
		{
			coordOut.m_nLine = static_cast<int>(m_Lines.size()) - 1;
			coordOut.m_nColumn = GetLineMaxColumn(coordOut.m_nLine);
		}
	}
	else
	{
		coordOut.m_nLine -= aLines;

		if (coordOut.m_nLine < 0)
		{
			coordOut.m_nLine = 0;
			coordOut.m_nColumn = 0;
		}
	}
}

// If you click on line 100, and the first line gets removed from the vector,
// the line you clicked will now be 99. This function corrects the selection
// again after the adjustments to the vector
void CTextLogger::MoveSelection(int aLines, bool aForward)
{
	// This always has to be called as interactives are the start/end pos of a
	// mouse click, which gets cached off.
	MoveInteractives(aLines, aForward);

	if (HasSelection())
	{
		Coordinates newCoords[2];

		newCoords[0] = m_State.m_SelectionStart;
		newCoords[1] = m_State.m_SelectionEnd;

		for (size_t i = 0; i < IM_ARRAYSIZE(newCoords); i++)
		{
			MoveCoordsInternal(newCoords[i], aLines, aForward);
		}

		SetSelectionStart(newCoords[0]);
		SetSelectionEnd(newCoords[1]);
	}
}

void CTextLogger::MoveInteractives(int aLines, bool aForward)
{
	Coordinates newCoords[2];

	newCoords[0] = m_InteractiveStart;
	newCoords[1] = m_InteractiveEnd;

	for (size_t i = 0; i < IM_ARRAYSIZE(newCoords); i++)
	{
		MoveCoordsInternal(newCoords[i], aLines, aForward);
	}

	m_InteractiveStart = newCoords[0];
	m_InteractiveEnd = newCoords[1];
}

void CTextLogger::MoveCursor(int aLines, bool aForward)
{
	MoveCoordsInternal(m_State.m_CursorPosition, aLines, aForward);
}

std::string CTextLogger::GetText() const
{
	return GetText(Coordinates(), Coordinates(static_cast<int>(m_Lines.size()), 0));
}

std::vector<std::string> CTextLogger::GetTextLines() const
{
	std::vector<std::string> result;
	result.reserve(m_Lines.size());

	for (const Line& line : m_Lines)
	{
		result.emplace_back(line.buffer);
	}

	return result;
}

std::string CTextLogger::GetSelectedText() const
{
	return GetText(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

std::string CTextLogger::GetCurrentLineText()const
{
	int lineLength = GetLineMaxColumn(m_State.m_CursorPosition.m_nLine);
	return GetText(
		Coordinates(m_State.m_CursorPosition.m_nLine, 0),
		Coordinates(m_State.m_CursorPosition.m_nLine, lineLength));
}

float CTextLogger::TextDistanceToLineStart(const Coordinates& aFrom) const
{
	const Line& line = m_Lines[aFrom.m_nLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom);

	for (size_t it = 0u; it < line.Length() && it < colIndex; )
	{
		if (line.buffer[it] == '\t')
		{
			distance = (1.0f + ImFloor((1.0f + distance) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
			++it;
		}
		else
		{
			size_t d = UTF8CharLength(line.buffer[it]);
			size_t i = 0;
			char tempCString[7];

			for (; i < 6 && d-- > 0 && it < line.Length(); i++, it++)
				tempCString[i] = line.buffer[it];

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}

void CTextLogger::EnsureCursorVisible()
{
	m_bScrollToCursor = true;
	const Coordinates pos = GetActualCursorCoordinates();

	const float scrollX = ImGui::GetScrollX();
	const float scrollY = ImGui::GetScrollY();

	const float width = ImGui::GetWindowWidth();
	const float height = ImGui::GetWindowHeight();

	const float top = ImCeil(scrollY / m_CharAdvance.y);
	const float bottom = ImCeil((scrollY + height) / m_CharAdvance.y);

	const float left = ImCeil(scrollX / m_CharAdvance.x);
	const float right = ImCeil((scrollX + width) / m_CharAdvance.x);

	const ImGuiWindow* const window = ImGui::GetCurrentWindow();

	// For right offset, +.1f as otherwise it would render right below the
	// first pixel making up the right perimeter.
	const float rightOffsetAmount = window->ScrollbarY ? 3.1f : 1.1f;
	const float bottomOffsetAmount = window->ScrollbarX ? 3.f : 2.f;

	if (pos.m_nColumn < left)
		ImGui::SetScrollX(ImMax(0.0f, (pos.m_nColumn) * m_CharAdvance.x));
	if (pos.m_nColumn > right - rightOffsetAmount)
		ImGui::SetScrollX(ImMax(0.0f, (pos.m_nColumn + rightOffsetAmount-1.f) * m_CharAdvance.x - width));
	if (pos.m_nLine < top)
		ImGui::SetScrollY(ImMax(0.0f, (pos.m_nLine) * m_CharAdvance.y));
	if (pos.m_nLine > bottom - bottomOffsetAmount)
		ImGui::SetScrollY(ImMax(0.0f, (pos.m_nLine + bottomOffsetAmount-1.f) * m_CharAdvance.y - height));
}

int CTextLogger::GetPageSize() const
{
	float height = ImGui::GetWindowHeight() - 20.0f;
	return static_cast<int>(ImFloor(height / m_CharAdvance.y));
}
