#include <iostream>
#include <vector>
#include <iterator>
#include <exception>
#include <map>
#include <limits>
#include <typeinfo>

#include "milo.h"
#include "milo_key.h"
#include "nodes.h"

using namespace std;

void Equation::asciiArt(Draw& draw)
{
	m_root->calcSize();
	m_root->calcOrig(0, 0);
	draw.set(m_root->getSizeX(), m_root->getSizeY());
	setSelect(draw);
	m_root->asciiArt(draw);
}

void Equation::setSelect(Draw& draw)
{
	if (m_selectStart != nullptr && m_selectStart == m_selectEnd) {
		draw.setSelect(m_selectStart);
	}
	else if (m_selectStart != nullptr) {
		int x0 = m_selectStart->getOrigX();
		int y0 = m_selectStart->getOrigY();
		int x = 0, y = 0;

		Node* node = nullptr;
		do {
			node = (node == nullptr) ? m_selectStart : node->getNextRight();
			x += m_selectStart->getSizeX();
			y = max(y, m_selectStart->getSizeY());
		}
		while (node != nullptr && node != m_selectEnd);
		draw.setSelect(x, y, x0, y0);
	}
}

void Equation::setCurrentInput(int in_sn)
{
	if (m_input_index >= 0) m_inputs[m_input_index]->m_current = false;
	if (in_sn < 0) {
		m_input_index = -1;
		return;
	}
	for (int i = 0; i < m_inputs.size(); ++i) {
		if (m_inputs[i]->m_sn == in_sn) {
			m_input_index = i;
			m_inputs[m_input_index]->m_current = true;
			break;
		}
	}
}

bool isZero(double x) {
	return abs(x)<1e-10;
}

bool isZero(Complex z) {
	return isZero(z.real()) && isZero(z.imag());
}

bool isInteger(const string& s)
{
	bool fInteger = true;
	for ( char c : s ) { fInteger *= isdigit(c); }
	return fInteger;
}

const vector<string> Node::select_tags = { "NONE", "START", "END", "ALL" };

Node* Node::begin()
{
	Node* node = downLeft();
	return (node != nullptr) ? node : this;
}

Node* Node::end()
{
	Node* node = downRight();
	return (node != nullptr) ? node : this;
}

Node* Node::getNextLeft()
{
	Node* left = m_parent->getLeftSibling(this);
	if (left != nullptr) { 
		return left->end();
	}
	else if (m_parent != nullptr) {
		return m_parent->getNextLeft();
	}
	else
		return nullptr;
}

Node* Node::getNextRight()
{
	Node* right = m_parent->getRightSibling(this);
	if (right != nullptr) { 
		return right->begin();
	}
	else if (m_parent != nullptr) {
		return m_parent->getNextRight();
	}
	else
		return nullptr;
}

void Divide::calcSize()
{
	m_first->calcSize();
	m_second->calcSize();
	setSize( max(m_first->getSizeX(), m_second->getSizeX()),
					 m_first->getSizeY() + 1 + m_second->getSizeY() );
	setBaseLine(m_first->getSizeY());
}

void Divide::calcOrig(int x, int y)
{
	setOrig(x, y);
	m_first->calcOrig(x + (getSizeX() - m_first->getSizeX())/2, y);
	m_second->calcOrig(x + (getSizeX() - m_second->getSizeX())/2, 
				 		   y + m_first->getSizeY() + 1);
}


void Divide::asciiArt(Draw& draw) const
{
	draw.horiz_line(getSizeX(), getOrigX(), getOrigY() + getBaseLine());
	m_first->asciiArt(draw);
	m_second->asciiArt(draw);
}

void Power::calcSize()
{
	m_first->calcSize();
	m_second->calcSize();
	setSize(m_first->getSizeX() + m_second->getSizeX(),
				m_first->getSizeY() + m_second->getSizeY());
	setBaseLine(m_second->getSizeY());
}

void Power::calcOrig(int x, int y)
{
	setOrig(x, y);
	m_first->calcOrig(x, y + m_second->getSizeY());
	m_second->calcOrig(x + m_first->getSizeX(), y);
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

void Function::init_functions()
{
	functions.emplace("sin", &sinZ);
	functions.emplace("cos", &cosZ);
	functions.emplace("tan", &tanZ);
	functions.emplace("log", &logZ);
	functions.emplace("exp", &expZ);
}

void Function::calcSize() 
{
	m_arg->calcSize();
	setSize(m_name.length() + m_arg->getSizeX(), m_arg->getSizeY());
	setBaseLine(m_arg->getSizeY()/2);
}

void Function::calcOrig(int x, int y)
{
	setOrig(x, y);
	m_arg->calcOrig(x + m_name.length(), y);
}

void Function::asciiArt(Draw & draw) const
{
	draw.at(getOrigX(), getOrigY() + getBaseLine(), m_name, Draw::Color::GREEN);
	m_arg->asciiArt(draw);
}

Constant::const_map Constant::constants;

void Constant::init_constants()
{
	constants.emplace('e', Complex(exp(1.0), 0));
	constants.emplace('P', Complex(4*atan(1.0), 0));
	constants.emplace('i', Complex(0, 1));
}

void Constant::calcSize() 
{
	setSize(1, 1);
	setBaseLine(0);
}

void Constant::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Constant::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), m_name);
}

void Variable::calcSize() 
{
	setSize(1, 1);
	setBaseLine(0);
}

void Variable::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Variable::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), m_name);
}

string Number::toString(double value, bool real) const
{
	double mag = abs(value);
	bool neg = signbit(value);

	if (isZero(value)) return (real ? "0" : "i0" );

	if (!m_isInteger) {
		string result;
		if (real && neg) result += "-";
		if (!real) result += (neg ? "-i" : "i");
		result += std::to_string(mag);
		while (result.back() == '0') result.erase(result.end() - 1);
		return result;
	}
	else {
		string result = to_string((int) (mag + 0.5));
		if (real) return string(neg ? "-" : "") + result;
		     else return string(neg ? "-i" : "i") + result;
	}
}

string Number::toString() const
{
	if (isZero(m_value)) {
		return "0";
	}
	else if ( !isZero(m_value.real()) ) {
		string result = toString(m_value.real(), true);
		if ( isZero(m_value.imag()) ) return result;

		if (m_value.imag() > 0) result += "+";
		return result + toString(m_value.imag(), false);
	}
	else
		return toString(m_value.imag(), false);
}

void Number::calcSize() 
{
	string n = toString();
	m_imag_pos = n.find('i');
	setSize(n.length(), 1);
	setBaseLine(0);
}

void Number::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Number::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), toString());

	if (m_imag_pos != string::npos) 
		draw.at(getOrigX() + m_imag_pos, getOrigY(), 'i', Draw::Color::RED);
}

void Term::calcSize() 
{
	int x = 0, b = 0, y = 0;
	for ( auto n : factors ) { 
		n->calcSize(); 
		x += n->getSizeX();
		y = max(y, n->getSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	setSize(x, b + y);
	setBaseLine(b);
}

void Term::calcOrig(int x, int y)
{
	setOrig(x, y);
	for ( auto n : factors ) {
		n->calcOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getSizeX();
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

Node* Term::getLeftSibling(Node* node)
{
	for (int i = 1; i < factors.size(); ++i) {
		if (factors[i] == node)
			return factors[i - 1];
	}
	return nullptr;
}

Node* Term::getRightSibling(Node* node)
{
	for (int i = 0; i < factors.size() - 2; ++i) {
		if (factors[i] == node) 
			return factors[i + 1];
	}
	return nullptr;
}

void Expression::calcSize() 
{
	int x = 0, b = 0, y = 0;
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	for ( auto n : terms ) { 
		if (n != terms[0] || !n->getSign()) x += 1;
		n->calcSize(); 
		x += n->getSizeX();
		y = max(y, n->getSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	setSize(x, y + b);
	setBaseLine(b);
}

void Expression::calcOrig(int x, int y)
{
	setOrig(x, y);
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) x+=1;
		n->calcOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getSizeX();
	}
}

void Expression::asciiArt(Draw& draw) const
{
	if (getParent() && getParent()->drawParenthesis()) draw.parenthesis(this);
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) draw.at(n->getOrigX() - 1, 
													n->getOrigY() + n->getBaseLine(),
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

Node* Expression::getLeftSibling(Node* node)
{
	for (int i = 1; i < terms.size(); ++i) {
		if (terms[i] == node)
			return terms[i - 1];
	}
	return nullptr;
}

Node* Expression::getRightSibling(Node* node)
{
	for (int i = 0; i < terms.size() - 2; ++i) {
		if (terms[i] == node) 
			return terms[i + 1];
	}
	return nullptr;
}

void Expression::deleteNode(Node* node)
{
	for (auto pos = terms.begin(); pos != terms.end(); ++pos) {
		if ( *pos == node ) terms.erase(pos);
	}
}

int Input::input_sn = -1;

Input::Input(Equation& eqn, std::string txt, bool current, Node* parent, bool neg, Node::Select s) :
	Node(parent, neg, s), m_typed(txt), m_sn(++input_sn), m_active(true), m_current(current), m_eqn(eqn)
{
	eqn.addInput(this);
	if (current) eqn.setCurrentInput(m_sn);
}

void Input::calcSize()
{
	setSize(m_typed.length() + 1, 1);
	setBaseLine(0);	
}

void Input::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Input::asciiArt(Draw& draw) const
{
	string in = toString();
	if (in.length() == 1) {
		draw.at(getOrigX(), getOrigY(), in[0]);
	}
	else {
		in.erase(in.begin()); in.erase(in.end()-1);
		draw.at(getOrigX(), getOrigY(), in);
	}
}

string Input::toString() const
{
	if (m_active) {
		if (m_typed.empty()) return (m_current ? "#" : "?");
		return "[" + m_typed  + "]";
	}
	else
		return m_typed;
}

bool Input::handleBackspace() 
{ 
	if (!m_typed.empty()) {
		m_typed.pop_back();    
		return true;
	}
	if (!m_left.empty() && m_left.back()->isLeaf()) {
		m_left.pop_back();
		return true;
	}
	return false;
}

bool Input::handleChar(int ch)
{
	if (isalnum(ch) || ch == '.') {
		addTyped(ch);
		return true;
	}
	bool fResult = true;
	string op(1, (char)ch);
	switch (ch) {
	    case Key::BACKSPACE:
			      if (!handleBackspace()) fResult = m_eqn.handleBackspace();
			      break;
	    case '+':
	    case '-': if (m_typed.empty()) m_typed = "?";
                  m_typed += op + "#";
		          break;
        case '/':
	    case '^': if (m_typed.empty()) m_typed = "?";
                  m_typed = "(" + m_typed + ")" + op + "(#)";
			      break;
        case '(': m_typed += "(#)";
			      break;
	    default:  fResult = false;
		 	      break;
	}
	return fResult;
}

void Input::swapLeftTerm(Node* node)
{ 
	Term* term = dynamic_cast<Term*>(node);
	if (term != nullptr) m_left.swap(term->factors); 
}

bool Equation::handleChar(int ch)
{
	bool fResult = true;
	switch(ch) {
 	    case ' ': if (m_selectStart != nullptr) {
			          if (m_selectStart->getParent() == nullptr) return false;

			          m_selectStart->setSelect(Node::Select::NONE);
					  m_selectEnd->setSelect(Node::Select::NONE);

					  Node* parent = m_selectStart->getParent();
					  while (parent && !parent->isFactor()) { parent = parent->getParent(); }
					  if (parent == nullptr) return false;

					  m_selectStart = parent;
			          m_selectEnd = m_selectStart;
					  m_selectStart->setSelect(Node::Select::ALL);
					  LOG_TRACE_MSG("current selection: " + m_selectStart->toString());
		          }
		          else if (getCurrentInput() == nullptr || getCurrentInput()->toString().length() == 1) {
					  return false;
				  }
				  else {
					  m_selectStart = getCurrentInput()->getParent();
					  getCurrentInput()->disable();
			          m_selectEnd = m_selectStart;
					  m_selectStart->setSelect(Node::Select::ALL);
				  }
			      break;
	    default:  fResult = false;
		 	      break;
	}
	return fResult;
}

void Equation::nextInput() 
{ 
	m_inputs[m_input_index]->setCurrent(false);
	m_input_index = (++m_input_index)%m_inputs.size();
	m_inputs[m_input_index]->setCurrent(true);
}

bool Equation::disableCurrentInput()
{
	bool fResult = false;
	Input* current = m_inputs[m_input_index];
	if ( m_inputs.size() > 1 && !current->m_typed.empty() ) {
		current->disable();
		nextInput();
		fResult = true;
	}
	return fResult;
}

bool Equation::handleBackspace()
{
	Input* in = getCurrentInput();
	Expression* parent = dynamic_cast<Expression*>(in->getParent());

	Node* leftNode = in->getNextLeft();
	if (!leftNode && leftNode->getParent() != in->getParent()) leftNode = nullptr;

	if (in->m_left.empty() && leftNode == nullptr) return false;
		
	if (in->m_left.empty()) {
		if (typeid(leftNode) == typeid(in)) { // left is Input*
			Input* left_in = dynamic_cast<Input*>(leftNode);
			if (left_in->m_right.empty()) {
				in->m_left.swap(left_in->m_left);
				left_in->disable();
			}
			else {
				in->m_left.swap(left_in->m_right);
			}
		}
		else { // left is Term*
			in->swapLeftTerm(leftNode);
			parent->deleteNode(leftNode);
		}
		return true;
	}
	else {
		setCurrentInput(-1);
		Node* sel = in->m_left.back();
		in->m_left.reserve(in->m_left.size() + in->m_right.size());
		in->m_left.insert(in->m_left.end(), in->m_right.begin(), in->m_right.end());
		Node* new_term = new Term(in->m_left, in->getParent());
		parent->replaceNode(in, new_term);
		setSelectStart(sel);
		setSelectEnd(sel);
		return true;
	}
	return false;
}

void EqnUndoList::save(Equation* eqn)
{ 
	string store;
	eqn->xml_out(store);
	LOG_TRACE_MSG("saved eqn xml:\n" + store);
	m_eqns.push_back(store);
}

Equation* EqnUndoList::undo()
{
	if (m_eqns.size() <= 1) return nullptr;
	m_eqns.pop_back();
	return top();
}

Equation* EqnUndoList::top()
{
	if (m_eqns.empty()) return nullptr;
	istringstream in(m_eqns.back());
	return new Equation(in);
}

namespace Log
{
	void msg(string m)
	{
		char timestamp[256];
		time_t now = time(NULL);
		struct tm* local = localtime(&now);
		strftime(timestamp, 256, "%F %T: ", local);
		fstream out(LOG_TRACE_FILE, fstream::out | fstream::app);
		out << timestamp << m << endl; 
		out.close();
	}
}
