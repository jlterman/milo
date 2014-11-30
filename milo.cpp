#include <iostream>
#include <vector>
#include <iterator>
#include <exception>
#include <map>
#include <limits>
#include <initializer_list>
#include "milo.h"

using namespace std;

class Parser
{
public:
	Parser(string expr, Equation& eqn) : m_expr(expr), m_eqn(eqn), m_pos(0) {}

	char peek() { return m_expr[m_pos]; }
	char next();
	bool match(const string& s);
	Equation& getEqn() { return m_eqn; }
private:
	Equation& m_eqn;
	string m_expr;
	size_t m_pos;
};

class XML
{
public:
    XML(std::ostream& os) : 
	    m_os(os), m_indent(1) { m_os << "<document>" << std::endl; }
	~XML() { m_os << "</document>" << std::endl; }

	void header(const std::string& tag, bool atomic, 
				std::initializer_list<std::string>);

	void header(const std::string& tag, bool atomic, const vector<string>& att);

	void header(const std::string& tag, bool atomic = false);

	void footer(const std::string& tag);
private:
	std::vector<std::string> m_tags;
	int m_indent;
	std::ostream& m_os;

	void print_indent(std::ostream& os) {
		for (int i = 0; i < m_indent; ++i) m_os << ' ';
	}
};

class XMLParser
{
public:
	enum { HEADER=1, FOOTER=2, ATOM=4, NAME=8, VALUE=16, END=32 };

	XMLParser(istream& in, Equation& eqn);
	~XMLParser();

	bool peek(const int state, const string& tag = string());
	string next();
	void next(string& name, string& value);

	bool next(const int state, const string& tag = string()) { 
		bool r = peek(state, tag); next(); return r;
	}
	bool getState(int state, const string& tag = string()) const;
	const string& getTag() const { return m_tags[m_pos]; }
	bool EOL() const { return m_pos == m_tags.size(); }
	const map<string, string> getAttributes();
	Equation& getEqn() const { return m_eqn; }
	
	static NodePtr parse(XMLParser& in, Node* parent = nullptr);
private:
	Equation& m_eqn;
	vector<string> m_tags;
	size_t m_pos;
	int m_state;

	void parse(istream& in);
    void peek(string& tag, int& state);
};

class Binary : public Node
{
public:
	Binary(char op, NodePtr one, NodePtr two, Node* parent) : 
		Node(parent), m_op(op), m_first(one), m_second(two) {}

	virtual ~Binary() {}

	string toString() const { return m_first->toString() + m_op + m_second->toString(); }

	static NodePtr parse(Parser& p, NodePtr one, Node* parent);
	static NodePtr xml_in(XMLParser& in, char op, const string& name, Node* parent);

	void xml_out(const string& tag, XML& xml) const;

protected:
	NodePtr m_first;
	NodePtr m_second;
	char m_op;
};

class Divide : public Binary
{
public:
	Divide(NodePtr one, NodePtr two, bool fNeg = false, Node* parent = nullptr) : 
		Binary('/', one, two, parent) { if (fNeg) negative(); }
	~Divide() {}

	void xml_out(XML& xml) const { Binary::xml_out("divide", xml); }
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;
};

class Power : public Binary
{
public:
	Power(NodePtr one, NodePtr two, bool fNeg = false, Node* parent = nullptr) : 
		Binary('^', one, two, parent) { if (fNeg) negative(); }
	~Power() {}

	void xml_out(XML& xml) const { Binary::xml_out("power", xml); }
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = map<string, func_ptr>;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

	string toString() const { return "\033[32m" + m_name + "\033[30m" + m_arg->toString(); }
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	Function(const string& name, func_ptr fp, NodePtr node, Node* parent, bool neg = false) : 
		Node(parent), m_name(name), m_func(fp), m_arg(node) { if (neg) negative(); }
	Function()=delete;
	~Function() {}
private:
	string m_name;
	func_ptr m_func;
	NodePtr m_arg;

	static func_map functions;

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
	Variable(Parser& p, Node* parent) : Node(parent), m_name( p.next() ) {}
	Variable(char name, Node* parent, bool neg) : 
		Node(parent), m_name(name) { if (neg) negative(); }
	~Variable() {}

	string toString() const { return string() + m_name; }
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

private:
	char m_name;
};

class Number : public Node
{
public:
	Number(Parser& p, Node* parent) : Node(parent), m_isInteger(true) { getNumber(p); }
	Number(string real, string imag, Node* parent, bool neg) : 
		Node(parent), m_value(Complex(stod(real), stod(imag))), m_isInteger(isZero(m_value))
	{ 
		if (neg) negative();
	}
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

private:
	string getInteger(Parser& p);

	Complex m_value;
	bool m_isInteger;
};

class Term : public Node
{
public:
	Term(Parser& p, Node* parent = nullptr) : Node(parent) { while(add(p)); }
	Term(XMLParser& in, Node* parent = nullptr);
	
	static NodePtr parse(Parser& p, Node* parent = nullptr);
	static NodePtr xml_in(XMLParser& in, Node* parent);

	string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

private:
	vector<NodePtr> factors;

	bool add(Parser& p);
	bool add(XMLParser& in);
};

class Expression : public Node
{
public:
	Expression(Parser& p, Node* parent = nullptr) : Node(parent) { while(add(p)); }
	Expression(XMLParser& in, Node* parent = nullptr);
	~Expression() {}

	string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	bool firstTerm(const NodePtr n) const { return (terms.size() > 1) && (terms[0] == n); }

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

private:
	vector<NodePtr> terms;	

	bool add(Parser& p);
	bool add(XMLParser& in);
};

void XML::header(const string& tag, bool atomic)
{
	print_indent(m_os);
	m_os << "<" << tag << (atomic ? "/>" : ">") << endl;
	if (!atomic) {
		m_indent += 1;
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
	m_indent += 1;
	m_tags.push_back(tag);
}

void XML::header(const std::string& tag, bool atomic, const vector<string>& children)
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
	m_indent += 1;
	m_tags.push_back(tag);
}


void XML::footer(const string& tag)
{
	string close_tag;
	do {
		close_tag = m_tags.back(); m_tags.pop_back();

		m_indent -= 1;
		print_indent(m_os);
		m_os << "</" << close_tag << ">" << endl;

	}
	while (close_tag.compare(tag) != 0);
}

void Equation::xml_out(XML& xml) const
{
	xml.header("equation");
	m_root->xml_out(xml);
	xml.footer("equation");
}

NodePtr Equation::xml_in(XMLParser& in)
{
	NodePtr root;
	if (in.next(XMLParser::HEADER|XMLParser::END, "equation")) {
		root = NodePtr(new Expression(in));
	}
	else
		throw logic_error("bad format");

	if (!in.next(XMLParser::FOOTER|XMLParser::END, "equation"))
		throw logic_error("bad format");
	return root;
}

Equation::Equation(string eq, Draw& draw) : m_draw(draw)
{ 
	Parser p(eq, *this); 
	m_root = NodePtr(new Expression(p));
}

Equation::Equation(istream& is, Draw& draw) : m_draw(draw) 
{
	XMLParser in(is, *this);
	m_root = xml_in(in);
}

void Equation::xml_out(ostream& os) const 
{
	XML xml(os);
	xml_out(xml);
}

void Equation::xml_out(string& str) const
{
	ostringstream os; 
	xml_out(os);
	str = os.str();
}

void Equation::asciiArt() const
{
	m_root->calcTermSize();
	m_root->calcTermOrig(0, 0);
	m_draw.set(m_root->getTermSizeX(), m_root->getTermSizeY());
	m_root->asciiArt(m_draw);
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
	setBaseLine(m_first->getTermSizeY());
}

void Divide::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	m_first->calcTermOrig(x + (getTermSizeX() - m_first->getTermSizeX())/2, y);
	m_second->calcTermOrig(x + (getTermSizeX() - m_second->getTermSizeX())/2, 
				 		   y + m_first->getTermSizeY() + 1);
}


void Divide::asciiArt(Draw& draw) const
{
	m_first->asciiArt(draw);
	for (int x = 0; x < getTermSizeX(); ++x) {
		draw.at(x + getTermOrigX(), getTermOrigY() + getBaseLine(), '-');
	}
	m_second->asciiArt(draw);
}

void Power::calcTermSize()
{
	m_first->calcTermSize();
	m_second->calcTermSize();
	setTermSize(m_first->getTermSizeX() + m_second->getTermSizeX(),
				m_first->getTermSizeY() + m_second->getTermSizeY());
	setBaseLine(m_second->getTermSizeY());
}

void Power::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	m_first->calcTermOrig(x, y + m_second->getTermSizeY());
	m_second->calcTermOrig(x + m_first->getTermSizeX(), y);
}

void Power::asciiArt(Draw& draw) const
{
	m_first->asciiArt(draw);
	m_second->asciiArt(draw);
}

Function::func_map Function::functions;

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

NodePtr Function::parse(Parser& p, Node* parent) {
	if (functions.empty()) { init_functions(functions);	}

	if (!isalpha(p.peek()) ) return nullptr;

	for ( auto m : functions ) { 
		if (p.match(m.first + "(")) {
			NodePtr arg = NodePtr(new Expression(p));
			if (!arg) throw logic_error("bad format");

			Node* node = new Function(m.first, m.second, arg, parent);
			arg->setParent(node);
			return NodePtr(node);
		}
	}
	return nullptr;
}

NodePtr Function::xml_in(XMLParser& in, Node* parent)
{
	if (functions.empty()) { init_functions(functions);	}

	if (!in.peek(XMLParser::HEADER, "function")) return nullptr;
	if (in.next(XMLParser::END, "function")) throw logic_error("bad format");
	
	bool fNeg = false;
	string fname;
	func_ptr fp = nullptr;

	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		else if (m.first.compare("name") == 0) {
			for ( auto f : functions ) {
				if (f.first.compare(m.second) == 0) {
					fname = f.first;
					fp = f.second;
				}
			}
			if (fname.empty() || fp == nullptr) throw logic_error("bad format");
		}
		else
			throw logic_error("bad format");
	}
	NodePtr arg = NodePtr(new Expression(in));
	
	if (!in.next(XMLParser::FOOTER|XMLParser::END, "function"))
		throw logic_error("bad format");

	Node* node = new Function(fname, fp, arg, parent, fNeg);
	arg->setParent(node);
	return NodePtr(node);
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
	setBaseLine(m_arg->getTermSizeY()/2);
}

void Function::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	m_arg->calcTermOrig(x + m_name.length(), y);
}

void Function::asciiArt(Draw & draw) const
{
	draw.at(getTermOrigX(), getTermOrigY() + getBaseLine(), m_name);
	m_arg->asciiArt(draw);
}

NodePtr Binary::parse(Parser& p, NodePtr one, Node* parent)
{
	char c = p.peek();
	if (c != '/' && c != '^') return one;

	c = p.next();
	NodePtr two = Term::parse(p);
	if (!two) throw logic_error("bad format");

	Node* binary;
	if ( c == '/' ) 
		binary = new Divide(one, two); 
	else 
		binary = new Power(one, two);

	one->setParent(binary);
	two->setParent(binary);
	binary->setParent(parent);
	return NodePtr(binary);
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

NodePtr Binary::xml_in(XMLParser& in, char op, const string& name, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, name)) return nullptr;
	in.next();
	bool fNeg = false;
	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		else
			throw logic_error("bad format");
	}

	NodePtr one = XMLParser::parse(in);
	NodePtr two = XMLParser::parse(in);
	
	if (!in.next(XMLParser::FOOTER|XMLParser::END, name))
		throw logic_error("bad format");

	Node* binary = nullptr;
	if (name.compare("divide") == 0) 
		binary = new Divide(one, two, fNeg);
	else if (name.compare("power") == 0)
		binary = new  Power(one, two, fNeg);
	else
		throw logic_error("bad format");

	one->setParent(binary);
	two->setParent(binary);
	binary->setParent(parent);
	return NodePtr(binary);
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

XMLParser::XMLParser(istream& in, Equation& eqn) : m_pos(0), m_eqn(eqn) 
{ 
	parse(in);  
	if (!next(HEADER|END, "document")) throw logic_error("bad format");
}

XMLParser::~XMLParser()
{
	if (!next(FOOTER|END, "document")) throw logic_error("bad format");
}

const map<string, string> XMLParser::getAttributes()
{
	map<string, string> attributes;
	if (getState(END)) return attributes;

	do {
		string name, value;
		next(name, value);
		attributes.emplace(name, value);
	}
	while (!peek(END));
	next();
	return attributes;
}

void XMLParser::peek(string& tag, int& state)
{
	tag = m_tags[m_pos];
	if (tag.find("</", 0) == 0) {
		tag.erase(0, 2);
		tag.erase(tag.find('>'));
		state = FOOTER|END;
		return;
	}
	if (tag.front() == '<') {
		tag.erase(tag.begin());
		size_t pos = tag.rfind("/>");
		if (pos != string::npos) {
			tag.erase(pos);
			state = HEADER|ATOM|END;
			return;
		}
		if (tag.back() == '>') {
			tag.erase(tag.end() - 1);
			state = HEADER|END;
			return;
		}
		state = HEADER;
		return;
	}
	if (tag.compare(">") == 0) {
		state = END;
		return;
	}
	if (tag.compare("/>") == 0) {
		state = ATOM|END;
		return;
	}
	if (tag.find("=\"", 0) == 0) {
		state = VALUE;
		tag.erase(0, 2);
		tag.erase(tag.end() - 1);
		return;
	}
	state = NAME;
	return;
}

bool XMLParser::peek(const int state, const string& tag)
{
	string p_tag;
	int p_state;
	peek(p_tag, p_state);
	return (tag.empty() || tag.compare(p_tag) == 0) && (state&p_state);
}

bool XMLParser::getState(int state, const string& tag) const
{
	return (tag.empty() || tag.compare(m_tags[m_pos]) == 0) && (state&m_state);	
}

string XMLParser::next()
{
	if (EOL()) throw range_error("xml exhausted");

	string tag;
	peek(tag, m_state);
	++m_pos;

#if 0
	cout << "XML read in: " << tag;
	if (m_state&HEADER) cout << " HEADER";
	if (m_state&FOOTER) cout << " FOOTER";
	if (m_state&ATOM) cout << " ATOM";
	if (m_state&NAME) cout << " NAME";
	if (m_state&VALUE) cout << " VALUE";
	if (m_state&END) cout << " END";
	cout << endl;
#endif

	return tag;
}
						  
void XMLParser::next(string& name, string& value)
{
	name = next();
	if (!getState(XMLParser::NAME)) throw logic_error("bad format");
	value = next();
	if (!getState(XMLParser::VALUE)) throw logic_error("bad format");
}

void XMLParser::parse(istream& in)
{
	string tag;
	string end;
	while (in >> tag) {
		size_t pos = tag.find("=\"", 0);
		if (pos != string::npos) {
			end.clear();
			if (tag.back() == '>') {
				end = (tag.rfind("/>") != string::npos) ? "/>" : ">";
				tag.erase(tag.find(end));
			}
			m_tags.push_back(tag.substr(0, pos));
			m_tags.push_back(tag.substr(pos));

			if (!end.empty()) m_tags.push_back(end);
		}
		else
			m_tags.push_back(tag);
	}
}

NodePtr XMLParser::parse(XMLParser& in, Node* parent)
{
	NodePtr    node = Expression::xml_in(in, parent);
	if (!node) node = Function::xml_in(in, parent);
	if (!node) node = Variable::xml_in(in, parent);
	if (!node) node = Number::xml_in(in, parent);
	if (!node) node = Input::xml_in(in, parent);
	if (!node) node = Binary::xml_in(in, '/', "divide", parent);
	if (!node) node = Binary::xml_in(in, '^', "power", parent);

	return node;
}

NodePtr Variable::parse(Parser& p, Node* parent)
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

NodePtr Variable::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "variable")) return nullptr;
	if (in.next(XMLParser::END, "variable")) throw logic_error("bad format");

	bool fNeg = false;
	char var_name;
	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		else if (m.first.compare("name") == 0) {
			if (m.second.empty()) throw logic_error("bad format");
			var_name = m.second[0];
		}
		else
			throw logic_error("bad format");
	}
	if (!in.getState(XMLParser::ATOM|XMLParser::END)) throw logic_error("bad format");
	return NodePtr(new Variable(var_name, parent, fNeg));
}

void Variable::calcTermSize() 
{
	setTermSize(1, 1);
	setBaseLine(0);
}

void Variable::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
}

void Variable::asciiArt(Draw& draw) const
{
	draw.at(getTermOrigX(), getTermOrigY(), m_name);
}

NodePtr Number::parse(Parser& p, Node* parent)
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

NodePtr Number::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "number")) return nullptr;
	if (in.next(XMLParser::END, "number")) throw logic_error("bad format");
	
	bool fNeg = false;
	string real = "0", imag = "0";
	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		else if (m.first.compare("real") == 0) {
			real = m.second;
		}
		else if (m.first.compare("imag") == 0) {
			imag = m.second;
		}
		else
			throw logic_error("bad format");
	}
	if (!in.getState(XMLParser::ATOM|XMLParser::END))  throw logic_error("bad format");
	return NodePtr(new Number(real, imag, parent, fNeg));
}


void Number::calcTermSize() 
{
	string n = toString();
	setTermSize(n.length(), 1);
	setBaseLine(0);
}

void Number::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
}

void Number::asciiArt(Draw& draw) const
{
	draw.at(getTermOrigX(), getTermOrigY(), toString());
}

Term::Term(XMLParser& in, Node* parent) : Node(parent)
{
	if (!in.next(XMLParser::HEADER, "term")) throw logic_error("bad format");

	bool fNeg = false;
	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		else
			throw logic_error("bad format");
	}
	if (fNeg) negative();
	while (add(in));

	if (!in.next(XMLParser::FOOTER|XMLParser::END, "term")) throw logic_error("bad format");
}

NodePtr Term::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "term")) return nullptr;

	return NodePtr(new Term(in, parent));
}

bool Term::add(XMLParser& in)
{
	NodePtr node = XMLParser::parse(in, nullptr);
	if (!node) return false;

	factors.push_back(node);
	return true;
}

NodePtr Term::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( c=='\0' || c=='+' || c=='-' || c==')' ) return false;

	NodePtr    node = Expression::parse(p, parent);
	if (!node) node = Function::parse(p, parent);
	if (!node) node = Variable::parse(p, parent);
	if (!node) node = Number::parse(p, parent);
	if (!node) node = Input::parse(p, parent);
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
	int x = 0, b = 0, y = 0;
	for ( auto n : factors ) { 
		n->calcTermSize(); 
		x += n->getTermSizeX();
		y = max(y, n->getTermSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	setTermSize(x, b + y);
	setBaseLine(b);
}

void Term::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	for ( auto n : factors ) {
		n->calcTermOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getTermSizeX();
	}
}

void Term::asciiArt(Draw& draw) const
{
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

Expression::Expression(XMLParser& in, Node* parent) : Node(parent)
{
	if (!in.next(XMLParser::HEADER, "expression")) throw logic_error("bad format");

	bool fNeg = false;
	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		else
			throw logic_error("bad format");
	}
	if (fNeg) negative();
	while (add(in));

	if (!in.next(XMLParser::FOOTER|XMLParser::END, "expression")) throw logic_error("bad format");
}

NodePtr Expression::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "expression")) return nullptr;

	return NodePtr(new Expression(in, parent));
}

bool Expression::add(XMLParser& in)
{
	if (!in.peek(XMLParser::HEADER, "term")) return false;
	
	NodePtr node = NodePtr( new Term(in, this) );
	terms.push_back(node);
	return true;
}

bool Expression::add(Parser& p)
{
	bool neg = false;
	if (p.peek() == '+' || p.peek() == '-') {
		char c = p.next();
		neg = (c == '-') ? true : false;
	}
	NodePtr term = NodePtr( new Term(p, this) );
	if (neg) term->negative();
	terms.push_back(term);
	
	if ( p.peek() == '\0' || p.peek() == ')' ) {
		char c = p.next();
		return false;
	}
	else
		return true;
}

NodePtr Expression::parse(Parser& p, Node* parent) {
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
	int x = 0, b = 0, y = 0;
	if (getParent()) x += 1;
	for ( auto n : terms ) { 
		if (n != terms[0] || !n->getSign()) x += 1;
		n->calcTermSize(); 
		x += n->getTermSizeX();
		y = max(y, n->getTermSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	if (getParent()) x += 1;
	setTermSize(x, y + b);
	setBaseLine(b);
}

void Expression::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	if (getParent()) x += 1;
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) x+=1;
		n->calcTermOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getTermSizeX();
	}
}

void Expression::asciiArt(Draw& draw) const
{
	if (getParent()) draw.parenthesis(this);
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) draw.at(n->getTermOrigX() - 1, 
													n->getTermOrigY() + n->getTermSizeY()/2,
													n->getSign() ? '+' : '-');
		n->asciiArt(draw); 
	}
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

int Input::input_sn = -1;

Input::Input(Parser& p, Node* parent) : 
	Node(parent), m_sn(++input_sn), m_active(true), m_current(false)
{
	p.getEqn().addInput(this);
	char c = p.next();
	if (c == '#') p.getEqn().setCurrentInput(m_sn);
}

Input::Input(Equation& eqn, std::string txt, Node* parent, bool neg, bool current) :
	Node(parent), m_typed(txt), m_sn(++input_sn), m_active(true), m_current(current)
{
	eqn.addInput(this);
	if (current) eqn.setCurrentInput(m_sn);
}

void Input::xml_out(XML& xml) const
{
	vector<string> attributes;
	if (m_typed.length() > 0) {
		attributes.emplace_back("text");
		attributes.push_back(m_typed);
	}
	if (m_current) {
		attributes.emplace_back("current");
		attributes.emplace_back("true");
	}
	if (getSign()) {
		attributes.emplace_back("negative");
		attributes.emplace_back("true");
	}

	if (attributes.size() == 0)
		xml.header("input", true);
	else
		xml.header("input", true, attributes);
}

NodePtr Input::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "input")) return nullptr;
	if (in.next(XMLParser::END, "input")) throw logic_error("bad format");

	bool fNeg = false;
	bool fCurrent = false;
	string text;
	for ( auto m : in.getAttributes() ) {
		if (m.first.compare("negative") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fNeg = m.second.compare("true") == 0;
		}
		if (m.first.compare("current") == 0) {
			if ((m.second.compare("true") && m.second.compare("value")))
				throw logic_error("bad format");
			fCurrent = m.second.compare("true") == 0;
		}
		else if (m.first.compare("text") == 0) {
			if (m.second.empty()) throw logic_error("bad format");
			text = m.second;
		}
		else
			throw logic_error("bad format");
	}
	if (!in.getState(XMLParser::ATOM|XMLParser::END)) throw logic_error("bad format");
	return NodePtr(new Input(in.getEqn(), text, parent, fNeg, fCurrent));
}

void Input::calcTermSize()
{
	setTermSize(m_typed.length() + 1, 1);
	setBaseLine(0);	
}

void Input::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
}

void Input::asciiArt(Draw& draw) const
{
	draw.at(getTermOrigX(), getTermOrigY(), toString());
}

string Input::toString() const
{
	if (m_active)
		return m_typed + (m_current ? '#' : '?');
	else
		return m_typed;
}

void Input::handleChar(char ch)
{
	if ( ch == '/' ) {
		m_typed = "(" + m_typed + ")/";
		return;
	}
	else if ( ch == '(' ) {
		m_typed = m_typed + "(#)";
		disable();
	}
}

NodePtr Input::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( c == '?' || c == '#' ) {
		return NodePtr(new Input(p, parent));
	}
	else
		return nullptr;
}
