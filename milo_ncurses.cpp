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
		mvaddch( y + m_yOrig, x + m_xOrig, c);
	}

	void out() { refresh(); }

	void set(int x, int y, int x0 = 0, int y0 = 0) { 
		int row, col;
		getmaxyx(stdscr, row, col);
		DrawText::set(x, y, (col - x)/2, (row - y)/2);
	}

private:
	static bool init;
};

bool DrawCurses::init = false;

int main(int argc, char* argv[])
{
	DrawCurses draw;
	Equation eqn(argv[1], draw);
	eqn.asciiArt();
	draw.out();
	mvaddch(20, 0, '>'); getch();
}
