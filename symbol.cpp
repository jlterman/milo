#include <iostream>
#include <vector>
#include <iterator>
#include <map>
#include <limits>
#include <regex>

#include "milo.h"
#include "nodes.h"

using namespace std;

static string skip_digits(const string& s) { return s.substr(s.find_first_not_of("+-0123456789")); }
static int get_digits(const string & s) 
{
	string n = s.substr(0, s.find_first_not_of("+-0123456789"));
	if (n.empty()) return 1;
	if (n.length() == 1 && !isdigit(n[0])) return (n[0] == '-' ? -1 : 1);
	return stoi(n);
}

const type_index Expression::type = typeid(Expression);
const type_index   Function::type = typeid(Function);
const type_index   Constant::type = typeid(Constant);
const type_index   Variable::type = typeid(Variable);
const type_index     Number::type = typeid(Number);
const type_index     Divide::type = typeid(Divide);
const type_index      Input::type = typeid(Input);
const type_index      Power::type = typeid(Power);
const type_index       Term::type = typeid(Term);


static const vector<type_index> factor_precedence = { 
	Number::type, Constant::type, Variable::type, Expression::type, 
	Function::type, Divide::type, Power::type, Input::type
};

static bool sort_terms(Term* a, Term* b)
{
	string a_str = skip_digits(a->toString());
	string b_str = skip_digits(b->toString());
	if (a_str == b_str) return (a->toString() < b->toString());
	return (a_str < b_str);
}

void Expression::normalize()
{
	for ( auto term : terms ) term->normalize();

	sort(terms.begin(), terms.end(), sort_terms);
}

bool Expression::simplify()
{
	bool result = false;
	for ( auto term : terms ) result |= term->simplify();

	int a_pos = 0;
	while ( a_pos < terms.size() - 1 ) {
		int b_pos = a_pos + 1;
		while ( b_pos < terms.size() ) {
			if (Term::simplify(terms, terms.begin() + a_pos, terms.begin() + b_pos)) {
				result = true;
			}
			else {
				a_pos = b_pos;
				break;
			}
		}
	}

	if (terms.size() == 1 && 
		this->getParent() != nullptr && 
		this->getParent()->getType() == Term::type) {
		Term* term = dynamic_cast<Term*>(this->getParent());
		term->simplify(this, terms.front());
		result = true;
	}

	return result;
}

void Expression::add(int n)
{
	auto num = new Number(abs(n), nullptr, (n < 0));
	auto term = new Term(num, this);
	terms.push_back(term);
}

void Expression::add(Expression* old_expr)
{
	for ( auto term : old_expr->terms ) terms.push_back(term);
	old_expr->terms.clear();
}

void Term::simplify(Node* ref, Term* new_term)
{
	auto pos = find(factors, ref);
	pos = factors.erase(pos);
	
	for ( auto factor : new_term->factors ) pos = factors.insert(pos, factor);
	new_term->factors.clear();
	delete ref;
}

static bool factor_cmp(Node* a, Node* b)
{
	if (a->getType() == b->getType()) return a->less(b);

	auto pos_a = find(factor_precedence, a->getType());
	auto pos_b = find(factor_precedence, b->getType());
	
	return (pos_a < pos_b);
}

void Term::normalize()
{
	for ( auto factor : factors ) factor->normalize();

	sort(factors.begin(), factors.end(), factor_cmp);
}

bool Term::simplify()
{
	bool result = false;
	
	for ( auto factor : factors ) { 
		factor->multNth(this->getNth());
		result |= factor->simplify();
	}
	this->setNth(1);

	int a_pos = 0;
	while ( a_pos < factors.size() - 1 ) {
		int b_pos = a_pos + 1;
		Node* a = factors.at(a_pos);
		while ( b_pos < factors.size() ) {
			Node* b = factors.at(b_pos);
			if (a->toString() == b->toString()) {
				a->addNth(b->getNth());
				delete b;
				factors.erase(factors.begin() + b_pos);
				result = true;
			}
			else {
				a_pos = b_pos - 1;
				break;
			}
		}
		++a_pos;
	}

	while (Power::simplify(factors)) { result = true; }
	return result;
}

bool Term::simplify(TermVector& terms, TermVector::iterator a, TermVector::iterator b)
{
	string a_str = (*a)->toString();
	string b_str = (*b)->toString();
	string a_base_str = skip_digits(a_str);
	string b_base_str = skip_digits(b_str);
	if (a_base_str != b_base_str) return false;

	int n = get_digits(a_str)*((*a)->getSign() ? 1 : -1) + 
		    get_digits(b_str)*((*b)->getSign() ? 1 : -1);
	if ((*a)->factors.front()->getType() == Number::type) {
		delete (*a)->factors.front();
		(*a)->factors.erase((*a)->factors.begin());
	}
	(*a)->factors.insert((*a)->factors.begin(), new Number(n, (*a)->getParent()));
	delete (*b);
	terms.erase(b);
	return true;
}

void Term::insertAfterMe(Node* me, Node* node)
{ 
	if (me->getParent()->getType() != Term::type) return;

	Term* term = dynamic_cast<Term*>(me->getParent());
	term->factors.insert(Term::pos(me) + 1, node);
}

void Power::normalize()
{
	m_first->normalize(); m_second->normalize();

	if (m_second->numFactors() != 1 || m_second->first()->getType() != Number::type) return;

	Complex z = m_second->getValue();
	if (isZero(z.imag()) && isInteger(z.real())) {
		m_first->multNth(m_second->getValue().real());
		*( Term::pos(this) ) = m_first;
		m_first = nullptr;
		delete this;
	}
}

bool Power::simplify()
{
	return m_first->simplify() | m_second->simplify();
}

bool Power::simplify(NodeVector& factors)
{
	for ( auto a = factors.begin(); a != factors.end(); ++a ) {
		for ( auto b = factors.begin(); b != factors.end(); ++b) {
			if (Power::simplify(a, b)) {
				delete (*b);
				factors.erase(b);
				return true;
			}
		}
	}
	return false;
}

bool Power::simplify(NodeVector::iterator a, NodeVector::iterator b)
{
	if ((*a)->getType() != Power::type || a == b) return false;

	Power* p_a = dynamic_cast<Power*>(*a);
	string base_a = p_a->m_first->toString();

	if ((*b)->getType() == Power::type) {
		Power* p_b = dynamic_cast<Power*>(*b);
		if (p_b->m_first->toString() == base_a) {
			p_a->getSecondExpression()->add(p_b->getSecondExpression());
			return true;
		}
	}
	else if (base_a == string("(" + (*b)->toString() + ")")) {
		p_a->getSecondExpression()->add((*b)->getNth());
		return true;
	}
	return false;
}

Expression* Binary::getFirstExpression()
{
	if (m_first->getType() != Expression::type) throw logic_error("expression expected");
	return dynamic_cast<Expression*>(m_first);
}

Expression* Binary::getSecondExpression()
{
	if (m_second->getType() != Expression::type) throw logic_error("expression expected");
	return dynamic_cast<Expression*>(m_second);
}

void Divide::normalize()
{
	m_first->normalize(); m_second->normalize();

	m_second->setNth(-m_second->getNth());

	Term::insertAfterMe(this, m_second);
	*( Term::pos(this) ) = m_first;
	m_first = nullptr;
	m_second = nullptr;
	delete this;
}

bool Divide::simplify()
{
	bool result = m_first->simplify() | m_second->simplify();
}
