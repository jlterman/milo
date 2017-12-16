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
#include <utf8.h>
#include "ui.h"

using namespace std;
using namespace UI;

/** Simple class to hold ncurses window
 */
class NcursesWindow
{
public:
	/** @name Constructors and Destructor */
	//@{

	/**
	 * NcursesWindow constructor
	 */
	NcursesWindow(int height, int width, int starty, int startx) :
		m_win(newwin(height, width, starty, startx)),
		m_starty(starty),
		m_startx(startx)
	{
		box(m_win, 0 , 0);
		wrefresh(m_win);
	}

	/**
	 * NcursesWindow deconstructor. Delete window.
	 */
	~NcursesWindow() { delwin(m_win); }
	//@}

	/**
	 * Get local coords
	 * @param[in|out] global y -> local y
	 * @param[in|out] global x -> local x
	 */
	void get_local(int& y, int& x) { y -= m_starty; x -= m_startx; }

	/**
	 * Get global coords
	 * @param[in|out] local y -> global y
	 * @param[in|out] local x -> global x
	 */
	void get_global(int& y, int& x) { y += m_starty; x += m_startx; }
	
	/**
	 * Get window pointer
	 * @return Window pointer
	 */
	operator WINDOW*() const { return m_win; }
private:
	WINDOW* m_win; ///< ncurses window pointer
	int m_starty;  ///< y origin
	int m_startx;  ///< x origin
};

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

/**
 * Abstract base menu class that all menu classes inherit from.
 */
class MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	MenuBaseItem(const StringMap& tags) :
		m_x0(-1), m_y0(-1), m_w(-1),
		m_name(tags.at("name")),
		m_type(tags.at("type")),
		m_width(-1) {}
	virtual ~MenuBaseItem() {}
	//@}

	/** 
	 * Get name of the menu item
	 * @return Menu item name
	 */
	const string& getName() { return m_name; }

	/** 
	 * Get type of the menu item
	 * @return Menu item type
	 */
	const string& getType() { return m_type; }

	/** 
	 * Get width of menu item
	 * @return Menu item width
	 */
	int getWidth() { if (m_width == -1) m_width = calc_width(); return m_width; }	

	
	/** @name Pure Virtual Public Member Functions */
	//@{
	/** 
	 * Draw menu on window at given coordinates
	 */
	virtual void draw(WINDOW* win, bool fHighlight, int y, int x)=0;

	/**
	 * Select this menu item
	 */
	virtual void select()=0;
	
	/**
	 * Get title of menu
	 * @return Return title
	 */
	virtual const string& getTitle()=0;

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	virtual bool getActive()=0;
	//@}

protected:
	int m_x0; ///< Horiz coord of where menu item is drawn
	int m_y0; ///< Vertical coord of where menu item is drawn
	int m_w;  ///< Width of where menu item is drawn

private:
	/** Private pure virtual member function to calc width of menu
	 */
	virtual int calc_width()=0;

	const string m_name; ///< menu name
	const string m_type; ///< menu type
	int m_width;         ///< menu width
};

using MenuBasePtr = shared_ptr<MenuBaseItem>; ///< Shared menu pointer for all menu classes

/**
 * Submenu class also for top menu.
 */
class Menu : public MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	Menu(const StringMap& tags) :
		MenuBaseItem(tags),
		m_active(tags.at("active") == "true"),
		m_title(tags.at("title")) {}
	~Menu() {}
	//@}

	/** @name Overriden Pure Virtual Member Functions */
	//@{
	/** 
	 * Draw menu on window at given coordinates
	 */
	void draw(WINDOW* win, bool fHighlight, int y, int x);

	/**
	 * Select this menu item
	 */
	void select() {}

	/**
	 * Get title of menu
	 * @return Return title
	 */
	const string& getTitle() { return m_title; }

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	bool getActive() { return m_active; }

	/** Calc width of menu
	 */
	int calc_width();
	//@}

	/** 
	 * Add menu item to menu
	 * @param m Menu item to add menu
	 */
	void addItem(MenuBaseItem* m) { m_items.push_back(MenuBasePtr(m)); }

	/**
	 * Handle mouse event in menu bar
	 * @param Mouse event in menu bar
	 */
	void selectMenuBar(MEVENT& /*mouse_event*/) {}

private:
	bool m_active;               ///< True if active
	const string m_title;        ///< Title of menu
	vector<MenuBasePtr> m_items; ///< List of menu items
};

using MenuPtr = shared_ptr<Menu>; ///< shared pointer for class Menu

/**
 * Menu item class
 */
class MenuItem : public MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	MenuItem(const StringMap& tags) :
		MenuBaseItem(tags),
		m_active(tags.at("active") == "true"),
		m_title(tags.at("title")),
		m_action(tags.at("action")),
		m_key(tags.at("key")) {}
	~MenuItem() {}
	//@}

	/** @name Overriden Pure Virtual Member Functions */
	//@{
	/** 
	 * Draw menu on window at given coordinates
	 */
	void draw(WINDOW* win, bool fHighlight, int y, int x);

	/**
	 * Select this menu item
	 */
	void select() { UI::doMenu(m_action); }

	/**
	 * Get title of menu
	 * @return Return title
	 */
	const string& getTitle() { return m_title; }

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	bool getActive() { return m_active; }

	/** Calc width of menu
	 */
	int calc_width();
	//@}

private:
	bool m_active;         ///< True if active
	const string m_title;  ///< Title of menu item
	const string m_action; ///< name of action of menu item
	const string m_key;    ///< Key binding of menu item
};

/**
 * Menu line item class
 */
class MenuLine : public MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	MenuLine() : MenuBaseItem({ {"type", "line"}, {"name", "line"} }) {}
	~MenuLine() {}
	//@}

	/** @name Overriden Pure Virtual Member Functions */
	//@{
	/** 
	 * Draw menu on window at given coordinates
	 */
	void draw(WINDOW* win, bool fHighlight, int y, int x);

	/**
	 * Select this menu item. Shouldn't be called.
	 */
	void select() {}

	/**
	 * Get title of menu
	 * @return Return title
	 */
	const string& getTitle() { return m_line_title; }

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	bool getActive() { return false; }

	/** Calc width of menu
	 */
	int calc_width() { return -1; }
	//@}
private:
	static const string m_line_title;
};

/**
 * GlobalContext for ncurses interface.
 */
class CursesContext : public GlobalContext
{
public:
	/** @name Constructors and Destructor */
	//@{
	CursesContext(Graphics* gc) : GlobalContext(gc), m_level(0) {}

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

	/**
	 * Initialize menu bar
	 */
	void initMenuBar();

	/** Draw m_current as menubar
	 */
	void drawMenuBar();

	/**
	 * Run main event loop
	 */
	void do_loop();

private:
	vector<MenuPtr> m_current;
	int m_level;
	stack<NcursesWindow> m_windows;
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
	{ 0,             KeyEvent(Keys::NO_KEY,    Modifiers::NO_MOD) },
	{ 1,             KeyEvent(Keys::CTRL_A,    Modifiers::NO_MOD) },
	{ 2,             KeyEvent(Keys::CTRL_B,    Modifiers::NO_MOD) },
	{ 3,             KeyEvent(Keys::CTRL_C,    Modifiers::NO_MOD) },
	{ 4,             KeyEvent(Keys::CTRL_D,    Modifiers::NO_MOD) },
	{ 5,             KeyEvent(Keys::CTRL_E,    Modifiers::NO_MOD) },
	{ 6,             KeyEvent(Keys::CTRL_F,    Modifiers::NO_MOD) },
	{ 7,             KeyEvent(Keys::CTRL_G,    Modifiers::NO_MOD) },
	{ 8,             KeyEvent(Keys::CTRL_H,    Modifiers::NO_MOD) },
	{ 9,             KeyEvent(Keys::TAB,       Modifiers::NO_MOD) },
	{ 10,            KeyEvent(Keys::ENTER,     Modifiers::NO_MOD) },
	{ 11,            KeyEvent(Keys::CTRL_K,    Modifiers::NO_MOD) },
	{ 12,            KeyEvent(Keys::CTRL_L,    Modifiers::NO_MOD) },
	{ 13,            KeyEvent(Keys::CTRL_M,    Modifiers::NO_MOD) },
	{ 14,            KeyEvent(Keys::CTRL_N,    Modifiers::NO_MOD) },
	{ 15,            KeyEvent(Keys::CTRL_O,    Modifiers::NO_MOD) },
	{ 16,            KeyEvent(Keys::CTRL_P,    Modifiers::NO_MOD) },
	{ 17,            KeyEvent(Keys::CTRL_Q,    Modifiers::NO_MOD) },
	{ 18,            KeyEvent(Keys::CTRL_R,    Modifiers::NO_MOD) },
	{ 19,            KeyEvent(Keys::CTRL_S,    Modifiers::NO_MOD) },
	{ 20,            KeyEvent(Keys::CTRL_T,    Modifiers::NO_MOD) },
	{ 21,            KeyEvent(Keys::CTRL_U,    Modifiers::NO_MOD) },
	{ 22,            KeyEvent(Keys::CTRL_V,    Modifiers::NO_MOD) },
	{ 23,            KeyEvent(Keys::CTRL_W,    Modifiers::NO_MOD) },
	{ 24,            KeyEvent(Keys::CTRL_X,    Modifiers::NO_MOD) },
	{ 25,            KeyEvent(Keys::CTRL_Y,    Modifiers::NO_MOD) },
	{ 26,            KeyEvent(Keys::CTRL_Z,    Modifiers::NO_MOD) },
	{ 27,            KeyEvent(Keys::ESC,       Modifiers::NO_MOD) },
	{ 32,            KeyEvent(Keys::SPACE,     Modifiers::NO_MOD) },
	{ 33,            KeyEvent(Keys::BANG,      Modifiers::NO_MOD) },
	{ 34,            KeyEvent(Keys::DBL_QUOTE, Modifiers::NO_MOD) },
	{ 35,            KeyEvent(Keys::HASH,      Modifiers::NO_MOD) },
	{ 36,            KeyEvent(Keys::DOLLAR,    Modifiers::NO_MOD) },
	{ 37,            KeyEvent(Keys::PERCENT,   Modifiers::NO_MOD) },
	{ 38,            KeyEvent(Keys::AMP,       Modifiers::NO_MOD) },
	{ 39,            KeyEvent(Keys::QUOTE,     Modifiers::NO_MOD) },
	{ 40,            KeyEvent(Keys::L_PAR,     Modifiers::NO_MOD) },
	{ 41,            KeyEvent(Keys::R_PAR,     Modifiers::NO_MOD) },
	{ 42,            KeyEvent(Keys::STAR,      Modifiers::NO_MOD) },
	{ 43,            KeyEvent(Keys::PLUS,      Modifiers::NO_MOD) },
	{ 44,            KeyEvent(Keys::COMMA,     Modifiers::NO_MOD) },
	{ 45,            KeyEvent(Keys::MINUS,     Modifiers::NO_MOD) },
	{ 46,            KeyEvent(Keys::DOT,       Modifiers::NO_MOD) },
	{ 47,            KeyEvent(Keys::DIVIDE,    Modifiers::NO_MOD) },
	{ 48,            KeyEvent(Keys::K0,        Modifiers::NO_MOD) },
	{ 49,            KeyEvent(Keys::K1,        Modifiers::NO_MOD) },
	{ 50,            KeyEvent(Keys::K2,        Modifiers::NO_MOD) },
	{ 51,            KeyEvent(Keys::K3,        Modifiers::NO_MOD) },
	{ 52,            KeyEvent(Keys::K4,        Modifiers::NO_MOD) },
	{ 53,            KeyEvent(Keys::K5,        Modifiers::NO_MOD) },
	{ 54,            KeyEvent(Keys::K6,        Modifiers::NO_MOD) },
	{ 55,            KeyEvent(Keys::K7,        Modifiers::NO_MOD) },
	{ 56,            KeyEvent(Keys::K8,        Modifiers::NO_MOD) },
	{ 57,            KeyEvent(Keys::K9,        Modifiers::NO_MOD) },
	{ 58,            KeyEvent(Keys::COLON,     Modifiers::NO_MOD) },
	{ 59,            KeyEvent(Keys::SEMI,      Modifiers::NO_MOD) },
	{ 60,            KeyEvent(Keys::LESS,      Modifiers::NO_MOD) },
	{ 61,            KeyEvent(Keys::EQUAL,     Modifiers::NO_MOD) },
	{ 62,            KeyEvent(Keys::GREATER,   Modifiers::NO_MOD) },
	{ 63,            KeyEvent(Keys::QUESTION,  Modifiers::NO_MOD) },
	{ 64,            KeyEvent(Keys::AT,        Modifiers::NO_MOD) },
	{ 65,            KeyEvent(Keys::A,         Modifiers::NO_MOD) },
	{ 66,            KeyEvent(Keys::B,         Modifiers::NO_MOD) },
	{ 67,            KeyEvent(Keys::C,         Modifiers::NO_MOD) },
	{ 68,            KeyEvent(Keys::D,         Modifiers::NO_MOD) },
	{ 60,            KeyEvent(Keys::E,         Modifiers::NO_MOD) },
	{ 70,            KeyEvent(Keys::F,         Modifiers::NO_MOD) },
	{ 71,            KeyEvent(Keys::G,         Modifiers::NO_MOD) },
	{ 72,            KeyEvent(Keys::H,         Modifiers::NO_MOD) },
	{ 73,            KeyEvent(Keys::I,         Modifiers::NO_MOD) },
	{ 74,            KeyEvent(Keys::J,         Modifiers::NO_MOD) },
	{ 75,            KeyEvent(Keys::K,         Modifiers::NO_MOD) },
	{ 76,            KeyEvent(Keys::L,         Modifiers::NO_MOD) },
	{ 77,            KeyEvent(Keys::M,         Modifiers::NO_MOD) },
	{ 78,            KeyEvent(Keys::N,         Modifiers::NO_MOD) },
	{ 79,            KeyEvent(Keys::O,         Modifiers::NO_MOD) },
	{ 80,            KeyEvent(Keys::P,         Modifiers::NO_MOD) },
	{ 81,            KeyEvent(Keys::Q,         Modifiers::NO_MOD) },
	{ 82,            KeyEvent(Keys::R,         Modifiers::NO_MOD) },
	{ 83,            KeyEvent(Keys::S,         Modifiers::NO_MOD) },
	{ 84,            KeyEvent(Keys::T,         Modifiers::NO_MOD) },
	{ 85,            KeyEvent(Keys::U,         Modifiers::NO_MOD) },
	{ 86,            KeyEvent(Keys::V,         Modifiers::NO_MOD) },
	{ 87,            KeyEvent(Keys::W,         Modifiers::NO_MOD) },
	{ 88,            KeyEvent(Keys::X,         Modifiers::NO_MOD) },
	{ 89,            KeyEvent(Keys::Y,         Modifiers::NO_MOD) },
	{ 90,            KeyEvent(Keys::Z,         Modifiers::NO_MOD) },
	{ 91,            KeyEvent(Keys::L_BRACKET, Modifiers::NO_MOD) },
	{ 92,            KeyEvent(Keys::B_SLASH,   Modifiers::NO_MOD) },
	{ 93,            KeyEvent(Keys::R_BRACKET, Modifiers::NO_MOD) },
	{ 94,            KeyEvent(Keys::POWER,     Modifiers::NO_MOD) },
	{ 95,            KeyEvent(Keys::U_SCORE,   Modifiers::NO_MOD) },
	{ 96,            KeyEvent(Keys::ACCENT,    Modifiers::NO_MOD) },
	{ 97,            KeyEvent(Keys::a,         Modifiers::NO_MOD) },
	{ 98,            KeyEvent(Keys::b,         Modifiers::NO_MOD) },
	{ 99,            KeyEvent(Keys::c,         Modifiers::NO_MOD) },
	{ 100,           KeyEvent(Keys::d,         Modifiers::NO_MOD) },
	{ 101,           KeyEvent(Keys::e,         Modifiers::NO_MOD) },
	{ 102,           KeyEvent(Keys::f,         Modifiers::NO_MOD) },
	{ 103,           KeyEvent(Keys::g,         Modifiers::NO_MOD) },
	{ 104,           KeyEvent(Keys::h,         Modifiers::NO_MOD) },
	{ 105,           KeyEvent(Keys::i,         Modifiers::NO_MOD) },
	{ 106,           KeyEvent(Keys::j,         Modifiers::NO_MOD) },
	{ 107,           KeyEvent(Keys::k,         Modifiers::NO_MOD) },
	{ 108,           KeyEvent(Keys::l,         Modifiers::NO_MOD) },
	{ 109,           KeyEvent(Keys::m,         Modifiers::NO_MOD) },
	{ 110,           KeyEvent(Keys::n,         Modifiers::NO_MOD) },
	{ 111,           KeyEvent(Keys::o,         Modifiers::NO_MOD) },
	{ 112,           KeyEvent(Keys::p,         Modifiers::NO_MOD) },
	{ 113,           KeyEvent(Keys::q,         Modifiers::NO_MOD) },
	{ 114,           KeyEvent(Keys::r,         Modifiers::NO_MOD) },
	{ 115,           KeyEvent(Keys::s,         Modifiers::NO_MOD) },
	{ 116,           KeyEvent(Keys::t,         Modifiers::NO_MOD) },
	{ 117,           KeyEvent(Keys::u,         Modifiers::NO_MOD) },
	{ 118,           KeyEvent(Keys::v,         Modifiers::NO_MOD) },
	{ 119,           KeyEvent(Keys::w,         Modifiers::NO_MOD) },
	{ 120,           KeyEvent(Keys::x,         Modifiers::NO_MOD) },
	{ 121,           KeyEvent(Keys::y,         Modifiers::NO_MOD) },
	{ 122,           KeyEvent(Keys::z,         Modifiers::NO_MOD) },
	{ 123,           KeyEvent(Keys::L_BRACE,   Modifiers::NO_MOD) },
	{ 124,           KeyEvent(Keys::PIPE,      Modifiers::NO_MOD) },
	{ 125,           KeyEvent(Keys::R_BRACE,   Modifiers::NO_MOD) },
	{ 126,           KeyEvent(Keys::TILDE,     Modifiers::NO_MOD) },
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

void CursesContext::define_menu(const StringMap& attributes)
{
	++m_level;
	m_current.emplace_back(new Menu(attributes));
}

void CursesContext::define_menu_end(const std::string& name)
{
	--m_level;
	if (m_current.back()->getName() != name) throw logic_error("Unexpected menu name: " + name);
	if (m_level > 1) m_current.pop_back();
}

void CursesContext::define_menu_line()
{
	m_current.back()->addItem(new MenuLine());
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
	KeyEvent key(menu_key->second);
	if (key) {
		key_menu_map.emplace(key, menu_action->second);	}
	m_current.back()->addItem(new MenuItem(attributes));
}

void CursesContext::initMenuBar()
{
	ifstream in_file("/usr/local/milo/data/menu/menu.xml");
	parse_menubar(in_file);
	m_current.back()->getWidth();
	drawMenuBar();
}

void Menu::draw(WINDOW* win, bool fHighlight, int y, int x)
{
	m_x0 = x;
	m_y0 = y;
	if (fHighlight) attron(A_REVERSE);
	int x_width = -1;
	getmaxyx(win, x, x_width);
	m_w = x_width;
	mvwaddstr(win, y, 1, m_title.c_str());
	x = utf8::distance(m_title.begin(), m_title.end()) + 1;
	while (x < x_width - 2) { mvwaddch(win, y, x++, ' '); }
	mvwaddstr(win, y, x, "\u25b6");
	if (fHighlight) attroff(A_REVERSE);
}

void MenuItem::draw(WINDOW* win, bool fHighlight, int y, int x)
{
	m_x0 = x;
	m_y0 = y;
	if (fHighlight) attron(A_REVERSE);
	int x_width = -1;
	getmaxyx(win, x, x_width);
	m_w = x_width;
	mvwaddstr(win, y, 1, m_title.c_str());
	x = utf8::distance(m_title.begin(), m_title.end()) + 1;
	int x1 = x_width - 1 - utf8::distance(m_key.begin(), m_key.end());
	while (x < x1) { mvwaddch(win, y, x++, ' '); }
	mvwaddstr(win, y, x, m_key.c_str());
	if (fHighlight) attroff(A_REVERSE);
}

void MenuLine::draw(WINDOW* win, bool, int y, int x)
{
	m_x0 = x;
	m_y0 = y;
	int x_width = -1;
	getmaxyx(win, x, x_width);
	m_w = x_width;
	for (x = 1; x < x_width - 1; ++x) mvaddch(y, x, ACS_HLINE);
}

void CursesContext::drawMenuBar()
{
	int x_width = -1;
	getmaxyx(stdscr, x_width, x_width);
	int x = -1;
	for ( auto m : m_current ) {
		mvaddch(0, x++, ' ');
		mvaddch(0, x++, ' ');
		mvaddstr(0, x, m->getTitle().c_str());
		x += utf8::distance(m->getTitle().begin(), m->getTitle().end());
	}
	while (x < x_width) { mvaddch(0, x++, ' '); }
}

int Menu::calc_width()
{
	return utf8::distance(m_title.begin(), m_title.end()) + 2;
}

int MenuItem::calc_width()
{
	return utf8::distance(m_title.begin(), m_title.end()) +
		   utf8::distance(m_key.begin(), m_key.end()) + 1;
}

const string MenuLine::m_line_title = "_line_";

void CursesContext::do_loop()
{
	constexpr int MOUSE_EVENT_MASK = 0x10000000;
	MEVENT mouse_event;
	bool fChanged = false;

	while (isRunning()) {
		CursesGraphics& gcurses = dynamic_cast<CursesGraphics&>(getGraphics());

		int xCursor = 0, yCursor = 0;
		getEqn().getCursorOrig(xCursor, yCursor);
		int code = gcurses.getChar(yCursor, xCursor - 1, getEqn().blink());
		
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

			auto key_map_entry = key_map.find(code);
			if (key_map_entry == key_map.end()) {
				continue;
			}
			
			auto key_menu_entry = key_menu_map.find(key_map_entry->second);
			if (key_menu_entry != key_menu_map.end()) {
				fChanged = doMenu(key_menu_entry->second);
			}
			else {
				fChanged = doKey(key_map_entry->second);
			}
		}
		if (fChanged) {
			saveEqn();
			fChanged = false;
		}
		drawMenuBar();
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

	CursesContext* cc;
	if (argc > 1) {
		ifstream in(argv[1]);
		cc = new CursesContext(new CursesGraphics(), in);
	}
	else {
		cc = new CursesContext(new CursesGraphics());
    }
	GlobalContext::current = GlobalContextPtr(cc);
	cc->saveEqn();
	cc->initMenuBar();
	cc->do_loop();
	return 0;
}
