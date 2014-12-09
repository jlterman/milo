#include <iostream>
#include <vector>
#include <iterator>
#include <exception>
#include <map>
#include <limits>
#include <initializer_list>
#include "milo.h"
#include "nodes.h"

using namespace std;

class DrawNull : public Draw
{
public:
	void at(int x, int y, char c) {}
	void at(int x, int y, const string& s) {}
	void out() {}
	void set(int x, int y, int x0 = 0, int y0 = 0) {}
	void parenthesis(int x_size, int y_size, int x0, int y0) {}
};


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
    XML(ostream& os) : 
	    m_os(os), m_indent(1) { m_os << "<document>" << endl; }
	~XML() { m_os << "</document>" << endl; }

	void header(const Node* node, const string& tag, bool atomic, initializer_list<string>);

	void header(const Node* node, const string& tag, bool atomic, const vector<string>& att);

	void header(const Node* node, const string& tag, bool atomic = false);

	void footer(const string& tag);
private:
	vector<string> m_tags;
	int m_indent;
	ostream& m_os;

	void print_indent() {
		for (int i = 0; i < m_indent; ++i) m_os << ' ';
	}
	void header_start(const Node* node, const string& tag);
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
	const map<string, string> getAttributes(bool& fNeg, Node::Select& select);
	Equation& getEqn() const { return m_eqn; }
	
	static Node* parse(XMLParser& in, Node* parent = nullptr);
private:
	Equation& m_eqn;
	vector<string> m_tags;
	size_t m_pos;
	int m_state;

	void parse(istream& in);
    void peek(string& tag, int& state);
};

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

void XML::header_start(const Node* node, const string& tag)
{
	m_os << "<" << tag;
	if (!node->getSign()) m_os << " negative=\"true\"";
	if (node->getSelect()) m_os << " select=\"" << Node::select_tags[node->getSelect()] << "\"";
}

void XML::header(const Node* node, const string& tag, bool atomic)
{
	print_indent();
	header_start(node, tag);
	m_os << (atomic ? "/>" : ">") << endl;
	if (!atomic) {
		m_indent += 1;
		m_tags.push_back(tag);
	}
}

void XML::header(const Node* node, const string& tag, bool atomic, initializer_list<string> children)
{
	print_indent();
	header_start(node, tag);
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

void XML::header(const Node* node, const std::string& tag, bool atomic, const vector<string>& children)
{
	print_indent();
	header_start(node, tag);
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
		print_indent();
		m_os << "</" << close_tag << ">" << endl;

	}
	while (close_tag.compare(tag) != 0);
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

const map<string, string> XMLParser::getAttributes(bool& fNeg, Node::Select& select)
{
	map<string, string> attributes;
	fNeg = false;
	select = Node::Select::NONE;

	if (getState(END)) return attributes;

	do {
		string name, value;
		next(name, value);
		
		if (name.compare("negative") == 0) {
			if (value.compare("true") && value.compare("false"))
				throw logic_error("bad format");
			fNeg = (value.compare("true") == 0);
		}
		else if (name.compare("select") == 0) {
			auto pos = find(Node::select_tags, value);
			if (pos == Node::select_tags.end()) throw logic_error("bad format");
			select = (Node::Select) distance(Node::select_tags.begin(), pos);
		}
		else 
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
	string log = "XML read in: " + tag;
	if (m_state&HEADER) log += " HEADER";
	if (m_state&FOOTER) log += " FOOTER";
	if (m_state&ATOM) log += " ATOM";
	if (m_state&NAME) log += " NAME";
	if (m_state&VALUE) log += " VALUE";
	if (m_state&END) log += " END";
	LOG_TRACE_MSG(log);
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

Node* XMLParser::parse(XMLParser& in, Node* parent)
{
	Node*    node = Expression::xml_in(in, parent);
	if (!node) node = Function::xml_in(in, parent);
	if (!node) node = Constant::xml_in(in, parent);
	if (!node) node = Variable::xml_in(in, parent);
	if (!node) node = Number::xml_in(in, parent);
	if (!node) node = Binary::xml_in(in, '/', "divide", parent);
	if (!node) node = Binary::xml_in(in, '^', "power", parent);
	if (!node) return nullptr;

	switch (node->getSelect()) {
	    case Node::Select::START: in.m_eqn.setSelectStart(node); break;
	    case Node::Select::END  : in.m_eqn.setSelectEnd(node);   break;
	    case Node::Select::ALL  : in.m_eqn.setSelectStart(node);
                     		      in.m_eqn.setSelectEnd(node);
						          break;
	    case Node::Select::NONE:
	                    default: break;
	}
	return node;
}

Node* Equation::xml_in(XMLParser& in)
{
	Node* root;
	if (in.next(XMLParser::HEADER|XMLParser::END, "equation")) {
		root = new Expression(in);
	}
	else
		throw logic_error("bad format");

	if (!in.next(XMLParser::FOOTER|XMLParser::END, "equation"))
		throw logic_error("bad format");
	return root;
}

Equation::Equation(istream& is)
{
	XMLParser in(is, *this);
	m_root = xml_in(in);
}

Node* Function::xml_in(XMLParser& in, Node* parent)
{
	if (functions.empty()) { init_functions();	}

	if (!in.peek(XMLParser::HEADER, "function")) return nullptr;
	if (in.next(XMLParser::END, "function")) throw logic_error("bad format");
	
	bool fNeg = false;
	Node::Select select;
	string fname;
	func_ptr fp = nullptr;

	for ( auto m : in.getAttributes(fNeg, select) ) {
		if (m.first.compare("name") == 0) {
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
	Node* arg = new Expression(in);
	
	if (!in.next(XMLParser::FOOTER|XMLParser::END, "function"))
		throw logic_error("bad format");

	return new Function(fname, fp, arg, parent, fNeg, select);
}

Node* Binary::xml_in(XMLParser& in, char op, const string& name, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, name)) return nullptr;
	in.next();
	bool fNeg = false;
	Node::Select select;
	if (!in.getAttributes(fNeg, select).empty()) throw logic_error("bad format");

	Node* one = XMLParser::parse(in);
	Node* two = XMLParser::parse(in);
	
	if (!in.next(XMLParser::FOOTER|XMLParser::END, name))
		throw logic_error("bad format");

	if (name.compare("divide") == 0) 
		return new Divide(one, two, parent, fNeg, select);
	else if (name.compare("power") == 0)
		return new Power(one, two, parent, fNeg, select);
	else
		throw logic_error("bad format");
}

Node* Variable::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "variable")) return nullptr;
	if (in.next(XMLParser::END, "variable")) throw logic_error("bad format");

	bool fNeg = false;
	Node::Select select;
	char var_name;
	for ( auto m : in.getAttributes(fNeg, select) ) {
		if (m.first.compare("name") == 0) {
			if (m.second.empty()) throw logic_error("bad format");
			var_name = m.second[0];
		}
		else
			throw logic_error("bad format");
	}
	if (!in.getState(XMLParser::ATOM|XMLParser::END)) throw logic_error("bad format");
	return new Variable(var_name, parent, fNeg, select);
}

Node* Constant::xml_in(XMLParser& in, Node* parent)
{
	if (constants.empty()) { init_constants();	}

	if (!in.peek(XMLParser::HEADER, "constant")) return nullptr;
	if (in.next(XMLParser::END, "constant")) throw logic_error("bad format");
	
	char cname;
	Complex value;

	bool fNeg = false;
	Node::Select select;
	for ( auto m : in.getAttributes(fNeg, select) ) {
		if (m.first.compare("name") == 0 && m.second.length() == 1) {
			auto it = constants.find(m.second[0]);
			if (it != constants.end()) {
				cname = m.second[0];
				value = it->second;
			}
			if (!cname || isZero(value)) throw logic_error("bad format");
		}
		else
			throw logic_error("bad format");
	}
	if (!in.getState(XMLParser::ATOM|XMLParser::END))  throw logic_error("bad format");
	return new Constant(cname, value, parent, fNeg, select);
}

Node* Number::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "number")) return nullptr;
	if (in.next(XMLParser::END, "number")) throw logic_error("bad format");
	
	bool fNeg = false;
	Node::Select select;
	string real = "0", imag = "0";
	for ( auto m : in.getAttributes(fNeg, select) ) {
		if (m.first.compare("real") == 0) {
			real = m.second;
		}
		else if (m.first.compare("imag") == 0) {
			imag = m.second;
		}
		else
			throw logic_error("bad format");
	}
	if (!in.getState(XMLParser::ATOM|XMLParser::END))  throw logic_error("bad format");
	return new Number(real, imag, parent, fNeg, select);
}

Node* Input::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "input")) return nullptr;
	in.next();

	bool fNeg = false;
	Node::Select select;
	bool fCurrent = false;
	string text;
	for ( auto m : in.getAttributes(fNeg, select) ) {
		if (m.first.compare("current") == 0) {
			if ((m.second.compare("true") && m.second.compare("false")))
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
	return new Input(in.getEqn(), text, fCurrent, parent, fNeg, select);
}

Term::Term(XMLParser& in, Node* parent) : Node(parent)
{
	if (!in.next(XMLParser::HEADER, "term")) throw logic_error("bad format");

	bool fNeg = false;
	Node::Select select;
	if ( !in.getAttributes(fNeg, select).empty() ) throw logic_error("bad format");
	if (fNeg) negative();
	if (select) setSelect(select);
	while (add(in));

	if (!in.next(XMLParser::FOOTER|XMLParser::END, "term")) throw logic_error("bad format");
}

Node* Term::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "term")) return nullptr;

	return new Term(in, parent);
}

bool Term::add(XMLParser& in)
{
	Node* node = XMLParser::parse(in, this);
	if (!node) return false;

	factors.push_back(node);
	return true;
}

bool Expression::add(XMLParser& in)
{
	Node* node = nullptr;
	if (in.peek(XMLParser::HEADER, "term")) {
		node = new Term(in, this);
	}
	else if (in.peek(XMLParser::HEADER, "input")) {
		node = Input::xml_in(in, this);
	}
	if (!node) return false;

	terms.push_back(node);
	return true;
}

Node* Expression::getTerm(Parser& p, Node* parent)
{
	Node* node = nullptr;
	bool neg = false;
	if (p.peek() == '+' || p.peek() == '-') {
		char c = p.next();
		neg = (c == '-') ? true : false;
	}
	node = Input::parse(p, parent);
	if (!node) node = new Term(p, parent);

	if (neg) node->negative();
	return node;
}

bool Expression::add(Parser& p)
{
	Node* node = getTerm(p, this);
	terms.push_back(node);
	
	if ( p.peek() == '\0' || p.peek() == ')' ) {
		char c = p.next();
		return false;
	}
	else
		return true;
}

Node* Expression::parse(Parser& p, Node* parent) {
	if (p.peek() != '(') return nullptr;
	
	Node* node = nullptr;
	if ( (node = Input::parse(p, parent)) ) return node;
	char c = p.next();
	return new Expression(p, parent);
}

Expression::Expression(XMLParser& in, Node* parent) : Node(parent)
{
	if (!in.next(XMLParser::HEADER, "expression")) throw logic_error("bad format");

	bool fNeg = false;
	Node::Select select;
	if ( !in.getAttributes(fNeg, select).empty() ) throw logic_error("bad format");
	if (fNeg) negative();
	if (select) setSelect(select);
	while (add(in));

	if (!in.next(XMLParser::FOOTER|XMLParser::END, "expression")) throw logic_error("bad format");
}

Node* Expression::xml_in(XMLParser& in, Node* parent)
{
	if (!in.peek(XMLParser::HEADER, "expression")) return nullptr;

	return new Expression(in, parent);
}

void Equation::xml_out(XML& xml) const
{
	xml.header(m_root, "equation");
	m_root->xml_out(xml);
	xml.footer("equation");
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

void Function::xml_out(XML& xml) const 
{
	xml.header(this, "function", false, {"name", m_name});
	m_arg->xml_out(xml);
	xml.footer("function");
}

void Binary::xml_out(const string& tag, XML& xml) const {
	xml.header(this, tag);
	m_first->xml_out(xml);
	m_second->xml_out(xml);
	xml.footer(tag);
}

void Variable::xml_out(XML& xml) const {
	xml.header(this, "variable", true, { "name", string(1, m_name) });
}

void Constant::xml_out(XML& xml) const {
	xml.header(this, "constant", true, { "name", string(1, m_name) });
}

void Number::xml_out(XML& xml) const {
	xml.header(this, "number", true, 
			   { "real", toString(m_value.real(), true), 
				 "imag", toString(m_value.imag(), true)
			   });
}

void Input::xml_out(XML& xml) const
{
	bool fAllAlphaNum = true;
	for ( char c : m_typed ) {  fAllAlphaNum *= (isalnum(c) || c =='.'); }
	if (fAllAlphaNum && m_active) {
		vector<string> attributes;
		if (m_typed.length() > 0) {
			attributes.emplace_back("text");
			attributes.push_back(m_typed);
		}
		if (m_current) {
			attributes.emplace_back("current");
			attributes.emplace_back("true");
		}		
		xml.header(this, "input", true, attributes);
	}
	else {
		Equation eqn("?");
		Parser p(m_typed, eqn);
		Node* term = Expression::getTerm(p, nullptr);
		if (getSelect() != Node::Select::NONE) 
			term->end()->setSelect(getSelect());
 		term->xml_out(xml);
	}
}

void Term::xml_out(XML& xml) const {
	xml.header(this, "term", false);
	for ( auto n : factors ) { n->xml_out(xml); }
	xml.footer("term");
}

void Expression::xml_out(XML& xml) const {
	xml.header(this, "expression", false);
	for ( auto n : terms ) { n->xml_out(xml); }
	xml.footer("expression");
}

Equation::Equation(string eq)
{ 
	Parser p(eq, *this); 
	m_root = new Expression(p);
}

Node* Function::parse(Parser& p, Node* parent) {
	if (functions.empty()) { init_functions();	}

	if (!isalpha(p.peek()) ) return nullptr;

	for ( auto m : functions ) { 
		if (p.match(m.first + "(")) {
			Node* arg = new Expression(p, parent);
			if (!arg) throw logic_error("bad format");

			return new Function(m.first, m.second, arg, parent);
		}
	}
	return nullptr;
}

Node* Binary::parse(Parser& p, Node* one, Node* parent)
{
	char c = p.peek();
	if (c != '/' && c != '^') return one;

	c = p.next();
	Node* two = Term::parse(p);
	if (!two) throw logic_error("bad format");

	Node* binary;
	if ( c == '/' ) 
		return new Divide(one, two, parent); 
	else if ( c == '^' ) 
		return new Power(one, two, parent);
	else
		return nullptr;
}
	
Variable::Variable(Parser& p, Node* parent) : Node(parent), m_name( p.next() ) {}

Node* Variable::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( isalpha(c) ) {
		return new Variable(p, parent);
	}
	else
		return nullptr;
}

Constant::Constant(Parser& p, Node* parent) : 
	Node(parent), m_name(p.next()), m_value(constants.find(m_name)->second) {}

Node* Constant::parse(Parser& p, Node* parent)
{
	if (constants.empty()) init_constants();

	char c = p.peek();
	if ( constants.find(c) != constants.end() ) {
		return new Constant(p, parent);
	}
	else
		return nullptr;			
}

Node* Number::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if (isdigit(c) || c == 'i' ) {
		return new Number(p, parent);
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

Node* Term::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( c=='\0' || c=='+' || c=='-' || c==')' ) return nullptr;

	Node*    node = Expression::parse(p, parent);
	if (!node) node = Function::parse(p, parent);
	if (!node) node = Constant::parse(p, parent);
	if (!node) node = Number::parse(p, parent);
	if (!node) node = Variable::parse(p, parent);
	if (!node) node = Input::parse(p, parent);
	if (!node) return nullptr;

	node = Binary::parse(p, node, parent);
	return node;
}

bool Term::add(Parser& p)
{
	Node* node = parse(p, this);
	if (!node) return false;

	factors.push_back(node);
	return true;
}

Input::Input(Parser& p, Node* parent) : 
	Node(parent), m_sn(++input_sn), m_active(true), m_current(false), m_typed(""), m_eqn(p.getEqn())
{
	p.getEqn().addInput(this);
	char c = p.next();
	if (c == '?') return;
	if (c == '[') {
		while ( (c = p.next()) != ']' ) { m_typed += c;	}
	}
	p.getEqn().setCurrentInput(m_sn);
}

Node* Input::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( c == '?' || c == '#' || c == '[' ) {
		return new Input(p, parent);
	}
	else
		return nullptr;
}
