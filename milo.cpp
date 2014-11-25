#include <iostream>
#include <vector>
#include <iterator>
#include <exception>
#include <map>
#include <limits>
#include <initializer_list>
#include "milo.h"

using namespace std;

class XML
{
public:
	XML(ostream& os) : 
		m_os(os), m_indent(2) { m_os << "<document>" << endl; }
	~XML() { m_os << "</document>" << endl; }

	void header(const string& tag, bool atomic, 
				initializer_list<string>);

	void header(const string& tag, bool atomic = false);

	void footer(const string& tag);
private:
	vector<string> m_tags;
	int m_indent;
	ostream& m_os;

	void print_indent(ostream& os) {
		for (int i = 0; i < m_indent; ++i) m_os << ' ';
	}
};

class Parser
{
public:
	Parser(string expr) : m_expr(expr), m_pos(0) {}

	char peek() { return m_expr[m_pos]; }
	char next();
	bool match(const string& s);
private:
	string m_expr;
	size_t m_pos;
};

class Binary : public Node
{
public:
	Binary(char op, NodePtr one, NodePtr two, NodePtr parent = nullptr) : 
		Node(parent), m_op(op), m_first(one), m_second(two) {}

	~Binary() {}

	string toString() const { return m_first->toString() + m_op + m_second->toString(); }

	static NodePtr parse(Parser& p, NodePtr one, NodePtr parent = nullptr);

	void xml_out(const string& tag, XML& xml) const;

protected:
	NodePtr m_first;
	NodePtr m_second;
	char m_op;
};

class Divide : public Binary
{
public:
	Divide(NodePtr one, NodePtr two, NodePtr parent = nullptr) : 
		Binary('/', one, two, parent) {}
	~Divide() {}

	void xml_out(XML& xml) const { Binary::xml_out("divide", xml); }
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;
};

class Power : public Binary
{
public:
	Power(NodePtr one, NodePtr two, NodePtr parent = nullptr) : 
		Binary('^', one, two, parent) {}
	~Power() {}

	void xml_out(XML& xml) const { Binary::xml_out("power", xml); }
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = map<string, func_ptr>;

	static NodePtr parse(Parser& p, NodePtr parent = nullptr);
	string toString() const { return "\033[32m" + m_name + "\033[30m" + m_arg->toString(); }
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;

	Function(const string& name, func_ptr fp, NodePtr node, NodePtr parent = nullptr) : 
		Node(parent), m_name(name), m_func(fp), m_arg(node) {}
	Function()=delete;
	~Function() {}
private:
	string m_name;
	func_ptr m_func;
	NodePtr m_arg;

	static void init_functions(func_map& fmap);
	static Complex sinZ(Complex z);
	static Complex cosZ(Complex z);
	static Complex tanZ(Complex z);
	static Complex logZ(Complex z);
	static Complex expZ(Complex z);
};

class Variable : public Node
{
public:
	Variable(Parser& p, NodePtr parent) : Node(parent), m_name( p.next() ) {}
	~Variable() {}

	string toString() const { return string(string("") + m_name); }
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;

	static NodePtr parse(Parser& p, NodePtr parent = nullptr);

private:
	char m_name;
};

class Number : public Node
{
public:
	Number(Parser& p, NodePtr parent) : Node(parent), m_isInteger(true) { getNumber(p); }
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;

	static NodePtr parse(Parser& p, NodePtr parent = nullptr);

private:
	string getInteger(Parser& p);

	Complex m_value;
	bool m_isInteger;
};

class Term : public Node
{
public:
	Term(Parser& p, NodePtr parent = nullptr) : Node(parent) { while(add(p)); }

	bool add(Parser& p);
	
	static NodePtr parse(Parser& p, NodePtr parent = nullptr);

	string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;

private:
	vector<NodePtr> factors;
};

class Expression : public Node
{
public:
	Expression(Parser& p, NodePtr parent = nullptr) : Node(parent) { while(add(p)); }
	~Expression() {}

	bool add(Parser& p);

	string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(DrawText& draw) const;

	static NodePtr parse(Parser& p, NodePtr parent = nullptr);

private:
	vector<NodePtr> terms;	
};

class DrawString : public DrawText
{
public:
	DrawString(int x, int  y) : DrawText(x, y) {
		string line;
		for (int i = 0; i < x; ++i) line += ' ';
		for (int i = 0; i < y; ++i) m_field.push_back(line);
	}

	DrawString(NodePtr n) : 
		DrawString(n->getTermSizeX(), n->getTermSizeY()) {}

	~DrawString() { }

	void at(int x, int y, char c) { m_field[y][x] = c; }
	void out(ostream& os) { for (int i = 0; i < m_field.size(); ++i ) os << m_field[i] << endl; }
private:
	vector<string> m_field;
};

void XML::header(const string& tag, bool atomic)
{
	print_indent(m_os);
	m_os << "<" << tag << (atomic ? "/>" : ">") << endl;
	if (!atomic) {
		m_indent += 2;
		m_tags.push_back(tag);
	}
}

void XML::header(const string& tag, bool atomic, 
				 initializer_list<string> children)
{
	print_indent(m_os);
	m_os << "<" << tag;
	if (children.size()%2 == 1) throw invalid_argument("odd xml tag pairs");
	auto it = children.begin();
	while ( it != children.end() ) {
		string name = *it++;
		string value = *it++;
		m_os << " " << name << "=\""<< value << "\"";
	}
	if (atomic) {
		m_os << "/>" << endl;
		return;
	}
	m_os << ">" << endl;
	m_indent += 2;
	m_tags.push_back(tag);
}

void XML::footer(const string& tag)
{
	string close_tag;
	do {
		close_tag = m_tags.back(); m_tags.pop_back();

		m_indent -= 2;
		print_indent(m_os);
		m_os << "</" << close_tag << ">" << endl;

	}
	while (close_tag.compare(tag) != 0);
}

void Node::asciiArtParenthesis(DrawText& draw) const
{
	if (getTermSizeY() == 1) {
		draw.at(getTermOrigX(), getTermOrigY(), '(');
		draw.at(getTermOrigX() + getTermSizeX() - 1, getTermOrigY(), ')');
	}
	else {
		draw.at(getTermOrigX(), getTermOrigY(), '/');
		draw.at(getTermOrigX(), getTermOrigY() + getTermSizeY() - 1, '\\');
		draw.at(getTermOrigX() + getTermSizeX() - 1, getTermOrigY(), '\\');
		draw.at(getTermOrigX() + getTermSizeX() - 1, getTermOrigY() + getTermSizeY() - 1, '/');
		for (int y = 1; y < getTermOrigY() + getTermSizeY() - 1; ++y) {
			draw.at(getTermOrigX(), y, '|');
			draw.at(getTermOrigX() + getTermSizeX() - 1, y, '|');
			
		}
	}
}

void Equation::xml_out(XML& xml) const
{
	xml.header("equation");
	m_root->xml_out(xml);
	xml.footer("equation");
}

Equation::Equation(string eq) { 
	Parser p(eq); 
	m_root = NodePtr(new Expression(p));
}

bool isZero(double x) {
	return abs(x)<1e-10;
}

bool isZero(Complex z) {
	return isZero(z.real()) && isZero(z.imag());
}

void Divide::calcTermSize()
{
	m_first->calcTermSize();
	m_second->calcTermSize();
	setTermSize( max(m_first->getTermSizeX(), m_second->getTermSizeX()),
					 m_first->getTermSizeY() + 1 + m_second->getTermSizeY() );
}

void Divide::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	m_first->calcTermOrig(x + (getTermSizeX() - m_first->getTermSizeX())/2, y);
	m_second->calcTermOrig(x + (getTermSizeX() - m_second->getTermSizeX())/2, 
				 		   y + m_first->getTermSizeY() + 1);
}


void Divide::asciiArt(DrawText& draw) const
{
	m_first->asciiArt(draw);
	for (int x = 0; x < getTermSizeX(); ++x) {
		draw.at(x + getTermOrigX(), getTermOrigY() + getTermSizeY()/2, '-');
	}
	m_second->asciiArt(draw);
}

void Power::calcTermSize()
{
	m_first->calcTermSize();
	m_second->calcTermSize();
	setTermSize(m_first->getTermSizeX() + m_second->getTermSizeX(),
				m_first->getTermSizeY() + m_second->getTermSizeY());
}

void Power::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	m_first->calcTermOrig(x, y + m_second->getTermSizeY());
	m_second->calcTermOrig(x + m_first->getTermSizeX(), y);
}

void Power::asciiArt(DrawText& draw) const
{
	m_first->asciiArt(draw);
	m_second->asciiArt(draw);
}

Complex Function::sinZ(Complex z) {
	double x = z.real(), y = z.imag();
	return Complex(sin(x)*cosh(y), cos(x)*sinh(y));
}

Complex Function::cosZ(Complex z) {
	double x = z.real(), y = z.imag();
	return Complex(cos(x)*cosh(y), -sin(x)*sinh(y));
}

Complex Function::tanZ(Complex z) {
	double x = z.real(), y = z.imag();
	double r = cos(x)*cos(x)*cosh(y)*cosh(y) + 
               sin(x)*sin(x)*sinh(y)*sinh(y);
	return Complex(sin(x)*cos(x)/r, sinh(y)*cosh(y)/r);
}

Complex Function::logZ(Complex z) {
	if (isZero(z)) {
		return Complex(-1*numeric_limits<float>::infinity(), 0);
	}
	double x = abs(z.real()), y = abs(z.imag());
	bool neg_x = signbit(z.real()), neg_y = signbit(z.imag());
	if (isZero(x)) return Complex(log(y), (neg_y ? -1 : 1)*M_PI/2);
	if (isZero(y)) return Complex(log(x),  neg_x ? -M_PI : 0);

	return Complex(log(x*x + y*y)/2, 
				   atan(y/x)*((neg_x == neg_y) ? 1 : -1));
}

Complex Function::expZ(Complex z) {
	double x = z.real(), y = z.imag();
	return Complex(exp(x)*cos(y), exp(x)*sin(y));
}

void Function::init_functions(func_map& fmap) {
	fmap.emplace("sin", &sinZ);
	fmap.emplace("cos", &cosZ);
	fmap.emplace("tan", &tanZ);
	fmap.emplace("log", &logZ);
	fmap.emplace("exp", &expZ);
}

NodePtr Function::parse(Parser& p, NodePtr parent) {
	static func_map functions;
	if (functions.empty()) { init_functions(functions);	}

	if (!isalpha(p.peek()) ) return nullptr;

	for ( auto m : functions ) { 
		if (p.match(m.first + "(")) {
			NodePtr arg = NodePtr(new Expression(p));
			if (!arg) throw logic_error("bad format");

			NodePtr node = NodePtr(new Function(m.first, m.second, arg, parent));
			arg->setParent(node);
			return node;
		}
	}
	return nullptr;
}

void Function::xml_out(XML& xml) const 
{
	if (getSign()) 
		xml.header("function", false, {"name", m_name});
	else
		xml.header("function", false, {"name", m_name, "negative", "true"});
	m_arg->xml_out(xml);
	xml.footer("function");
}

void Function::calcTermSize() 
{
	m_arg->calcTermSize();
	setTermSize(m_name.length() + m_arg->getTermSizeX(), m_arg->getTermSizeY());
}

void Function::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	m_arg->calcTermOrig(x + m_name.length(), y);
}

void Function::asciiArt(DrawText & draw) const
{
	for (int x = 0; x < m_name.length(); ++x) {
		draw.at(getTermOrigX() + x, getTermOrigY() + getTermSizeY()/2, m_name[x]);
	}
	m_arg->asciiArt(draw);
}

NodePtr Binary::parse(Parser& p, NodePtr one, NodePtr parent)
{
	char c = p.peek();
	if (c != '/' && c != '^') return one;

	c = p.next();
	NodePtr two = Term::parse(p);
	if (!two) throw logic_error("bad format");

	NodePtr binary = ( c == '/' ) ? NodePtr(new Divide(one, two)) : 
		                            NodePtr(new Power(one, two));
	one->setParent(binary);
	two->setParent(binary);
	binary->setParent(parent);
	return binary;
}

void Binary::xml_out(const string& tag, XML& xml) const {
	if (getSign()) 
		xml.header(tag);
	else 
		xml.header(tag, false, { "negative", "true" });
	
	m_first->xml_out(xml);
	m_second->xml_out(xml);
	xml.footer(tag);
}

char Parser::next()
{
	if (m_pos < m_expr.length()) return m_expr[m_pos++];
	                        else return '\0';
}

bool Parser::match(const string& s)
{
	size_t pos = m_expr.find(s, m_pos); 
	if ( pos != string::npos && pos == m_pos ) {
		m_pos += s.length();
		return true;
	}
	else
		return false;
}

NodePtr Variable::parse(Parser& p, NodePtr parent)
{
	char c = p.peek();
	if ( isalpha(c) && c != 'i' && c != 'e' ) {
		return NodePtr(new Variable(p, parent));
	}
	else
		return nullptr;
}

void Variable::xml_out(XML& xml) const {
	string name = string("") + m_name;
	if (getSign()) 
		xml.header("variable", true, {"name", name});
	else
		xml.header("variable", true, {"name", name, "negative", "true"});
}

void Variable::calcTermSize() 
{
	setTermSize(1, 1);
}

void Variable::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
}

void Variable::asciiArt(DrawText& draw) const
{
	draw.at(getTermOrigX(), getTermOrigY(), m_name);
}

NodePtr Number::parse(Parser& p, NodePtr parent)
{
	char c = p.peek();
	if (isdigit(c) || c == 'i' ) {
		return NodePtr(new Number(p, parent));
	}
	else
		return nullptr;
}

string Number::getInteger(Parser& p)
{
	string n;
	while (isdigit(p.peek())) { n += p.next(); }
	return n;
}

double Number::getReal(Parser& p)
{
	string n = getInteger(p);
	if (p.peek() != '.' && p.peek() != 'E') {
		return stod(n);
	}
	char c = p.next();
	if (c == '.' && isdigit(p.peek())) {
		n += "." +  getInteger(p);
		if (p.peek() == 'E') c = p.next();
		                else c = '\0';
	}
	if (c == 'E') {
		n += 'E';
		if (p.peek() == '-' || p.peek() == '+') n += p.next();

		if (!isdigit(p.peek())) throw logic_error("bad format");
		n += getInteger(p);
	}
	m_isInteger = false;
	return stod(n);
}

void Number::getNumber(Parser& p)
{
	if (p.peek() == 'i') {
		char c = p.next();
		m_value = isdigit(p.peek()) ? Complex(0, getReal(p)) : Complex(0, 1);
	}
	else {
		m_value = Complex(getReal(p), 0);
	}
}

string Number::toString() const
{
	double realValue = abs(m_value.real());
	bool realNeg = signbit(m_value.real());
	double imagValue = abs(m_value.imag());
	bool imagNeg = signbit(m_value.imag());

	if (isZero(realValue) && isZero(imagValue)) return "0";

	string result = "";
	if (m_isInteger && !isZero(realValue)) 
		result += std::to_string((int) (realValue + 0.2));

	if (!m_isInteger && !isZero(realValue)) 
		result += std::to_string(realValue);

	if (m_isInteger && !isZero(imagValue)) {
		if (!isZero(realValue)) result += realNeg ? "-" : "+";
		result += "\033[31mi\033[30m" + std::to_string((int) (imagValue + 0.2));
	}
	if (!m_isInteger && !isZero(imagValue)) {
		if (!isZero(realValue)) result += realNeg ? "-" : "+";
		result += "\033[31mi\033[30m" + std::to_string(imagValue);
	}
	return result;
}

void Number::xml_out(XML& xml) const {
	xml.header("number", true, 
			   { "real", to_string(m_value.real()),
				 "imag", to_string(m_value.imag()),
				 "negative", (getSign() ? "false" : "true") });
					   
}

void Number::calcTermSize() 
{
	string n = toString();
	setTermSize(n.length(), 1);
}

void Number::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
}

void Number::asciiArt(DrawText& draw) const
{
	string n = toString();
	for (int x = 0; x < n.length(); ++x) {
		draw.at(getTermOrigX() + x, getTermOrigY() + getTermSizeY()/2, n[x]);
	}
}

NodePtr Term::parse(Parser& p, NodePtr parent)
{
	char c = p.peek();
	if ( c=='\0' || c=='+' || c=='-' || c==')' ) return false;

	NodePtr    node = Expression::parse(p, parent);
	if (!node) node = Function::parse(p, parent);
	if (!node) node = Variable::parse(p, parent);
	if (!node) node = Number::parse(p, parent);
	if (!node) return nullptr;

	node = Binary::parse(p, node, parent);
	return node;
}

bool Term::add(Parser& p)
{
	NodePtr node = parse(p);
	if (!node) return false;

	factors.push_back(node);
	return true;
}

void Term::xml_out(XML& xml) const {
	if (getSign()) 
		xml.header("term", false);
	else 
		xml.header("term", false, { "negative", "true" });

	for ( auto n : factors ) { n->xml_out(xml); }
	xml.footer("term");
}

void Term::calcTermSize() 
{
	int x = 0, y = 0;
	for ( auto n : factors ) { 
		n->calcTermSize(); 
		x += n->getTermSizeX();
		y = max(y, n->getTermSizeY());
	}
	setTermSize(x + 1, y);
}

void Term::calcTermOrig(int x, int y)
{
	setTermOrig(x, y); ++x;
	for ( auto n : factors ) {
		n->calcTermOrig(x, y + (getTermSizeY() - n->getTermSizeY())/2);
		x += n->getTermSizeX();
	}
}

void Term::asciiArt(DrawText& draw) const
{
	draw.at(getTermOrigX(), getTermOrigY() + getTermSizeY()/2, getSign() ? '+' : '-');
	for ( auto n : factors ) n->asciiArt(draw);
}

string Term::toString() const
{
	string s = "";
	for ( auto f : factors ) { 
		s += ( f->getSign() ? "" : "(-" ) + f->toString() + ( f->getSign() ? "" : ")" );
	}
	return s; 	
}

bool Expression::add(Parser& p)
{
	bool neg = false;
	if (p.peek() == '+' || p.peek() == '-') {
		char c = p.next();
		neg = (c == '-') ? true : false;
	}
	NodePtr term = NodePtr( new Term(p) );
	if (neg) term->negative();
	terms.push_back(term);
	
	if ( p.peek() == '\0' || p.peek() == ')' ) {
		char c = p.next();
		return false;
	}
	else
		return true;
}

NodePtr Expression::parse(Parser& p, NodePtr parent) {
	if (p.peek() != '(') return nullptr;
	char c = p.next();
	NodePtr node = NodePtr(new Expression(p, parent));
	return node;
}

void Expression::xml_out(XML& xml) const {
	if (getSign()) 
		xml.header("expression", false);
	else 
		xml.header("expression", false, { "negative", "true" });

	for ( auto n : terms ) { n->xml_out(xml); }
	xml.footer("expression");
}

void Expression::calcTermSize() 
{
	int x = 0, y = 0;
	if (getParent()) x += 1;
	for ( auto n : terms ) { 
		n->calcTermSize(); 
		x += n->getTermSizeX();
		y = max(y, n->getTermSizeY());
	}
	if (getParent()) x += 1;
	setTermSize(x, y);
}

void Expression::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	if (getParent()) x += 1;
	for ( auto n : terms ) {
		n->calcTermOrig(x, y + (getTermSizeY() - n->getTermSizeY())/2);
		x += n->getTermSizeX();
	}
}

void Expression::asciiArt(DrawText& draw) const
{
	if (getParent()) asciiArtParenthesis(draw);
	for ( auto n : terms ) n->asciiArt(draw); 
}

string Expression::toString() const
{ 
	string s = "(";
	for ( auto t : terms ) { 
		s += t->getSign() ? '+' : '-'; 
		s += t->toString();
	}
	return s += ")";
}

void Equation::asciiArt(ostream& os) const
{
	m_root->calcTermSize();
	m_root->calcTermOrig(0, 0);
	DrawString draw(m_root);
	m_root->asciiArt(draw);
	draw.out(os);
}


int main(int argc, char* argv[])
{
	Equation eqn(argv[1]);
	cout << eqn.toString() << endl;
	cout << "---------" << endl;
	{
		XML xml(cout);
		eqn.xml_out(xml);
	}
	cout << "---------" << endl;
	eqn.asciiArt(cout);
}
