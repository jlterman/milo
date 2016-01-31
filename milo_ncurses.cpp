#include <ncursesw/ncurses.h>
#include <locale.h>
#include "milo.h"

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
			keypad(stdscr, TRUE);
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
	static bool init;          ///< Singleton has been initialized.
	static bool m_has_colors;  ///< Singleton flag is screen has colors.

	static const map<Attributes, int> attribute_map;

	static const map<char, string> char_map;

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
bool CursesGraphics::m_has_colors = false;

const map<Graphics::Attributes, int> CursesGraphics::attribute_map = {
	{ Graphics::Attributes::NONE,   A_NORMAL },
	{ Graphics::Attributes::BOLD,   A_BOLD },
	{ Graphics::Attributes::ITALIC, A_ITALIC },
	{ Graphics::Attributes::BOLD_ITALIC, A_ITALIC|A_BOLD }
};

const map<char, string> CursesGraphics::char_map = {
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
 * Main routine for milo ncurses command line app.
 * @param argc Number of arguments. Only first is used for equation xml file.
 * @param argv Array of argugments.
 * @return Exit code.
 */
int main(int argc, char* argv[])
{
	LOG_TRACE_CLEAR();
	LOG_TRACE_MSG("Starting milo_ncurses...");

	Equation* eqn;
	if (argc > 1) {
		ifstream in(argv[1]);
		eqn = new Equation(in);
	}
	else
		eqn = new Equation("#");

	CursesGraphics& gc = CursesGraphics::getInstance();
	EqnUndoList eqns;
	eqns.save(eqn);
	
	bool fRunning = true;
	bool fChanged = true;
	Node* start_select = nullptr;
	int start_mouse_x = ERR, start_mouse_y = ERR;
	while (fRunning) {
		if (fChanged) {
			eqns.save(eqn);
			delete eqn;
			eqn = eqns.top();
			eqn->draw(gc, true);
			fChanged = false;
		}

		int xCursor = 0, yCursor = 0;
		eqn->getCursorOrig(xCursor, yCursor);
		int ch = gc.getChar(yCursor, xCursor - 1, eqn->blink());

		string mkey;
		if (ch < 32) mkey = "ctrl-" + string(1, (char) ch+'@');
		else if (ch == 32) mkey = "SPACE";
		else if (ch > 127) mkey = to_string(ch);
		else mkey = string(1, (char)ch);
		LOG_TRACE_MSG("Char typed: " + mkey);

		switch (ch) 
		{
		    case 3: { // ctrl-c typed
				LOG_TRACE_MSG("ctrl-c typed");
				fRunning = false;
				break;
			}
		    case 19: { // ctrl-s typed
				string store;
				eqn->xml_out(store);
				fstream out("eqn.xml", fstream::out | fstream::trunc);
				out << store << endl;
				out.close();
				break;
			}
		    case 26: { // ctrl-z typed
				Equation* undo_eqn = eqns.undo();
				if (undo_eqn) {
					delete eqn;
					eqn = undo_eqn;
					LOG_TRACE_MSG("undo to " + eqn->toString());
					eqn->draw(gc, true);
				}
				break;
			}
		    case KEY_MOUSE: {
				MEVENT event;
				if (getmouse(&event) == OK) {
					if (event.bstate & BUTTON1_PRESSED) {
						start_select = eqn->findNode(gc, event.x, event.y);
						start_mouse_x = event.x; start_mouse_y = event.y;
						if (start_select == nullptr) break;
						eqn->setSelect(start_select);
						eqn->draw(gc, true);
					}
					else if (event.bstate & BUTTON1_RELEASED) {
						start_mouse_x = start_mouse_y = ERR;
						if (start_select->getSelect() == Node::Select::ALL) {
							eqn->selectNodeOrInput(start_select);
						}
						start_select = nullptr;
						fChanged = true;
					}
					else if (event.bstate & REPORT_MOUSE_POSITION) {
						Box box = { 0, 0, 0, 0 };
						if (start_mouse_x != ERR && start_mouse_y != ERR) {
							box = { abs(start_mouse_x - event.x),
									abs(start_mouse_y - event.y),
									min(start_mouse_x,  event.x),
									min(start_mouse_y,  event.y)
							}; 
						}
						if (start_select != nullptr && box.area() > 1 ) {
							Node* new_select = eqn->findNode(gc, box);
							if (new_select != nullptr && new_select->getDepth() < start_select->getDepth()) start_select = new_select;
							eqn->selectBox(gc, start_select, box);
							eqn->draw(gc, true);
						}
						else if (start_select == nullptr && box.area() > 0) {
							start_select = eqn->findNode(gc, box);
							eqn->setSelect(start_select);
							eqn->draw(gc, true);
						}
					}
				}
				break;
			}
		    case KEY_RESIZE: {
				LOG_TRACE_MSG("screen resize detected");
				break;
			}
		    default: {
				fChanged = eqn->handleChar(ch);
				if (!fChanged) LOG_TRACE_MSG("character not handled");
				break;
			}
		}
	}
}
