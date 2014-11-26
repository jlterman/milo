#include <iostream>
#include <vector>
#include "milo.h"

using namespace std;

class DrawString : public DrawText
{
public:
	DrawString(ostream& os) : DrawText(), m_os(os) {}

	~DrawString() { }

	void at(int x, int y, char c) { m_field[y][x] = c; }

	void out() { for (int i = 0; i < m_field.size(); ++i ) m_os << m_field[i] << endl; }

	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		DrawText::set(x, y, x0, y0);
		string line;
		for (int i = 0; i < x; ++i) line += ' ';
		for (int i = 0; i < y; ++i) m_field.push_back(line);
	}
private:
	vector<string> m_field;
	ostream& m_os;
};

int main(int argc, char* argv[])
{
	DrawString draw(cout);
	Equation eqn(argv[1], draw);
	cout << eqn.toString() << endl;
	cout << "---------" << endl;
	{
		XML xml(cout);
		eqn.xml_out(xml);
	}
	cout << "---------" << endl;
	{
		eqn.asciiArt();
		draw.out();
	}
}
