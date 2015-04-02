/*
 * parser.cpp
 * This file is part of milo
 *
 * Copyright (C) 2015 - James Terman
 *
 * milo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * milo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include <vector>
#include <iterator>
#include <map>
#include <limits>
#include <initializer_list>

#include "milo.h"
#include "nodes.h"
#include "xml.h"

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

class EqnXMLParser : public XML::Parser
{
public:
	typedef Node* (*createPtr)(EqnXMLParser&, Node*);

    EqnXMLParser(std::istream& in, Equation& eqn) : XML::Parser(in), m_eqn(eqn) {}
	~EqnXMLParser() {}

	Equation& getEqn() const { return m_eqn; }
	Node* getFactor(Node* parent);
	void assertNoAttributes() { if (hasAttributes()) syntaxError("Unknown attribute"); }
private:
	Equation& m_eqn;

	static const std::map<std::string, createPtr> create_factors;
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

Node* EqnXMLParser::getFactor(Node* parent)
{
	if (!check(XML::HEADER)) return nullptr;

	next(XML::HEADER);
	if (create_factors.find(getTag()) == create_factors.end()) return nullptr;

	auto cp = create_factors.at(getTag());
	return cp(*this, parent);
}

void Node::out(XML::Stream& xml)
{ 
	xml << XML::HEADER << getName();
	if (m_nth != 1) xml << XML::NAME_VALUE << "nth" << to_string(m_nth);
	if (!m_sign) xml << XML::NAME_VALUE << "negative" << "true";
	if (m_select != NONE) xml << XML::NAME_VALUE << "select" << select_tags.at(m_select);
	xml_out(xml);
}

Node::Node(EqnXMLParser& in, Node* parent, const string& name) : 
	m_parent(parent), m_sign(true), m_select(NONE), m_nth(1)
{
	if (in.check(XML::NAME_VALUE)) in.next(XML::NAME_VALUE);

	string value;
	if (in.getAttribute("negative", value)) {
		if (value != "true" && value != "false") in.syntaxError("bad boolean value");
		m_sign = (value == "false");
	}
	if (in.getAttribute("select", value)) {
		auto pos = find(select_tags, value);
		if (pos == select_tags.end()) in.syntaxError("unknown select node value");
		m_select = (Select) distance(select_tags.begin(), pos);
		switch (m_select) {
		    START: in.getEqn().setSelectStart(this);
			       break;
		    END:   in.getEqn().setSelectEnd(this);
			       break;
		    ALL:   in.getEqn().setSelect(this);
			       break;
		    default: 
				   break;
		}
	}
	if (in.getAttribute("nth", value)) {
		if (!isInteger(value)) in.syntaxError("not an integer");
		m_nth = atoi(value.c_str());
	}
	in.getEqn().setSelectFromNode(this);
}

void Equation::xml_in(EqnXMLParser& in)
{
	in.next(XML::HEADER, "equation").next(XML::HEADER_END).next(XML::HEADER, Expression::name);

	m_root = new Expression(in, nullptr);
	m_root->setDrawParenthesis(false);

	in.next(XML::FOOTER);
}

Equation::Equation(istream& is)
{
	EqnXMLParser in(is, *this);
	xml_in(in);
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
	xml_in(in);
	return *this;
}

const string Differential::name = "differential";
const string   Expression::name = "expression";
const string     Function::name = "function";
const string     Constant::name = "constant";
const string     Variable::name = "variable";
const string       Number::name = "number";
const string       Divide::name = "divide";
const string        Input::name = "input";
const string        Power::name = "power";
const string         Term::name = "term";

template <class T>
static Node* create(EqnXMLParser& in, Node* parent)
{
	return new T(in, parent);
}

const map<string, EqnXMLParser::createPtr> EqnXMLParser::create_factors =
	{ { Differential::name, create<Differential> },
	  {   Expression::name, create<Expression>   },
	  {     Function::name, create<Function>     },
	  {     Constant::name, create<Constant>     },
	  {     Variable::name, create<Variable>     },
	  {       Number::name, create<Number>       },
	  {       Divide::name, create<Divide>       },
	  {        Input::name, create<Input>        },
	  {        Power::name, create<Power>        },
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

Expression* Expression::parse(Parser& p, Node* parent) {
	if (p.peek() != '(') return nullptr;
	
	char c = p.next();
	return new Expression(p, parent);
}

void Equation::xml_out(XML::Stream& xml) const
{
	xml << XML::HEADER << "equation" << XML::HEADER_END;
	m_root->out(xml);
	xml << XML::FOOTER;
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
	xml << XML::NAME_VALUE << "name" << m_name << XML::HEADER_END;
	m_arg->out(xml);
	xml << XML::FOOTER;
}

void Binary::xml_out(XML::Stream& xml) const 
{
	xml << XML::HEADER_END;
	m_first->out(xml);
	m_second->out(xml);
	xml << XML::FOOTER;
}

void Variable::xml_out(XML::Stream& xml) const
{
	xml << XML::NAME_VALUE << "name" << string(1, m_name) << XML::ATOM_END;
}

void Constant::xml_out(XML::Stream& xml) const 
{
	xml << XML::NAME_VALUE << "name" << string(1, m_name) << XML::ATOM_END;
}

void Number::xml_out(XML::Stream& xml) const 
{
	xml << XML::NAME_VALUE << "value" << to_string(m_value) << XML::ATOM_END;
}

void Input::xml_out(XML::Stream& xml) const
{
	vector<string> attributes;
	if (!m_typed.empty()) {
		xml << XML::NAME_VALUE << "text" << m_typed;
	}
	if (m_current) {
		xml << XML::NAME_VALUE << "current" << "true";
	}		
	xml << XML::ATOM_END;
}

void Term::xml_out(XML::Stream& xml) const
{
	xml << XML::HEADER_END;
	for ( auto n : factors ) { n->out(xml); }
	xml << XML::FOOTER;
}

void Expression::xml_out(XML::Stream& xml) const
{
	xml << XML::HEADER_END;
	for ( auto n : terms ) { n->out(xml); }
	xml << XML::FOOTER;
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

Function* Function::parse(Parser& p, Node* parent) {
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

Variable* Variable::parse(Parser& p, Node* parent)
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

Constant* Constant::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( constants.find(c) != constants.end() ) {
		return new Constant(p, parent);
	}
	else
		return nullptr;			
}

Number* Number::parse(Parser& p, Node* parent)
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

	Node*      node = Expression::parse(p, parent);
	if (!node) node = Function::parse(p, parent);
	if (!node) node = Differential::parse(p, parent);
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
	in.next(XML::HEADER_END);

	if (in.getAttribute("name", m_name)) {
		if (functions.find(m_name) == functions.end()) 
			in.syntaxError("function name unknown: " + m_name);
		m_func = functions.at(m_name);
	}
	else
		in.syntaxError("function name not found");

	in.assertNoAttributes();
	m_arg  = in.getFactor(this);
	if (m_arg == nullptr) in.syntaxError("header for factor expected");

	in.next(XML::FOOTER);
}

Binary::Binary(EqnXMLParser& in, Node* parent, const string& name) : Node(in, parent, name)
{
	in.next(XML::HEADER_END);
	in.assertNoAttributes();

	m_first  = in.getFactor(this);
	if (m_first == nullptr) in.syntaxError("header for factor expected");

	m_second = in.getFactor(this);
	if (m_second == nullptr) in.syntaxError("header for factor expected");

	in.next(XML::FOOTER);
}

Variable::Variable(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	string value;
	if (in.getAttribute("name", value)) {
		m_name = value[0];
	}
	else
		in.syntaxError("Missing name attribute");

	in.assertNoAttributes();
	in.next(XML::ATOM_END);
}

Constant::Constant(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	string value;
	if (in.getAttribute("name", value)) {
		m_name = value[0];

		if (constants.find(m_name) == constants.end()) 
			in.syntaxError("Unknown constant name");
	}
	else
		in.syntaxError("Missing name attribute");

	in.assertNoAttributes();
	in.next(XML::ATOM_END);
}

Number::Number(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	string real = "0";
	if (!in.getAttribute("value", real)) in.syntaxError("Missing value attribute");

	m_value = stod(real);
	m_isInteger = isInteger(real);

	in.assertNoAttributes();
	in.next(XML::ATOM_END);
}

Input::Input(EqnXMLParser& in, Node* parent) : Node(in, parent, name), m_eqn(in.getEqn()), m_sn(++input_sn)
{
	m_eqn.addInput(this);
	string value;
	if (in.getAttribute("current", value)) {
		if (value != "true" && value != "false") in.syntaxError("bad boolean value");
		m_current = (value == "true");
		if (m_current) m_eqn.setCurrentInput(m_sn);
	}
	if (in.getAttribute("text", value)) {
		m_typed = value;
	}
	in.next(XML::ATOM_END);
}

Term::Term(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	in.next(XML::HEADER_END);
	in.assertNoAttributes();

	while (Node* factor = in.getFactor(this)) { 
		factors.push_back(factor);
	}

	in.next(XML::FOOTER);
}

Expression::Expression(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	in.next(XML::HEADER_END);
	in.assertNoAttributes();
	
	while (in.check(XML::HEADER, Term::name)) { 
		in.next(XML::HEADER, Term::name);
		terms.push_back(new Term(in, this));
	}
	setDrawParenthesis(true);

	in.next(XML::FOOTER);
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

Input* Input::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( c == '?' || c == '#' || c == '[' ) {
		return new Input(p, parent);
	}
	else
		return nullptr;
}

Differential::Differential(Parser& p, Node* parent) : Node(parent)
{
	p.match("D/D");
	m_variable = p.next();
	if (!isalpha(m_variable)) throw logic_error("expected variable name");

	m_function = Expression::parse(p, this);
	if (m_function == nullptr) throw logic_error("exected expression");
}

Differential::Differential(EqnXMLParser& in, Node* parent) : Node(in, parent, name)
{
	string name;
	if (!in.getAttribute("variable", name)) in.syntaxError("missing variable name");
	m_variable = name[0];

	in.assertNoAttributes();
	in.next(XML::HEADER_END);
	in.next(XML::HEADER, Expression::name);

	m_function = new Expression(in, this);
	in.next(XML::FOOTER);
}

Differential* Differential::parse(Parser& p, Node* parent)
{
	if (p.match("D/D")) 
		return new Differential(p, parent);
	else 
		return nullptr;
}

string Differential::toString() const
{
	return "D/D" + string(1, m_variable) + m_function->toString();
}

void Differential::xml_out(XML::Stream& xml) const
{
	xml << XML::NAME_VALUE << "variable" << string(1, m_variable) << XML::HEADER_END;
	m_function->out(xml);
	xml << XML::FOOTER;
}


