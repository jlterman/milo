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

Input::Input(Equation& eqn, std::string txt, Node* parent, bool neg, bool current) :
	Node(parent), m_typed(txt), m_sn(++input_sn), m_active(true), m_current(current)
{
	eqn.addInput(this);
	if (current) eqn.setCurrentInput(m_sn);
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
