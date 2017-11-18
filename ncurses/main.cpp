/* Copyright (C) 2017 - James Terman
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
 * @file ncurses/main.cpp
 * This file implements CursesGraphics that implments the ABC Graphics
 * and runs the event loop for ncurses.
 */

#include <ncursesw/ncurses.h>
#include <locale.h>
#include <unordered_map>
#include "ui.h"

using namespace std;

/**
 * Class derived from ABC Graphics to allow milo to draw to a ncurses ascii screen.
 */
class CursesGraphics : public UI::Graphics
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
	 * @return Height of differential in lines of text.
	 */
	int getDifferentialHeight(char) { return 3; }

	/**
	 * Get width of differential in lines of text.
	 * @return Width of differential in lines of text.
	 */
	int getDifferentialWidth(char)  { return 2; }

	/**
	 * Get vertical offset of differential.
	 * @return Vertical offset of differential in lines of text.
	 */
	int getDifferentialBase(char)   { return 1; }

	/**
	 * Draw a character at x,y with a color.
	 * @param x Horizontal origin of line.
	 * @param y Vertical origin of line.
	 * @param c Character to be drawn at x0,y0.
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
	 * @param x Horizontal origin of line.
	 * @param y Vertical origin of line.
	 * @param s String to be drawn at x0,y0.
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
	 * @param x Horizontal origin of line.
	 * @param y Vertical origin of line.
	 * @param s C-String to be drawn at x0,y0.
	 */
	void at(int x, int y, const char* s) {
		at(x, y, string(s), NONE);
	}

	/**
	 * ncurses handles refresh event.
	 */
	void refresh() {}

	/**
	 * Flush all characters to the ncurses screen.
	 */
	void out() { refresh(); }

	/**
	 * Set size of this graphics window (origin is always 0,0).
	 * @param x Horizontal size of graphics window.
	 * @param y Vertical size of graphics window.
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
	 * @param active True if cursor should blink
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
	static bool init;       ///< Singleton has been initialized.
	bool m_has_colors;      ///< If true, flag is screen has colors.
	int m_xMouse;           ///< Last mouse horizontal coordinate
	int m_yMouse;           ///< Last mouse vertical coordinate

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
	static const unordered_map<int, UI::KeyEvent> event_map;

	/**
	 * Draw a character at x,y.
	 * @param x Horizontal origin of line.
	 * @param y Vertical origin of line.
	 * @param c Character to be drawn at x0,y0.
	 */
	void at(int x, int y, int c) {
		if (m_select.inside(x, y)) c |= A_REVERSE;
		mvaddch(y + m_yOrig, x + m_xOrig, c);
	}
};

bool CursesGraphics::init = false;

const unordered_map<UI::Graphics::Attributes, int> CursesGraphics::attribute_map = {
	{ UI::Graphics::Attributes::NONE,   A_NORMAL },
	{ UI::Graphics::Attributes::BOLD,   A_BOLD },
	{ UI::Graphics::Attributes::ITALIC, A_ITALIC },
	{ UI::Graphics::Attributes::BOLD_ITALIC, A_ITALIC|A_BOLD }
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

/**
 * Map ncurses key code to a key event.
 */
const unordered_map<int, UI::KeyEvent> CursesGraphics::event_map = {
	{ KEY_F(1),      UI::KeyEvent(UI::Keys::F1,        UI::Modifiers::NO_MOD) },
	{ KEY_F(2),      UI::KeyEvent(UI::Keys::F2,        UI::Modifiers::NO_MOD) },
	{ KEY_F(3),      UI::KeyEvent(UI::Keys::F3,        UI::Modifiers::NO_MOD) },
	{ KEY_F(4),      UI::KeyEvent(UI::Keys::F4,        UI::Modifiers::NO_MOD) },
	{ KEY_F(5),      UI::KeyEvent(UI::Keys::F5,        UI::Modifiers::NO_MOD) },
	{ KEY_F(6),      UI::KeyEvent(UI::Keys::F6,        UI::Modifiers::NO_MOD) },
	{ KEY_F(7),      UI::KeyEvent(UI::Keys::F7,        UI::Modifiers::NO_MOD) },
	{ KEY_F(8),      UI::KeyEvent(UI::Keys::F8,        UI::Modifiers::NO_MOD) },
	{ KEY_F(9),      UI::KeyEvent(UI::Keys::F9,        UI::Modifiers::NO_MOD) },
	{ KEY_F(10),     UI::KeyEvent(UI::Keys::F10,       UI::Modifiers::NO_MOD) },
	{ KEY_F(11),     UI::KeyEvent(UI::Keys::F11,       UI::Modifiers::NO_MOD) },
	{ KEY_F(12),     UI::KeyEvent(UI::Keys::F12,       UI::Modifiers::NO_MOD) },
	{ KEY_F(13),     UI::KeyEvent(UI::Keys::F1,        UI::Modifiers::SHIFT) },
	{ KEY_F(14),     UI::KeyEvent(UI::Keys::F2,        UI::Modifiers::SHIFT) },
	{ KEY_F(15),     UI::KeyEvent(UI::Keys::F3,        UI::Modifiers::SHIFT) },
	{ KEY_F(16),     UI::KeyEvent(UI::Keys::F4,        UI::Modifiers::SHIFT) },
	{ KEY_F(17),     UI::KeyEvent(UI::Keys::F5,        UI::Modifiers::SHIFT) },
	{ KEY_F(18),     UI::KeyEvent(UI::Keys::F6,        UI::Modifiers::SHIFT) },
	{ KEY_F(19),     UI::KeyEvent(UI::Keys::F7,        UI::Modifiers::SHIFT) },
	{ KEY_F(20),     UI::KeyEvent(UI::Keys::F8,        UI::Modifiers::SHIFT) },
	{ KEY_F(21),     UI::KeyEvent(UI::Keys::F9,        UI::Modifiers::SHIFT) },
	{ KEY_F(22),     UI::KeyEvent(UI::Keys::F10,       UI::Modifiers::SHIFT) },
	{ KEY_F(23),     UI::KeyEvent(UI::Keys::F11,       UI::Modifiers::SHIFT) },
	{ KEY_F(24),     UI::KeyEvent(UI::Keys::F12,       UI::Modifiers::SHIFT) },
	{ KEY_F(25),     UI::KeyEvent(UI::Keys::F1,        UI::Modifiers::CTRL) },
	{ KEY_F(26),     UI::KeyEvent(UI::Keys::F2,        UI::Modifiers::CTRL) },
	{ KEY_F(27),     UI::KeyEvent(UI::Keys::F3,        UI::Modifiers::CTRL) },
	{ KEY_F(28),     UI::KeyEvent(UI::Keys::F4,        UI::Modifiers::CTRL) },
	{ KEY_F(29),     UI::KeyEvent(UI::Keys::F5,        UI::Modifiers::CTRL) },
	{ KEY_F(30),     UI::KeyEvent(UI::Keys::F6,        UI::Modifiers::CTRL) },
	{ KEY_F(31),     UI::KeyEvent(UI::Keys::F7,        UI::Modifiers::CTRL) },
	{ KEY_F(32),     UI::KeyEvent(UI::Keys::F8,        UI::Modifiers::CTRL) },
	{ KEY_F(33),     UI::KeyEvent(UI::Keys::F9,        UI::Modifiers::CTRL) },
	{ KEY_F(34),     UI::KeyEvent(UI::Keys::F10,       UI::Modifiers::CTRL) },
	{ KEY_F(35),     UI::KeyEvent(UI::Keys::F11,       UI::Modifiers::CTRL) },
	{ KEY_F(36),     UI::KeyEvent(UI::Keys::F12,       UI::Modifiers::CTRL) },
	{ KEY_F(37),     UI::KeyEvent(UI::Keys::F1,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(38),     UI::KeyEvent(UI::Keys::F2,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(39),     UI::KeyEvent(UI::Keys::F3,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(40),     UI::KeyEvent(UI::Keys::F4,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(41),     UI::KeyEvent(UI::Keys::F5,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(42),     UI::KeyEvent(UI::Keys::F6,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(43),     UI::KeyEvent(UI::Keys::F7,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(44),     UI::KeyEvent(UI::Keys::F8,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(45),     UI::KeyEvent(UI::Keys::F9,        UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(46),     UI::KeyEvent(UI::Keys::F10,       UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(47),     UI::KeyEvent(UI::Keys::F11,       UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(48),     UI::KeyEvent(UI::Keys::F12,       UI::Modifiers::CTRL_SHIFT) },
	{ KEY_F(49),     UI::KeyEvent(UI::Keys::F1,        UI::Modifiers::ALT) },
	{ KEY_F(50),     UI::KeyEvent(UI::Keys::F2,        UI::Modifiers::ALT) },
	{ KEY_F(51),     UI::KeyEvent(UI::Keys::F3,        UI::Modifiers::ALT) },
	{ KEY_F(52),     UI::KeyEvent(UI::Keys::F4,        UI::Modifiers::ALT) },
	{ KEY_F(53),     UI::KeyEvent(UI::Keys::F5,        UI::Modifiers::ALT) },
	{ KEY_F(54),     UI::KeyEvent(UI::Keys::F6,        UI::Modifiers::ALT) },
	{ KEY_F(55),     UI::KeyEvent(UI::Keys::F7,        UI::Modifiers::ALT) },
	{ KEY_F(56),     UI::KeyEvent(UI::Keys::F8,        UI::Modifiers::ALT) },
	{ KEY_F(57),     UI::KeyEvent(UI::Keys::F9,        UI::Modifiers::ALT) },
	{ KEY_F(58),     UI::KeyEvent(UI::Keys::F10,       UI::Modifiers::ALT) },
	{ KEY_F(59),     UI::KeyEvent(UI::Keys::F11,       UI::Modifiers::ALT) },
	{ KEY_F(60),     UI::KeyEvent(UI::Keys::F12,       UI::Modifiers::ALT) },
	{ KEY_BTAB,      UI::KeyEvent(UI::Keys::TAB,       UI::Modifiers::SHIFT) },
	{ KEY_IC,        UI::KeyEvent(UI::Keys::INS,       UI::Modifiers::NO_MOD) },
	{ 01032,         UI::KeyEvent(UI::Keys::INS,       UI::Modifiers::ALT) },
	{ KEY_DC,        UI::KeyEvent(UI::Keys::DEL,       UI::Modifiers::NO_MOD) },
	{ 01005,         UI::KeyEvent(UI::Keys::DEL,       UI::Modifiers::ALT) },
	{ KEY_SDC,       UI::KeyEvent(UI::Keys::DEL,       UI::Modifiers::SHIFT) },
	{ 01007,         UI::KeyEvent(UI::Keys::DEL,       UI::Modifiers::CTRL)  },
	{ 01010,         UI::KeyEvent(UI::Keys::DEL,       UI::Modifiers::CTRL_SHIFT)  },
	{ KEY_HOME,      UI::KeyEvent(UI::Keys::HOME,      UI::Modifiers::NO_MOD) },
	{ 01025,         UI::KeyEvent(UI::Keys::HOME,      UI::Modifiers::ALT) },
	{ KEY_SHOME,     UI::KeyEvent(UI::Keys::HOME,      UI::Modifiers::SHIFT) },
	{ 01027,         UI::KeyEvent(UI::Keys::HOME,      UI::Modifiers::CTRL)  },
	{ 01026,         UI::KeyEvent(UI::Keys::HOME,      UI::Modifiers::ALT_SHIFT) },
	{ 01030,         UI::KeyEvent(UI::Keys::HOME,      UI::Modifiers::CTRL_SHIFT)  },
	{ KEY_END,       UI::KeyEvent(UI::Keys::END,       UI::Modifiers::NO_MOD) },
	{ 01020,         UI::KeyEvent(UI::Keys::END,       UI::Modifiers::ALT) },
	{ KEY_SEND,      UI::KeyEvent(UI::Keys::END,       UI::Modifiers::SHIFT) },
	{ 01022,         UI::KeyEvent(UI::Keys::END,       UI::Modifiers::CTRL)  },
	{ 01021,         UI::KeyEvent(UI::Keys::END,       UI::Modifiers::ALT_SHIFT) },
	{ 01023,         UI::KeyEvent(UI::Keys::END,       UI::Modifiers::CTRL_SHIFT)  },
	{ KEY_PPAGE,     UI::KeyEvent(UI::Keys::PAGE_UP,   UI::Modifiers::NO_MOD) },
	{ KEY_NPAGE,     UI::KeyEvent(UI::Keys::PAGE_DOWN, UI::Modifiers::NO_MOD) },
	{ KEY_SNEXT,     UI::KeyEvent(UI::Keys::PAGE_DOWN, UI::Modifiers::SHIFT) },
	{ KEY_LEFT,      UI::KeyEvent(UI::Keys::LEFT,      UI::Modifiers::NO_MOD) },
	{ KEY_SLEFT,     UI::KeyEvent(UI::Keys::LEFT,      UI::Modifiers::SHIFT) },
	{ KEY_RIGHT,     UI::KeyEvent(UI::Keys::RIGHT,     UI::Modifiers::NO_MOD) },
	{ KEY_SRIGHT,    UI::KeyEvent(UI::Keys::RIGHT,     UI::Modifiers::SHIFT) },
	{ KEY_UP,        UI::KeyEvent(UI::Keys::UP,        UI::Modifiers::NO_MOD) },
	{ KEY_SR,        UI::KeyEvent(UI::Keys::UP,        UI::Modifiers::SHIFT) },
	{ KEY_DOWN,      UI::KeyEvent(UI::Keys::DOWN,      UI::Modifiers::NO_MOD) },
	{ KEY_SF,        UI::KeyEvent(UI::Keys::DOWN,      UI::Modifiers::SHIFT) },
	{ KEY_SPREVIOUS, UI::KeyEvent(UI::Keys::PAGE_UP,   UI::Modifiers::SHIFT) },
	{ KEY_BACKSPACE, UI::KeyEvent(UI::Keys::BSPACE,    UI::Modifiers::NO_MOD) }
};

/**
 * Map ncurses key code (modulo mouse mask) to a mouse event.
 */
static const unordered_map<int, UI::MouseEvent> mouse_event_map = {
	{ 0x10000001, UI::MouseEvent(UI::Mouse::RELEASED, 1, UI::Modifiers::NO_MOD) },
	{ 0x12000001, UI::MouseEvent(UI::Mouse::RELEASED, 1, UI::Modifiers::SHIFT) },
	{ 0x10000002, UI::MouseEvent(UI::Mouse::PRESSED,  1, UI::Modifiers::NO_MOD) },
	{ 0x12000002, UI::MouseEvent(UI::Mouse::PRESSED,  1, UI::Modifiers::SHIFT) },
	{ 0x10000004, UI::MouseEvent(UI::Mouse::CLICKED,  1, UI::Modifiers::NO_MOD) },
	{ 0x12000004, UI::MouseEvent(UI::Mouse::CLICKED,  1, UI::Modifiers::SHIFT) },
	{ 0x10000008, UI::MouseEvent(UI::Mouse::DOUBLE,   1, UI::Modifiers::NO_MOD) },
	{ 0x12000008, UI::MouseEvent(UI::Mouse::DOUBLE,   1, UI::Modifiers::SHIFT) },
	{ 0x18000000, UI::MouseEvent(UI::Mouse::POSITION, 0, UI::Modifiers::NO_MOD) }
};

/**
 * Map ncurses keys to a name representing a function to call.
 */
static unordered_map<int, string> key_menu_map = {
	{ UI::Keys::CTRL_Q, "quit" },
	{ UI::Keys::CTRL_S, "save" },
	{ UI::Keys::CTRL_Z, "undo" },
	{ KEY_RESIZE,       "refresh" }
};

/**
 * Function to handle ncurses events
 */ 
void do_ncurses_loop()
{
	constexpr int MOUSE_EVENT_MASK = 0x10000000;
	MEVENT mouse_event;
	bool fChanged = false;

	while (UI::isRunning()) {
		CursesGraphics& gcurses = dynamic_cast<CursesGraphics&>(UI::GlobalContext::current->getGraphics());

		int xCursor = 0, yCursor = 0;
	    UI::GlobalContext::current->getEqn().getCursorOrig(xCursor, yCursor);
		int code = gcurses.getChar(yCursor, xCursor - 1, UI::GlobalContext::current->getEqn().blink());
		
		if (code < 0x80) {
			auto key_menu_entry = key_menu_map.find(code);
			if (key_menu_entry != key_menu_map.end()) {
				fChanged = UI::doMenu(key_menu_entry->second);
			}
			else
				fChanged = UI::doKey(UI::KeyEvent((char) code));
		}
		else if (code == KEY_MOUSE && getmouse(&mouse_event) == OK) {
			code = mouse_event.bstate|MOUSE_EVENT_MASK;
			auto mouse_event_entry = mouse_event_map.find(code);

			if (mouse_event_entry == mouse_event_map.end())
				continue;

			UI::MouseEvent mouseEvent = mouse_event_entry->second;
			mouseEvent.setCoords(mouse_event.x, mouse_event.y);
			fChanged = doMouse(mouseEvent);
				
			LOG_TRACE_MSG("mouse event: " + to_hexstring(code) + ", (x,y) = " +
						  to_string(mouse_event.x) + ", " + to_string(mouse_event.y));
		}
		if (fChanged) {
			UI::GlobalContext::current->saveEqn();
			fChanged = false;
		}
	}
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

	if (argc > 1) {
		ifstream in(argv[1]);
		UI::GlobalContext::current = make_shared<UI::GlobalContext>(new CursesGraphics(), in);
	}
	else
		UI::GlobalContext::current = make_shared<UI::GlobalContext>(new CursesGraphics());

	UI::GlobalContext::current->saveEqn();
	do_ncurses_loop();
	return 0;
}
