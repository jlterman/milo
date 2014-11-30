#include <iostream>
#include <vector>
#include "milo.h"

using namespace std;

class DrawString : public Draw
{
public:
	DrawString(ostream& os) : Draw(), m_os(os) {}

	~DrawString() { }

	void parenthesis(int x_size, int y_size, int x0, int y0);
	void at(int x, int y, char c) { m_field[y][x] = c; }
	void at(int x, int y, const string& s);

	void out() { for (int i = 0; i < m_field.size(); ++i ) m_os << m_field[i] << endl; }

	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		Draw::set(x, y, x0, y0);
		string line;
		for (int i = 0; i < x; ++i) line += ' ';
		for (int i = 0; i < y; ++i) m_field.push_back(line);
	}
private:
	vector<string> m_field;
	ostream& m_os;
};

void DrawString::at(int x, int y, const string& s)
{
	for (int n = 0; n < s.length(); ++n) { m_field[y][x + n] = s[n]; }
}

void DrawString::parenthesis(int x_size, int y_size, int x0, int y0)
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

int main(int argc, char* argv[])
{
	DrawString draw(cout);
	Equation eqn(argv[1], draw);
	cout << "---------" << endl;
	cout << eqn.toString() << endl;
	cout << "---------" << endl;
	string xml;
	eqn.xml_out(xml);
	cout << xml;
	cout << "---------" << endl;
	eqn.asciiArt();
	draw.out();
	cout << "---------" << endl;
	istringstream in(xml);
	Equation new_eqn(in, draw);
	new_eqn.xml_out(cout);
	cout << "---------" << endl;
}
