#ifndef __MILO_H
#define __MILO_H

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
 * @file milo.h
 * This file contains the common declerations for the equations module part of milo.
 * The Equation class is declared, plus the supporting abstract base class Node 
 * which is the base class for every node in the equation tree. An abstract base class
 * for the graphics context is also declared.
 */

#include <string>
#include <memory>
#include <complex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include <typeindex>

#include "util.h"
#include "xml.h"
#include "smart.h"

// Forward class declerations
namespace UI { class Graphics; }
class Node;
class Term;
class Expression;
class Input;
class Equation;

// Hidden class declerations for pointers and reference
class Parser;
namespace XML { class Parser; }

/** @name Global Type Declerations  */
//@{
using Complex = std::complex<double>;     ///< Specialized as complex double.
using NodePtr = SmartPtr<Node>;           ///< Shared pointer for Node class and derived classes.
using NodeVector = SmartVector<Node>;     ///< Storage of node pointers.
using NodeIter = NodeVector::iterator;    ///< Iterator into vector of node pointers.
using TermPtr = SmartPtr<Term>;           ///< Shared pointer for Term class
using TermVector = SmartVector<Term>;     ///< Specialized smart vector of terms.
using ExpressionPtr = SmartPtr<Expression>; ///< Shared pointer for expression class.
using EqnPtr = SmartPtr<Equation>;        ///< Shared pointer for equation
//@}

/**
 * Abstract base class for symbolic classes that make up an equation.
 * Any class that will be part of the equation tree structor derives from this class.
 */
class Node : public std::enable_shared_from_this<Node>
{
public:
	/**
	 * Possible selection states for a node:
	 * 
	 * NONE - not selected
	 * 
	 * START - node starts selection
	 * 
	 * END - node ends selection
	 * 
	 * ALL- just this node selected
	 */
	enum Select { NONE, START, END, ALL };

	/**
	 * List of strings corresponding to enum Select.
	 * Use enum State as index into select_tags to get its strings.
	 */
	static const std::vector<std::string> select_tags;

	/**
	 * Size and vertical offset of a graphic for a node.
	 * This classholds the box that contains the node graphic,
	 * plus its base for vertical alignment.
	 */
	struct Frame { Box box; int base; };

	/** @name Constructors and Virtual Destructor */
	//@{
	/**
	 * Constructor for Node class.
	 * Directly initialize the Node class private data members.
	 * @param eqn Equation associated with this node.
	 * @param parent Node object that is parent of this node. If NULL, this node is root.
	 * @param fNeg True, if node is negated.
	 * @param s Node selection state.
	 */
    Node(Equation& eqn, Node* parent, bool fNeg = false, Select s = NONE ) : 
	    m_eqn(eqn), m_parent(parent), m_sign(!fNeg), m_select(s) {}

	/**
	 * Constructor for Node class.
	 * Directly initialize the Node class private data members from parser
	 * @param parser Parser to create this node from.
	 * @param parent Node object that is parent of this node. If NULL, this node is root.
	 * @param fNeg True, if node is negated.
	 * @param s Node selection state.
	 */
	Node(Parser& p, Node* parent, bool fNeg = false, Select s = NONE );

	/**
	 * XML constructor for Node class.
	 * Initialize the constructor from XML input stream.
	 * @param in XML input stream.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent node object.
	 */
	Node(XML::Parser& in, Equation& eqn, Node* parent);
	
	virtual ~Node() {} ///< Abstract base class needs virtual desctructor.
	//@}

	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Recursive virtual function to represent this subtree as string.
	 * @return String representing this node and child nodes.
	 */
	virtual std::string toString() const=0;

	/**
	 * Virtual function to query whether this is a leaf node or subtree.
	 * Default function returns true. Override to return false.
	 * @return True if there are no child nodes.
	 */
	virtual bool isLeaf() const { return true; }

	/**
	 * Virtual function to query whether this node is a factor.
	 * Default function returns true. Only class Term returns false.
	 * @return True if a factor.
	 */
	virtual bool isFactor() const { return true; }

	/**
	 * Virtual recursive function to return deepest node at point (x,y).
	 * Leaf nodes will return true or false depending on whether (x,y)
	 * is inside their box. Non-leaf nodes will query their children.
	 * @param x horizontal coordinate.
	 * @param y vertical coordinate.
	 * @return Deepest node at point (x,y).
	 */
	virtual Node* findNode(int x, int y);

	/**
	 * Virtual function that queries node of number of factor in its subtree.
	 * Default function returns 1 which is correct for leaf node. Non-leaf 
	 * node queries its children.
	 * @return Number of factors in node's subtree.
	 */
	virtual int numFactors() const { return 1; }

	/**
	 * Put node's subtree to a standard algebraic form.
	 */
	virtual void normalize() {}

	/**
	 * Simplify node's subtree algebraiclly.
	 * @return True if node's subtree was changed.
	 */
	virtual bool simplify() { return false; }

	/**
	 * Compare this node string representation to the given node's string representation.
	 * Default function works for most nodes but some are overridden.
	 * @param b Node to compare.
	 @ @return True if this node is less than given node. 
	 */
	virtual bool less(NodePtr b) const { return this->toString() < b->toString(); }

	/**
	 * Get final class name of node class object.
	 * @return Class name of node object.
	 */
	virtual const std::string& getName() const=0;

	/**
	 * Get final class type of node class object.
	 * @return Class type of node object.
	 */
	virtual std::type_index getType() const=0;
	//@}

	/**
	 * Calculate size of node subtree in given graphics context.
	 * @param gc Graphics context used to calculate size.
	 */
	void calculateSize(UI::Graphics& gc);

	/**
	 * Serialize this node and child nodes to XML output stream.
	 * @param xml XML ouput stream.
	 * @return XML stream object.
	 */
	XML::Stream& out(XML::Stream& xml);

	/**
	 * Calculate origin of each node in subtree.
	 * The size of each node in subtree needs to be precalculated.
	 * @param gc Graphics context to calculate origin.
	 * @param x  Horizontal origin of root node in subtree.
	 * @param y  Vertical origin of root node in subtree.
	 */
	void calculateOrigin(UI::Graphics& gc, int x, int y);

	/**
	 * Calculate the size and origin of the subtree of this node.
	 * @param gc Graphics context class object.
	 */
	void setUpDraw(UI::Graphics& gc);

	/**
	 * Draw equation subtree whose root is this node.
	 * @param gc Graphics context class object.
	 */
	void draw(UI::Graphics& gc) const;

	/**
	 * Set selection state of this node.
	 * @param s New selection state of node.
	 */
	void setSelect(Select s) { m_select = s; }

	/**
	 * Get selection state of this node.
	 * @return Selection state of this node.
	 */
	Select getSelect() const { return m_select; }

	/**
	 * Set draw parenthesis flag of this node.
	 * @param fDrawPar If true, draw a parenthesises around node.
	 */
	void setDrawParenthesis(bool fDrawPar) { m_fDrawParenthesis = fDrawPar; }

	/**
	 * Get draw parenthesis flag of this node.
	 * @return Draw parenthesis flag value.
	 */
	bool getDrawParenthesis() const { return m_fDrawParenthesis; }

	/**
	 * Get frame of this node.
	 * @return Frame of this node.
	 */
	const Frame& getFrame() const { return m_frame; }

	/**
	 * Get left most node of this node's subtree.
	 * @return Left mode node.
	 */
	Node* first();

	/**
	 * Get right most node of this node's subtree.
	 * @return Right mode node.
	 */
	Node* last();

	/**
	 * Get next left node, going up if necessary.
	 * @return Next left node.
	 */
	Node* getNextLeft();

	/**
	 * Get next right node, going up if necessary.
	 * @return Next right node.
	 */
	Node* getNextRight();

	/**
	 * Set parent of this node.
	 */
	void setParent(Node* parent) { m_parent = parent; }

	/**
	 * Get parent of this node.
	 * @return Parent node.
	 */
	Node* getParent() const { return m_parent; }

	/**
	 * Get depth from root of this node. 0 is root.
	 * @return Depth of this node.
	 */
	int getDepth() const { return getDepth(0); }
	/**
	 * Get value of integer power of this node.
	 * @return Value of m_nth.
	 */
	int getNth() const { return m_nth; }

	/**
	 * Add integer n to power value of this node.
	 * @param n Integet power to be added.
	 */
	void addNth(int n) { m_nth += n; }

	/**
	 * Multiply integer n to power value of this node.
	 * @param n Integer power to be multiplied.
	 */
	void multNth(int n) { m_nth *= n; }

	/**
	 * Set integer power value of this node.
	 * @param n Integer power to be set.
	 */
	void setNth(int n) { m_nth = n; }

	/**
	 * Negate this node.
	 */
	void negative() { m_sign = !m_sign; }

	/**
	 * Get sign of this node.
	 * @return True if positive.
	 */
	bool getSign() const { return m_sign; }

	/**
	 * Calculate value of this node and it subtree.
	 * @return Calculated value of this node's subtree.
	 */
	Complex getValue() const;

	/**
	 * Create node by its name in the given equation
	 * @param name Name of node to be created.
	 * @param eqn Equation that has current input to take node
	 * @return True, if node was created
	 */
	static bool createNodeByName(std::string name, Equation& eqn);

	/**
	 * Return shared_ptr for this Node object.
	 * If none exists, create one.
	 * @return Shared pointer for this Node object
	 */
	std::shared_ptr<Node> getSharedPtr()
	{
		std::shared_ptr<Node> sp;
		try {
			sp = this->shared_from_this();
		}
		catch (std::exception& ex) {
			sp = std::shared_ptr<Node>(this);
		}
		return sp;
	}

	friend class FactorIterator;
protected:
	Equation& m_eqn;   ///< Equation object associated with this node.

private:
	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Get left most node of this node's subtree.
	 * Default function returns NULL assuming it is leaf node.
	 * @return Left most node of this node's subtree.
	 */
	virtual Node* downLeft() { return nullptr; }

	/**
	 * Get right most node of this node's subtree.
	 * Default function returns NULL assuming it is leaf node.
	 * @return Righ most node of this node's subtree.
	 */
	virtual Node* downRight() { return nullptr; }

	/**
	 * Get left sibiling of node argument.
	 * Default function returns NULL assuming this is a leaf node.
	 * @return Left sibling node.
	 */
	virtual Node* getLeftSibling(Node*) { return nullptr; }

	/**
	 * Get right sibiling of node argument.
	 * Default function returns NULL assuming this is a leaf node.
	 * @return Right sibling node.
	 */
	virtual Node* getRightSibling(Node*) { return nullptr; }

	/**
	 * Calculate frame of this node's subtree in graphics context.
	 * Frame includes size and vertical offset.
	 * @param gc Graphics context.
	 * @return Return frame of this subtree.
	 */
	virtual Frame calcSize(UI::Graphics& gc)=0;

	/**
	 * Calculate origin of the nodes in the subtree relative to point x,y.
	 * @param gc Graphics context.
	 * @param x  Horizontal origin.
	 * @param y  Vertical origin.
	 */
	virtual void calcOrig(UI::Graphics& gc, int x, int y)=0;

	/**
	 * Draw subtree in this graphics context.
	 * @param gc Graphics context.
	 */
	virtual void drawNode(UI::Graphics& gc) const=0;

	/**
	 * Get value of this node's subtree.
	 * @return Subtree value.
	 */
	virtual Complex getNodeValue() const=0;

	/**
	 * Stream subtree to XML stream.
	 * @param xml XML stream.
	 */
	virtual void xml_out(XML::Stream& xml) const=0;
	//@}

	/**
	 * Recursively calculate the depth of this node.
	 * @param depth Current calculated depth.
	 * @return Return depth of this node.
	 */
	int getDepth(int depth) const {
		return (!m_parent) ? depth : m_parent->getDepth(++depth);
	}

	Node* m_parent;    ///< Parent node of this node. Can be null if root.
	bool m_sign;       ///< True if positve.
	Select m_select;   ///< Select state of this node.
	Frame m_frame;     ///< Frame of this node includes enclosing rect and vertical offset.
	Box m_parenthesis; ///< Rectangle that contains paranthesis of this node.
	int m_nth = 1;     ///< Integer power of ths node.
	bool m_fDrawParenthesis = false; ///< If true, draw paranthesis around this node.
};

/**
 * Represents a term as a vector of factors which are multiplied together.
 */
class Term : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Parser constructor for Term class.
	 * @param p Parser object.
	 * @param parent Parent node.
	 */
    Term(Parser& p, Expression* parent) : Node(p, (Node*) parent) { while(add(p)); }

	/**
	 * XML constructor for Term class.
	 * Read in XML for Term class.
	 * @param in XML input stream.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent node object.	 
	 */
	Term(XML::Parser& in, Equation& eqn, Node* parent);

	/**
	 * Constructor for Term class loading in factors from a node vector.
	 * @param f Factors to be loaded into new Term object.
	 * @param parent Parent expresion.
	 */
    Term(NodeVector& f, Equation& eqn, Expression* parent) : Node(eqn, (Node*) parent) { factors.swap(f); }
	
 	/**
	 * Constructor for Term class.
	 * @param node Initialize node vector with this node.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent expresion.
	 * @param fNeg If true, node is negative.
	 */
    Term(Node* node, Equation& eqn, Expression* parent, bool fNeg = false) : 
	    Node(eqn, (Node*) parent, fNeg), factors(node) 
	{
		node->setParent(this);
	}
 
	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Term() {}
	//@}
	
	/**
	 * Static helper function to parse Term class.
	 * @param p Parser object pointing to operator.
	 * @param parent Parent node.
	 * @return Number object or null if number not present.
	 */
	static Node* parse(Parser& p, Node* parent = nullptr);

	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Get string representation of Term class.
	 * The factors are concatenated into one string.
	 * @return String representation of Term class.
	 */
	std::string toString() const;

	/**
	 * Override for isLeaf virtual function.
	 * @return False, this node has child nodes.
	 */
	bool isLeaf() const { return false; }

	/**
	 * Override for isFactor virtual function.
	 * @return False, this node is not a factor.
	 */
	bool isFactor() const { return false; }

	/**
	 * Rescursive search to find deepest node at point (x,y).
	 * @param x horizontal coordinate.
	 * @param y vertical coordinate.
	 * @return Deepest node at point (x,y).	 
	 */
	Node* findNode(int x, int y);

	/**
	 * Get number of factor in this subtree.
	 * @return Return total number of factors in term.
	 */
	int numFactors() const;

	/**
	 * Refactor subtree to a standard form.
	 * For Term class, reorder factors in a standard order
	 * and combine common factors.
	 * 2cb(3a)^4 normalized to 162bc(a^4)
	 */
	void normalize();

	/**
	 * Attempt to algebraically simplify the subtree this node is the root.
	 * @return True, if subtree was changed.
	 */
	bool simplify();

	/**
	 * Get name of this class.
	 * @return Name of this class.
	 */
	const std::string& getName() const { return name; }

	/**
	 * Get name of this class.
	 * @return Name of this class.
	 */
	std::type_index getType() const { return type; }
	//@}
	
	static const std::string name;     ///< Name of Term class.
	static const std::type_index type; ///< Type of Term class.

	/**
	 * Helper function to take factors from a term and insert into this term.
	 * @param ref Node in this term where new factors will be inserted.
	 * @param new_term Term object with factors to be transferred.
	 */
	void simplify(NodePtr ref, TermPtr new_term);

	/**
	 * Compare every term to every other term to see if can combined.
	 * If terms are the same excpet for a number, they are combined.
	 * @param terms Reference to term vector.
	 * @param a Iterator to first term to be compared.
	 * @param b Iterator to second term to be compared. If terms are combined this is deleted.
	 * @return True if terms were combined.
	 */
	static bool simplify(TermVector& terms, TermVector::iterator a, TermVector::iterator b);

	/**
	 * Insert new node after a reference node in node vector.
	 * @param me Reference node.
	 * @param node New node.
	 */
	static void insertAfterMe(Node* me, NodePtr node);

	/**
	 * Set all factor's parent to this Term.
	 */
	void setParent() { for ( auto f : factors ) f->setParent(this); }

	/**
	 * Reset parent expression of this Term.
	 * @param parent New parent expression.
	 */
	void setParent(Expression* parent) { Node::setParent((Node*) parent); }

	/**
	 * Get iterator to first factor of Term.
	 * @return Iterator of first factor.
	 */
	NodeIter begin() { return factors.begin(); }

	/**
	 * Get iterator to end of Term.
	 * @return Last factor iterator.
	 */
	NodeIter end()   { return factors.end(); }

	/**
	 * Helper static function to get iterator to given node.
	 * @param me Pointer to a factor.
	 * @return Iterator to node.
	 */
	static NodeIter pos(Node* me);

	/**
	 * Multiply this term by a number by inserting Number object.
	 * @param n Value to multiply term by.
	 */
	void multiply(double n);

	/**
	 * Mulitiply this by term by another term.
	 * @param old_term Move factors into this Term object.
	 */
	void multiply(TermPtr old_term);

	/**
	 * Return shared_ptr for this Term object.
	 * Downcast it from base Node shared pointer.
	 * @return Shared pointer for this Term object
	 */
	std::shared_ptr<Term> getSharedPtr()
	{
		return std::dynamic_pointer_cast<Term>(this->Node::getSharedPtr());
	}

	friend class FactorIterator;
private:
	NodeVector factors; ///< Term owns this tree
	Box m_internal;     ///< Bounding box of this node.

	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Get left node.
	 * For Term class is its first factor.
	 * @return Left node.
	 */
	Node* downLeft()  { return factors.front(); }

	/**
	 * Get right node.
	 * For Term class is its last factor.
	 * @return Right node.
	 */
	Node* downRight() { return factors.back(); }

	/**
	 * Left sibiling of given node.
	 * @param node Reference node.
	 * @return Left sibiling or null.
	 */
	Node* getLeftSibling(Node* node);

	/**
	 * Right sibiling of given node.
	 * @param node Reference node.
	 * @return Right sibiling or null.
	 */

	Node* getRightSibling(Node* node);

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const;

	/**
	 * Output XML of this node.
	 * @param xml XML output stream.
	 */
	void xml_out(XML::Stream& xml) const;
	
	/**
	 * Calculate size of this node in given graphics context.
	 * @param gc Graphics context.
	 * @return Return frame containing size of this node.
	 */
	Frame calcSize(UI::Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(UI::Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(UI::Graphics& gc) const;
	//@}
	
	/**
	 * Add factors to this Term object from parser.
	 * @return True if factors were added.
	 */
	bool add(Parser& p);
};

/**
 * Represents an expression as a vector of terms which are added together.
 * Subtract happens by setting nodes to a negative sign.
 * A vector of (ac,-b,2,i,3) is equivalent to ac-b+2+i3.
 */
class Expression : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	 /**
	  * XML constructor for Expression class.
	  * Read in XML for Expression class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
      * @param eqn Equation associated with this node.
	  */
	Expression(XML::Parser& in, Equation& eqn, Node* parent);

	/**
	 * Parser constructor for Expression class.
	 * @param p Parser object.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 */
    Expression(Parser& p, Node* parent = nullptr) : Node(p, parent)
	{
		while(add(p));
		setDrawParenthesis(true);
	}

 	/**
	 * Constructor for Expression class.
	 * @param term Initialize term vector with this term.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent expresion.
	 */
    Expression(Term* term, Equation& eqn, Node* parent = nullptr) : Node(eqn, parent), terms(term)
	{ 
		term->setParent(this); setDrawParenthesis(true);
	}

 	/**
	 * Constructor for Expression class.
	 * @param factor Initialize term vector with new term containing node.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent expresion.
	 */
    Expression(Node* factor, Equation& eqn, Node* parent = nullptr) : 
	    Node(eqn, parent), terms(new Term(factor, eqn, this))
	{ 
		factor->setParent(terms[0]); setDrawParenthesis(true);
	}

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Expression() {}
	//@}
	
	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Get representation of expression as string.
	 * String are terms separated by '+' or '-'.
	 * @return String representation of expression.
	 */
	std::string toString() const;

	/**
	 * Override for isLeaf virtual function.
	 * @return False, this node has child nodes.
	 */
	bool isLeaf() const { return false; }

	/**
	 * Rescursive search to find deepest node at point (x,y).
	 * @param x horizontal coordinate.
	 * @param y vertical coordinate.
	 * @return Deepest node at point (x,y).	 
	 */
	Node* findNode(int x, int y);

	/**
	 * Get number of factors in this node.
	 * @return Number of factors.
	 */
	int numFactors() const;

	/**
	 * Refactor subtree to a standard form.
	 * For Expression class, reorder terms in alphabetical order
	 * and combine common terms.
	 * ab+3ba normalized to 4ab.
	 */
	void normalize();

	/**
	 * Attempt to algebraically simplify the subtree this node is the root.
	 * @return True, if subtree was changed.
	 */
	bool simplify();

	/**
	 * Get name of this class.
	 * @return Name of this class.
	 */
	const std::string& getName() const { return name; }

	/**
	 * Get name of this class.
	 * @return Name of this class.
	 */
	std::type_index getType() const { return type; }
	//@}
	
	static const std::string name;     ///< Name of this class.
	static const std::type_index type; ///< Type of this class.

	/**
	 * Get number of terms.
	 * @return Number of terms.
	 */
	int numTerms() const { return terms.size(); }

	/**
	 * Set parent of all terms to this expression.
	 */
	void setParent() { for ( auto t : terms ) t->setParent(this); }

	/**
	 * Get iterator to first term of Expression.
	 * @return Iterator of first term.
	 */
	TermVector::iterator begin() { return terms.begin(); }

	/**
	 * Get iterator to end of Expression.
	 * @return Last term iterator.
	 */
	TermVector::iterator end()   { return terms.end(); }

	/**
	 * Helper static function to parse term from string value.
	 * @param eqn Equation containg parent expression.
	 * @param text String value to be parsed.
	 * @param parent Parent expression.
	 * @return Created Term object.
	 */
	static Term* getTerm(Equation& eqn, const std::string& text, Expression* parent = nullptr);

	/**
	 * Helper static function to parse term.
	 * @param p Parser object.
	 * @param parent Parent expression.
	 * @return Created Term object.
	 */
	static Term* getTerm(Parser& p, Expression* parent);

	/**
	 * Helper static function to parse Expression.
	 * @param p Parser object.
	 * @param parent Parent expression.
	 * @return Created Expression object.
	 */
	static Node* parse(Parser& p, Node* parent);

	/**
	 * Add a Number to expression.
	 * @param n Numeric value to be added.
	 * @return Expression + n.
	 */
	void add(double n);

	/**
	 * Add terms from Expression object.
	 * @param old_expr Expression object.
	 */
	void add(ExpressionPtr old_expr);

	friend class FactorIterator;
private:
	TermVector terms; ///< Expression owns this tree a list of terms.
	Box m_internal;   ///< Bounding box of this node.

	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Get left node.
	 * For Expression class is its first term.
	 * @return Left node.
	 */
	Node* downLeft()  { return terms.front(); } 

	/**
	 * Get right node.
	 * For Expression class is its last term.
	 * @return Right node.
	 */
	Node* downRight() { return terms.back(); }

	/**
	 * Left sibiling of given node.
	 * @param node Reference node.
	 * @return Left sibiling or null.
	 */
	Node* getLeftSibling(Node* node);

	/**
	 * Right sibiling of given node.
	 * @param node Reference node.
	 * @return Right sibiling or null.
	 */
	Node* getRightSibling(Node* node);
	/**
	 * Output XML of this node.
	 * @param xml XML output stream.
	 */
	void xml_out(XML::Stream& xml) const;
	
	/**
	 * Calculate size of this node in given graphics context.
	 * @param gc Graphics context.
	 * @return Return frame containing size of this node.
	 */
	Frame calcSize(UI::Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(UI::Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(UI::Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const;
	//@}
	
	/**
	 * Add terms to expression from parser.
	 * @param p Parser object.
	 * @return True, if term was added.
	 */
	bool add(Parser& p);
};

/**
 * Iterate over every node in an equation foward and backward.
 * The precedence goes from deepest left most node to deepest right mode node.
 */
class NodeIterator : public std::iterator< std::bidirectional_iterator_tag, Node* >
{
private:
	Node* m_node; ///< Current node pointed to be iterator.

	void next();  ///< Advance this iterator next node.
	void prev();  ///< Iterator goes back previous node.

public:
	/**
	 * Constructor for NodeIterator class.
	 * Initialize the current node this iterator points to.
	 * @param node New current node.
	 */
	NodeIterator(Node* node) : m_node(node) {}

	/** @name Overloaded Operators */
	//@{
	/**
	 * Overloaded dereference operator.
	 * @return Current node.
	 */
	Node*& operator*() { return m_node; }

	/**
	 * Overloaded member function operator.
	 * Returns pointer to current node to access member functions.
	 * @return Current node member functions.
	 */
	Node*& operator->() { return m_node; }

	/**
	 * Overloaded equal operator.
	 * @param rhs Node to be compared.
	 * @return True if node is equal to rhs node.
	 */
	bool operator==(const NodeIterator& rhs) { return m_node == rhs.m_node; }

	/**
	 * Overloaded not equal operator.
	 * @param rhs Node to be compared.
	 * @return True if node is not equal to rhs node.
	 */
	bool operator!=(const NodeIterator& rhs) { return m_node != rhs.m_node; }

	/**
	 * Overloaded postfix increment operator.
	 * @return NodeIterator object pointing to next factor.
	 */
	NodeIterator& operator++()   { next(); return *this; }

	/**
	 * Overloaded prefix increment operator.
	 * This NodeIterator object will be decremented.
	 * Return an object with old iterator value.
	 * @return Copy of NodeIterator pointing to old iterator value.
	 */
	NodeIterator operator++(int) { NodeIterator tmp(*this); next(); return tmp; }

	/**
	 * Overloaded postfix decrement operator.
	 * @return NodeIterator object pointing to previous factor.
	 */
	NodeIterator& operator--()   { prev(); return *this; }

	/**
	 * Overloaded prefix decrement operator.
	 * This NodeIterator object will be decremented.
	 * Return an object with old iterator value.
	 * @return Copy of NodeIterator pointing to old iterator value.
	 */
	NodeIterator operator--(int) { NodeIterator tmp(*this); prev(); return tmp; }
	//@}
};

/**
 * Iteratorate over every factor in an expression.
 * The precedence is the first factor in the first termn to the last factor in the last term.
 */
class FactorIterator : public std::iterator< std::bidirectional_iterator_tag, Node* >
{
 private:
	Node* m_node;                  ///< Current factor.
	Term* m_pTerm;                 ///< Parent term of current factor.
	Expression* m_gpExpr;          ///< Grandparent expresson of current factor.
	unsigned int m_factor_index;   ///< Index of current factor in parent term.
	unsigned int m_term_index;     ///< Term index of current factor in grandparent expression.

	void next();                   ///< Get next factor in expression.
	void prev();                   ///< Get previous factor in expression.

public:
	/** @name Constructors */
	//@{
	/**
	 * Constructor for FactorIterator class.
	 * Initialize the current node this iterator points to.
	 * @param node New current node.
	 */
    FactorIterator(Node* node);

	/**
	 * Constructor for FactorIterator class from NodeIterator.
	 * Initialize the current node this iterator points to from NodeIterator.
	 * @param n New current node.
	 */
    FactorIterator(NodeIterator n) : FactorIterator(*n) {}

	/**
	 * Constructor for FactorIterator class from Expression object.
	 * Initialize the current node this iterator points to from first factor in Expression.
	 * @param expr New cucrrent node from first factor of expr.
	 */
    FactorIterator(Expression* expr);
	//@}
	
	/** @name Overloaded Operators */
	//@{
	/**
	 * Overloaded dereference operator.
	 * @return Current node.
	 */
	Node*& operator*() { return m_node; }

	/**
	 * Overloaded member function operator.
	 * Returns pointer to current node to access member functions.
	 * @return Current node member functions.
	 */
	Node*& operator->() { return m_node; }

	/**
	 * Overloaded equal operator.
	 * @param rhs Node to be compared.
	 * @return True if node is equal to rhs node.
	 */
	bool operator==(const FactorIterator& rhs) { return m_node == rhs.m_node; }

	/**
	 * Overloaded not equal operator.
	 * @param rhs Node to be compared.
	 * @return True if node is not equal to rhs node.
	 */
	bool operator!=(const FactorIterator& rhs) { return m_node != rhs.m_node; }

	/** 
	 * Overloaded greater operator.
	 * @param rhs Node to be compared
	 * @return True if rhs if after this node in same expression.
	 */
	bool operator>(const FactorIterator& rhs);

	/** 
	 * Overloaded greater or equal operator.
	 * @param rhs Node to be compared
	 * @return True if rhs if after this node in same expression.
	 */
	bool operator>=(const FactorIterator& rhs);

	/** 
	 * Overloaded lesser operator.
	 * @param rhs Node to be compared
	 * @return True if rhs if after this node in same expression.
	 */
	bool operator<(const FactorIterator& rhs);

	/** 
	 * Overloaded lesser or equal operator.
	 * @param rhs Node to be compared
	 * @return True if rhs if after this node in same expression.
	 */
	bool operator<=(const FactorIterator& rhs);
	
	/**
	 * Overloaded postfix increment operator.
	 * @return NodeIterator object pointing to next factor.
	 */
	FactorIterator& operator++()   { next(); return *this; }

	/**
	 * Overloaded prefix increment operator.
	 * This NodeIterator object will be decremented.
	 * Return an object with old iterator value.
	 * @return Copy of NodeIterator pointing to old iterator value.
	 */
	FactorIterator operator++(int) { FactorIterator tmp(*this); next(); return tmp; }

	/**
	 * Overloaded postfix decrement operator.
	 * @return NodeIterator object pointing to previous factor.
	 */
	FactorIterator& operator--()   { prev(); return *this; }

	/**
	 * Overloaded prefix decrement operator.
	 * This NodeIterator object will be decremented.
	 * Return an object with old iterator value.
	 * @return Copy of NodeIterator pointing to old iterator value.
	 */
	FactorIterator operator--(int) { FactorIterator tmp(*this); prev(); return tmp; }
	//@}

	/** @name Helper Member Functions */
	//@{	
	/**
	 * Erase factor pointed to by iterator. 
	 * Erase current node leaving iterator pointing to next factor.
	 */
	void erase();

	/**
	 * Erase all nodes to given iterator.
	 * Starting with this iterators node to node pointed to by given iterator is erased.
	 * Iterator will be left pointing to next iterator.
	 * @param end Final node to be erased.
	 */
	void erase(const FactorIterator& end);

	/**
	 * Merge term pointed to by this iterator with next term if it exists.
	 */
	void mergeNextTerm();

	/**
	 * Split current term at factor pointed to by this iterator.
	 * @param fNeg If true, term is negative.
	 * @return New term split from old one.
	 */
	Term* splitTerm(bool fNeg = false);
	
	/**
	 * Replace the parent term of factor pointed to by iterator with new term.
	 * @param term Term to replace factor's parent term pointed to by iterator.
	 */
	void replace(Term* term);
	
	/**
	 * Replace factor pointed to by iterator with node.
	 * @param node Node to replace factor pointed to by iterator.
	 */
	void replace(Node* node);

	/**
	 * Insert node before factor pointed to by this iterator.
	 * After insertion this iterator will point to new node. 
	 * @param node Factor to be inserted.
	 */
	void insert(Node* node);

	/**
	 * Insert node after factor pointed to by this iterator.
	 * After insertion this iterator will point to new node. 
	 * If iterator is end, exception thrown.
	 * @param node Factor to be inserted.
	 */
	void insertAfter(Node* node);

	/**
	 * Insert term before the parent term of the current factor.
	 * @param node Term to be inserted.
	 * @param sign Sign of term.
	 */
	void insert(Term* node, bool sign = true);

	/**
	 * Insert term after the parent term of the current factor.
	 * @param term Term to be inserted.
	 * @param sign Sign of term.
	 */
	void insertAfter(Term* term, bool sign = true);

	/**
	 * Check if current factor is first in its grandparent expresion.
	 * @return Return true if factor is first in expression.
	 */
	bool isBegin() { return m_factor_index == 0 && m_term_index == 0; }

	/**
	 * Check if current factor is last in its grandparent expresion.
	 * @return Return true if factor is last in expression.
	 */
	bool isEnd()   { return !m_node; }

	/**
	 * Check if current factor is first in its parent term.
	 * @return Return true if factor is first in term.
	 */
	bool isBeginTerm() { return m_factor_index == 0; }

	/**
	 * Check if current factor is last in its parent term.
	 * @return Return true if factor is last in term.
	 */
	bool isEndTerm();

	/**
	 * Point iterator to a specific factor in a specific term.
	 * @param factor Index of factor in term.
	 * @param term   Index of term in expression.
	 */
	void setNode(int factor, int term);

	/**
	 * Get iterator pointing to first factor in the expression of this FactorIterator.
	 * @return New iterator pointing to first factor.
	 */
	FactorIterator getBegin() { FactorIterator tmp(*this); tmp.setNode(0, 0); return tmp; }

	/**
	 * Get iterator pointing to last factor in the expression of this FactorIterator.
	 * @return New iterator pointing to last factor.
	 */
	FactorIterator getLast() { FactorIterator tmp(*this); tmp.setNode(-1, -1); return tmp; }
	//@}

	static void swap(FactorIterator& a, FactorIterator& b);
};

/**
 * Holds the node tree that represents a mathematical equation. It also
 * provides an interface for manipulating the nodes to algebraically 
 * transform the equation and to save, load and draw an equation.
 */
class Equation
{
public:
	/** @name Constructors and Virtual Destructor */
	//@{
	/**
	 * Constructor to load an equation represented by string such as 'a+b/c'.
	 * @param eq String containing equation to be created.
	 */
	Equation(const std::string& eq);

	/**
	 * Constructor to load equation from xml
	 * <equation><expression>...</expression></equation>
	 * @param in XML::Parser object
	 */
	Equation(XML::Parser& in) { xml_in(in); }
	
	/**
	 * Constructor to load an equation from xml.
	 * @param is Input stream containing xml.
	 */
    Equation(std::istream& is);

	/**
	 * Copy constructor based on overloaded equal operator.
	 * @param eqn Equation class object to be copied
	 */
	Equation(const Equation& eqn) { *this = eqn; }

	/**
	 * Destructor deletes node tree to clean up after itself.
	 */
	~Equation() {}
	//@}
	
	/** @name Overloaded Equal Operators */
	//@{
	/**
	 * Overloaded equal operator that copies node tree from another object.
	 * Given equation object is serialized to xml and then deserialized into this object.
	 * @param eqn Equation class object to be copied.
	 */
	Equation& operator=(const Equation& eqn);

	Equation(Equation&& eqn)=delete;            ///< No move copy constructor.
	Equation& operator=(Equation&& eqn)=delete; ///< No move equal operator.
	//@}
	
	/**
	 * Get string representation of equation.
	 * Example:  '(a+b/c)'.
	 * @return String representation of equation.
	 */
	std::string toString() const { return m_root->toString(); }

	/**
	 * Serialize this equation's root and child nodes to XML output stream.
	 * @param xml XML ouput stream.
	 * @return XML stream object.
	 */
	XML::Stream& out(XML::Stream& xml) { xml_out(xml); return xml; }

	/**
	 * Serialize equation as xml to output stream.
	 * @param os Output stream.
	 */
	void xml_out(std::ostream& os) const;

	/**
	 * Serialize equation as xml to string.
	 * @param str Output string object.
	 */
	void xml_out(std::string& str) const;

	/**
	 * Draw equation in given graphic context.
	 * @param gc Grahics context.
	 */
	void draw(UI::Graphics& gc);

	/**
	 * Get equation object that is copy of this equation object.
	 * @return Cloned equation object.
	 */
	Equation* clone();

	/**
	 * Check where there is an active input in this equation object.
	 * @return If true, there is an active input.
	 */
	bool blink() { return m_input_index >= 0; }

	/**
	 * Get coordinates of current active input.
	 * @param[out] x Horizontal origin of curosr.
	 * @param[out] y Vertical origin cursor.
	 */
	void getCursorOrig(int& x, int& y);

	/**
	 * Set current active input node.
	 * @param in_sn Serial number of new active input.
	 */
	void setCurrentInput(int in_sn);

	/**
	 * Add new input node to this equation's list of inputs.
	 * @param in New input node.
	 */
	void addInput(Input* in) { m_inputs.push_back(in); }

	/**
	 * Remove input node from equation's list of input nodes.
	 * @param in Input node to be removed.
	 */
	void removeInput(Input* in);

	/**
	 * Get current input node.
	 * @return Current input node.
	 */
	Input* getCurrentInput() { 
		if (m_inputs.empty() || m_input_index < 0)
			return nullptr;
		else
			return m_inputs[m_input_index];
	}

	/**
	 * Get next input in list after current input node.
	 */
	void nextInput(bool fShift);

	/**
	 * Remove current input node.
	 */
	FactorIterator disableCurrentInput();

	/**
	 * Set start of selection.
	 * @return start Node to be start of selection.
	 */
	void setSelectStart(Node* start) { m_selectStart = start; }

	/**
	 * Set end of selection.
	 * @return end Node to be end of selection.
	 */
	void setSelectEnd(Node* end)   { m_selectEnd = end; }

	void clearSelect(); ///< Clear current selection

	/**
	 * Set start and end of selection.
	 * If end parameter is null, then only start node is selected.
	 * @param start Node that starts selection.
	 * @param end   Node that ends selection.
	 */
	void setSelect(Node* start, Node* end = nullptr);

	/**
	 * Draw current selection in graphics context.
	 * @param gc Graphics context.
	 */
	void setSelect(UI::Graphics& gc);

	/**
	 * Either select single node or if Input node make current input.
	 * @param node Node to be selected or input to be activated.
	 */
	void selectNodeOrInput(Node* node);
	
	/**
	 * Set selection of this equation from selection state of node.
	 * If node has selection set to start, set as start of selection.
	 * If node has selection set to end, set as end of selection.
	 * If node has selection set to all, set as start and end of selection.
	 * return node Node whose selection state is referenced.
	 */
	void setSelectFromNode(Node* node);

	/**
	 * Query root node for deepest node at coordinates x, y in given graphics context.
	 * Call findNode method of root node. If input node found, delete it and search again.
	 * @param x  Horizontal coordinate.
	 * @param y  Vertical coordinate.
	 * @return Node found at x,y coordinates. Null if none found.
	 */
	Node* findNode(int x, int y) { return m_root->findNode(x, y); }

	/**
	 * Select nodes inside bounding box inside same expression.
	 * rectangle b.
	 * @param b Bounding box.
	 */
	void selectBox(Box b);

	/**
	 * Erase current selection and replace with new node.
	 * If node parameter is null, selection will just be erased.
	 * @param node Node to replace current selection.
	 */
	void eraseSelection(Node* node);

	/**
	 * Simplify equation algebracially.
	 * Call normalize() method of root node and then simplifly().
	 * @return Return result of m_root->simplify().
	 */
	bool simplify() { m_root->normalize(); return m_root->simplify(); }

	/**
	 * Normalize equation to standard format.
	 */
	void normalize() { m_root->normalize(); }

	/**
	 * Get left most node of equation.
	 */
	NodeIterator begin() { return NodeIterator(m_root->first()); }

	/**
	 * Get end iterator of equation.
	 */
	NodeIterator end()   { return NodeIterator(nullptr); }

	/**
	 * Get right most node of equation.
	 */
	NodeIterator last()  { return NodeIterator(m_root->last()); }

	/**
	 * Use iterator to insert factors from parsed string.
	 * @param[in,out] it Iterator to position to insert factors. 
                         On exit points to factor after last factor inserted.
	 * @param text String to get parsed factors.
	 */
	void insert(FactorIterator& it, const std::string& text);

	Node* getRoot() { return m_root; }

	Node* getSelectStart() { return m_selectStart; }

	Node* getSelectEnd() { return m_selectEnd; }
private:
	NodePtr m_root;                ///< Equation owns this tree.
	std::vector<Input*> m_inputs;  ///< List of input nodes in equation.
	int m_input_index = -1;        ///< Index of current input.
	Node* m_selectStart = nullptr; ///< Node at start of selection.
	Node* m_selectEnd = nullptr;   ///< Node at end of selection.

	/**
	 * Helper static function that parses term in string, load factors into array.
	 */
	void factor(std::string text, NodeVector& factors, Node* parent);

	/**
	 * Output equation into output xml stream.
	 * @return xml XML output stream.
	 */
	void xml_out(XML::Stream& xml) const;

	/**
	 * Load equation from input xml stream.
	 * @param in Input XML stream.
	 */
	void xml_in(XML::Parser& in);
};

/**
 * Take input from keyboard and display it on screen.
 * Usually just holds a string of letters and numbers representing a term.
 */
class Input : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Constructor using a text parser for input.
	 * @param p Input parser.
	 * @param parent Parent of this input node.
	 */
    Input(Parser& p, Node* parent = nullptr);

	/**
	 * Constructor directly taking all needed parameters.
	 * @param eqn Equation holding this input.
	 * @param txt Text stored from keyboard.
	 * @param current True if active.
	 * @param parent  Parent node.
	 * @param neg True if negative sign.
	 * @param s   Selection state.
	 */
    Input(Equation& eqn, std::string txt = std::string(), bool current = true, Node* parent = nullptr,
		  bool neg = false, Node::Select s = Node::Select::NONE);

	/**
	 * Constructor using xml for input
	 * @param in XML input stream.
	 * @param parent Parent of this input node.
     * @param eqn Equation associated with this node.
	 */
	Input(XML::Parser& in, Equation& eqn, Node* parent);
	
	~Input() {} ///< Virtual desctructor.
	//@}
	
	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Output text repesentation of input node.
	 * If empty output a '?' or a '#' if active. Otherwise '[typed_text]'.
	 * @return String containing text representation.
	 */
	std::string toString() const;

	/**
	 * Get number of factors in this node.
	 * @return Number of factors.
	 */
	int numFactors() const { return m_typed.size(); }

	/**
	 * Get name of this class.
	 * @return Name of this class.
	 */
	const std::string& getName() const { return name; }

	/**
	 * Get type of this class.
	 * @return Type of this class.
	 */
	std::type_index getType() const { return type; }
	//@}

	/** @name Helper Public Member functions */
	//@{
	/**
	 * Check if input buffer is empty.
	 * @return True if input buffer is empty
	 */
	bool empty() const { return m_typed.empty(); }

	/**
	 * Add charactor to end of input buffer.
	 * @param ch Character to add to end of buffer.
	 */
	void add(char ch) { m_typed += std::string(1, ch); }

	/**
	 * Add string to end of input buffer.
	 * @param s String to add to end of buffer.
	 */
	void add(const std::string& s) { m_typed += s; }
	
	/**
	 * Remove character from end of input buffer.
	 */
	void remove() { m_typed.erase(m_typed.end() - 1); }

	/**
	 * Clear input buffer.
	 */
	void clear() { m_typed.clear(); }
	
	/**
	 * Get input buffer.
	 * @return Reference to input buffer.
	 */
	const std::string& getBuffer() { return m_typed; }

	/**
	 * Check if serial number matches this node.
	 * @param sn Serial number to check.
	 * @return True if serial number matches.
	 */
	bool checkSN(int sn) { return m_sn == sn; }
	/**
	 * Get state of this node being the active input.
	 * @return If true, this is the current input.
	 */
	bool getCurrent() { return m_current; }

	/**
	 * Set state of this node being the active input.
	 * @param current True, if current active input.
	 */
	void setCurrent(bool current) { m_current = current; }

	/**
	 * Make this node the current input.
	 */
	void makeCurrent() { m_eqn.setCurrentInput(m_sn); }

	/**
	 * Parse and empty the conents of the input buffer.
	 * Parse the contents of the buffer and insert them before the Input object.
	 * Clear the contents of the buffer and return iterator to new position of Input object.
	 * @return Iterator to current position of this object.
	 */
	FactorIterator emptyBuffer();

	/**
	 * Determine if input is to be removed when deactivated.
	 * Input nodes that are empy and the only factor in their term 
	 * should not be removed when deactivated. All others are.
	 * @return True if input node should be removed when deactivated.
	 */
	bool unremovable() { return getParent()->numFactors() == 0; }
	//@}
	
	/**
	 * Static helper function to parse Input class.
	 * Parser class object should be pointing to a string
	 * representation of an input node.
	 * @param p Parser class object.
	 * @param parent Parent of return Input class object.
	 * @return Newly created Input class object.
	 */
	static Input* parse(Parser& p, Node* parent = nullptr);

	static const std::string name;     ///< Name of Input class.
	static const std::type_index type; ///< Type of Input class.
private:
	int m_sn;            ///< Unique serial number of Input class.
	std::string m_typed; ///< Stored type characters from keyboard.
	bool m_current;      ///< True if current active Input class object.
	Box m_internal;      ///< Bounding box of Input node.
	
	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Calculate size of this node in given graphics context.
	 * @param gc Graphics context.
	 * @return Return frame containing size of this node.
	 */
	Frame calcSize(UI::Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(UI::Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(UI::Graphics& gc) const;

	/**
	 * Throw an error if this is called.
	 * @return Nothing is ever returned.
	 */
	Complex getNodeValue() const;
	
	/**
	 * Output XML of this node.
	 * @param xml XML output stream.
	 */
	void xml_out(XML::Stream& xml) const;
	//@}

	static int input_sn; ///< Serial number of last created Input node.
};
#endif // __MILO_H
