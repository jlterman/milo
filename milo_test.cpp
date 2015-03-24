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
	int getDifferentialHeight(char c) { return 3; }
	int getDifferentialWidth(char c)  { return 2; }
	int getDifferentialBase(char c)   { return 1; }

	void differential(int x0, int y0, char variable);
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

void AsciiGraphics::differential(int x0, int y0, char variable)
{
	m_field[y0][x0+1] = 'd';
	m_field[y0+1][x0+1] = '-';
	m_field[y0+2][x0] = 'd';
	m_field[y0+2][x0+ 1] = variable;
}

static Equation eqn("?");

static void parse(const string& eqn_str)
{
	Equation new_eqn(eqn_str);
	eqn = new_eqn;
}

static void xml_in(const string& fname)
{
	ifstream in(fname);
	Equation new_eqn(in);
	eqn = new_eqn;
}

static void xml_out(const string&)
{
	string xml;
	eqn.xml_out(xml);
	cout << xml << endl;
}

static void test(const string&)
{
	AsciiGraphics gc(cout);
	cout << "---------" << endl;
	cout << eqn.toString() << endl;
	cout << "---------" << endl;
	string xml;
	eqn.xml_out(xml);
	cout << xml << endl;
	cout << "---------" << endl;
	eqn.draw(gc);
	gc.out();
	cout << "---------" << endl;
	istringstream in(xml);
	Equation new_eqn(in);
	string xml2;
	new_eqn.xml_out(xml2);
	if (xml != xml2) cout << xml2; else cout << "XML test passed" << endl; 
	cout << "---------" << endl;
}

static void eqn_out(const string&)
{
	cout << eqn.toString() << endl;
}

static void art(const string&)
{
	AsciiGraphics gc(cout);
	eqn.draw(gc);
	gc.out();
}

static void normalize(const string&)
{
	eqn.normalize();
}

static void simplify(const string&)
{
	eqn.simplify();
}

static void help(const string&);

typedef void (*func_ptr)(const string& s);

const map<string, func_ptr> test_funcs = {
	{ "parse:",    parse     },
	{ "xml:",      xml_in    },
	{ "test",      test      },
	{ "ascii-art", art       },
	{ "eqn-out",   eqn_out   },
	{ "xml-out",   xml_out   },
	{ "normalize", normalize },
	{ "simplify",  simplify  },
	{ "help",      help      }
};

static void help(const string&)
{
	cerr << "Options:" << endl;
	for (auto& m : test_funcs) {
		cerr << "    --";

		string option = m.first;
		if (option.back() == ':') {
			cerr << option.substr(0, option.length() - 1) << " <arg> " << endl;
		}
		else {
			cerr << option << endl;
		}
	}
}

int main(int argc, char* argv[])
{
	int i = 1;
	while (i < argc && argv[i][0] == '-' && argv[i][1] == '-') {
		string option(argv[i]+2);
		if ( test_funcs.find(option) != test_funcs.end() ) {
			test_funcs.at(option)(string());
		}
		else if ( test_funcs.find(option + ":") != test_funcs.end() ) {
			test_funcs.at(option + ":")(argv[++i]);
		}
		else {
			cerr << "Unknown option" << endl;
			exit(1);
		}
		++i;
	}
}
