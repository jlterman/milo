#include <fstream>
#include <unordered_map>
#include "ui.h"
#include "milo.h"

using namespace std;

static bool fRunning = true;
static Node* start_select = nullptr;
static int start_mouse_x = -1;
static int start_mouse_y = -1;
static Graphics* gc = nullptr;
static EqnUndoList eqns;
static Equation* eqn;

typedef bool (*event_handler)(const UI::Event&);

static bool do_quit(const UI::Event&)
{
	fRunning = false;
	return true;
}

static bool do_save(const UI::Event&)
{
	string store;
	eqn->xml_out(store);
	fstream out("eqn.xml", fstream::out | fstream::trunc);
	out << store << endl;
	out.close();
	return true;
}

static bool do_undo(const UI::Event&)
{
	Equation* undo_eqn = eqns.undo();
	if (undo_eqn) {
		delete eqn;
		eqn = undo_eqn;
		LOG_TRACE_MSG("undo to " + eqn->toString());
		eqn->draw(*gc, true);
	}
	return true;
}

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

static bool do_mouse_released(const UI::Event&)
{
	start_mouse_x = start_mouse_y = -1;
	if (start_select->getSelect() == Node::Select::ALL) {
		eqn->selectNodeOrInput(start_select);
	}
	start_select = nullptr;
	return true;
}

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

static bool do_mouse_double(const UI::Event&)
{
	int mouse_x = -1, mouse_y = -1;
	gc->getMouseCoords(mouse_x, mouse_y);
	Node* node = eqn->findNode(*gc, mouse_x, mouse_y);
	if (node == nullptr) return false;
	if (eqn->getCurrentInput() != nullptr) eqn->disableCurrentInput();
	eqn->clearSelect();
	auto it = FactorIterator(node);
	it.insert(new Input(*eqn));
	return true;
}

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

static bool do_enter(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (eqn->getSelectStart() != nullptr) {
		auto it = FactorIterator(eqn->getSelectStart());
		eqn->clearSelect();
		it.insert(new Input(*eqn));
	}
	else if (in != nullptr) {
		if (eqn->getCurrentInput()->unremovable()) return false;
		eqn->disableCurrentInput();
		eqn->nextInput();
	}
	return true;
}

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

static bool do_divide(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("divide", *eqn);
}

static bool do_power(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	return Node::createNodeByName("power", *eqn);
}

static bool do_left_parenthesis(const UI::Event&)
{
	Input* in = eqn->getCurrentInput();
	if (in == nullptr) return false;

	in->add("(#)");
	eqn->disableCurrentInput();
	return true;
}

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
		const UI::Event& event = gc->getNextEvent(xCursor, yCursor, eqn->blink());
		if (event_map.find(event) != event_map.end()) {
			fChanged = event_map.at(event)(event);
		}
		else
			fChanged = false;
	}
}

