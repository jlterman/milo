#include <ncurses.h>
#include <iostream>
#include <typeinfo>
#include "milo.h"

using namespace std;

class CursesGraphics : public Graphics
{
public:
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

	int getTextHeight() { return 1; }
	int getTextLength(const std::string& s) { return s.length(); }
	int getCharLength(char c) { return 1; }
	int getParenthesisWidth(int height = 1) { return 1; }
	int getDivideLineHeight() { return 1; }
	int getDifferentialHeight(char c) { return 3; }
	int getDifferentialWidth(char c)  { return 2; }
	int getDifferentialBase(char c)   { return 1; }

	void at(int x, int y, int c, Color color = BLACK) {
		if (c == 'P') c = ACS_PI;
		if (m_select.inside(x, y)) c |= A_REVERSE;
		if (color != BLACK && m_has_colors) attron(COLOR_PAIR(color));
		mvaddch(y + m_yOrig, x + m_xOrig, c);
		if (color != BLACK && m_has_colors) attroff(COLOR_PAIR(color));
	}

	void at(int x, int y, const string& s, Color color = BLACK) {
		if (color != BLACK && m_has_colors) attron(COLOR_PAIR(color));
		if (m_select.inside(x, y)) attron(A_REVERSE);

		move(y + m_yOrig, x + m_xOrig); printw(s.c_str());

		if (color != BLACK && m_has_colors) attroff(COLOR_PAIR(color));
		if (m_select.inside(x, y)) attroff(A_REVERSE);
	}

	void out() { refresh(); }

	void set(int x, int y, int x0 = 0, int y0 = 0) { 
		int row, col;
		getmaxyx(stdscr, row, col);
		Graphics::set(x, y, (col - x)/2, (row - y)/2);
		clear();
	}

	void horiz_line(int x_size, int x0, int y0) { 
		for (int i = 0; i < x_size; ++i) at(x0 + i, y0, ACS_HLINE);
	}

	void differential(int x0, int y0, char variable);

	void parenthesis(int x_size, int y_size, int x0, int y0);

	void setSelect(int x, int y, int x0, int y0) { 
		Graphics::setSelect(x, y, x0, y0);
		for (int j = 0; j < y; ++j) mvchgat(m_yOrig + y0 + j, m_xOrig + x0, x, A_REVERSE, 0, NULL);
	}

	int ins(int x, int y) {
		return mvinch(y + m_yOrig, x + m_xOrig);
	}

	int getChar(int y, int x) {
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

	static CursesGraphics& getInstance() {
		static CursesGraphics draw;
		
		return draw;
	}

private:
	static bool init;
	static bool m_has_colors;
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
