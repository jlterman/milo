#ifndef __MILO_H
#define __MILO_H

/*
 * milo.h
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

#include "xml.h"

class Graphics;
class Node;
class Input;
class Term;
class Expression;
class Equation;
class EqnXMLParser;

using Complex = std::complex<double>;
using NodeVector = std::vector<Node*>;
using NodeIter = NodeVector::iterator;

/* Convert integer of any size to a hex string
 */
inline std::string to_hexstring(unsigned long x)
{
	std::string hex;
	while (x != 0) {
		int n = x % 16; x /= 16;
		char d = (n < 10) ? '0' + n : 'A' + n - 10;
		hex.insert(0, 1, d);
	}
	return hex;
}

/* Convenience function to look for a value in entire vector
 */
template <class T>
inline auto find(const std::vector<T>& v, const T& val) -> decltype( v.cbegin() )
{
	return std::find(v.cbegin(), v.cend(), val);
}

/* Passing a vector that contains pointer which need to be individually freed
 */
template <class T>
inline void freeVector(std::vector<T*>& v)
{
	for ( auto p : v ) { delete p; }
	v.clear();
}

/* Add vector b to vector a and clear vector a
 */
template <class T>
inline void mergeVectors(std::vector<T>& a, std::vector<T>& b)
{
	a.reserve(a.size() + b.size());
	for ( auto e : b ) { a.insert(a.end(), e); }
	b.clear();
}

/* Erase an element in a vector by index
 */
template <class T>
inline void eraseElement(std::vector<T>& v, int index)
{
	v.erase((index < 0 ? v.end() : v.begin()) + index);
}

/* Insert an element in a vector by index
 */
template <class T>
inline void insertElement(std::vector<T>& v, int index, T e)
{
	v.insert((index < 0 ? v.end() : v.begin()) + index, e);
}

/* Template class Rectangle that holds a rectange as an origin
 * and a width and height, plus helper member functions
 */
template <class T>
class Rectangle
{
public:
    Rectangle(T x, T y, T x0, T y0) : m_rect{x, y, x0, y0} {}
    Rectangle() : m_rect{0, 0, 0, 0} {}

	void set(T x, T y, T x0, T y0) { m_rect = {x, y, x0, y0}; }
	void setOrigin(T x0, T y0)     { m_rect[X0] = x0; m_rect[Y0] = y0; }
	void setSize(T x, T y)         { m_rect[WIDTH] = x; m_rect[HEIGHT] = y; }

	/* Return true if point (x,y) is inside rectangle
	 */
	bool inside(T x, T y) const { 
		return ( x >= x0() && x < (x0() + width()) && 
				 y >= y0() && y < (y0() + height()) );
	}

	int& x0() { return m_rect[X0]; }
	int& y0() { return m_rect[Y0]; }
	int& width()   { return m_rect[WIDTH]; }
	int& height()  { return m_rect[HEIGHT]; }

	int x0() const { return m_rect[X0]; }
	int y0() const { return m_rect[Y0]; }
	int width()  const { return m_rect[WIDTH]; }
	int height() const { return m_rect[HEIGHT]; }

	/* Return true if rectangle r intersect rectangle in class object
	 */
	bool intersect(const Rectangle& r) const {
		bool noOverlap = (x0() - r.x0()) > r.width() ||
			             (r.x0() - x0()) > width() ||
			             (y0() - r.y0()) > r.height() ||
			             (r.y0() - y0()) > height();
		return !noOverlap;
	}

	/* Merge rectangle in class object with rectangle r
	 */
	void merge(const Rectangle& r) {
		T x0 = min(x0(), r.x0());
		T y0 = min(y0(), r.y0());
		T x1 = max(x0() + width(), r.x0() + r.width());
		T y1 = max(y0() + height(), r.y0() + r.height());
		set(x1 - x0, y1 - y0, x0, y0);
	}

	/* return rectangle that is merger of r1 and r2
	 */
	static Rectangle merge(const Rectangle& r1, const Rectangle& r2) {
		Rectangle r{r1};
		r.merge(r2);
		return r;
	}

private:
	enum { WIDTH, HEIGHT, X0, Y0, SIZE };
	std::array<T, SIZE> m_rect;
};

/* Box is specialization of integer rectangle
 */
using Box = Rectangle<int>;

/*************************************************************************
 *
 * class Node is the virtual base class for all objects that make up an
 * equation tree structure.
 */
class Node 
{
public:
	/*  enum Select are the possible selection states for a node
	 *  NONE  - not selected
	 *  START - node starts selection
	 *  END   - node ends selection
	 *  ALL   - just this node selected
	 *
	 *  vector select_tags: read-only list of strings corresponding to enum Select
	 */
	enum Select { NONE, START, END, ALL };
	static const std::vector<std::string> select_tags;

	/* struct Frame holds the box that contains the node graphic,
	 * plus its base for vertical alignment
	 */
	struct Frame { Box box; int base; };

	/* Basic constructor for Node that initializes 3 key parameters:
	 * m_parent - parent node of this node (can be NULL if root)
	 * m_fNeg   - true if negative
	 * m_select - selection state of this node
	 */
    Node(Node* parent = nullptr, bool fNeg = false, Select s = NONE ) : 
	    m_parent(parent), m_sign(!fNeg), m_select(s) {}

    /* Node constructor that takes in xml to build object
	 */
	Node(EqnXMLParser& in, Node* parent);

	/* All objects derived from this class are polymorphic and not copyable
	 */
	virtual ~Node() {}
	Node(const Node&)=delete;
	Node& operator=(const Node&)=delete;

	// Stream this node and its subtree as XML output
	void out(XML::Stream& xml);

	/*****************************
	 * Public virtual functions
	 */

	// Express node as unique string
	virtual std::string toString() const=0;

	// False is node has children node. Default true
	virtual bool isLeaf() const { return true; }

	// False if class type Term. Default true
	virtual bool isFactor() const { return true; }

	// Return pointer to deepest node where point (x,y) is inside
	virtual Node* findNode(int x, int y);

	// number of factors in this node and any associated subtree
	virtual int numFactors() const { return 1; }

	// Change this node and attached subtree to a well-defined algebraic form 
	virtual void normalize() {}

	// Simplify this node and attached subtree to simplier algebraic form
	virtual bool simplify() { return false; }

	// Compare the this node to the given node
	virtual bool less(Node* b) const { return this->toString() < b->toString(); }

	// Return name of the derived type
	virtual const std::string& getName() const=0;

	// Return type_index of the derived type
	virtual std::type_index getType() const=0;

	/************************************************
	 * Member functions to support drawing equations
	 */

	// Calculate size of this node and in subtree in graphics context 
	void calculateSize(Graphics& gc);

	// Set the origin of this node and calculate size of nodes in
	// subtree based on their size
	void calculateOrigin(Graphics& gc, int x, int y);

	// Set up drawing for this node and its subtree
	void setUpDraw(Graphics& gc);

	// Draw this node and subtree in this graphic context
	void draw(Graphics& gc) const;

	// Set selection state of this node
	void setSelect(Select s) { m_select = s; }

	// Get selections state of this node
	Select getSelect() const { return m_select; }

	// Set draw parenthesis flag of this node
	void setDrawParenthesis(bool fDrawPar) { m_fDrawParenthesis = fDrawPar; }

	// Get draw parenthesis flag of this node
	bool getDrawParenthesis() const { return m_fDrawParenthesis; }

	// Get frame of this node
	const Frame& getFrame() const { return m_frame; }

	/****************************************
	 * Member functions for navigating tree
	 */
	
	// Return left most node
	Node* first();

	// Return right most node
	Node* last();

	// Return node to the left. Go up if necessary
	Node* getNextLeft();

	// Return node to the right. Go down if necessary
	Node* getNextRight();

	// Set parent of this node
	void setParent(Node* parent) { m_parent = parent; }

	// Get parent of this node
	Node* getParent() const { return m_parent; }

	/*****************************************************
	 * Member functions involved in symbolic manipulation
	 */

	// Get value of m_nth
	int getNth() const { return m_nth; }

	// Add n to node's value of m_nth
	void addNth(int n) { m_nth += n; }

	// Multiply n to node's value of m_nth
	void multNth(int n) { m_nth *= n; }

	// Set node's value of m_nth
	void setNth(int n) { m_nth = n; }

	// Negate this node
	void negative() { m_sign = !m_sign; }

	// Get sign of this node
	bool getSign() const { return m_sign; }

	// Calculate value of this node and it subtree
	Complex getValue() const;

private:
	/*******************************************
	 * Private virtual member functions
	 */
	
	// Return left most child  inside this node
	virtual Node* downLeft() { return nullptr; }

	// Return right most child  inside this node
	virtual Node* downRight() { return nullptr; }

	// Return left sibling of this node
	virtual Node* getLeftSibling(Node*) { return nullptr; }

	// Return right sibling of this node
	virtual Node* getRightSibling(Node*) { return nullptr; }

	// Calculate size of this node. To do this it is necessary
	// to calculate all descendant nodes first
	
	virtual Frame calcSize(Graphics& gc)=0;

	// Calculate origin of this node relative to the coord given it
	virtual void calcOrig(Graphics& gc, int x, int y)=0;

	// Draw this node in given graphic context
	virtual void drawNode(Graphics& gc) const=0;

	// Return value of this node
	virtual Complex getNodeValue() const=0;

	// Generate xml serialization of this node
	virtual void xml_out(XML::Stream& xml) const=0;

	/**************************************************
	 * Private member data
	 */

	// Parent node of this node. Can be null if root
	Node* m_parent;

	// True if positve
	bool m_sign;

	// Select state of this node
	Select m_select;

	// Frame of this node includes enclosing rect and vertical offset
	Frame m_frame;

	// Rectangle that contains paranthesis of this node
	Box m_parenthesis;

	// Integer power of ths node
	int m_nth = 1;

	// If true, draw paranthesis around this node
	bool m_fDrawParenthesis = false;
};

/*************************************************************************
 *
 * Graphics class is a Virtual base class to provide a context free 
 * interface to allow the nodes to draw themselves
 */
class Graphics
{
public:
    Graphics() {}
	virtual ~Graphics() {}

	// Predefined colors
	enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

	/***********************************
	 * Public virtual functions
	 */

	// Draw differential of width x0 and height y0 with char variable name
	virtual void differential(int x0, int y0, char variable)=0;

	// Draw pair of parenthesis around a node of size x_size, y_size and origin x0,y0
	virtual void parenthesis(int x_size, int y_size, int x0, int y0)=0;

	// Draw a horizontal line starting at x0,y0 length x_size
	virtual void horiz_line(int x_size, int x0, int y0)=0;

	// Draw a character at x,y with a color
	virtual void at(int x, int y, int c, Color color = BLACK)=0;

	// Draw a string at x,y with a color
	virtual void at(int x, int y, const std::string& s, Color color = BLACK)=0;

	// Buffer out all drawing so far
	virtual void out()=0;

	// Return height of text in pixels
	virtual int getTextHeight()=0;

	// Return length of string in pixels
	virtual int getTextLength(const std::string& s)=0;

	// Return length of character in pixels
	virtual int getCharLength(char c)=0;

	// Return width of a parenthesis for a given height in pixels
	virtual int getParenthesisWidth(int height = 1)=0;

	// Return height of line in a division node
	virtual int getDivideLineHeight()=0;

	// get height of differential for a variable c
	virtual int getDifferentialHeight(char c)=0;

	// get width of differential for a variable c
	virtual int getDifferentialWidth(char c)=0;

	// get base of differential for a variable c
	virtual int getDifferentialBase(char c)=0;

	// Set size and origin of this graphics window
	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
	}

	// Set size and origin of this graphics window from a rectangle
	void set(const Box& box) { 
		set(box.width(), box.height(), box.x0(), box.y0());
	}

	// Select the area of size x,y at origin x0,y0
	virtual void setSelect(int x, int y, int x0, int y0) { m_select.set(x, y, x0, y0); }

	// Select the box
	void setSelect(const Box& box) { 
		setSelect(box.width(), box.height(), box.x0(), box.y0());
	}

	// Draw a pair of parenthesis inside the rectangle box
	void parenthesis(const Box& box) {
		parenthesis(box.width(), box.height(), box.x0(), box.y0());
	}

	// Change the coordinates x,y to be relative to the origin
	void relativeOrig(int& x, int& y) { x -= m_xOrig; y -= m_yOrig; }
protected:
	/**********************************************************************
	 * Protected member data (proctected so derived classes have access)
	 */

	int m_xSize;   // horizontal size
	int m_ySize;   // vertical size
	int m_xOrig;   // horizontal position
	int m_yOrig;   // vertical position
	Box m_select;  // currently selected area
};

/*************************************************************************
 *
 * NodeIterator is an iterator class that allows you to go backwards and
 * forwards over every node in an equation. The precedence goes from deepest
 * left most node to deepest right mode node.
 */
class NodeIterator : public std::iterator< std::bidirectional_iterator_tag, Node* >
{
private:
	Node* m_node; // current node

	void next();  // get next node
	void prev();  // get previous node

public:
	// NodeIterator constructor initialzing iterator with node
	NodeIterator(Node* node) : m_node(node) {}

	// Overloaded dereference operator to get node iterator points to
	Node*& operator*() { return m_node; }

	// Overloaded member function operator to access node member functions
	Node*& operator->() { return m_node; }

	// overloaded equal operator
	bool operator==(const NodeIterator& rhs) { return m_node == rhs.m_node; }

	// overloaded not equal operator
	bool operator!=(const NodeIterator& rhs) { return m_node != rhs.m_node; }

	// overloaded postfix increment operator
	NodeIterator& operator++()   { next(); return *this; }

	// overloaded prefix increment operator
	NodeIterator operator++(int) { NodeIterator tmp(*this); next(); return tmp; }

	// overloaded postfix decrement operator
	NodeIterator& operator--()   { prev(); return *this; }

	// overloaded prefix decrement operator
	NodeIterator operator--(int) { NodeIterator tmp(*this); prev(); return tmp; }
};

/*************************************************************************
 *
 * FactorIterator is an iterator class that allows you to go backwards and
 * forwards over every factor in each term in an expressiont. The precedence 
 * first factor in first termn to last factor in last term.
 */
class FactorIterator : public std::iterator< std::bidirectional_iterator_tag, Node* >
{
 private:
	Node* m_node;                  // current factor
	Term* m_pTerm;                 // parent term of current factor
	Expression* m_gpExpr;          // grandparent expresson of current factor
	unsigned int m_factor_index;   // index of current factor in parent termn
	unsigned int m_term_index;     // term index of current factor in grandparent expression

	void next();                   // get next factor in expression
	void prev();                   // get previous factor in expression

public:
	// FactorIterator constructor for iterator points at node
    FactorIterator(Node* node);

	// FactorIterator constructor from NodeIterator
    FactorIterator(NodeIterator n) : FactorIterator(*n) {}

	// FactorIterator starts at beginning of expression
    FactorIterator(Expression* expr);

	// Overloaded dereference operator to get node iterator points to
	Node*& operator*() { return m_node; }

	// Overloaded member function operator to access node member functions
	Node*& operator->() { return m_node; }
	bool operator==(const FactorIterator& rhs) { return m_node == rhs.m_node; }

	// overloaded not equal operator
	bool operator!=(const FactorIterator& rhs) { return m_node != rhs.m_node; }

	// overloaded postfix increment operator
	FactorIterator& operator++()   { next(); return *this; }

	// overloaded prefix increment operator
	FactorIterator operator++(int) { FactorIterator tmp(*this); next(); return tmp; }

	// overloaded postfix decrement operator
	FactorIterator& operator--()   { prev(); return *this; }

	// overloaded prefix decrement operator
	FactorIterator operator--(int) { FactorIterator tmp(*this); prev(); return tmp; }

	// erase factor pointed to by iterator. After erasure iterator points to next factor
	void erase();

	// insert node after factor pointed to by this iterator
	void insert(Node* node);

	// return true if current factor is first in its grandparent expresion
	bool isBegin() { return m_factor_index == 0 && m_term_index == 0; }

	// return true if current factor is last in its grandparent expresion
	bool isEnd()   { return m_node == nullptr; }

	// return true if current factor parent term is first in expression
	bool isBeginTerm() { return m_factor_index == 0; }

	// return true if current factor parent term is last in expression
	bool isEndTerm();

	// point iterator to a specific factor in a specific term
	void getNode(int factor, int term);

	// return first factor in the expression of this FactorIterator
	FactorIterator getBegin() { FactorIterator tmp(*this); tmp.getNode(0, 0); return tmp; }

	// return last factor in the expression of this FactorIterator
	FactorIterator getLast() { FactorIterator tmp(*this); tmp.getNode(-1, -1); return tmp; }
	
	friend class Equation;
};

/*
 * Equation class holds the node tree that represents a mathematical equation. It also
 * provides an interface for manipulating the nodes to algebraically transform the equation
 * and to save, load and draw an equation.
 */
class Equation
{
public:
	// Constructor to load an equation represented by string such as 'a+b/c'
	Equation(std::string eq);

	// Constructor to load an equation from xml
    Equation(std::istream& is);

	// Copy constructor based on overloaded operator=
	Equation(const Equation& eqn) { *this = eqn; }

	// Overloaded operator= that copies node tree to another equation object
	Equation& operator=(const Equation& eqn);

	// No move copy constructor
	Equation(Equation&& eqn)=delete;

	// No move operator=
	Equation& operator=(Equation&& eqn)=delete;

	// Destructor deletes node tree to clean up after itself
	~Equation() { delete m_root; }

	// Return string version of equation '(a+b/c)'
	std::string toString() const { return m_root->toString(); }

	// Serialize equation as xml to output stream
	void xml_out(std::ostream& os) const;

	// Serialize equation as xml to string
	void xml_out(std::string& str) const;

	// Draw equation to given graphic context
	void draw(Graphics& gc);

	// Return equation object that is copy of this equation object
	Equation* clone();

	// Return where there is an active input in this equation object
	bool blink() { return m_input_index >= 0; }

	// Return coordinates of current active input
	void getCursorOrig(int& x, int& y);

	// Set current active input node
	void setCurrentInput(int in_sn);

	// Add input node to equation's list of input nodes
	void addInput(Input* in) { m_inputs.push_back(in); }

	// Remove input node from equation's list of input nodes
	void removeInput(Input* in);

	// Send character to current input for processing. Return true if used
	bool handleChar(int ch);

	// Set node as start of selection
	void setSelectStart(Node* start) { m_selectStart = start; }

	// Set node as end of selelection
	void setSelectEnd(Node* end)   { m_selectEnd = end; }

	// Clear current selection
	void clearSelect();

	// Set start and end of selection
	void setSelect(Node* start, Node* end = nullptr);

	// if node has selection set to start, set as start of selection
	// if node has selection set to end, set as end of selection
	// if node has selection set to all, set as start and end of selection
	void setSelectFromNode(Node* node);

	// Return node at coordinates x, y in given graphics conext
	Node* findNode(Graphics& gc, int x, int y);

	// Simplify equation algebracially by recursivelly calling node->simplify() starting at root
	bool simplify() { m_root->normalize(); return m_root->simplify(); }

	// Normalize equation to standard format to make simplification easier
	void normalize() { m_root->normalize(); }

	// return left most node of equation
	NodeIterator begin() { return NodeIterator(m_root->first()); }

	// return end iterator of equation
	NodeIterator end()   { return NodeIterator(nullptr); }

	// return right most node of equation
	NodeIterator last()  { return NodeIterator(m_root->last()); }

	// insert equation string (a+b/c) after node pointed to by FactorIterator
	FactorIterator insert(FactorIterator it, std::string text);

	// Erase node pointed to by factor iterator
	FactorIterator erase(FactorIterator it, bool free = true);

private:
	Node* m_root = nullptr;        // Equation owns this tree

	std::vector<Input*> m_inputs;  // List of input nodes in equation
	int m_input_index = -1;        // Index of current input
	Node* m_selectStart = nullptr; // node at start of selection
	Node* m_selectEnd = nullptr;   // node at end of selection

	// Return current input node
	Input* getCurrentInput() { 
		return (m_inputs.empty()) ? nullptr : m_inputs[m_input_index];
	}

	// Get next input in input list after current
	void nextInput();

	// Remove current input
	FactorIterator disableCurrentInput();

	// Draw current selection in graphics context
	void setSelect(Graphics& gc);

	// Helper fucntions that parses term in string, load factors into array
	void factor(std::string text, NodeVector& factors, Node* parent);

	// Output equation into output xml stream
	void xml_out(XML::Stream& xml) const;

	// Load equaiton from input xml stream
	void xml_in(EqnXMLParser& in);

	// Erase current selection and replace it with node if non-null
	void eraseSelection(Node* node);

	// Helper static function that inserts factor after factor pointed to by FactorIterator
	static FactorIterator insert(FactorIterator it, Node* node);

	// Helper static fucntion that inserts term after the parent term of factor pointed to by FactorIterator
	static FactorIterator insert(FactorIterator it, Term* term, bool sign = true);

	// Erase previous node to the factor pointed to by FactorIterator
	FactorIterator back_erase(FactorIterator it);

	// Erase next node to the factor pointed to by FactorIterator
	FactorIterator forward_erase(FactorIterator it);

	// Erase factor pointed to by FactorIterator and replace with node
	static void replace(FactorIterator it, Node* node, bool free = true);

	// Erase the term parent to the factor pointed to by FactorIterator and replace with term
	static void replace(FactorIterator it, Term* term, bool free = true);
};

/* EquUndoList class holds a list of pointers to equation objects to support a undo mechanisim
 */
class EqnUndoList
{
public:
	// EqnUndoList default constructor
	EqnUndoList() {}

	// Add eqn to list
	void save(Equation* eqn);

	// Pop last eqn entered and return pointer to it
	Equation* undo();

	// REturn last eqn entered
	Equation* top();

private:
	std::vector<std::string> m_eqns; // list of pointer to equation objects in undo history
};

/* Log::msg will print message along with file and line number information to milo log
 */
namespace Log
{
	void msg(std::string m);
}

#define LOG_TRACE_MSG(m) Log::msg(string("") + __FILE__ + ": " + to_string(__LINE__) + ": " + (m))
#define LOG_TRACE_FILE "/tmp/milo.log"
#define LOG_TRACE_CLEAR() remove(LOG_TRACE_FILE)
#endif // __MILO_H
