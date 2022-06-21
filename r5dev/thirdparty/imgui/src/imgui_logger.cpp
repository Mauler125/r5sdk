#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "thirdparty/imgui/include/imgui_logger.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "thirdparty/imgui/include/imgui.h" // for imGui::GetCurrentWindow()

template<class InputIt1, class InputIt2, class BinaryPredicate>
bool equals(InputIt1 first1, InputIt1 last1,
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
	: m_flLineSpacing(1.0f)
	, m_nTabSize(4)
	, m_Overwrite(false)
	, m_bWithinRender(false)
	, m_bScrollToCursor(false)
	, m_flTextStart(0.f)
	, m_nLeftMargin(0)
	, m_bCursorPositionChanged(false)
	, m_nColorRangeMin(0)
	, m_nColorRangeMax(0)
	, m_SelectionMode(SelectionMode::Normal)
	, m_flLastClick(-1.0f)
	, m_bHandleKeyboardInputs(true)
	, m_bHandleMouseInputs(true)
	, m_bShowWhiteSpaces(false)
	, m_nStartTime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{
	m_Lines.push_back(Line());
}

CTextLogger::~CTextLogger()
{
}

std::string CTextLogger::GetText(const Coordinates & aStart, const Coordinates & aEnd) const
{
	std::string result;

	auto lstart = aStart.m_nLine;
	auto lend = aEnd.m_nLine;
	auto istart = GetCharacterIndex(aStart);
	auto iend = GetCharacterIndex(aEnd);
	size_t s = 0;

	for (size_t i = lstart; i < lend; i++)
		s += m_Lines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= (int)m_Lines.size())
			break;

		auto& line = m_Lines[lstart];
		if (istart < (int)line.size())
		{
			result += line[istart].m_Char;
			istart++;
		}
		else
		{
			istart = 0;
			++lstart;
			result += '\n';
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
	auto line = aValue.m_nLine;
	auto column = aValue.m_nColumn;
	if (line >= (int)m_Lines.size())
	{
		if (m_Lines.empty())
		{
			line = 0;
			column = 0;
		}
		else
		{
			line = (int)m_Lines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	}
	else
	{
		column = m_Lines.empty() ? 0 : std::min(column, GetLineMaxColumn(line));
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

// "Borrowed" from ImGui source
static inline int ImTextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
	if (c < 0x80)
	{
		buf[0] = (char)c;
		return 1;
	}
	if (c < 0x800)
	{
		if (buf_size < 2) return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000)
	{
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00)
	{
		if (buf_size < 4) return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));
		return 4;
	}
	//else if (c < 0x10000)
	{
		if (buf_size < 3) return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));
		return 3;
	}
}

void CTextLogger::Advance(Coordinates & aCoordinates) const
{
	if (aCoordinates.m_nLine < (int)m_Lines.size())
	{
		auto& line = m_Lines[aCoordinates.m_nLine];
		auto cindex = GetCharacterIndex(aCoordinates);

		if (cindex + 1 < (int)line.size())
		{
			auto delta = UTF8CharLength(line[cindex].m_Char);
			cindex = std::min(cindex + delta, (int)line.size() - 1);
		}
		else
		{
			++aCoordinates.m_nLine;
			cindex = 0;
		}
		aCoordinates.m_nColumn = GetCharacterColumn(aCoordinates.m_nLine, cindex);
	}
}

void CTextLogger::DeleteRange(const Coordinates & aStart, const Coordinates & aEnd)
{
	assert(aEnd >= aStart);

	//printf("D(%d.%d)-(%d.%d)\n", aStart.mLine, aStart.mColumn, aEnd.mLine, aEnd.mColumn);

	if (aEnd == aStart)
		return;

	auto start = GetCharacterIndex(aStart);
	auto end = GetCharacterIndex(aEnd);

	if (aStart.m_nLine == aEnd.m_nLine)
	{
		auto& line = m_Lines[aStart.m_nLine];
		auto n = GetLineMaxColumn(aStart.m_nLine);
		if (aEnd.m_nColumn >= n)
			line.erase(line.begin() + start, line.end());
		else
			line.erase(line.begin() + start, line.begin() + end);
	}
	else
	{
		auto& firstLine = m_Lines[aStart.m_nLine];
		auto& lastLine = m_Lines[aEnd.m_nLine];

		firstLine.erase(firstLine.begin() + start, firstLine.end());
		lastLine.erase(lastLine.begin(), lastLine.begin() + end);

		if (aStart.m_nLine < aEnd.m_nLine)
			firstLine.insert(firstLine.end(), lastLine.begin(), lastLine.end());

		if (aStart.m_nLine < aEnd.m_nLine)
			RemoveLine(aStart.m_nLine + 1, aEnd.m_nLine + 1);
	}
}

int CTextLogger::InsertTextAt(Coordinates& /* inout */ aWhere, const char * aValue, ImVec4 aColor)
{
	int cindex = GetCharacterIndex(aWhere);
	int totalLines = 0;

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
			if (cindex < (int)m_Lines[aWhere.m_nLine].size())
			{
				auto& newLine = InsertLine(aWhere.m_nLine + 1);
				auto& line = m_Lines[aWhere.m_nLine];
				newLine.insert(newLine.begin(), line.begin() + cindex, line.end());
				line.erase(line.begin() + cindex, line.end());
			}
			else
			{
				auto& newLine = InsertLine(aWhere.m_nLine + 1);
				auto& line = m_Lines[aWhere.m_nLine];
				line.insert(line.begin() + cindex, Glyph(*aValue, aColor));
			}
			++aWhere.m_nLine;
			aWhere.m_nColumn = 0;
			cindex = 0;
			++totalLines;
			++aValue;
		}
		else
		{
			auto& line = m_Lines[aWhere.m_nLine];
			auto d = UTF8CharLength(*aValue);
			while (d-- > 0 && *aValue != '\0')
				line.insert(line.begin() + cindex++, Glyph(*aValue++, aColor));
			++aWhere.m_nColumn;
		}
	}

	return totalLines;
}

CTextLogger::Coordinates CTextLogger::ScreenPosToCoordinates(const ImVec2& aPosition) const
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

	int lineNo = std::max(0, (int)floor(local.y / m_CharAdvance.y));

	int columnCoord = 0;

	if (lineNo >= 0 && lineNo < (int)m_Lines.size())
	{
		auto& line = m_Lines.at(lineNo);

		int columnIndex = 0;
		float columnX = 0.0f;

		while ((size_t)columnIndex < line.size())
		{
			float columnWidth = 0.0f;

			if (line[columnIndex].m_Char == '\t')
			{
				float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ").x;
				float oldX = columnX;
				float newColumnX = (1.0f + std::floor((1.0f + columnX) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
				columnWidth = newColumnX - oldX;
				if (m_flTextStart + columnX + columnWidth * 0.5f > local.x)
					break;
				columnX = newColumnX;
				columnCoord = (columnCoord / m_nTabSize) * m_nTabSize + m_nTabSize;
				columnIndex++;
			}
			else
			{
				char buf[7];
				auto d = UTF8CharLength(line[columnIndex].m_Char);
				int i = 0;
				while (i < 6 && d-- > 0)
					buf[i++] = line[columnIndex++].m_Char;
				buf[i] = '\0';
				columnWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf).x;
				if (m_flTextStart + columnX + columnWidth * 0.5f > local.x)
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
	if (at.m_nLine >= (int)m_Lines.size())
		return at;

	auto& line = m_Lines[at.m_nLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	while (cindex > 0 && isspace(line[cindex].m_Char))
		--cindex;

	while (cindex > 0)
	{
		auto c = line[cindex].m_Char;
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
	if (at.m_nLine >= (int)m_Lines.size())
		return at;

	auto& line = m_Lines[at.m_nLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	bool prevspace = (bool)isspace(line[cindex].m_Char);
	while (cindex < (int)line.size())
	{
		auto c = line[cindex].m_Char;
		auto d = UTF8CharLength(c);

		if (prevspace != !!isspace(c))
		{
			if (isspace(c))
				while (cindex < (int)line.size() && !isspace(line[cindex].m_Char))
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
	if (at.m_nLine >= (int)m_Lines.size())
		return at;

	// skip to the next non-word character
	auto cindex = GetCharacterIndex(aFrom);
	bool isword = false;
	bool skip = false;
	if (cindex < (int)m_Lines[at.m_nLine].size())
	{
		auto& line = m_Lines[at.m_nLine];
		isword = isalnum(line[cindex].m_Char);
		skip = isword;
	}

	while (!isword || skip)
	{
		if (at.m_nLine >= m_Lines.size())
		{
			auto l = std::max(0, (int) m_Lines.size() - 1);
			return Coordinates(l, GetLineMaxColumn(l));
		}

		auto& line = m_Lines[at.m_nLine];
		if (cindex < (int)line.size())
		{
			isword = isalnum(line[cindex].m_Char);

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
	if (aCoordinates.m_nLine >= m_Lines.size())
		return -1;
	auto& line = m_Lines[aCoordinates.m_nLine];
	int c = 0;
	int i = 0;
	for (; i < static_cast<int>(line.size()) && c < aCoordinates.m_nColumn;)
	{
		if (line[i].m_Char == '\t')
			c = (c / m_nTabSize) * m_nTabSize + m_nTabSize;
		else
			++c;
		i += UTF8CharLength(line[i].m_Char);
	}
	return i;
}

int CTextLogger::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= static_cast<int>(m_Lines.size()))
		return 0;

	auto& line = m_Lines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i < static_cast<int>(line.size()))
	{
		auto c = line[i].m_Char;
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

	int c = 0;
	for (size_t i = 0; i < m_Lines[aLine].size(); c++)
		i += static_cast<size_t>(UTF8CharLength(m_Lines[aLine][i].m_Char));
	return c;
}

int CTextLogger::GetLineMaxColumn(int aLine) const
{
	if (aLine >= static_cast<int>(m_Lines.size()))
		return 0;

	int col = 0;
	for (size_t i = 0; i < m_Lines[aLine].size(); )
	{
		auto c = m_Lines[aLine][i].m_Char;
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
	if (aAt.m_nLine >= (int)m_Lines.size() || aAt.m_nColumn == 0)
		return true;

	auto& line = m_Lines[aAt.m_nLine];
	size_t cindex = static_cast<size_t>(GetCharacterIndex(aAt));
	if (cindex >= line.size())
		return true;

	return isspace(line[cindex].m_Char) != isspace(line[cindex - 1].m_Char);
}

void CTextLogger::RemoveLine(int aStart, int aEnd)
{
	m_Mutex.lock();

	assert(aEnd >= aStart);
	assert(m_Lines.size() > (size_t)(aEnd - aStart));

	m_Lines.erase(m_Lines.begin() + aStart, m_Lines.begin() + aEnd);
	assert(!m_Lines.empty());

	m_Mutex.unlock();
}

void CTextLogger::RemoveLine(int aIndex)
{
	m_Mutex.lock();

	assert(m_Lines.size() > 1);

	m_Lines.erase(m_Lines.begin() + aIndex);
	assert(!m_Lines.empty());

	m_Mutex.unlock();
}

CTextLogger::Line& CTextLogger::InsertLine(int aIndex)
{
	auto& result = *m_Lines.insert(m_Lines.begin() + aIndex, Line());
	return result;
}

std::string CTextLogger::GetWordUnderCursor() const
{
	auto c = GetCursorPosition();
	return GetWordAt(c);
}

std::string CTextLogger::GetWordAt(const Coordinates & aCoords) const
{
	auto start = FindWordStart(aCoords);
	auto end = FindWordEnd(aCoords);

	std::string r;

	auto istart = GetCharacterIndex(start);
	auto iend = GetCharacterIndex(end);

	for (auto it = istart; it < iend; ++it)
		r.push_back(m_Lines[aCoords.m_nLine][it].m_Char);

	return r;
}

ImU32 CTextLogger::GetGlyphColor(const Glyph & aGlyph) const
{
	ImVec4 color = aGlyph.m_Color;
	return ImGui::ColorConvertFloat4ToU32(color);
}

void CTextLogger::HandleKeyboardInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowFocused())
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		//ImGui::CaptureKeyboardFromApp(true);

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
		else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			m_Overwrite ^= true;
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			SelectAll();
	}
}

void CTextLogger::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	if (ImGui::IsWindowHovered())
	{
		if (!shift && !alt)
		{
			auto click = ImGui::IsMouseClicked(0);
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);
			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick && (m_flLastClick != -1.0f && (t - m_flLastClick) < io.MouseDoubleClickTime);

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (!ctrl)
				{
					m_State.m_CursorPosition = m_InteractiveStart = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
					m_SelectionMode = SelectionMode::Line;
					SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);
				}

				m_flLastClick = -1.0f;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (!ctrl)
				{
					m_State.m_CursorPosition = m_InteractiveStart = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());

					if (m_SelectionMode == SelectionMode::Line)
						m_SelectionMode = SelectionMode::Normal;
					else
						m_SelectionMode = SelectionMode::Word;
					SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);
				}

				m_flLastClick = (float)ImGui::GetTime();
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				m_State.m_CursorPosition = m_InteractiveStart = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				if (ctrl)
					m_SelectionMode = SelectionMode::Word;
				else
					m_SelectionMode = SelectionMode::Normal;
				SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);

				m_flLastClick = (float)ImGui::GetTime();
			}
			// Mouse left button dragging (=> update selection)
			else if (ImGui::IsMouseDragging(0) && ImGui::IsMouseDown(0))
			{
				io.WantCaptureMouse = true;
				m_State.m_CursorPosition = m_InteractiveEnd = ScreenPosToCoordinates(ImGui::GetMousePos());
				SetSelection(m_InteractiveStart, m_InteractiveEnd, m_SelectionMode);
			}
		}
	}
}

void CTextLogger::Render()
{
	m_Mutex.lock();

	m_bWithinRender = true;
	m_bCursorPositionChanged = false;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	if (m_bHandleKeyboardInputs)
	{
		HandleKeyboardInputs();
		ImGui::PushAllowKeyboardFocus(true);
	}

	if (m_bHandleMouseInputs)
		HandleMouseInputs();

	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	m_CharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * m_flLineSpacing);

	assert(m_svLineBuffer.empty());

	auto contentSize = ImGui::GetWindowContentRegionMax();
	auto drawList = ImGui::GetWindowDrawList();
	float longest(m_flTextStart);

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	auto scrollX = ImGui::GetScrollX();
	auto scrollY = ImGui::GetScrollY();

	auto lineNo = (int)floor(scrollY / m_CharAdvance.y);
	auto lineMax = std::max(0, std::min((int)m_Lines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / m_CharAdvance.y)));

	//m_flTextStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + m_nLeftMargin;

	if (!m_Lines.empty())
	{
		float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

		while (lineNo <= lineMax)
		{
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * m_CharAdvance.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + m_flTextStart, lineStartScreenPos.y);

			auto& line = m_Lines[lineNo];
			longest = std::max(m_flTextStart + TextDistanceToLineStart(Coordinates(lineNo, GetLineMaxColumn(lineNo))), longest);
			auto columnNo = 0;
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
				ImVec2 vstart(lineStartScreenPos.x + m_flTextStart + sstart, lineStartScreenPos.y);
				ImVec2 vend(lineStartScreenPos.x + m_flTextStart + ssend, lineStartScreenPos.y + m_CharAdvance.y);
				drawList->AddRectFilled(vstart, vend, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
			}

			if (m_State.m_CursorPosition.m_nLine == lineNo)
			{
				bool focused = ImGui::IsWindowFocused();
				ImVec2 start = ImVec2(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

				// Highlight the current line (where the cursor is)
				//if (!HasSelection())
				//{
				//	auto end = ImVec2(start.x + contentSize.x + scrollX, start.y + m_CharAdvance.y);
				//	drawList->AddRectFilled(start, end, 0x80a06020);
				//	drawList->AddRect(start, end, 0x80a06020, 1.0f);
				//}

				// Render the cursor
				if (focused)
				{
					auto timeEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					auto elapsed = timeEnd - m_nStartTime;
					if (elapsed > 400)
					{
						float width = 1.0f;
						auto cindex = GetCharacterIndex(m_State.m_CursorPosition);
						float cx = TextDistanceToLineStart(m_State.m_CursorPosition);

						if (m_Overwrite && cindex < (int)line.size())
						{
							auto c = line[cindex].m_Char;
							if (c == '\t')
							{
								auto x = (1.0f + std::floor((1.0f + cx) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
								width = x - cx;
							}
							else
							{
								char buf2[2];
								buf2[0] = line[cindex].m_Char;
								buf2[1] = '\0';
								width = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf2).x;
							}
						}
						ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
						ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + m_CharAdvance.y);
						drawList->AddRectFilled(cstart, cend, 0xffe0e0e0);
						if (elapsed > 800)
							m_nStartTime = timeEnd;
					}
				}
			}

			// Render colorized text
			//auto prevColor = GetGlyphColor(line[0]);
			ImVec2 bufferOffset;

			for (size_t i = 0; i < line.size();)
			{
				Glyph& glyph = line[i];
				ImU32 color = 0xff605040;

				if (m_itFilter.PassFilter(GetTextFromLine(line).c_str()))
					color = GetGlyphColor(glyph);

				if ((glyph.m_Char == '\t' || glyph.m_Char == '\n' || glyph.m_Char == ' ') && !m_svLineBuffer.empty())
				{
					const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
					drawList->AddText(newOffset, color, m_svLineBuffer.c_str());
					auto textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, m_svLineBuffer.c_str(), nullptr, nullptr);
					bufferOffset.x += textSize.x;
					m_svLineBuffer.clear();
				}

				if (glyph.m_Char == '\t' || glyph.m_Char == '\n')
				{
					auto oldX = bufferOffset.x;
					bufferOffset.x = (1.0f + std::floor((1.0f + bufferOffset.x) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
					++i;

					if (m_bShowWhiteSpaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x1 = textScreenPos.x + oldX + 1.0f;
						const auto x2 = textScreenPos.x + bufferOffset.x - 1.0f;
						const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						const ImVec2 p1(x1, y);
						const ImVec2 p2(x2, y);
						const ImVec2 p3(x2 - s * 0.2f, y - s * 0.2f);
						const ImVec2 p4(x2 - s * 0.2f, y + s * 0.2f);
						drawList->AddLine(p1, p2, 0x90909090);
						drawList->AddLine(p2, p3, 0x90909090);
						drawList->AddLine(p2, p4, 0x90909090);
					}
				}
				else if (glyph.m_Char == ' ')
				{
					if (m_bShowWhiteSpaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x = textScreenPos.x + bufferOffset.x + spaceSize * 0.5f;
						const auto y = textScreenPos.y + bufferOffset.y + s * 0.5f;
						drawList->AddCircleFilled(ImVec2(x, y), 1.5f, 0x80808080, 4);
					}
					bufferOffset.x += spaceSize;
					i++;
				}
				else
				{
					if (line.size() > 0)
					{
						auto l = UTF8CharLength(glyph.m_Char); // !TODO: (this currently causes crashes)
						while (l-- > 0)
							m_svLineBuffer.push_back(line[i++].m_Char);
					}
				}
				++columnNo;
			}

			if (!m_svLineBuffer.empty())
			{
				const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
				drawList->AddText(newOffset, 0xffffffff, m_svLineBuffer.c_str()); // COLOR (obtain from glyph?)
				m_svLineBuffer.clear();
			}

			++lineNo;
		}
	}


	ImGui::Dummy(ImVec2((longest + 2), m_Lines.size() * m_CharAdvance.y));

	if (m_bScrollToCursor)
	{
		EnsureCursorVisible();
		ImGui::SetWindowFocus();
		m_bScrollToCursor = false;
	}

	if (m_bHandleKeyboardInputs)
		ImGui::PopAllowKeyboardFocus();

	ImGui::PopStyleVar();
	m_bWithinRender = false;

	m_Mutex.unlock();
}

void CTextLogger::SetText(const CConLog& aText)
{
	m_Lines.clear();
	m_Lines.emplace_back(Line());
	for (auto chr : aText.m_svConLog)
	{
		if (chr == '\r') // ignore the carriage return character
			continue;
		else if (chr == '\n')
			m_Lines.emplace_back(Line());
		else
			m_Lines.back().emplace_back(Glyph(chr, aText.m_imColor));
	}
}

void CTextLogger::SetTextLines(const std::vector<CConLog>& aLines)
{
	m_Lines.clear();

	if (aLines.empty())
	{
		m_Lines.emplace_back(Line());
	}
	else
	{
		m_Lines.resize(aLines.size());

		for (size_t i = 0; i < aLines.size(); ++i)
		{
			const std::string & aLine = aLines[i].m_svConLog;

			m_Lines[i].reserve(aLine.size());
			for (size_t j = 0; j < aLine.size(); ++j)
			{
				m_Lines[i].emplace_back(Glyph(aLine[j], aLines[i].m_imColor));
			}
		}
	}
}

void CTextLogger::SetCursorPosition(const Coordinates & aPosition)
{
	if (m_State.m_CursorPosition != aPosition)
	{
		m_State.m_CursorPosition = aPosition;
		m_bCursorPositionChanged = true;
		EnsureCursorVisible();
	}
}

void CTextLogger::SetSelectionStart(const Coordinates & aPosition)
{
	m_State.m_SelectionStart = SanitizeCoordinates(aPosition);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		std::swap(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

void CTextLogger::SetSelectionEnd(const Coordinates & aPosition)
{
	m_State.m_SelectionEnd = SanitizeCoordinates(aPosition);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		std::swap(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

void CTextLogger::SetSelection(const Coordinates & aStart, const Coordinates & aEnd, SelectionMode aMode)
{
	auto oldSelStart = m_State.m_SelectionStart;
	auto oldSelEnd = m_State.m_SelectionEnd;

	m_State.m_SelectionStart = SanitizeCoordinates(aStart);
	m_State.m_SelectionEnd = SanitizeCoordinates(aEnd);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		std::swap(m_State.m_SelectionStart, m_State.m_SelectionEnd);

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
		const auto lineNo = m_State.m_SelectionEnd.m_nLine;
		const auto lineSize = (size_t)lineNo < m_Lines.size() ? m_Lines[lineNo].size() : 0;
		m_State.m_SelectionStart = Coordinates(m_State.m_SelectionStart.m_nLine, 0);
		m_State.m_SelectionEnd = Coordinates(lineNo, GetLineMaxColumn(lineNo));
		break;
	}
	default:
		break;
	}

	if (m_State.m_SelectionStart != oldSelStart ||
		m_State.m_SelectionEnd != oldSelEnd)
		m_bCursorPositionChanged = true;
}

void CTextLogger::SetTabSize(int aValue)
{
	m_nTabSize = std::max(0, std::min(32, aValue));
}

void CTextLogger::InsertText(const CConLog & aValue)
{
	m_Mutex.lock();

	if (!aValue.m_svConLog.empty())
	{
		auto pos = GetActualLastLineCoordinates();

		auto start = std::min(pos, m_State.m_SelectionStart);
		int totalLines = pos.m_nLine - start.m_nLine;

		totalLines += InsertTextAt(pos, aValue.m_svConLog.c_str(), aValue.m_imColor);
	}

	m_Mutex.unlock();
}

void CTextLogger::MoveUp(int aAmount, bool aSelect)
{
	auto oldPos = m_State.m_CursorPosition;
	m_State.m_CursorPosition.m_nLine = std::max(0, m_State.m_CursorPosition.m_nLine - aAmount);
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
		SetSelection(m_InteractiveStart, m_InteractiveEnd);

		EnsureCursorVisible();
	}
}

void CTextLogger::MoveDown(int aAmount, bool aSelect)
{
	assert(m_State.m_CursorPosition.m_nColumn >= 0);
	auto oldPos = m_State.m_CursorPosition;
	m_State.m_CursorPosition.m_nLine = std::max(0, std::min((int)m_Lines.size() - 1, m_State.m_CursorPosition.m_nLine + aAmount));

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

	auto oldPos = m_State.m_CursorPosition;
	m_State.m_CursorPosition = GetActualCursorCoordinates();
	auto line = m_State.m_CursorPosition.m_nLine;
	auto cindex = GetCharacterIndex(m_State.m_CursorPosition);

	while (aAmount-- > 0)
	{
		if (cindex == 0)
		{
			if (line > 0)
			{
				--line;
				if ((int)m_Lines.size() > line)
					cindex = (int)m_Lines[line].size();
				else
					cindex = 0;
			}
		}
		else
		{
			--cindex;
			if (cindex > 0)
			{
				if ((int)m_Lines.size() > line)
				{
					while (cindex > 0 && IsUTFSequence(m_Lines[line][cindex].m_Char))
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
		m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
	SetSelection(m_InteractiveStart, m_InteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void CTextLogger::MoveRight(int aAmount, bool aSelect, bool aWordMode)
{
	auto oldPos = m_State.m_CursorPosition;

	if (m_Lines.empty() || oldPos.m_nLine >= m_Lines.size())
		return;

	auto cindex = GetCharacterIndex(m_State.m_CursorPosition);
	while (aAmount-- > 0)
	{
		auto lindex = m_State.m_CursorPosition.m_nLine;
		auto& line = m_Lines[lindex];

		if (cindex >= line.size())
		{
			if (m_State.m_CursorPosition.m_nLine < m_Lines.size() - 1)
			{
				m_State.m_CursorPosition.m_nLine = std::max(0, std::min((int)m_Lines.size() - 1, m_State.m_CursorPosition.m_nLine + 1));
				m_State.m_CursorPosition.m_nColumn = 0;
			}
			else
				return;
		}
		else
		{
			cindex += UTF8CharLength(line[cindex].m_Char);
			m_State.m_CursorPosition = Coordinates(lindex, GetCharacterColumn(lindex, cindex));
			if (aWordMode)
				m_State.m_CursorPosition = FindNextWord(m_State.m_CursorPosition);
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
		m_InteractiveStart = m_InteractiveEnd = m_State.m_CursorPosition;
	SetSelection(m_InteractiveStart, m_InteractiveEnd, aSelect && aWordMode ? SelectionMode::Word : SelectionMode::Normal);

	EnsureCursorVisible();
}

void CTextLogger::MoveTop(bool aSelect)
{
	auto oldPos = m_State.m_CursorPosition;
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
	auto oldPos = GetCursorPosition();
	auto newPos = Coordinates((int)m_Lines.size() - 1, 0);
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
	auto oldPos = m_State.m_CursorPosition;
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
	auto oldPos = m_State.m_CursorPosition;
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
	auto c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}

void CTextLogger::SelectAll()
{
	SetSelection(Coordinates(0, 0), Coordinates((int)m_Lines.size(), 0));
}

bool CTextLogger::HasSelection() const
{
	return m_State.m_SelectionEnd > m_State.m_SelectionStart;
}

void CTextLogger::Copy()
{
	if (HasSelection())
	{
		ImGui::SetClipboardText(GetSelectedText().c_str());
	}
	else
	{
		if (!m_Lines.empty())
		{
			std::string str;
			auto& line = m_Lines[GetActualCursorCoordinates().m_nLine];
			for (auto& g : line)
				str.push_back(g.m_Char);
			ImGui::SetClipboardText(str.c_str());
		}
	}
}

std::string CTextLogger::GetText() const
{
	return GetText(Coordinates(), Coordinates((int)m_Lines.size(), 0));
}

std::vector<std::string> CTextLogger::GetTextLines() const
{
	std::vector<std::string> result;

	result.reserve(m_Lines.size());

	for (auto & line : m_Lines)
	{
		std::string text;

		text.resize(line.size());

		for (size_t i = 0; i < line.size(); ++i)
			text[i] = line[i].m_Char;

		result.emplace_back(std::move(text));
	}

	return result;
}

std::string CTextLogger::GetSelectedText() const
{
	return GetText(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

std::string CTextLogger::GetCurrentLineText()const
{
	auto lineLength = GetLineMaxColumn(m_State.m_CursorPosition.m_nLine);
	return GetText(
		Coordinates(m_State.m_CursorPosition.m_nLine, 0),
		Coordinates(m_State.m_CursorPosition.m_nLine, lineLength));
}

std::string CTextLogger::GetTextFromLine(const Line& aLine) const
{
	std::string result;
	if (aLine.size() < 1)
		return result;

	for (size_t i = 0; i < aLine.size(); i++)
	{
		result += (aLine[i].m_Char);
	}
	return result;
}

int CTextLogger::GetTotalFilterMatches() const
{
	if (!m_itFilter.IsActive())
		return static_cast<int>(m_Lines.size());

	int result = 0;
	for (size_t i = 0; i < m_Lines.size(); i++)
	{
		std::string svLineBuffer;
		for (size_t j = 0; j < m_Lines[i].size(); j++)
		{
			svLineBuffer += m_Lines[i][j].m_Char;
		}
		if (m_itFilter.PassFilter(svLineBuffer.c_str()))
		{
			result++;
		}
	}
	return result;
}

float CTextLogger::TextDistanceToLineStart(const Coordinates& aFrom) const
{
	auto& line = m_Lines[aFrom.m_nLine];
	float distance = 0.0f;
	float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;
	int colIndex = GetCharacterIndex(aFrom);
	for (size_t it = 0u; it < line.size() && it < colIndex; )
	{
		if (line[it].m_Char == '\t')
		{
			distance = (1.0f + std::floor((1.0f + distance) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
			++it;
		}
		else
		{
			auto d = UTF8CharLength(line[it].m_Char);
			char tempCString[7];
			int i = 0;
			for (; i < 6 && d-- > 0 && it < (int)line.size(); i++, it++)
				tempCString[i] = line[it].m_Char;

			tempCString[i] = '\0';
			distance += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, tempCString, nullptr, nullptr).x;
		}
	}

	return distance;
}

void CTextLogger::EnsureCursorVisible()
{
	if (!m_bWithinRender)
	{
		m_bScrollToCursor = true;
		return;
	}

	float scrollX = ImGui::GetScrollX();
	float scrollY = ImGui::GetScrollY();

	auto height = ImGui::GetWindowHeight();
	auto width = ImGui::GetWindowWidth();

	auto top = 1 + (int)ceil(scrollY / m_CharAdvance.y);
	auto bottom = (int)ceil((scrollY + height) / m_CharAdvance.y);

	auto left = (int)ceil(scrollX / m_CharAdvance.x);
	auto right = (int)ceil((scrollX + width) / m_CharAdvance.x);

	auto pos = GetActualCursorCoordinates();
	auto len = TextDistanceToLineStart(pos);

	if (pos.m_nLine < top)
		ImGui::SetScrollY(std::max(0.0f, (pos.m_nLine - 1) * m_CharAdvance.y));
	if (pos.m_nLine > bottom - 4)
		ImGui::SetScrollY(std::max(0.0f, (pos.m_nLine + 4) * m_CharAdvance.y - height));
	if (len + m_flTextStart < left + 4)
		ImGui::SetScrollX(std::max(0.0f, len + m_flTextStart - 4));
	if (len + m_flTextStart > right - 4)
		ImGui::SetScrollX(std::max(0.0f, len + m_flTextStart + 4 - width));
}

int CTextLogger::GetPageSize() const
{
	auto height = ImGui::GetWindowHeight() - 20.0f;
	return (int)floor(height / m_CharAdvance.y);
}
