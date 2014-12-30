#include <iostream>
#include <vector>
#include <iterator>
#include <map>
#include <limits>
#include <initializer_list>
#include "milo.h"
#include "nodes.h"

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
    XML(ostream& os) : 
	    m_os(os), m_indent(1) { m_os << "<document>" << endl; }
	~XML() { m_os << "</document>" << endl; }

	void header(const Node* node, const string& tag, bool atomic, initializer_list<string>);

	void header(const Node* node, const string& tag, bool atomic, const vector<string>& att);

	void header(const Node* node, const string& tag, bool atomic = false);

	void header(const string& tag) { 
		print_indent(); m_os << "<" + tag + ">" << endl; m_indent += 1; m_tags.push_back(tag);
	}

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
	enum { CLEAR=0, HEADER=1, FOOTER=2, ATOM=4 };
	typedef Node* (*createPtr)(XMLParser&, Node*);

	XMLParser(istream& in, Equation& eqn);
	~XMLParser();

	void next();
	bool next(int state, const string& tag = string()) { next(); return getState(state, tag); }
	bool getState(int state, const string& tag = string()) const;
	const string& getTag() const { return m_tag; }
	bool getAttribute(const string& name, string& value);
	bool hasAttributes() { return m_attributes.size() != 0; }
	Equation& getEqn() const { return m_eqn; }
	Node* getFactor(Node* parent);
	
private:
	Equation& m_eqn;
	vector<string> m_tags;
	size_t m_pos;
	map<string, string> m_attributes;
	int m_state;
	string m_tag;

	static const map<string, createPtr> create_factors;

	bool EOL() const { return m_pos == m_tags.size(); }
	void parse(istream& in);
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
	if (!next(HEADER, "document")) throw logic_error("bad format");
}

XMLParser::~XMLParser()
{
	if (!next(FOOTER, "document")) throw logic_error("bad format");
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

void XMLParser::next()
{
	if (EOL()) throw logic_error("out of xml");

	m_tag = m_tags[m_pos++];
	m_attributes.clear();
	m_state = CLEAR;
	if (m_tag.find("</", 0) == 0) {
		m_tag.erase(0, 2);
		m_tag.erase(m_tag.find('>'));
		m_state = FOOTER;
		return;
	}
	if (m_tag.front() == '<') {
		m_tag.erase(m_tag.begin());
		size_t pos = m_tag.rfind("/>");
		if (pos != string::npos) {
			m_tag.erase(pos);
			m_state = HEADER|ATOM;
			return;
		}
		if (m_tag.back() == '>') {
			m_tag.erase(m_tag.end() - 1);
			m_state = HEADER;
			return;
		}
	}
	m_state = HEADER;
	while (m_tags[m_pos].rfind(">") == string::npos) {
		if (m_tags[m_pos].find("=\"", 0) == 0) throw logic_error("bad format");
		string name = m_tags[m_pos];
		++m_pos;
		if (m_tags[m_pos].find("=\"", 0) != 0) throw logic_error("bad format");
		string value = m_tags[m_pos].substr(2, m_tags[m_pos].size() - 3);
		m_attributes.emplace(name, value);
		++m_pos;
	}
	if (m_tags[m_pos++].compare("/>") == 0) m_state |= ATOM;
	return;
}

bool XMLParser::getState(int state, const string& tag) const
{
	return ((state&m_state) != 0) && (tag.empty() || (tag.compare(m_tag) == 0));
}

bool XMLParser::getAttribute(const string& name, string& value)
{
	if (m_attributes.find(name) == m_attributes.end()) return false;

	value = m_attributes.at(name);
	m_attributes.erase(name);
	return true;
}

Node* XMLParser::getFactor(Node* parent)
{
	next();
	if (create_factors.find(m_tag) == create_factors.end()) return nullptr;
	auto cp = create_factors.at(m_tag);
	return cp(*this, parent);
}

Node::Node(XMLParser& in, Node* parent, const string& name) : 
	m_parent(parent), m_sign(true), m_select(NONE)
{
	if (!in.getState(XMLParser::HEADER, name)) 
		throw logic_error("bad header: expected: " + name + ", got: " + in.getTag());

	string value;
	if (in.getAttribute("negative", value)) {
		if (value.compare("true") && value.compare("false")) throw logic_error("bad format");
		m_sign = (value.compare("false") == 0);
	}
	if (in.getAttribute("select", value)) {
		auto pos = find(select_tags, value);
		if (pos == select_tags.end()) throw logic_error("bad format");
		m_select = (Select) distance(select_tags.begin(), pos);
	}
	in.getEqn().setSelect(this);
}

Equation::Equation(istream& is)
{
	XMLParser in(is, *this);
	if (!in.next(XMLParser::HEADER, "equation") ||
		!in.next(XMLParser::HEADER, Expression::name)) throw logic_error("bad format");

	m_root = new Expression(in, nullptr);

	if (!in.next(XMLParser::FOOTER, "equation")) throw logic_error("bad format");
}

const string Expression::name = "expression";
const string   Function::name = "function";
const string   Constant::name = "constant";
const string   Variable::name = "variable";
const string     Number::name = "number";
const string     Divide::name = "divide";
const string      Input::name = "input";
const string      Power::name = "power";
const string       Term::name = "term";

template <class T>
Node* create(XMLParser& in, Node* parent)
{
	return new T(in, parent);
}

const map<string, XMLParser::createPtr> XMLParser::create_factors =
	{ { Expression::name, create<Expression> },
	  {   Function::name, create<Function> },
	  {   Constant::name, create<Constant> },
	  {   Variable::name, create<Variable> },
	  {     Number::name, create<Number> },
	  {     Divide::name, create<Divide> },
	  {      Input::name, create<Input> },
	  {      Power::name, create<Power> },
	};

Term* Expression::getTerm(Equation& eqn, string text, Expression* parent)
{
	Parser p(text, eqn);
	return getTerm(p, parent);
}

Term* Expression::getTerm(Parser& p, Expression* parent)
{
	bool neg = false;
	if (p.peek() == '+' || p.peek() == '-') {
		char c = p.next();
		neg = (c == '-') ? true : false;
	}
	Term* node = new Term(p, parent);
	if (neg) node->negative();
	return node;
}

bool Expression::add(Parser& p)
{
	Term* term = getTerm(p, this);
	terms.push_back(term);
	
	if ( p.peek() == '\0' || p.peek() == ')' ) {
		char c = p.next();
		return false;
	}
	else
		return true;
}

Node* Expression::parse(Parser& p, Node* parent) {
	if (p.peek() != '(') return nullptr;
	
	char c = p.next();
	return new Expression(p, parent);
}

void Equation::xml_out(XML& xml) const
{
	xml.header("equation");
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
	xml.header(this, name, false, {"name", m_name});
	m_arg->xml_out(xml);
	xml.footer(name);
}

void Binary::xml_out(XML& xml) const {
	xml.header(this, getName());
	m_first->xml_out(xml);
	m_second->xml_out(xml);
	xml.footer(getName());
}

void Variable::xml_out(XML& xml) const {
	xml.header(this, name, true, { "name", string(1, m_name) });
}

void Constant::xml_out(XML& xml) const {
	xml.header(this, name, true, { "name", string(1, m_name) });
}

void Number::xml_out(XML& xml) const {
	xml.header(this, name, true, 
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
		xml.header(this, name, true, attributes);
	}
	else {
		Equation eqn("?");
		Parser p(m_typed, eqn);
		while (p.peek()) {
			Term* term = Expression::getTerm(p, nullptr);
			term->xml_out(xml);
			delete term;
		}
	}
}

void Term::xml_out(XML& xml) const {
	xml.header(this, name, false);
	for ( auto n : factors ) { n->xml_out(xml); }
	xml.footer(name);
}

void Expression::xml_out(XML& xml) const {
	xml.header(this, name, false);
	for ( auto n : terms ) { n->xml_out(xml); }
	xml.footer(name);
}

Equation::Equation(string eq)
{ 
	Parser p(eq, *this); 
	m_root = new Expression(p);
}

void Equation::factor(string text, NodeVector& factors, Node* parent)
{
	Parser p(text, *this);
	while (p.peek()) {
		factors.push_back(Term::parse(p, parent));
	}	
}

Node* Function::parse(Parser& p, Node* parent) {
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

Function::Function(XMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (in.getAttribute("name", m_name)) {
		if (functions.find(m_name) == functions.end()) throw logic_error("function not found");
		m_func = functions.at(m_name);
	}
	else
		throw logic_error("bad format");

	if (in.hasAttributes()) throw logic_error("bad format");

	m_arg  = in.getFactor(this);

	if (!in.next(XMLParser::FOOTER, name)) throw logic_error("bad format");
}

Binary::Binary(XMLParser& in, Node* parent, const string& name) : Node(in, parent, name)
{
	if (in.hasAttributes()) throw logic_error("bad format");

	m_first  = in.getFactor(this);
	m_second = in.getFactor(this);

	if (!in.next(XMLParser::FOOTER, name)) throw logic_error("bad format");
}

Variable::Variable(XMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::ATOM, name)) throw logic_error("bad format");

	string value;
	if (in.getAttribute("name", value)) {
		m_name = value[0];
	}
	else
		throw logic_error("bad format");

	if (in.hasAttributes()) throw logic_error("bad format");
}

Constant::Constant(XMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::ATOM, name)) throw logic_error("bad format");

	string value;
	if (in.getAttribute("name", value)) {
		m_name = value[0];

		if (constants.find(m_name) == constants.end()) throw logic_error("bad format");
	}
	else
		throw logic_error("bad format");

	if (in.hasAttributes()) throw logic_error("bad format");
}

Number::Number(XMLParser& in, Node* parent) : Node(in, parent, name), m_imag_pos(-1)
{
	if (!in.getState(XMLParser::ATOM, name)) throw logic_error("bad format");

	string real = "0", imag = "0";
	in.getAttribute("real", real);
	in.getAttribute("imag", imag);

	m_value = Complex(stod(real), stod(imag));
	m_isInteger = isInteger(real) && isInteger(imag);

	if (in.hasAttributes()) throw logic_error("bad format");
}

Input::Input(XMLParser& in, Node* parent) : Node(in, parent, name), m_eqn(in.getEqn())
{
	if (!in.getState(XMLParser::ATOM, name)) throw logic_error("bad format");

	string value;
	if (in.getAttribute("current", value)) {
		if (value.compare("true") && value.compare("false")) throw logic_error("bad format");
		m_current = (value.compare("true") == 0);
	}
	if (in.getAttribute("text", value)) {
		m_typed = value;
	}
	if (in.hasAttributes()) throw logic_error("bad format");
}

Term::Term(XMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::HEADER, name)) throw logic_error("bad format");
	if (in.hasAttributes()) throw logic_error("bad format");
	
	while (Node* factor = in.getFactor(this)) { 
		factors.push_back(factor);
	}
	if (!in.getState(XMLParser::FOOTER, name)) throw logic_error("bad format");	
}

Expression::Expression(XMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::HEADER, name)) throw logic_error("bad format");
	if (in.hasAttributes()) throw logic_error("bad format");
	
	while (in.next(XMLParser::HEADER, Term::name)) { 
		terms.push_back(new Term(in, this));
	}
	if (!in.getState(XMLParser::FOOTER, name)) throw logic_error("bad format");	
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
