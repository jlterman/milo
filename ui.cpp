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
 * @file ui.cpp
 * This file implements the milo user interface implementing user generated
 * events.
 */

#include <fstream>
#include <unordered_map>
#include "ui.h"
#include "milo.h"

using namespace std;
using namespace UI;

static bool fRunning = true;         ///< When false, quit program.

/**
 * Handle quit event.
 * Set GlobalContext::fRunning false to exit main loop.
 * @return Always false.
 */
static bool do_quit()
{
	fRunning = false;
	return false;
}

/**
 * Handle save event.
 * Save equation to file "eqn.xml".
 * @return Always true.
 */
static bool do_save()
{
	string store;
	GlobalContext::current->getEqn().xml_out(store);
	fstream out("eqn.xml", fstream::out | fstream::trunc);
	out << store << endl;
	out.close();
	return true;
}

/**
 * Handle undo event.
 * If non empty, pop and load an equation off the top of the undo stack.
 * @return Always return false.
 */
static bool do_undo()
{
	GlobalContext::current->undoEqn();
	return false;
}

/**
 * Handle mouse press event.
 * If node has been selected, remember it and mouse coordinates.
 * @return Always false.
 */
static bool do_mouse_pressed(const MouseEvent& mouse)
{
	mouse.getCoords(GlobalContext::current->start_mouse_x, GlobalContext::current-> start_mouse_y);
	LOG_TRACE_MSG("mouse press x: " + to_string(GlobalContext::current->start_mouse_x) +
				            ", y: " + to_string(GlobalContext::current->start_mouse_y));
	GlobalContext::current->start_select = GlobalContext::current->getEqn().findNode(
				       GlobalContext::current->getGraphics(),
					   GlobalContext::current->start_mouse_x,
					   GlobalContext::current->start_mouse_y
				   );
	if (GlobalContext::current->start_select == nullptr) {
		return false;
	}
	LOG_TRACE_MSG("node found: " + GlobalContext::current->start_select->toString());
	GlobalContext::current->getEqn().setSelect(GlobalContext::current->start_select);
	GlobalContext::current->getEqn().draw(GlobalContext::current->getGraphics(), true);
	return false;
}

/**
 * Handle mouse release event.
 * If mouse released where it was pressed, select node.
 * Otherwise, end mouse dragging state.
 * @return Always true.
 */
static bool do_mouse_released(const MouseEvent&)
{
	LOG_TRACE_MSG("mouse release");
	GlobalContext::current->start_mouse_x = GlobalContext::current->start_mouse_y = -1;
	if (GlobalContext::current->start_select != nullptr &&
		GlobalContext::current->start_select->getSelect() == Node::Select::ALL)
	{
		GlobalContext::current->getEqn().selectNodeOrInput(GlobalContext::current->start_select);
	}
	GlobalContext::current->start_select = nullptr;
	return true;
}

/**
 * Handle mouse click.
 * If mouse clicks on node select it or activate input.
 * @return True if node selected.
 */
static bool do_mouse_clicked(const MouseEvent& mouse)
{
	int mouse_x, mouse_y;
	mouse.getCoords(mouse_x, mouse_y);
	LOG_TRACE_MSG("mouse clicked x: " + to_string(mouse_x) + ", y: " + to_string(mouse_y));
	Node* node = GlobalContext::current->getEqn().findNode(GlobalContext::current->getGraphics(), mouse_x, mouse_y);
	if (node == nullptr) return false;
	LOG_TRACE_MSG("node found: " + node->toString());
	GlobalContext::current->getEqn().clearSelect();
	GlobalContext::current->getEqn().selectNodeOrInput(node);
	GlobalContext::current->getEqn().draw(GlobalContext::current->getGraphics(), true);
	return true;	
}

/**
 * Handle mouse double click.
 * If no node found, return false. Otherwise activate if input selected
 * or add input before selected node.
 * @return True, if node found.
 */
static bool do_mouse_double(const MouseEvent& mouse)
{
	int mouse_x, mouse_y;
	mouse.getCoords(mouse_x, mouse_y);
	LOG_TRACE_MSG("mouse double clicked x: " + to_string(mouse_x) + ", y: " + to_string(mouse_y));
	Node* node = GlobalContext::current->getEqn().findNode(GlobalContext::current->getGraphics(), mouse_x, mouse_y);
	if (node == nullptr || node == GlobalContext::current->getEqn().getCurrentInput()) return false;
	LOG_TRACE_MSG("node found: " + node->toString());
	if (node->getType() == Input::type) {
		GlobalContext::current->getEqn().selectNodeOrInput(node);
		return true;
	}				   
	if (GlobalContext::current->getEqn().getCurrentInput() != nullptr) GlobalContext::current->getEqn().disableCurrentInput();
	GlobalContext::current->getEqn().clearSelect();
	auto it = FactorIterator(node);
	it.insert(new Input(GlobalContext::current->getEqn()));
	return true;
}

/**
 * Handle mouse position reports,
 * Update part of equation selected by mouse dragging.
 * @return Always false.
 */
static bool do_mouse_position(const MouseEvent& mouse)
{
	Box box = { 0, 0, 0, 0 };
	if (GlobalContext::current->start_mouse_x > 0 && GlobalContext::current->start_mouse_y > 0) {
		int event_x = -1, event_y = -1;
		mouse.getCoords(event_x, event_y);
		box = { abs(GlobalContext::current->start_mouse_x - event_x),
				abs(GlobalContext::current->start_mouse_y - event_y),
				min(GlobalContext::current->start_mouse_x,  event_x),
				min(GlobalContext::current->start_mouse_y,  event_y)
		};
		LOG_TRACE_MSG("mouse position current box: " + box.toString());
	}
	else {
		mouse.getCoords(GlobalContext::current->start_mouse_x, GlobalContext::current->start_mouse_y);
	}
	if (GlobalContext::current->start_select != nullptr && box.area() > 1 ) {
		Node* new_select = GlobalContext::current->getEqn().findNode(GlobalContext::current->getGraphics(), box);
		LOG_TRACE_MSG("node found: " + ((new_select == nullptr) ? "None" : new_select->toString()));
		if (new_select != nullptr && new_select->getDepth() < GlobalContext::current->start_select->getDepth()) {
			GlobalContext::current->start_select = new_select;
		}
		GlobalContext::current->getEqn().selectBox(
		    GlobalContext::current->getGraphics(),
			GlobalContext::current->start_select,
			box
		);
		GlobalContext::current->getEqn().draw(GlobalContext::current->getGraphics(), true);
	}
	else if (GlobalContext::current->start_select == nullptr && box.area() > 0) {
		GlobalContext::current->start_select = GlobalContext::current->getEqn().findNode(
											       GlobalContext::current->getGraphics(),
												   box
			                                   );
		LOG_TRACE_MSG("node found: " + ((GlobalContext::current->start_select == nullptr) ?
										"None" :
										GlobalContext::current->start_select->toString()));
		if (GlobalContext::current->start_select != nullptr) {
			GlobalContext::current->getEqn().setSelect(GlobalContext::current->start_select);
			GlobalContext::current->getEqn().draw(GlobalContext::current->getGraphics(), true);
		}
	}
	return false;
}

/**
 * Handle letter or number key event.
 * Insert letter or number key into equation if there is a current input or selection.
 * @param event Key event.
 * @return True if key was inserted.
 */
static bool do_key(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		GlobalContext::current->getEqn().eraseSelection(
	        new Input(GlobalContext::current->getEqn(), string(1, (char)event.getKey()))
		);
	}
	else {
		Input* in = GlobalContext::current->getEqn().getCurrentInput();
		if (in == nullptr) return false;
		in->add((char)event.getKey());
	}
	return true;
}

/**
 * Handle backspace key event.
 * Erase selection if it exists, or previous factor before current input.
 * @param event Key event.
 * @return True if backspace is processed.
 */
static bool do_backspace(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		GlobalContext::current->getEqn().eraseSelection(new Input(GlobalContext::current->getEqn()));
	}
	else {
		Input* in = GlobalContext::current->getEqn().getCurrentInput();
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
			GlobalContext::current->getEqn().disableCurrentInput();
			GlobalContext::current->getEqn().setSelect(*prev_pos);
		}
	}
	return true;
}

/**
 * Handle left arrow key event.
 * Move to node to the left of selection or current input.
 * @param event Key event.
 * @return True if left arrow proccessed.
 */
static bool do_left(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		FactorIterator n(GlobalContext::current->getEqn().getSelectStart());
		if (n.isBegin()) return false;
		--n;
		GlobalContext::current->getEqn().selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;
	
		in_pos = in->emptyBuffer();
		FactorIterator prev{in_pos};
		--prev;
		if (prev->getType() == Input::type) {
			GlobalContext::current->getEqn().selectNodeOrInput(*prev);
		}
		else if (in->unremovable()) {
			GlobalContext::current->getEqn().disableCurrentInput();
			prev.insertAfter(new Input(GlobalContext::current->getEqn()));
		} else
			FactorIterator::swap(prev, in_pos);
	}
	else
		GlobalContext::current->getEqn().setSelect(GlobalContext::current->getEqn().getRoot());
	return true;
}

/**
 * Handle right arrow key event.
 * Move to node to the right of selection or current input.
 * @param event Key event.
 * @return True if right arrow proccessed.
 */
static bool do_right(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		FactorIterator n(GlobalContext::current->getEqn().getSelectStart());
		++n;
		if (n.isEnd()) return false;
		GlobalContext::current->getEqn().selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		Input* in =  GlobalContext::current->getEqn().getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos == in_pos.getLast()) return false;

		in_pos = in->emptyBuffer();
		FactorIterator nxt{in_pos};
		++nxt;
		if (nxt->getType() == Input::type) {
			GlobalContext::current->getEqn().selectNodeOrInput(*nxt);
		}
		else if (in->unremovable()) {
			GlobalContext::current->getEqn().disableCurrentInput();
			nxt.insert(new Input(GlobalContext::current->getEqn()));;
		} else
			FactorIterator::swap(nxt, in_pos);
	}
	else
		GlobalContext::current->getEqn().setSelect(GlobalContext::current->getEqn().getRoot()->last());
	return true;
}

/**
 * Handle left arrow with shift key event.
 * Either add factor on the left to selection or 
 * select factor to left of cursor.
 * @param event Key event.
 * @return True if shift left arrow is processed.
 */
static bool do_shift_left(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		auto start = FactorIterator(GlobalContext::current->getEqn().getSelectStart());
		
		if (start.isBegin()) return false;
		--start;

		GlobalContext::current->getEqn().setSelect(*start, GlobalContext::current->getEqn().getSelectEnd());
	}
	else {
		Input* in =  GlobalContext::current->getEqn().getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;

		in_pos = GlobalContext::current->getEqn().disableCurrentInput();
		GlobalContext::current->getEqn().setSelect(*in_pos, *in_pos);
	}
	return true;
}

/**
 * Handle right arrow with shift key event.
 * Either add factor on the right to selection or 
 * select factor to right of cursor.
 * @param event Key event.
 * @return True if shift right arrow is processed.
 */
static bool do_shift_right(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		auto end = FactorIterator(GlobalContext::current->getEqn().getSelectEnd());
		
		if (end == end.getLast()) return false;
		++end;

		GlobalContext::current->getEqn().setSelect(GlobalContext::current->getEqn().getSelectStart(), *end);
	}
	else {
		Input* in = GlobalContext::current->getEqn().getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos.isEnd()) return false;

		in_pos = ++GlobalContext::current->getEqn().disableCurrentInput();
		GlobalContext::current->getEqn().setSelect(*in_pos, *in_pos);
	}
	return true;
}

/**
 * Handle up arrow key event.
 * Transverse the entire equation tree to the left 
 * of the selection or the current input. Select first
 * node if there is no selection or input.
 * @param event Key event.
 * @return Always true.
 */
static bool do_up(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		NodeIterator n(GlobalContext::current->getEqn().getSelectStart());
		if (n == GlobalContext::current->getEqn().begin()) return false;
		--n;
		GlobalContext::current->getEqn().selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		in_pos = in->emptyBuffer();
		NodeIterator prev{*in_pos};
		if (prev == GlobalContext::current->getEqn().begin()) return false;
		--prev;
		GlobalContext::current->getEqn().disableCurrentInput();
		GlobalContext::current->getEqn().setSelect(*prev);
	}
	else
		GlobalContext::current->getEqn().setSelect(*(GlobalContext::current->getEqn().begin()));
	return true;
}

/**
 * Handle down arrow key event.
 * Transverse the entire equation tree to the right
 * of the selection or the current input. Select last
 * node if there is no selection or input.
 * @param event Key event.
 * @return Always true.
 */
static bool do_down(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		NodeIterator n(GlobalContext::current->getEqn().getSelectEnd());
		if (n == GlobalContext::current->getEqn().last()) return false;
		++n;
		GlobalContext::current->getEqn().selectNodeOrInput(*n);
	}
	else if (in != nullptr) {
		FactorIterator in_pos(in);
		if (in_pos == GlobalContext::current->getEqn().last()) return false;
	
		in_pos = in->emptyBuffer();
		NodeIterator nxt{*in_pos};
		++nxt;
		GlobalContext::current->getEqn().disableCurrentInput();
		GlobalContext::current->getEqn().setSelect(*nxt);
	}
	else
		GlobalContext::current->getEqn().setSelect(*(GlobalContext::current->getEqn().last()));
	return true;
}

/**
 * Handle up arrow with shift key event.
 * Select the expression to the left of the current input
 * or add it to the existing selection.
 * @param event Key event.
 * @return True if up arrow with shift is processed.
 */
static bool do_shift_up(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		auto start = FactorIterator(GlobalContext::current->getEqn().getSelectStart());
		
		if (start.isBegin()) return false;
		start.setNode(0, 0);
		GlobalContext::current->getEqn().setSelect(*start, GlobalContext::current->getEqn().getSelectEnd());
	}
	else {
		Input* in = GlobalContext::current->getEqn().getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in->empty() && in_pos.isBegin()) return false;
		in->emptyBuffer();
		auto start = in_pos.getBegin();
		GlobalContext::current->getEqn().setSelect(*start, GlobalContext::current->getEqn().getSelectEnd());
	}
	return true;
}

/**
 * Handle down arrow with shift key event.
 * Select the expression to the right of the current input
 * or add it to the existing selection.
 * @param event Key event.
 * @return True if down arrow with shift is processed.
 */
static bool do_shift_down(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		auto end = FactorIterator(GlobalContext::current->getEqn().getSelectEnd());
		
		if (end == end.getLast()) return false;
		end.setNode(-1, -1);
		GlobalContext::current->getEqn().setSelect(GlobalContext::current->getEqn().getSelectStart(), *end);
	}
	else {
		Input* in = GlobalContext::current->getEqn().getCurrentInput();
		if (in == nullptr) return false;

		FactorIterator in_pos(in);
		if (in_pos.isEnd()) return false;
		in->emptyBuffer();
		auto end = in_pos.getLast();
		GlobalContext::current->getEqn().setSelect(GlobalContext::current->getEqn().getSelectStart(), *end);
	}
	return true;
}

/**
 * Handle enter key event.
 * Either disable current input or replace 
 * current selection with a new input.
 * @param event Key event.
 * @return True if enter is proccessed.
 */
static bool do_enter(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	Node* start = GlobalContext::current->getEqn().getSelectStart();
	if (start != nullptr) {
		auto it = FactorIterator(start);
		GlobalContext::current->getEqn().clearSelect();
		it.insertAfter(new Input(GlobalContext::current->getEqn()));
	}
	else if (in != nullptr) {
		if (GlobalContext::current->getEqn().getCurrentInput()->unremovable()) return false;
		GlobalContext::current->getEqn().disableCurrentInput();
		GlobalContext::current->getEqn().nextInput(false);
	}
	else
		return false;
	return true;
}

/**
 * Handle tab key event.
 * Move to next input. If shift down move to previous input.
 * @param event Key event.
 * @return Always true
 */
static bool do_tab(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	GlobalContext::current->getEqn().nextInput(event.shiftMod());
	return true;
}

/**
 * Handle +/- key event.
 * Add a new term with a new input. New term is positive 
 * or negative depending on whether character is +/-.
 * @param event Key event.
 * @return True if input is active.
 */
static bool do_plus_minus(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (in == nullptr) return false;

	FactorIterator in_pos(in);
	in_pos = in->emptyBuffer();
	if (in_pos.isBeginTerm() && in->empty()) {
		in_pos.insert(new Input(GlobalContext::current->getEqn()));
		++in_pos;
		in->makeCurrent();
	}
	in_pos.insert(in_pos.splitTerm(((char) event.getKey()) == '-'));
	return true;
}

/**
 * Insert divide term.
 * @param event Key event.
 * @return True if successful.
 */
static bool do_divide(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("divide", GlobalContext::current->getEqn());
}

/**
 * Insert power factor.
 * @param event Key event.
 * @return True if successfull.
 */
static bool do_power(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("power", GlobalContext::current->getEqn());
}

/**
 * Handle left parenthesis key event.
 * Add "(*)". Side effect will be to add any functions
 * that matches or an expression as a new factor.
 * @param event Key event.
 * @return True if successful.
 */
static bool do_left_parenthesis(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (!in) return false;

	in->add("(#)");
	GlobalContext::current->getEqn().disableCurrentInput();
	return true;
}

/**
 * Handle space key event.
 * Either select factor at input or select one level higher.
 * @param event Key event.
 * @return True if successful.
 */
static bool do_space(const KeyEvent& event)
{
	LOG_TRACE_MSG(event.toString());
	Input* in = GlobalContext::current->getEqn().getCurrentInput();
	if (in != nullptr) {
		if (in->empty()) return false;

		auto pos = GlobalContext::current->getEqn().disableCurrentInput();
		GlobalContext::current->getEqn().setSelect(*pos);
	}
	else if (GlobalContext::current->getEqn().getSelectStart() != nullptr) {
		if (GlobalContext::current->getEqn().getSelectStart()->getParent() == nullptr) return false;

		Node* parent = nullptr;
		int num = 0;
		Node* start = GlobalContext::current->getEqn().getSelectStart();
		Node* end   = GlobalContext::current->getEqn().getSelectEnd();
		
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
			GlobalContext::current->getEqn().setSelect(*(term->begin()), *(--term->end()));
		}
		else
			GlobalContext::current->getEqn().setSelect(parent);
	}
	else {
		GlobalContext::current->getEqn().setSelect(GlobalContext::current->getEqn().getRoot()->first());
	}
	return true;
}

/**
 * Refresh screen.
 * Returning true will cause UI to redraw screen.
 * @return Always false;
 */
static bool do_refresh()
{
	GlobalContext::current->getGraphics().refresh();
	return false;
}

/**
 * Function pointer to handle a menu item.
 * Return true, if event changed equation.
 */
using menu_handler = bool (*)(void);

/**
 * Map of names to functions that handle menu items.
 */
static const unordered_map<string, menu_handler> menu_map = {
	{ "refresh", do_refresh },
	{ "quit", do_quit },
	{ "save", do_save },
	{ "undo", do_undo }
};

/**
 * Function pointer to handle a mouse even
 * Return true, if event changed equation.
 */
using mouse_handler = bool (*)(const MouseEvent&);

/**
 * Map of events to functions that handle mouse events.
 */
static const unordered_map<UI::MouseEvent, mouse_handler> mouse_event_map = {
	{ UI::MouseEvent(Mouse::RELEASED, 1), do_mouse_released },
	{ UI::MouseEvent(Mouse::PRESSED,  1), do_mouse_pressed },
	{ UI::MouseEvent(Mouse::CLICKED,  1), do_mouse_clicked },
	{ UI::MouseEvent(Mouse::DOUBLE,   1), do_mouse_double },
    { UI::MouseEvent(Mouse::POSITION, 0), do_mouse_position }
};

/**
 * Function pointer to handle an event. Pointer to event passed as argument.
 * Return true, if event changed equation.
 */

using key_event_handler = bool (*)(const KeyEvent&);

/**
 * Map of events to functions that handle key events.
 */
static const unordered_map<KeyEvent, key_event_handler> key_event_map = {
	{ KeyEvent(Keys::LEFT,  Modifiers::SHIFT), do_shift_left },
	{ KeyEvent(Keys::RIGHT, Modifiers::SHIFT), do_shift_right },
	{ KeyEvent(Keys::UP,    Modifiers::SHIFT), do_shift_up },
	{ KeyEvent(Keys::DOWN,  Modifiers::SHIFT), do_shift_down },
	{ KeyEvent(Keys::TAB,   Modifiers::SHIFT), do_tab },
	
	{ KeyEvent(Keys::PLUS),   do_plus_minus },
	{ KeyEvent(Keys::MINUS),  do_plus_minus },
	{ KeyEvent(Keys::DIVIDE), do_divide },
	{ KeyEvent(Keys::POWER),  do_power },
	{ KeyEvent(Keys::L_PAR),  do_left_parenthesis },
	{ KeyEvent(Keys::SPACE),  do_space },
	{ KeyEvent(Keys::LEFT),   do_left },
	{ KeyEvent(Keys::RIGHT),  do_right },
	{ KeyEvent(Keys::UP),     do_up },
	{ KeyEvent(Keys::DOWN),   do_down },
	{ KeyEvent(Keys::TAB),    do_tab },
	{ KeyEvent(Keys::ENTER),  do_enter },
	{ KeyEvent(Keys::BSPACE), do_backspace },
	{ KeyEvent(Keys::DOT),    do_key },
	
	{ KeyEvent(Keys::K0), do_key },
	{ KeyEvent(Keys::K1), do_key },
	{ KeyEvent(Keys::K2), do_key },
	{ KeyEvent(Keys::K3), do_key },
	{ KeyEvent(Keys::K4), do_key },
	{ KeyEvent(Keys::K5), do_key },
	{ KeyEvent(Keys::K6), do_key },
	{ KeyEvent(Keys::K7), do_key },
	{ KeyEvent(Keys::K8), do_key },
	{ KeyEvent(Keys::K9), do_key },
	{ KeyEvent(Keys::A), do_key },
	{ KeyEvent(Keys::B), do_key },
	{ KeyEvent(Keys::C), do_key },
	{ KeyEvent(Keys::D), do_key },
	{ KeyEvent(Keys::E), do_key },
	{ KeyEvent(Keys::F), do_key },
	{ KeyEvent(Keys::H), do_key },
	{ KeyEvent(Keys::I), do_key },
	{ KeyEvent(Keys::J), do_key },
	{ KeyEvent(Keys::K), do_key },
	{ KeyEvent(Keys::L), do_key },
	{ KeyEvent(Keys::M), do_key },
	{ KeyEvent(Keys::N), do_key },
	{ KeyEvent(Keys::O), do_key },
	{ KeyEvent(Keys::P), do_key },
	{ KeyEvent(Keys::Q), do_key },
	{ KeyEvent(Keys::R), do_key },
	{ KeyEvent(Keys::S), do_key },
	{ KeyEvent(Keys::T), do_key },
	{ KeyEvent(Keys::U), do_key },
	{ KeyEvent(Keys::V), do_key },
	{ KeyEvent(Keys::W), do_key },
	{ KeyEvent(Keys::X), do_key },
	{ KeyEvent(Keys::Y), do_key },
	{ KeyEvent(Keys::Z), do_key },
	{ KeyEvent(Keys::a), do_key },
	{ KeyEvent(Keys::b), do_key },
	{ KeyEvent(Keys::c), do_key },
	{ KeyEvent(Keys::d), do_key },
	{ KeyEvent(Keys::e), do_key },
	{ KeyEvent(Keys::f), do_key },
	{ KeyEvent(Keys::g), do_key },
	{ KeyEvent(Keys::h), do_key },
	{ KeyEvent(Keys::i), do_key },
	{ KeyEvent(Keys::j), do_key },
	{ KeyEvent(Keys::k), do_key },
	{ KeyEvent(Keys::l), do_key },
	{ KeyEvent(Keys::m), do_key },
	{ KeyEvent(Keys::n), do_key },
	{ KeyEvent(Keys::o), do_key },
	{ KeyEvent(Keys::p), do_key },
	{ KeyEvent(Keys::q), do_key },
	{ KeyEvent(Keys::r), do_key },
	{ KeyEvent(Keys::s), do_key },
	{ KeyEvent(Keys::t), do_key },
	{ KeyEvent(Keys::u), do_key },
	{ KeyEvent(Keys::v), do_key },
	{ KeyEvent(Keys::w), do_key },
	{ KeyEvent(Keys::x), do_key },
	{ KeyEvent(Keys::y), do_key },
	{ KeyEvent(Keys::z), do_key },
};

void UI::GlobalContext::saveEqn()
{
	m_eqns.save(m_eqn);
	m_eqn = EqnPtr(m_eqns.top());
	m_eqn->draw(*m_gc, true);
}

void UI::GlobalContext::undoEqn()
{
	EqnPtr undo_eqn = m_eqns.undo();
	if (undo_eqn) {
		m_eqn = undo_eqn;
		LOG_TRACE_MSG("undo to " + m_eqn->toString());
		m_eqn->draw(*m_gc, true);
	}
}

GlobalContext::GlobalContext(Graphics* gc) : m_gc(gc), m_eqn(new Equation("#")) {}

GlobalContext::GlobalContext(Graphics* gc, std::istream& is) : m_gc(gc), m_eqn(new Equation(is)) {}

shared_ptr<UI::GlobalContext> UI::GlobalContext::current = nullptr;

static const unordered_map<enum Mouse, string> mouse_string = {
	{ POSITION, "POSITION" }, { PRESSED, "PRESSED" }, { RELEASED, "RELEASED" },
	{ CLICKED, "CLICKED" }, { DOUBLE, "DOUBLE" }
};

static const unordered_map<enum Modifiers, string> mod_string = {
	{ ALT, "ALT-" }, { SHIFT, "SHIFT-" }, { ALT_SHIFT, "ALT-SHIFT-" }, { CTRL, "CTRL-" },
	{ CTRL_SHIFT, "CTRL-SHIFT-" }, { CTRL_ALT_SHIFT, "CTRL-ALT-SHIFT-" }, { NO_MOD, string() }
};

static const unordered_map<enum Keys, string> key_string = {
	{ F1, "F1" }, { F2, "F2" }, { F3, "F3" }, { F4, "F4" }, { F5, "F5" }, { F6, "F6" }, 
	{ F7, "F7" }, { F8, "F8" }, { F9, "F9" }, { F10, "F10" }, { F11, "F11" }, { F12, "F12" },
	{ INS, "INS" }, { DEL, "DEL" }, { HOME, "HOME" }, { END, "END" }, { PAGE_UP, "PAGE_UP" },
	{ PAGE_DOWN, "PAGE_DOWN" }, { UP, "UP" }, { DOWN, "DOWN" }, { LEFT, "LEFT" },
	{ RIGHT, "RIGHT" }, { BSPACE, "BACKSPACE" }, { SPACE, "SPACE" }
};

static const unordered_map<string, enum Modifiers> stringToMod = {
	{ "ALT", ALT }, { "SHIFT", SHIFT }, { "ALT_SHIFT", ALT_SHIFT }, { "CTRL", CTRL },
	{ "CTRL_SHIFT", CTRL_SHIFT }, { "CTRL_ALT_SHIFT", CTRL_ALT_SHIFT }, { string(), NO_MOD }
};

static const unordered_map<string, enum Keys> stringToKey = {
	{ "F1", F1 }, { "F2", F2 }, { "F3", F3 }, { "F4", F4 }, { "F5", F5 }, { "F6", F6 }, 
	{ "F7", F7 }, { "F8", F8 }, { "F9", F9 }, { "F10", F10 }, { "F11", F11 }, { "F12", F12 },
	{ "INS", INS }, { "DEL", DEL }, { "\u2302", HOME }, { "END", END }, { "PAGE\u25b2", PAGE_UP },
	{ "PAGE\u25bc", PAGE_DOWN }, { "\u25b2", UP }, { "\u25bc", DOWN }, { "\u25c0", LEFT },
	{ "\u25b6", RIGHT }, { "BSP", BSPACE }, { "SP", SPACE }, { "TAB", TAB }, { "ENTER", ENTER},
	{ "ESC", ESC },	{ "HOME", HOME }, { "PAGE_UP", PAGE_UP }, { "PAGE_DOWN", PAGE_DOWN },
	{ "UP", UP }, { "DOWN", DOWN },	{ "LEFT", LEFT }, { "RIGHT", RIGHT }, { "PLUS", PLUS }, { "MINUS", MINUS }
};

UI::KeyEvent::KeyEvent(const string& key)
{
	auto dash = key.find("-");
	m_mod = NO_MOD;
	m_key = NO_KEY;
	if (dash != string::npos) {
		auto it = stringToMod.find(key.substr(0, dash));
		if (it == stringToMod.end()) {
			return;
		}
		m_mod = it->second;
	}
	string letter = key.substr(dash+1);	 
	if (letter.length() == 1 && m_mod == NO_MOD) {
		m_key = Keys(letter[0]);
	}
	else if (letter.length() == 1 && isalpha(letter[0]) && m_mod == CTRL) {
		m_key = Keys(char(letter[0]) - '@');
		m_mod = NO_MOD;
	}
	else {
		auto it = stringToKey.find(letter);
		if (it == stringToKey.end()) {
			m_mod = NO_MOD;
			return;
		}
		m_key = it->second;
	}
}

string UI::MouseEvent::toString() const
{
	return string("Mouse event: ") +
		   mod_string.at(m_mod) +
		   mouse_string.at(m_type) +
		   "-" + to_string(m_button);
}

string UI::KeyEvent::toString() const
{

	if ( m_key < ESC )
		return string("Key event: " + mod_string.at(CTRL) + string(1, (char) '@' + m_key));
	else if ( m_key > SPACE && m_key < F1)
		return string("key event: " + mod_string.at(m_mod) +  string(1, (char) m_key));
	else
		return string("Key event: ") + mod_string.at(m_mod) + key_string.at(m_key);
}

bool UI::isRunning()
{
	return fRunning;
}

bool UI::doMenu(const string& menuFunctionName)
{
	auto menu_entry = menu_map.find(menuFunctionName);
	if (menu_entry != menu_map.end()) {
		return (menu_entry->second)();
	}
	return false;
}

bool UI::doKey(const UI::KeyEvent& key)
{
	auto key_entry = key_event_map.find(key);
	if (key_entry != key_event_map.end()) {
		return (key_entry->second)(key);
	}
	return false;
								  
}

bool UI::doMouse(const UI::MouseEvent& mouse)
{
	auto mouse_entry = mouse_event_map.find(mouse);
	if (mouse_entry != mouse_event_map.end()) {
		return (mouse_entry->second)(mouse);
	}
	return false;
								  
}

void UI::GlobalContext::parse_menu(XML::Parser& in)
{
	in.next(XML::HEADER, "menu").next(XML::NAME_VALUE);

	string type;
	if (!in.getAttribute("type", type)) throw logic_error("type not found");
	if (type == "line") {
		define_menu_line();
		in.next(XML::ATOM_END);
		return;
	}

	string value;
	string name;
	if (!in.getAttribute("name", name)) throw logic_error("name not found");
	if (type == "menu") {
	    StringMap attributes;
		attributes.emplace("name", name);
		if (!in.getAttribute("active", value)) throw logic_error(s + " not found");
		if (value != "true" && value != "false") throw logic_error("bad boolean value");
		attributes.emplace("active", value);
		if (in.getAttribute("title", value))
			attributes.emplace("title", value);
		else
			attributes.emplace("title", name);

		define_menu(attributes);
		in.next(XML::HEADER_END);
		while (in.check(XML::HEADER, "menu")) parse_menu(in);
		in.next(XML::FOOTER);
		define_menu_end(name);
	}
	else if (type == "item") {
		StringMap attributes;
		attributes.emplace("name", name);
		for ( const string& s : { "active", "action", "key" } ) {
			if (!in.getAttribute(s, value)) throw logic_error(s + " not found");
			attributes.emplace(s, value);
		}
		if (in.getAttribute("title", value))
			attributes.emplace("title", value);
		else
			attributes.emplace("title", name);

		define_menu_item(attributes);
		in.next(XML::ATOM_END);
	}
	else
		throw logic_error("unknown type: " + value);
}

void UI::GlobalContext::parse_menubar(istream& is)
{
	XML::Parser in(is, "menubar");
	while (in.check(XML::HEADER, "menu")) parse_menu(in);
	in.next(XML::FOOTER);
}
