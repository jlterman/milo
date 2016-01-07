#include <iostream>
#include <vector>
#include <map>
#include "milo.h"

using namespace std;

/**
 * Class derived from ABC graphics so Equation can draw to a 2D character array.
 */
class AsciiGraphics : public Graphics
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	AsciiGraphics(ostream& os) : Graphics(), m_os(os) {}

	~AsciiGraphics() { }
	//@}

	/** @name Virtual Public Member Functions */
	//@{

	/**
	 * Get height of text in lines.
	 * @return All ASCII characters are one line in height.
	 */
	int getTextHeight() { return 1; }

	/**
	 * Get length of string in characters.
	 * @return Length of string in characters.
	 */
	int getTextLength(const std::string& s) { return s.length(); }

	/**
	 * Length of character in ASCII is 1.
	 * @return Return one.
	 */
	int getCharLength(char) { return 1; }

	/**
	 * Get width of a parenthesis in characters which is always 1.
	 * @return Return one.
	 */
	int getParenthesisWidth(int) { return 1; }

	/**
	 * Get height of line in a division node which is always 1.
	 * @return Return one.
	 */
	int getDivideLineHeight() { return 1; }

	/**
	 * Get height of differential in lines of text.
	 * @param c Variable name of differential.
	 * @return Height of differential in lines of text.
	 */
	int getDifferentialHeight(char) { return 3; }

	/**
	 * Get width of differential in lines of text.
	 * @param c Variable name of differential.
	 * @return Width of differential in lines of text.
	 */
	int getDifferentialWidth(char)  { return 2; }

	/**
	 * Get vertical offset of differential.
	 * @param c Variable name of differential.
	 * @return Vertical offset of differential in lines of text.
	 */
	int getDifferentialBase(char)   { return 1; }

	/**
	 * Draw differential of width x0 and height y0 with char variable name.
	 * @param x0 Horizontal origin of differential.
	 * @param y0 Vertical origin of differential.
	 * @param variable Name of variable of differential.
	 */
	void differential(int x0, int y0, char variable);

	/**
	 * Draw pair of parenthesis around a node of size x_size, y_size and origin x0,y0.
	 * @param x_size Horizontal size of both parenthesis.
	 * @param y_size Vertical size of parenthesis.
	 * @param x0 Horizontal origin of parenthesis.
	 * @param y0 Vertical origin of parenthesis.
	 */
	void parenthesis(int x_size, int y_size, int x0, int y0);

	/**
	 * Draw a horizontal line starting at x0,y0 length x_size.
	 * @param x_size Horizontal size of both line.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 */
	void horiz_line(int x_size, int x0, int y0) { for (int i = 0; i < x_size; ++i) at(x0 + i, y0, '-'); }

	/**
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param c  Character to be drawn at x0,y0.
	 * @param color Color of line.
	 */
	void at(int x, int y, int c, Attributes, Color color = BLACK) { m_field[y][x] = c; m_colors[y][x] = color; }

	/**
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param s  String to be drawn at x0,y0.
	 * @param color Color of line.
	 */
	void at(int x, int y, const string& s, Attributes, Color color = BLACK);


	/**
	 * Flush all characters to the text array.
	 */
	void out();

	/**
	 * Set size of the text array (origin is always 0,0).
	 * @param x Horizontal size of text array.
	 * @param y Vertical size of text array.
	 * @param x0 Not used.
	 * @param y0 Not used.
	 */
	virtual void set(int x, int y, int, int) { 
		Graphics::set(x, y, 0, 0);
		for (int i = 0; i < y; ++i) {
			m_field.emplace_back(x, ' ');
			m_colors.emplace_back(x, Color::BLACK);
		}
	}
	//@}
private:
	vector<string> m_field;           ///< 2D text array.
	vector< vector<Color> > m_colors; ///< 2D color array.
	ostream& m_os;                    ///< Stream to ouput text array.

	/**
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param c  Character to be drawn at x0,y0.
	 */
	void at(int x, int y, int c) { m_field[y][x] = c; m_colors[y][x] = Color::BLACK; }
};

void AsciiGraphics::at(int x, int y, const string& s, Attributes, Color color)
{
	for (unsigned int n = 0; n < s.length(); ++n) { 
		m_field[y][x + n] = s[n]; m_colors[y][x + n] = color;
	}
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

/** Global Equation object.
 */
static Equation eqn("?");

/**
 * Load parsed equation string into global equation object.
 * @param eqn_str String with text version of equation.
 */
static void parse(const string& eqn_str)
{
	Equation new_eqn(eqn_str);
	eqn = new_eqn;
}

/**
 * Load filename with xml version of equation
 * @param fname Name of file with xml
 */
static void xml_in(const string& fname)
{
	ifstream in(fname);
	Equation new_eqn(in);
	eqn = new_eqn;
}

/** Output current equation in xml to standard output.
 */
static void xml_out(const string&)
{
	string xml;
	eqn.xml_out(xml);
	cout << xml << endl;
}

/** Test output of current equation.
 */
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

/** Output current equation in ascii to standard output.
 */
static void eqn_out(const string&)
{
	cout << eqn.toString() << endl;
}

/** Output current equation as ascii art to standard output.
 */
static void art(const string&)
{
	AsciiGraphics gc(cout);
	eqn.draw(gc);
	gc.out();
}

/** Normalize current equation.
 */
static void normalize(const string&)
{
	eqn.normalize();
}

/** Simplify current equation.
 */
static void simplify(const string&)
{
	eqn.simplify();
}

/** Output help to standard output.
 */
static void help(const string&);

/** Pointer to function from command line argument.
 */
typedef void (*func_ptr)(const string& s);

/** Associate command with an internal function. ':' means it takes argument.
 */
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

/**
 * Command line milo test main function.
 * Takes name of test for argument plus argument passed to text if needed.
 * More than one test command maybe passed on command line for serial tests.
 * @param argc Number of arguments.
 * @param argv Array of argugments.
 * @return Exit code.
 */
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
