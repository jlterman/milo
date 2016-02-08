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
 * @file ui.cpp
 * This file implements the milo user interface implementing user generated
 * events.
 */

#include <fstream>
#include <unordered_map>
#include "ui.h"
#include "milo.h"

using namespace std;

/** @name File specific global variables. */
//@{
static bool fRunning = true;          ///< If true, keep running main loop.
static Node* start_select = nullptr;  ///< If not null, node is selected.
static int start_mouse_x = -1;        ///< Horiz coord of start of mouse drag.
static int start_mouse_y = -1;        ///< Vertical coord of start of mouse drag.
static Graphics* gc = nullptr;        ///< Pointer to graphics context.
static EqnUndoList eqns;              ///< Equation undo stack
static Equation* eqn;                 ///< Pointer to current equation.
//@}

/**
 * Function pointer to handle an event. Pointer to event passed as argument.
 * Return true, if event changed equation.
 */
typedef bool (*event_handler)(const UI::Event&);

/**
 * Handle quit event.
 * Set fRunning false to exit main loop.
 * @return Always false.
 */
static bool do_quit(const UI::Event&)
{
	fRunning = false;
	return false;
}

/**
 * Handle save event.
 * Save equation to file "eqn.xml".
 * @return Always true.
 */
static bool do_save(const UI::Event&)
{
	string store;
	eqn->xml_out(store);
	fstream out("eqn.xml", fstream::out | fstream::trunc);
	out << store << endl;
	out.close();
	return true;
}

/**
 * Handle undo event.
 * If non empty, pop and load an equation off the top of the undo stack.
 * @return If true, equation was loaded from undo stack.
 */
static bool do_undo(const UI::Event&)
{
	Equation* undo_eqn = eqns.undo();
	if (undo_eqn) {
		delete eqn;
		eqn = undo_eqn;
		LOG_TRACE_MSG("undo to " + eqn->toString());
		eqn->draw(*gc, true);
		return true;
	}
	return false;
}

/**
 * Handle mouse press event.
 * If node has been selected, remember it and mouse coordinates.
 * @return Always false.
 */
static bool do_mouse_pressed(const UI::Event&)
{
	gc->getMouseCoords(start_mouse_x, start_mouse_y);
	start_select = eqn->findNode(*gc, start_mouse_x, start_mouse_y);
	if (start_select == nullptr) {
		start_mouse_x = start_mouse_y = -1;
		return false;
	}
	eqn->setSelect(start_select);
	eqn->draw(*gc, true);
	return false;
}

/**
 * Handle mouse release event.
 * If mouse released where it was pressed, select node.
 * Otherwise, end mouse dragging state.
 * @return Always true.
 */
static bool do_mouse_released(const UI::Event&)
{
	start_mouse_x = start_mouse_y = -1;
	if (start_select->getSelect() == Node::Select::ALL) {
		eqn->selectNodeOrInput(start_select);
	}
	start_select = nullptr;
	return true;
}

/**
 * Handle mouse click.
 * If mouse clicks on node select it or activate input.
 * @return True if node selected.
 */
static bool do_mouse_clicked(const UI::Event&)
{
	int mouse_x = -1, mouse_y = -1;
	gc->getMouseCoords(mouse_x, mouse_y);
	Node* node = eqn->findNode(*gc, mouse_x, mouse_y);
	if (node == nullptr) return false;
	eqn->clearSelect();
	eqn->selectNodeOrInput(node);
	eqn->draw(*gc, true);
	return true;	
}

/**
 * Handle mouse double click.
 * If no node found, return false. Otherwise activate if input selected
 * or add input before selected node.
 * @return True, if node found.
 */
static bool do_mouse_double(const UI::Event&)
{
	int mouse_x = -1, mouse_y = -1;
	gc->getMouseCoords(mouse_x, mouse_y);
	Node* node = eqn->findNode(*gc, mouse_x, mouse_y);
	if (node == nullptr || node == eqn->getCurrentInput()) return false;
	if (node->getType() == Input::type) {
		eqn->selectNodeOrInput(node);
		return true;
	}				   
	if (eqn->getCurrentInput() != nullptr) eqn->disableCurrentInput();
	eqn->clearSelect();
	auto it = FactorIterator(node);
	it.insert(new Input(*eqn));
	return true;
}

/**
 * Handle mouse position reports,
 * Update part of equation selected by mouse dragging.
 * @return Always false.
 */
static bool do_mouse_position(const UI::Event&)
{
	Box box = { 0, 0, 0, 0 };
	if (start_mouse_x > 0 && start_mouse_y > 0) {
		int event_x = -1, event_y = -1;
		gc->getMouseCoords(event_x, event_y);
		box = { abs(start_mouse_x - event_x),
				abs(start_mouse_y - event_y),
				min(start_mouse_x,  event_x),
				min(start_mouse_y,  event_y)
		};
	}
	if (start_select != nullptr && box.area() > 1 ) {
		Node* new_select = eqn->findNode(*gc, box);
		if (new_select != nullptr && new_select->getDepth() < start_select->getDepth())
			start_select = new_select;
		eqn->selectBox(*gc, start_select, box);
		eqn->draw(*gc, true);
	}
	else if (start_select == nullptr && box.area() > 0) {
		start_select = eqn->findNode(*gc, box);
		eqn->setSelect(start_select);
		eqn->draw(*gc, true);
	}
	return false;
}

/**
 * Handle letter or number key event.
 * Insert letter or number key into equation if there is a current input or selection.
 * @param event Key event.
 * @return True if key was inserted.
 */
static bool do_key(const UI::Event& event)
{
	if (eqn->getSelectStart() != nullptr) {
		eqn->eraseSelection(new Input(*eqn, string(1, (char)event.getKey())));
	}
	else {
		Input* in = eqn->getCurrentInput();
		if (in == nullptr) return false;
		in->add((char)event.getKey());
	}
	return true;
}

/**
 * Handle backspace key event.
 * Erase selection if it exists, or previous factor before current input.
 * @return True if backspace is processed.
 */
static bool do_backspace(const UI::Event&)
{
	if (eqn->getSelectStart() != nullptr) {
		eqn->eraseSelection(new Input(*eqn));
	}
	else {
		Input* in = eqn->getCurrentInput();
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
			eqn->disableCurrentInput();
			eqn->setSelect(*prev_pos);
		}
	}
	return true;
}

/**
 * Handle left arrow key event.
 * Move to node to the left of selection or current input.
 * @return True if left arrow proccessed.
 */
static bool do_left(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (eqn->getSelectStart() != nullptr) {
		FactorIterator n(eqn->getSelectStart());
		if (n.isBegin()) return false;
		--n;
		eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;
	
		in_pos = in->emptyBuffer();
		FactorIterator prev{in_pos};
		--prev;
		if (in->unremovable()) {
			eqn->disableCurrentInput();
			eqn->setSelect(*prev);
		} else
			FactorIterator::swap(prev, in_pos);
	}
	else
		eqn->setSelect(eqn->getRoot());
	return true;
}

/**
 * Handle right arrow key event.
 * Move to node to the right of selection or current input.
 * @return True if right arrow proccessed.
 */
static bool do_right(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (eqn->getSelectStart() != nullptr) {
		FactorIterator n(eqn->getSelectStart());
		++n;
		if (n.isEnd()) return false;
		eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		Input* in =  eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos == in_pos.getLast()) return false;
				
		in_pos = in->emptyBuffer();
		FactorIterator nxt{in_pos};
		--nxt;
		if (in->unremovable()) {
			eqn->disableCurrentInput();
			eqn->setSelect(*nxt);
		} else
			FactorIterator::swap(nxt, in_pos);
	}
	else
		eqn->setSelect(eqn->getRoot()->last());
	return true;
}

/**
 * Handle left arrow with shift key event.
 * Either add factor on the left to selection or 
 * select factor to left of cursor.
 * @return True if shift left arrow is processed.
 */
static bool do_shift_left(const UI::Event&)
{
	if (eqn->getSelectStart() != nullptr) {
		auto start = FactorIterator(eqn->getSelectStart());
		
		if (start.isBegin()) return false;
		--start;

		eqn->setSelect(*start, eqn->getSelectEnd());
	}
	else {
		Input* in =  eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;

		auto start = in->emptyBuffer();
		auto end   = in_pos--;
		eqn->setSelect(*start, *end);
	}
	return true;
}

/**
 * Handle right arrow with shift key event.
 * Either add factor on the right to selection or 
 * select factor to right of cursor.
 * @return True if shift right arrow is processed.
 */
static bool do_shift_right(const UI::Event&)
{
	if (eqn->getSelectStart() != nullptr) {
		auto end = FactorIterator(eqn->getSelectEnd());
		
		if (end == end.getLast()) return false;
		++end;

		eqn->setSelect(eqn->getSelectStart(), *end);
	}
	else {
		Input* in = eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos.isEnd()) return false;

		auto end   = in->emptyBuffer();
		auto start = in_pos++;
		eqn->setSelect(*start, *end);
	}
	return true;
}

/**
 * Handle up arrow key event.
 * Transverse the entire equation tree to the left 
 * of the selection or the current input. Select first
 * node if there is no selection or input.
 * @return Always true.
 */
static bool do_up(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (eqn->getSelectStart() != nullptr) {
		NodeIterator n(eqn->getSelectStart());
		if (n == eqn->begin()) return false;
		--n;
		eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		in_pos = in->emptyBuffer();
		NodeIterator prev{*in_pos};
		if (prev == eqn->begin()) return false;
		--prev;
		eqn->disableCurrentInput();
		eqn->setSelect(*prev);
	}
	else
		eqn->setSelect(*(eqn->begin()));
	return true;
}

/**
 * Handle down arrow key event.
 * Transverse the entire equation tree to the right
 * of the selection or the current input. Select last
 * node if there is no selection or input.
 * @return Always true.
 */
static bool do_down(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (eqn->getSelectStart() != nullptr) {
		NodeIterator n(eqn->getSelectEnd());
		if (n == eqn->last()) return false;
		++n;
		eqn->selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		if (in_pos == eqn->last()) return false;
	
		in_pos = in->emptyBuffer();
		NodeIterator nxt{*in_pos};
		++nxt;
		eqn->disableCurrentInput();
		eqn->setSelect(*nxt);
	}
	else
		eqn->setSelect(*(eqn->last()));
	return true;
}

/**
 * Handle up arrow with shift key event.
 * Select the expression to the left of the current input
 * or add it to the existing selection.
 * @return True if up arrow with shift is processed.
 */
static bool do_shift_up(const UI::Event&)
{
	if (eqn->getSelectStart() != nullptr) {
		auto start = FactorIterator(eqn->getSelectStart());
		
		if (start.isBegin()) return false;
		start.setNode(0, 0);
		eqn->setSelect(*start, eqn->getSelectEnd());
	}
	else {
		Input* in = eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;
		in->emptyBuffer();
		auto start = in_pos.getBegin();
		eqn->setSelect(*start, eqn->getSelectEnd());
	}
	return true;
}

/**
 * Handle down arrow with shift key event.
 * Select the expression to the right of the current input
 * or add it to the existing selection.
 * @return True if down arrow with shift is processed.
 */
static bool do_shift_down(const UI::Event&)
{
	if (eqn->getSelectStart() != nullptr) {
		auto end = FactorIterator(eqn->getSelectEnd());
		
		if (end == end.getLast()) return false;
		end.setNode(-1, -1);
		eqn->setSelect(eqn->getSelectStart(), *end);
	}
	else {
		Input* in = eqn->getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos.isEnd()) return false;
		in->emptyBuffer();
		auto end = in_pos.getLast();
		eqn->setSelect(eqn->getSelectStart(), *end);
	}
	return true;
}

/**
 * Handle enter key event.
 * Either disable current input or replace 
 * current selection with a new input.
 * @return True if enter is proccessed.
 */
static bool do_enter(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	Node* start = eqn->getSelectStart();
	if (start != nullptr) {
		auto it = FactorIterator(start);
		eqn->clearSelect();
		it.insert(new Input(*eqn));
	}
	else if (in != nullptr) {
		if (eqn->getCurrentInput()->unremovable()) return false;
		eqn->disableCurrentInput();
		eqn->nextInput();
	}
	else
		return false;
	return true;
}

/**
 * Handle +/- key event.
 * Add a new term with a new input. New term is positive 
 * or negative depending on whether character is +/-.
 * @return True if input is active.
 */
static bool do_plus_minus(const UI::Event& event)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	FactorIterator in_pos(in);
	in_pos = in->emptyBuffer();
	if (in_pos.isBeginTerm() && in->empty()) {
		in_pos.insert(new Input(*eqn));
		++in_pos;
		in->makeCurrent();
	}
	in_pos.insert(in_pos.splitTerm(((char) event.getKey()) == '-'));
	return true;
}

/**
 * Insert divide term.
 * @return True if successful.
 */
static bool do_divide(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("divide", *eqn);
}

/**
 * Insert power factor.
 * @return True if successfull.
 */
static bool do_power(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("power", *eqn);
}

/**
 * Handle left parenthesis key event.
 * Add "(*)". Side effect will be to add any functions
 * that matches or an expression as a new factor.
 */
static bool do_left_parenthesis(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	in->add("(#)");
	eqn->disableCurrentInput();
	return true;
}

/**
 * Handle space key event.
 * Either select factor at input or select one level higher.
 * @return True if successful.
 */
static bool do_space(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in != nullptr) {
		if (in->empty()) return false;

		auto pos = eqn->disableCurrentInput();
		eqn->setSelect(*pos);
	}
	else if (eqn->getSelectStart() != nullptr) {
		if (eqn->getSelectStart()->getParent() == nullptr) return false;

		Node* parent = nullptr;
		int num = 0;
		Node* start = eqn->getSelectStart();
		Node* end   = eqn->getSelectEnd();
		
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
			eqn->setSelect(*(term->begin()), *(--term->end()));
		}
		else
			eqn->setSelect(parent);
	}
	else {
		eqn->setSelect(eqn->getRoot()->first());
	}
	return true;
}

/**
 * Map of events to functions that handle these events.
 */
static const unordered_map<UI::Event, event_handler> event_map = {
	{ UI::Event(UI::Mouse::RELEASED, 1), do_mouse_released },
	{ UI::Event(UI::Mouse::PRESSED,  1), do_mouse_pressed },
	{ UI::Event(UI::Mouse::CLICKED,  1), do_mouse_clicked },
	{ UI::Event(UI::Mouse::DOUBLE,   1), do_mouse_double },
    { UI::Event(UI::Mouse::POSITION, 0), do_mouse_position },
	
	{ UI::Event(UI::Keys::LEFT,  UI::Modifiers::SHIFT), do_shift_right },
	{ UI::Event(UI::Keys::RIGHT, UI::Modifiers::SHIFT), do_shift_left },
	{ UI::Event(UI::Keys::UP,    UI::Modifiers::SHIFT), do_shift_up },
	{ UI::Event(UI::Keys::DOWN,  UI::Modifiers::SHIFT), do_shift_down },
	
	{ UI::Event(UI::Keys::PLUS),   do_plus_minus },
	{ UI::Event(UI::Keys::MINUS),  do_plus_minus },
	{ UI::Event(UI::Keys::DIVIDE), do_divide },
	{ UI::Event(UI::Keys::POWER),  do_power },
	{ UI::Event(UI::Keys::L_PAR),  do_left_parenthesis },
	{ UI::Event(UI::Keys::SPACE),  do_space },
	{ UI::Event(UI::Keys::LEFT),   do_left },
	{ UI::Event(UI::Keys::RIGHT),  do_right },
	{ UI::Event(UI::Keys::UP),     do_up },
	{ UI::Event(UI::Keys::DOWN),   do_down },
	{ UI::Event(UI::Keys::ENTER),  do_enter },
	{ UI::Event(UI::Keys::BSPACE), do_backspace },
	{ UI::Event(UI::Keys::CTRL_Q), do_quit },
	{ UI::Event(UI::Keys::CTRL_S), do_save },
	{ UI::Event(UI::Keys::CTRL_Z), do_undo },
	{ UI::Event(UI::Keys::DOT),    do_key },
	
	{ UI::Event(UI::Keys::K0), do_key },
	{ UI::Event(UI::Keys::K1), do_key },
	{ UI::Event(UI::Keys::K2), do_key },
	{ UI::Event(UI::Keys::K3), do_key },
	{ UI::Event(UI::Keys::K4), do_key },
	{ UI::Event(UI::Keys::K5), do_key },
	{ UI::Event(UI::Keys::K6), do_key },
	{ UI::Event(UI::Keys::K7), do_key },
	{ UI::Event(UI::Keys::K8), do_key },
	{ UI::Event(UI::Keys::K9), do_key },
	{ UI::Event(UI::Keys::A), do_key },
	{ UI::Event(UI::Keys::B), do_key },
	{ UI::Event(UI::Keys::C), do_key },
	{ UI::Event(UI::Keys::D), do_key },
	{ UI::Event(UI::Keys::E), do_key },
	{ UI::Event(UI::Keys::F), do_key },
	{ UI::Event(UI::Keys::H), do_key },
	{ UI::Event(UI::Keys::I), do_key },
	{ UI::Event(UI::Keys::J), do_key },
	{ UI::Event(UI::Keys::K), do_key },
	{ UI::Event(UI::Keys::L), do_key },
	{ UI::Event(UI::Keys::M), do_key },
	{ UI::Event(UI::Keys::N), do_key },
	{ UI::Event(UI::Keys::O), do_key },
	{ UI::Event(UI::Keys::P), do_key },
	{ UI::Event(UI::Keys::Q), do_key },
	{ UI::Event(UI::Keys::R), do_key },
	{ UI::Event(UI::Keys::S), do_key },
	{ UI::Event(UI::Keys::T), do_key },
	{ UI::Event(UI::Keys::U), do_key },
	{ UI::Event(UI::Keys::V), do_key },
	{ UI::Event(UI::Keys::W), do_key },
	{ UI::Event(UI::Keys::X), do_key },
	{ UI::Event(UI::Keys::Y), do_key },
	{ UI::Event(UI::Keys::Z), do_key },
	{ UI::Event(UI::Keys::a), do_key },
	{ UI::Event(UI::Keys::b), do_key },
	{ UI::Event(UI::Keys::c), do_key },
	{ UI::Event(UI::Keys::d), do_key },
	{ UI::Event(UI::Keys::e), do_key },
	{ UI::Event(UI::Keys::f), do_key },
	{ UI::Event(UI::Keys::g), do_key },
	{ UI::Event(UI::Keys::h), do_key },
	{ UI::Event(UI::Keys::i), do_key },
	{ UI::Event(UI::Keys::j), do_key },
	{ UI::Event(UI::Keys::k), do_key },
	{ UI::Event(UI::Keys::l), do_key },
	{ UI::Event(UI::Keys::m), do_key },
	{ UI::Event(UI::Keys::n), do_key },
	{ UI::Event(UI::Keys::o), do_key },
	{ UI::Event(UI::Keys::p), do_key },
	{ UI::Event(UI::Keys::q), do_key },
	{ UI::Event(UI::Keys::r), do_key },
	{ UI::Event(UI::Keys::s), do_key },
	{ UI::Event(UI::Keys::t), do_key },
	{ UI::Event(UI::Keys::u), do_key },
	{ UI::Event(UI::Keys::v), do_key },
	{ UI::Event(UI::Keys::w), do_key },
	{ UI::Event(UI::Keys::x), do_key },
	{ UI::Event(UI::Keys::y), do_key },
	{ UI::Event(UI::Keys::z), do_key },
};

void UI::doMainLoop(int argc, char* argv[], Graphics& graphicsContext)
{
	LOG_TRACE_MSG("Starting event loop...");

	if (argc > 1) {
		ifstream in(argv[1]);
		eqn = new Equation(in);
	}
	else
		eqn = new Equation("#");

	eqns.save(eqn);
	gc = &graphicsContext;
	bool fChanged = true;
	while (fRunning) {
		if (fChanged) {
			eqns.save(eqn);
			delete eqn;
			eqn = eqns.top();
			eqn->draw(*gc, true);
			fChanged = false;
		}
		
		int xCursor = 0, yCursor = 0;
		eqn->getCursorOrig(xCursor, yCursor);
		auto event = gc->getNextEvent(xCursor, yCursor, eqn->blink());
		if (event_map.find(event) != event_map.end()) {
			fChanged = event_map.at(event)(event);
		}
		else
			fChanged = false;
	}
}
