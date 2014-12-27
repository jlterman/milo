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

void Equation::asciiArt(Draw& draw)
{
	m_root->calcSize();
	m_root->calcOrig(0, 0);
	draw.set(m_root);
	setSelect(draw);
	m_root->asciiArt(draw);
}

void Equation::setSelect(Draw& draw)
{
	if (m_selectStart == nullptr && m_selectEnd == nullptr) {
		draw.setSelect(0, 0, 0, 0);
	}
	else if (m_selectStart != nullptr && m_selectStart == m_selectEnd) {
		draw.setSelect(m_selectStart);
	}
	else if (m_selectStart != nullptr) {
		int x0 = 0, y0 = 0, x = 0, y = 0;
		auto it = FactorIterator(m_selectStart);
		auto end = FactorIterator(m_selectEnd);
		do {
			if (x0 < it->getOrigX()) x0 = it->getOrigX();
			if (y0 < it->getOrigY()) y0 = it->getOrigY();
			if (x < it->getSizeX()) x = it->getSizeX();
			if (y < it->getSizeY()) y = it->getSizeY();
		}
		while (++it != end);

		draw.setSelect(x, y, x0, y0);
	}
}

void Equation::setCurrentInput(int in_sn)
{
	if (m_input_index >= 0) m_inputs[m_input_index]->m_current = false;
	if (in_sn < 0) {
		m_input_index = -1;
		return;
	}
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


void Divide::calcSize()
{
	m_first->calcSize();
	m_second->calcSize();
	setSize( max(m_first->getSizeX(), m_second->getSizeX()),
					 m_first->getSizeY() + 1 + m_second->getSizeY() );
	setBaseLine(m_first->getSizeY());
}

void Divide::calcOrig(int x, int y)
{
	setOrig(x, y);
	m_first->calcOrig(x + (getSizeX() - m_first->getSizeX())/2, y);
	m_second->calcOrig(x + (getSizeX() - m_second->getSizeX())/2, 
				 		   y + m_first->getSizeY() + 1);
}


void Divide::asciiArt(Draw& draw) const
{
	draw.horiz_line(getSizeX(), getOrigX(), getOrigY() + getBaseLine());
	m_first->asciiArt(draw);
	m_second->asciiArt(draw);
}

Complex Divide::getNodeValue() const
{
	Complex a = m_first->getValue();
	Complex b = m_second->getValue();
	return a / b;
}

void Power::calcSize()
{
	m_first->calcSize();
	m_second->calcSize();
	setSize(m_first->getSizeX() + m_second->getSizeX(),
				m_first->getSizeY() + m_second->getSizeY());
	setBaseLine(m_second->getSizeY());
}

void Power::calcOrig(int x, int y)
{
	setOrig(x, y);
	m_first->calcOrig(x, y + m_second->getSizeY());
	m_second->calcOrig(x + m_first->getSizeX(), y);
}

void Power::asciiArt(Draw& draw) const
{
	m_first->asciiArt(draw);
	m_second->asciiArt(draw);
}

Complex Power::getNodeValue() const
{
	Complex a = m_first->getValue();
	Complex b = m_second->getValue();
	return pow(a, b);
}

Function::func_map Function::functions;

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

void Function::init_functions()
{
	functions.emplace("sin", &sinZ);
	functions.emplace("cos", &cosZ);
	functions.emplace("tan", &tanZ);
	functions.emplace("log", &logZ);
	functions.emplace("exp", &expZ);
}

void Function::calcSize() 
{
	m_arg->calcSize();
	setSize(m_name.length() + m_arg->getSizeX(), m_arg->getSizeY());
	setBaseLine(m_arg->getSizeY()/2);
}

void Function::calcOrig(int x, int y)
{
	setOrig(x, y);
	m_arg->calcOrig(x + m_name.length(), y);
}

void Function::asciiArt(Draw & draw) const
{
	draw.at(getOrigX(), getOrigY() + getBaseLine(), m_name, Draw::Color::GREEN);
	m_arg->asciiArt(draw);
}

Complex Function::getNodeValue() const
{
	Complex arg = m_arg->getValue();
	return m_func(arg);
}

Constant::const_map Constant::constants;

void Constant::init_constants()
{
	constants.emplace('e', Complex(exp(1.0), 0));
	constants.emplace('P', Complex(4*atan(1.0), 0));
	constants.emplace('i', Complex(0, 1));
}

void Constant::calcSize() 
{
	setSize(1, 1);
	setBaseLine(0);
}

void Constant::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Constant::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), m_name);
}

Variable::var_map Variable::values;

void Variable::calcSize() 
{
	setSize(1, 1);
	setBaseLine(0);
}

void Variable::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Variable::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), m_name);
}

void Variable::setValue(char name, Complex value)
{
	auto it = values.find(name);
	if (it != values.end() ) it->second = value;
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

void Number::calcSize() 
{
	string n = toString();
	m_imag_pos = n.find('i');
	setSize(n.length(), 1);
	setBaseLine(0);
}

void Number::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Number::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), toString());

	if (m_imag_pos != string::npos) 
		draw.at(getOrigX() + m_imag_pos, getOrigY(), 'i', Draw::Color::RED);
}

void Term::calcSize() 
{
	int x = 0, b = 0, y = 0;
	for ( auto n : factors ) { 
		n->calcSize(); 
		x += n->getSizeX();
		y = max(y, n->getSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	setSize(x, b + y);
	setBaseLine(b);
}

void Term::calcOrig(int x, int y)
{
	setOrig(x, y);
	for ( auto n : factors ) {
		n->calcOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getSizeX();
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

Node* Term::getLeftSibling(Node* node)
{
	for (int i = 1; i < factors.size(); ++i) {
		if (factors[i] == node)
			return factors[i - 1];
	}
	return nullptr;
}

Node* Term::getRightSibling(Node* node)
{
	for (int i = 0; i < factors.size() - 1; ++i) {
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

void Expression::calcSize() 
{
	int x = 0, b = 0, y = 0;
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	for ( auto n : terms ) { 
		if (n != terms[0] || !n->getSign()) x += 1;
		n->calcSize(); 
		x += n->getSizeX();
		y = max(y, n->getSizeY() - n->getBaseLine());
		b = max(b, n->getBaseLine());
	}
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	setSize(x, y + b);
	setBaseLine(b);
}

void Expression::calcOrig(int x, int y)
{
	setOrig(x, y);
	if (getParent() && getParent()->drawParenthesis()) x += 1;
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) x+=1;
		n->calcOrig(x, y + getBaseLine() - n->getBaseLine());
		x += n->getSizeX();
	}
}

void Expression::asciiArt(Draw& draw) const
{
	if (getParent() && getParent()->drawParenthesis()) draw.parenthesis(this);
	for ( auto n : terms ) {
		if (n != terms[0] || !n->getSign()) draw.at(n->getOrigX() - 1, 
													n->getOrigY() + n->getBaseLine(),
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

Complex Expression::getNodeValue() const
{
	Complex value = 0;
	for (auto n : terms) { value += n->getValue(); }
	return value;
}

Node* Expression::getLeftSibling(Node* node)
{
	for (int i = 1; i < terms.size(); ++i) {
		if (terms[i] == node)
			return terms[i - 1]->last();
	}
	return nullptr;
}

Node* Expression::getRightSibling(Node* node)
{
	for (int i = 0; i < terms.size() - 1; ++i) {
		if (terms[i] == node)
			return terms[i + 1]->first();
	}
	return nullptr;
}

int Input::input_sn = -1;

Input::Input(Equation& eqn, std::string txt, bool current, Node* parent, bool neg, Node::Select s) :
	Node(parent, neg, s), m_typed(txt), m_sn(++input_sn), m_active(true), m_current(current), m_eqn(eqn)
{
	eqn.addInput(this);
	if (current) eqn.setCurrentInput(m_sn);
}

void Input::calcSize()
{
	setSize(m_typed.length() + 1, 1);
	setBaseLine(0);	
}

void Input::calcOrig(int x, int y)
{
	setOrig(x, y);
}

void Input::asciiArt(Draw& draw) const
{
	draw.at(getOrigX(), getOrigY(), m_typed + "?");
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

Complex Input::getNodeValue() const
{
	throw logic_error("input has no value");
}

void Equation::eraseSelection(Node* node)
{
	if (m_selectStart == m_root) {
		delete m_root;
		if (node == nullptr) node = new Input(*this, string(), true);
		Term* term = new Term(node);
		Expression* expr = new Expression(term);
		term->setParent(expr);
		m_root = expr;
	}
	else if (m_selectStart == m_selectEnd) {
		auto it = erase(m_selectStart);
		insert(it, node);
	}
	else {
		auto it = FactorIterator(m_selectStart);
		for (it = erase(it); it != m_selectEnd; it = forward_erase(it));
		if (node == nullptr) erase(it);
		                else replace(it, node);
	}
	m_selectStart = m_selectEnd = nullptr;
}

bool Equation::handleChar(int ch)
{
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
		    case Key::SLEFT: {
				auto start = FactorIterator(m_selectStart);
				if (start.isBegin()) {
					fResult = false;
				}
				else {
					--start;
					m_selectStart = *start;
					
				}
			}
		    case Key::SRIGHT: {
				auto end = FactorIterator(m_selectEnd);
				if ((++end).isEnd()) {
					fResult = false;
				}
				else {
					++end;
					m_selectEnd = *end;
				}
			}
		    case Key::UP: {
				NodeIterator n{m_selectStart, *this};
				if (n != begin() ) {
					--n;
					setSelect(*n);
				}
				else
					fResult = false;
				break;
			}
		    case Key::DOWN: {
				NodeIterator n{m_selectStart, *this};
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
				
				Node* parent = m_selectStart->getParent();
				while (parent && !parent->isFactor()) { parent = parent->getParent(); }
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
					insert(in_pos, Expression::getTerm(*this, in->m_typed));
					in->m_typed.clear();
					if (ch == '-') in->getParent()->negative();
				}
				break;
			}
	        case '/': {
				bool fNeg = !in->getParent()->getSign();
				if (fNeg) in->getParent()->negative();

				if (!in->m_typed.empty()) in_pos = disableCurrentInput();
				Expression* upper = new Expression(in_pos.m_pTerm);
				in_pos.m_pTerm->setParent(upper);
				Expression* lower = new Expression(new Input(*this, string(), true));
				lower->setParent();
				
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
				b->setParent();
				
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
					delete in;
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
	        case ' ': {
				setSelect(m_root->first());
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
	m_input_index = (++m_input_index)%m_inputs.size();
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
	pos = erase(pos, false);
	delete in;
	return pos;
}

void Equation::removeInput(Input* in)
{
	int in_index =  distance(m_inputs.cbegin(), find(m_inputs, in));
	eraseElement(m_inputs, in_index);
	
	if (m_inputs.size() > 0 && in_index == m_input_index) {
		m_input_index = (++m_input_index)%m_inputs.size();
		m_inputs[m_input_index]->setCurrent(true);
	}
	else
		m_input_index = -1;
}

Equation* Equation::clone()
{
	string store;
	xml_out(store);
	istringstream in(store);
	return new Equation(in);
}

void Equation::getCursorOrig(int& x, int& y) 
{ 
	if (getCurrentInput() == nullptr) return;

	x = getCurrentInput()->getOrigX() + getCurrentInput()->getSizeX(); 
	y = getCurrentInput()->getOrigY();
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

	if (end == nullptr) {
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

Node* Equation::findNode(Draw& draw, int x, int y)
{
	draw.relativeOrig(x, y);
	Node* node = m_root->findNode(x, y);
	if (node != nullptr && typeid(node) == typeid(Input)) {
		delete node;
		return m_root->findNode(x, y);
	}
	else
		return node;
}

FactorIterator Equation::insert(FactorIterator it, Node* node)
{ 
	insertElement(it.m_pTerm->factors, it.m_factor_index, node);
	node->setParent(it.m_pTerm);
	it.m_node = node;
	return it;
}

FactorIterator Equation::insert(FactorIterator it, string text)
{
	NodeVector factors;
	factor(text, factors, it.m_pTerm);
	for ( auto f : factors ) { it = insert(it, f); ++it; }	
	return it;
}

FactorIterator Equation::insert(FactorIterator it, Term* term)
{
	insertElement(it.m_gpExpr->terms, it.m_term_index, term);
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
	if (--it.m_factor_index < 0) it.m_factor_index = 0;

	if (it.m_pTerm->factors.size() == 0) {
		if (it.m_gpExpr->terms.size() == 1) {
			it.m_gpExpr->terms[0]->factors.push_back(new Input(*this, string(), true));
			it.m_factor_index = 0;
		}
		else {
			if (free) delete it.m_gpExpr->terms[it.m_term_index];
			eraseElement(it.m_gpExpr->terms, it.m_term_index);
			if (--it.m_term_index < 0) it.m_term_index = 0;
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
	if (x >= m_xOrig && x < m_xOrig + m_xSize &&
		y >= m_yOrig && y < m_yOrig + m_ySize)
		return this;
	else
		return nullptr;
}

Node* Function::findNode(int x, int y)
{
	return m_arg->findNode(x, y);
}

Node* Binary::findNode(int x, int y)
{
	Node* node = m_first->findNode(x, y);
	if (node == nullptr) return m_second->findNode(x, y);
	return node;
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

Node* Expression::findNode(int x, int y)
{
	Node* node = nullptr;
	for ( auto n : terms ) {
		node = n->findNode(x, y);
		if (node != nullptr) break;
	}
	return node;
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

void FactorIterator::getNode(int factor, int term)
{
	m_term_index = term + (term < 0) ? m_gpExpr->terms.size() : 0;
	m_pTerm = m_gpExpr->terms[m_term_index];

	m_factor_index = factor + (factor < 0) ? m_pTerm->factors.size() : 0;
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

	if (--m_factor_index < 0) {
		--m_term_index;
	}
	if (m_term_index >= 0) {
		m_pTerm = m_gpExpr->terms[m_term_index];

		if (m_factor_index < 0) m_factor_index += m_pTerm->factors.size();
		m_node = m_pTerm->factors[m_factor_index];
	}
	else 
		throw range_error("out of range");		
}

bool FactorIterator::isEndTerm()
{ 
	if (m_gpExpr == nullptr) throw logic_error("null iterator");

	return m_term_index == m_pTerm->factors.size() - 1;
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
