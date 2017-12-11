/* Copyright (C) 2017 - James Terman
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

#include "util.h"
#include "milo.h"
#include "ui.h"

using namespace std;

/* Take the selection state and draw it in the graphics context.
 */
void Equation::setSelect(UI::Graphics& gc)
{
	if (!m_selectStart && !m_selectEnd) {
		/* No selection. Clear it.
		 */
		gc.setSelect(0, 0, 0, 0);
	}
	else if (m_selectStart && m_selectStart == m_selectEnd) {
		/* On one node selected.
		 */
		gc.setSelect(m_selectStart->getFrame().box);
	}
	else if (m_selectStart) {
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
	for (unsigned int i = 0; i < m_inputs.size(); ++i) {
		if (m_inputs[i]->checkSN(in_sn)) {
			m_input_index = i;
			m_inputs[m_input_index]->setCurrent(true);
		}
		else {
			m_inputs[i]->setCurrent(false);
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
	while (down) {
		node = down;
		down = node->downLeft();
	}
	return (down) ? down : node;
}

Node* Node::last()
{
	Node* node = this;
	Node* down = this->downRight();
	while (down) {
		node = down;
		down = node->downRight();
	}
	return (down) ? down : node;
}

// Return node to the left. Go up if necessary
Node* Node::getNextLeft()
{
	Node* left = (m_parent) ? m_parent->getLeftSibling(this) : nullptr;
	if (left) { 
		return left->last();
	}
	else if (m_parent) {
		return m_parent->getNextLeft();
	}
	else
		return nullptr;
}

// Return node to the right. Go down if necessary
Node* Node::getNextRight()
{
	Node* right = (m_parent) ? m_parent->getRightSibling(this) : nullptr;
	if (right) { 
		return right->first();
	}
	else if (m_parent) {
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

void Node::calculateSize(UI::Graphics& gc)
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

void Node::calculateOrigin(UI::Graphics& gc, int x, int y)
{
	m_frame.box.setOrigin(x, y);
	if (m_nth == 1) m_parenthesis.setOrigin(x, y); else m_parenthesis.setOrigin(x, y + gc.getTextHeight());
	if (m_fDrawParenthesis)    x += gc.getParenthesisWidth();
	if (isFactor() && !m_sign) x += gc.getCharLength('-');

	calcOrig(gc, x, y);
}

// Draw this node and subtree in this graphic context
void Node::draw(UI::Graphics& gc) const
{
	if (m_fDrawParenthesis)  gc.parenthesis(m_parenthesis);
	if (isFactor() && !m_sign) {
		gc.at(m_frame.box.x0() + ( m_fDrawParenthesis ? gc.getParenthesisWidth() : 0 ), 
			  m_frame.box.y0() + m_frame.base, '-', UI::Graphics::Attributes::NONE);
	}

	if (m_nth != 1) {
		string n = to_string(m_nth);
		gc.at(m_frame.box.x0() + m_frame.box.width() - gc.getTextLength(n),
			  m_frame.box.y0(), n, UI::Graphics::Attributes::NONE);
	}
	drawNode(gc);	
}

// Set up drawing for this node and its subtree
void Node::setUpDraw(UI::Graphics& gc)
{
	calculateSize(gc);
	calculateOrigin(gc, 0, 0);
	gc.set(m_frame.box);
}

void Equation::draw(UI::Graphics& gc, bool fRefresh)
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
		if (!node) node = new Input(*this);
		m_root = new Expression(node);
	}
	else if (start == end) {
		auto it = FactorIterator(start);
		if (!node) it.erase();
		      else it.replace(node);
	}
	else if (start != end) {
		auto it = FactorIterator(start);
		if (node) {
			it.insert(node); ++it;
		}
		auto it_end = FactorIterator(end);
		it.erase(it_end);
	}
}

void Equation::nextInput(bool fShift) 
{
	if (m_input_index >= 0 && m_inputs.size() == 1) return;

	if (!fShift && m_input_index < 0) {
		m_input_index = 0;
	}
	else if (fShift && m_input_index < 0) {
		m_input_index = m_inputs.size() - 1;
	}
	else {
		if (m_inputs[m_input_index]->unremovable()) {
			m_inputs[m_input_index]->setCurrent(false);
		}
		else {
			disableCurrentInput();
		}
		if (fShift)
			m_input_index = (m_inputs.size() + m_input_index - 1)%m_inputs.size();
		else
			m_input_index = (m_input_index + 1)%m_inputs.size();			
	}
	m_inputs[m_input_index]->makeCurrent();
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
	pos.erase();
	if (!pos.isBeginTerm()) --pos;
	removeInput(in);
	return pos;
}

void Equation::removeInput(Input* in)
{
	int in_index =  distance(m_inputs.cbegin(),find(m_inputs, in));
	eraseElement(m_inputs, in_index);
	
	if (m_inputs.size() > 0 && in_index == m_input_index) {
		m_input_index = (m_input_index + 1)%m_inputs.size();
		m_inputs[m_input_index]->setCurrent(true);
	}
	else if (m_inputs.size() == 0) {
		m_input_index = -1;
	}
}

void Equation::getCursorOrig(int& x, int& y) 
{ 
	if (!getCurrentInput()) return;

	x = getCurrentInput()->getFrame().box.x0() + getCurrentInput()->getFrame().box.width(); 
	y = getCurrentInput()->getFrame().box.y0();
}

void Equation::clearSelect()
{
	if (m_selectStart) m_selectStart->setSelect(Node::Select::NONE);
	if (m_selectEnd) m_selectEnd->setSelect(Node::Select::NONE);
}

void Equation::setSelect(Node* start, Node* end)
{
	if (!start) return;
	clearSelect();

	if (!end || end == start) {
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
	if (getCurrentInput()) disableCurrentInput();

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

Node* Equation::findNode(UI::Graphics& gc, int x, int y)
{
	gc.relativeOrig(x, y);
	return m_root->findNode(x, y);
}

Node* Equation::findNode(UI::Graphics& gc, Box b)
{
	gc.relativeOrig(b.x0(), b.y0());
	return m_root->findNode(b);
}

void Equation::selectBox(UI::Graphics& gc, Node* start, Box b)
{
	gc.relativeOrig(b.x0(), b.y0());
	if (!start->getParent() || start->getParent()->getType() != Term::type) {
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
	if (!m_node) throw range_error("out of range");

	m_node = m_node->getNextRight();
}

void NodeIterator::prev()
{
	Node* prev_node = m_node->getNextLeft();
	if (!prev_node) throw range_error("out of range");

	m_node = prev_node;
}

FactorIterator::FactorIterator(Node* node) : m_node(node)
{
	if (!m_node) return;

	m_pTerm = dynamic_cast<Term*>(m_node->getParent());
	m_gpExpr = dynamic_cast<Expression*>(m_pTerm->getParent());
	m_factor_index = m_pTerm->factors.get_index(m_node);
	m_term_index = m_gpExpr->terms.get_index(m_pTerm);
}

FactorIterator::FactorIterator(Expression* exp) : FactorIterator(exp->terms[0]->factors[0]) {}


void FactorIterator::setNode(int factor, int term)
{
	m_term_index = term + (term < 0 ? m_gpExpr->terms.size() : 0);
	m_pTerm = m_gpExpr->terms[m_term_index];

	m_factor_index = factor + (factor < 0 ? m_pTerm->factors.size() : 0);
	m_node = m_pTerm->factors[m_factor_index];
}

void FactorIterator::next()
{
	if (!m_gpExpr) throw logic_error("null iterator");

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
	if (!m_gpExpr) throw logic_error("null iterator");

	if (!m_node) {
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
	if (!m_gpExpr) throw logic_error("null iterator");

	return m_factor_index == m_pTerm->factors.size() - 1;
}

void FactorIterator::erase()
{
	m_pTerm->factors.erase_index(m_factor_index);

	if (m_pTerm->factors.size() == 0) {
		if (m_gpExpr->terms.size() == 1) throw logic_error("Empty expression");

		m_gpExpr->terms.erase_index(m_term_index);
	}
	--m_factor_index;
	next(); // recaculate current position
}

void FactorIterator::erase(const FactorIterator& end)
{
	if (m_gpExpr != end.m_gpExpr) logic_error("Bad factor iterator with wrong expression");
	if (*this<=end) logic_error("Passed iterator needs to be greater or equal to this iterator");

	// merge terms between this iterator and end
	if (m_term_index != end.m_term_index) {
		for (auto t = m_term_index; t < end.m_term_index; ++t) mergeNextTerm();
	}
	while (m_node != end.m_node) { erase(); }
	erase();
}

void FactorIterator::mergeNextTerm()
{
	if (m_term_index >= m_gpExpr->terms.size() - 1) return;

	m_gpExpr->terms[m_term_index]->factors.merge(m_gpExpr->terms[m_term_index + 1]->factors);
	m_gpExpr->terms.erase_index(m_term_index + 1);
}

Term* FactorIterator::splitTerm(bool fNeg)
{
	Term* oldTerm = m_pTerm;
	Term* newTerm = new Term(m_node, m_gpExpr, fNeg);
	erase();
	while (m_node && m_pTerm == oldTerm) {
		newTerm->factors.push_back(m_node);
		erase();
	}
	return newTerm;
}

void FactorIterator::insert(Node* node)
{
	if (!m_node)
		m_pTerm->factors.push_back(node);
	else
		m_pTerm->factors.insert_index(m_factor_index, node);
	node->setParent(m_pTerm);
	m_node = node;
}

void FactorIterator::insert(Term* term, bool sign)
{
	if (!m_node)
		m_gpExpr->terms.push_back(term);
	else
		m_gpExpr->terms.insert_index(m_term_index, term);
	if (!sign) term->negative();
	term->setParent(m_gpExpr);
	m_factor_index = 0;
	m_pTerm = m_gpExpr->terms[m_term_index];
	m_node = m_pTerm->factors[m_factor_index];
}

void FactorIterator::insertAfter(Term* term, bool sign)
{
	if (!m_node) throw logic_error("Out of range");

	++m_term_index;
	m_gpExpr->terms.insert_index(m_term_index, term);
	if (!sign) term->negative();
	term->setParent(m_gpExpr);
	m_factor_index = 0;
	m_pTerm = m_gpExpr->terms[m_term_index];
	m_node = m_pTerm->factors[m_factor_index];
}

void FactorIterator::insertAfter(Node* node)
{
	if (!m_node) throw logic_error("Out of range");

	++m_factor_index;
	m_pTerm->factors.insert_index(m_factor_index, node);
	node->setParent(m_pTerm);
	m_node = node;
}

void FactorIterator::replace(Node* node)
{
	if (!node) logic_error("Node cannot be null");
	m_pTerm->factors[m_factor_index] = node;
	node->setParent(m_pTerm);
}

void FactorIterator::replace(Term* term)
{
	if (!term) logic_error("Term cannot be null");
	m_gpExpr->terms[m_term_index] = term;
	term->setParent(m_gpExpr);
}

void FactorIterator::swap(FactorIterator& a, FactorIterator& b)
{
	NodePtr tmp; tmp = a.m_node;
	a.m_pTerm->factors[a.m_factor_index] = b.m_pTerm->factors[b.m_factor_index];
	a.m_node = b.m_node;

	b.m_pTerm->factors[b.m_factor_index] = tmp;
	b.m_node = tmp;
}

void EqnUndoList::save(EqnPtr eqn)
{ 
	string store;
	eqn->xml_out(store);
	LOG_TRACE_MSG("saved eqn xml:\n" + store);
	m_eqns.push_back(store);
}

EqnPtr EqnUndoList::undo()
{
	if (m_eqns.size() <= 1) return nullptr;
	m_eqns.pop_back();
	return top();
}

EqnPtr EqnUndoList::top()
{
	if (m_eqns.empty()) return nullptr;
	istringstream in(m_eqns.back());
	return make_shared<Equation>(in);
}

namespace Log
{
	void msg(const string& m)
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
