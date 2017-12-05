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
#include <memory>
#include "ui.h"

using namespace std;
using namespace UI;

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

class CursesContext : public GlobalContext
{
public:
	/** @name Constructors and Destructor */
	//@{
	CursesContext(Graphics* gc) : GlobalContext(gc) {}

	/**
	 * Constructor with input equation.
	 */
	CursesContext(Graphics* gc, std::istream& is) : GlobalContext(gc, is) {}

	~CursesContext() {}
	//@}
	
	/** @name Overridden Public Member Functions */
	//@{
	/**
	 * Start of menu with attributes
	 * @param attributes All menu settings in map.
	 */
	void define_menu(const StringMap& attributes);
	
	/**
	 * No more menu items for current menu
	 * @param name Name of menu coming to end
	 */
	void define_menu_end(const std::string& name);
	
	/**
	 * Menu item for current menu
	 * @param attributes All menu item settings in map.
	 */
	void define_menu_item(const StringMap& attributes);
	
	/**
	 * Menu line for current menu
	 */
	void define_menu_line();
	//@}
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

/**
 * Map ncurses key code to a key event.
 */
static const unordered_map<int, KeyEvent> key_map = {
	{ KEY_F(1),      KeyEvent(Keys::F1,        Modifiers::NO_MOD) },
	{ KEY_F(2),      KeyEvent(Keys::F2,        Modifiers::NO_MOD) },
	{ KEY_F(3),      KeyEvent(Keys::F3,        Modifiers::NO_MOD) },
	{ KEY_F(4),      KeyEvent(Keys::F4,        Modifiers::NO_MOD) },
	{ KEY_F(5),      KeyEvent(Keys::F5,        Modifiers::NO_MOD) },
	{ KEY_F(6),      KeyEvent(Keys::F6,        Modifiers::NO_MOD) },
	{ KEY_F(7),      KeyEvent(Keys::F7,        Modifiers::NO_MOD) },
	{ KEY_F(8),      KeyEvent(Keys::F8,        Modifiers::NO_MOD) },
	{ KEY_F(9),      KeyEvent(Keys::F9,        Modifiers::NO_MOD) },
	{ KEY_F(10),     KeyEvent(Keys::F10,       Modifiers::NO_MOD) },
	{ KEY_F(11),     KeyEvent(Keys::F11,       Modifiers::NO_MOD) },
	{ KEY_F(12),     KeyEvent(Keys::F12,       Modifiers::NO_MOD) },
	{ KEY_F(13),     KeyEvent(Keys::F1,        Modifiers::SHIFT) },
	{ KEY_F(14),     KeyEvent(Keys::F2,        Modifiers::SHIFT) },
	{ KEY_F(15),     KeyEvent(Keys::F3,        Modifiers::SHIFT) },
	{ KEY_F(16),     KeyEvent(Keys::F4,        Modifiers::SHIFT) },
	{ KEY_F(17),     KeyEvent(Keys::F5,        Modifiers::SHIFT) },
	{ KEY_F(18),     KeyEvent(Keys::F6,        Modifiers::SHIFT) },
	{ KEY_F(19),     KeyEvent(Keys::F7,        Modifiers::SHIFT) },
	{ KEY_F(20),     KeyEvent(Keys::F8,        Modifiers::SHIFT) },
	{ KEY_F(21),     KeyEvent(Keys::F9,        Modifiers::SHIFT) },
	{ KEY_F(22),     KeyEvent(Keys::F10,       Modifiers::SHIFT) },
	{ KEY_F(23),     KeyEvent(Keys::F11,       Modifiers::SHIFT) },
	{ KEY_F(24),     KeyEvent(Keys::F12,       Modifiers::SHIFT) },
	{ KEY_F(25),     KeyEvent(Keys::F1,        Modifiers::CTRL) },
	{ KEY_F(26),     KeyEvent(Keys::F2,        Modifiers::CTRL) },
	{ KEY_F(27),     KeyEvent(Keys::F3,        Modifiers::CTRL) },
	{ KEY_F(28),     KeyEvent(Keys::F4,        Modifiers::CTRL) },
	{ KEY_F(29),     KeyEvent(Keys::F5,        Modifiers::CTRL) },
	{ KEY_F(30),     KeyEvent(Keys::F6,        Modifiers::CTRL) },
	{ KEY_F(31),     KeyEvent(Keys::F7,        Modifiers::CTRL) },
	{ KEY_F(32),     KeyEvent(Keys::F8,        Modifiers::CTRL) },
	{ KEY_F(33),     KeyEvent(Keys::F9,        Modifiers::CTRL) },
	{ KEY_F(34),     KeyEvent(Keys::F10,       Modifiers::CTRL) },
	{ KEY_F(35),     KeyEvent(Keys::F11,       Modifiers::CTRL) },
	{ KEY_F(36),     KeyEvent(Keys::F12,       Modifiers::CTRL) },
	{ KEY_F(37),     KeyEvent(Keys::F1,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(38),     KeyEvent(Keys::F2,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(39),     KeyEvent(Keys::F3,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(40),     KeyEvent(Keys::F4,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(41),     KeyEvent(Keys::F5,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(42),     KeyEvent(Keys::F6,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(43),     KeyEvent(Keys::F7,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(44),     KeyEvent(Keys::F8,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(45),     KeyEvent(Keys::F9,        Modifiers::CTRL_SHIFT) },
	{ KEY_F(46),     KeyEvent(Keys::F10,       Modifiers::CTRL_SHIFT) },
	{ KEY_F(47),     KeyEvent(Keys::F11,       Modifiers::CTRL_SHIFT) },
	{ KEY_F(48),     KeyEvent(Keys::F12,       Modifiers::CTRL_SHIFT) },
	{ KEY_F(49),     KeyEvent(Keys::F1,        Modifiers::ALT) },
	{ KEY_F(50),     KeyEvent(Keys::F2,        Modifiers::ALT) },
	{ KEY_F(51),     KeyEvent(Keys::F3,        Modifiers::ALT) },
	{ KEY_F(52),     KeyEvent(Keys::F4,        Modifiers::ALT) },
	{ KEY_F(53),     KeyEvent(Keys::F5,        Modifiers::ALT) },
	{ KEY_F(54),     KeyEvent(Keys::F6,        Modifiers::ALT) },
	{ KEY_F(55),     KeyEvent(Keys::F7,        Modifiers::ALT) },
	{ KEY_F(56),     KeyEvent(Keys::F8,        Modifiers::ALT) },
	{ KEY_F(57),     KeyEvent(Keys::F9,        Modifiers::ALT) },
	{ KEY_F(58),     KeyEvent(Keys::F10,       Modifiers::ALT) },
	{ KEY_F(59),     KeyEvent(Keys::F11,       Modifiers::ALT) },
	{ KEY_F(60),     KeyEvent(Keys::F12,       Modifiers::ALT) },
	{ KEY_BTAB,      KeyEvent(Keys::TAB,       Modifiers::SHIFT) },
	{ KEY_IC,        KeyEvent(Keys::INS,       Modifiers::NO_MOD) },
	{ 01032,         KeyEvent(Keys::INS,       Modifiers::ALT) },
	{ KEY_DC,        KeyEvent(Keys::DEL,       Modifiers::NO_MOD) },
	{ 01005,         KeyEvent(Keys::DEL,       Modifiers::ALT) },
	{ KEY_SDC,       KeyEvent(Keys::DEL,       Modifiers::SHIFT) },
	{ 01007,         KeyEvent(Keys::DEL,       Modifiers::CTRL)  },
	{ 01010,         KeyEvent(Keys::DEL,       Modifiers::CTRL_SHIFT)  },
	{ KEY_HOME,      KeyEvent(Keys::HOME,      Modifiers::NO_MOD) },
	{ 01025,         KeyEvent(Keys::HOME,      Modifiers::ALT) },
	{ KEY_SHOME,     KeyEvent(Keys::HOME,      Modifiers::SHIFT) },
	{ 01027,         KeyEvent(Keys::HOME,      Modifiers::CTRL)  },
	{ 01026,         KeyEvent(Keys::HOME,      Modifiers::ALT_SHIFT) },
	{ 01030,         KeyEvent(Keys::HOME,      Modifiers::CTRL_SHIFT)  },
	{ KEY_END,       KeyEvent(Keys::END,       Modifiers::NO_MOD) },
	{ 01020,         KeyEvent(Keys::END,       Modifiers::ALT) },
	{ KEY_SEND,      KeyEvent(Keys::END,       Modifiers::SHIFT) },
	{ 01022,         KeyEvent(Keys::END,       Modifiers::CTRL)  },
	{ 01021,         KeyEvent(Keys::END,       Modifiers::ALT_SHIFT) },
	{ 01023,         KeyEvent(Keys::END,       Modifiers::CTRL_SHIFT)  },
	{ KEY_PPAGE,     KeyEvent(Keys::PAGE_UP,   Modifiers::NO_MOD) },
	{ KEY_NPAGE,     KeyEvent(Keys::PAGE_DOWN, Modifiers::NO_MOD) },
	{ KEY_SNEXT,     KeyEvent(Keys::PAGE_DOWN, Modifiers::SHIFT) },
	{ KEY_LEFT,      KeyEvent(Keys::LEFT,      Modifiers::NO_MOD) },
	{ KEY_SLEFT,     KeyEvent(Keys::LEFT,      Modifiers::SHIFT) },
	{ KEY_RIGHT,     KeyEvent(Keys::RIGHT,     Modifiers::NO_MOD) },
	{ KEY_SRIGHT,    KeyEvent(Keys::RIGHT,     Modifiers::SHIFT) },
	{ KEY_UP,        KeyEvent(Keys::UP,        Modifiers::NO_MOD) },
	{ KEY_SR,        KeyEvent(Keys::UP,        Modifiers::SHIFT) },
	{ KEY_DOWN,      KeyEvent(Keys::DOWN,      Modifiers::NO_MOD) },
	{ KEY_SF,        KeyEvent(Keys::DOWN,      Modifiers::SHIFT) },
	{ KEY_SPREVIOUS, KeyEvent(Keys::PAGE_UP,   Modifiers::SHIFT) },
	{ KEY_BACKSPACE, KeyEvent(Keys::BSPACE,    Modifiers::NO_MOD) }
};

/**
 * Map ncurses key code (modulo mouse mask) to a mouse event.
 */
static const unordered_map<int, MouseEvent> mouse_event_map = {
	{ 0x10000001, MouseEvent(Mouse::RELEASED, 1, Modifiers::NO_MOD) },
	{ 0x12000001, MouseEvent(Mouse::RELEASED, 1, Modifiers::SHIFT) },
	{ 0x10000002, MouseEvent(Mouse::PRESSED,  1, Modifiers::NO_MOD) },
	{ 0x12000002, MouseEvent(Mouse::PRESSED,  1, Modifiers::SHIFT) },
	{ 0x10000004, MouseEvent(Mouse::CLICKED,  1, Modifiers::NO_MOD) },
	{ 0x12000004, MouseEvent(Mouse::CLICKED,  1, Modifiers::SHIFT) },
	{ 0x10000008, MouseEvent(Mouse::DOUBLE,   1, Modifiers::NO_MOD) },
	{ 0x12000008, MouseEvent(Mouse::DOUBLE,   1, Modifiers::SHIFT) },
	{ 0x18000000, MouseEvent(Mouse::POSITION, 0, Modifiers::NO_MOD) }
};

/**
 * Map ncurses keys to a name representing a function to call.
 */
static unordered_map<KeyEvent, string> key_menu_map;

static const unordered_map<string, enum Modifiers> stringToMod = {
	{ "ALT", ALT }, { "SHIFT", SHIFT }, { "ALT_SHIFT", ALT_SHIFT }, { "CTRL", CTRL },
	{ "CTRL_SHIFT", CTRL_SHIFT }, { "CTRL_ALT_SHIFT", CTRL_ALT_SHIFT }, { string(), NO_MOD }
};

static const unordered_map<string, enum Keys> stringToKey = {
	{ "F1", F1 }, { "F2", F2 }, { "F3", F3 }, { "F4", F4 }, { "F5", F5 }, { "F6", F6 }, 
	{ "F7", F7 }, { "F8", F8 }, { "F9", F9 }, { "F10", F10 }, { "F11", F11 }, { "F12", F12 },
	{ "INS", INS }, { "DEL", DEL }, { "\u2302", HOME }, { "END", END }, { "PAGE\u25b2", PAGE_UP },
	{ "PAGE\u25bc", PAGE_DOWN }, { "\u25b2", UP }, { "\u25bc", DOWN }, { "\u25c0", LEFT },
	{ "\u25b6", RIGHT }, { "BSP", BSPACE }, { "SP", SPACE }, { "TAB", TAB }, { "ENTER", ENTER}, { "ESC", ESC }
};

void CursesContext::define_menu(const StringMap& /*attributes*/)
{
}

void CursesContext::define_menu_end(const std::string& /*name*/)
{
}

void CursesContext::define_menu_line()
{
}

void CursesContext::define_menu_item(const StringMap& attributes)
{
	auto menu_key = attributes.find("key");
	if ( menu_key == attributes.end()) {
		throw logic_error("No key attribute in menu item");
	}
	auto menu_action = attributes.find("action");
	if ( menu_action == attributes.end()) {
		throw logic_error("No action attribute in menu item");
	}
	string key = menu_key->second;
	auto dash = key.find("-");
	string letter = key;
	enum Modifiers mod = NO_MOD;
	if (dash != string::npos) {
		auto mod_key = stringToMod.find(key.substr(0, dash));
		if (mod_key == stringToMod.end()) {
			throw logic_error("Unknown key modification in menu item:" + key);
		}
		letter = key.substr(dash+1);
		mod = mod_key->second;
	}
	if (letter.length() == 1) {
		key_menu_map.emplace(KeyEvent(letter[0], mod), menu_action->second);
	} else {
		auto key_iter = stringToKey.find(letter);
		if (key_iter == stringToKey.end()) {
			throw  logic_error("Unknown key letter in menu item" + letter);
		}
		key_menu_map.emplace(KeyEvent(key_iter->second, mod), menu_action->second);
	}
}

/**
 * Function to handle ncurses events
 */ 
void do_ncurses_loop()
{
	constexpr int MOUSE_EVENT_MASK = 0x10000000;
	MEVENT mouse_event;
	bool fChanged = false;

	while (isRunning()) {
		CursesGraphics& gcurses = dynamic_cast<CursesGraphics&>(GlobalContext::current->getGraphics());

		int xCursor = 0, yCursor = 0;
	    GlobalContext::current->getEqn().getCursorOrig(xCursor, yCursor);
		int code = gcurses.getChar(yCursor, xCursor - 1, GlobalContext::current->getEqn().blink());
		
		if (code == KEY_MOUSE && getmouse(&mouse_event) == OK) {
			code = mouse_event.bstate|MOUSE_EVENT_MASK;
			auto mouse_event_entry = mouse_event_map.find(code);

			if (mouse_event_entry == mouse_event_map.end())
				continue;

			MouseEvent mouseEvent = mouse_event_entry->second;
			mouseEvent.setCoords(mouse_event.x, mouse_event.y);
			fChanged = doMouse(mouseEvent);
				
			LOG_TRACE_MSG("mouse event: " + to_hexstring(code) + ", (x,y) = " +
						  to_string(mouse_event.x) + ", " + to_string(mouse_event.y));
		}
		else {
			if (code == KEY_RESIZE) {
				doMenu("refresh");
				continue;
			}
			string action;
			shared_ptr<KeyEvent> key;
			if (code < Keys::ESC) {
				key = make_shared<KeyEvent>(Keys::A + code - 1, Modifiers::CTRL);
			}
			else if (code < Keys::F1) {
				key = make_shared<KeyEvent>(code);
			} else {
				auto key_map_entry = key_map.find(code);
				if (key_map_entry != key_map.end()) {
					key = make_shared<KeyEvent>(key_map_entry->second);
				}
			}
			if (key) {
				auto key_menu_entry = key_menu_map.find(*key);
				if (key_menu_entry != key_menu_map.end()) {
					action = key_menu_entry->second;
				}
				if (action.length() > 0) {
					fChanged = doMenu(action);
				}
				else
					fChanged = doKey(*key);
			}
		}
		if (fChanged) {
			GlobalContext::current->saveEqn();
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
		GlobalContext::current = GlobalContextPtr(new CursesContext(new CursesGraphics(), in));
	}
	else {
		GlobalContext::current = GlobalContextPtr(new CursesContext(new CursesGraphics()));
    }
	GlobalContext::current->saveEqn();
	ifstream in_file("/usr/local/milo/menu.xml");
	GlobalContext::current->parse_menubar(in_file);
	do_ncurses_loop();
	return 0;
}
