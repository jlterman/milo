#ifndef __NODES_H
#define __NODES_H

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
 * @file nodes.h
 * This file defines the classes that make up the nodes in an equation tree.
 * They derive from the abstract base class Node and need to implement the 
 * virtual member functions. This header file is not part of the external interface.
 */
#include <vector>
#include <exception>
#include <unordered_map>
#include "milo.h"

// Forward class decleration
class Parser;

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
	 * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Binary(char op, Node* one, Node* two, Equation& eqn, Node* parent, bool neg, Node::Select s) : 
	    Node(eqn, parent, neg, s), m_op(op), m_first(one), m_second(two) {}

	/**
	 * XML constructor for Binary class.
	 * Read in XML that is common for all nodes that have a Binary base class.
	 * @param in XML input stream.
	 * @param eqn Equation associated with this node.
	 * @param parent Parent node object.	 
	 */
	Binary(XML::Parser& in, Equation& eqn, Node* parent);

	/**
	 * Virtual desctructor.
	 * Abstract base class needs virtual desctructor.
	 */
	virtual ~Binary() {}
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
	ExpressionPtr getFirstExpression();

	/**
	 * Get second expression of this node.
	 * @return Second expression.
	 */
	ExpressionPtr getSecondExpression();

	/**
	 * Static helper function to parse Binary class.
	 * @param p Parser object pointing to operator.
	 * @param one First expression already parsed.
	 * @param parent Parent node.
	 * @return Binary object or null if operator not binary.
	 */
	static Node* parse(Parser& p, Node* one, Node* parent);

protected:
	char m_op;        ///< Character repesentation of operator.
	NodePtr m_first;  ///< First expression owned by this object.
	NodePtr m_second; ///< Second expression owned by this object.
	Box m_internal;   ///< Bounding box of this node.

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
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Divide(Node* one, Node* two, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('/', one, two, eqn, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this); 
		m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}

   /**
	 * XML constructor for Divide class.
	 * Read in XML for Divide class.
	 * @param in XML input stream.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node object.	 
 	 */
    Divide(XML::Parser& in, Equation& eqn, Node* parent) : Binary(in, eqn, parent)
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

	/**
	 * Static helper function to create new Divide node object.
	 * Create a new divide node object at the eqn's current input.
	 * @param eqn Equation to create new divide object.
	 * @return bool True, if divide object created.
	 */
	static bool create(Equation& eqn);
	
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
	 * Static Helper function for Divide::normalize()
	 * @param n Node* containing a/b
	 * @return  Return expressoin ab^-1
	 */
	static Node* normalize(Node* n);
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
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Power(Node* one, Node* two, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('^', one, two, eqn, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this);
		m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}

	/**
	 * XML constructor for Divide class.
	 * Read in XML for Divide class.
	 * @param in XML input stream.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node object.	 
 	 */
    Power(XML::Parser& in, Equation& eqn, Node* parent) : Binary(in, eqn, parent)
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
	//@}

	/**
	 * Static helper function to create new Power node object.
	 * Create a new power node object at the eqn's current input.
	 * @param eqn Equation to create new power object.
	 * @return bool True, if power object created.
	 */
	static bool create(Equation& eqn);

	/**
	 * Static helper function for simplifying Power expressions.
	 * Simplify each node in the vector against every other node.
	 * @return True, if node vector was changed.
	 */
	static bool simplify(NodeVector& factors);
	
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
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Constant(char name, Complex value, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(eqn, parent, neg, s), m_name(name), m_value(value) {}

	 /**
	  * XML constructor for Constant class.
	  * Read in XML for Constant class.
	  * @param in XML input stream.
      * @param eqn Equation associated with this node.
	  * @param parent Parent node object.	 
	  */
	Constant(XML::Parser& in, Equation& eqn, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Constant() {}
	//@}
	
	using const_map = std::unordered_map<char, Complex>; ///< @brief Specialization for mapping name to value.

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
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Variable(char name, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(eqn, parent, neg, s), m_name(name)
    { 
		values.emplace(name, Complex(0, 0)); 
	}

	/**
	 * XML constructor for Variable class.
	 * Read in XML for Variable class.
	 * @param in XML input stream.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node object.	 
	 */
	Variable(XML::Parser& in, Equation& eqn, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Variable() {}
	//@}
	
	using var_map = std::unordered_map<char, Complex>; ///< @brief Specialization for mapping name to value.

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
	static void setValue(char name, const Complex& value);

	/**
	 * Static helper function to set real value of a variable.
	 * @param name Variable name.
	 * @param real Real value of Variable.
	 */
	static void setRealValue(char name, double real);

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
    Number(Parser& p, Node* parent) : Node(p, parent), m_isInteger(true) { getNumber(p); }

	/**
	 * Constructor for Number class.
	 * @param real String containing real value.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Number(std::string real, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(eqn, parent, neg, s), m_value(stod(real)), m_isInteger(isInteger(real)) {}

	/**
	 * Constructor for Number class.
	 * @param n Integer containing real value.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Number(int n, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(eqn, parent, neg, s), m_value(n), m_isInteger(true) {}

	/**
	 * Constructor for Number class.
	 * @param d Double containing real value.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
	Number(double d, Equation& eqn, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(eqn, parent, neg, s), m_value(d), m_isInteger(isInteger(d)) {}

	 /**
	  * XML constructor for Number class.
	  * Read in XML for Number class.
	  * @param in XML input stream.
      * @param eqn Equation associated with this node.
	  * @param parent Parent node object.	 
	  */
	Number(XML::Parser& in, Equation& eqn, Node* parent);

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
	Complex getNodeValue() const { return {m_value, 0}; }
	//@}
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
	using func_ptr = Complex (*)(Complex);

	/**
	 * @brief Map that associates function name with its function call.
	 */
	using func_map = std::unordered_map<std::string, func_ptr>;

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
	bool less(NodePtr b) const;

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
     * @param eqn Equation associated with this node.
	 * @param parent Parent node.
	 * @param neg If true, node is negative.
	 * @param s   Selection state of node.
	 */
    Function(const std::string& name, func_ptr fp, Parser& p, Node* parent, 
			 bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(p, parent, neg, s), m_name(name), m_func(fp), m_arg(new Expression(p, this)) {}

	/**
	 * XML constructor for Function class.
	 * Read in XML for Function class.
	 * @param in XML input stream.
     * @param eqn Equation associated with this node.
	 * @param parent Parent node object.	 
	 */
	Function(XML::Parser& in, Equation& eqn, Node* parent);

	/**
	 * Abstract base class needs virtual destructor.
	 */
	~Function() {}
	//@}
private:
	std::string m_name; ///< Name of function.
	func_ptr m_func;    ///< Function pointer to evaluate function.
	NodePtr m_arg;      ///< Function own this tree.
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
      * @param eqn Equation associated with this node.
	  * @param parent Parent node object.	 
	  */
	Differential(XML::Parser& in, Equation& eqn, Node* parent);

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
	Box m_internal;      ///< Bounding box of this node.
	char m_variable;     ///< Variable of Differential.
	NodePtr m_function;  ///< Function to be differentiated.

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
	Complex getNodeValue() const { return {0, 0}; }
	//@}
};

#endif // __NODES_H
