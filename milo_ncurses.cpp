#include <ncurses.h>
#include <iostream>
#include "milo.h"

using namespace std;

class DrawCurses : public DrawText
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
		DrawText::set(x, y, (col - x)/2, (row - y)/2);
		clear();
	}

	int getChar(int x, int y) {
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

static vector<string> eqn_list;
static const bool isAlphaNum[128] = 
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, //  !"#$%&'()*+,-./
	  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 0123456789:;<=>?
	  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @ABCDEFGHIJKLMNO
	  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // PQRSTUVWXYZ[\]^_
	  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // `abcdefghijklmno
	  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0  // pqrstuvwxyz{|}~
	};

int main(int argc, char* argv[])
{
	DrawCurses& draw = DrawCurses::getInstance();
	eqn_list.emplace_back("#");
	Equation* eqn = new Equation("#", draw);
	bool fRunning = true;
	while (fRunning) {
		eqn->asciiArt();
		DrawCurses::getInstance().out();
		Input* cur = eqn->getCurrentInput();

		bool fChanged = true;
		char ch = draw.getChar(cur->getTermOrigY(), cur->getTermOrigX());
		
		if (isAlphaNum[ch]) {
			cur->addTyped(ch);
		}
		else {
			switch (ch) 
			{
				case KEY_BACKSPACE: {
					cur->delTyped();
					break;
				}
			    case 3: { // ctrl-c typed
					fRunning = false;
					fChanged = false;
				}
				case 9: { // tab typed
					draw.at(cur->getTermOrigY(), cur->getTermOrigX(), '?');
					eqn->nextInput();
					fChanged = false;
					break;
				}
			    case 13: { // enter typed
					if ( isalnum(cur->toString().back()) ) {
					    cur->disable();
						eqn->nextInput();
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
					fChanged = false;
			    }
				case '/':
				case '(': {
					cur->handleChar(ch);
					break;
				}
				default: {
					fChanged = false;
					break;
				}
			}
		}
		if (fChanged) {
			string new_eqn;
			eqn->xml_out(new_eqn);
			eqn_list.push_back(new_eqn);
			delete eqn;
			istringstream in(new_eqn);
			eqn = new Equation(in, DrawCurses::getInstance());
		}
	}
}
