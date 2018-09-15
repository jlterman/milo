/* Copyright (C) 2018 - James Terman
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
 * @file eqn.cpp
 * This file contains the implementation of the panels that use class Equation.
 */

#include "panel.h"
#include "milo.h"

using namespace std;
using namespace UI;

const string EqnPanel::name = "equation";

bool EqnPanel::init = EqnPanel::do_init();

bool EqnPanel::do_init()
{
	MiloPanel::panel_map[EqnPanel::name] = createPanel<EqnPanel>;
	MiloPanel::panel_xml_map[EqnPanel::name] = createPanelXML<EqnPanel>;
	return true;
}

bool EqnPanel::do_alphaNumber(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (m_eqn->getSelectStart() != nullptr) {
		m_eqn->eraseSelection(new Input(getEqn(), string(1, (char)event.getKey())));
	}
	else {
		Input* in = m_eqn->getCurrentInput();
		if (in == nullptr) return false;
		in->add((char)event.getKey());
	}
	return true;
}

bool EqnPanel::do_backspace(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (m_eqn->getSelectStart() != nullptr) {
		m_eqn->eraseSelection(new Input(getEqn()));
	}
	else {
		Input* in = m_eqn->getCurrentInput();
		if (in == nullptr) return false;
		FactorIterator in_pos(in);
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
			m_eqn->disableCurrentInput();
			m_eqn->setSelect(*prev_pos);
		}
	}
	return true;
}

bool EqnPanel::do_left(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (m_eqn->getSelectStart() != nullptr) {
		FactorIterator n(m_eqn->getSelectStart());
		if (n.isBegin()) return false;
		--n;
		m_eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;
	
		in_pos = in->emptyBuffer();
		FactorIterator prev{in_pos};
		--prev;
		if (prev->getType() == Input::type) {
			m_eqn->selectNodeOrInput(*prev);
		}
		else if (in->unremovable()) {
			m_eqn->disableCurrentInput();
			prev.insertAfter(new Input(getEqn()));
		} else
			FactorIterator::swap(prev, in_pos);
	}
	else
		m_eqn->setSelect(m_eqn->getRoot());
	return true;
}

bool EqnPanel::do_right(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (m_eqn->getSelectStart() != nullptr) {
		FactorIterator n(m_eqn->getSelectStart());
		++n;
		if (n.isEnd()) return false;
		m_eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		Input* in =  m_eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos == in_pos.getLast()) return false;

		in_pos = in->emptyBuffer();
		FactorIterator nxt{in_pos};
		++nxt;
		if (nxt->getType() == Input::type) {
			m_eqn->selectNodeOrInput(*nxt);
		}
		else if (in->unremovable()) {
			m_eqn->disableCurrentInput();
			nxt.insert(new Input(getEqn()));;
		} else
			FactorIterator::swap(nxt, in_pos);
	}
	else
		m_eqn->setSelect(m_eqn->getRoot()->last());
	return true;
}

bool EqnPanel::do_shift_left(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (m_eqn->getSelectStart() != nullptr) {
		auto start = FactorIterator(m_eqn->getSelectStart());
		
		if (start.isBegin()) return false;
		--start;

		m_eqn->setSelect(*start, m_eqn->getSelectEnd());
	}
	else {
		Input* in =  m_eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;

		in_pos = m_eqn->disableCurrentInput();
		m_eqn->setSelect(*in_pos, *in_pos);
	}
	return true;
}

bool EqnPanel::do_shift_right(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (m_eqn->getSelectStart() != nullptr) {
		auto end = FactorIterator(m_eqn->getSelectEnd());
		
		if (end == end.getLast()) return false;
		++end;

		m_eqn->setSelect(m_eqn->getSelectStart(), *end);
	}
	else {
		Input* in = m_eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos.isEnd()) return false;

		in_pos = ++m_eqn->disableCurrentInput();
		m_eqn->setSelect(*in_pos, *in_pos);
	}
	return true;
}

bool EqnPanel::do_up(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (m_eqn->getSelectStart() != nullptr) {
		NodeIterator n(m_eqn->getSelectStart());
		if (n == m_eqn->begin()) return false;
		--n;
		m_eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		in_pos = in->emptyBuffer();
		NodeIterator prev{*in_pos};
		if (prev == m_eqn->begin()) return false;
		--prev;
		m_eqn->disableCurrentInput();
		m_eqn->setSelect(*prev);
	}
	else
		m_eqn->setSelect(*(m_eqn->begin()));
	return true;
}

bool EqnPanel::do_down(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (m_eqn->getSelectStart() != nullptr) {
		NodeIterator n(m_eqn->getSelectEnd());
		if (n == m_eqn->last()) return false;
		++n;
		m_eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		if (in_pos == m_eqn->last()) return false;
	
		in_pos = in->emptyBuffer();
		NodeIterator nxt{*in_pos};
		++nxt;
		m_eqn->disableCurrentInput();
		m_eqn->setSelect(*nxt);
	}
	else
		m_eqn->setSelect(*(m_eqn->last()));
	return true;
}

bool EqnPanel::do_shift_up(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (m_eqn->getSelectStart() != nullptr) {
		auto start = FactorIterator(m_eqn->getSelectStart());
		
		if (start.isBegin()) return false;
		start.setNode(0, 0);
		m_eqn->setSelect(*start, m_eqn->getSelectEnd());
	}
	else {
		Input* in = m_eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;
		in->emptyBuffer();
		auto start = in_pos.getBegin();
		m_eqn->setSelect(*start, m_eqn->getSelectEnd());
	}
	return true;
}

bool EqnPanel::do_shift_down(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (m_eqn->getSelectStart() != nullptr) {
		auto end = FactorIterator(m_eqn->getSelectEnd());
		
		if (end == end.getLast()) return false;
		end.setNode(-1, -1);
		m_eqn->setSelect(m_eqn->getSelectStart(), *end);
	}
	else {
		Input* in = m_eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos.isEnd()) return false;
		in->emptyBuffer();
		auto end = in_pos.getLast();
		m_eqn->setSelect(m_eqn->getSelectStart(), *end);
	}
	return true;
}

bool EqnPanel::do_enter(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	Node* start = m_eqn->getSelectStart();
	if (start != nullptr) {
		auto it = FactorIterator(start);
		m_eqn->clearSelect();
		it.insertAfter(new Input(getEqn()));
	}
	else if (in != nullptr) {
		if (m_eqn->getCurrentInput()->unremovable()) return false;
		m_eqn->disableCurrentInput();
		m_eqn->nextInput(false);
	}
	else
		return false;
	return true;
}

bool EqnPanel::do_tab(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	m_eqn->nextInput(event.shiftMod());
	return true;
}

bool EqnPanel::do_plus_minus(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (in == nullptr) return false;

	FactorIterator in_pos(in);
	in_pos = in->emptyBuffer();
	if (in_pos.isBeginTerm() && in->empty()) {
		in_pos.insert(new Input(getEqn()));
		++in_pos;
		in->makeCurrent();
	}
	in_pos.insert(in_pos.splitTerm(((char) event.getKey()) == '-'));
	return true;
}

bool EqnPanel::do_divide(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("divide", getEqn());
}

bool EqnPanel::do_power(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("power", getEqn());
}

bool EqnPanel::do_left_parenthesis(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (!in) return false;

	in->add("(#)");
	m_eqn->disableCurrentInput();
	return true;
}

bool EqnPanel::do_space(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = m_eqn->getCurrentInput();
	if (in != nullptr) {
		if (in->empty()) return false;

		auto pos = m_eqn->disableCurrentInput();
		m_eqn->setSelect(*pos);
	}
	else if (m_eqn->getSelectStart() != nullptr) {
		if (m_eqn->getSelectStart()->getParent() == nullptr) return false;

		Node* parent = nullptr;
		int num = 0;
		Node* start = m_eqn->getSelectStart();
		Node* end   = m_eqn->getSelectEnd();
		
		if (start != end) {
			FactorIterator end_pos(end);
			for (auto it = FactorIterator(start); it != end_pos; ++it) {
				num += it->numFactors();
			}
			parent = start->getParent()->getParent();
		}
		else {
			num = start->numFactors();
			parent = start->getParent();
		}
		
		while (parent && parent->numFactors() == num) { parent = parent->getParent(); }
		if (parent == nullptr) return false;
		
		if (parent->getType() == Term::type) {
			Term* term = dynamic_cast<Term*>(parent);
			m_eqn->setSelect(*(term->begin()), *(--term->end()));
		}
		else
			m_eqn->setSelect(parent);
	}
	else {
		m_eqn->setSelect(m_eqn->getRoot()->first());
	}
	return true;
}

bool EqnPanel::do_mouse_pressed(const MouseEvent& mouse)
{
	mouse.getCoords(m_start_mouse_x, m_start_mouse_y);
	LOG_TRACE_MSG("mouse press x: " + to_string(m_start_mouse_x) +
				            ", y: " + to_string(m_start_mouse_y));
	m_start_select = m_eqn->findNode(m_start_mouse_x, m_start_mouse_y);
	if (m_start_select == nullptr) {
		return false;
	}
	LOG_TRACE_MSG("node found: " + m_start_select->toString());
	m_eqn->setSelect(m_start_select);
	m_eqn->draw(*m_gc);
	return false;
}

bool EqnPanel::do_mouse_released(const MouseEvent&)
{
	LOG_TRACE_MSG("mouse release");
	m_start_mouse_x = m_start_mouse_y = -1;
	if (m_start_select != nullptr && m_start_select->getSelect() == Node::Select::ALL)
	{
		m_eqn->selectNodeOrInput(m_start_select);
	}
	m_start_select = nullptr;
	return true;
}

bool EqnPanel::do_mouse_clicked(const MouseEvent& mouse)
{
	int mouse_x, mouse_y;
	mouse.getCoords(mouse_x, mouse_y);
	LOG_TRACE_MSG("mouse clicked x: " + to_string(mouse_x) + ", y: " + to_string(mouse_y));
	Node* node = m_eqn->findNode(mouse_x, mouse_y);
	if (node == nullptr) return false;
	LOG_TRACE_MSG("node found: " + node->toString());
	m_eqn->clearSelect();
	m_eqn->selectNodeOrInput(node);
	m_eqn->draw(*m_gc);
	return true;	
}

bool EqnPanel::do_mouse_double(const MouseEvent& mouse)
{
	int mouse_x, mouse_y;
	mouse.getCoords(mouse_x, mouse_y);
	LOG_TRACE_MSG("mouse double clicked x: " + to_string(mouse_x) + ", y: " + to_string(mouse_y));
	Node* node = m_eqn->findNode(mouse_x, mouse_y);
	if (node == nullptr || node == m_eqn->getCurrentInput()) return false;
	LOG_TRACE_MSG("node found: " + node->toString());
	if (node->getType() == Input::type) {
		m_eqn->selectNodeOrInput(node);
		return true;
	}				   
	if (m_eqn->getCurrentInput() != nullptr) m_eqn->disableCurrentInput();
	m_eqn->clearSelect();
	auto it = FactorIterator(node);
	it.insert(new Input(getEqn()));
	return true;
}

bool EqnPanel::do_mouse_position(const MouseEvent& mouse)
{
	if (m_start_mouse_x < 0 && m_start_mouse_y < 0) {
		return false;
	}

	Box box = { 0, 0, 0, 0 };
	int event_x = -1, event_y = -1;
	mouse.getCoords(event_x, event_y);
	box = { abs(m_start_mouse_x - event_x),
			abs(m_start_mouse_y - event_y),
			min(m_start_mouse_x,  event_x),
			min(m_start_mouse_y,  event_y)
	};
	LOG_TRACE_MSG("mouse position current box: " + box.toString());

	if (m_start_select != nullptr && box.area() > 1 ) {
		Node* new_select = m_eqn->findNode(box);
		LOG_TRACE_MSG("node found: " + ((new_select == nullptr) ? "None" : new_select->toString()));
		if (new_select != nullptr && new_select->getDepth() < m_start_select->getDepth()) {
			m_start_select = new_select;
		}
		m_eqn->selectBox(m_start_select, box);
		m_eqn->draw(*m_gc);
	}
	else if (m_start_select == nullptr && box.area() > 0) {
		m_start_select = m_eqn->findNode(box);
		LOG_TRACE_MSG("node found: " + ((m_start_select == nullptr) ?
										"None" :
										m_start_select->toString()));
		if (m_start_select != nullptr) {
			m_eqn->setSelect(m_start_select);
			m_eqn->draw(*m_gc);
		}
	}
	return false;
}
const unordered_map<MouseEvent, EqnPanel::mouse_handler> EqnPanel::mouse_event_map = {
	{ MouseEvent(Mouse::RELEASED, 1), [](EqnPanel& p, const MouseEvent& m) { return p.do_mouse_released(m); } },
	{ MouseEvent(Mouse::PRESSED,  1), [](EqnPanel& p, const MouseEvent& m) { return p.do_mouse_pressed(m); } },
	{ MouseEvent(Mouse::CLICKED,  1), [](EqnPanel& p, const MouseEvent& m) { return p.do_mouse_clicked(m); } },
	{ MouseEvent(Mouse::DOUBLE,   1), [](EqnPanel& p, const MouseEvent& m) { return p.do_mouse_double(m); } },
    { MouseEvent(Mouse::POSITION, 0), [](EqnPanel& p, const MouseEvent& m) { return p.do_mouse_position(m); } }
};

bool EqnPanel::emit_key(EqnPanel& panel, const KeyEvent& key)
{
	return panel.do_alphaNumber(key);
}

const unordered_map<KeyEvent, EqnPanel::key_handler> EqnPanel::key_event_map = {
	{ KeyEvent(Keys::LEFT,  Modifiers::SHIFT), [](EqnPanel& p, const KeyEvent& k) { return p.do_shift_left(k); } },
	{ KeyEvent(Keys::RIGHT, Modifiers::SHIFT), [](EqnPanel& p, const KeyEvent& k) { return p.do_shift_right(k); } },
	{ KeyEvent(Keys::UP,    Modifiers::SHIFT), [](EqnPanel& p, const KeyEvent& k) { return p.do_shift_up(k); } },
	{ KeyEvent(Keys::DOWN,  Modifiers::SHIFT), [](EqnPanel& p, const KeyEvent& k) { return p.do_shift_down(k); } },
	{ KeyEvent(Keys::TAB,   Modifiers::SHIFT), [](EqnPanel& p, const KeyEvent& k) { return p.do_tab(k); } },
	
	{ KeyEvent(Keys::PLUS),   [](EqnPanel& p, const KeyEvent& k) { return p.do_plus_minus(k); } },
	{ KeyEvent(Keys::MINUS),  [](EqnPanel& p, const KeyEvent& k) { return p.do_plus_minus(k); } },
	{ KeyEvent(Keys::DIVIDE), [](EqnPanel& p, const KeyEvent& k) { return p.do_divide(k); } },
	{ KeyEvent(Keys::POWER),  [](EqnPanel& p, const KeyEvent& k) { return p.do_power(k); } },
	{ KeyEvent(Keys::L_PAR),  [](EqnPanel& p, const KeyEvent& k) { return p.do_left_parenthesis(k); } },
	{ KeyEvent(Keys::SPACE),  [](EqnPanel& p, const KeyEvent& k) { return p.do_space(k); } },
	{ KeyEvent(Keys::LEFT),   [](EqnPanel& p, const KeyEvent& k) { return p.do_left(k); } },
	{ KeyEvent(Keys::RIGHT),  [](EqnPanel& p, const KeyEvent& k) { return p.do_right(k); } },
	{ KeyEvent(Keys::UP),     [](EqnPanel& p, const KeyEvent& k) { return p.do_up(k); } },
	{ KeyEvent(Keys::DOWN),   [](EqnPanel& p, const KeyEvent& k) { return p.do_down(k); } },
	{ KeyEvent(Keys::TAB),    [](EqnPanel& p, const KeyEvent& k) { return p.do_tab(k); } },
	{ KeyEvent(Keys::ENTER),  [](EqnPanel& p, const KeyEvent& k) { return p.do_enter(k); } },
	{ KeyEvent(Keys::BSPACE), [](EqnPanel& p, const KeyEvent& k) { return p.do_backspace(k); } },
	{ KeyEvent(Keys::DOT),    emit_key },
	
	{ KeyEvent(Keys::K0), emit_key },
	{ KeyEvent(Keys::K1), emit_key },
	{ KeyEvent(Keys::K2), emit_key },
	{ KeyEvent(Keys::K3), emit_key },
	{ KeyEvent(Keys::K4), emit_key },
	{ KeyEvent(Keys::K5), emit_key },
	{ KeyEvent(Keys::K6), emit_key },
	{ KeyEvent(Keys::K7), emit_key },
	{ KeyEvent(Keys::K8), emit_key },
	{ KeyEvent(Keys::K9), emit_key },
	{ KeyEvent(Keys::A), emit_key },
	{ KeyEvent(Keys::B), emit_key },
	{ KeyEvent(Keys::C), emit_key },
	{ KeyEvent(Keys::D), emit_key },
	{ KeyEvent(Keys::E), emit_key },
	{ KeyEvent(Keys::F), emit_key },
	{ KeyEvent(Keys::H), emit_key },
	{ KeyEvent(Keys::I), emit_key },
	{ KeyEvent(Keys::J), emit_key },
	{ KeyEvent(Keys::K), emit_key },
	{ KeyEvent(Keys::L), emit_key },
	{ KeyEvent(Keys::M), emit_key },
	{ KeyEvent(Keys::N), emit_key },
	{ KeyEvent(Keys::O), emit_key },
	{ KeyEvent(Keys::P), emit_key },
	{ KeyEvent(Keys::Q), emit_key },
	{ KeyEvent(Keys::R), emit_key },
	{ KeyEvent(Keys::S), emit_key },
	{ KeyEvent(Keys::T), emit_key },
	{ KeyEvent(Keys::U), emit_key },
	{ KeyEvent(Keys::V), emit_key },
	{ KeyEvent(Keys::W), emit_key },
	{ KeyEvent(Keys::X), emit_key },
	{ KeyEvent(Keys::Y), emit_key },
	{ KeyEvent(Keys::Z), emit_key },
	{ KeyEvent(Keys::a), emit_key },
	{ KeyEvent(Keys::b), emit_key },
	{ KeyEvent(Keys::c), emit_key },
	{ KeyEvent(Keys::d), emit_key },
	{ KeyEvent(Keys::e), emit_key },
	{ KeyEvent(Keys::f), emit_key },
	{ KeyEvent(Keys::g), emit_key },
	{ KeyEvent(Keys::h), emit_key },
	{ KeyEvent(Keys::i), emit_key },
	{ KeyEvent(Keys::j), emit_key },
	{ KeyEvent(Keys::k), emit_key },
	{ KeyEvent(Keys::l), emit_key },
	{ KeyEvent(Keys::m), emit_key },
	{ KeyEvent(Keys::n), emit_key },
	{ KeyEvent(Keys::o), emit_key },
	{ KeyEvent(Keys::p), emit_key },
	{ KeyEvent(Keys::q), emit_key },
	{ KeyEvent(Keys::r), emit_key },
	{ KeyEvent(Keys::s), emit_key },
	{ KeyEvent(Keys::t), emit_key },
	{ KeyEvent(Keys::u), emit_key },
	{ KeyEvent(Keys::v), emit_key },
	{ KeyEvent(Keys::w), emit_key },
	{ KeyEvent(Keys::x), emit_key },
	{ KeyEvent(Keys::y), emit_key },
	{ KeyEvent(Keys::z), emit_key },
};

void EqnPanel::doKey(const KeyEvent& key)
{
	bool fChange = false;
	auto key_entry = key_event_map.find(key);
	if (key_entry != key_event_map.end()) {
		fChange = (key_entry->second)(*this, key);
	}
	if (fChange) {
		calculateSize();
		pushUndo();
	}
}

void EqnPanel::doMouse(const MouseEvent& mouse)
{
	bool fChange = false;
	auto mouse_entry = mouse_event_map.find(mouse);
	if (mouse_entry != mouse_event_map.end()) {
		fChange = (mouse_entry->second)(*this, mouse);
	}
	if (fChange) {
		calculateSize();
		pushUndo();
	}
}

const unordered_map<string, EqnPanel::menu_handler> EqnPanel::menu_map = {
	{ string("simplify"),  [](EqnPanel& p) { return p.getEqn().simplify(); } },
	{ string("normalize"), [](EqnPanel& p) { p.getEqn().normalize(); return true; } },
};

bool EqnPanel::doMenu(const string& menuFunctionName)
{
	bool fChange = false;
	auto menu_entry = menu_map.find(menuFunctionName);
	if (menu_entry != menu_map.end()) {
		fChange = (menu_entry->second)(*this);
		if (fChange) {
			calculateSize();
			pushUndo();
		}
		return true;
	}
	return false;
}

void EqnPanel::pushUndo()
{
	m_eqns.save(m_eqn);
	m_eqn = EqnPtr(m_eqns.top());
}

void EqnPanel::popUndo()
{
	EqnPtr undo_eqn = m_eqns.undo();
	if (undo_eqn) {
		m_eqn = undo_eqn;
		LOG_TRACE_MSG("undo to " + m_eqn->toString());
		m_eqn->draw(*m_gc);
	}
}

bool EqnPanel::blink()
{
	return m_eqn->blink();
}

void EqnPanel::getCursorOrig(int& x, int& y)
{
	m_eqn->getCursorOrig(x, y);
}

void EqnPanel::doDraw()
{
	m_eqn->setSelect(*m_gc);
	m_eqn->getRoot()->draw(*m_gc);
}

Box EqnPanel::calculateSize()
{
	m_eqn->getRoot()->calculateSize(*m_gc);
	m_eqn->getRoot()->calculateOrigin(*m_gc, 0, 0);
	return m_eqn->getRoot()->getFrame().box;
}

const string AlgebraPanel::name = "algebra";

const std::string AlgebraPanel::side_tag    = "side";
const std::string AlgebraPanel::left_value  = "left";
const std::string AlgebraPanel::right_value = "right";

bool AlgebraPanel::init = AlgebraPanel::do_init();

bool AlgebraPanel::do_init()
{
	MiloPanel::panel_map[AlgebraPanel::name] = createPanel<AlgebraPanel>;
	MiloPanel::panel_xml_map[AlgebraPanel::name] = createPanelXML<AlgebraPanel>;
	return true;
}

void AlgebraPanel::doDraw()
{
	m_left->doDraw();
	m_right->doDraw();
	Node::Frame left = m_left->getEqn().getRoot()->getFrame();
    m_gc->at(left.box.x0() + left.box.width() + m_gc->getTextLength("="),
			 m_frame.base, "=",
			 UI::Graphics::Attributes::NONE);
}

void AlgebraPanel::setBox(int x, int y, int x0, int y0)
{
	m_gc->set(x, y, x0, y0);
	
	auto leftFrame = m_left->getEqn().getRoot()->getFrame();
	auto rightFrame = m_right->getEqn().getRoot()->getFrame();

	m_left->getGraphics().set(leftFrame.box.width(),
							  leftFrame.box.height(),
							  x0,
							  y0 + m_frame.base - leftFrame.base);
	
	m_right->getGraphics().set(rightFrame.box.width(),
							   rightFrame.box.height(),
							   x0 + leftFrame.box.width() + m_gc->getTextLength("==="),
							   y0 + m_frame.base - rightFrame.base);
							   
}

Box AlgebraPanel::calculateSize()
{
	m_left->calculateSize();
	m_right->calculateSize();

	auto leftFrame = m_left->getEqn().getRoot()->getFrame();
	auto rightFrame = m_right->getEqn().getRoot()->getFrame();

	int y_max = max(leftFrame.box.height() -  leftFrame.base,
					rightFrame.box.height() - rightFrame.base);

	int b_max = max(leftFrame.base, rightFrame.base);

	int x = leftFrame.box.width() + m_gc->getTextLength("===") + rightFrame.box.width();

	m_frame = { { x, b_max + y_max, 0, 0 }, b_max };

	return m_frame.box;
}

void AlgebraPanel::xml_out(XML::Stream& xml)
{
	xml << XML::HEADER << side_tag << XML::HEADER_END << XML::ELEMENT;
	xml << (( m_side == LEFT) ? left_value : right_value) << XML::FOOTER;
	xml << m_left->getEqn() << m_right->getEqn();
}

AlgebraPanel::Side AlgebraPanel::readSide(XML::Parser& in)
{
	in.next(XML::HEADER, side_tag).next(XML::HEADER_END).next(XML::ELEMENT);
	if (!in.hasElement()) { in.syntaxError("Missing side element"); }
	string value = in.getElement();
	if (value != left_value && value != right_value) {
		in.syntaxError("bad element value not " + left_value + " or " + right_value);
	}
	Side s = ( value == "left" ) ? LEFT : RIGHT;
	in.assertNoAttributes();
	in.next(XML::FOOTER);
	return s;
}
