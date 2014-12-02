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
	eqn_list.emplace_back("#");
	Equation* eqn = new Equation("#", draw);
	bool fRunning = true;
	while (fRunning) {
		eqn->asciiArt();
		DrawCurses::getInstance().out();
		Input* cur = eqn->getCurrentInput();

		bool fChanged = true;
		int ch = draw.getChar(cur->getTermOrigY(), cur->getTermOrigX() + cur->getTermSizeX() - 1);
		LOG_TRACE_MSG("Char typed: " + string(1, (char)ch));
		
		if (isalnum(ch)) {
			cur->addTyped(ch);
		}
		else {
			switch (ch) 
			{
				case KEY_BACKSPACE: {
					cur->delTyped();                           LOG_TRACE_MSG("Delete typed char");
					break;
				}
			    case 3: { // ctrl-c typed
					fRunning = false;                          LOG_TRACE_MSG("ctrl-c typed");
					fChanged = false;
				}
				case 9: { // tab typed
					draw.at(cur->getTermOrigY(), cur->getTermOrigX(), '?');
					eqn->nextInput();
					fChanged = false;                          LOG_TRACE_MSG("tab typed");
					break;
				}
			    case 13: { // enter typed
					if ( isalnum(cur->toString().back()) ) {
					    cur->disable();
						eqn->nextInput();                      LOG_TRACE_MSG("enter typed, disabled input");
					}
					else {
						fChanged = false;
					}
					break;
				}
			    case 26: { // ctrl-z typed
					string old_eqn = eqn_list.back();
					eqn_list.pop_back();
					delete eqn;
					istringstream in(old_eqn);
					eqn = new Equation(in, DrawCurses::getInstance());
					fChanged = false;                          LOG_TRACE_MSG("undo");
					break;
			    }
			    case '+': case '-':	case '^': case '/':	case '(': {
					cur->handleChar(ch);                       LOG_TRACE_MSG("handle operator chr");
					break;
				}
				default: {
					fChanged = false;                          LOG_TRACE_MSG("char not handled");
					break;
				}
			}
		}
		if (fChanged) {
			string new_eqn;
			eqn->xml_out(new_eqn);                             LOG_TRACE_MSG("wrote xml");
			eqn_list.push_back(new_eqn);
			delete eqn;
			istringstream in(new_eqn);
			eqn = new Equation(in, DrawCurses::getInstance()); LOG_TRACE_MSG("got new eqn");
		}
	}
}
