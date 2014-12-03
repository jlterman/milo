#include <ncurses.h>
#include <iostream>
#include "milo.h"

using namespace std;

class DrawCurses : public Draw
{
public:
	DrawCurses() { 
		if (!init) {
			initscr(); raw(); noecho();
			keypad(stdscr, TRUE);
			init = true;
		}
	}
	~DrawCurses() { endwin(); }

	void at(int x, int y, char c) {
		mvaddch(y + m_yOrig, x + m_xOrig, c);
	}

	void at(int x, int y, const string& s) {
		move(y + m_yOrig, x + m_xOrig); printw(s.c_str());
	}

	void out() { refresh(); }

	void set(int x, int y, int x0 = 0, int y0 = 0) { 
		int row, col;
		getmaxyx(stdscr, row, col);
		Draw::set(x, y, (col - x)/2, (row - y)/2);
		clear();
	}

	void parenthesis(int x_size, int y_size, int x0, int y0);

	int getChar(int y, int x) {
		mvaddch(y + m_yOrig, x + m_xOrig, ' '); 
		move(y + m_yOrig, x + m_xOrig);
		return getch();
	}

	static DrawCurses& getInstance() {
		static DrawCurses draw;
		
		return draw;
	}

private:
	static bool init;
};

bool DrawCurses::init = false;

void DrawCurses::parenthesis(int x_size, int y_size, int x0, int y0)
{
	if (y_size == 1) {
		at(x0, y0, '(');
		at(x0 + x_size - 1, y0, ')');
	}
	else {
		at(x0, y0, '/');
		at(x0, y0 + y_size - 1, '\\');
		at(x0 + x_size - 1, y0, '\\');
		at(x0 + x_size - 1, y0 + y_size - 1, '/');
		for (int y = 1; y < y_size - 1; ++y) {
			at(x0, y + y0, '|');
			at(x0 + x_size - 1, y + y0, '|');
			
		}
	}
}

static vector<string> eqn_list;

int main(int argc, char* argv[])
{
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

		int ch = draw.getChar(cur->getTermOrigY(), cur->getTermOrigX() + cur->getTermSizeX() - 1);
		LOG_TRACE_MSG("Char typed: " + (ch<32 ? "ctrl-" + to_string(ch) : string(1, (char)ch)));

		bool fChanged = true;
		if (!cur->handleChar(ch)) {
			switch (ch) 
			{
			    case 3: { // ctrl-c typed
					fRunning = false;                          LOG_TRACE_MSG("ctrl-c typed");
					fChanged = false;
				}
				case 9: { // tab typed
					draw.at(cur->getTermOrigY(), cur->getTermOrigX() + cur->getTermSizeX() - 1, '?');
					eqn->nextInput();
					fChanged = false;                          LOG_TRACE_MSG("tab typed");
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
				default: {
					fChanged = false;                          LOG_TRACE_MSG("char not handled");
					break;
				}
			}
		}
		if (fChanged) {
			eqns.save(eqn);
			delete eqn;
			eqn = eqns.top();                                  LOG_TRACE_MSG("got new eqn");
		}
	}
}
