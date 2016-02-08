/* Copyright (C) 2016 - James Terman
 *
 * milo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * milo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file milo_ncurses.cpp
 * This file implements CursesGraphics that implments the ABC Graphics
 * for ncurses.
 */

#include <ncursesw/ncurses.h>
#include <locale.h>
#include <unordered_map>
#include <map>
#include "ui.h"

using namespace std;

/**
 * Class derived from ABC Graphics to allow milo to draw to a ncurses ascii screen.
 */
class CursesGraphics : public Graphics
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	CursesGraphics() { 
		if (!init) {
			setenv("TERM", "xterm-milo", true);
			setlocale(LC_ALL,"");
			initscr(); raw(); noecho();
#ifndef DEBUG
			curs_set(0);
#endif
			keypad(stdscr, TRUE); mouseinterval(300);
			mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
			m_has_colors = (has_colors() == TRUE);
			if (m_has_colors) {
				start_color();/* Start color */
				init_color(COLOR_WHITE, 1000, 1000, 1000);
				assume_default_colors(COLOR_BLACK, COLOR_WHITE);
				for (int i = Graphics::Color::RED; i <= Graphics::Color::WHITE; ++i)
					init_pair(i, COLOR_BLACK + i, COLOR_WHITE);
			}
			init = true;
		}
	}
	~CursesGraphics() { endwin(); }
	//@}

	/** @name Virtual Public Member Functions */
	//@{

	/**
	 * Get mouse poition from Graphics context
	 * @param[out] xMouse Horizontal coordinate of mouse
	 * @param[out] yMouse Vertical coordinate of mouse
	 */
	void getMouseCoords(int& xMouse, int& yMouse) { xMouse = m_xMouse; yMouse = m_yMouse; }

	/**
	 * Get next event.
	 * Blocking function that returns a reference to an base class that defines
	 * an interface to get event info.
	 * @param yCursor horiz coord of cursor
	 * @param xCursor vertical coord of cursor
	 * @param fBlink  If true, blink cursor
	 * @return Reference to UI::Event object
	 */
	const UI::Event& getNextEvent(int xCursorX, int yCursor, bool fBlink);
	
	/**
	 * Get height of text in lines.
	 * @return All ASCII characters are one line in height.
	 */
	int getTextHeight() { return 1; }

	/**
	 * Get length of string in characters.
	 * @return Length of string in characters.
	 */
	int getTextLength(const std::string& s) { return s.length(); }

	/**
	 * Length of character in ASCII is 1.
	 * @return Return one.
	 */
	int getCharLength(char) { return 1; }

	/**
	 * Get width of a parenthesis in characters which is always 1.
	 * @return Return one.
	 */
	int getParenthesisWidth(int) { return 1; }

	/**
	 * Get height of line in a division node which is always 1.
	 * @return Return one.
	 */
	int getDivideLineHeight() { return 1; }

	/**
	 * Get height of differential in lines of text.
	 * @param c Variable name of differential.
	 * @return Height of differential in lines of text.
	 */
	int getDifferentialHeight(char) { return 3; }

	/**
	 * Get width of differential in lines of text.
	 * @param c Variable name of differential.
	 * @return Width of differential in lines of text.
	 */
	int getDifferentialWidth(char)  { return 2; }

	/**
	 * Get vertical offset of differential.
	 * @param c Variable name of differential.
	 * @return Vertical offset of differential in lines of text.
	 */
	int getDifferentialBase(char)   { return 1; }

	/**
	 * Draw a character at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param c  Character to be drawn at x0,y0.
	 * @param chrAttr Attribute of character.
	 * @param color Color of line.
	 */
	void at(int x, int y, int c, Attributes chrAttr, Color color = BLACK) {
		auto iter = char_map.find((char)c);
		if (iter == char_map.end()) {
			if (chrAttr != NONE) c |= attribute_map.at(chrAttr);
			if (m_select.inside(x, y)) c |= A_REVERSE;
			if (color != BLACK && m_has_colors) attron(COLOR_PAIR(color));
			mvaddch(y + m_yOrig, x + m_xOrig, c);
			if (color != BLACK && m_has_colors) attroff(COLOR_PAIR(color));
		}
		else {
			at(x, y, iter->second, chrAttr, color);
		}
	}

	/**
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param s  String to be drawn at x0,y0.
	 * @param chrAttr Attribute of character.
	 * @param color Color of line.
	 */
	void at(int x, int y, const string& s, Attributes chrAttr, Color color = BLACK) {
		if (color != BLACK && m_has_colors) attron(COLOR_PAIR(color));
		if (m_select.inside(x, y)) attron(A_REVERSE);
		if (chrAttr != NONE) attron(attribute_map.at(chrAttr));

		move(y + m_yOrig, x + m_xOrig); printw(s.c_str());

		if (color != BLACK && m_has_colors) attroff(COLOR_PAIR(color));
		if (m_select.inside(x, y)) attroff(A_REVERSE);
		if (chrAttr != NONE) attroff(attribute_map.at(chrAttr));
	}

	/**
	 * Draw a c-string at x,y with black color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param s  C-String to be drawn at x0,y0.
	 * @param chrAttr Attribute of character.
	 * @param color Color of line.
	 */
	void at(int x, int y, const char* s) {
		at(x, y, string(s), NONE);
	}

	/**
	 * Flush all characters to the ncurses screen.
	 */
	void out() { refresh(); }

	/**
	 * Set size of this graphics window (origin is always 0,0).
	 * @param x Horizontal size of graphics window.
	 * @param y Vertical size of graphics window.
	 * @param x0 Not used.
	 * @param y0 Not used.
	 */
	void set(int x, int y, int, int) { 
		int row, col;
		getmaxyx(stdscr, row, col);
		Graphics::set(x, y, (col - x)/2, (row - y)/2);
		clear();
	}

	/**
	 * Draw a horizontal line starting at x0,y0 length x_size.
	 * @param x_size Horizontal size of both line.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 */
	void horiz_line(int x_size, int x0, int y0) { 
		for (int i = 0; i < x_size; ++i) at(x0 + i, y0, ACS_HLINE);
	}

	/**
	 * Draw differential of width x0 and height y0 with char variable name.
	 * @param x0 Horizontal origin of differential.
	 * @param y0 Vertical origin of differential.
	 * @param variable Name of variable of differential.
	 */
	void differential(int x0, int y0, char variable);

	/**
	 * Draw pair of parenthesis around a node of size x_size, y_size and origin x0,y0.
	 * @param x_size Horizontal size of both parenthesis.
	 * @param y_size Vertical size of parenthesis.
	 * @param x0 Horizontal origin of parenthesis.
	 * @param y0 Vertical origin of parenthesis.
	 */
	void parenthesis(int x_size, int y_size, int x0, int y0);

	/**
	 * Select the area of size x,y at origin x0,y0.
	 * @param x Horizontal size of both selection area.
	 * @param y Vertical size of selection area.
	 * @param x0 Horizontal origin of selection area.
	 * @param y0 Vertical origin of selection area.
	 */
	void setSelect(int x, int y, int x0, int y0) { 
		Graphics::setSelect(x, y, x0, y0);
		for (int j = 0; j < y; ++j) mvchgat(m_yOrig + y0 + j, m_xOrig + x0, x, A_REVERSE, 0, NULL);
	}
	//@}

	/**
	 * Return character drawn at screen coordinates.
	 * @param x Horizontal coordinate.
	 * @param y Vertical coordinate.
	 * @return Character at x,y coordinate.
	 */
	unsigned int ins(int x, int y) {
		return mvinch(y + m_yOrig, x + m_xOrig);
	}

	/**
	 * Return character from key press. Put up a blinking cursor at given coordinates.
	 * Erase cursor after key press.
	 * @param y Vertical coordinate.
	 * @param x Horizontal coordinate.
	 * @return Character from keyboard.
	 */
	unsigned int getChar(int y, int x, bool active) {
		if (!active) {
			move(0, 0);
#ifndef DEBUG
			curs_set(0);
#endif
			return getch();
		}
		curs_set(2);
		mvaddch(y + m_yOrig, x + m_xOrig, ' '); 
		move(y + m_yOrig, x + m_xOrig);
		int ch = getch();
#ifndef DEBUG
		curs_set(0);
#endif
		mvaddch(y + m_yOrig, x + m_xOrig, '?'); 
		return ch;
	}

	/**
	 * Static fucntion that return singleton CursesGraphics object attached to screen
	 * @return CursesGraphics singleton.
	 */
	static CursesGraphics& getInstance() {
		static CursesGraphics draw;
		
		return draw;
	}

private:
	static bool init;         ///< Singleton has been initialized.
	bool m_has_colors;        ///< If true, flag is screen has colors.
	UI::Event m_keyEvent = 0; ///< Storage for key event.
	int m_xMouse;             ///< Last mouse horizontal coordinate
	int m_yMouse;             ///< Last mouse vertical coordinate

	/** Map Graphics attributs to ncurses attributes.
	 */
	static const unordered_map<Attributes, int> attribute_map;

	/** 
	 * Special characters that are mapped to special unicode characters.
	 * Upper case characters mapped to greek letters.
	 */
	static const unordered_map<char, string> char_map;

	/** Map ncurses key and mouse code to UI:Event equivalents.
	 */
	static const unordered_map<int, UI::Event> event_map;

	/**
	 * Draw a character at x,y.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param c  Character to be drawn at x0,y0.
	 */
	void at(int x, int y, int c) {
		if (m_select.inside(x, y)) c |= A_REVERSE;
		mvaddch(y + m_yOrig, x + m_xOrig, c);
	}
};

bool CursesGraphics::init = false;

const unordered_map<Graphics::Attributes, int> CursesGraphics::attribute_map = {
	{ Graphics::Attributes::NONE,   A_NORMAL },
	{ Graphics::Attributes::BOLD,   A_BOLD },
	{ Graphics::Attributes::ITALIC, A_ITALIC },
	{ Graphics::Attributes::BOLD_ITALIC, A_ITALIC|A_BOLD }
};

const unordered_map<char, string> CursesGraphics::char_map = {
	{ 'A', "\u03b1" }, { 'B', "\u03b2" }, { 'C', "\u03c8" }, { 'D', "\u03b4" }, 
	{ 'E', "\u03b5" }, { 'F', "\u03c6" }, { 'G', "\u03b3" }, { 'H', "\u03b7" }, 
	{ 'I', "\u03b9" }, { 'J', "\u03be" }, { 'K', "\u03ba" }, { 'L', "\u03bb" }, 
	{ 'M', "\u03bc" }, { 'N', "\u03bd" }, { 'O', "\u03bf" }, { 'P', "\u03c0" }, 
	{ 'Q', "\u03d9" }, { 'R', "\u03c1" }, { 'S', "\u03c3" }, { 'T', "\u03c4" }, 
	{ 'U', "\u03b8" }, { 'V', "\u03c9" }, { 'W', "\u03c2" }, { 'X', "\u03c7" }, 
	{ 'Y', "\u03c5" }, { 'Z', "\u03b6" }
};

void CursesGraphics::parenthesis(int x_size, int y_size, int x0, int y0)
{
	if (y_size == 1) {
		at(x0, y0, '(');
		at(x0 + x_size - 1, y0, ')');
	}
	else {
		at(x0, y0, "\u239b");
		at(x0, y0 + y_size - 1, "\u239d");
		at(x0 + x_size - 1, y0, "\u239e");
		at(x0 + x_size - 1, y0 + y_size - 1, "\u23a0");
		for (int y = 1; y < y_size - 1; ++y) {
			at(x0, y + y0, "\u239c");
			at(x0 + x_size - 1, y + y0, "\u239f");
			
		}
	}
}

void CursesGraphics::differential(int x0, int y0, char variable)
{
	at(x0+1, y0, 'd');
	at(x0+1, y0+1, '-');
	at(x0, y0+2, 'd');
	at(x0+1, y0+2, variable);
}

const unordered_map<int, UI::Event> CursesGraphics::event_map = {
	{ KEY_F(1),   UI::Event(UI::Keys::F1,  UI::Modifiers::NO_MOD) },
	{ KEY_F(2),   UI::Event(UI::Keys::F2,  UI::Modifiers::NO_MOD) },
	{ KEY_F(3),   UI::Event(UI::Keys::F3,  UI::Modifiers::NO_MOD) },
	{ KEY_F(4),   UI::Event(UI::Keys::F4,  UI::Modifiers::NO_MOD) },
	{ KEY_F(5),   UI::Event(UI::Keys::F5,  UI::Modifiers::NO_MOD) },
	{ KEY_F(6),   UI::Event(UI::Keys::F6,  UI::Modifiers::NO_MOD) },
	{ KEY_F(7),   UI::Event(UI::Keys::F7,  UI::Modifiers::NO_MOD) },
	{ KEY_F(8),   UI::Event(UI::Keys::F8,  UI::Modifiers::NO_MOD) },
	{ KEY_F(9),   UI::Event(UI::Keys::F9,  UI::Modifiers::NO_MOD) },
	{ KEY_F(10),  UI::Event(UI::Keys::F10, UI::Modifiers::NO_MOD) },
	{ KEY_F(11),  UI::Event(UI::Keys::F11, UI::Modifiers::NO_MOD) },
	{ KEY_F(12),  UI::Event(UI::Keys::F12, UI::Modifiers::NO_MOD) },
	{ KEY_F(13),  UI::Event(UI::Keys::F1,  UI::Modifiers::SHIFT) },
	{ KEY_F(14),  UI::Event(UI::Keys::F2,  UI::Modifiers::SHIFT) },
	{ KEY_F(15),  UI::Event(UI::Keys::F3,  UI::Modifiers::SHIFT) },
	{ KEY_F(16),  UI::Event(UI::Keys::F4,  UI::Modifiers::SHIFT) },
	{ KEY_F(17),  UI::Event(UI::Keys::F5,  UI::Modifiers::SHIFT) },
	{ KEY_F(18),  UI::Event(UI::Keys::F6,  UI::Modifiers::SHIFT) },
	{ KEY_F(19),  UI::Event(UI::Keys::F7,  UI::Modifiers::SHIFT) },
	{ KEY_F(20),  UI::Event(UI::Keys::F8,  UI::Modifiers::SHIFT) },
	{ KEY_F(21),  UI::Event(UI::Keys::F9,  UI::Modifiers::SHIFT) },
	{ KEY_F(22),  UI::Event(UI::Keys::F10, UI::Modifiers::SHIFT) },
	{ KEY_F(23),  UI::Event(UI::Keys::F11, UI::Modifiers::SHIFT) },
	{ KEY_F(24),  UI::Event(UI::Keys::F12, UI::Modifiers::SHIFT) },
	{ KEY_F(25),  UI::Event(UI::Keys::F1,  UI::Modifiers::CTRL) },
	{ KEY_F(26),  UI::Event(UI::Keys::F2,  UI::Modifiers::CTRL) },
	{ KEY_F(27),  UI::Event(UI::Keys::F3,  UI::Modifiers::CTRL) },
	{ KEY_F(28),  UI::Event(UI::Keys::F4,  UI::Modifiers::CTRL) },
	{ KEY_F(29),  UI::Event(UI::Keys::F5,  UI::Modifiers::CTRL) },
	{ KEY_F(30),  UI::Event(UI::Keys::F6,  UI::Modifiers::CTRL) },
	{ KEY_F(31),  UI::Event(UI::Keys::F7,  UI::Modifiers::CTRL) },
	{ KEY_F(32),  UI::Event(UI::Keys::F8,  UI::Modifiers::CTRL) },
	{ KEY_F(33),  UI::Event(UI::Keys::F9,  UI::Modifiers::CTRL) },
	{ KEY_F(34),  UI::Event(UI::Keys::F10, UI::Modifiers::CTRL) },
	{ KEY_F(35),  UI::Event(UI::Keys::F11, UI::Modifiers::CTRL) },
	{ KEY_F(36),  UI::Event(UI::Keys::F12, UI::Modifiers::CTRL) },
	{ KEY_F(37),  UI::Event(UI::Keys::F1,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(38),  UI::Event(UI::Keys::F2,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(39),  UI::Event(UI::Keys::F3,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(40),  UI::Event(UI::Keys::F4,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(41),  UI::Event(UI::Keys::F5,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(42),  UI::Event(UI::Keys::F6,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(43),  UI::Event(UI::Keys::F7,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(44),  UI::Event(UI::Keys::F8,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(45),  UI::Event(UI::Keys::F9,  UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(46),  UI::Event(UI::Keys::F10, UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(47),  UI::Event(UI::Keys::F11, UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(48),  UI::Event(UI::Keys::F12, UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(49),  UI::Event(UI::Keys::F1,  UI::Modifiers::ALT) },
	{ KEY_F(50),  UI::Event(UI::Keys::F2,  UI::Modifiers::ALT) },
	{ KEY_F(51),  UI::Event(UI::Keys::F3,  UI::Modifiers::ALT) },
	{ KEY_F(52),  UI::Event(UI::Keys::F4,  UI::Modifiers::ALT) },
	{ KEY_F(53),  UI::Event(UI::Keys::F5,  UI::Modifiers::ALT) },
	{ KEY_F(54),  UI::Event(UI::Keys::F6,  UI::Modifiers::ALT) },
	{ KEY_F(55),  UI::Event(UI::Keys::F7,  UI::Modifiers::ALT) },
	{ KEY_F(56),  UI::Event(UI::Keys::F8,  UI::Modifiers::ALT) },
	{ KEY_F(57),  UI::Event(UI::Keys::F9,  UI::Modifiers::ALT) },
	{ KEY_F(58),  UI::Event(UI::Keys::F10, UI::Modifiers::ALT) },
	{ KEY_F(59),  UI::Event(UI::Keys::F11, UI::Modifiers::ALT) },
	{ KEY_F(60),  UI::Event(UI::Keys::F12, UI::Modifiers::ALT) },
	{ KEY_BTAB,   UI::Event(UI::Keys::TAB, UI::Modifiers::SHIFT) },
	{ KEY_IC,     UI::Event(UI::Keys::INS,  UI::Modifiers::NO_MOD) },
	{ 01032,      UI::Event(UI::Keys::INS,  UI::Modifiers::ALT) },
	{ KEY_DC,     UI::Event(UI::Keys::DEL,  UI::Modifiers::NO_MOD) },
	{ 01005,      UI::Event(UI::Keys::DEL,  UI::Modifiers::ALT) },
	{ KEY_SDC,    UI::Event(UI::Keys::DEL,  UI::Modifiers::SHIFT) },
	{ 01007,      UI::Event(UI::Keys::DEL,  UI::Modifiers::CTRL)  },
	{ 01010,      UI::Event(UI::Keys::DEL,  UI::Modifiers::CTRL_SHIFT)  },
	{ KEY_HOME,   UI::Event(UI::Keys::HOME, UI::Modifiers::NO_MOD) },
	{ 01025,      UI::Event(UI::Keys::HOME, UI::Modifiers::ALT) },
	{ KEY_SHOME,  UI::Event(UI::Keys::HOME, UI::Modifiers::SHIFT) },
	{ 01027,      UI::Event(UI::Keys::HOME, UI::Modifiers::CTRL)  },
	{ 01026,      UI::Event(UI::Keys::HOME, UI::Modifiers::ALT_SHIFT) },
	{ 01030,      UI::Event(UI::Keys::HOME, UI::Modifiers::CTRL_SHIFT)  },
	{ KEY_END,    UI::Event(UI::Keys::END,  UI::Modifiers::NO_MOD) },
	{ 01020,      UI::Event(UI::Keys::END,  UI::Modifiers::ALT) },
	{ KEY_SEND,   UI::Event(UI::Keys::END,  UI::Modifiers::SHIFT) },
	{ 01022,      UI::Event(UI::Keys::END,  UI::Modifiers::CTRL)  },
	{ 01021,      UI::Event(UI::Keys::END,  UI::Modifiers::ALT_SHIFT) },
	{ 01023,      UI::Event(UI::Keys::END,  UI::Modifiers::CTRL_SHIFT)  },
	{ KEY_PPAGE,  UI::Event(UI::Keys::PAGE_UP, UI::Modifiers::NO_MOD) },
	{ KEY_NPAGE,  UI::Event(UI::Keys::PAGE_DOWN, UI::Modifiers::NO_MOD) },
	{ KEY_SNEXT,  UI::Event(UI::Keys::PAGE_DOWN, UI::Modifiers::SHIFT) },
	{ KEY_LEFT,   UI::Event(UI::Keys::LEFT, UI::Modifiers::NO_MOD) },
	{ KEY_SLEFT,  UI::Event(UI::Keys::LEFT, UI::Modifiers::SHIFT) },
	{ KEY_RIGHT,  UI::Event(UI::Keys::RIGHT, UI::Modifiers::NO_MOD) },
	{ KEY_SRIGHT, UI::Event(UI::Keys::RIGHT, UI::Modifiers::SHIFT) },
	{ KEY_UP,     UI::Event(UI::Keys::UP, UI::Modifiers::NO_MOD) },
	{ KEY_SR,     UI::Event(UI::Keys::UP, UI::Modifiers::SHIFT) },
	{ KEY_DOWN,   UI::Event(UI::Keys::DOWN, UI::Modifiers::NO_MOD) },
	{ KEY_SF,     UI::Event(UI::Keys::DOWN, UI::Modifiers::SHIFT) },
	{ KEY_SPREVIOUS, UI::Event(UI::Keys::PAGE_UP, UI::Modifiers::SHIFT) },
	{ KEY_BACKSPACE, UI::Event(UI::Keys::BSPACE, UI::Modifiers::NO_MOD) },
	{ 0x10000001, UI::Event(UI::Mouse::RELEASED, 1, UI::Modifiers::NO_MOD) },
	{ 0x12000001, UI::Event(UI::Mouse::RELEASED, 1, UI::Modifiers::SHIFT) },
	{ 0x10000002, UI::Event(UI::Mouse::PRESSED,  1, UI::Modifiers::NO_MOD) },
	{ 0x12000002, UI::Event(UI::Mouse::PRESSED,  1, UI::Modifiers::SHIFT) },
	{ 0x10000004, UI::Event(UI::Mouse::CLICKED,  1, UI::Modifiers::NO_MOD) },
	{ 0x12000004, UI::Event(UI::Mouse::CLICKED,  1, UI::Modifiers::SHIFT) },
	{ 0x10000008, UI::Event(UI::Mouse::DOUBLE,   1, UI::Modifiers::NO_MOD) },
	{ 0x12000008, UI::Event(UI::Mouse::DOUBLE,   1, UI::Modifiers::SHIFT) },
	{ 0x18000000, UI::Event(UI::Mouse::POSITION, 0, UI::Modifiers::NO_MOD) }
};

const UI::Event& CursesGraphics::getNextEvent(int xCursor, int yCursor, bool fBlink)
{
	constexpr int MOUSE_EVENT_MASK = 0x10000000;

	int code = 0;
	while (event_map.find(code) == event_map.end()) {
		MEVENT event;
		code = getChar(yCursor, xCursor - 1, fBlink);
		
		if (code < 0x80) {
			m_keyEvent = UI::Event((char)code);
			return m_keyEvent;
		}
		else if (code == KEY_MOUSE && getmouse(&event) == OK) {
			code = event.bstate|MOUSE_EVENT_MASK;
			break;
		}
		else
			code = 0;
	}

	return event_map.at(code);
}

/**
 * Main routine for milo ncurses command line app.
 * @param argc Number of arguments. Only first is used for equation xml file.
 * @param argv Array of argugments.
 * @return Exit code.
 */
int main(int argc, char* argv[])
{
	LOG_TRACE_CLEAR();
	LOG_TRACE_MSG("Starting milo_ncurses...");

	CursesGraphics gc = CursesGraphics();
	UI::doMainLoop(argc, argv, gc);
	return 0;
}
