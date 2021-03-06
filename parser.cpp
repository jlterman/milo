/* Copyright (C) 2015 - James Terman
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

/**
 * @file parser.cpp
 * This file contains the private class Parser and the member 
 * functions of other public classes that use them. Parser is
 * referenced as opaque references in other files.
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

/**
 * Parse string as an equation.
 * This class expects a string that contains the same string representation
 * that is generated by the Node class in infix notation.
 */
class Parser
{
public:
	/**
	 * Constructor for Parser class.
	 * @param expr String containing equation.
	 * @param eqn  Equation object container for node tree.
	 */
	Parser(const string& expr, Equation& eqn) : m_expr(expr), m_eqn(eqn), m_pos(0) {}

	/**
	 * Get the next character to be parsed without advancing.
	 * @return Next character.
	 */
	char peek() { return m_expr[m_pos]; }

	/**
	 * Get next character in string and advance pointer.
	 * @return Next character.
	 */
	char next();

	/**
	 * Match string in parser stream at current pointer.
	 * @return True if string is matched.
	 */
	bool match(const string& s);

	/**
	 * Get reference to Equation object.
	 * @return Equation object reference.
	 */
	Equation& getEqn() { return m_eqn; }
private:
	string m_expr;   ///< String to be parsed.
	Equation& m_eqn; ///< Equation containing node tree.
	size_t m_pos;    ///< Pointer to next character to be parsed.
};

// Get next character to be parsed or '\0' if at end.
char Parser::next()
{
	if (m_pos < m_expr.length()) return m_expr[m_pos++];
	                        else return '\0';
}

// Return true if string in parser.
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

/* Serialize xml for Node object.
 * First output header with name of derived class and 
 * attributes nth, negatvie and select if needed.
 * Then call virtual member function xml_out for rest.
 */
XML::Stream& Node::out(XML::Stream& xml)
{ 
	xml << XML::HEADER << getName();
	if (m_nth != 1) xml << XML::NAME_VALUE << "nth" << to_string(m_nth);
	if (!m_sign) xml << XML::NAME_VALUE << "negative" << "true";
	if (m_select != NONE) xml << XML::NAME_VALUE << "select" << select_tags.at(m_select);
	xml_out(xml);
	return xml;
}

Node::Node(Parser& p, Node* parent, bool fNeg, Select s) : 
	m_eqn(p.getEqn()), m_parent(parent), m_sign(!fNeg), m_select(s) {}

// Constructor for node class from XML parser.
Node::Node(XML::Parser& in, Equation& eqn, Node* parent) : 
	 m_eqn(eqn), m_parent(parent),m_sign(true), m_select(NONE), m_nth(1)
{
	// Check for name, value pairs to load in.
	if (in.check(XML::NAME_VALUE)) in.next(XML::NAME_VALUE);

	// Check for name, value pairs common to all nodes.
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
		    case START: eqn.setSelectStart(this);
			            break;
		    case END:   eqn.setSelectEnd(this);
			            break;
		    case ALL:   eqn.setSelect(this);
			            break;
		    default: 
				   break;
		}
	}
	if (in.getAttribute("nth", value)) {
		if (!isInteger(value)) in.syntaxError("not an integer");
		m_nth = atoi(value.c_str());
	}
	eqn.setSelectFromNode(this); // Register selection with Equation.
}

// Read in Equation from XML.
void Equation::xml_in(XML::Parser& in)
{
	// Read in equation header and then expression header.
	in.next(XML::HEADER, "equation").next(XML::HEADER_END).next(XML::HEADER, Expression::name);

	m_root = new Expression(in, *this, nullptr);
	m_root->setDrawParenthesis(false);

	in.next(XML::FOOTER);
}

// Constructor for Equation read in from input stream.
Equation::Equation(istream& is)
{
	XML::Parser in(is);
	xml_in(in);
}

// Implement operator= for Equation class.
Equation& Equation::operator=(const Equation& eqn)
{
	// Clear out all equation data.
	m_inputs.clear();
	m_input_index = -1;

	// Serialize equation to be copied.
	string store;
	eqn.xml_out(store);

	// Read in serialized equation.
	istringstream is(store);
	XML::Parser in(is);
	in.next(XML::HEADER, "equation");
	xml_in(in);
	return *this;
}

// Clone this Equation by serializing/deserializing.
Equation* Equation::clone()
{
	string store;
	xml_out(store);
	istringstream in(store);
	return new Equation(in);
}

const string Differential::name = "differential"; // Differential class name.
const string   Expression::name = "expression";   // Expression class name.
const string     Function::name = "function";     // Function class name.
const string     Constant::name = "constant";     // Constant class name.
const string     Variable::name = "variable";     // Variable class name.
const string       Number::name = "number";       // Number class name.
const string       Divide::name = "divide";       // Divide class name.
const string        Input::name = "input";        // Input class name.  
const string        Power::name = "power";        // Power class name.
const string         Term::name = "term";         // Term class name.

/**
 * Template function to return a new class object.
 * Use the template specialization to call the correct
 * constructor to get the desired derived class. Note: 
 * pointer is to the node base class.
 */
template <class T>
static Node* create(XML::Parser& in, Equation& eqn, Node* parent)
{
	return new T(in, eqn, parent);
}

/**
 * Function pointer to create a new node.
 */
using createPtr = Node* (*)(XML::Parser&, Equation&, Node*);

/**
 * Map of class name to named constructor.
 */
static const unordered_map<string, createPtr> create_factors =
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

/**
 * Get next factor from xml.
 * @param parent Set parent of factor.
 * @param eqn Equation associated with this node.
 * @param in XML input parser object.
 * @return Parser factor or null if not found.
 */
static Node* getFactor(XML::Parser& in, Equation& eqn, Node* parent)
{
	if (!in.check(XML::HEADER)) return nullptr;

	in.next(XML::HEADER);
	if (create_factors.find(in.getTag()) == create_factors.end()) return nullptr;

	auto cp = create_factors.at(in.getTag());
	return cp(in, eqn, parent);
}

Term* Expression::getTerm(Equation& eqn, const string& text, Expression* parent)
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
		p.next();
		return false;
	}
	else
		return true;
}

Node* Expression::parse(Parser& p, Node* parent) {
	if (p.peek() != '(') return nullptr;
	
	p.next();
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

Equation::Equation(const string& eq)
{ 
	Parser p(eq, *this); 
	m_root = new Expression(p);
	m_root->setDrawParenthesis(false);
}

void Equation::insert(FactorIterator& it, const string& text)
{
	Parser p(text, *this);
	while (p.peek()) {
		it.insert(Term::parse(p, nullptr));
		++it;
	}	
}

Function* Function::parse(Parser& p, Node* parent) {
	if (!isalpha(p.peek()) ) return nullptr;

	// Search all built in functions and return a match.
	for ( auto m : functions ) { 
		if (p.match(m.first + "(")) {
			return new Function(m.first, m.second, p, parent);
		}
	}
	return nullptr; // No match found.
}

Node* Binary::parse(Parser& p, Node* one, Node* parent)
{
	char c = p.peek();
	if (c != '/' && c != '^') return one; // No binary op found, return given node.

	c = p.next();
	Node* two = Term::parse(p);
	if (!two) throw logic_error("bad format");

	if ( c == '/' ) 
		return new Divide(one, two, p.getEqn(), parent); 
	else if ( c == '^' ) 
		return new Power(one, two, p.getEqn(), parent);
	return one;
}

// Parser constructor for variable. Get name from parser.
Variable::Variable(Parser& p, Node* parent) : Node(p.getEqn(), parent), m_name( p.next() ) {}

Variable* Variable::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( isalpha(c) ) {
		return new Variable(p, parent);
	}
	else
		return nullptr; // No variable node found, return null.
}

// Parser constructor for constant. Get name from parser and find value.
Constant::Constant(Parser& p, Node* parent) : 
	Node(p.getEqn(), parent), m_name(p.next()), m_value(constants.find(m_name)->second) {}

Constant* Constant::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if ( constants.find(c) != constants.end() ) {
		return new Constant(p, parent);
	}
	else
		return nullptr; // This is not a constant, return null.
}

Number* Number::parse(Parser& p, Node* parent)
{
	char c = p.peek();
	if (isdigit(c)) {
		return new Number(p, parent);
	}
	else
		return nullptr; // Not a number, return null.
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

Function::Function(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
{
	in.next(XML::HEADER_END);

	if (in.getAttribute("name", m_name)) {
		if (functions.find(m_name) == functions.end()) 
			in.syntaxError("function name unknown: " + m_name);
		m_func = functions.at(m_name);
	}
	else
		in.syntaxError("function name not found");

	m_arg  = getFactor(in, eqn, this);
	if (!m_arg) in.syntaxError("header for factor expected");

	in.next(XML::FOOTER);
}

Binary::Binary(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
{
	in.next(XML::HEADER_END);

	m_first  = getFactor(in, eqn, this);
	if (!m_first) in.syntaxError("header for factor expected");

	m_second = getFactor(in, eqn, this);
	if (!m_second) in.syntaxError("header for factor expected");

	in.next(XML::FOOTER);
}

Variable::Variable(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
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

Constant::Constant(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
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

Number::Number(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
{
	string real = "0";
	if (!in.getAttribute("value", real)) in.syntaxError("Missing value attribute");

	m_value = stod(real);
	m_isInteger = isInteger(real);

	in.assertNoAttributes();
	in.next(XML::ATOM_END);
}

Input::Input(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent), m_sn(++input_sn), m_current(false)
{
	m_eqn.addInput(this);
	string value;
	if (in.getAttribute("current", value)) {
		if (value != "true" && value != "false") in.syntaxError("bad boolean value");
	} else {
		value = "false";
	}
	m_current = (value == "true");
	if (m_current) m_eqn.setCurrentInput(m_sn);
	if (in.getAttribute("text", value)) {
		m_typed = value;
	}
	in.next(XML::ATOM_END);
}

Term::Term(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
{
	in.next(XML::HEADER_END);
	in.assertNoAttributes();

	while (Node* factor = getFactor(in, eqn, this)) { 
		factors.push_back(factor);
	}

	in.next(XML::FOOTER);
}

Expression::Expression(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
{
	in.next(XML::HEADER_END);
	in.assertNoAttributes();
	
	while (in.check(XML::HEADER, Term::name)) { 
		in.next(XML::HEADER, Term::name);
		terms.push_back(new Term(in, eqn, this));
	}
	setDrawParenthesis(true);

	in.next(XML::FOOTER);
}

Input::Input(Parser& p, Node* parent) : 
	Node(p, parent), m_sn(++input_sn), m_typed(""), m_current(false)
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

Differential::Differential(Parser& p, Node* parent) : Node(p, parent)
{
	p.match("D/D");
	m_variable = p.next();
	if (!isalpha(m_variable)) throw logic_error("expected variable name");

	m_function = Expression::parse(p, this);
	if (!m_function) throw logic_error("exected expression");
}

Differential::Differential(XML::Parser& in, Equation& eqn, Node* parent) : Node(in, eqn, parent)
{
	string name;
	if (!in.getAttribute("variable", name)) in.syntaxError("missing variable name");
	m_variable = name[0];

	in.assertNoAttributes();
	in.next(XML::HEADER_END);
	in.next(XML::HEADER, Expression::name);

	m_function = new Expression(in, eqn, this);
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


