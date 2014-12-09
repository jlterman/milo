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
	void horiz_line(int x_size, int x0, int y0) { for (int i = 0; i < x_size; ++i) at(x0 + i, y0, '-'); }
	void at(int x, int y, int c, Color color = BLACK) { m_field[y][x] = c; m_colors[y][x] = color; }
	void at(int x, int y, const string& s, Color color = BLACK);

	void out();

	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		Draw::set(x, y, x0, y0);
		for (int i = 0; i < y; ++i) {
			m_field.emplace_back(x, ' ');
			m_colors.emplace_back(x, Color::BLACK);
		}
	}
private:
	vector<string> m_field;
	vector< vector<Color> > m_colors;
	ostream& m_os;
};

void DrawString::at(int x, int y, const string& s, Color color)
{
	for (int n = 0; n < s.length(); ++n) { m_field[y][x + n] = s[n]; m_colors[y][x + n] = color; }
}

void DrawString::out()
{ 
	for (int i = 0; i < m_ySize; ++i ) {
		for (int j = 0; j < m_xSize; ++j) {
			if (m_colors[i][j]) m_os << "\033[3" + to_string(m_colors[i][j]) + "m";
			m_os << m_field[i][j];
			if (m_colors[i][j]) m_os << "\033[37m";
		}
		m_os << endl;
	}
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
	Equation eqn(argv[1]);
	cout << "---------" << endl;
	cout << eqn.toString() << endl;
	cout << "---------" << endl;
	string xml;
	eqn.xml_out(xml);
	cout << xml;
	cout << "---------" << endl;
	eqn.asciiArt(draw);
	draw.out();
	cout << "---------" << endl;
	istringstream in(xml);
	Equation new_eqn(in);
	string xml2;
	new_eqn.xml_out(xml2);
	if (xml.compare(xml2) != 0) cout << xml2; else cout << "XML test passed" << endl; 
	cout << "---------" << endl;
}
