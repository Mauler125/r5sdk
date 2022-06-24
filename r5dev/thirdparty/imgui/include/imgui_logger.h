#pragma once

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <regex>
#include <thread>
#include <mutex>
#include "imgui.h"

struct CConLog
{
	CConLog(const std::string& svConLog, const ImVec4& imColor)
	{
		m_svConLog = svConLog;
		m_imColor = imColor;
	}
	std::string m_svConLog;
	ImVec4 m_imColor;
};

class CTextLogger
{
public:

	enum class SelectionMode
	{
		Normal,
		Word,
		Line
	};

	// Represents a character coordinate from the user's point of view,
	// i. e. consider an uniform grid (assuming fixed-width font) on the
	// screen as it is rendered, and each cell has its own coordinate, starting from 0.
	// Tabs are counted as [1..mTabSize] count empty spaces, depending on
	// how many space is necessary to reach the next tab stop.
	// For example, coordinate (1, 5) represents the character 'B' in a line "\tABC", when mTabSize = 4,
	// because it is rendered as "    ABC" on the screen.
	struct Coordinates
	{
		int m_nLine, m_nColumn;
		Coordinates() : m_nLine(0), m_nColumn(0) {}
		Coordinates(int aLine, int aColumn) : m_nLine(aLine), m_nColumn(aColumn)
		{
			assert(aLine >= 0);
			assert(aColumn >= 0);
		}
		static Coordinates Invalid() { static Coordinates invalid(-1, -1); return invalid; }

		bool operator ==(const Coordinates& o) const
		{
			return
				m_nLine == o.m_nLine &&
				m_nColumn == o.m_nColumn;
		}

		bool operator !=(const Coordinates& o) const
		{
			return
				m_nLine != o.m_nLine ||
				m_nColumn != o.m_nColumn;
		}

		bool operator <(const Coordinates& o) const
		{
			if (m_nLine != o.m_nLine)
				return m_nLine < o.m_nLine;
			return m_nColumn < o.m_nColumn;
		}

		bool operator >(const Coordinates& o) const
		{
			if (m_nLine != o.m_nLine)
				return m_nLine > o.m_nLine;
			return m_nColumn > o.m_nColumn;
		}

		bool operator <=(const Coordinates& o) const
		{
			if (m_nLine != o.m_nLine)
				return m_nLine < o.m_nLine;
			return m_nColumn <= o.m_nColumn;
		}

		bool operator >=(const Coordinates& o) const
		{
			if (m_nLine != o.m_nLine)
				return m_nLine > o.m_nLine;
			return m_nColumn >= o.m_nColumn;
		}
	};

	typedef std::string String;
	typedef uint8_t Char;

	struct Glyph
	{
		Char m_Char;
		ImVec4 m_Color = ImVec4(0.23f, 0.47f, 0.85f, 1.00f);

		Glyph(Char aChar, ImVec4 aColor = ImVec4(0.80f, 0.80f, 0.80f, 1.00f)) : m_Char(aChar), m_Color(aColor) {}
	};

	typedef std::vector<Glyph> Line;
	typedef std::vector<Line> Lines;

	CTextLogger();
	~CTextLogger();

	void Render();
	void Copy(bool aCopyAll = false);

	void SetText(const CConLog& aText);
	std::string GetText() const;

	void SetTextLines(const std::vector<CConLog>& aLines);
	std::vector<std::string> GetTextLines() const;

	ImGuiTextFilter GetFilter() const { return m_itFilter; };
	std::string GetSelectedText() const;
	std::string GetCurrentLineText() const;
	std::string GetTextFromLine(const Line& aLine) const;

	int GetTotalFilterMatches() const;
	int GetTotalLines() const { return (int)m_Lines.size(); }

	Coordinates GetCursorPosition() const { return GetActualCursorCoordinates(); }

	void SetCursorPosition(const Coordinates& aPosition);
	bool IsCursorPositionChanged() const { return m_bCursorPositionChanged; }

	inline void SetHandleMouseInputs    (bool aValue){ m_bHandleMouseInputs    = aValue;}
	inline bool IsHandleMouseInputsEnabled() const { return m_bHandleKeyboardInputs; }

	inline void SetHandleKeyboardInputs (bool aValue){ m_bHandleKeyboardInputs = aValue;}
	inline bool IsHandleKeyboardInputsEnabled() const { return m_bHandleKeyboardInputs; }

	inline void SetShowWhitespaces(bool aValue) { m_bShowWhiteSpaces = aValue; }
	inline bool IsShowingWhitespaces() const { return m_bShowWhiteSpaces; }

	void SetTabSize(int aValue);
	inline int GetTabSize() const { return m_nTabSize; }

	void InsertText(const CConLog& aValue);

	void MoveUp(int aAmount = 1, bool aSelect = false);
	void MoveDown(int aAmount = 1, bool aSelect = false);
	void MoveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveRight(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveTop(bool aSelect = false);
	void MoveBottom(bool aSelect = false);
	void MoveHome(bool aSelect = false);
	void MoveEnd(bool aSelect = false);

	void SetSelectionStart(const Coordinates& aPosition);
	void SetSelectionEnd(const Coordinates& aPosition);
	void SetSelection(const Coordinates& aStart, const Coordinates& aEnd, SelectionMode aMode = SelectionMode::Normal);
	void SelectWordUnderCursor();
	void SelectAll();
	bool HasSelection() const;

	void RemoveLine(int aStart, int aEnd, bool aInternal = false);
	void RemoveLine(int aIndex, bool aInternal = false);

private:
	struct EditorState
	{
		Coordinates m_SelectionStart;
		Coordinates m_SelectionEnd;
		Coordinates m_CursorPosition;
	};

	float TextDistanceToLineStart(const Coordinates& aFrom) const;
	void EnsureCursorVisible();
	int GetPageSize() const;
	std::string GetText(const Coordinates& aStart, const Coordinates& aEnd) const;
	Coordinates GetActualLastLineCoordinates() const;
	Coordinates GetActualCursorCoordinates() const;
	Coordinates SanitizeCoordinates(const Coordinates& aValue) const;
	void Advance(Coordinates& aCoordinates) const;
	void DeleteRange(const Coordinates& aStart, const Coordinates& aEnd);
	int InsertTextAt(Coordinates& aWhere, const char* aValue, ImVec4 aColor);
	Coordinates ScreenPosToCoordinates(const ImVec2& aPosition) const;
	Coordinates FindWordStart(const Coordinates& aFrom) const;
	Coordinates FindWordEnd(const Coordinates& aFrom) const;
	Coordinates FindNextWord(const Coordinates& aFrom) const;
	int GetCharacterIndex(const Coordinates& aCoordinates) const;
	int GetCharacterColumn(int aLine, int aIndex) const;
	int GetLineCharacterCount(int aLine) const;
	int GetLineMaxColumn(int aLine) const;
	bool IsOnWordBoundary(const Coordinates& aAt) const;
	Line& InsertLine(int aIndex);
	std::string GetWordUnderCursor() const;
	std::string GetWordAt(const Coordinates& aCoords) const;
	ImU32 GetGlyphColor(const Glyph& aGlyph) const;

	void HandleKeyboardInputs(bool bHoveredScrollbar, bool bActiveScrollbar);
	void HandleMouseInputs(bool bHoveredScrollbar, bool bActiveScrollbar);

public:
	bool m_bAutoScroll;
	bool m_bScrollToBottom;
private:
	bool m_bHandleKeyboardInputs;
	bool m_bHandleMouseInputs;
	bool m_bShowWhiteSpaces;
	bool m_bScrollToCursor;
	bool m_bCursorPositionChanged;
	float m_flTextStart;                   // position (in pixels) where a code line starts relative to the left of the TextLogger.
	float m_flLineSpacing;
	double m_flLastClick;
	int m_nTabSize;
	int m_nLeftMargin;
	int m_nColorRangeMin;
	int m_nColorRangeMax;
	uint64_t m_nStartTime;

	SelectionMode m_SelectionMode;
	Coordinates m_InteractiveStart;
	Coordinates m_InteractiveEnd;
	ImVec2 m_CharAdvance;

	Lines m_Lines;
	EditorState m_State;
	std::mutex m_Mutex;
	std::string m_svLineBuffer;
public:
	ImGuiTextFilter m_itFilter;
};
