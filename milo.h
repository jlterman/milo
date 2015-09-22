#ifndef __MILO_H
#define __MILO_H

/* Copyright (C) 2015 - James Terman
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

#include "xml.h"

// Forward class declerations
class Graphics;
class Node;
class Equation;

// Hidden class declerations for pointers and reference
class Input;
class Term;
class Expression;
class EqnXMLParser;

/** 
 */
using Complex = std::complex<double>;   ///< @brief Specialized as complex double.
using NodeVector = std::vector<Node*>;  ///< @brief Storage of node pointers.
using NodeIter = NodeVector::iterator;  ///< @brief Iterator into vector of node pointers.

/**
 * Convert long integer to hex string no leading zeroes.
 * @return String containing converted hexidecimal string.
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

/**
 * Find an iterator to a value in a vector.
 * Template function will look for a value whose type is stored in the given vector.
 * Returns an iterator to the found value or the end iterator if not found.
 * @param v Vector containing type T.
 * @param val Value of type T.
 * @return Return iterator to found value or end iterator.
 */
template <class T>
inline auto find(const std::vector<T>& v, const T& val) -> decltype( v.cbegin() )
{
	return std::find(v.cbegin(), v.cend(), val);
}

/**
 * Delete all items in a vector that contains pointers
 * Template function delete each pointer in a vector.
 * @param v Vector containing pointer type T*.
 */
template <class T>
inline void freeVector(std::vector<T*>& v)
{
	for ( auto p : v ) { delete p; }
	v.clear();
}

/**
 * Merge vector two vectors.
 * @param a Vector ending up items of both vectors.
 * @param b Vector whose items are merged and cleared.
 */
template <class T>
inline void mergeVectors(std::vector<T>& a, std::vector<T>& b)
{
	a.reserve(a.size() + b.size());
	for ( auto e : b ) { a.insert(a.end(), e); }
	b.clear();
}

/**
 * Erase an element in a vector by index.
 * Helper function to convert index to iterator to erase element.
 * @param v Vector storing type T.
 * @param index Index of element to be erased.
 */
template <class T>
inline void eraseElement(std::vector<T>& v, int index)
{
	v.erase((index < 0 ? v.end() : v.begin()) + index);
}

/**
 * Insert an element after the element pointed to by index in vector.
 * Use vector.insert() with iterator created from index.
 * @param v Vector storing type T.
 * @param index Index of element where new element will be inserted.
 * @param e Element of type T.
 */
template <class T>
inline void insertElement(std::vector<T>& v, int index, T e)
{
	v.insert((index < 0 ? v.end() : v.begin()) + index, e);
}

/**
 * Rectangle template class.
 * Holds a rectange as an origin and a width and height, plus helper member functions.
 * Template type can be any numerical type.
 */
template <class T>
class Rectangle
{
public:
	/**
	 * Default constructor for Rectangle.
	 * Set origin and dimensions to zero.
	 */
    Rectangle() : m_rect{0, 0, 0, 0} {}

	/**
	 * Constructor for Rectangle.
	 * @param x Width of rectangle.
	 * @param y Height of rectangle.
	 * @param x0 Horizontal origin of rectangle.
	 * @param y0 Vertical origin of rectangle.
	 */
    Rectangle(T x, T y, T x0, T y0) : m_rect{x, y, x0, y0} {}

	/**
	 * Mutator to set origin and dimension of Rectangle.
	 * @param x Width of rectangle.
	 * @param y Height of rectangle.
	 * @param x0 Horizontal origin of rectangle.
	 * @param y0 Vertical origin of rectangle.
	 */
	void set(T x, T y, T x0, T y0) { m_rect = {x, y, x0, y0}; }

	/**
	 * Mutator to set origin of Rectangle.
	 * @param x0 Horizontal origin of rectangle.
	 * @param y0 Vertical origin of rectangle.
	 */
	void setOrigin(T x0, T y0)     { m_rect[X0] = x0; m_rect[Y0] = y0; }

	/**
	 * Mutator to set dimension of Rectangle.
	 * @param x Width of rectangle.
	 * @param y Height of rectangle.
	 */
	void setSize(T x, T y)         { m_rect[WIDTH] = x; m_rect[HEIGHT] = y; }

	/**
	 * Check if point is inside Rectangle.
	 * @param x Horizontal origin of point.
	 * @param y Vertical origin of point.
	 * @return Return true if point (x,y) is inside Rectangle.
	 */
	bool inside(T x, T y) const { 
		return ( x >= x0() && x < (x0() + width()) && 
				 y >= y0() && y < (y0() + height()) );
	}

	/**
	 * Accessor of reference to Rectangle's horizontal origin.
	 * @return Reference to Rectangle's horizontal origin.
	 */
	int& x0() { return m_rect[X0]; }

	/**
	 * Accessor of reference to Rectangle's vertical origin.
	 * @return Reference to Rectangle's vertical origin.
	 */
	int& y0() { return m_rect[Y0]; }

	/**
	 * Accessor of reference to Rectangle's width.
	 * @return Reference to Rectangle's width.
	 */
	int& width()   { return m_rect[WIDTH]; }

	/**
	 * Accessor of reference to Rectangle's height.
	 * @return Reference to Rectangle's height.
	 */
	int& height()  { return m_rect[HEIGHT]; }

	/**
	 * Accessor of Rectangle's horizontal origin
	 * @return Rectangle's horizontal origin
	 */
	int x0() const { return m_rect[X0]; }

	/**
	 * Accessor of Rectangle's vertical origin
	 * @return Rectangle's vertical origin
	 */
	int y0() const { return m_rect[Y0]; }

	/**
	 * Accessor of Rectangle's width
	 * @return Rectangle's width
	 */
	int width()  const { return m_rect[WIDTH]; }

	/**
	 * Accessor of Rectangle's height
	 * @return Rectangle's height
	 */
	int height() const { return m_rect[HEIGHT]; }

	/**
	 * Detect overlap between this rectangle and another.
	 * @return True if rectangle r intersects with rectangle in class object.
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

using Box = Rectangle<int>; ///< @brief Specialization of integer rectangle.

/**
 * Abstract base class for symbolic classes that make up an equation.
 * Any class that will be part of the equation tree structor derives from this class.
 */
class Node 
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

	/**
	 * Constructor for Node class.
	 * Directly initialize the Node class private data members.
	 * @param parent Node object that is parent of this node. If NULL, this node is root.
	 * @param fNeg True, if node is negated.
	 * @param s Node selection state.
	 */
    Node(Node* parent = nullptr, bool fNeg = false, Select s = NONE ) : 
	    m_parent(parent), m_sign(!fNeg), m_select(s) {}

	/**
	 * XML constructor for Node class.
	 * Initialize the constructor from XML input stream.
	 * @param in XML input stream.
	 * @param parent Parent node object.
	 */
	Node(EqnXMLParser& in, Node* parent);

	virtual ~Node() {} ///< Abstract base class needs virtual desctructor.

	/**
	 * Serialize this node and child nodes to XML output stream.
	 * @param xml XML ouput stream.
	 */
	void out(XML::Stream& xml);

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
	virtual bool less(Node* b) const { return this->toString() < b->toString(); }

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

	/**
	 * Calculate size of node subtree in given graphics context.
	 * @param gc Graphics context used to calculate size.
	 */
	void calculateSize(Graphics& gc);

	/**
	 * Calculate origin of each node in subtree.
	 * The size of each node in subtree needs to be precalculated.
	 * @param gc Graphics context to calculate origin.
	 * @param x  Horizontal origin of root node in subtree.
	 * @param y  Vertical origin of root node in subtree.
	 */
	void calculateOrigin(Graphics& gc, int x, int y);

	/**
	 * Calculate the size and origin of the subtree of this node.
	 * @param gc Graphics context class object.
	 */
	void setUpDraw(Graphics& gc);

	/**
	 * Draw equation subtree whose root is this node.
	 * @param gc Graphics context class object.
	 */
	void draw(Graphics& gc) const;

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

private:
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
	virtual Frame calcSize(Graphics& gc)=0;

	/**
	 * Calculate origin of the nodes in the subtree relative to point x,y.
	 * @param gc Graphics context.
	 * @param x  Horizontal origin.
	 * @param y  Vertical origin.
	 */
	virtual void calcOrig(Graphics& gc, int x, int y)=0;

	/**
	 * Draw subtree in this graphics context.
	 * @param gc Graphics context.
	 */
	virtual void drawNode(Graphics& gc) const=0;

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

	Node* m_parent;    ///< Parent node of this node. Can be null if root.
	bool m_sign;       ///< True if positve.
	Select m_select;   ///< Select state of this node.
	Frame m_frame;     ///< Frame of this node includes enclosing rect and vertical offset.
	Box m_parenthesis; ///< Rectangle that contains paranthesis of this node.
	int m_nth = 1;     ///< Integer power of ths node.
	bool m_fDrawParenthesis = false; ///< If true, draw paranthesis around this node.
};

/**
 * Abstract base class to provide a context free graphical interface.
 * Provides an interface of helper functions that allow nodes to draw themselves
 * on the current graphical interface.
 */
class Graphics
{
public:
	/**
	 * Default constructor.
	 */
    Graphics() {}

	/**
	 * Abstract base class needs vertical destructor.
	 */
	virtual ~Graphics() {}

	/**
	 * Predefined colors.
	 */
	enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

	/**
	 * Draw differential of width x0 and height y0 with char variable name.
	 * @param x0 Horizontal origin of differential.
	 * @param y0 Vertical origin of differential.
	 * @param variable Name of variable of differential.
	 */
	virtual void differential(int x0, int y0, char variable)=0;

	/**
	 * Draw pair of parenthesis around a node of size x_size, y_size and origin x0,y0.
	 * @param x_size Horizontal size of both parenthesis.
	 * @param y_size Vertical size of parenthesis.
	 * @param x0 Horizontal origin of parenthesis.
	 * @param y0 Vertical origin of parenthesis.
	 */
	virtual void parenthesis(int x_size, int y_size, int x0, int y0)=0;

	/**
	 * Draw a horizontal line starting at x0,y0 length x_size.
	 * @param x_size Horizontal size of both line.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 */
	virtual void horiz_line(int x_size, int x0, int y0)=0;

	/**
	 * Draw a character at x,y with a color.
	 * @param x0 Horizontal origin of character.
	 * @param y0 Vertical origin of character.
	 * @param c  Character to be drawn at x0,y0.
	 * @param color Color of character.
	 */
	virtual void at(int x0, int y0, int c, Color color = BLACK)=0;

	/**
	 * Draw a string at x,y with a color.
	 * @param x0 Horizontal origin of line.
	 * @param y0 Vertical origin of line.
	 * @param s  String to be drawn at x0,y0.
	 * @param color Color of line.
	 */
	virtual void at(int x0, int y0, const std::string& s, Color color = BLACK)=0;

	/**
	 * Flush all drawing so far to graphic interface.
	 */
	virtual void out()=0;

	/**
	 * Get height of text in pixels.
	 * @return Height of text in pixels.
	 */
	virtual int getTextHeight()=0;

	/**
	 * Get length of string in pixels.
	 * @return Length of string in pixels.
	 */
	virtual int getTextLength(const std::string& s)=0;

	/**
	 * Get length of character in pixels.
	 * @return Length of character in pixels.
	 */
	virtual int getCharLength(char c)=0;

	/**
	 * Get width of a parenthesis for a given height in pixels.
	 * @return Width of parenthesis.
	 */
	virtual int getParenthesisWidth(int height = 1)=0;

	/**
	 * Get height of line in a division node.
	 * @return Height of division line.
	 */
	virtual int getDivideLineHeight()=0;

	/**
	 * Get height of differential.
	 * @param c Variable name of differential.
	 * @return Height of differential in pixels.
	 */
	virtual int getDifferentialHeight(char c)=0;

	/**
	 * Get width of differential.
	 * @param c Variable name of differential.
	 * @return Width of differential in pixels.
	 */
	virtual int getDifferentialWidth(char c)=0;

	/**
	 * Get vertical offset of differential.
	 * @param c Variable name of differential.
	 * @return Vertical offset of differential in pixels.
	 */
	virtual int getDifferentialBase(char c)=0;

	/**
	 * Set size and origin of this graphics window.
	 * @param x Horizontal size of both graphics window.
	 * @param y Vertical size of graphics window.
	 * @param x0 Horizontal origin of graphics window.
	 * @param y0 Vertical origin of graphics window.
	 */
	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
	}

	/**
	 * Set size and origin of this graphics window from a rectangle.
	 * @param box Rectangle containing origin and size of window.
	 */
	void set(const Box& box) { 
		set(box.width(), box.height(), box.x0(), box.y0());
	}

	/**
	 * Select the area of size x,y at origin x0,y0.
	 * @param x Horizontal size of both selection area.
	 * @param y Vertical size of selection area.
	 * @param x0 Horizontal origin of selection area.
	 * @param y0 Vertical origin of selection area.
	 */
	virtual void setSelect(int x, int y, int x0, int y0) { m_select.set(x, y, x0, y0); }

	/**
	 * Set selection area from rectangle.
	 * @param box Rectangle containing origin and size of selection area.
	 */
	void setSelect(const Box& box) { 
		setSelect(box.width(), box.height(), box.x0(), box.y0());
	}

	/**
	 * Draw a pair of parenthesis inside the rectangle box.
	 * @param box Rectangle containing origin and size of parenthesis.
	 */
	void parenthesis(const Box& box) {
		parenthesis(box.width(), box.height(), box.x0(), box.y0());
	}

	/**
	 * Change the coordinates x,y to be relative to the graphic's window origin.
	 * @param[in,out] x Horizontal coordinate to be shifted.
	 * @param[in,out] y Vertical coordinate to be shifted.
	 */
	void relativeOrig(int& x, int& y) { x -= m_xOrig; y -= m_yOrig; }
protected:
	int m_xSize;   ///< Horizontal size of graphic window.
	int m_ySize;   ///< Vertical size of graphic window.
	int m_xOrig;   ///< Horizontal origin of graphic window.
	int m_yOrig;   ///< Vertical origin of graphic window.
	Box m_select;  ///< Currently selected area.
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

	/**
	 * Erase factor pointed to by iterator. 
	 * Erase current node leaving iterator pointing to next factor.
	 */
	void erase();

	/**
	 * Insert node after factor pointed to by this iterator.
	 * @param node Factor to be inserted.
	 */
	void insert(Node* node);

	/**
	 * Check if current factor is first in its grandparent expresion.
	 * @return Return true if factor is first in expression.
	 */
	bool isBegin() { return m_factor_index == 0 && m_term_index == 0; }

	/**
	 * Check if current factor is last in its grandparent expresion.
	 * @return Return true if factor is last in expression.
	 */
	bool isEnd()   { return m_node == nullptr; }

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
	void getNode(int factor, int term);

	/**
	 * Get iterator pointing to first factor in the expression of this FactorIterator.
	 * @return New iterator pointing to first factor.
	 */
	FactorIterator getBegin() { FactorIterator tmp(*this); tmp.getNode(0, 0); return tmp; }

	/**
	 * Get iterator pointing to last factor in the expression of this FactorIterator.
	 * @return New iterator pointing to last factor.
	 */
	FactorIterator getLast() { FactorIterator tmp(*this); tmp.getNode(-1, -1); return tmp; }
	
	friend class Equation;
};

/**
 * Holds the node tree that represents a mathematical equation. It also
 * provides an interface for manipulating the nodes to algebraically 
 * transform the equation and to save, load and draw an equation.
 */
class Equation
{
public:
	/**
	 * Constructor to load an equation represented by string such as 'a+b/c'.
	 * @param eq String containing equation to be created.
	 */
	Equation(std::string eq);

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
	 * Overloaded equal operator that copies node tree from another object.
	 * Given equation object is serialized to xml and then deserialized into this object.
	 * @param eqn Equation class object to be copied.
	 */
	Equation& operator=(const Equation& eqn);

	Equation(Equation&& eqn)=delete;            ///< No move copy constructor.
	Equation& operator=(Equation&& eqn)=delete; ///< No move equal operator.

	/**
	 * Destructor deletes node tree to clean up after itself.
	 */
	~Equation() { delete m_root; }

	/**
	 * Get string representation of equation.
	 * Example:  '(a+b/c)'.
	 * @return String representation of equation.
	 */
	std::string toString() const { return m_root->toString(); }

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
	void draw(Graphics& gc);

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
	 * Process character as a command.
	 * @return True if character is valid.
	 */
	bool handleChar(int ch);

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
	 * Set selection of this equation from selection state of node.
	 * If node has selection set to start, set as start of selection.
	 * If node has selection set to end, set as end of selection.
	 * If node has selection set to all, set as start and end of selection.
	 * return node Node whose selection state is referenced.
	 */
	void setSelectFromNode(Node* node);

	/**
	 * Query root node for deepest node at coordinates x, y in given graphics conext.
	 * Call findNode method of root node. If input node found, delete it and search again.
	 * @param gc Graphics context.
	 * @param x  Horizontal coordinate.
	 * @param y  Vertical coordinate.
	 */
	Node* findNode(Graphics& gc, int x, int y);

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
	 * Process string into subequation and insert after node pointed to by FactorIterator.
	 * @param it   Position of insertion.
	 * @param text String containing equation, i.e. '(a+b)/c'.
	 */
	FactorIterator insert(FactorIterator it, std::string text);

	/**
	 * Erase node pointed to by iterator.
	 * @param it   Iterator pointing to node.
	 * @param free If true, free memory for deleted node.
	 */
	FactorIterator erase(FactorIterator it, bool free = true);

private:
	Node* m_root = nullptr;        ///< Equation owns this tree.
	std::vector<Input*> m_inputs;  ///< List of input nodes in equation.
	int m_input_index = -1;        ///< Index of current input.
	Node* m_selectStart = nullptr; ///< Node at start of selection.
	Node* m_selectEnd = nullptr;   ///< Node at end of selection.

	/**
	 * Get current input node.
	 * @return Current input node.
	 */
	Input* getCurrentInput() { 
		return (m_inputs.empty()) ? nullptr : m_inputs[m_input_index];
	}

	/**
	 * Get next input in list after current input node.
	 */
	void nextInput();

	/**
	 * Remove current input node.
	 */
	FactorIterator disableCurrentInput();

	/**
	 * Draw current selection in graphics context.
	 * @param gc Graphics context.
	 */
	void setSelect(Graphics& gc);

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
	void xml_in(EqnXMLParser& in);

	/**
	 * Erase current selection and replace with new node.
	 * If node parameter is null, selection will just be erased.
	 * @param node Node to replace current selection.
	 */
	void eraseSelection(Node* node);

	/**
	 * Helper static function that inserts node after iterator position.
	 * @param it   Iterator position for insert.
	 * @param node Node to be inserted.
	 */
	static FactorIterator insert(FactorIterator it, Node* node);

	/**
	 * Helper static fucntion that inserts term after the parent term pointed to by FactorIterator.
	 * @param it   Iterator position for insert.
	 * @param term Term to be inserted.
	 * @param sign Sign of term.
	 */
	static FactorIterator insert(FactorIterator it, Term* term, bool sign = true);

	/**
	 * Erase node behind the iterator position.
	 * @param it   Iterator position for insert.
	 */
	FactorIterator back_erase(FactorIterator it);

	/**
	 * Erase node ahead the iterator position.
	 * @param it   Iterator position for insert.
	 */
	FactorIterator forward_erase(FactorIterator it);

	/**
	 * Replace node pointed to by iterator.
	 * @param it   Iterator pointing to node to be deleted.
	 * @param node Node to replace deleted node.
	 * @param free If true, free memory for deleted node.
	 */
	static void replace(FactorIterator it, Node* node, bool free = true);

	/**
	 * Replace node pointed to by iterator.
	 * @param it   Iterator pointing to term to be deleted (current node's parent).
	 * @param term Term to replace deleted term
	 * @param free If true, free memory for deleted term.
	 */
	static void replace(FactorIterator it, Term* term, bool free = true);
};

/**
 * Holds a list of pointers to equation objects to support a undo mechanisim.
 */
class EqnUndoList
{
public:
	/**
	 * Default constructor.
	 */
	EqnUndoList() {}

	/**
	 * Add pointer to Equation object to list.
	 */
	void save(Equation* eqn);

	/**
	 * Pop pointer to last equation object.
	 * @return Last equation saved.
	 */
	Equation* undo();

	/**
	 * Get pointer to last equation saved.
	 * @return Last equation saved.
	 */
	Equation* top();

private:
	std::vector<std::string> m_eqns; ///< List of pointer to equation objects in undo history.
};

/**
 * Logging for milo program.
 */
namespace Log
{
	/**
	 * Pint message along with file and line number information to milo log.
	 */
	void msg(std::string m);
}

#define LOG_TRACE_MSG(m) Log::msg(string("") + __FILE__ + ": " + to_string(__LINE__) + ": " + (m))
#define LOG_TRACE_FILE "/tmp/milo.log"
#define LOG_TRACE_CLEAR() remove(LOG_TRACE_FILE)
#endif // __MILO_H
