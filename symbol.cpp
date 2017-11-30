/*
 * symbol.cpp
 * This file is part of milo
 *
 * Copyright (C) 2015 - James Terman
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

static bool isNumber(const string& s) { return s.find_first_not_of("+-0123456789") == string::npos; }

const type_index Differential::type = typeid(Differential);
const type_index   Expression::type = typeid(Expression);
const type_index     Function::type = typeid(Function);
const type_index     Constant::type = typeid(Constant);
const type_index     Variable::type = typeid(Variable);
const type_index       Number::type = typeid(Number);
const type_index       Divide::type = typeid(Divide);
const type_index        Input::type = typeid(Input);
const type_index        Power::type = typeid(Power);
const type_index         Term::type = typeid(Term);


static const vector<type_index> factor_precedence = { 
	Number::type, Constant::type, Variable::type, Expression::type, 
	Function::type, Divide::type, Power::type, Differential::type, Input::type
};

static bool sort_terms(TermPtr a, TermPtr b)
{
	if ( !isNumber(a->toString()) && !isNumber(b->toString()) ) {
		string a_str = skip_digits(a->toString());
		string b_str = skip_digits(b->toString());
		if (a_str == b_str) return (a->toString() < b->toString());
		return (a_str < b_str);
	}
	else if ( isNumber(a->toString()) && isNumber(b->toString()) ) {
		return (a->toString() < b->toString());
	}
	else {
		return (a->toString() > b->toString());
	}
}

bool Function::less(NodePtr b) const
{ 
	auto bf = dynamic_pointer_cast<Function>(b);
	if (!bf) throw logic_error("Illegal call of less()");
	if ( m_name == bf->m_name ) return toString() < bf->toString(); 
	return m_name > bf-> m_name;
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

	if ( terms.back()->numFactors() == 1 && isNumber(terms.back()->toString()) ) {
		double v = 0;
		while ( !terms.empty() && terms.back()->numFactors() == 1 && isNumber(terms.back()->toString()) ) {
			v += terms.back()->getValue().real();
			terms.erase(terms.end() - 1);
		}
		add(v);
	}

	unsigned int a_pos = 0;
	while ( a_pos < terms.size() - 1 ) {
		unsigned int b_pos = a_pos + 1;
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

	return result;
}

void Expression::add(double n)
{
	if ( n == 0 ) return;
	auto num = new Number(abs(n), nullptr);
	auto term = new Term(num, this, (n < 0));
	terms.push_back(term);
}

void Expression::add(ExpressionPtr old_expr)
{
	terms.merge(old_expr->terms);
}

void Term::multiply(double n)
{
	if ( n == 1 ) return;
	auto num = new Number(abs(n), nullptr, (n < 0));
	factors.insert(factors.begin(), num);
}

void Term::multiply(TermPtr old_term)
{
	factors.merge(old_term->factors, factors.begin());
}

void Term::simplify(NodePtr ref, TermPtr new_term)
{
	auto pos = factors.find(ref);
	pos = factors.erase(pos);
	
	for ( auto factor : new_term->factors ) pos = factors.insert(pos, factor);
	new_term->factors.clear();
}

static bool factor_cmp(NodePtr a, NodePtr b)
{
	if (a->getType() == b->getType()) return a->less(b);

	auto pos_a = find(factor_precedence, a->getType());
	auto pos_b = find(factor_precedence, b->getType());
	
	return (pos_a < pos_b);
}

void Term::normalize()
{
	for ( auto factor : factors ) factor->normalize();

	auto pos = factors.begin(); 
	while ( pos != factors.end() ) {
		if ( (*pos)->getNth() == 0 ) { 
			pos = factors.erase(pos);
			if ( factors.empty() ) {
				factors.push_back(new Number(1, this));
				break;
			}
			continue;
		}
		if ( (*pos)->getType() != Expression::type ) { ++pos; continue; }

		auto expr = dynamic_pointer_cast<Expression>(*pos);
		if (expr->numTerms() == 1) {
			TermPtr term = *(expr->begin());
			for ( auto t : term->factors ) {
				t->multNth(expr->getNth());
				pos = factors.insert(pos, t) + 1;
			}
			term->factors.clear();
			pos = factors.erase(pos);
			if ( term->getSign() != expr->getSign() ) negative();
		}
		else
			++pos;
	}

	sort(factors.begin(), factors.end(), factor_cmp);

	bool zero = false;
	bool sign = true;
	for ( auto factor : factors ) {
		zero |= (factor->getType() == Number::type && factor->getValue().real() == 0);
		if (!factor->getSign()) {
			sign = !sign;
			factor->negative();
		}
	}
	if (zero) {
		factors.clear();
		factors.push_back(new Number(0, this));
	} else if (!sign) {
		negative();
	}
}

bool Term::simplify()
{
	bool result = false;
	
	for ( auto factor : factors ) { 
		factor->multNth(this->getNth());
		result |= factor->simplify();
	}
	this->setNth(1);

	if ( factors.front()->getType() == Number::type && factors.size() > 1 ) {
		double v = 1.0;
		while ( !factors.empty() && factors.front()->getType() == Number::type ) {
			Node* factor = factors.front();
			v *= factor->getValue().real();
			factors.erase(factors.begin());
		}
		multiply(v);
	}

	unsigned int a_pos = 0;
	while ( a_pos < factors.size() - 1 ) {
		unsigned int b_pos = a_pos + 1;
		Node* a = factors.at(a_pos);
		while ( b_pos < factors.size() ) {
			Node* b = factors.at(b_pos);
			if (a->toString() == b->toString()) {
				a->addNth(b->getNth());
				factors.erase_index(b_pos);
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
	if (isNumber(a_str) || isNumber(b_str)) return false;

	string a_base_str = skip_digits(a_str);
	string b_base_str = skip_digits(b_str);
	if (a_base_str != b_base_str) return false;

	int n = get_digits(a_str)*((*a)->getSign() ? 1 : -1) + 
		    get_digits(b_str)*((*b)->getSign() ? 1 : -1);
	if ((*a)->factors.front()->getType() == Number::type) {
		(*a)->factors.erase((*a)->factors.begin());
	}
	(*a)->factors.insert((*a)->factors.begin(), new Number(n, (*a)->getParent()));
	terms.erase(b);
	return true;
}

void Term::insertAfterMe(Node* me, NodePtr node)
{ 
	if (me->getParent()->getType() != Term::type) return;

	Term* term = dynamic_cast<Term*>(me->getParent());
	term->factors.insert(Term::pos(me) + 1, node);
}

void Power::normalize()
{
	m_first->normalize();

	if (getNth() != 1) {
		Term* term = new Term(m_second);
		term->multiply(getNth());
		setNth(1);
		m_second = new Expression(term, this);
	}
	m_second->normalize();

	if (m_second->numFactors() == 1 && m_second->first()->getType() == Number::type) 
	{
		double n = m_second->getValue().real();
		if (isInteger(n))
		{
			m_first->multNth(n);
			*( Term::pos(this) ) = m_first;
		}
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

	auto p_a = dynamic_pointer_cast<Power>(*a);
	string base_a = p_a->m_first->toString();

	if ((*b)->getType() == Power::type) {
		auto p_b = dynamic_pointer_cast<Power>(*b);
		if (p_b->m_first->toString() == base_a) {
			p_a->getSecondExpression()->add(p_b->getSecondExpression());
			return true;
		}
	}
	else if ( (base_a == (*b)->toString()) || (base_a == string("(+" + (*b)->toString() + ")")) ) {
		p_a->getSecondExpression()->add((*b)->getNth());
		return true;
	}
	return false;
}

ExpressionPtr Binary::getFirstExpression()
{
	if (m_first->getType() != Expression::type) throw logic_error("expression expected");
	return ExpressionPtr(dynamic_pointer_cast<Expression>(m_first));
}

ExpressionPtr Binary::getSecondExpression()
{
	if (m_second->getType() != Expression::type) throw logic_error("expression expected");
	return ExpressionPtr(dynamic_pointer_cast<Expression>(m_second));
}

Node* Divide::normalize(Node* n)
{
	Divide* d = dynamic_cast<Divide*>(n);
	NodeVector factors = { d->m_first, d->m_second };
	d->m_second->multNth(-1);
	return new Expression(new Term(factors, nullptr), n->getParent());
}

void Divide::normalize()
{
	m_first->normalize();
	if (m_first->getType() == Divide::type) {
		m_first = normalize(m_first);
	}

	m_second->normalize();
	if (m_second->getType() == Divide::type) {
		m_second = normalize(m_second);
	}

	if (getParent()->getType() == Term::type) {
		m_second->multNth(-1);
		Term::insertAfterMe(this, m_second);
		*( Term::pos(this) ) = m_first;
	}
	else if (getParent()->getType() == Divide::type) {
		return; // Normalize happens from parent
	}
	else
		throw logic_error("can't handle " + getParent()->getName() + " as parent");
}

bool Divide::simplify()
{
	return m_first->simplify() | m_second->simplify();
}

bool Number::simplify()
{
	if (getNth() == 1) return false;

	if (getNth() == 0) {
		m_value = 1;
		m_isInteger = true;
	}
	else {
		double v = m_value;
		for (int i = 1; i < getNth(); ++i) v*=m_value;
		m_value = v;
	}
	setNth(1);
	return true;
}
