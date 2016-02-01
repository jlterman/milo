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
 * @file milo.cpp
 * This file contains the implementation of member functions of classes 
 * Equation, Node, FactorIterator and NodeIterator.
 */

#include <iostream>
#include <vector>
#include <iterator>
#include <map>
#include <limits>
#include <typeinfo>

#include "milo.h"
#include "milo_key.h"

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
	if (m_input_index >= 0) m_inputs[m_input_index]->setCurrent(false);
	if (in_sn < 0) {
		m_input_index = -1;
		return;
	}
	for (unsigned int i = 0; i < m_inputs.size(); ++i) {
		if (m_inputs[i]->checkSN(in_sn)) {
			m_input_index = i;
			m_inputs[m_input_index]->setCurrent(true);
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

void Equation::draw(Graphics& gc, bool fRefresh)
{
	m_root->setUpDraw(gc);
	setSelect(gc);
	m_root->draw(gc);
	if (fRefresh) gc.out();
}


void Equation::eraseSelection(Node* node)
{
	Node* start = m_selectStart;
	Node* end = m_selectEnd;
	clearSelect();
	if (start == m_root) {
		delete m_root;
		if (node == nullptr) node = new Input(*this);
		m_root = new Expression(node);
	}
	else if (start == end) {
		auto it = FactorIterator(start);
		if (node == nullptr) it.erase();
		                else it.replace(node);
	}
	else if (start != end) {
		auto it = FactorIterator(start);
		if (node != nullptr) {
			it.insert(node); ++it;
		}
		auto it_end = FactorIterator(end);
		it.erase(it_end);
	}
}

bool Equation::handleChar(int ch)
{
	string store;
	this->xml_out(store);
	LOG_TRACE_MSG("handleChar eqn xml:\n" + store);

	bool fResult = true;
	if (m_selectStart != nullptr) {
		if (isalnum(ch) || ch == '.') {
			eraseSelection(new Input(*this, string(1, (char)ch)));
			return true;
		}
		switch(ch) {
		    case Key::BACKSPACE: {
				eraseSelection(new Input(*this));
				break;
			}
		    case Key::LEFT: {
				FactorIterator n{m_selectStart};
				if (n.isBegin()) return false;
				--n;
				selectNodeOrInput(*n);
				break;
			}
		    case Key::RIGHT: {
				FactorIterator n{m_selectStart};
				++n;
				if (n.isEnd()) return false;
				selectNodeOrInput(*n);
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
				auto it = FactorIterator(m_selectStart);
				clearSelect();
				it.insert(new Input(*this));
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

				if (parent->getType() == Term::type) {
					Term* term = dynamic_cast<Term*>(parent);
					setSelect(*(term->begin()), *(--term->end()));
				}
				else
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
			in->add((char)ch);
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
				if (getCurrentInput()->unremovable()) return false;
				disableCurrentInput();
				nextInput();
				break;
			}
	        case Key::BACKSPACE: {
				Input* in = getCurrentInput();
				if (in->empty() && in_pos.isBegin()) {
					return false;
				}
				if (!in->empty()) {
					in->remove();
					return true;
				}
				auto prev_pos = in_pos; --prev_pos;
				if (in_pos.isBeginTerm()) {
					prev_pos.mergeNextTerm();
				}
				else if (prev_pos->isLeaf()) {
					prev_pos.erase();
				} else {
					disableCurrentInput();
					setSelect(*prev_pos);
				}
			    break;
			}
		    case Key::LEFT: {
				if (in->empty() && in_pos.isBegin()) return false;
				
				in_pos = in->emptyBuffer();
				FactorIterator prev{in_pos};
				--prev;
				if (in->unremovable()) {
					disableCurrentInput();
					setSelect(*prev);
				} else
					FactorIterator::swap(prev, in_pos);
				break;
			}
		    case Key::RIGHT: {
				if (in_pos == in_pos.getLast()) return false;

				in_pos = in->emptyBuffer();
				FactorIterator nxt{in_pos};
				++nxt;
				if (in->unremovable()) {
					disableCurrentInput();
					setSelect(*nxt);
				} else
					FactorIterator::swap(nxt, in_pos);
				break;
			}
		    case Key::UP: {
				auto pos = disableCurrentInput();
				NodeIterator n{*pos};
				if (n != begin() ) {
					--n;
					setSelect(*n);
				}
				else
					fResult = false;
				break;
			}
		    case Key::DOWN: {
				auto pos = disableCurrentInput();
				NodeIterator n{*pos};
				++n;
				if (n != end())
					setSelect(*n);
				else
					fResult = false;
				break;
			}
	        case '+':
	        case '-': {
				in_pos = in->emptyBuffer();
				if (in_pos.isBeginTerm() && in->empty()) {
					in_pos.insert(new Input(*this));
					++in_pos;
					in->makeCurrent();
				}
				in_pos.insert(in_pos.splitTerm(ch == '-'));
				break;
			}
	        case '/': {
				fResult = Node::createNodeByName("divide", *this);
				break;
			}
	        case '^': {
				fResult = Node::createNodeByName("power", *this);
				break;
			}
	        case '(': {
				in->add("(#)");

				disableCurrentInput();
				break;
			}
	        case ' ': {
				if (in->empty()) {
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
	if (m_input_index >= 0 && m_inputs.size() == 1) return;

	if (m_input_index < 0) {
		m_input_index = 0;
	}
	else if (m_inputs[m_input_index]->unremovable()) {
		m_inputs[m_input_index]->setCurrent(false);
		m_input_index = (m_input_index + 1)%m_inputs.size();
	}
	else {
		disableCurrentInput();
	}
	
	m_inputs[m_input_index]->setCurrent(true);
}

FactorIterator Equation::disableCurrentInput()
{
	if (m_input_index < 0) throw logic_error("no current input to disable");

	Input* in = m_inputs[m_input_index];
	if (in->unremovable()) {
		in->setCurrent(false);
		m_input_index = -1;
		return FactorIterator(in);
	}

	FactorIterator pos = in->emptyBuffer();
	pos.erase(false);
	if (!pos.isBeginTerm()) --pos;
	removeInput(in);
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
	delete in;
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

void Equation::selectNodeOrInput(Node* node)
{
	if (getCurrentInput() != nullptr) disableCurrentInput();

	if (node->getType() == Input::type) {
		Input* in = dynamic_cast<Input*>(node);
		in->makeCurrent();
		clearSelect();
	}
	else
		setSelect(node);
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
	return m_root->findNode(x, y);
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

	++m_factor_index;
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
	else
		throw range_error("out of range");
}

void FactorIterator::prev()
{
	if (m_gpExpr == nullptr) throw logic_error("null iterator");

	if (m_node == nullptr) {
		m_term_index = m_gpExpr->terms.size() - 1;
		m_factor_index = m_pTerm->factors.size() - 1;
	}
	else if (m_factor_index > 0) {
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

bool FactorIterator::operator>(const FactorIterator& rhs)
{
	if (m_gpExpr != rhs.m_gpExpr) logic_error("Incompatible iterators for >");

	return (m_term_index > rhs.m_term_index ||
			(m_term_index == rhs.m_term_index && m_factor_index > rhs.m_factor_index));
}

bool FactorIterator::operator>=(const FactorIterator& rhs)
{
	if (m_gpExpr != rhs.m_gpExpr) logic_error("Incompatible iterators for >=");

	return (m_term_index > rhs.m_term_index ||
			(m_term_index == rhs.m_term_index && m_factor_index >= rhs.m_factor_index));
}

bool FactorIterator::operator<(const FactorIterator& rhs)
{
	if (m_gpExpr != rhs.m_gpExpr) logic_error("Incompatible iterators for <");

	return (m_term_index < rhs.m_term_index ||
			(m_term_index == rhs.m_term_index && m_factor_index < rhs.m_factor_index));
}

bool FactorIterator::operator<=(const FactorIterator& rhs)
{
	if (m_gpExpr != rhs.m_gpExpr) logic_error("Incompatible iterators for <=");

	return (m_term_index < rhs.m_term_index ||
			(m_term_index == rhs.m_term_index && m_factor_index <= rhs.m_factor_index));
}

bool FactorIterator::isEndTerm()
{ 
	if (m_gpExpr == nullptr) throw logic_error("null iterator");

	return m_factor_index == m_pTerm->factors.size() - 1;
}

void FactorIterator::erase(bool free)
{
	if (free) delete m_pTerm->factors.at(m_factor_index);
	eraseElement(m_pTerm->factors, m_factor_index);

	if (m_pTerm->factors.size() == 0) {
		if (m_gpExpr->terms.size() == 1) throw logic_error("Empty expression");

		delete m_pTerm;
		eraseElement(m_gpExpr->terms, m_term_index);
	}
	--m_factor_index;
	next(); // recaculate current position
}

void FactorIterator::erase(const FactorIterator& end, bool free)
{
	if (m_gpExpr != end.m_gpExpr) logic_error("Bad factor iterator with wrong expression");
	if (*this<=end) logic_error("Passed iterator needs to be greater or equal to this iterator");

	// merge terms between this iterator and end
	if (m_term_index != end.m_term_index) {
		for (auto t = m_term_index; t < end.m_term_index; ++t) mergeNextTerm();
	}
	while (m_node != end.m_node) { erase(free); }
	erase(free);
}

void FactorIterator::mergeNextTerm()
{
	if (m_term_index >= m_gpExpr->terms.size() - 1) return;

	mergeVectors(m_gpExpr->terms[m_term_index]->factors,
				 m_gpExpr->terms[m_term_index + 1]->factors);
	delete m_gpExpr->terms[m_term_index + 1];
	eraseElement(m_gpExpr->terms, m_term_index + 1);
}

Term* FactorIterator::splitTerm(bool fNeg)
{
	Term* oldTerm = m_pTerm;
	Term* newTerm = new Term(m_node, m_gpExpr, fNeg);
	erase(false);
	while (m_node != nullptr && m_pTerm == oldTerm) {
		newTerm->factors.push_back(m_node);
		erase(false);
	}
	return newTerm;
}

void FactorIterator::insert(Node* node)
{
	if (m_node == nullptr)
		m_pTerm->factors.push_back(node);
	else
		insertElement(m_pTerm->factors, m_factor_index, node);
	node->setParent(m_pTerm);
	m_node = node;
}

void FactorIterator::insert(Term* term, bool sign)
{
	if (m_node == nullptr)
		m_gpExpr->terms.push_back(term);
	else
		insertElement(m_gpExpr->terms, m_term_index, term);
	if (!sign) term->negative();
	term->setParent(m_gpExpr);
	m_factor_index = 0;
	m_pTerm = m_gpExpr->terms[m_term_index];
	m_node = m_pTerm->factors[m_factor_index];
}

void FactorIterator::insertAfter(Term* term, bool sign)
{
	if (m_node == nullptr) throw logic_error("Out of range");

	++m_term_index;
	insertElement(m_gpExpr->terms, m_term_index, term);
	if (!sign) term->negative();
	term->setParent(m_gpExpr);
	m_factor_index = 0;
	m_pTerm = m_gpExpr->terms[m_term_index];
	m_node = m_pTerm->factors[m_factor_index];
}

void FactorIterator::insertAfter(Node* node)
{
	if (m_node == nullptr) throw logic_error("Out of range");

	++m_factor_index;
	insertElement(m_pTerm->factors, m_factor_index, node);
	node->setParent(m_pTerm);
	m_node = node;
}

void FactorIterator::replace(Node* node, bool free)
{
	if (node == nullptr) logic_error("Node cannot be null");
	if (free) delete m_pTerm->factors[m_factor_index];
	m_pTerm->factors[m_factor_index] = node;
	node->setParent(m_pTerm);
}

void FactorIterator::replace(Term* term, bool free)
{
	if (term == nullptr) logic_error("Term cannot be null");
	if (free) delete m_gpExpr->terms[m_term_index];
	m_gpExpr->terms[m_term_index] = term;
	term->setParent(m_gpExpr);
}

void FactorIterator::swap(FactorIterator& a, FactorIterator& b)
{
	Node* tmp = a.m_node;
	a.m_pTerm->factors[a.m_factor_index] = b.m_pTerm->factors[b.m_factor_index];
	a.m_node = b.m_node;

	b.m_pTerm->factors[b.m_factor_index] = tmp;
	b.m_node = tmp;
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
