#ifndef __NODES_H
#define __NODES_H

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
 * @file nodes.h
 * This file defines the classes that make up the nodes in an equation tree.
 * They derive from the abstract base class Node and need to implement the 
 * virtual member functions. This header file is not part of the external interface.
 */
#include <vector>
#include <exception>
#include <map>
#include "milo.h"

// Forward class decleration
class Parser;

/** @name Global Utility Functions */
//@{
/**
 * Check if floating point is smaller than a limit.
 * @return If smaller than limit, return true for zero.
 */
bool isZero(double x);

/**
 * Check if complex floating point is smaller than a limit.
 * @return If smaller than limit, return true for zero.
 */
bool isZero(Complex z);

/**
 * Check if string contains integer value.
 * @return True, if integer.
 */
bool isInteger(const std::string& n);

/**
 * Check if double is integer value.
 * @return True is fractional value is smaller than a limit.
 */
bool isInteger(double value);
//@}

/** @name Global Type Declerations  */
//@{
using TermVector = std::vector<Term*>; ///< @brief Specialized vector of terms.
//@}

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
    Input(Equation& eqn, std::string txt = std::string(), bool current = false, Node* parent = nullptr,
		  bool neg = false, Node::Select s = Node::Select::NONE);

	/**
	 * Constructor using xml for input
	 * @param in XML input stream.
	 * @param parent Parent of this input node.
	 */
	Input(EqnXMLParser& in, Node* parent);
	
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

	friend class Equation;
private:
	int m_sn;            ///< Unique serial number of Input class.
	std::string m_typed; ///< Stored type characters from keyboard.
	bool m_current;      ///< True if current active Input class object.
	Equation& m_eqn;     ///< Equation class object containing this input node.
	Box m_internal;      ///< Bounding box of Input node.
	
	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Calculate size of this node in given graphics context.
	 * @param gc Graphics context.
	 * @return Return frame containing size of this node.
	 */
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

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
	
	/**
	 * Set state of this node being the active input.
	 * @param current True, if current active input.
	 */
	void setCurrent(bool current) { m_current = current; }

	static int input_sn; ///< Serial number of last created Input node.
};

/**
 * Base class for nodes represent an operator that contains two expressions.
 * Not all pure virtual functions are overriden so is an abstract base class.
 */
class Binary : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Constructor for Binary class.
	 * @param op  Character representing operator.
	 * @param one First expression.
	 * @param two Second expression.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
	Binary(char op, Node* one, Node* two, Node* parent, bool neg, Node::Select s) : 
	    Node(parent, neg, s), m_op(op), m_first(one), m_second(two) {}

	/**
	 * XML constructor for Binary class.
	 * Read in XML that is common for all nodes that have a Binary base class.
	 * @param in XML input stream.
	 * @param parent Parent node object.	 
	 */
	Binary(EqnXMLParser& in, Node* parent);

	/**
	 * Virtual desctructor.
	 * Abstract base class needs virtual desctructor.
	 */
	virtual ~Binary() { delete m_first; delete m_second; }
	//@}
	
	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Get string representation of Binary object.
	 * Simply first node, operator, second node.
	 * @return String representation of Binary object.
	 */
	std::string toString() const { return m_first->toString() + std::string(1, m_op) + m_second->toString(); }

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
	 * Recursive function to return first node from a top to bottom
	 * search whose frame is inside Box b.
	 * @param b Bounding box.
	 * @return Top node inside bounding box.
	 */
	Node* findNode(const Box& b);

	/**
	 * Get number of factor in this subtree.
	 * @return Return number of factors in both expressions.
	 */
	int numFactors() const { return m_first->numFactors() + m_second->numFactors(); }
	//@}
	
	/**
	 * Get first expression of this node.
	 * @return First expression.
	 */
	Expression* getFirstExpression();

	/**
	 * Get second expression of this node.
	 * @return Second expression.
	 */
	Expression* getSecondExpression();

	/**
	 * Static helper function to parse Binary class.
	 * @param p Parser object pointing to operator.
	 * @param one First expression already parsed.
	 * @param parent Parent node.
	 * @return Binary object or null if operator not binary.
	 */
	static Node* parse(Parser& p, Node* one, Node* parent);

protected:
	char m_op;      ///< Character repesentation of operator.
	Node* m_first;  ///< First expression owned by this object.
	Node* m_second; ///< Second expression owned by this object.
	Box m_internal; ///< Bounding box of this node.

private:
	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Get left node.
	 * For Binary class is its first expression.
	 * @return Left node.
	 */
	Node* downLeft()  { return m_first; }

	/**
	 * Get right node.
	 * For Binary class is its second expression.
	 * @return Right node.
	 */
	Node* downRight() { return m_second; }

	/**
	 * Left sibiling of given node.
	 * If node given is right node, return left node.
	 * @param node Reference node.
	 * @return Left sibiling or null.
	 */
	Node* getLeftSibling(Node* node)  { return (m_second == node) ? m_first->last() : nullptr; }

	/**
	 * Right sibiling of given node.
	 * If node given is left node, return right node.
	 * @param node Reference node.
	 * @return Right sibiling or null.
	 */
	Node* getRightSibling(Node* node) { return (m_first == node) ? m_second->first() : nullptr; }

	/**
	 * Output XML of this node.
	 * @param xml XML output stream.
	 */
	void xml_out(XML::Stream& xml) const;
	//@}
};

/**
 * Represents the division operator.
 * Inherits from Binary class and is drawn with two expressions separated by a horizontal line.
 */
class Divide : public Binary
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Constructor for Divide class.
	 * @param one First expression.
	 * @param two Second expression.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Divide(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('/', one, two, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this); 
		m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}

	 /**
	 * XML constructor for Divide class.
	 * Read in XML for Divide class.
	 * @param in XML input stream.
	 * @param parent Parent node object.	 
	 */
    Divide(EqnXMLParser& in, Node* parent) : Binary(in, parent)
	{ 
		m_op = '/'; m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Divide() {}
	//@}
	
	/** @name Virtual Public Member Functions */
	//@{
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

	/**
	 * Refactor subtree to a standard form.
	 * For Divide class, replace a/b with ab^-1.
	 */
	void normalize();

	/**
	 * Attempt to algebraically simplify the subtree this node is the root.
	 * @return True, if subtree was changed.
	 */
	bool simplify();
	//@}
	
	static const std::string name;     ///< Name of Divide class.
	static const std::type_index type; ///< Type of Divide class.

 private:
	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Calculate size of this node in given graphics context.
	 * @param gc Graphics context.
	 * @return Return frame containing size of this node.
	 */
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const;
	//@}
};

/**
 * Represents the Power operator.
 * Inherits from Binary class and is drawn with the first expression 
 * to the power of the second expression.
 */
class Power : public Binary
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Constructor for Power class.
	 * @param one First expression.
	 * @param two Second expression.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
     Power(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('^', one, two, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this);
		m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}

	/**
	  * XML constructor for Divide class.
	  * Read in XML for Divide class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
    Power(EqnXMLParser& in, Node* parent) : Binary(in, parent)
	{ 
		m_op = '^'; m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Power() {}
	//@}
	
	/** @name Virtual Public Member Functions */
	//@{
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

	/**
	 * Refactor subtree to a standard form.
	 * For Power class, replace (ab)^2(c^d)^2  with (a^2)(b^2)c^(2d)
	 */
	void normalize();

	/**
	 * Attempt to algebraically simplify the subtree this node is the root.
	 * @return True, if subtree was changed.
	 */
	bool simplify();

	/**
	 * Static helper function for simplifying Power expressions.
	 * Simplify each node in the vector against every other node.
	 * @return True, if node vector was changed.
	 */
	static bool simplify(NodeVector& factors);
	//@}
	
	static const std::string name;     ///< Name of Power class.
	static const std::type_index type; ///< Type of Power class.

private:
	/** @name Virtual Private Member Functions */
	//@{
  	/**
	 * Calculate size of this node in given graphics context.
	 * @param gc Graphics context.
	 * @return Return frame containing size of this node.
	 */
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const;
	//@}
	
	/**
	 * Static helper function for simplifying Power expressions.
	 * If both iterators point to a Power object, see if they can be combined.
	 * @param a First iterator.
	 * @param b Second iterator.
	 * @return True, if node vector was changed.
	 */
	static bool simplify(NodeVector::iterator a, NodeVector::iterator b);
};

/**
 * Represents a Constant factor node.
 * Each Constant maps a name to a value implemented by a constant static map.
 */
class Constant : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Parser constructor for Constant class.
	 * @param p Parser object.
	 * @param parent Parent node.
	 */
	Constant(Parser& p, Node* parent);

	/**
	 * Constructor for Constant class.
	 * @param name Constant name.
	 * @param value Complex value of constant.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Constant(char name, Complex value, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name), m_value(value) {}

	 /**
	  * XML constructor for Constant class.
	  * Read in XML for Constant class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
	Constant(EqnXMLParser& in, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Constant() {}
	//@}
	
	using const_map = std::map<char, Complex>; ///< @brief Specialization for mapping name to value.

	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * String representation of constant is its name.
	 * @return Name of constant.
	 */
	std::string toString() const { return std::string() + m_name; }

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
	
	static const std::string name;     ///< Name of constant.
	static const std::type_index type; ///< Type of constant.

	/**
	 * Static helper function to parse Constant class.
	 * @param p Parser object pointing to operator.
	 * @param parent Parent node.
	 * @return Constant object or null if constant not present.
	 */
	static Constant* parse(Parser& p, Node* parent);
private:
	char m_name;                      ///< Name of constant.
	Complex m_value;                  ///< Value of constant.
	Box m_internal;                   ///< Bounding box of this node.
	
	static const const_map constants; ///< Mapping of all constant name to their values.

	/** @name Virtual Private Member Functions */
	//@{
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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this constant.
	 * @return Complex value of this constant.
	 */
	Complex getNodeValue() const { return constants.at(m_name); }
	//@}
};


/**
 * Represents a Variable factor node.
 */
class Variable : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Parser constructor for Variable class.
	 * @param p Parser object.
	 * @param parent Parent node.
	 */
	Variable(Parser& p, Node* parent);

	/**
	 * Constructor for Constant class.
	 * @param name Variable name.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
     Variable(char name, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name)
    { 
		values.emplace(name, Complex(0, 0)); 
	}

	 /**
	  * XML constructor for Variable class.
	  * Read in XML for Variable class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
	Variable(EqnXMLParser& in, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Variable() {}
	//@}
	
	using var_map = std::map<char, Complex>; ///< @brief Specialization for mapping name to value.

	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * String representation of variable is its name.
	 * @return Name of variable.
	 */
	std::string toString() const { return std::string(1, m_name); }

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
	
	static const std::string name;     ///< Name of Variable class.
	static const std::type_index type; ///< Type of Variable class.

	/**
	 * Static helper function to parse Variable class.
	 * @param p Parser object pointing to operator.
	 * @param parent Parent node.
	 * @return Variable object or null if variable not present.
	 */
	static Variable* parse(Parser& p, Node* parent);

	/**
	 * Static helper function to set value of a variable.
	 * @param name Variable name.
	 * @param value Value of Variable.
	 */
	static void setValue(char name, Complex value);

private:
	char m_name;    ///< Name of Variable
	Box m_internal; ///< Bounding box of this node.

	static var_map values; ///< Mapping of all variable name to their values.

	/** @name Virtual Private Member Functions */
	//@{
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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const { return values[m_name]; }
	//@}
};

/**
 * Represents a Number factor node that contains a real value.
 * Complex numbers are represented with with the i Constant factor. 
 */
class Number : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Parser constructor for Number class.
	 * @param p Parser object.
	 * @param parent Parent node.
	 */
    Number(Parser& p, Node* parent) : Node(parent), m_isInteger(true) { getNumber(p); }

	/**
	 * Constructor for Number class.
	 * @param real String containing real value.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
	Number(std::string real, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(stod(real)), m_isInteger(isInteger(real)) {}

	/**
	 * Constructor for Number class.
	 * @param n Integer containing real value.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
	Number(int n, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(n), m_isInteger(true) {}

	/**
	 * Constructor for Number class.
	 * @param d Double containing real value.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
	Number(double d, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(d), m_isInteger(isInteger(d)) {}

	 /**
	  * XML constructor for Number class.
	  * Read in XML for Number class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
	Number(EqnXMLParser& in, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Number() {}
	//@}
	
	/**
	 * Load numeric value from Parser.
	 * @param p Parser object.
	 */
	void getNumber(Parser& p);

	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Get string representation of Number value.
	 * @return String representation of Number value.
	 */
	std::string toString() const;

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

	/**
	 * Attempt to algebraically simplify the subtree this node is the root.
	 * @return True, if subtree was changed.
	 */
	bool simplify();
	//@}
	
	static const std::string name;     ///< Name of Number class.
	static const std::type_index type; ///< Type of Number class.

	/**
	 * Static helper function to parse Number class.
	 * @param p Parser object pointing to operator.
	 * @param parent Parent node.
	 * @return Number object or null if number not present.
	 */
	static Number* parse(Parser& p, Node* parent);

private:
	double m_value;   ///< Value of Number.
	bool m_isInteger; ///< True, if integer.
	Box m_internal;   ///< Bounding box of this node.

	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Static helper function to parse integer from Parser.
	 * @param p Parser object.
	 * @return String containing integer.
	 */
	static std::string getInteger(Parser& p);

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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const { return {m_value, 0}; }
	//@}
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
    Term(Parser& p, Expression* parent = nullptr) : Node((Node*) parent) { while(add(p)); }

	 /**
	  * XML constructor for Term class.
	  * Read in XML for Term class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
	Term(EqnXMLParser& in, Node* parent = nullptr);

	/**
	 * Constructor for Term class loading in factors from a node vector.
	 * @param f Factors to be loaded into new Term object.
	 * @param parent Parent expresion.
	 */
    Term(NodeVector& f, Expression* parent) : Node((Node*) parent) { factors.swap(f); }

 	/**
	 * Constructor for Term class.
	 * @param node Initialize node vector with this node.
	 * @param parent Parent expresion.
	 * @param fNeg If true, node is negative.
	 */
     Term(Node* node, Expression* parent = nullptr, bool fNeg = false) : 
	    Node((Node*) parent, fNeg), factors(1, node) 
	{
		node->setParent(this);
	}

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Term() { freeVector(factors); }
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
	 * Recursive function to return first node from a top to bottom
	 * search whose frame is inside Box b.
	 * @param b Bounding box.
	 * @return Top node inside bounding box.
	 */
	Node* findNode(const Box& b);

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
	void simplify(Node* ref, Term* new_term);

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
	static void insertAfterMe(Node* me, Node* node);

	/**
	 * Get index of factor in Term's internal vector node.
	 * @param node Factor to be indexed.
	 * @return Index of node.
	 */
	int getFactorIndex(Node* node) { return distance(factors.cbegin(), find(factors, node)); }

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
	void multiply(Term* old_term);

	friend class Equation;
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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;
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
	  */
	Expression(EqnXMLParser& in, Node* parent = nullptr);

	/**
	 * Parser constructor for Expression class.
	 * @param p Parser object.
	 * @param parent Parent node.
	 */
    Expression(Parser& p, Node* parent = nullptr) : Node(parent)
	{
		while(add(p)); setDrawParenthesis(true);
	}

 	/**
	 * Constructor for Expression class.
	 * @param term Initialize term vector with this term.
	 * @param parent Parent expresion.
	 */
    Expression(Term* term, Node* parent = nullptr) : Node(parent), terms(1, term)
	{ 
		term->setParent(this); setDrawParenthesis(true);
	}

 	/**
	 * Constructor for Expression class.
	 * @param factor Initialize term vector with new term containing node.
	 * @param parent Parent expresion.
	 */
    Expression(Node* factor, Node* parent = nullptr) : 
	    Node(parent), terms(1, new Term(factor, this))
	{ 
		factor->setParent(terms[0]); setDrawParenthesis(true);
	}

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Expression() { freeVector(terms); }
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
	 * Recursive function to return first node from a top to bottom
	 * search whose frame is inside Box b.
	 * @param b Bounding box.
	 * @return Top node inside bounding box.
	 */
	Node* findNode(const Box& b);

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
	 * Get index of Term object in expression.
	 * @param term Term to be indexed.
	 * @return Index of term.
	 */
	int getTermIndex(Term* term) { return distance(terms.cbegin(), find(terms, term)); }

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
	static Term* getTerm(Equation& eqn, std::string text, Expression* parent = nullptr);

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
	static Expression* parse(Parser& p, Node* parent);

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
	void add(Expression* old_expr);

	friend class Equation;
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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

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
 * Represents a function by containing an expression passed to a mathematical function.
 */
class Function : public Node
{
public:
	/**
	 * Function pointer that takes a single complex argument and returns a complex value.
	 */
	typedef Complex (*func_ptr)(Complex);

	/**
	 * @brief Map that associates function name with its function call.
	 */
	using func_map = std::map<std::string, func_ptr>;

	/**
	 * Get function object from Parser.
	 * @param p Parser object.
	 * @param parent Parent for new Function object.
	 * @return Function object or null if no function found in parser.
	 */
	static Function* parse(Parser& p, Node* parent);

	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Get representation of Function as string.
	 * String representation is function_name(argument).
	 * @return String representation of Function.
	 */
	std::string toString() const { return m_name + m_arg->toString(); }

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
	 * Recursive function to return first node from a top to bottom
	 * search whose frame is inside Box b.
	 * @param b Bounding box.
	 * @return Top node inside bounding box.
	 */
	Node* findNode(const Box& b);

	/**
	 * Get number of factors in this node.
	 * @return Number of factors of argument plus this node.
	 */
	int numFactors() const { return m_arg->numFactors() + 1; }

	/**
	 * Refactor subtree to a standard form.
	 * For Function class, normalize argument.
	 */
	void normalize() { m_arg->normalize(); }

	/**
	 * Function class specialization of default virtural member function.
	 * Sort functions in reverse alphabetical order.
	 * @param b Reference node.
	 * @return True, if b is less than this node.
	 */
	bool less(Node* b) const;

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
	
	static const std::string name;     ///< Name of Function class.
	static const std::type_index type; ///< Type of Function type.
	
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Constructor for Function class.
	 * @param name Name of function.
	 * @param fp Pointer to function to evaluate arguments.
	 * @param p Parser object.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Function(const std::string& name, func_ptr fp, Parser& p, Node* parent, 
			 bool neg = false, Node::Select s = Node::Select::NONE) : 
	             Node(parent, neg, s), m_name(name), m_func(fp), m_arg(new Expression(p, this)) {}

	 /**
	  * XML constructor for Function class.
	  * Read in XML for Function class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
	Function(EqnXMLParser& in, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Function() { delete m_arg; }
	//@}
private:
	std::string m_name; ///< Name of function.
	func_ptr m_func;    ///< Function pointer to evaluate function.
	Node* m_arg;        ///< Function own this tree.
	Box m_internal;     ///< Bounding box of this node.

	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Get left node.
	 * For Function class left most node of argument.
	 * @return Left node.
	 */
	Node* downLeft()  { return m_arg->first(); }

	/**
	 * Get right node.
	 * For Function class right most node of argument.
	 * @return Right node.
	 */
	Node* downRight() { return m_arg->last(); }
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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const;
	//@}
	
	/** Association of function names with their function pointers.	
	 */
	static const func_map functions;

	static Complex sinZ(Complex z); ///< Function for sin.
	static Complex cosZ(Complex z); ///< Function for cos.
	static Complex tanZ(Complex z); ///< Function for tan.
	static Complex logZ(Complex z); ///< Function for log.
	static Complex expZ(Complex z); ///< Function for exp.
};

/**
 * Represents a differential of an expression to a variable.
 */
class Differential : public Node
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	/**
	 * Constructor for Differential class.
	 * @param p Parser object.
	 * @param parent Parent node.
	 */
    Differential(Parser& p, Node* parent);

	/**
	  * XML constructor for Differential class.
	  * Read in XML for Differential class.
	  * @param in XML input stream.
	  * @param parent Parent node object.	 
	  */
	Differential(EqnXMLParser& in, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Differential() {}
	//@}
	
	/** @name Virtual Public Member Functions */
	//@{
	/**
	 * Override for isLeaf virtual function.
	 * @return False, this node has child nodes.
	 */
	bool isLeaf() const { return false; }

	/**
	 * Get number of factors in this node.
	 * @return Number of factors in function plus this node.
	 */
	int numFactors() const { return m_function->numFactors()+1; }

	/**
	 * Rescursive search to find deepest node at point (x,y).
	 * @param x horizontal coordinate.
	 * @param y vertical coordinate.
	 * @return Deepest node at point (x,y).	 
	 */
	Node* findNode(int x, int y) { return m_function->findNode(x, y); }

	/**
	 * Recursive function to return first node from a top to bottom
	 * search whose frame is inside Box b.
	 * @param b Bounding box.
	 * @return Top node inside bounding box.
	 */
	Node* findNode(const Box& b);

	/**
	 * Get string representation of this node.
	 * @return String representation.
	 */
	std::string toString() const;

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

	/**
	 * Refactor subtree to a standard form.
	 * For Differential class, normalize function argument.
	 */
	void normalize() { m_function->normalize(); }

	/**
	 * Attempt to algebraically simplify the subtree this node is the root.
	 * @return True, if subtree was changed.
	 */
	bool simplify() { return false; }
	//@}
	
	static const std::string name;     ///< Name of Differential class.
	static const std::type_index type; ///< Type of Differential class.

	/**
	 * Get Differential object from Parser.
	 * @param p Parser object.
	 * @param parent Parent for new Differential object.
	 * @return Differential object or null if no differential found in parser.
	 */
	static Differential* parse(Parser& p, Node* parent);

private:
	Box m_internal;          ///< Bounding box of this node.
	char m_variable;         ///< Variable of Differential.
	Expression* m_function;  ///< Function to be differentiated.

	/** @name Virtual Private Member Functions */
	//@{
	/**
	 * Get left node.
	 * For Differential class left most node of function argument.
	 * @return Left node.
	 */
	Node* downLeft()  { return m_function->first(); }

	/**
	 * Get right node.
	 * For Differential class right most node of function argument.
	 * @return Right node.
	 */
	Node* downRight() { return m_function->last(); }
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
	Frame calcSize(Graphics& gc);

	/**
	 * Calculate origin of this node offset from the point (x,y) given it.
	 * @param gc Graphics context.
	 * @param x Horizontal origin.
	 * @param y Vertical origin.
	 */
	void calcOrig(Graphics& gc, int x, int y);

	/**
	 * Draw this node in the given graphical context.
	 * @param gc Graphical context.
	 */
	void drawNode(Graphics& gc) const;

	/**
	 * Get value of this subtree.
	 * @return Complex value of this subtree.
	 */
	Complex getNodeValue() const { return {0, 0}; }
	//@}
};

#endif // __NODES_H
