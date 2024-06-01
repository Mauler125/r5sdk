#pragma once

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <regex>
#include "imgui.h"

class CTextEditor
{
public:
	enum class PaletteIndex
	{
		Default,
		Keyword,
		Number,
		String,
		CharLiteral,
		Punctuation,
		Preprocessor,
		Identifier,
		KnownIdentifier,
		PreprocIdentifier,
		Comment,
		MultiLineComment,
		Background,
		Cursor,
		Selection,
		ErrorMarker,
		Breakpoint,
		LineNumber,
		CurrentLineFill,
		CurrentLineFillInactive,
		CurrentLineEdge,
		Max
	};

	enum class SelectionMode
	{
		Normal,
		Word,
		Line
	};

	struct Breakpoint
	{
		int mLine;
		bool mEnabled;
		std::string mCondition;

		Breakpoint()
			: mLine(-1)
			, mEnabled(false)
		{}
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

		int m_nLine, m_nColumn;
	};

	struct Identifier
	{
		Coordinates m_Location;
		std::string m_svDeclaration;
	};

	typedef std::string String;
	typedef std::unordered_map<std::string, Identifier> Identifiers;
	typedef std::unordered_set<std::string> Keywords;
	typedef std::map<int, std::string> ErrorMarkers;
	typedef std::unordered_set<int> Breakpoints;
	typedef std::array<ImU32, (unsigned)PaletteIndex::Max> Palette;
	typedef uint8_t Char;

	struct Glyph
	{
		Char m_Char;
		PaletteIndex m_ColorIndex = PaletteIndex::Default;
		bool m_bComment : 1;
		bool m_bMultiLineComment : 1;
		bool m_bPreprocessor : 1;

		Glyph(Char aChar, PaletteIndex aColorIndex) : m_Char(aChar), m_ColorIndex(aColorIndex),
			m_bComment(false), m_bMultiLineComment(false), m_bPreprocessor(false) {}
	};

	typedef std::vector<Glyph> Line;
	typedef std::vector<Line> Lines;

	struct LanguageDefinition
	{
		typedef std::pair<std::string, PaletteIndex> TokenRegexString;
		typedef std::vector<TokenRegexString> TokenRegexStrings;
		typedef bool(*TokenizeCallback)(const char * in_begin, const char * in_end, const char *& out_begin, const char *& out_end, PaletteIndex & paletteIndex);

		std::string m_svName;
		Keywords m_Keywords;
		Identifiers m_Identifiers;
		Identifiers m_PreprocIdentifiers;
		std::string m_svCommentStart, m_svCommentEnd, m_svSingleLineComment;
		char m_PreprocChar;
		bool m_bAutoIndentation;

		TokenizeCallback m_Tokenize;

		TokenRegexStrings m_TokenRegexStrings;

		bool mCaseSensitive;

		LanguageDefinition()
			: m_PreprocChar('#'), m_bAutoIndentation(true), m_Tokenize(nullptr), mCaseSensitive(true)
		{
		}

		static const LanguageDefinition& CPlusPlus();
		static const LanguageDefinition& HLSL();
		static const LanguageDefinition& GLSL();
		static const LanguageDefinition& C();
		static const LanguageDefinition& SQL();
		static const LanguageDefinition& AngelScript();
		static const LanguageDefinition& Lua();
	};

	CTextEditor();
	~CTextEditor();

	void SetLanguageDefinition(const LanguageDefinition& aLanguageDef);
	const LanguageDefinition& GetLanguageDefinition() const { return m_LanguageDefinition; }

	const Palette& GetPalette() const { return m_PaletteBase; }
	void SetPalette(const Palette& aValue);

	void SetErrorMarkers(const ErrorMarkers& aMarkers) { m_ErrorMarkers = aMarkers; }
	void SetBreakpoints(const Breakpoints& aMarkers) { m_Breakpoints = aMarkers; }

	void Render(const char* aTitle, const ImVec2& aSize = ImVec2(), bool aBorder = false);
	void SetText(const std::string& aText);
	std::string GetText() const;

	void SetTextLines(const std::vector<std::string>& aLines);
	std::vector<std::string> GetTextLines() const;

	std::string GetSelectedText() const;
	std::string GetCurrentLineText()const;

	int GetTotalLines() const { return (int)m_Lines.size(); }
	bool IsOverwrite() const { return m_Overwrite; }

	void SetReadOnly(bool aValue);
	bool IsReadOnly() const { return m_bReadOnly; }
	bool IsTextChanged() const { return m_bTextChanged; }
	bool IsCursorPositionChanged() const { return m_bCursorPositionChanged; }

	bool IsColorizerEnabled() const { return m_bColorizerEnabled; }
	void SetColorizerEnable(bool aValue);

	Coordinates GetCursorPosition() const { return GetActualCursorCoordinates(); }
	void SetCursorPosition(const Coordinates& aPosition);

	inline void SetHandleMouseInputs    (bool aValue){ m_bHandleMouseInputs    = aValue;}
	inline bool IsHandleMouseInputsEnabled() const { return m_bHandleMouseInputs; }

	inline void SetHandleKeyboardInputs (bool aValue){ m_bHandleKeyboardInputs = aValue;}
	inline bool IsHandleKeyboardInputsEnabled() const { return m_bHandleKeyboardInputs; }

	inline void SetImGuiChildIgnored    (bool aValue){ m_bIgnoreImGuiChild     = aValue;}
	inline bool IsImGuiChildIgnored() const { return m_bIgnoreImGuiChild; }

	inline void SetShowWhitespaces(bool aValue) { m_bShowWhitespaces = aValue; }
	inline bool IsShowingWhitespaces() const { return m_bShowWhitespaces; }

	void SetTabSize(int aValue);
	inline int GetTabSize() const { return m_nTabSize; }

	void InsertText(const std::string& aValue);
	void InsertText(const char* aValue);

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

	void Copy();
	void Cut();
	void Paste();
	void Delete();

	bool CanUndo() const;
	bool CanRedo() const;
	void Undo(int aSteps = 1);
	void Redo(int aSteps = 1);

	static const Palette& GetDarkPalette();
	static const Palette& GetLightPalette();
	static const Palette& GetRetroBluePalette();

private:
	typedef std::vector<std::pair<std::regex, PaletteIndex>> RegexList;

	struct EditorState
	{
		Coordinates m_SelectionStart;
		Coordinates m_SelectionEnd;
		Coordinates m_CursorPosition;
	};

	class UndoRecord
	{
	public:
		UndoRecord() {}
		~UndoRecord() {}

		UndoRecord(
			const std::string& aAdded,
			const CTextEditor::Coordinates aAddedStart,
			const CTextEditor::Coordinates aAddedEnd,

			const std::string& aRemoved,
			const CTextEditor::Coordinates aRemovedStart,
			const CTextEditor::Coordinates aRemovedEnd,

			CTextEditor::EditorState& aBefore,
			CTextEditor::EditorState& aAfter);

		void Undo(CTextEditor* aEditor);
		void Redo(CTextEditor* aEditor);

		std::string m_svAdded;
		Coordinates m_AddedStart;
		Coordinates m_AddedEnd;

		std::string m_svRemoved;
		Coordinates m_RemovedStart;
		Coordinates m_RemovedEnd;

		EditorState m_Before;
		EditorState m_After;
	};

	typedef std::vector<UndoRecord> UndoBuffer;

	void ProcessInputs();
	void Colorize(int aFromLine = 0, int aCount = -1);
	void ColorizeRange(int aFromLine = 0, int aToLine = 0);
	void ColorizeInternal();
	float TextDistanceToLineStart(const Coordinates& aFrom) const;
	void EnsureCursorVisible();
	int GetPageSize() const;
	std::string GetText(const Coordinates& aStart, const Coordinates& aEnd) const;
	Coordinates GetActualCursorCoordinates() const;
	Coordinates SanitizeCoordinates(const Coordinates& aValue) const;
	void Advance(Coordinates& aCoordinates) const;
	void DeleteRange(const Coordinates& aStart, const Coordinates& aEnd);
	int InsertTextAt(Coordinates& aWhere, const char* aValue);
	void AddUndo(UndoRecord& aValue);
	Coordinates ScreenPosToCoordinates(const ImVec2& aPosition) const;
	Coordinates FindWordStart(const Coordinates& aFrom) const;
	Coordinates FindWordEnd(const Coordinates& aFrom) const;
	Coordinates FindNextWord(const Coordinates& aFrom) const;
	int GetCharacterIndex(const Coordinates& aCoordinates) const;
	int GetCharacterColumn(int aLine, int aIndex) const;
	int GetLineCharacterCount(int aLine) const;
	int GetLineMaxColumn(int aLine) const;
	bool IsOnWordBoundary(const Coordinates& aAt) const;
	void RemoveLine(int aStart, int aEnd);
	void RemoveLine(int aIndex);
	Line& InsertLine(int aIndex);
	void EnterCharacter(ImWchar aChar, bool aShift);
	void Backspace();
	void DeleteSelection();
	std::string GetWordUnderCursor() const;
	std::string GetWordAt(const Coordinates& aCoords) const;
	ImU32 GetGlyphColor(const Glyph& aGlyph) const;

	void HandleKeyboardInputs();
	void HandleMouseInputs();
	void Render();

	float m_flLineSpacing;
	Lines m_Lines;
	EditorState m_State;
	UndoBuffer m_UndoBuffer;
	int m_nUndoIndex;

	int m_nTabSize;
	bool m_Overwrite;
	bool m_bReadOnly;
	bool m_bWithinRender;
	bool m_bScrollToCursor;
	bool m_bScrollToTop;
	bool m_bTextChanged;
	bool m_bColorizerEnabled;
	float m_flTextStart;                   // position (in pixels) where a code line starts relative to the left of the TextEditor.
	int  m_nLeftMargin;
	bool m_bCursorPositionChanged;
	int m_nColorRangeMin;
	int m_nColorRangeMax;
	SelectionMode m_SelectionMode;
	bool m_bHandleKeyboardInputs;
	bool m_bHandleMouseInputs;
	bool m_bIgnoreImGuiChild;
	bool m_bShowWhitespaces;

	Palette m_PaletteBase;
	Palette m_Palette;
	LanguageDefinition m_LanguageDefinition;
	RegexList m_RegexList;

	bool m_bCheckComments;
	Breakpoints m_Breakpoints;
	ErrorMarkers m_ErrorMarkers;
	ImVec2 m_CharAdvance;
	Coordinates m_InteractiveStart;
	Coordinates m_InteractiveEnd;
	std::string m__svLineBuffer;
	uint64_t m_nStartTime;

	float m_flLastClick;
};
