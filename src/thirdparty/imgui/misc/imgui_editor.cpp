#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include "imgui_editor.h"
#include "imgui.h" // for imGui::GetCurrentWindow()

// TODO
// - multiline comments vs single-line: latter is blocking start of a ML

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

CTextEditor::CTextEditor()
	: m_flLineSpacing(1.0f)
	, m_nUndoIndex(0)
	, m_nTabSize(4)
	, m_Overwrite(false)
	, m_bReadOnly(false)
	, m_bWithinRender(false)
	, m_bScrollToCursor(false)
	, m_bScrollToTop(false)
	, m_bTextChanged(false)
	, m_bColorizerEnabled(true)
	, m_flTextStart(20.0f)
	, m_nLeftMargin(10)
	, m_bCursorPositionChanged(false)
	, m_nColorRangeMin(0)
	, m_nColorRangeMax(0)
	, m_SelectionMode(SelectionMode::Normal)
	, m_bCheckComments(true)
	, m_flLastClick(-1.0f)
	, m_bHandleKeyboardInputs(true)
	, m_bHandleMouseInputs(true)
	, m_bIgnoreImGuiChild(false)
	, m_bShowWhitespaces(true)
	, m_nStartTime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
{
	SetPalette(GetDarkPalette());
	SetLanguageDefinition(LanguageDefinition::HLSL());
	m_Lines.push_back(Line());
}

CTextEditor::~CTextEditor()
{
}

void CTextEditor::SetLanguageDefinition(const LanguageDefinition & aLanguageDef)
{
	m_LanguageDefinition = aLanguageDef;
	m_RegexList.clear();

	for (auto& r : m_LanguageDefinition.m_TokenRegexStrings)
		m_RegexList.push_back(std::make_pair(std::regex(r.first, std::regex_constants::optimize), r.second));

	Colorize();
}

void CTextEditor::SetPalette(const Palette & aValue)
{
	m_PaletteBase = aValue;
}

std::string CTextEditor::GetText(const Coordinates & aStart, const Coordinates & aEnd) const
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

CTextEditor::Coordinates CTextEditor::GetActualCursorCoordinates() const
{
	return SanitizeCoordinates(m_State.m_CursorPosition);
}

CTextEditor::Coordinates CTextEditor::SanitizeCoordinates(const Coordinates & aValue) const
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
static int UTF8CharLength(CTextEditor::Char c)
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

void CTextEditor::Advance(Coordinates & aCoordinates) const
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

void CTextEditor::DeleteRange(const Coordinates & aStart, const Coordinates & aEnd)
{
	assert(aEnd >= aStart);
	assert(!m_bReadOnly);

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

	m_bTextChanged = true;
}

int CTextEditor::InsertTextAt(Coordinates& /* inout */ aWhere, const char * aValue)
{
	assert(!m_bReadOnly);

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
				InsertLine(aWhere.m_nLine + 1);
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
				line.insert(line.begin() + cindex++, Glyph(*aValue++, PaletteIndex::Default));
			++aWhere.m_nColumn;
		}

		m_bTextChanged = true;
	}

	return totalLines;
}

void CTextEditor::AddUndo(UndoRecord& aValue)
{
	assert(!m_bReadOnly);
	//printf("AddUndo: (@%d.%d) +\'%s' [%d.%d .. %d.%d], -\'%s', [%d.%d .. %d.%d] (@%d.%d)\n",
	//	aValue.mBefore.mCursorPosition.mLine, aValue.mBefore.mCursorPosition.mColumn,
	//	aValue.mAdded.c_str(), aValue.mAddedStart.mLine, aValue.mAddedStart.mColumn, aValue.mAddedEnd.mLine, aValue.mAddedEnd.mColumn,
	//	aValue.mRemoved.c_str(), aValue.mRemovedStart.mLine, aValue.mRemovedStart.mColumn, aValue.mRemovedEnd.mLine, aValue.mRemovedEnd.mColumn,
	//	aValue.mAfter.mCursorPosition.mLine, aValue.mAfter.mCursorPosition.mColumn
	//	);

	m_UndoBuffer.resize((size_t)(m_nUndoIndex + 1));
	m_UndoBuffer.back() = aValue;
	++m_nUndoIndex;
}

CTextEditor::Coordinates CTextEditor::ScreenPosToCoordinates(const ImVec2& aPosition) const
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

CTextEditor::Coordinates CTextEditor::FindWordStart(const Coordinates & aFrom) const
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

	auto cstart = (PaletteIndex)line[cindex].m_ColorIndex;
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
			if (cstart != (PaletteIndex)line[size_t(cindex - 1)].m_ColorIndex)
				break;
		}
		--cindex;
	}
	return Coordinates(at.m_nLine, GetCharacterColumn(at.m_nLine, cindex));
}

CTextEditor::Coordinates CTextEditor::FindWordEnd(const Coordinates & aFrom) const
{
	Coordinates at = aFrom;
	if (at.m_nLine >= (int)m_Lines.size())
		return at;

	auto& line = m_Lines[at.m_nLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	bool prevspace = (bool)isspace(line[cindex].m_Char);
	auto cstart = (PaletteIndex)line[cindex].m_ColorIndex;
	while (cindex < (int)line.size())
	{
		auto c = line[cindex].m_Char;
		auto d = UTF8CharLength(c);
		if (cstart != (PaletteIndex)line[cindex].m_ColorIndex)
			break;

		if (prevspace != !!isspace(c))
		{
			if (isspace(c))
				while (cindex < (int)line.size() && isspace(line[cindex].m_Char))
					++cindex;
			break;
		}
		cindex += d;
	}
	return Coordinates(aFrom.m_nLine, GetCharacterColumn(aFrom.m_nLine, cindex));
}

CTextEditor::Coordinates CTextEditor::FindNextWord(const Coordinates & aFrom) const
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

int CTextEditor::GetCharacterIndex(const Coordinates& aCoordinates) const
{
	if (aCoordinates.m_nLine >= m_Lines.size())
		return -1;
	auto& line = m_Lines[aCoordinates.m_nLine];
	int c = 0;
	int i = 0;
	for (; i < line.size() && c < aCoordinates.m_nColumn;)
	{
		if (line[i].m_Char == '\t')
			c = (c / m_nTabSize) * m_nTabSize + m_nTabSize;
		else
			++c;
		i += UTF8CharLength(line[i].m_Char);
	}
	return i;
}

int CTextEditor::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= m_Lines.size())
		return 0;
	auto& line = m_Lines[aLine];
	int col = 0;
	int i = 0;
	while (i < aIndex && i < (int)line.size())
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

int CTextEditor::GetLineCharacterCount(int aLine) const
{
	if (aLine >= m_Lines.size())
		return 0;
	auto& line = m_Lines[aLine];
	int c = 0;
	for (unsigned i = 0; i < line.size(); c++)
		i += UTF8CharLength(line[i].m_Char);
	return c;
}

int CTextEditor::GetLineMaxColumn(int aLine) const
{
	if (aLine >= m_Lines.size())
		return 0;
	auto& line = m_Lines[aLine];
	int col = 0;
	for (unsigned i = 0; i < line.size(); )
	{
		auto c = line[i].m_Char;
		if (c == '\t')
			col = (col / m_nTabSize) * m_nTabSize + m_nTabSize;
		else
			col++;
		i += UTF8CharLength(c);
	}
	return col;
}

bool CTextEditor::IsOnWordBoundary(const Coordinates & aAt) const
{
	if (aAt.m_nLine >= (int)m_Lines.size() || aAt.m_nColumn == 0)
		return true;

	auto& line = m_Lines[aAt.m_nLine];
	auto cindex = GetCharacterIndex(aAt);
	if (cindex >= (int)line.size())
		return true;

	if (m_bColorizerEnabled)
		return line[cindex].m_ColorIndex != line[size_t(cindex - 1)].m_ColorIndex;

	return isspace(line[cindex].m_Char) != isspace(line[cindex - 1].m_Char);
}

void CTextEditor::RemoveLine(int aStart, int aEnd)
{
	assert(!m_bReadOnly);
	assert(aEnd >= aStart);
	assert(m_Lines.size() > (size_t)(aEnd - aStart));

	ErrorMarkers etmp;
	for (auto& i : m_ErrorMarkers)
	{
		ErrorMarkers::value_type e(i.first >= aStart ? i.first - 1 : i.first, i.second);
		if (e.first >= aStart && e.first <= aEnd)
			continue;
		etmp.insert(e);
	}
	m_ErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : m_Breakpoints)
	{
		if (i >= aStart && i <= aEnd)
			continue;
		btmp.insert(i >= aStart ? i - 1 : i);
	}
	m_Breakpoints = std::move(btmp);

	m_Lines.erase(m_Lines.begin() + aStart, m_Lines.begin() + aEnd);
	assert(!m_Lines.empty());

	m_bTextChanged = true;
}

void CTextEditor::RemoveLine(int aIndex)
{
	assert(!m_bReadOnly);
	assert(m_Lines.size() > 1);

	ErrorMarkers etmp;
	for (auto& i : m_ErrorMarkers)
	{
		ErrorMarkers::value_type e(i.first > aIndex ? i.first - 1 : i.first, i.second);
		if (e.first - 1 == aIndex)
			continue;
		etmp.insert(e);
	}
	m_ErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : m_Breakpoints)
	{
		if (i == aIndex)
			continue;
		btmp.insert(i >= aIndex ? i - 1 : i);
	}
	m_Breakpoints = std::move(btmp);

	m_Lines.erase(m_Lines.begin() + aIndex);
	assert(!m_Lines.empty());

	m_bTextChanged = true;
}

CTextEditor::Line& CTextEditor::InsertLine(int aIndex)
{
	assert(!m_bReadOnly);

	auto& result = *m_Lines.insert(m_Lines.begin() + aIndex, Line());

	ErrorMarkers etmp;
	for (auto& i : m_ErrorMarkers)
		etmp.insert(ErrorMarkers::value_type(i.first >= aIndex ? i.first + 1 : i.first, i.second));
	m_ErrorMarkers = std::move(etmp);

	Breakpoints btmp;
	for (auto i : m_Breakpoints)
		btmp.insert(i >= aIndex ? i + 1 : i);
	m_Breakpoints = std::move(btmp);

	return result;
}

std::string CTextEditor::GetWordUnderCursor() const
{
	auto c = GetCursorPosition();
	return GetWordAt(c);
}

std::string CTextEditor::GetWordAt(const Coordinates & aCoords) const
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

ImU32 CTextEditor::GetGlyphColor(const Glyph & aGlyph) const
{
	if (!m_bColorizerEnabled)
		return m_Palette[(int)PaletteIndex::Default];
	if (aGlyph.m_bComment)
		return m_Palette[(int)PaletteIndex::Comment];
	if (aGlyph.m_bMultiLineComment)
		return m_Palette[(int)PaletteIndex::MultiLineComment];
	auto const color = m_Palette[(int)aGlyph.m_ColorIndex];
	if (aGlyph.m_bPreprocessor)
	{
		const auto ppcolor = m_Palette[(int)PaletteIndex::Preprocessor];
		const int c0 = ((ppcolor & 0xff) + (color & 0xff)) / 2;
		const int c1 = (((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
		const int c2 = (((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
		const int c3 = (((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
		return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
	}
	return color;
}

void CTextEditor::HandleKeyboardInputs()
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

		if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
			Undo();
		else if (!IsReadOnly() && !ctrl && !shift && alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Undo();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y)))
			Redo();
		else if (!ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
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
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Delete();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
			Backspace();
		else if (!ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			m_Overwrite ^= true;
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Copy();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C)))
			Copy();
		else if (!IsReadOnly() && !ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Insert)))
			Paste();
		else if (!IsReadOnly() && ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V)))
			Paste();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_X)))
			Cut();
		else if (!ctrl && shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
			Cut();
		else if (ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A)))
			SelectAll();
		else if (!IsReadOnly() && !ctrl && !shift && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
			EnterCharacter('\n', false);
		else if (!IsReadOnly() && !ctrl && !alt && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
			EnterCharacter('\t', shift);

		if (!IsReadOnly() && !io.InputQueueCharacters.empty())
		{
			for (int i = 0; i < io.InputQueueCharacters.Size; i++)
			{
				auto c = io.InputQueueCharacters[i];
				if (c != 0 && (c == '\n' || c >= 32))
					EnterCharacter(c, shift);
			}
			io.InputQueueCharacters.resize(0);
		}
	}
}

void CTextEditor::HandleMouseInputs()
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

void CTextEditor::Render()
{
	/* Compute mCharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	m_CharAdvance = ImVec2(fontSize, ImGui::GetTextLineHeightWithSpacing() * m_flLineSpacing);

	/* Update palette with the current alpha from style */
	for (int i = 0; i < (int)PaletteIndex::Max; ++i)
	{
		auto color = ImGui::ColorConvertU32ToFloat4(m_PaletteBase[i]);
		color.w *= ImGui::GetStyle().Alpha;
		m_Palette[i] = ImGui::ColorConvertFloat4ToU32(color);
	}

	assert(m__svLineBuffer.empty());

	auto contentSize = ImGui::GetWindowContentRegionMax();
	auto drawList = ImGui::GetWindowDrawList();
	float longest(m_flTextStart);

	if (m_bScrollToTop)
	{
		m_bScrollToTop = false;
		ImGui::SetScrollY(0.f);
	}

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	auto scrollX = ImGui::GetScrollX();
	auto scrollY = ImGui::GetScrollY();

	auto lineNo = (int)floor(scrollY / m_CharAdvance.y);
	auto globalLineMax = (int)m_Lines.size();
	auto lineMax = std::max(0, std::min((int)m_Lines.size() - 1, lineNo + (int)floor((scrollY + contentSize.y) / m_CharAdvance.y)));

	// Deduce mTextStart by evaluating mLines size (global lineMax) plus two spaces as text width
	char buf[16];
	snprintf(buf, 16, " %d ", globalLineMax);
	m_flTextStart = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x + m_nLeftMargin;

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
				drawList->AddRectFilled(vstart, vend, m_Palette[(int)PaletteIndex::Selection]);
			}

			// Draw breakpoints
			auto start = ImVec2(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

			if (m_Breakpoints.count(lineNo + 1) != 0)
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + m_CharAdvance.y);
				drawList->AddRectFilled(start, end, m_Palette[(int)PaletteIndex::Breakpoint]);
			}

			// Draw error markers
			auto errorIt = m_ErrorMarkers.find(lineNo + 1);
			if (errorIt != m_ErrorMarkers.end())
			{
				auto end = ImVec2(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + m_CharAdvance.y);
				drawList->AddRectFilled(start, end, m_Palette[(int)PaletteIndex::ErrorMarker]);

				if (ImGui::IsMouseHoveringRect(lineStartScreenPos, end))
				{
					ImGui::BeginTooltip();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::Text("Error at line %d:", errorIt->first);
					ImGui::PopStyleColor();
					ImGui::Separator();
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
					ImGui::Text("%s", errorIt->second.c_str());
					ImGui::PopStyleColor();
					ImGui::EndTooltip();
				}
			}

			// Draw line number (right aligned)
			snprintf(buf, 16, "%d  ", lineNo + 1);

			auto lineNoWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, buf, nullptr, nullptr).x;
			drawList->AddText(ImVec2(lineStartScreenPos.x + m_flTextStart - lineNoWidth, lineStartScreenPos.y), m_Palette[(int)PaletteIndex::LineNumber], buf);

			if (m_State.m_CursorPosition.m_nLine == lineNo)
			{
				auto focused = ImGui::IsWindowFocused();

				// Highlight the current line (where the cursor is)
				if (!HasSelection())
				{
					auto end = ImVec2(start.x + contentSize.x + scrollX, start.y + m_CharAdvance.y);
					drawList->AddRectFilled(start, end, m_Palette[(int)(focused ? PaletteIndex::CurrentLineFill : PaletteIndex::CurrentLineFillInactive)]);
					drawList->AddRect(start, end, m_Palette[(int)PaletteIndex::CurrentLineEdge], 1.0f);
				}

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
						drawList->AddRectFilled(cstart, cend, m_Palette[(int)PaletteIndex::Cursor]);
						if (elapsed > 800)
							m_nStartTime = timeEnd;
					}
				}
			}

			// Render colorized text
			auto prevColor = line.empty() ? m_Palette[(int)PaletteIndex::Default] : GetGlyphColor(line[0]);
			ImVec2 bufferOffset;

			for (int i = 0; i < line.size();)
			{
				auto& glyph = line[i];
				auto color = GetGlyphColor(glyph);

				if ((color != prevColor || glyph.m_Char == '\t' || glyph.m_Char == ' ') && !m__svLineBuffer.empty())
				{
					const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
					drawList->AddText(newOffset, prevColor, m__svLineBuffer.c_str());
					auto textSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, m__svLineBuffer.c_str(), nullptr, nullptr);
					bufferOffset.x += textSize.x;
					m__svLineBuffer.clear();
				}
				prevColor = color;

				if (glyph.m_Char == '\t')
				{
					auto oldX = bufferOffset.x;
					bufferOffset.x = (1.0f + std::floor((1.0f + bufferOffset.x) / (float(m_nTabSize) * spaceSize))) * (float(m_nTabSize) * spaceSize);
					++i;

					if (m_bShowWhitespaces)
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
					if (m_bShowWhitespaces)
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
					auto l = UTF8CharLength(glyph.m_Char);
					while (l-- > 0)
						m__svLineBuffer.push_back(line[i++].m_Char);
				}
				++columnNo;
			}

			if (!m__svLineBuffer.empty())
			{
				const ImVec2 newOffset(textScreenPos.x + bufferOffset.x, textScreenPos.y + bufferOffset.y);
				drawList->AddText(newOffset, prevColor, m__svLineBuffer.c_str());
				m__svLineBuffer.clear();
			}

			++lineNo;
		}

		// Draw a tooltip on known identifiers/preprocessor symbols
		if (ImGui::IsMousePosValid())
		{
			auto id = GetWordAt(ScreenPosToCoordinates(ImGui::GetMousePos()));
			if (!id.empty())
			{
				auto it = m_LanguageDefinition.m_Identifiers.find(id);
				if (it != m_LanguageDefinition.m_Identifiers.end())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(it->second.m_svDeclaration.c_str());
					ImGui::EndTooltip();
				}
				else
				{
					auto pi = m_LanguageDefinition.m_PreprocIdentifiers.find(id);
					if (pi != m_LanguageDefinition.m_PreprocIdentifiers.end())
					{
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(pi->second.m_svDeclaration.c_str());
						ImGui::EndTooltip();
					}
				}
			}
		}
	}


	ImGui::Dummy(ImVec2((longest + 2), m_Lines.size() * m_CharAdvance.y));

	if (m_bScrollToCursor)
	{
		EnsureCursorVisible();
		ImGui::SetWindowFocus();
		m_bScrollToCursor = false;
	}
}

void CTextEditor::Render(const char* aTitle, const ImVec2& aSize, bool aBorder)
{
	m_bWithinRender = true;
	m_bTextChanged = false;
	m_bCursorPositionChanged = false;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(m_Palette[(int)PaletteIndex::Background]));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	if (!m_bIgnoreImGuiChild)
		ImGui::BeginChild(aTitle, aSize, aBorder, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);

	if (m_bHandleKeyboardInputs)
	{
		HandleKeyboardInputs();
		ImGui::PushAllowKeyboardFocus(true);
	}

	if (m_bHandleMouseInputs)
		HandleMouseInputs();

	ColorizeInternal();
	Render();

	if (m_bHandleKeyboardInputs)
		ImGui::PopAllowKeyboardFocus();

	if (!m_bIgnoreImGuiChild)
		ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	m_bWithinRender = false;
}

void CTextEditor::SetText(const std::string & aText)
{
	m_Lines.clear();
	m_Lines.emplace_back(Line());
	for (auto chr : aText)
	{
		if (chr == '\r')
		{
			// ignore the carriage return character
		}
		else if (chr == '\n')
			m_Lines.emplace_back(Line());
		else
		{
			m_Lines.back().emplace_back(Glyph(chr, PaletteIndex::Default));
		}
	}

	m_bTextChanged = true;
	m_bScrollToTop = true;

	m_UndoBuffer.clear();
	m_nUndoIndex = 0;

	Colorize();
}

void CTextEditor::SetTextLines(const std::vector<std::string> & aLines)
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
			const std::string & aLine = aLines[i];

			m_Lines[i].reserve(aLine.size());
			for (size_t j = 0; j < aLine.size(); ++j)
				m_Lines[i].emplace_back(Glyph(aLine[j], PaletteIndex::Default));
		}
	}

	m_bTextChanged = true;
	m_bScrollToTop = true;

	m_UndoBuffer.clear();
	m_nUndoIndex = 0;

	Colorize();
}

void CTextEditor::EnterCharacter(ImWchar aChar, bool aShift)
{
	assert(!m_bReadOnly);

	UndoRecord u;

	u.m_Before = m_State;

	if (HasSelection())
	{
		if (aChar == '\t' && m_State.m_SelectionStart.m_nLine != m_State.m_SelectionEnd.m_nLine)
		{

			auto start = m_State.m_SelectionStart;
			auto end = m_State.m_SelectionEnd;
			auto originalEnd = end;

			if (start > end)
				std::swap(start, end);
			start.m_nColumn = 0;
			//			end.mColumn = end.mLine < mLines.size() ? mLines[end.mLine].size() : 0;
			if (end.m_nColumn == 0 && end.m_nLine > 0)
				--end.m_nLine;
			if (end.m_nLine >= (int)m_Lines.size())
				end.m_nLine = m_Lines.empty() ? 0 : (int)m_Lines.size() - 1;
			end.m_nColumn = GetLineMaxColumn(end.m_nLine);

			//if (end.mColumn >= GetLineMaxColumn(end.mLine))
			//	end.mColumn = GetLineMaxColumn(end.mLine) - 1;

			u.m_RemovedStart = start;
			u.m_RemovedEnd = end;
			u.m_svRemoved = GetText(start, end);

			bool modified = false;

			for (int i = start.m_nLine; i <= end.m_nLine; i++)
			{
				auto& line = m_Lines[i];
				if (aShift)
				{
					if (!line.empty())
					{
						if (line.front().m_Char == '\t')
						{
							line.erase(line.begin());
							modified = true;
						}
						else
						{
							for (int j = 0; j < m_nTabSize && !line.empty() && line.front().m_Char == ' '; j++)
							{
								line.erase(line.begin());
								modified = true;
							}
						}
					}
				}
				else
				{
					line.insert(line.begin(), Glyph('\t', CTextEditor::PaletteIndex::Background));
					modified = true;
				}
			}

			if (modified)
			{
				start = Coordinates(start.m_nLine, GetCharacterColumn(start.m_nLine, 0));
				Coordinates rangeEnd;
				if (originalEnd.m_nColumn != 0)
				{
					end = Coordinates(end.m_nLine, GetLineMaxColumn(end.m_nLine));
					rangeEnd = end;
					u.m_svAdded = GetText(start, end);
				}
				else
				{
					end = Coordinates(originalEnd.m_nLine, 0);
					rangeEnd = Coordinates(end.m_nLine - 1, GetLineMaxColumn(end.m_nLine - 1));
					u.m_svAdded = GetText(start, rangeEnd);
				}

				u.m_AddedStart = start;
				u.m_AddedEnd = rangeEnd;
				u.m_After = m_State;

				m_State.m_SelectionStart = start;
				m_State.m_SelectionEnd = end;
				AddUndo(u);

				m_bTextChanged = true;

				EnsureCursorVisible();
			}

			return;
		} // c == '\t'
		else
		{
			u.m_svRemoved = GetSelectedText();
			u.m_RemovedStart = m_State.m_SelectionStart;
			u.m_RemovedEnd = m_State.m_SelectionEnd;
			DeleteSelection();
		}
	} // HasSelection

	auto coord = GetActualCursorCoordinates();
	u.m_AddedStart = coord;

	assert(!m_Lines.empty());

	if (aChar == '\n')
	{
		InsertLine(coord.m_nLine + 1);
		auto& line = m_Lines[coord.m_nLine];
		auto& newLine = m_Lines[coord.m_nLine + 1];

		if (m_LanguageDefinition.m_bAutoIndentation)
			for (size_t it = 0; it < line.size() && isascii(line[it].m_Char) && isblank(line[it].m_Char); ++it)
				newLine.push_back(line[it]);

		const size_t whitespaceSize = newLine.size();
		auto cindex = GetCharacterIndex(coord);
		newLine.insert(newLine.end(), line.begin() + cindex, line.end());
		line.erase(line.begin() + cindex, line.begin() + line.size());
		SetCursorPosition(Coordinates(coord.m_nLine + 1, GetCharacterColumn(coord.m_nLine + 1, (int)whitespaceSize)));
		u.m_svAdded = (char)aChar;
	}
	else
	{
		char buf[7];
		int e = ImTextCharToUtf8(buf, 7, aChar);
		if (e > 0)
		{
			buf[e] = '\0';
			auto& line = m_Lines[coord.m_nLine];
			auto cindex = GetCharacterIndex(coord);

			if (m_Overwrite && cindex < (int)line.size())
			{
				auto d = UTF8CharLength(line[cindex].m_Char);

				u.m_RemovedStart = m_State.m_CursorPosition;
				u.m_RemovedEnd = Coordinates(coord.m_nLine, GetCharacterColumn(coord.m_nLine, cindex + d));

				while (d-- > 0 && cindex < (int)line.size())
				{
					u.m_svRemoved += line[cindex].m_Char;
					line.erase(line.begin() + cindex);
				}
			}

			for (auto p = buf; *p != '\0'; p++, ++cindex)
				line.insert(line.begin() + cindex, Glyph(*p, PaletteIndex::Default));
			u.m_svAdded = buf;

			SetCursorPosition(Coordinates(coord.m_nLine, GetCharacterColumn(coord.m_nLine, cindex)));
		}
		else
			return;
	}

	m_bTextChanged = true;

	u.m_AddedEnd = GetActualCursorCoordinates();
	u.m_After = m_State;

	AddUndo(u);

	Colorize(coord.m_nLine - 1, 3);
	EnsureCursorVisible();
}

void CTextEditor::SetReadOnly(bool aValue)
{
	m_bReadOnly = aValue;
}

void CTextEditor::SetColorizerEnable(bool aValue)
{
	m_bColorizerEnabled = aValue;
}

void CTextEditor::SetCursorPosition(const Coordinates & aPosition)
{
	if (m_State.m_CursorPosition != aPosition)
	{
		m_State.m_CursorPosition = aPosition;
		m_bCursorPositionChanged = true;
		EnsureCursorVisible();
	}
}

void CTextEditor::SetSelectionStart(const Coordinates & aPosition)
{
	m_State.m_SelectionStart = SanitizeCoordinates(aPosition);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		std::swap(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

void CTextEditor::SetSelectionEnd(const Coordinates & aPosition)
{
	m_State.m_SelectionEnd = SanitizeCoordinates(aPosition);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		std::swap(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

void CTextEditor::SetSelection(const Coordinates & aStart, const Coordinates & aEnd, SelectionMode aMode)
{
	auto oldSelStart = m_State.m_SelectionStart;
	auto oldSelEnd = m_State.m_SelectionEnd;

	m_State.m_SelectionStart = SanitizeCoordinates(aStart);
	m_State.m_SelectionEnd = SanitizeCoordinates(aEnd);
	if (m_State.m_SelectionStart > m_State.m_SelectionEnd)
		std::swap(m_State.m_SelectionStart, m_State.m_SelectionEnd);

	switch (aMode)
	{
	case CTextEditor::SelectionMode::Normal:
		break;
	case CTextEditor::SelectionMode::Word:
	{
		m_State.m_SelectionStart = FindWordStart(m_State.m_SelectionStart);
		if (!IsOnWordBoundary(m_State.m_SelectionEnd))
			m_State.m_SelectionEnd = FindWordEnd(FindWordStart(m_State.m_SelectionEnd));
		break;
	}
	case CTextEditor::SelectionMode::Line:
	{
		const auto lineNo = m_State.m_SelectionEnd.m_nLine;
		//const auto lineSize = (size_t)lineNo < m_Lines.size() ? m_Lines[lineNo].size() : 0;
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

void CTextEditor::SetTabSize(int aValue)
{
	m_nTabSize = std::max(0, std::min(32, aValue));
}

void CTextEditor::InsertText(const std::string & aValue)
{
	InsertText(aValue.c_str());
}

void CTextEditor::InsertText(const char * aValue)
{
	if (aValue == nullptr)
		return;

	auto pos = GetActualCursorCoordinates();
	auto start = std::min(pos, m_State.m_SelectionStart);
	int totalLines = pos.m_nLine - start.m_nLine;

	totalLines += InsertTextAt(pos, aValue);

	SetSelection(pos, pos);
	SetCursorPosition(pos);
	Colorize(start.m_nLine - 1, totalLines + 2);
}

void CTextEditor::DeleteSelection()
{
	assert(m_State.m_SelectionEnd >= m_State.m_SelectionStart);

	if (m_State.m_SelectionEnd == m_State.m_SelectionStart)
		return;

	DeleteRange(m_State.m_SelectionStart, m_State.m_SelectionEnd);

	SetSelection(m_State.m_SelectionStart, m_State.m_SelectionStart);
	SetCursorPosition(m_State.m_SelectionStart);
	Colorize(m_State.m_SelectionStart.m_nLine, 1);
}

void CTextEditor::MoveUp(int aAmount, bool aSelect)
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

void CTextEditor::MoveDown(int aAmount, bool aSelect)
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

void CTextEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode)
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

void CTextEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode)
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

void CTextEditor::MoveTop(bool aSelect)
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

void CTextEditor::MoveBottom(bool aSelect)
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

void CTextEditor::MoveHome(bool aSelect)
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

void CTextEditor::MoveEnd(bool aSelect)
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

void CTextEditor::Delete()
{
	assert(!m_bReadOnly);

	if (m_Lines.empty())
		return;

	UndoRecord u;
	u.m_Before = m_State;

	if (HasSelection())
	{
		u.m_svRemoved = GetSelectedText();
		u.m_RemovedStart = m_State.m_SelectionStart;
		u.m_RemovedEnd = m_State.m_SelectionEnd;

		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		auto& line = m_Lines[pos.m_nLine];

		if (pos.m_nColumn == GetLineMaxColumn(pos.m_nLine))
		{
			if (pos.m_nLine == (int)m_Lines.size() - 1)
				return;

			u.m_svRemoved = '\n';
			u.m_RemovedStart = u.m_RemovedEnd = GetActualCursorCoordinates();
			Advance(u.m_RemovedEnd);

			auto& nextLine = m_Lines[pos.m_nLine + 1];
			line.insert(line.end(), nextLine.begin(), nextLine.end());
			RemoveLine(pos.m_nLine + 1);
		}
		else
		{
			auto cindex = GetCharacterIndex(pos);
			u.m_RemovedStart = u.m_RemovedEnd = GetActualCursorCoordinates();
			u.m_RemovedEnd.m_nColumn++;
			u.m_svRemoved = GetText(u.m_RemovedStart, u.m_RemovedEnd);

			auto d = UTF8CharLength(line[cindex].m_Char);
			while (d-- > 0 && cindex < (int)line.size())
				line.erase(line.begin() + cindex);
		}

		m_bTextChanged = true;

		Colorize(pos.m_nLine, 1);
	}

	u.m_After = m_State;
	AddUndo(u);
}

void CTextEditor::Backspace()
{
	assert(!m_bReadOnly);

	if (m_Lines.empty())
		return;

	UndoRecord u;
	u.m_Before = m_State;

	if (HasSelection())
	{
		u.m_svRemoved = GetSelectedText();
		u.m_RemovedStart = m_State.m_SelectionStart;
		u.m_RemovedEnd = m_State.m_SelectionEnd;

		DeleteSelection();
	}
	else
	{
		auto pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);

		if (m_State.m_CursorPosition.m_nColumn == 0)
		{
			if (m_State.m_CursorPosition.m_nLine == 0)
				return;

			u.m_svRemoved = '\n';
			u.m_RemovedStart = u.m_RemovedEnd = Coordinates(pos.m_nLine - 1, GetLineMaxColumn(pos.m_nLine - 1));
			Advance(u.m_RemovedEnd);

			auto& line = m_Lines[m_State.m_CursorPosition.m_nLine];
			auto& prevLine = m_Lines[m_State.m_CursorPosition.m_nLine - 1];
			auto prevSize = GetLineMaxColumn(m_State.m_CursorPosition.m_nLine - 1);
			prevLine.insert(prevLine.end(), line.begin(), line.end());

			ErrorMarkers etmp;
			for (auto& i : m_ErrorMarkers)
				etmp.insert(ErrorMarkers::value_type(i.first - 1 == m_State.m_CursorPosition.m_nLine ? i.first - 1 : i.first, i.second));
			m_ErrorMarkers = std::move(etmp);

			RemoveLine(m_State.m_CursorPosition.m_nLine);
			--m_State.m_CursorPosition.m_nLine;
			m_State.m_CursorPosition.m_nColumn = prevSize;
		}
		else
		{
			auto& line = m_Lines[m_State.m_CursorPosition.m_nLine];
			auto cindex = GetCharacterIndex(pos) - 1;
			auto cend = cindex + 1;
			while (cindex > 0 && IsUTFSequence(line[cindex].m_Char))
				--cindex;

			//if (cindex > 0 && UTF8CharLength(line[cindex].mChar) > 1)
			//	--cindex;

			u.m_RemovedStart = u.m_RemovedEnd = GetActualCursorCoordinates();
			--u.m_RemovedStart.m_nColumn;
			--m_State.m_CursorPosition.m_nColumn;

			while (cindex < line.size() && cend-- > cindex)
			{
				u.m_svRemoved += line[cindex].m_Char;
				line.erase(line.begin() + cindex);
			}
		}

		m_bTextChanged = true;

		EnsureCursorVisible();
		Colorize(m_State.m_CursorPosition.m_nLine, 1);
	}

	u.m_After = m_State;
	AddUndo(u);
}

void CTextEditor::SelectWordUnderCursor()
{
	auto c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}

void CTextEditor::SelectAll()
{
	SetSelection(Coordinates(0, 0), Coordinates((int)m_Lines.size(), 0));
}

bool CTextEditor::HasSelection() const
{
	return m_State.m_SelectionEnd > m_State.m_SelectionStart;
}

void CTextEditor::Copy()
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

void CTextEditor::Cut()
{
	if (IsReadOnly())
	{
		Copy();
	}
	else
	{
		if (HasSelection())
		{
			UndoRecord u;
			u.m_Before = m_State;
			u.m_svRemoved = GetSelectedText();
			u.m_RemovedStart = m_State.m_SelectionStart;
			u.m_RemovedEnd = m_State.m_SelectionEnd;

			Copy();
			DeleteSelection();

			u.m_After = m_State;
			AddUndo(u);
		}
	}
}

void CTextEditor::Paste()
{
	if (IsReadOnly())
		return;

	auto clipText = ImGui::GetClipboardText();
	if (clipText != nullptr && strlen(clipText) > 0)
	{
		UndoRecord u;
		u.m_Before = m_State;

		if (HasSelection())
		{
			u.m_svRemoved = GetSelectedText();
			u.m_RemovedStart = m_State.m_SelectionStart;
			u.m_RemovedEnd = m_State.m_SelectionEnd;
			DeleteSelection();
		}

		u.m_svAdded = clipText;
		u.m_AddedStart = GetActualCursorCoordinates();

		InsertText(clipText);

		u.m_AddedEnd = GetActualCursorCoordinates();
		u.m_After = m_State;
		AddUndo(u);
	}
}

bool CTextEditor::CanUndo() const
{
	return !m_bReadOnly && m_nUndoIndex > 0;
}

bool CTextEditor::CanRedo() const
{
	return !m_bReadOnly && m_nUndoIndex < (int)m_UndoBuffer.size();
}

void CTextEditor::Undo(int aSteps)
{
	while (CanUndo() && aSteps-- > 0)
		m_UndoBuffer[--m_nUndoIndex].Undo(this);
}

void CTextEditor::Redo(int aSteps)
{
	while (CanRedo() && aSteps-- > 0)
		m_UndoBuffer[m_nUndoIndex++].Redo(this);
}

const CTextEditor::Palette & CTextEditor::GetDarkPalette()
{
	const static Palette p = { {
			0xff7f7f7f,	// Default
			0xffd69c56,	// Keyword	
			0xff00ff00,	// Number
			0xff7070e0,	// String
			0xff70a0e0, // Char literal
			0xffffffff, // Punctuation
			0xff408080,	// Preprocessor
			0xffaaaaaa, // Identifier
			0xff9bc64d, // Known identifier
			0xffc040a0, // Preproc identifier
			0xff206020, // Comment (single line)
			0xff406020, // Comment (multi line)
			0xff101010, // Background
			0xffe0e0e0, // Cursor
			0x80a06020, // Selection
			0x800020ff, // ErrorMarker
			0x40f08000, // Breakpoint
			0xff707000, // Line number
			0x40000000, // Current line fill
			0x40808080, // Current line fill (inactive)
			0x40a0a0a0, // Current line edge
		} };
	return p;
}

const CTextEditor::Palette & CTextEditor::GetLightPalette()
{
	const static Palette p = { {
			0xff7f7f7f,	// None
			0xffff0c06,	// Keyword	
			0xff008000,	// Number
			0xff2020a0,	// String
			0xff304070, // Char literal
			0xff000000, // Punctuation
			0xff406060,	// Preprocessor
			0xff404040, // Identifier
			0xff606010, // Known identifier
			0xffc040a0, // Preproc identifier
			0xff205020, // Comment (single line)
			0xff405020, // Comment (multi line)
			0xffffffff, // Background
			0xff000000, // Cursor
			0x80600000, // Selection
			0xa00010ff, // ErrorMarker
			0x80f08000, // Breakpoint
			0xff505000, // Line number
			0x40000000, // Current line fill
			0x40808080, // Current line fill (inactive)
			0x40000000, // Current line edge
		} };
	return p;
}

const CTextEditor::Palette & CTextEditor::GetRetroBluePalette()
{
	const static Palette p = { {
			0xff00ffff,	// None
			0xffffff00,	// Keyword	
			0xff00ff00,	// Number
			0xff808000,	// String
			0xff808000, // Char literal
			0xffffffff, // Punctuation
			0xff008000,	// Preprocessor
			0xff00ffff, // Identifier
			0xffffffff, // Known identifier
			0xffff00ff, // Preproc identifier
			0xff808080, // Comment (single line)
			0xff404040, // Comment (multi line)
			0xff800000, // Background
			0xff0080ff, // Cursor
			0x80ffff00, // Selection
			0xa00000ff, // ErrorMarker
			0x80ff8000, // Breakpoint
			0xff808000, // Line number
			0x40000000, // Current line fill
			0x40808080, // Current line fill (inactive)
			0x40000000, // Current line edge
		} };
	return p;
}


std::string CTextEditor::GetText() const
{
	return GetText(Coordinates(), Coordinates((int)m_Lines.size(), 0));
}

std::vector<std::string> CTextEditor::GetTextLines() const
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

std::string CTextEditor::GetSelectedText() const
{
	return GetText(m_State.m_SelectionStart, m_State.m_SelectionEnd);
}

std::string CTextEditor::GetCurrentLineText()const
{
	auto lineLength = GetLineMaxColumn(m_State.m_CursorPosition.m_nLine);
	return GetText(
		Coordinates(m_State.m_CursorPosition.m_nLine, 0),
		Coordinates(m_State.m_CursorPosition.m_nLine, lineLength));
}

void CTextEditor::ProcessInputs()
{
}

void CTextEditor::Colorize(int aFromLine, int aLines)
{
	int toLine = aLines == -1 ? (int)m_Lines.size() : std::min((int)m_Lines.size(), aFromLine + aLines);
	m_nColorRangeMin = std::min(m_nColorRangeMin, aFromLine);
	m_nColorRangeMax = std::max(m_nColorRangeMax, toLine);
	m_nColorRangeMin = std::max(0, m_nColorRangeMin);
	m_nColorRangeMax = std::max(m_nColorRangeMin, m_nColorRangeMax);
	m_bCheckComments = true;
}

void CTextEditor::ColorizeRange(int aFromLine, int aToLine)
{
	if (m_Lines.empty() || aFromLine >= aToLine)
		return;

	std::string buffer;
	std::cmatch results;
	std::string id;

	int endLine = std::max(0, std::min((int)m_Lines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i)
	{
		auto& line = m_Lines[i];

		if (line.empty())
			continue;

		buffer.resize(line.size());
		for (size_t j = 0; j < line.size(); ++j)
		{
			auto& col = line[j];
			buffer[j] = col.m_Char;
			col.m_ColorIndex = PaletteIndex::Default;
		}

		const char * bufferBegin = &buffer.front();
		const char * bufferEnd = bufferBegin + buffer.size();

		auto last = bufferEnd;

		for (auto first = bufferBegin; first != last; )
		{
			const char * token_begin = nullptr;
			const char * token_end = nullptr;
			PaletteIndex token_color = PaletteIndex::Default;

			bool hasTokenizeResult = false;

			if (m_LanguageDefinition.m_Tokenize != nullptr)
			{
				if (m_LanguageDefinition.m_Tokenize(first, last, token_begin, token_end, token_color))
					hasTokenizeResult = true;
			}

			if (hasTokenizeResult == false)
			{
				// todo : remove
				//printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

				for (auto& p : m_RegexList)
				{
					if (std::regex_search(first, last, results, p.first, std::regex_constants::match_continuous))
					{
						hasTokenizeResult = true;

						auto& v = *results.begin();
						token_begin = v.first;
						token_end = v.second;
						token_color = p.second;
						break;
					}
				}
			}

			if (hasTokenizeResult == false)
			{
				first++;
			}
			else
			{
				const size_t token_length = token_end - token_begin;

				if (token_color == PaletteIndex::Identifier)
				{
					id.assign(token_begin, token_end);

					// todo : almost all language definitions use lower case to specify keywords, so shouldn't this use ::tolower ?
					if (!m_LanguageDefinition.mCaseSensitive)
					{
						std::transform(id.begin(), id.end(), id.begin(),
							[](unsigned char c) { return static_cast<unsigned char>(::toupper(c)); });
					}

					if (!line[first - bufferBegin].m_bPreprocessor)
					{
						if (m_LanguageDefinition.m_Keywords.count(id) != 0)
							token_color = PaletteIndex::Keyword;
						else if (m_LanguageDefinition.m_Identifiers.count(id) != 0)
							token_color = PaletteIndex::KnownIdentifier;
						else if (m_LanguageDefinition.m_PreprocIdentifiers.count(id) != 0)
							token_color = PaletteIndex::PreprocIdentifier;
					}
					else
					{
						if (m_LanguageDefinition.m_PreprocIdentifiers.count(id) != 0)
							token_color = PaletteIndex::PreprocIdentifier;
					}
				}

				for (size_t j = 0; j < token_length; ++j)
					line[(token_begin - bufferBegin) + j].m_ColorIndex = token_color;

				first = token_end;
			}
		}
	}
}

void CTextEditor::ColorizeInternal()
{
	if (m_Lines.empty() || !m_bColorizerEnabled)
		return;

	if (m_bCheckComments)
	{
		auto endLine = m_Lines.size();
		auto endIndex = 0;
		auto commentStartLine = endLine;
		auto commentStartIndex = endIndex;
		auto withinString = false;
		auto withinSingleLineComment = false;
		auto withinPreproc = false;
		auto firstChar = true;			// there is no other non-whitespace characters in the line before
		auto concatenate = false;		// '\' on the very end of the line
		auto currentLine = 0;
		auto currentIndex = 0;
		while (currentLine < endLine || currentIndex < endIndex)
		{
			auto& line = m_Lines[currentLine];

			if (currentIndex == 0 && !concatenate)
			{
				withinSingleLineComment = false;
				withinPreproc = false;
				firstChar = true;
			}

			concatenate = false;

			if (!line.empty())
			{
				auto& g = line[currentIndex];
				auto c = g.m_Char;

				if (c != m_LanguageDefinition.m_PreprocChar && !isspace(c))
					firstChar = false;

				if (currentIndex == (int)line.size() - 1 && line[line.size() - 1].m_Char == '\\')
					concatenate = true;

				bool inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

				if (withinString)
				{
					line[currentIndex].m_bMultiLineComment = inComment;

					if (c == '\"')
					{
						if (currentIndex + 1 < (int)line.size() && line[currentIndex + 1].m_Char == '\"')
						{
							currentIndex += 1;
							if (currentIndex < (int)line.size())
								line[currentIndex].m_bMultiLineComment = inComment;
						}
						else
							withinString = false;
					}
					else if (c == '\\')
					{
						currentIndex += 1;
						if (currentIndex < (int)line.size())
							line[currentIndex].m_bMultiLineComment = inComment;
					}
				}
				else
				{
					if (firstChar && c == m_LanguageDefinition.m_PreprocChar)
						withinPreproc = true;

					if (c == '\"')
					{
						withinString = true;
						line[currentIndex].m_bMultiLineComment = inComment;
					}
					else
					{
						auto pred = [](const char& a, const Glyph& b) { return a == b.m_Char; };
						auto from = line.begin() + currentIndex;
						auto& startStr = m_LanguageDefinition.m_svCommentStart;
						auto& singleStartStr = m_LanguageDefinition.m_svSingleLineComment;

						if (singleStartStr.size() > 0 &&
							currentIndex + singleStartStr.size() <= line.size() &&
							equals(singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(), pred))
						{
							withinSingleLineComment = true;
						}
						else if (!withinSingleLineComment && currentIndex + startStr.size() <= line.size() &&
							equals(startStr.begin(), startStr.end(), from, from + startStr.size(), pred))
						{
							commentStartLine = currentLine;
							commentStartIndex = currentIndex;
						}

						inComment = inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

						line[currentIndex].m_bMultiLineComment = inComment;
						line[currentIndex].m_bComment = withinSingleLineComment;

						auto& endStr = m_LanguageDefinition.m_svCommentEnd;
						if (currentIndex + 1 >= (int)endStr.size() &&
							equals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred))
						{
							commentStartIndex = endIndex;
							commentStartLine = endLine;
						}
					}
				}
				line[currentIndex].m_bPreprocessor = withinPreproc;
				currentIndex += UTF8CharLength(c);
				if (currentIndex >= (int)line.size())
				{
					currentIndex = 0;
					++currentLine;
				}
			}
			else
			{
				currentIndex = 0;
				++currentLine;
			}
		}
		m_bCheckComments = false;
	}

	if (m_nColorRangeMin < m_nColorRangeMax)
	{
		const int increment = (m_LanguageDefinition.m_Tokenize == nullptr) ? 10 : 10000;
		const int to = std::min(m_nColorRangeMin + increment, m_nColorRangeMax);
		ColorizeRange(m_nColorRangeMin, to);
		m_nColorRangeMin = to;

		if (m_nColorRangeMax == m_nColorRangeMin)
		{
			m_nColorRangeMin = std::numeric_limits<int>::max();
			m_nColorRangeMax = 0;
		}
		return;
	}
}

float CTextEditor::TextDistanceToLineStart(const Coordinates& aFrom) const
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

void CTextEditor::EnsureCursorVisible()
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

int CTextEditor::GetPageSize() const
{
	auto height = ImGui::GetWindowHeight() - 20.0f;
	return (int)floor(height / m_CharAdvance.y);
}

CTextEditor::UndoRecord::UndoRecord(
	const std::string& aAdded,
	const CTextEditor::Coordinates aAddedStart,
	const CTextEditor::Coordinates aAddedEnd,
	const std::string& aRemoved,
	const CTextEditor::Coordinates aRemovedStart,
	const CTextEditor::Coordinates aRemovedEnd,
	CTextEditor::EditorState& aBefore,
	CTextEditor::EditorState& aAfter)
	: m_svAdded(aAdded)
	, m_AddedStart(aAddedStart)
	, m_AddedEnd(aAddedEnd)
	, m_svRemoved(aRemoved)
	, m_RemovedStart(aRemovedStart)
	, m_RemovedEnd(aRemovedEnd)
	, m_Before(aBefore)
	, m_After(aAfter)
{
	assert(m_AddedStart <= m_AddedEnd);
	assert(m_RemovedStart <= m_RemovedEnd);
}

void CTextEditor::UndoRecord::Undo(CTextEditor * aEditor)
{
	if (!m_svAdded.empty())
	{
		aEditor->DeleteRange(m_AddedStart, m_AddedEnd);
		aEditor->Colorize(m_AddedStart.m_nLine - 1, m_AddedEnd.m_nLine - m_AddedStart.m_nLine + 2);
	}

	if (!m_svRemoved.empty())
	{
		auto start = m_RemovedStart;
		aEditor->InsertTextAt(start, m_svRemoved.c_str());
		aEditor->Colorize(m_RemovedStart.m_nLine - 1, m_RemovedEnd.m_nLine - m_RemovedStart.m_nLine + 2);
	}

	aEditor->m_State = m_Before;
	aEditor->EnsureCursorVisible();

}

void CTextEditor::UndoRecord::Redo(CTextEditor * aEditor)
{
	if (!m_svRemoved.empty())
	{
		aEditor->DeleteRange(m_RemovedStart, m_RemovedEnd);
		aEditor->Colorize(m_RemovedStart.m_nLine - 1, m_RemovedEnd.m_nLine - m_RemovedStart.m_nLine + 1);
	}

	if (!m_svAdded.empty())
	{
		auto start = m_AddedStart;
		aEditor->InsertTextAt(start, m_svAdded.c_str());
		aEditor->Colorize(m_AddedStart.m_nLine - 1, m_AddedEnd.m_nLine - m_AddedStart.m_nLine + 1);
	}

	aEditor->m_State = m_After;
	aEditor->EnsureCursorVisible();
}

static bool TokenizeCStyleString(const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end)
{
	const char * p = in_begin;

	if (*p == '"')
	{
		p++;

		while (p < in_end)
		{
			// handle end of string
			if (*p == '"')
			{
				out_begin = in_begin;
				out_end = p + 1;
				return true;
			}

			// handle escape character for "
			if (*p == '\\' && p + 1 < in_end && p[1] == '"')
				p++;

			p++;
		}
	}

	return false;
}

static bool TokenizeCStyleCharacterLiteral(const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end)
{
	const char * p = in_begin;

	if (*p == '\'')
	{
		p++;

		// handle escape characters
		if (p < in_end && *p == '\\')
			p++;

		if (p < in_end)
			p++;

		// handle end of character literal
		if (p < in_end && *p == '\'')
		{
			out_begin = in_begin;
			out_end = p + 1;
			return true;
		}
	}

	return false;
}

static bool TokenizeCStyleIdentifier(const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end)
{
	const char * p = in_begin;

	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
	{
		p++;

		while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
			p++;

		out_begin = in_begin;
		out_end = p;
		return true;
	}

	return false;
}

static bool TokenizeCStyleNumber(const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end)
{
	const char * p = in_begin;

	const bool startsWithNumber = *p >= '0' && *p <= '9';

	if (*p != '+' && *p != '-' && !startsWithNumber)
		return false;

	p++;

	bool hasNumber = startsWithNumber;

	while (p < in_end && (*p >= '0' && *p <= '9'))
	{
		hasNumber = true;

		p++;
	}

	if (hasNumber == false)
		return false;

	bool isFloat = false;
	bool isHex = false;
	bool isBinary = false;

	if (p < in_end)
	{
		if (*p == '.')
		{
			isFloat = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '9'))
				p++;
		}
		else if (*p == 'x' || *p == 'X')
		{
			// hex formatted integer of the type 0xef80

			isHex = true;

			p++;

			while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
				p++;
		}
		else if (*p == 'b' || *p == 'B')
		{
			// binary formatted integer of the type 0b01011101

			isBinary = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '1'))
				p++;
		}
	}

	if (isHex == false && isBinary == false)
	{
		// floating point exponent
		if (p < in_end && (*p == 'e' || *p == 'E'))
		{
			isFloat = true;

			p++;

			if (p < in_end && (*p == '+' || *p == '-'))
				p++;

			bool hasDigits = false;

			while (p < in_end && (*p >= '0' && *p <= '9'))
			{
				hasDigits = true;

				p++;
			}

			if (hasDigits == false)
				return false;
		}

		// single precision floating point type
		if (p < in_end && *p == 'f')
			p++;
	}

	if (isFloat == false)
	{
		// integer size type
		while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
			p++;
	}

	out_begin = in_begin;
	out_end = p;
	return true;
}

static bool TokenizeCStylePunctuation(const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end)
{
	(void)in_end;

	switch (*in_begin)
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '!':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '-':
	case '+':
	case '=':
	case '~':
	case '|':
	case '<':
	case '>':
	case '?':
	case ':':
	case '/':
	case ';':
	case ',':
	case '.':
		out_begin = in_begin;
		out_end = in_begin + 1;
		return true;
	}

	return false;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::CPlusPlus()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const cppKeywords[] = {
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
			"compl", "concept", "const", "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
			"for", "friend", "goto", "if", "import", "inline", "int", "long", "module", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
			"register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized", "template", "this", "thread_local",
			"throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
		};
		for (auto& k : cppKeywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "printf", "sprintf", "snprintf", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper",
			"std", "string", "vector", "map", "unordered_map", "set", "unordered_set", "min", "max"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_Tokenize = [](const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end, PaletteIndex & paletteIndex) -> bool
		{
			paletteIndex = PaletteIndex::Max;

			while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end)
			{
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = PaletteIndex::Default;
			}
			else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::String;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::CharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Identifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Number;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Punctuation;

			return paletteIndex != PaletteIndex::Max;
		};

		langDef.m_svCommentStart = "/*";
		langDef.m_svCommentEnd = "*/";
		langDef.m_svSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.m_bAutoIndentation = true;

		langDef.m_svName = "C++";

		inited = true;
	}
	return langDef;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::HLSL()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
			"CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
			"export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
			"linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
			"pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
			"RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
			"static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
			"Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
			"VertexShader", "void", "volatile", "while",
			"bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
			"uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
			"float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
			"float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
			"half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
			"half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
		};
		for (auto& k : keywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
			"asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
			"ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
			"distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
			"f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
			"GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
			"InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
			"ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
			"Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
			"ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
			"radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
			"tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
			"tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.m_svCommentStart = "/*";
		langDef.m_svCommentEnd = "*/";
		langDef.m_svSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.m_bAutoIndentation = true;

		langDef.m_svName = "HLSL";

		inited = true;
	}
	return langDef;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::GLSL()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		for (auto& k : keywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.m_svCommentStart = "/*";
		langDef.m_svCommentEnd = "*/";
		langDef.m_svSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.m_bAutoIndentation = true;

		langDef.m_svName = "GLSL";

		inited = true;
	}
	return langDef;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::C()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		for (auto& k : keywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_Tokenize = [](const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end, PaletteIndex & paletteIndex) -> bool
		{
			paletteIndex = PaletteIndex::Max;

			while (in_begin < in_end && isascii(*in_begin) && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end)
			{
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = PaletteIndex::Default;
			}
			else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::String;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::CharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Identifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Number;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Punctuation;

			return paletteIndex != PaletteIndex::Max;
		};

		langDef.m_svCommentStart = "/*";
		langDef.m_svCommentEnd = "*/";
		langDef.m_svSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.m_bAutoIndentation = true;

		langDef.m_svName = "C";

		inited = true;
	}
	return langDef;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::SQL()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"ADD", "EXCEPT", "PERCENT", "ALL", "EXEC", "PLAN", "ALTER", "EXECUTE", "PRECISION", "AND", "EXISTS", "PRIMARY", "ANY", "EXIT", "PRINT", "AS", "FETCH", "PROC", "ASC", "FILE", "PROCEDURE",
			"AUTHORIZATION", "FILLFACTOR", "PUBLIC", "BACKUP", "FOR", "RAISERROR", "BEGIN", "FOREIGN", "READ", "BETWEEN", "FREETEXT", "READTEXT", "BREAK", "FREETEXTTABLE", "RECONFIGURE",
			"BROWSE", "FROM", "REFERENCES", "BULK", "FULL", "REPLICATION", "BY", "FUNCTION", "RESTORE", "CASCADE", "GOTO", "RESTRICT", "CASE", "GRANT", "RETURN", "CHECK", "GROUP", "REVOKE",
			"CHECKPOINT", "HAVING", "RIGHT", "CLOSE", "HOLDLOCK", "ROLLBACK", "CLUSTERED", "IDENTITY", "ROWCOUNT", "COALESCE", "IDENTITY_INSERT", "ROWGUIDCOL", "COLLATE", "IDENTITYCOL", "RULE",
			"COLUMN", "IF", "SAVE", "COMMIT", "IN", "SCHEMA", "COMPUTE", "INDEX", "SELECT", "CONSTRAINT", "INNER", "SESSION_USER", "CONTAINS", "INSERT", "SET", "CONTAINSTABLE", "INTERSECT", "SETUSER",
			"CONTINUE", "INTO", "SHUTDOWN", "CONVERT", "IS", "SOME", "CREATE", "JOIN", "STATISTICS", "CROSS", "KEY", "SYSTEM_USER", "CURRENT", "KILL", "TABLE", "CURRENT_DATE", "LEFT", "TEXTSIZE",
			"CURRENT_TIME", "LIKE", "THEN", "CURRENT_TIMESTAMP", "LINENO", "TO", "CURRENT_USER", "LOAD", "TOP", "CURSOR", "NATIONAL", "TRAN", "DATABASE", "NOCHECK", "TRANSACTION",
			"DBCC", "NONCLUSTERED", "TRIGGER", "DEALLOCATE", "NOT", "TRUNCATE", "DECLARE", "NULL", "TSEQUAL", "DEFAULT", "NULLIF", "UNION", "DELETE", "OF", "UNIQUE", "DENY", "OFF", "UPDATE",
			"DESC", "OFFSETS", "UPDATETEXT", "DISK", "ON", "USE", "DISTINCT", "OPEN", "USER", "DISTRIBUTED", "OPENDATASOURCE", "VALUES", "DOUBLE", "OPENQUERY", "VARYING","DROP", "OPENROWSET", "VIEW",
			"DUMMY", "OPENXML", "WAITFOR", "DUMP", "OPTION", "WHEN", "ELSE", "OR", "WHERE", "END", "ORDER", "WHILE", "ERRLVL", "OUTER", "WITH", "ESCAPE", "OVER", "WRITETEXT"
		};

		for (auto& k : keywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"ABS",  "ACOS",  "ADD_MONTHS",  "ASCII",  "ASCIISTR",  "ASIN",  "ATAN",  "ATAN2",  "AVG",  "BFILENAME",  "BIN_TO_NUM",  "BITAND",  "CARDINALITY",  "CASE",  "CAST",  "CEIL",
			"CHARTOROWID",  "CHR",  "COALESCE",  "COMPOSE",  "CONCAT",  "CONVERT",  "CORR",  "COS",  "COSH",  "COUNT",  "COVAR_POP",  "COVAR_SAMP",  "CUME_DIST",  "CURRENT_DATE",
			"CURRENT_TIMESTAMP",  "DBTIMEZONE",  "DECODE",  "DECOMPOSE",  "DENSE_RANK",  "DUMP",  "EMPTY_BLOB",  "EMPTY_CLOB",  "EXP",  "EXTRACT",  "FIRST_VALUE",  "FLOOR",  "FROM_TZ",  "GREATEST",
			"GROUP_ID",  "HEXTORAW",  "INITCAP",  "INSTR",  "INSTR2",  "INSTR4",  "INSTRB",  "INSTRC",  "LAG",  "LAST_DAY",  "LAST_VALUE",  "LEAD",  "LEAST",  "LENGTH",  "LENGTH2",  "LENGTH4",
			"LENGTHB",  "LENGTHC",  "LISTAGG",  "LN",  "LNNVL",  "LOCALTIMESTAMP",  "LOG",  "LOWER",  "LPAD",  "LTRIM",  "MAX",  "MEDIAN",  "MIN",  "MOD",  "MONTHS_BETWEEN",  "NANVL",  "NCHR",
			"NEW_TIME",  "NEXT_DAY",  "NTH_VALUE",  "NULLIF",  "NUMTODSINTERVAL",  "NUMTOYMINTERVAL",  "NVL",  "NVL2",  "POWER",  "RANK",  "RAWTOHEX",  "REGEXP_COUNT",  "REGEXP_INSTR",
			"REGEXP_REPLACE",  "REGEXP_SUBSTR",  "REMAINDER",  "REPLACE",  "ROUND",  "ROWNUM",  "RPAD",  "RTRIM",  "SESSIONTIMEZONE",  "SIGN",  "SIN",  "SINH",
			"SOUNDEX",  "SQRT",  "STDDEV",  "SUBSTR",  "SUM",  "SYS_CONTEXT",  "SYSDATE",  "SYSTIMESTAMP",  "TAN",  "TANH",  "TO_CHAR",  "TO_CLOB",  "TO_DATE",  "TO_DSINTERVAL",  "TO_LOB",
			"TO_MULTI_BYTE",  "TO_NCLOB",  "TO_NUMBER",  "TO_SINGLE_BYTE",  "TO_TIMESTAMP",  "TO_TIMESTAMP_TZ",  "TO_YMINTERVAL",  "TRANSLATE",  "TRIM",  "TRUNC", "TZ_OFFSET",  "UID",  "UPPER",
			"USER",  "USERENV",  "VAR_POP",  "VAR_SAMP",  "VARIANCE",  "VSIZE "
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.m_svCommentStart = "/*";
		langDef.m_svCommentEnd = "*/";
		langDef.m_svSingleLineComment = "//";

		langDef.mCaseSensitive = false;
		langDef.m_bAutoIndentation = false;

		langDef.m_svName = "SQL";

		inited = true;
	}
	return langDef;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::AngelScript()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"and", "abstract", "auto", "bool", "break", "case", "cast", "class", "const", "continue", "default", "do", "double", "else", "enum", "false", "final", "float", "for",
			"from", "funcdef", "function", "get", "if", "import", "in", "inout", "int", "interface", "int8", "int16", "int32", "int64", "is", "mixin", "namespace", "not",
			"null", "or", "out", "override", "private", "protected", "return", "set", "shared", "super", "switch", "this ", "true", "typedef", "uint", "uint8", "uint16", "uint32",
			"uint64", "void", "while", "xor"
		};

		for (auto& k : keywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"cos", "sin", "tab", "acos", "asin", "atan", "atan2", "cosh", "sinh", "tanh", "log", "log10", "pow", "sqrt", "abs", "ceil", "floor", "fraction", "closeTo", "fpFromIEEE", "fpToIEEE",
			"complex", "opEquals", "opAddAssign", "opSubAssign", "opMulAssign", "opDivAssign", "opAdd", "opSub", "opMul", "opDiv"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.m_svCommentStart = "/*";
		langDef.m_svCommentEnd = "*/";
		langDef.m_svSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.m_bAutoIndentation = true;

		langDef.m_svName = "AngelScript";

		inited = true;
	}
	return langDef;
}

const CTextEditor::LanguageDefinition& CTextEditor::LanguageDefinition::Lua()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"and", "break", "do", "", "else", "elseif", "end", "false", "for", "function", "if", "in", "", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"
		};

		for (auto& k : keywords)
			langDef.m_Keywords.insert(k);

		static const char* const identifiers[] = {
			"assert", "collectgarbage", "dofile", "error", "getmetatable", "ipairs", "loadfile", "load", "loadstring",  "next",  "pairs",  "pcall",  "print",  "rawequal",  "rawlen",  "rawget",  "rawset",
			"select",  "setmetatable",  "tonumber",  "tostring",  "type",  "xpcall",  "_G",  "_VERSION","arshift", "band", "bnot", "bor", "bxor", "btest", "extract", "lrotate", "lshift", "replace",
			"rrotate", "rshift", "create", "resume", "running", "status", "wrap", "yield", "isyieldable", "debug","getuservalue", "gethook", "getinfo", "getlocal", "getregistry", "getmetatable",
			"getupvalue", "upvaluejoin", "upvalueid", "setuservalue", "sethook", "setlocal", "setmetatable", "setupvalue", "traceback", "close", "flush", "input", "lines", "open", "output", "popen",
			"read", "tmpfile", "type", "write", "close", "flush", "lines", "read", "seek", "setvbuf", "write", "__gc", "__tostring", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "tointeger",
			"floor", "fmod", "ult", "log", "max", "min", "modf", "rad", "random", "randomseed", "sin", "sqrt", "string", "tan", "type", "atan2", "cosh", "sinh", "tanh",
			"pow", "frexp", "ldexp", "log10", "pi", "huge", "maxinteger", "mininteger", "loadlib", "searchpath", "seeall", "preload", "cpath", "path", "searchers", "loaded", "module", "require", "clock",
			"date", "difftime", "execute", "exit", "getenv", "remove", "rename", "setlocale", "time", "tmpname", "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len", "lower", "match", "rep",
			"reverse", "sub", "upper", "pack", "packsize", "unpack", "concat", "maxn", "insert", "pack", "unpack", "remove", "move", "sort", "offset", "codepoint", "char", "len", "codes", "charpattern",
			"coroutine", "table", "io", "os", "string", "utf8", "bit32", "math", "debug", "package"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.m_svDeclaration = "Built-in function";
			langDef.m_Identifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.m_TokenRegexStrings.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.m_svCommentStart = "--[[";
		langDef.m_svCommentEnd = "]]";
		langDef.m_svSingleLineComment = "--";

		langDef.mCaseSensitive = true;
		langDef.m_bAutoIndentation = false;

		langDef.m_svName = "Lua";

		inited = true;
	}
	return langDef;
}
