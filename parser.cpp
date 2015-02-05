#include <iostream>
#include <vector>
#include <iterator>
#include <map>
#include <limits>
#include <initializer_list>

#include "milo.h"
#include "nodes.h"
#include "parser.h"

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

XMLParser::XMLParser(istream& in) : m_pos(0)
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
	if (m_tags[m_pos++] == "/>") m_state |= ATOM;
	return;
}

bool XMLParser::getState(int state, const string& tag) const
{
	return ((state&m_state) != 0) && (tag.empty() || (tag == m_tag));
}

bool XMLParser::getAttribute(const string& name, string& value)
{
	if (m_attributes.find(name) == m_attributes.end()) return false;

	value = m_attributes.at(name);
	m_attributes.erase(name);
	return true;
}

namespace XML
{
	Stream& operator<<(Stream& xml, Stream::State state)
	{
		switch (state) {
		    case Stream::HEADER_START: {
				//if (xml.m_state != Stream::END) throw logic_error("bad xml state");
				break;
			}
	        case Stream::HEADER_END: {
				//if (xml.m_state != Stream::NAME) throw logic_error("bad xml state");
				xml.m_os << ">" << xml.m_sep; 
				xml.m_indent += xml.m_indent_step; 
				state = Stream::END;
				break;
			}
		    case Stream::ATOM_END: {
				//if (xml.m_state != Stream::NAME) throw logic_error("bad xml state");
				xml.m_os << "/>" << xml.m_sep;
				state = Stream::END; 
				xml.tags.pop();
				break;
			}
	        case Stream::FOOTER: {
				//if (xml.m_state != Stream::END) throw logic_error("bad xml state");
				xml.m_indent -= xml.m_indent_step; 
				if (xml.m_indent > 0) xml.m_os << string(xml.m_indent, ' ');
				xml.m_os << "</" << xml.tags.top() << ">" << xml.m_sep; 
				xml.tags.pop();
				state = Stream::END;
				break;
			}
		    default: {
				//throw logic_error("bad xml state");
				break;
			}
		}
		xml.m_state = state;
		return xml;
	}
	
	Stream& operator<<(Stream& xml, const string& tag)
	{
		switch (xml.m_state) {
	        case Stream::HEADER_START: {
				if (xml.m_indent > 0) xml.m_os << string(xml.m_indent, ' ');
				xml.m_os << "<" << tag;
				xml.tags.push(tag);
				xml.m_state = Stream::NAME;
				break;
			}
	        case Stream::NAME: {
			    xml.m_os << " " << tag << ((tag.back() == '=') ? "" : "=");
			    xml.m_state = Stream::VALUE;
			    break;
		    }
	        case Stream::VALUE: {
				xml.m_os << "\"" << tag << "\"";
				xml.m_state = Stream::NAME;
				break;
			}
		    default: {
				//throw logic_error("bad xml state");
				break;
			}
		}	
	}

	Stream::Stream(ostream& os, int step, string sep) : 
		m_sep(sep), m_os(os), m_indent_step(step), m_indent(0), m_state(END)
	{
		*this << HEADER_START << "document" << HEADER_END;
	}

	Stream::~Stream()
	{ 
		*this << FOOTER;
	}
}

Node* EqnXMLParser::getFactor(Node* parent)
{
	next();
	if (create_factors.find(getTag()) == create_factors.end()) return nullptr;
	auto cp = create_factors.at(getTag());
	return cp(*this, parent);
}

void Node::out(XML::Stream& xml)
{ 
	xml << XML::Stream::HEADER_START << getName();
	if (m_nth != 1) xml << "nth" << to_string(m_nth);
	xml_out(xml);
}

Node::Node(EqnXMLParser& in, Node* parent, const string& name) : 
	m_parent(parent), m_sign(true), m_select(NONE), m_nth(1)
{
	if (!in.getState(XMLParser::HEADER, name)) 
		throw logic_error("bad header: expected: " + name + ", got: " + in.getTag());

	string value;
	if (in.getAttribute("negative", value)) {
		if (value != "true" && value != "false") throw logic_error("bad format");
		m_sign = (value == "false");
	}
	if (in.getAttribute("select", value)) {
		auto pos = find(select_tags, value);
		if (pos == select_tags.end()) throw logic_error("bad format");
		m_select = (Select) distance(select_tags.begin(), pos);
	}
	if (in.getAttribute("nth", value)) {
		if (!isInteger(value)) throw logic_error("bad format");
		m_nth = atoi(value.c_str());
	}
	in.getEqn().setSelectFromNode(this);
}

Equation::Equation(istream& is)
{
	EqnXMLParser in(is, *this);
	if (!in.next(XMLParser::HEADER, "equation") ||
		!in.next(XMLParser::HEADER, Expression::name)) throw logic_error("bad format");

	m_root = new Expression(in, nullptr);
	m_root->setDrawParenthesis(false);

	if (!in.next(XMLParser::FOOTER, "equation")) throw logic_error("bad format");
}

Equation& Equation::operator=(const Equation& eqn)
{
	m_inputs.clear();
	m_input_index = -1;
	delete m_root;

	string store;
	eqn.xml_out(store);

	istringstream is(store);
	EqnXMLParser in(is, *this);
	if (!in.next(XMLParser::HEADER, "equation") ||
		!in.next(XMLParser::HEADER, Expression::name)) throw logic_error("bad format");

	m_root = new Expression(in, nullptr);
	m_root->setDrawParenthesis(false);

	if (!in.next(XMLParser::FOOTER, "equation")) throw logic_error("bad format");
	return *this;
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
static Node* create(EqnXMLParser& in, Node* parent)
{
	return new T(in, parent);
}

const map<string, EqnXMLParser::createPtr> EqnXMLParser::create_factors =
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

void Equation::xml_out(XML::Stream& xml) const
{
	xml << XML::Stream::HEADER_START << "equation" << XML::Stream::HEADER_END;
	m_root->out(xml);
	xml << XML::Stream::FOOTER;
}

void Equation::xml_out(ostream& os) const 
{
	XML::Stream xml(os);
	xml_out(xml);
}

void Equation::xml_out(string& str) const
{
	ostringstream os; 
	xml_out(os);
	str = os.str();
}

void Function::xml_out(XML::Stream& xml) const 
{
	xml << "name" << m_name << XML::Stream::HEADER_END;
	m_arg->out(xml);
	xml << XML::Stream::FOOTER;
}

void Binary::xml_out(XML::Stream& xml) const 
{
	xml << XML::Stream::HEADER_END;
	m_first->out(xml);
	m_second->out(xml);
	xml << XML::Stream::FOOTER;
}

void Variable::xml_out(XML::Stream& xml) const
{
	xml << "name" << string(1, m_name) << XML::Stream::ATOM_END;
}

void Constant::xml_out(XML::Stream& xml) const 
{
	xml << "name" << string(1, m_name) << XML::Stream::ATOM_END;
}

void Number::xml_out(XML::Stream& xml) const 
{
	xml << "value" << to_string(m_value) << XML::Stream::ATOM_END;
}

void Input::xml_out(XML::Stream& xml) const
{
	vector<string> attributes;
	if (!m_typed.empty()) {
		xml << "text" << m_typed;
	}
	if (m_current) {
		xml << "current" << "true";
	}		
	xml << XML::Stream::ATOM_END;
}

void Term::xml_out(XML::Stream& xml) const
{
	xml << XML::Stream::HEADER_END;
	for ( auto n : factors ) { n->out(xml); }
	xml << XML::Stream::FOOTER;
}

void Expression::xml_out(XML::Stream& xml) const
{
	xml << XML::Stream::HEADER_END;
	for ( auto n : terms ) { n->out(xml); }
	xml << XML::Stream::FOOTER;
}

Equation::Equation(string eq)
{ 
	Parser p(eq, *this); 
	m_root = new Expression(p);
	m_root->setDrawParenthesis(false);
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
			return new Function(m.first, m.second, p, parent);
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
	if (isdigit(c)) {
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

void Number::getNumber(Parser& p)
{
	string n = getInteger(p);
	if (!p.peek() || (p.peek() != '.' && toupper(p.peek()) != 'E')) {
		m_value = stod(n);
		return;
	}
	char c = p.next();
	if (c == '.' && isdigit(p.peek())) {
		n += "." +  getInteger(p);
	}
	if (toupper(p.peek()) == 'E') {
		p.next(); n += 'E';
		if (p.peek() == '-' || p.peek() == '+') n += p.next();

		if (!isdigit(p.peek())) throw logic_error("bad format");
		n += getInteger(p);
	}

	m_value = stod(n);
	m_isInteger = isInteger(m_value);
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

Function::Function(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
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

Binary::Binary(EqnXMLParser& in, Node* parent, const string& name) : Node(in, parent, name)
{
	if (in.hasAttributes()) throw logic_error("bad format");

	m_first  = in.getFactor(this);
	m_second = in.getFactor(this);

	if (!in.next(XMLParser::FOOTER, name)) throw logic_error("bad format");
}

Variable::Variable(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
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

Constant::Constant(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
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

Number::Number(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::ATOM, name)) throw logic_error("bad format");

	string real = "0";
	in.getAttribute("value", real);

	m_value = stod(real);
	m_isInteger = isInteger(real);

	if (in.hasAttributes()) throw logic_error("bad format");
}

Input::Input(EqnXMLParser& in, Node* parent) : Node(in, parent, name), m_eqn(in.getEqn())
{
	if (!in.getState(XMLParser::ATOM, name)) throw logic_error("bad format");

	string value;
	if (in.getAttribute("current", value)) {
		if (value != "true" && value != "false") throw logic_error("bad format");
		m_current = (value == "true");
	}
	if (in.getAttribute("text", value)) {
		m_typed = value;
	}
	if (in.hasAttributes()) throw logic_error("bad format");
}

Term::Term(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::HEADER, name)) throw logic_error("bad format");
	if (in.hasAttributes()) throw logic_error("bad format");
	
	while (Node* factor = in.getFactor(this)) { 
		factors.push_back(factor);
	}
	if (!in.getState(XMLParser::FOOTER, name)) throw logic_error("bad format");	
}

Expression::Expression(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	if (!in.getState(XMLParser::HEADER, name)) throw logic_error("bad format");
	if (in.hasAttributes()) throw logic_error("bad format");
	
	while (in.next(XMLParser::HEADER, Term::name)) { 
		terms.push_back(new Term(in, this));
	}
	setDrawParenthesis(true);
	if (!in.getState(XMLParser::FOOTER, name)) throw logic_error("bad format");	
}

Input::Input(Parser& p, Node* parent) : 
	Node(parent), m_sn(++input_sn), m_current(false), m_typed(""), m_eqn(p.getEqn())
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
