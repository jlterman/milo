#include <ncurses.h>
#include <iostream>
#include "milo.h"

using namespace std;

class DrawCurses : public Draw
{
public:
	DrawCurses() { 
		if (!init) {
			initscr(); raw(); noecho(); //curs_set(0);
			keypad(stdscr, TRUE);
			m_has_colors = (has_colors() == TRUE);
			if (m_has_colors) {
				start_color();/* Start color */
				init_color(COLOR_WHITE, 1000, 1000, 1000);
				assume_default_colors(COLOR_BLACK, COLOR_WHITE);
				for (int i = Draw::Color::RED; i <= Draw::Color::WHITE; ++i)
					init_pair(i, COLOR_BLACK + i, COLOR_WHITE);
			}
			init = true;
		}
	}
	~DrawCurses() { endwin(); }

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
		Draw::set(x, y, (col - x)/2, (row - y)/2);
		clear();
	}

	void horiz_line(int x_size, int x0, int y0) { 
		for (int i = 0; i < x_size; ++i) at(x0 + i, y0, ACS_HLINE);
	}

	void parenthesis(int x_size, int y_size, int x0, int y0);

	void setSelect(int x, int y, int x0, int y0) { 
		Draw::setSelect(x, y, x0, y0);
		for (int j = 0; j < y; ++j) mvchgat(m_yOrig + y0 + j, m_xOrig + x0, x, A_REVERSE, 0, NULL);
	}

	int ins(int x, int y) {
		return mvinch(y + m_yOrig, x + m_xOrig);
	}

	int getChar(int y, int x) {
		//curs_set(2);
		mvaddch(y + m_yOrig, x + m_xOrig, ' '); 
		move(y + m_yOrig, x + m_xOrig);
		int ch = getch();
		//curs_set(0);
		return ch;
	}

	static DrawCurses& getInstance() {
		static DrawCurses draw;
		
		return draw;
	}

private:
	static bool init;
	static bool m_has_colors;
};

bool DrawCurses::init = false;
bool DrawCurses::m_has_colors = false;

void DrawCurses::parenthesis(int x_size, int y_size, int x0, int y0)
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

static vector<string> eqn_list;

int main(int argc, char* argv[])
{
	LOG_TRACE_CLEAR();
	LOG_TRACE_MSG("Starting milo_ncurses...");
	DrawCurses& draw = DrawCurses::getInstance();
	EqnUndoList eqns;
	Equation* eqn = new Equation("#");
	eqns.save(eqn);

	bool fRunning = true;
	while (fRunning) {
		eqn->asciiArt(draw);
		DrawCurses::getInstance().out();
		Input* cur = eqn->getCurrentInput();
		int ch = (cur) ? draw.getChar(cur->getOrigY(), cur->getOrigX() + cur->getSizeX() - 1)
			           : getchar();

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
					fRunning = false;                          LOG_TRACE_MSG("ctrl-c typed");
					fChanged = false;
					break;
				}
				case 9: { // tab typed
					fChanged = false;                          LOG_TRACE_MSG("tab typed");
					if (!cur) break;
					draw.at(cur->getOrigY(), cur->getOrigX() + cur->getSizeX() - 1, '?');
					eqn->nextInput();
					break;
				}
			    case 10: { // enter typed
					fChanged = eqn->disableCurrentInput();
					break;
				}
			    case 26: { // ctrl-z typed
					Equation* undo_eqn = eqns.undo();
					if (undo_eqn) {
						delete eqn;
						eqn = undo_eqn;
						fChanged = false;                      LOG_TRACE_MSG("undo to " + eqn->toString());
					}
					break;
			    }
			    case KEY_RESIZE: {
				    fChanged = false;                          LOG_TRACE_MSG("screen resize detected");
				    break;
			    }
				default: {
					fChanged = false;                          LOG_TRACE_MSG("char not handled");
					break;
				}
			}
		}
		if (fChanged) {
			eqns.save(eqn);
			delete eqn;
			eqn = eqns.top();                                  LOG_TRACE_MSG("got new eqn: " + eqn->toString());
		}
	}
}
