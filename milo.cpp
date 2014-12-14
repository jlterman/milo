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
		int x0 = m_selectStart->getOrigX();
		int y0 = m_selectStart->getOrigY();
		int x = 0, y = 0;

		Node* node = nullptr;
		do {
			node = (node == nullptr) ? m_selectStart : node->getNextRight();
			x += m_selectStart->getSizeX();
			y = max(y, m_selectStart->getSizeY());
		}
		while (node != nullptr && node != m_selectEnd);
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

Node* Node::begin()
{
	Node* node = this;
	Node* down = this->downLeft();
	while (down != nullptr) {
		node = down;
		down = node->downLeft();
	}
	return (down != nullptr) ? down : node;
}

Node* Node::end()
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
	Node* left = m_parent->getLeftSibling(this);
	if (left != nullptr) { 
		return left->end();
	}
	else if (m_parent != nullptr) {
		return m_parent->getNextLeft();
	}
	else
		return nullptr;
}

Node* Node::getNextRight()
{
	Node* right = m_parent->getRightSibling(this);
	if (right != nullptr) { 
		return right->begin();
	}
	else if (m_parent != nullptr) {
		return m_parent->getNextRight();
	}
	else
		return nullptr;
}

Term* Node::getParentTerm() 
{ 
	return dynamic_cast<Term*>(m_parent);
}

Expression* Term::getParentExpression() 
{ 
	return dynamic_cast<Expression*>(getParent());
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
	Complex a = m_first->getNodeValue();
	Complex b = m_second->getNodeValue();
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
	Complex a = m_first->getNodeValue();
	Complex b = m_second->getNodeValue();
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
	Complex arg = m_arg->getNodeValue();
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
	for (int i = 0; i < factors.size() - 2; ++i) {
		if (factors[i] == node) 
			return factors[i + 1];
	}
	return nullptr;
}

Complex Term::getNodeValue() const
{
	Complex value = 1;
	for (auto n : factors) { value *= n->getNodeValue(); }
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
	for (auto n : terms) { value += n->getNodeValue(); }
	return value;
}

Node* Expression::getLeftSibling(Node* node)
{
	for (int i = 1; i < terms.size(); ++i) {
		if (terms[i] == node)
			return terms[i - 1];
	}
	return nullptr;
}

Node* Expression::getRightSibling(Node* node)
{
	for (int i = 0; i < terms.size() - 2; ++i) {
		if (terms[i] == node) 
			return terms[i + 1];
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
	Term* sel_term = m_selectStart->getParentTerm();
	if (m_selectStart->getParentTerm() == nullptr) {
		delete m_root;
		Term* term = new Term(node);
		Expression* expr = new Expression(term);
		term->setParent(expr);
		m_root = expr;
	}
	else if (sel_term != nullptr) {
		int fact_index = sel_term->getFactorIndex(m_selectStart);
		while (sel_term->factors[fact_index] != m_selectEnd) {
			delete sel_term->factors[fact_index];
			sel_term->factors.erase(sel_term->factors.begin() + fact_index);
		}
		delete sel_term->factors[fact_index];
		
		if (node != nullptr) {
			node->setParent(sel_term);
			sel_term->factors[fact_index] = node;
		}
		else
			sel_term->factors.erase(sel_term->factors.begin() + fact_index);
	}
	m_selectStart = m_selectEnd = nullptr;
}

bool Equation::handleChar(int ch)
{
	bool fResult = true;
	if (getCurrentInput() == nullptr) {
		if (m_selectStart != nullptr && (isalnum(ch) || ch == '.')) {
			eraseSelection(new Input(*this, string(1, (char)ch), true));
			return true;
		}
		switch(ch) {
		    case Key::BACKSPACE: {
				eraseSelection(new Input(*this, string(), true));
				break;
			}
		    case 10:
		    case 13: {
				clearSelect();
				Term* sel_term = m_selectStart->getParentTerm();
				int fact_index = sel_term->getFactorIndex(m_selectStart);
				sel_term->factors.insert(sel_term->factors.begin() + fact_index + 1, 
										 new Input(*this, string(), true, sel_term));
				break;
			}
	        case ' ': {
		        if (m_selectStart != nullptr) {
					if (m_selectStart->getParent() == nullptr) return false;
					
					Node* parent = m_selectStart->getParent();
					while (parent && !parent->isFactor()) { parent = parent->getParent(); }
					if (parent == nullptr) return false;
					
					setSelect(parent);
					m_input_index = -1;
					LOG_TRACE_MSG("current selection: " + m_selectStart->toString());
				}
				else {
					setSelect(m_root->begin());
				}
				break;
			}
            default:
			   fResult = false;
			   break;
		}
	}
	else if (m_input_index >= 0) {
		if (isalnum(ch) || ch == '.') {
			getCurrentInput()->m_typed += string(1, (char)ch);
			return true;
		}
		Input* in = getCurrentInput();
		Term* in_term = in->getParentTerm();
		int fact_index = in_term->getFactorIndex(in);
		Expression* in_expr = in_term->getParentExpression();
		int term_index = in_expr->getTermIndex(in_term);

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
				fResult = handleBackspace();
			    break;
			}
	        case '+':
	        case '-': {
				if (in->m_typed.empty()) {
					Term* term = dynamic_cast<Term*>(Expression::getTerm(*this, string(1, (char)ch)+"#", in_expr));
					in_expr->terms.insert(in_expr->terms.begin() + term_index + 1, term);
				}
				else {
					Term* term = dynamic_cast<Term*>(Expression::getTerm(*this, in->m_typed, in_expr));
					in_expr->terms.insert(in_expr->terms.begin() + term_index, term);
					in->m_typed.clear();
					if (ch == '-') in->getParent()->negative();
				}
				break;
			}
	        case '/': {
				bool fNeg = false;
				if (!in->m_typed.empty()) {
					in_term->factors.erase(in_term->factors.begin() + fact_index);
					NodeVector factors;
					factor(in->m_typed, factors, in_term); in->m_typed.clear();
					for ( auto f : factors ) {
						in_term->factors.insert(in_term->factors.begin() + fact_index++, f);
					}
				}
				else {
					in_term->factors[fact_index] = new Input(*this, string(), false, in_term);
				}
				fNeg = !in_term->getSign(); if (fNeg) in_term->negative();
				Expression* new_upper_expr = new Expression(in_term);
				in_term->setParent(new_upper_expr);
				
				Term* new_lower_term = new Term(in);
				in->setParent(new_lower_term);
				Expression* new_lower_expr = new Expression(new_lower_term);
				new_lower_term->setParent(new_lower_expr);
				
				Divide* d = new Divide(new_upper_expr, new_lower_expr, in_term);
				Term* divide_term = new Term(d, in_expr, fNeg);
				in_expr->terms[term_index] = divide_term;
				break;
			}
	        case '^': {
				Node* node = nullptr;
				in_term->factors.erase(in_term->factors.begin() + fact_index);
				if (!in->m_typed.empty()) {
					NodeVector factors;
					factor(in->m_typed, factors, in_term); in->m_typed.clear();
					node = factors.front();
					for (auto it = factors.begin() + 1; it != factors.end(); ++it) {
						in_term->factors.insert(in_term->factors.begin() + fact_index++, node);
						node = *it;
					}
				}
				else {
					node = new Input(*this, string(), false, in_term);
				}
				Term* a_term = new Term(node);
				node->setParent(a_term);
				Expression* a_expr = new Expression(a_term);
				a_term->setParent(a_expr);
				
				Term* b_term = new Term(in);
				node->setParent(b_term);
				Expression* b_expr = new Expression(b_term);
				a_term->setParent(b_expr);
				
				Power* p = new Power(a_expr, b_expr, in_term);
				in_term->factors.insert(in_term->factors.begin() + fact_index, p);
				break;
			}
	        case '(': {
				in->m_typed += "(#)";

				disableCurrentInput();
				break;
			}
	        case ' ': {
				if (getCurrentInput()->m_typed.empty()) {
					fResult = false;
				}
				else {
					Input* in = m_inputs[m_input_index];
					Term* in_term = in->getParentTerm();
					int fact_index = in_term->getFactorIndex(in) + in->m_typed.length() - 1;
					disableCurrentInput();

					setSelect(in_term->factors[fact_index]);
				}
				break;
			}
	        default:  
				fResult = false;
				break;
		}
	}
	else
		fResult = false;

	return fResult;
}

void Equation::nextInput() 
{ 
	m_inputs[m_input_index]->setCurrent(false);
	m_input_index = (++m_input_index)%m_inputs.size();
	m_inputs[m_input_index]->setCurrent(true);
}

void Equation::disableCurrentInput()
{
	if (m_input_index < 0) throw logic_error("no current input to disable");

	Input* in = m_inputs[m_input_index];
	disableInput(in);
}

void Equation::disableInput(Input* in)
{
	Term* in_term = in->getParentTerm();
	int fact_index = in_term->getFactorIndex(in);

	in_term->factors.erase(in_term->factors.begin() + fact_index);
	if (!in->m_typed.empty()) {
		NodeVector factors;
		factor(in->m_typed, factors, in_term);
		for ( auto f : factors ) {
			in_term->factors.insert(in_term->factors.begin() + fact_index++, f);
		}
	}
	m_inputs.erase(m_inputs.begin() + m_input_index);
	if (m_inputs.size() > 0) {
		m_input_index = (++m_input_index)%m_inputs.size();
		m_inputs[m_input_index]->setCurrent(true);
	}
	else
		m_input_index = -1;
	delete in;
}

bool Equation::handleBackspace()
{
	bool fResult = true;
	if (m_input_index >= 0) {
		Input* in = getCurrentInput();
		if (!in->m_typed.empty()) {
			in->m_typed.erase(in->m_typed.end() - 1);
		}
		else {
			Term* in_term = in->getParentTerm();
			int fact_index = in_term->getFactorIndex(in);
			Expression* in_expr = in_term->getParentExpression();
			int term_index = in_expr->getTermIndex(in_term);

			if (fact_index > 0 && in_term->factors[fact_index-1]->isLeaf()) {
				delete in_term->factors[fact_index-1];
				in_term->factors.erase(in_term->factors.begin() + fact_index - 1);
			}
			else if (fact_index > 0) {
				in->disable();
				setSelect(in_term->factors[fact_index-1]);
				m_input_index = -1;
				LOG_TRACE_MSG("current selection: " + m_selectStart->toString());
			}
			else if (term_index > 0) {
				Term* prev_term = dynamic_cast<Term*>(in_expr->terms[term_index - 1]);
				Term* term = dynamic_cast<Term*>(in_expr->terms[term_index]);
				
				int prev_size = prev_term->factors.size();
				prev_term->factors.resize( prev_size + term->factors.size(), nullptr );
				move(term->factors.begin(), term->factors.end(), prev_term->factors.begin() + prev_size);
				term->factors.clear();
				in_expr->terms.erase(in_expr->terms.begin() + term_index);
			}
			else
				fResult = false;
		}
	}
	return fResult;
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
		disableInput(dynamic_cast<Input*>(node));
		return m_root->findNode(x, y);
	}
	else
		return node;
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
