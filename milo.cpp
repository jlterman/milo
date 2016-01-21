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
 * @file milo.cpp
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
#include "milo_key.h"
#include "nodes.h"

using namespace std;

/* Take the selection state and draw it in the graphics context.
 */
void Equation::setSelect(Graphics& gc)
{
	if (m_selectStart == nullptr && m_selectEnd == nullptr) {
		/* No selection. Clear it.
		 */
		gc.setSelect(0, 0, 0, 0);
	}
	else if (m_selectStart != nullptr && m_selectStart == m_selectEnd) {
		/* On one node selected.
		 */
		gc.setSelect(m_selectStart->getFrame().box);
	}
	else if (m_selectStart != nullptr) {
		/* Multiple nodes selected. Figure out bounding box.
		 */
		auto it = FactorIterator(m_selectStart);
		auto end = FactorIterator(m_selectEnd);
		int x0 = it->getFrame().box.x0(), y0 = 0, x = 0, y = 0;
		do {
			x = it->getFrame().box.x0() - x0 + it->getFrame().box.width();
			int y_new = it->getFrame().box.height();
			if (y_new > y) {
				y = y_new;
				y0 = it->getFrame().box.y0();
			}
		}
		while (it++ != end);

		gc.setSelect(x, y, x0, y0);
	}
}

/* Clear old input and set the new one active.
 */
void Equation::setCurrentInput(int in_sn)
{
	if (m_input_index >= 0) m_inputs[m_input_index]->m_current = false;
	if (in_sn < 0) {
		m_input_index = -1;
		return;
	}
	for (unsigned int i = 0; i < m_inputs.size(); ++i) {
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
	return isInteger(stod(s));
}

bool isInteger(double value)
{
	return isZero(value - ((int) value));	
}

const vector<string> Node::select_tags = { "NONE", "START", "END", "ALL" };

Node* Node::first()
{
	Node* node = this;
	Node* down = this->downLeft();
	while (down != nullptr) {
		node = down;
		down = node->downLeft();
	}
	return (down != nullptr) ? down : node;
}

Node* Node::last()
{
	Node* node = this;
	Node* down = this->downRight();
	while (down != nullptr) {
		node = down;
		down = node->downRight();
	}
	return (down != nullptr) ? down : node;
}

// Return node to the left. Go up if necessary
Node* Node::getNextLeft()
{
	Node* left = (m_parent) ? m_parent->getLeftSibling(this) : nullptr;
	if (left != nullptr) { 
		return left->last();
	}
	else if (m_parent != nullptr) {
		return m_parent->getNextLeft();
	}
	else
		return nullptr;
}

// Return node to the right. Go down if necessary
Node* Node::getNextRight()
{
	Node* right = (m_parent) ? m_parent->getRightSibling(this) : nullptr;
	if (right != nullptr) { 
		return right->first();
	}
	else if (m_parent != nullptr) {
		return m_parent->getNextRight();
	}
	else
		return nullptr;
}

Complex Node::getValue() const
{
	Complex z(1, 0);
	for (int i = 0; i < m_nth; ++i) { z *= getNodeValue(); }
	if (!m_sign && ((m_nth&1) == 1)) z *= Complex(-1, 0);
	return z;
}

void Node::calculateSize(Graphics& gc)
{
	Node::Frame frame = calcSize(gc);

	m_fDrawParenthesis |= (isFactor() && !m_sign) || (!isLeaf() && m_nth != 1);
	if (m_fDrawParenthesis)    frame.box.width() += 2*gc.getParenthesisWidth();
	if (isFactor() && !m_sign) frame.box.width() += gc.getCharLength('-');

	m_parenthesis.setSize(frame.box.width(), frame.box.height());
	if (m_nth != 1) {
		string n = to_string(m_nth);
		frame.box.width() += gc.getTextLength(n);
		frame.box.height() += gc.getTextHeight();
		frame.base += gc.getTextHeight();
	}

	m_frame = frame;
}

void Node::calculateOrigin(Graphics& gc, int x, int y)
{
	m_frame.box.setOrigin(x, y);
	if (m_nth == 1) m_parenthesis.setOrigin(x, y); else m_parenthesis.setOrigin(x, y + gc.getTextHeight());
	if (m_fDrawParenthesis)    x += gc.getParenthesisWidth();
	if (isFactor() && !m_sign) x += gc.getCharLength('-');

	calcOrig(gc, x, y);
}

// Draw this node and subtree in this graphic context
void Node::draw(Graphics& gc) const
{
	if (m_fDrawParenthesis)  gc.parenthesis(m_parenthesis);
	if (isFactor() && !m_sign) {
		gc.at(m_frame.box.x0() + ( m_fDrawParenthesis ? gc.getParenthesisWidth() : 0 ), 
			  m_frame.box.y0() + m_frame.base, '-', Graphics::Attributes::NONE);
	}

	if (m_nth != 1) {
		string n = to_string(m_nth);
		gc.at(m_frame.box.x0() + m_frame.box.width() - gc.getTextLength(n),
			  m_frame.box.y0(), n, Graphics::Attributes::NONE);
	}
	drawNode(gc);	
}

// Set up drawing for this node and its subtree
void Node::setUpDraw(Graphics& gc)
{
	calculateSize(gc);
	calculateOrigin(gc, 0, 0);
	gc.set(m_frame.box);
}

void Equation::draw(Graphics& gc)
{
	m_root->setUpDraw(gc);
	setSelect(gc);
	m_root->draw(gc);
}


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
	gc.at(m_internal.x0(), m_internal.y0(), m_typed, Graphics::Attributes::ITALIC);
	gc.at(m_internal.x0() + m_typed.length(), m_internal.y0(), '?', Graphics::Attributes::BOLD); 
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

void Equation::eraseSelection(Node* node)
{
	if (m_selectStart == m_root) {
		delete m_root;
		if (node == nullptr) node = new Input(*this, string(), true);
		m_root = new Expression(node);
	}
	else if (m_selectStart == m_selectEnd && m_selectStart->isFactor()) {
		auto it = FactorIterator(m_selectStart);
		it = insert(it, node);
		erase(it);
	}
	else if (m_selectStart == m_selectEnd && node != nullptr) {
		Term* term = dynamic_cast<Term*>(m_selectStart);
		freeVector(term->factors);
		term->factors.push_back(node);
	}
	else if (m_selectStart != m_selectEnd) {
		auto end = FactorIterator(m_selectEnd);
		auto it = FactorIterator(m_selectStart);
		for (it = erase(it); it != end; it = forward_erase(it));
		if (node == nullptr) erase(it);
		                else replace(it, node);
	}
	clearSelect();
}

bool Equation::handleChar(int ch)
{
	string store;
	this->xml_out(store);
	LOG_TRACE_MSG("handleChar eqn xml:\n" + store);

	bool fResult = true;
	if (m_selectStart != nullptr) {
		if (isalnum(ch) || ch == '.') {
			eraseSelection(new Input(*this, string(1, (char)ch), true));
			return true;
		}
		switch(ch) {
		    case Key::BACKSPACE: {
				eraseSelection(new Input(*this, string(), true));
				break;
			}
		    case Key::LEFT: {
				FactorIterator n{m_selectStart};
				if (!n.isBegin() ) {
					--n;
					setSelect(*n);
				}
				else
					fResult = false;
				break;
			}
		    case Key::RIGHT: {
				FactorIterator n{m_selectStart};
				++n;
				if (!n.isEnd()) setSelect(*n); else fResult = false;
				break;
			}
		    case Key::SRIGHT:
		    case Key::SLEFT: {
				auto start = FactorIterator(m_selectStart);
				auto end = FactorIterator(m_selectEnd);
				auto last = end.getLast();

				if ((ch == Key::SLEFT && start.isBegin()) ||
					(ch == Key::SRIGHT && (end == last))) {
					fResult = false;
				}
				else if (ch == Key::SLEFT) {
					--start;
				}
				else if (ch == Key::SRIGHT) {
					++end;
				}
				setSelect(*start, *end);
				break;
			}
		    case Key::UP: {
				NodeIterator n{m_selectStart};
				if (n != begin() ) {
					--n;
					setSelect(*n);
				}
				else
					fResult = false;
				break;
			}
		    case Key::DOWN: {
				NodeIterator n{m_selectStart};
				++n;
				if (n != end()) setSelect(*n); else fResult = false;
				break;
			}
		    case 10:
		    case 13: {
				auto it = m_selectStart;
				clearSelect();
				insert(it, new Input(*this, string(), true));
				break;
			}
	        case ' ': {
				if (m_selectStart->getParent() == nullptr) return false;

				Node* parent = nullptr;
				int num = 0;
				if (m_selectStart != m_selectEnd) {
					FactorIterator end(m_selectEnd);
					for (auto it = FactorIterator(m_selectStart); it != end; ++it) {
						num += it->numFactors();
					}
					parent = m_selectStart->getParent()->getParent();
				}
				else {
					num = m_selectStart->numFactors();
					parent = m_selectStart->getParent();
				}

				while (parent && parent->numFactors() == num) { parent = parent->getParent(); }
				if (parent == nullptr) return false;
				setSelect(parent);
				break;
			}
            default:
			   fResult = false;
			   break;
		}
	}
	else if (m_input_index >= 0) {
		Input* in = getCurrentInput();
		FactorIterator in_pos(in);
		if (isalnum(ch) || ch == '.') {
			in->m_typed += string(1, (char)ch);
			return true;
		}
		switch(ch) {
	        case 9: {
				if (m_inputs.size() > 1) nextInput();
				                    else fResult = false;
				break;
			}
		    case 10:
	        case 13: {
				disableCurrentInput();
				break;
			}
	        case Key::BACKSPACE: {
				Input* in = getCurrentInput();
				if (!in->m_typed.empty()) {
					in->m_typed.erase(in->m_typed.end() - 1);
				}
				else {
					if (in_pos.isBegin()) fResult = false;
					                 else back_erase(in_pos);
				}
			    break;
			}
	        case '+':
	        case '-': {
				if (in->m_typed.empty()) {
					insert(in_pos, Expression::getTerm(*this, string(1, (char)ch)+"#"));
				}
				else {
					bool sign = in->getParent()->getSign();
					insert(in_pos, Expression::getTerm(*this, in->m_typed), sign);
					in->m_typed.clear();
					if (!sign^(ch == '-')) in->getParent()->negative();
				}
				break;
			}
	        case '/': {
				bool fNeg = !in->getParent()->getSign();
				if (fNeg) in->getParent()->negative();

				if (!in->m_typed.empty()) in_pos = disableCurrentInput();
				Expression* upper = new Expression(in_pos.m_pTerm);
				Expression* lower = new Expression(new Input(*this, string(), true));
				
				Divide* d = new Divide(upper, lower, nullptr);
				Term* divide_term = new Term(d, nullptr, fNeg);
				replace(in_pos, divide_term, false);
				break;
			}
	        case '^': {
				Node* a_factor = nullptr;
				Expression* a = nullptr;
				if (!in->m_typed.empty()) {
					in_pos = disableCurrentInput();
					a_factor = *in_pos;
				}
				else {
					a_factor = in;
				}
				a = new Expression(a_factor);

				Expression* b = new Expression(new Input(*this, string(), true));
				Power* p = new Power(a, b, in->getParent());
				replace(in_pos, p, false);
				break;
			}
	        case '(': {
				in->m_typed += "(#)";

				disableCurrentInput();
				break;
			}
	        case ' ': {
				if (in->m_typed.empty()) {
					fResult = false;
				}
				else {
					auto pos = disableCurrentInput();
					setSelect(*pos);
				}
				break;
			}
	        default:  
				fResult = false;
				break;
		}
	}
	else {
		switch(ch) {
		    case Key::UP:
		    case Key::LEFT:
	        case ' ': {
				setSelect(m_root->first());
				break;
			}
		    case Key::RIGHT:
		    case Key::DOWN: {
				setSelect(m_root->last());
				break;
			}
            default:
			    fResult = false;
			    break;
		}
	}

	return fResult;
}

void Equation::nextInput() 
{ 
	m_inputs[m_input_index]->setCurrent(false);
	m_input_index = (m_input_index + 1)%m_inputs.size();
	m_inputs[m_input_index]->setCurrent(true);
}

FactorIterator Equation::disableCurrentInput()
{
	if (m_input_index < 0) throw logic_error("no current input to disable");

	Input* in = m_inputs[m_input_index];
	FactorIterator pos(in);
	if (!in->m_typed.empty()) {
		pos = insert(pos, in->m_typed);
	}
	removeInput(in);
	pos = erase(pos);
	return pos;
}

void Equation::removeInput(Input* in)
{
	int in_index =  distance(m_inputs.cbegin(), find(m_inputs, in));
	eraseElement(m_inputs, in_index);
	
	if (m_inputs.size() > 0 && in_index == m_input_index) {
		m_input_index = (m_input_index + 1)%m_inputs.size();
		m_inputs[m_input_index]->setCurrent(true);
	}
	else
		m_input_index = -1;
}

void Equation::getCursorOrig(int& x, int& y) 
{ 
	if (getCurrentInput() == nullptr) return;

	x = getCurrentInput()->getFrame().box.x0() + getCurrentInput()->getFrame().box.width(); 
	y = getCurrentInput()->getFrame().box.y0();
}

void Equation::clearSelect()
{
	if (m_selectStart != nullptr) m_selectStart->setSelect(Node::Select::NONE);
	if (m_selectEnd != nullptr) m_selectEnd->setSelect(Node::Select::NONE);
}

void Equation::setSelect(Node* start, Node* end)
{
	if (start == nullptr) return;
	clearSelect();

	if (end == nullptr || end == start) {
		m_selectStart = m_selectEnd = start;
		start->setSelect(Node::Select::ALL);
	}
	else {
		m_selectStart = start;
		m_selectEnd = end;
		start->setSelect(Node::Select::START);
		end->setSelect(Node::Select::END);
	}
}

void Equation::setSelectFromNode(Node* node)
{
	switch (node->getSelect()) {
	    case Node::Select::START: setSelectStart(node); break;
	    case Node::Select::END  : setSelectEnd(node);   break;
	    case Node::Select::ALL  : setSelectStart(node);
                     		      setSelectEnd(node);
						          break;
	    case Node::Select::NONE :
	                    default : break;
	}
}

Node* Equation::findNode(Graphics& gc, int x, int y)
{
	gc.relativeOrig(x, y);
	Node* node = m_root->findNode(x, y);
	if (node != nullptr && typeid(node) == typeid(Input)) {
		delete node;
		return m_root->findNode(x, y);
	}
	else
		return node;
}

Node* Equation::findNode(Graphics& gc, Box b)
{
	gc.relativeOrig(b.x0(), b.y0());
	return m_root->findNode(b);
}


void Equation::selectBox(Graphics& gc, Node* start, Box b)
{
	gc.relativeOrig(b.x0(), b.y0());
	if (start->getParent() == nullptr || start->getParent()->getType() != Term::type) {
		setSelect(start);
		return;
	}
	Node* end = nullptr;
	FactorIterator itr(start);
	while (!itr.isEnd() && (*itr)->getFrame().box.inside(b)) {
		end = *itr;
		++itr;
	}
	setSelect(start, end);
}

FactorIterator Equation::insert(FactorIterator it, Node* node)
{ 
	insertElement(it.m_pTerm->factors, it.m_factor_index, node);
	node->setParent(it.m_pTerm);
	it.m_node = node;
	return ++it;
}

FactorIterator Equation::insert(FactorIterator it, std::string text)
{
	NodeVector factors;
	factor(text, factors, it.m_pTerm);
	for ( auto f : factors ) { it = insert(it, f); }	
	return it;
}

FactorIterator Equation::insert(FactorIterator it, Term* term, bool sign)
{
	insertElement(it.m_gpExpr->terms, it.m_term_index, term);
	if (!sign) term->negative();
	term->setParent(it.m_gpExpr);
	++it.m_term_index;
	return it;
}

void Equation::replace(FactorIterator it, Node* node, bool free)
{
	if (free) delete it.m_pTerm->factors[it.m_factor_index];
	it.m_pTerm->factors[it.m_factor_index] = node;
	node->setParent(it.m_pTerm);
}

void Equation::replace(FactorIterator it, Term* term, bool free)
{ 
	if (free) delete it.m_gpExpr->terms[it.m_term_index];
	it.m_gpExpr->terms[it.m_term_index] = term;
	term->setParent(it.m_gpExpr);
}

FactorIterator Equation::erase(FactorIterator it, bool free)
{
	if (free) delete it.m_pTerm->factors[it.m_factor_index];
	eraseElement(it.m_pTerm->factors, it.m_factor_index);
	if (it.m_factor_index > 0) --it.m_factor_index;

	if (it.m_pTerm->factors.size() == 0) {
		if (it.m_gpExpr->terms.size() == 1) {
			it.m_gpExpr->terms[0]->factors.push_back(new Input(*this, string(), true));
			it.m_factor_index = 0;
		}
		else {
			if (free) delete it.m_gpExpr->terms[it.m_term_index];
			eraseElement(it.m_gpExpr->terms, it.m_term_index);
			if (it.m_term_index > 0) --it.m_term_index;
			it.m_pTerm = it.m_gpExpr->terms[it.m_term_index];
		}
	}
	it.m_node = it.m_pTerm->factors[it.m_factor_index];
	return it;
}

FactorIterator Equation::back_erase(FactorIterator it)
{
	if (it.isBegin()) return it;

	if (it.m_factor_index == 0) {
		it.m_factor_index = it.m_gpExpr->terms[it.m_term_index - 1]->factors.size();
		mergeVectors(it.m_gpExpr->terms[it.m_term_index - 1]->factors, 
					 it.m_gpExpr->terms[it.m_term_index]->factors);
		eraseElement(it.m_gpExpr->terms, it.m_term_index--);
	}
	return erase(--it);
}

FactorIterator Equation::forward_erase(FactorIterator it)
{
	return back_erase(++it);
}

Node* Node::findNode(int x, int y)
{
	if (getFrame().box.inside(x, y))
		return this;
	else
		return nullptr;
}

Node* Node::findNode(const Box& b)
{
	if (getFrame().box.inside(b))
		return this;
	else
		return nullptr;
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

void NodeIterator::next()
{
	if (m_node == nullptr) throw range_error("out of range");

	m_node = m_node->getNextRight();
}

void NodeIterator::prev()
{
	Node* prev_node = m_node->getNextLeft();
	if (prev_node == nullptr) throw range_error("out of range");

	m_node = prev_node;
}

FactorIterator::FactorIterator(Node* node) : m_node(node)
{
	if (m_node == nullptr) return;

	m_pTerm = dynamic_cast<Term*>(m_node->getParent());
	m_gpExpr = dynamic_cast<Expression*>(m_pTerm->getParent());
	m_factor_index = m_pTerm->getFactorIndex(m_node);
	m_term_index = m_gpExpr->getTermIndex(m_pTerm);
}

FactorIterator::FactorIterator(Expression* exp) : FactorIterator(exp->terms[0]->factors[0]) {}


void FactorIterator::getNode(int factor, int term)
{
	m_term_index = term + (term < 0 ? m_gpExpr->terms.size() : 0);
	m_pTerm = m_gpExpr->terms[m_term_index];

	m_factor_index = factor + (factor < 0 ? m_pTerm->factors.size() : 0);
	m_node = m_pTerm->factors[m_factor_index];
}

void FactorIterator::next()
{
	if (m_gpExpr == nullptr) throw logic_error("null iterator");

	if (++m_factor_index == m_pTerm->factors.size()) {
		m_factor_index = 0;
		++m_term_index;
	}
	if (m_term_index < m_gpExpr->terms.size()) {
		m_pTerm = m_gpExpr->terms[m_term_index];
		m_node = m_pTerm->factors[m_factor_index];
	}
	else if (m_term_index == m_gpExpr->terms.size()) {
		m_node = nullptr; 
		m_pTerm = m_gpExpr->terms.back();
		m_factor_index = m_pTerm->factors.size();
	}
	else
		throw range_error("out of range");
}

void FactorIterator::prev()
{
	if (m_gpExpr == nullptr) throw logic_error("null iterator");

	if (m_factor_index > 0) {
		--m_factor_index;
	}
	else if (m_term_index > 0) {
		--m_term_index;
		m_pTerm = m_gpExpr->terms[m_term_index];
		m_factor_index += m_pTerm->factors.size() - 1;
	}
	else 
		throw range_error("out of range");		

	m_node = m_pTerm->factors[m_factor_index];
}

bool FactorIterator::isEndTerm()
{ 
	if (m_gpExpr == nullptr) throw logic_error("null iterator");

	return m_factor_index == m_pTerm->factors.size() - 1;
}

void FactorIterator::erase()
{
	m_pTerm->factors.erase(m_pTerm->factors.begin() + m_factor_index);

	if (m_factor_index == m_pTerm->factors.size()) {
		m_factor_index = 0;
		++m_term_index;
	}
	if (m_term_index < m_gpExpr->terms.size()) {
		m_pTerm = m_gpExpr->terms[m_term_index];
		m_node = m_pTerm->factors[m_factor_index];
	}
	else if (m_term_index == m_gpExpr->terms.size()) {
		m_node = nullptr; 
		m_pTerm = m_gpExpr->terms.back();
		m_factor_index = m_pTerm->factors.size();
	}
}

void FactorIterator::insert(Node* node)
{
	auto it = m_pTerm->factors.begin() + m_factor_index;
	m_pTerm->factors.insert(it, node);
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
