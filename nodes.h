#ifndef __NODES_H
#define __NODES_H

/*
 * nodes.h
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

#include <vector>
#include <exception>
#include <map>
#include "milo.h"

class Parser;

bool isZero(double x);
bool isZero(Complex z);
bool isInteger(const std::string& n);
bool isInteger(double value);

using TermVector = std::vector<Term*>;

class Input : public Node
{
public:
    Input(Parser& p, Node* parent = nullptr);
    Input(Equation& eqn, std::string txt = std::string(), bool current = false, Node* parent = nullptr,
		  bool neg = false, Node::Select s = Node::Select::NONE);
	Input(EqnXMLParser& in, Node* parent);
	~Input() {}

	std::string toString() const;
	void xml_out(XML::Stream& xml) const;
	int numFactors() const { return m_typed.size(); }
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }

	static Input* parse(Parser& p, Node* parent = nullptr);

	static const std::string name;
	static const std::type_index type;

	friend class Equation;
private:
	int m_sn;
	std::string m_typed;
	bool m_current;
	Equation& m_eqn;
	Box m_internal;

	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const;

	void setCurrent(bool current) { m_current = current; }

	static int input_sn;
};

class Binary : public Node
{
public:
	Binary(char op, Node* one, Node* two, Node* parent, bool neg, Node::Select s) : 
	    Node(parent, neg, s), m_op(op), m_first(one), m_second(two) {}
	Binary(EqnXMLParser& in, Node* parent);
	virtual ~Binary() { delete m_first; delete m_second; }

	std::string toString() const { return m_first->toString() + std::string(1, m_op) + m_second->toString(); }
	bool isLeaf() const { return false; }
	Node* findNode(int x, int y);
	int numFactors() const { return m_first->numFactors() + m_second->numFactors(); }
	Expression* getFirstExpression();
	Expression* getSecondExpression();

	static Node* parse(Parser& p, Node* one, Node* parent);
	void xml_out(XML::Stream& xml) const;

protected:
	char m_op;
	Node* m_first;  // Binary own this tree
	Node* m_second; // Binary own this tree
	Box m_internal;

private:
	Node* downLeft()  { return m_first; }
	Node* downRight() { return m_second; }
	Node* getLeftSibling(Node* node)  { return (m_second == node) ? m_first->last() : nullptr; }
	Node* getRightSibling(Node* node) { return (m_first == node) ? m_second->first() : nullptr; }

};

class Divide : public Binary
{
public:
    Divide(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('/', one, two, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this); 
		m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}
    Divide(EqnXMLParser& in, Node* parent) : Binary(in, parent)
	{ 
		m_op = '/'; m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}
	~Divide() {}

	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const;
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	void normalize();
	bool simplify();

	static const std::string name;
	static const std::type_index type;
};

class Power : public Binary
{
public:
    Power(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('^', one, two, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this);
		m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}
    Power(EqnXMLParser& in, Node* parent) : Binary(in, parent)
	{ 
		m_op = '^'; m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}
	~Power() {}

	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const;
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	void normalize();
	bool simplify();
 
	static bool simplify(NodeVector& factors);

	static const std::string name;
	static const std::type_index type;

private:
	static bool simplify(NodeVector::iterator a, NodeVector::iterator b);
};

class Constant : public Node
{
public:
	Constant(Parser& p, Node* parent);
    Constant(char name, Complex value, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name), m_value(value) {}
	Constant(EqnXMLParser& in, Node* parent);
	~Constant() {}

	using const_map = std::map<char, Complex>;

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return constants.at(m_name); }
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }

	static const std::string name;
	static const std::type_index type;

	static Constant* parse(Parser& p, Node* parent);
private:
	char m_name;
	Complex m_value;
	Box m_internal;
	static const const_map constants;
};

class Variable : public Node
{
public:
	Variable(Parser& p, Node* parent);
    Variable(char name, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name)
    { 
		values.emplace(name, Complex(0, 0)); 
	}
	Variable(EqnXMLParser& in, Node* parent);
	~Variable() {}

	using var_map = std::map<char, Complex>;

	std::string toString() const { return std::string(1, m_name); }
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return values[m_name]; }
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }

	static const std::string name;
	static const std::type_index type;

	static Variable* parse(Parser& p, Node* parent);
	static void setValue(char name, Complex value);

private:
	char m_name;
	Box m_internal;

	static var_map values;
};

class Number : public Node
{
public:
    Number(Parser& p, Node* parent) : Node(parent), m_isInteger(true) { getNumber(p); }
	Number(std::string real, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(stod(real)), m_isInteger(isInteger(real)) {}
	Number(int n, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(n), m_isInteger(true) {}
	Number(double d, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(d), m_isInteger(isInteger(d)) {}
	Number(EqnXMLParser& in, Node* parent);
	~Number() {}

	void getNumber(Parser& p);

	std::string toString() const;
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return {m_value, 0}; }
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	bool simplify();

	static const std::string name;
	static const std::type_index type;

	static Number* parse(Parser& p, Node* parent);

private:
	std::string getInteger(Parser& p);

	double m_value;
	bool m_isInteger;
	Box m_internal;
};

class Term : public Node
{
public:
    Term(Parser& p, Expression* parent = nullptr) : Node((Node*) parent) { while(add(p)); }
	Term(EqnXMLParser& in, Node* parent = nullptr);
    Term(NodeVector& f, Expression* parent) : Node((Node*) parent) { factors.swap(f); }
    Term(Node* node, Expression* parent = nullptr, bool fNeg = false) : 
	    Node((Node*) parent, fNeg), factors(1, node) 
	{
		node->setParent(this);
	}
	~Term() { freeVector(factors); }
	
	static Node* parse(Parser& p, Node* parent = nullptr);

	std::string toString() const;
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	bool isLeaf() const { return false; }
	bool isFactor() const { return false; }
	Complex getNodeValue() const;
	Node* findNode(int x, int y);
	int numFactors() const;
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	void normalize();
	bool simplify();
	void simplify(Node* ref, Term* new_term);

	static void insertAfterMe(Node* me, Node* node);
	static bool simplify(TermVector& terms, TermVector::iterator a, TermVector::iterator b);

	static const std::string name;
	static const std::type_index type;

	int getFactorIndex(Node* node) { return distance(factors.cbegin(), find(factors, node)); }
	void setParent() { for ( auto f : factors ) f->setParent(this); }
	void setParent(Expression* parent) { Node::setParent((Node*) parent); }

	NodeIter begin() { return factors.begin(); }
	NodeIter end()   { return factors.end(); }
	static NodeIter pos(Node* me);

	void multiply(double n);
	void multiply(Term* old_term);

	friend class Equation;
	friend class FactorIterator;
private:
	Node* downLeft()  { return factors.front(); }
	Node* downRight() { return factors.back(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

	NodeVector factors; // Term owns this tree
	Box m_internal;

	bool add(Parser& p);
};

class Expression : public Node
{
public:
	Expression(EqnXMLParser& in, Node* parent = nullptr);

    Expression(Parser& p, Node* parent = nullptr) : Node(parent)
	{
		while(add(p)); setDrawParenthesis(true);
	}

    Expression(Term* term, Node* parent = nullptr) : Node(parent), terms(1, term)
	{ 
		term->setParent(this); setDrawParenthesis(true);
	}

    Expression(Node* factor, Node* parent = nullptr) : 
	    Node(parent), terms(1, new Term(factor, this))
	{ 
		factor->setParent(terms[0]); setDrawParenthesis(true);
	}
	~Expression() { freeVector(terms); }

	std::string toString() const;
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	bool isLeaf() const { return false; }
	Complex getNodeValue() const;
	Node* findNode(int x, int y);
	int numFactors() const;
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	void normalize();
	bool simplify();

	static const std::string name;
	static const std::type_index type;

	int numTerms() const { return terms.size(); }
	int getTermIndex(Term* term) { return distance(terms.cbegin(), find(terms, term)); }
	void setParent() { for ( auto t : terms ) t->setParent(this); }

	TermVector::iterator begin() { return terms.begin(); }
	TermVector::iterator end()   { return terms.end(); }

	static Term* getTerm(Equation& eqn, std::string text, Expression* parent = nullptr);
	static Term* getTerm(Parser& p, Expression* parent);
	static Expression* parse(Parser& p, Node* parent);

	void add(double n);
	void add(Expression* old_expr);

	friend class Equation;
	friend class FactorIterator;

private:
	Node* downLeft()  { return terms.front(); } 
	Node* downRight() { return terms.back(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

	TermVector terms;	// Expression owns this tree
	Box m_internal;

	bool add(Parser& p);
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = std::map<std::string, func_ptr>;

	static Function* parse(Parser& p, Node* parent);

	std::string toString() const { return m_name + m_arg->toString(); }
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	bool isLeaf() const { return false; }
	Complex getNodeValue() const;
	Node* findNode(int x, int y);
	int numFactors() const { return m_arg->numFactors() + 1; }
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	void normalize() { m_arg->normalize(); }
	bool less(Node* b) const { 
		auto bf = dynamic_cast<Function*>(b); 
		if ( m_name == bf->m_name ) return toString() < bf->toString(); 
		return m_name > bf-> m_name;
	}

	static const std::string name;
	static const std::type_index type;

    Function(const std::string& name, func_ptr fp, Parser& p, Node* parent, 
			 bool neg = false, Node::Select s = Node::Select::NONE) : 
	             Node(parent, neg, s), m_name(name), m_func(fp), m_arg(new Expression(p, this)) {}

	Function(EqnXMLParser& in, Node* parent);
	~Function() { delete m_arg; }

private:
	Node* downLeft()  { return m_arg->first(); }
	Node* downRight() { return m_arg->last(); }

	std::string m_name;
	func_ptr m_func;
	Node* m_arg;       // Function own this tree
	Box m_internal;

	static const func_map functions;

	static Complex sinZ(Complex z);
	static Complex cosZ(Complex z);
	static Complex tanZ(Complex z);
	static Complex logZ(Complex z);
	static Complex expZ(Complex z);
};


class Differential : public Node
{
public:
    Differential(Parser& p, Node* parent);
	Differential(EqnXMLParser& in, Node* parent);
	~Differential() {}

	bool isLeaf() const { return false; }
	int numFactors() const { return m_function->numFactors(); }
	Node* findNode(int x, int y) { return m_function->findNode(x, y); }
	std::string toString() const;
	void xml_out(XML::Stream& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return {0, 0}; }
	const std::string& getName() const { return name; }
	std::type_index getType() const { return type; }
	void normalize() { m_function->normalize(); }
	bool simplify() { return false; }

	static const std::string name;
	static const std::type_index type;

	static Differential* parse(Parser& p, Node* parent);

private:
	Node* downLeft()  { return m_function->first(); }
	Node* downRight() { return m_function->last(); }

	Box m_internal;
	char m_variable;
	Expression* m_function;
};

#endif // __NODES_H
