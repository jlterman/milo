#include <iostream>
#include <vector>
#include <iterator>
#include <exception>
#include <map>
#include <limits>
#include <initializer_list>

#include "milo.h"
#include "milo_key.h"
#include "nodes.h"

using namespace std;

void Equation::asciiArt(Draw& draw) const
{
	m_root->calcTermSize();
	m_root->calcTermOrig(0, 0);
	draw.set(m_root->getTermSizeX(), m_root->getTermSizeY());
	m_root->asciiArt(draw);
}

void Equation::setCurrentInput(int in_sn)
{
	if (m_input_index >= 0) m_inputs[m_input_index]->m_current = false;
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
	draw.horiz_line(getTermSizeX(), getTermOrigX(), getTermOrigY() + getBaseLine());
	m_first->asciiArt(draw);
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

void Function::init_functions()
{
	functions.emplace("sin", &sinZ);
	functions.emplace("cos", &cosZ);
	functions.emplace("tan", &tanZ);
	functions.emplace("log", &logZ);
	functions.emplace("exp", &expZ);
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
	draw.at(getTermOrigX(), getTermOrigY() + getBaseLine(), m_name, Color::GREEN);
	m_arg->asciiArt(draw);
}

Constant::const_map Constant::constants;

void Constant::init_constants()
{
	constants.emplace('e', Complex(exp(1.0), 0));
	constants.emplace('P', Complex(4*atan(1.0), 0));
	constants.emplace('i', Complex(0, 1));
}

void Constant::calcTermSize() 
{
	setTermSize(1, 1);
	setBaseLine(0);
}

void Constant::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
}

void Constant::asciiArt(Draw& draw) const
{
	draw.at(getTermOrigX(), getTermOrigY(), m_name);
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

void Number::calcTermSize() 
{
	string n = toString();
	m_imag_pos = n.find('i');
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

	if (m_imag_pos != string::npos) 
		draw.at(getTermOrigX() + m_imag_pos, getTermOrigY(), 'i', Color::RED);
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

void Expression::calcTermSize() 
{
	int x = 0, b = 0, y = 0;
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	for ( auto n : terms ) { 
		if (n != terms[0] || !n->getSign()) x += 1;
		n->calcTermSize(); 
		x += n->getTermSizeX();
		y = max(y, n->getTermSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	setTermSize(x, y + b);
	setBaseLine(b);
}

void Expression::calcTermOrig(int x, int y)
{
	setTermOrig(x, y);
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) x+=1;
		n->calcTermOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getTermSizeX();
	}
}

void Expression::asciiArt(Draw& draw) const
{
	if (getParent() && getParent()->drawParenthesis()) draw.parenthesis(this);
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) draw.at(n->getTermOrigX() - 1, 
													n->getTermOrigY() + n->getBaseLine(),
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

Input::Input(Equation& eqn, std::string txt, Node* parent, bool neg, bool current) :
	Node(parent), m_typed(txt), m_sn(++input_sn), m_active(true), m_current(current)
{
	eqn.addInput(this);
	if (current) eqn.setCurrentInput(m_sn);
	if (neg) negative();
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
	string in = toString();
	if (in.length() == 1) {
		draw.at(getTermOrigX(), getTermOrigY(), in[0]);
	}
	else {
		in.erase(in.begin()); in.erase(in.end()-1);
		draw.at(getTermOrigX(), getTermOrigY(), in);
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

bool Input::handleChar(int ch)
{
	if (isalnum(ch) || ch == '.') {
		addTyped(ch);
		return true;
	}
	bool fResult = true;
	string op(1, (char)ch);
	switch (ch) {
	    case Key::BACKSPACE: {
			delTyped();
			break;
		}
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

void EqnUndoList::save(Equation* eqn)
{ 
	string store;
	eqn->xml_out(store);
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
		fstream out("/tmp/milo.log", fstream::out | fstream::app);
		out << timestamp << m << endl; 
		out.close();
	}
}
