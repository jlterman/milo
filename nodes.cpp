/* Copyright (C) 2016 - James Terman
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
 * @file nodes.cpp
 * This file contains the implementation of member functions of classes 
 * derived from Node not involved in construction from a parser, or 
 * symbolic maninpulation. Mostly it is support for the GUI.
 */

#include <iostream>
#include <vector>
#include <iterator>
#include <map>
#include <limits>
#include <typeinfo>

#include "milo.h"
#include "nodes.h"
#include "ui.h"

using namespace std;


Node::Frame Divide::calcSize(Graphics& gc)
{
	m_first->calculateSize(gc);
	m_second->calculateSize(gc);
	Frame frame = { 
		{ max(m_first->getFrame().box.width(), m_second->getFrame().box.width()),
		  m_first->getFrame().box.height() + gc.getDivideLineHeight() + m_second->getFrame().box.height(), 
		  0, 0
		},
		m_first->getFrame().box.height() + gc.getDivideLineHeight()/2
	};
	m_internal = frame.box;
    return frame;
}

void Divide::calcOrig(Graphics& gc, int x, int y)
{
	m_internal.setOrigin(x, y);
	m_first->calculateOrigin(gc,  x + (m_internal.width() - m_first->getFrame().box.width())/2, 
							      y + getFrame().base - m_first->getFrame().box.height());
	m_second->calculateOrigin(gc, x + (m_internal.width() - m_second->getFrame().box.width())/2,
							      y + getFrame().base + gc.getDivideLineHeight());
}


void Divide::drawNode(Graphics& gc) const
{
	gc.horiz_line(m_internal.width(), m_internal.x0(), m_internal.y0() + getFrame().base);
	m_first->draw(gc);
	m_second->draw(gc);
}

Complex Divide::getNodeValue() const
{
	Complex a = m_first->getValue();
	Complex b = m_second->getValue();
	return a / b;
}

Node::Frame Power::calcSize(Graphics& gc)
{
	m_first->calculateSize(gc);
	m_second->calculateSize(gc);
	Frame frame = { 
		{ m_first->getFrame().box.width() + m_second->getFrame().box.width(),
		  m_first->getFrame().box.height() + 
		  m_second->getFrame().box.height() - 
		  gc.getTextHeight()/2, 0, 0
		},
		m_second->getFrame().box.height() - gc.getTextHeight()/2
	};
	m_internal = frame.box;
    return frame;
}

void Power::calcOrig(Graphics& gc, int x, int y)
{
	m_internal.setOrigin(x, y);
	m_first->calculateOrigin(gc, x, y + getFrame().base);
	m_second->calculateOrigin(gc, x + m_first->getFrame().box.width(), 
							      y + getFrame().base - m_second->getFrame().box.height() 
                                    + gc.getTextHeight()/2);
}

void Power::drawNode(Graphics& gc) const
{
	m_first->draw(gc);
	m_second->draw(gc);
}

Complex Power::getNodeValue() const
{
	Complex a = m_first->getValue();
	Complex b = m_second->getValue();
	return pow(a, b);
}

Complex Function::sinZ(Complex z)
{
	return sin(z);
}

Complex Function::cosZ(Complex z)
{
	return cos(z);
}

Complex Function::tanZ(Complex z)
{
	return tan(z);
}

Complex Function::logZ(Complex z) {
	if (isZero(z)) {
		return Complex(-1*numeric_limits<float>::infinity(), 0);
	}
	return log(z);
}

Complex Function::expZ(Complex z) {
	return exp(z);
}

const Function::func_map Function::functions = {
	{ "sin", &sinZ },
	{ "cos", &cosZ },
	{ "tan", &tanZ },
	{ "log", &logZ },
	{ "exp", &expZ }
};

Node::Frame Function::calcSize(Graphics& gc) 
{
	m_arg->calculateSize(gc);
	Frame frame = { 
		{ gc.getTextLength(m_name) + m_arg->getFrame().box.width(),
		  m_arg->getFrame().box.height(), 0, 0
		},
		m_arg->getFrame().box.height()/2
	};
	m_internal = frame.box;
    return frame;
}

void Function::calcOrig(Graphics& gc, int x, int y)
{
	m_internal.setOrigin(x, y);
	m_arg->calculateOrigin(gc, x + gc.getTextLength(m_name), 
						       y + getFrame().base - m_arg->getFrame().base);
}

void Function::drawNode(Graphics& gc) const
{
	gc.at(m_internal.x0(), m_internal.y0(), m_name,
		  Graphics::Attributes::NONE, Graphics::Color::GREEN);
	m_arg->draw(gc);
}

Complex Function::getNodeValue() const
{
	Complex arg = m_arg->getValue();
	return m_func(arg);
}

Node::Frame Differential::calcSize(Graphics& gc)
{
	m_function->calculateSize(gc);
	Frame frame = {
		{ gc.getDifferentialWidth(m_variable) + m_function->getFrame().box.width(),
		  max(gc.getDifferentialHeight(m_variable), m_function->getFrame().box.height()), 0, 0
		},
		max(gc.getDifferentialBase(m_variable), m_function->getFrame().base)
	};
	m_internal = frame.box;
	return frame;
}

void Differential::calcOrig(Graphics& gc, int x, int y)
{
	m_internal.setOrigin(x, y);
	m_function->calculateOrigin(gc, x + gc.getDifferentialWidth(m_variable),
								    y + getFrame().base - m_function->getFrame().base);
}

void Differential::drawNode(Graphics& gc) const 
{
	gc.differential(m_internal.x0(), 
					m_internal.y0() + getFrame().base - gc.getDifferentialBase(m_variable),
					m_variable);
	m_function->draw(gc);
}

Node* Differential::findNode(const Box& b)
{
	if (getFrame().box.inside(b))
		return this;
	else
		return m_function->findNode(b);
}

const Constant::const_map Constant::constants = {
	{ 'e', Complex(exp(1.0), 0) },
	{ 'P', Complex(4*atan(1.0), 0) },
	{ 'i', Complex(0, 1) },
};

Node::Frame Constant::calcSize(Graphics& gc) 
{
    Frame frame = { { gc.getCharLength(m_name), gc.getTextHeight(), 0, 0 }, 0 };
	m_internal = frame.box;
    return frame;
}

void Constant::calcOrig(Graphics&, int x, int y)
{
	m_internal.setOrigin(x, y + getFrame().base);
}

void Constant::drawNode(Graphics& gc) const
{
	gc.at(m_internal.x0(), m_internal.y0(), m_name,
		  Graphics::Attributes::ITALIC, Graphics::Color::RED);
}

Variable::var_map Variable::values;

Node::Frame Variable::calcSize(Graphics& gc) 
{
	Frame frame = { { gc.getCharLength(m_name), gc.getTextHeight(), 0, 0 }, 0 };
	m_internal = frame.box;
    return frame;
}

void Variable::calcOrig(Graphics&, int x, int y)
{
	m_internal.setOrigin(x, y + getFrame().base);
}

void Variable::drawNode(Graphics& gc) const
{
	gc.at(m_internal.x0(), m_internal.y0(), m_name, Graphics::Attributes::ITALIC);
}

void Variable::setValue(char name, Complex value)
{
	auto it = values.find(name);
	if (it != values.end() ) it->second = value;
}

string Number::toString() const
{
	if (m_isInteger) return to_string((int) m_value);

	string n = to_string(m_value);
	if (n.find('.') == string::npos) throw logic_error("bad number format");
	size_t pos;
	if ((pos = n.find_last_of('e')) != string::npos || 
		(pos = n.find_last_of('E')) != string::npos ) {
		--pos;
	}
    else {
		pos = n.length() - 1;
	}
	while (n[pos] == '0') n.erase(pos--, 1);
	return n;
}

Node::Frame Number::calcSize(Graphics& gc) 
{
	Frame frame = { { gc.getTextLength(toString()), gc.getTextHeight(), 0, 0 }, 0 };
	m_internal = frame.box;
    return frame;
}

void Number::calcOrig(Graphics&, int x, int y)
{
	m_internal.setOrigin(x, y + getFrame().base);
}

void Number::drawNode(Graphics& gc) const
{
	gc.at(m_internal.x0(), m_internal.y0(), toString(), Graphics::Attributes::NONE);
}

Node::Frame Term::calcSize(Graphics& gc) 
{
	int x = 0, b = 0, y = 0;
	for ( auto n : factors ) { 
		if (n->getType() == Expression::type) n->setDrawParenthesis(true);
		n->calculateSize(gc); 
		x += n->getFrame().box.width();
		y = max(y, n->getFrame().box.height() - n->getFrame().base);
		b = max(b, n->getFrame().base);
	}
    Frame frame = { { x, b + y, 0, 0 }, b };
	m_internal = frame.box;
	return frame;
}

void Term::calcOrig(Graphics& gc, int x, int y)
{
	for ( auto n : factors ) {
		n->calculateOrigin(gc, x, y + getFrame().base - n->getFrame().base);
		x += n->getFrame().box.width();
	}
}

void Term::drawNode(Graphics& gc) const
{
	for ( auto n : factors ) n->draw(gc);
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
	for (unsigned int i = 1; i < factors.size(); ++i) {
		if (factors[i] == node)
			return factors[i - 1];
	}
	return nullptr;
}

Node* Term::getRightSibling(Node* node)
{
	for (unsigned int i = 0; i < factors.size() - 1; ++i) {
		if (factors[i] == node) 
			return factors[i + 1];
	}
	return nullptr;
}

Complex Term::getNodeValue() const
{
	Complex value = 1;
	for (auto n : factors) { value *= n->getValue(); }
	return value;
}

NodeIter Term::pos(Node* me)
{
	if (me->getParent() == nullptr || me->getParent()->getType() != Term::type) throw logic_error("bad parent");
	Term* term = dynamic_cast<Term*>(me->getParent());
	return find(term->factors.begin(), term->factors.end(), me);
}

Node::Frame Expression::calcSize(Graphics& gc) 
{
	int x = 0, b = 0, y = 0;
	for ( auto n : terms ) { 
		if (n != terms[0] || !n->getSign()) x += gc.getCharLength('-');
		n->calculateSize(gc); 
		x += n->getFrame().box.width();
		y = max(y, n->getFrame().box.height() - n->getFrame().base);
		b = max(b, n->getFrame().base);
	}
	Frame frame = { { x, b + y, 0, 0 }, b };
	m_internal = frame.box;
	return frame;
}

void Expression::calcOrig(Graphics& gc, int x, int y)
{
	m_internal.setOrigin(x, y);
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) x += gc.getCharLength('-');
		n->calculateOrigin(gc, x, y + getFrame().base - n->getFrame().base);
		x += n->getFrame().box.width();
	}
}

void Expression::drawNode(Graphics& gc) const
{
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) {
			gc.at(n->getFrame().box.x0() - 1, 
				  n->getFrame().box.y0() + n->getFrame().base,
				  n->getSign() ? '+' : '-', Graphics::Attributes::NONE);
		}
		n->draw(gc); 
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

Complex Expression::getNodeValue() const
{
	Complex value = 0;
	for (auto n : terms) { value += n->getValue(); }
	return value;
}

Node* Expression::getLeftSibling(Node* node)
{
	for (unsigned int i = 1; i < terms.size(); ++i) {
		if (terms[i] == node)
			return terms[i - 1]->last();
	}
	return nullptr;
}

Node* Expression::getRightSibling(Node* node)
{
	for (unsigned int i = 0; i < terms.size() - 1; ++i) {
		if (terms[i] == node)
			return terms[i + 1]->first();
	}
	return nullptr;
}

int Input::input_sn = -1;

Input::Input(Equation& eqn, std::string txt, bool current, Node* parent, bool neg, Node::Select s) :
	Node(parent, neg, s), m_sn(++input_sn), m_typed(txt), m_current(current), m_eqn(eqn)
{
	eqn.addInput(this);
	if (current) eqn.setCurrentInput(m_sn);
}

Node::Frame Input::calcSize(Graphics& gc)
{
	Frame frame = { { gc.getTextLength(m_typed) + gc.getCharLength('?'), gc.getTextHeight(), 0, 0}, 0};
	m_internal = frame.box;
    return frame;
}

void Input::calcOrig(Graphics&, int x, int y)
{
	m_internal.setOrigin(x, y);
}

void Input::drawNode(Graphics& gc) const
{
	for (size_t i = 0; i < m_typed.length(); ++i) {
		gc.at(m_internal.x0() + i, m_internal.y0(), m_typed.at(i), Graphics::Attributes::ITALIC);
	}
	gc.at(m_internal.x0() + m_typed.length(), m_internal.y0(), '?', Graphics::Attributes::BOLD_ITALIC); 
}

string Input::toString() const
{
	if (m_typed.empty()) 
		return (m_current ? "#" : "?");
	else 
		return "[" + m_typed  + "]";
}

Complex Input::getNodeValue() const
{
	throw logic_error("input has no value");
}

FactorIterator Input::emptyBuffer()
{
	FactorIterator pos(this);
	if (!m_typed.empty()) {
		m_eqn.insert(pos, m_typed);
		m_typed.clear();
		pos = FactorIterator(this);
	}
	return pos;
}


/** Function pointer that takes equation and creates a node at Equation's current input.
 */
typedef bool (*create_ptr)(Equation&);

/** @brief Map that associates a name with a static function that creates node.
 */
using create_map = std::map<std::string, create_ptr>;

static const create_map createNodes = {
	{ Divide::name, &Divide::create },
	{  Power::name, &Power::create  }
};

bool Node::createNodeByName(string name, Equation& eqn)
{
	return createNodes.at(name)(eqn);
}

bool Divide::create(Equation& eqn)
{
	Input* in = eqn.getCurrentInput();
	if (in == nullptr) return false;
	
	FactorIterator in_pos(in);
	auto pos = in_pos;
	bool fNeg = !in->getParent()->getSign();
	if (fNeg) in->getParent()->negative();
	Term* upper_term = dynamic_cast<Term*>(in->getParent());
	
	in_pos = in->emptyBuffer();
	if (in_pos.isBeginTerm() && in->empty()) {
		in_pos.insert(new Input(eqn));
		++in_pos;
		in->makeCurrent();
	}
	Term* lower_term = in_pos.splitTerm();
	Expression* upper = new Expression(upper_term);
	Expression* lower = new Expression(lower_term);
	
	Divide* d = new Divide(upper, lower, nullptr);
	Term* divide_term = new Term(d, nullptr, fNeg);
	pos.replace(divide_term, false);
	return true;
}

bool Power::create(Equation& eqn)
{
	Input* in = eqn.getCurrentInput();
	if (in == nullptr) return false;
	
	FactorIterator in_pos(in);
	Node* a_factor = nullptr;
	Expression* a = nullptr;
	if (!in->empty()) {
		in_pos = eqn.disableCurrentInput();
		a_factor = *in_pos;
	}
	else {
		a_factor = in;
	}
	a = new Expression(a_factor);
	
	Expression* b = new Expression(new Input(eqn));
	Power* p = new Power(a, b, in->getParent());
	in_pos.replace(p, false);
	return true;
}

Node* Function::findNode(int x, int y)
{
	return m_arg->findNode(x, y);
}

Node* Function::findNode(const Box& b)
{
	if (getFrame().box.inside(b))
		return this;
	else
		return m_arg->findNode(b);
}

Node* Binary::findNode(int x, int y)
{
	Node* node = m_first->findNode(x, y);
	if (node == nullptr) return m_second->findNode(x, y);
	return node;
}

Node* Binary::findNode(const Box& b)
{
	if (getFrame().box.inside(b))
		return this;
	else if (m_first->getFrame().box.intersect(b))
		return m_first->findNode(b);
	else
		return m_second->findNode(b);
}

Node* Term::findNode(int x, int y)
{
	Node* node = nullptr;
	for ( auto n : factors ) {
		node = n->findNode(x, y);
		if (node != nullptr) break;
	}
	return node;
}

Node* Term::findNode(const Box& b)
{
	Node* node = nullptr;
	for ( auto n : factors ) {
		node = n->findNode(b);
		if (node != nullptr) break;
	}
	return node;
}

int Term::numFactors() const
{ 
	int n = 0; 
	for ( auto f : factors ) { n += f->numFactors(); }
	return n;
}

Node* Expression::findNode(int x, int y)
{
	Node* node = nullptr;
	for ( auto n : terms ) {
		node = n->findNode(x, y);
		if (node != nullptr) break;
	}
	return node;
}

Node* Expression::findNode(const Box& b)
{
	if (getFrame().box.inside(b)) return this;
	
	Node* node = nullptr;
	for ( auto n : terms ) {
		node = n->findNode(b);
		if (node != nullptr) break;
	}
	return node;
}

int Expression::numFactors() const
{ 
	int n = 0; 
	for ( auto t : terms ) { n += t->numFactors(); }
	return n;
}
