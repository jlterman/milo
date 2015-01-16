#include <iostream>
#include <vector>
#include <map>
#include "milo.h"

using namespace std;

class AsciiGraphics : public Graphics
{
public:
	AsciiGraphics(ostream& os) : Graphics(), m_os(os) {}

	~AsciiGraphics() { }

	int getTextHeight() { return 1; }
	int getTextLength(const std::string& s) { return s.length(); }
	int getCharLength(char c) { return 1; }
	int getParenthesisWidth(int height = 1) { return 1; }
	int getDivideLineHeight() { return 1; }

	void parenthesis(int x_size, int y_size, int x0, int y0);
	void horiz_line(int x_size, int x0, int y0) { for (int i = 0; i < x_size; ++i) at(x0 + i, y0, '-'); }
	void at(int x, int y, int c, Color color = BLACK) { m_field[y][x] = c; m_colors[y][x] = color; }
	void at(int x, int y, const string& s, Color color = BLACK);

	void out();

	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		Graphics::set(x, y, x0, y0);
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

void AsciiGraphics::at(int x, int y, const string& s, Color color)
{
	for (int n = 0; n < s.length(); ++n) { m_field[y][x + n] = s[n]; m_colors[y][x + n] = color; }
}

void AsciiGraphics::out()
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

void AsciiGraphics::parenthesis(int x_size, int y_size, int x0, int y0)
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

static void test_parse(const string& eqn_str)
{
	AsciiGraphics gc(cout);
	Equation eqn(eqn_str);
	cout << "---------" << endl;
	cout << eqn.toString() << endl;
	cout << "---------" << endl;
	string xml;
	eqn.xml_out(xml);
	cout << xml;
	cout << "---------" << endl;
	eqn.draw(gc);
	gc.out();
	cout << "---------" << endl;
	istringstream in(xml);
	Equation new_eqn(in);
	string xml2;
	new_eqn.xml_out(xml2);
	if (xml.compare(xml2) != 0) cout << xml2; else cout << "XML test passed" << endl; 
	cout << "---------" << endl;
}

static void test_xml(const string& fname)
{
	AsciiGraphics gc(cout);
	ifstream in(fname);
	Equation eqn(in);
	cout << eqn.toString() << endl;
	cout << "------------" << endl;
	eqn.draw(gc);
	gc.out();
}

typedef void (*func_ptr)(const string& s);

const map<string, func_ptr> test_funcs = {
	{ "parse", test_parse },
	{ "xml",   test_xml },
};

int main(int argc, char* argv[])
{
	map<string, string> args;
	int i = 1;
	while (i < argc && argv[i][0] == '-' && argv[i][1] == '-') {
		string option(argv[i]+2);
		string arg;
		if (  test_funcs.find(option) == test_funcs.end() ) {
			cerr << "Unknown option: " << option << endl;
			exit(1);
		}
		if ((i+1) < argc && argv[i+1][0] != '-') arg = argv[++i];
		test_funcs.at(option)(arg);
		++i;
	}
	string opt;
	string arg;
	for ( auto it : args ) {
		tie( opt, arg ) = it;
		auto f = test_funcs.find(opt);
		test_funcs.at(opt)(arg);
	}
}
