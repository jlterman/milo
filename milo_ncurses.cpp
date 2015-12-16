#include <ncurses.h>
#include <iostream>
#include <typeinfo>
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
			initscr(); raw(); noecho(); 
#ifndef DEBUG
			curs_set(0);
#endif
			keypad(stdscr, TRUE);
			mousemask(ALL_MOUSE_EVENTS, NULL);
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
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param c  Character to be drawn at x0,y0.
	 * @param color Color of line.
	 */
	void at(int x, int y, int c, Color color = BLACK) {
		if (c == 'P') c = ACS_PI;
		if (m_select.inside(x, y)) c |= A_REVERSE;
		if (color != BLACK && m_has_colors) attron(COLOR_PAIR(color));
		mvaddch(y + m_yOrig, x + m_xOrig, c);
		if (color != BLACK && m_has_colors) attroff(COLOR_PAIR(color));
	}

	/**
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param s  String to be drawn at x0,y0.
	 * @param color Color of line.
	 */
	void at(int x, int y, const string& s, Color color = BLACK) {
		if (color != BLACK && m_has_colors) attron(COLOR_PAIR(color));
		if (m_select.inside(x, y)) attron(A_REVERSE);

		move(y + m_yOrig, x + m_xOrig); printw(s.c_str());

		if (color != BLACK && m_has_colors) attroff(COLOR_PAIR(color));
		if (m_select.inside(x, y)) attroff(A_REVERSE);
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
	unsigned int getChar(int y, int x) {
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
};

bool CursesGraphics::init = false;
bool CursesGraphics::m_has_colors = false;

void CursesGraphics::parenthesis(int x_size, int y_size, int x0, int y0)
{
	if (y_size == 1) {
		at(x0, y0, '[');
		at(x0 + x_size - 1, y0, ']');
	}
	else {
		at(x0, y0 - 1, ins(x0, y0 - 1) == ACS_HLINE ? ACS_TTEE : ACS_ULCORNER);
		at(x0, y0 + y_size, ins(x0, y0 + y_size) == ACS_HLINE ? ACS_BTEE : ACS_LLCORNER);
		at(x0 + x_size - 1, y0 - 1, ins(x0 + x_size - 1, y0 - 1) == ACS_HLINE ? ACS_TTEE : ACS_URCORNER);
		at(x0 + x_size - 1, y0 + y_size, ins(x0 + x_size - 1, y0 + y_size) == ACS_HLINE ? ACS_BTEE : ACS_LRCORNER);

		for (int y = 0; y < y_size; ++y) {
			at(x0, y + y0, ACS_VLINE);
			at(x0 + x_size - 1, y + y0, ACS_VLINE);
			
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
	while (fRunning) {
		eqn->draw(gc);
		gc.out();
		int xCursor = 0, yCursor = 0;
		eqn->getCursorOrig(xCursor, yCursor);
		move(0,0);
		int ch = (eqn->blink()) ? gc.getChar(yCursor, xCursor - 1) : getch();

		string mkey;
		if (ch < 32) mkey = "ctrl-" + string(1, (char) ch+'@');
		else if (ch == 32) mkey = "SPACE";
		else if (ch > 127) mkey = to_string(ch);
		else mkey = string(1, (char)ch);
		LOG_TRACE_MSG("Char typed: " + mkey);

		bool fChanged = true;
		if (!eqn->handleChar(ch)) {
			switch (ch) 
			{
			    case 3: { // ctrl-c typed
					LOG_TRACE_MSG("ctrl-c typed");
					fRunning = false;
					fChanged = false;
					break;
				}
			    case 19: { // ctrl-s typed
					string store;
					eqn->xml_out(store);
					fstream out("eqn.xml", fstream::out | fstream::trunc);
					out << store << endl;
					out.close();
					fChanged = false;
					break;
				}
			    case 26: { // ctrl-z typed
					Equation* undo_eqn = eqns.undo();
					if (undo_eqn) {
						delete eqn;
						eqn = undo_eqn;
						fChanged = false;
						LOG_TRACE_MSG("undo to " + eqn->toString());
					}
					break;
			    }
			    case KEY_MOUSE: {
					static Node* start = nullptr;
					static Node* end = nullptr;
					MEVENT event;
					if(getmouse(&event) == OK) {
						LOG_TRACE_MSG("mouse event " + to_hexstring(event.bstate) + " click at: " + 
									  to_string(event.x) + ", " + to_string(event.y));
						if(event.bstate & BUTTON1_PRESSED) {
							start = eqn->findNode(gc, event.x, event.y);
							if (start == nullptr) break;
							eqn->setSelect(start);
							LOG_TRACE_MSG(string("found node: ") + typeid(start).name() + ": " + 
										  start->toString());
							fChanged = false;
						}
						else if(event.bstate & BUTTON1_RELEASED) {
							end = eqn->findNode(gc, event.x, event.y);
							if (end == nullptr) break;
							eqn->setSelect(start, end);
							LOG_TRACE_MSG(string("found node: ") + typeid(end).name() + ": " + 
										  end->toString());
							start = nullptr;
							end = nullptr;
						}
					}
					break;
				}
			    case KEY_RESIZE: {
				    fChanged = false;
					LOG_TRACE_MSG("screen resize detected");
				    break;
			    }
				default: {
					fChanged = false;
					LOG_TRACE_MSG("char not handled");
					break;
				}
			}
		}
		if (fChanged) {
			eqns.save(eqn);
			delete eqn;
			eqn = eqns.top();
			LOG_TRACE_MSG("got new eqn: " + eqn->toString());
		}
	}
}
