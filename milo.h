#ifndef __MILO_H
#define __MILO_H

#include <string>
#include <memory>
#include <complex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <algorithm>

using Complex = std::complex<double>;

class Draw;
class XML;
class XMLParser;
class Parser;
class Node;
class Input;
class Term;
class Expression;
class Equation;
using NodeVector = std::vector<Node*>;

template <class T>
inline auto find(const std::vector<T>& v, const T& val) -> decltype( v.cbegin() )
{
	return std::find(v.cbegin(), v.cend(), val);
}

template <class T>
inline void freeVector(std::vector<T*> v)
{
	for ( auto p : v ) { delete p; }
	v.clear();
}

template <class T>
inline void mergeVectors(std::vector<T>& a, std::vector<T>& b)
{
	a.reserve(a.size() + b.size());
	for ( auto e : b ) { a.insert(a.end(), e); }
	b.clear();
}

template <class T>
inline void eraseElement(std::vector<T>& v, int index)
{
	v.erase((index < 0 ? v.end() : v.begin()) + index);
}

template <class T>
inline void insertElement(std::vector<T>& v, int index, T e)
{
	v.insert((index < 0 ? v.end() : v.begin()) + index, e);
}

template <class T>
class Rectangle
{
public:
    Rectangle(T x, T y, T x0, T y0) : m_width(x), m_height(y), m_xOrig(x0), m_yOrig(y0) {}
    Rectangle() : m_width(0), m_height(0), m_xOrig(0), m_yOrig(0) {}

	void set(T x, T y, T x0, T y0) { m_width = x; m_height = y; m_xOrig = x0; m_yOrig = y0; }

	bool inside(T x, T y) { 
		return ( x >= m_xOrig && x < (m_xOrig + m_width) && y >= m_yOrig && y < (m_yOrig + m_height) );
	}

	bool intersect(const Rectangle& r) {
		bool noOverlap = (m_xOrig - r.m_xOrig) > r.m_width ||
			             (r.m_xOrig - m_xOrig) > m_width ||
			             (m_yOrig - r.m_yOrig) > r.m_height ||
			             (r.m_yOrig - m_yOrig) > m_height;
		return !noOverlap;
	}

private:
	T m_width;
	T m_height;
	T m_xOrig;
	T m_yOrig;
};

class Node 
{
public:
	enum Select { NONE, START, END, ALL };
	static const std::vector<std::string> select_tags;

    Node(Node* parent = nullptr, bool fNeg = false, Select s = NONE ) : 
	    m_parent(parent), m_sign(!fNeg), m_select(s) {}
	virtual ~Node() {}

	virtual void xml_out(XML& xml) const=0;
	virtual std::string toString() const=0;
	virtual void calcSize()=0;
	virtual void calcOrig(int x, int y)=0;
	virtual void asciiArt(Draw& draw) const=0;
	virtual bool drawParenthesis() const { return false; }
	virtual bool isLeaf() const { return true; }
	virtual bool isFactor() const { return true; }
	virtual Complex getNodeValue() const=0;
	virtual Node* findNode(int x, int y);

	Node* first();
	Node* last();
	Node* getNextLeft();
	Node* getNextRight();
	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }
	int getSignValue() const { return m_sign ? 1 : -1; }
	Complex getValue() { return Complex(getSignValue(), 0)*getNodeValue(); }

	int getSizeX() const { return m_xSize; }
	int getSizeY() const { return m_ySize; }
	int getOrigX() const { return m_xOrig; }
	int getOrigY() const { return m_yOrig; }
	int  getBaseLine() const { return m_base; }

	void setSize(int x, int y) { m_xSize = x; m_ySize = y; }
	void setOrig(int x, int y) { m_xOrig = x; m_yOrig = y; }
	void setBaseLine(int base) { m_base = base; }

	void setParent(Node* parent) { m_parent = parent; }
	Node* getParent() const { return m_parent; }

	void setSelect(Select s) { m_select = s; }
	Select getSelect() const { return m_select; }

private:
	virtual Node* downLeft() { return nullptr; }
	virtual Node* downRight() { return nullptr; }
	virtual Node* getLeftSibling(Node* node) { return nullptr; }
	virtual Node* getRightSibling(Node* node) { return nullptr; }

	int m_xSize;
	int m_ySize;
	int m_xOrig;
	int m_yOrig;
	bool m_sign;
	Node* m_parent;
	int m_base;
	Select m_select;
};

class Draw
{
public:
    Draw() {}
	virtual ~Draw() {}

	enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

	virtual void parenthesis(int x_size, int y_size, int x0, int y0)=0;
	virtual void horiz_line(int x_size, int x0, int y0)=0;
	virtual void at(int x, int y, int c, Color color = BLACK)=0;
	virtual void at(int x, int y, const std::string& s, Color color = BLACK)=0;
	virtual void out()=0;
   
	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
	}

	void set(Node* node) { 
		set(node->getSizeX(), node->getSizeY(), node->getOrigX(), node->getOrigY());
	}

	virtual void setSelect(int x, int y, int x0, int y0) { m_select.set(x, y, x0, y0); }

	void setSelect(const Node* node) { 
		setSelect(node->getSizeX(), node->getSizeY(), node->getOrigX(), node->getOrigY());
	}


	void parenthesis(const Node* node) { 
		parenthesis(node->getSizeX(), node->getSizeY(), 
					node->getOrigX(), node->getOrigY());
	}

	void relativeOrig(int& x, int& y) { x -= m_xOrig; y -= m_yOrig; }
protected:
	int m_xSize;
	int m_ySize;
	int m_xOrig;
	int m_yOrig;
	Rectangle<int> m_select;
};

class NodeIterator : public std::iterator< std::bidirectional_iterator_tag, Node* >
{
private:
	Node* m_node;

	void next();
	void prev();

public:
	NodeIterator(Node* node, Equation& eqn) : m_node(node) {}

	Node*& operator*() { return m_node; }
	Node*& operator->() { return m_node; }
	bool operator==(const NodeIterator& rhs) { return m_node == rhs.m_node; }
	bool operator!=(const NodeIterator& rhs) { return m_node != rhs.m_node; }

	NodeIterator& operator++()   { next(); return *this; }
	NodeIterator operator++(int) { NodeIterator tmp(*this); next(); return tmp; }
	NodeIterator& operator--()   { prev(); return *this; }
	NodeIterator operator--(int) { NodeIterator tmp(*this); prev(); return tmp; }
};

class FactorIterator : public std::iterator< std::bidirectional_iterator_tag, Node* >
{
 private:
	Node* m_node;
	Term* m_pTerm;
	Expression* m_gpExpr;
	int m_factor_index;
	int m_term_index;

	void next();
	void prev();

public:
    FactorIterator(Node* node);
    FactorIterator(NodeIterator n) : FactorIterator(*n) {}

	Node*& operator*() { return m_node; }
	Node*& operator->() { return m_node; }
	bool operator==(const FactorIterator& rhs) { return m_node == rhs.m_node; }
	bool operator!=(const FactorIterator& rhs) { return m_node != rhs.m_node; }

	FactorIterator& operator++()   { next(); return *this; }
	FactorIterator operator++(int) { FactorIterator tmp(*this); next(); return tmp; }
	FactorIterator& operator--()   { prev(); return *this; }
	FactorIterator operator--(int) { FactorIterator tmp(*this); prev(); return tmp; }

private:
	bool isBegin() { return m_factor_index == 0 && m_term_index == 0; }
	bool isEnd()   { return m_node == nullptr; }
	bool isBeginTerm() { return m_factor_index == 0; }
	bool isEndTerm();

	void getNode(int factor, int term);

	FactorIterator getBegin() { FactorIterator tmp(*this); tmp.getNode(0, 0); return tmp; }
	FactorIterator getEnd() { FactorIterator tmp(*this); tmp.getNode(-1, -1); return tmp; }
	
	friend class Equation;
};

class Equation
{
public:
	Equation(std::string eq);
    Equation(std::istream& is);
	~Equation() { delete m_root; }
	std::string toString() const { return m_root->toString(); }
	void xml_out(std::ostream& os) const;
	void xml_out(std::string& str) const;
	void asciiArt(Draw& draw);

	Equation* clone();
	bool blink() { return m_input_index >= 0; }
	void getCursorOrig(int& x, int& y);
	void setCurrentInput(int in_sn);
	void addInput(Input* in) { m_inputs.push_back(in); }
	void removeInput(Input* in);
	bool handleChar(int ch);
	void setSelectStart(Node* start) { m_selectStart = start; }
	void setSelectEnd(Node* end)   { m_selectEnd = end; }
	void clearSelect();
	void setSelect(Node* start, Node* end = nullptr);
	Node* findNode(Draw& draw, int x, int y);

	NodeIterator begin() { return NodeIterator(m_root->first(), *this); }
	NodeIterator end()   { return NodeIterator(nullptr, *this); }
	NodeIterator last()  { return NodeIterator(m_root->last(), *this); }

	FactorIterator insert(FactorIterator it, std::string text);
	FactorIterator erase(FactorIterator it, bool free = true);

private:
	Node* m_root = nullptr; // Equation owns this tree

	std::vector<Input*> m_inputs;
	int m_input_index = -1;
	Node* m_selectStart = nullptr;
	Node* m_selectEnd = nullptr;

	Input* getCurrentInput() { 
		return (m_input_index < 0) ? nullptr : m_inputs[m_input_index];
	}
	void nextInput();
	FactorIterator disableCurrentInput();
	void setSelect(Draw& draw);
	void factor(std::string text, NodeVector& factors, Node* parent);
	void xml_out(XML& xml) const;
	void eraseSelection(Node* node);

	static FactorIterator insert(FactorIterator it, Node* node);
	static FactorIterator insert(FactorIterator it, Term* term);
	FactorIterator back_erase(FactorIterator it);
	FactorIterator forward_erase(FactorIterator it);
	static void replace(FactorIterator it, Node* node, bool free = true);
	static void replace(FactorIterator it, Term* term, bool free = true);

	static Node* xml_in(XMLParser& in);
};

class EqnUndoList
{
public:
	EqnUndoList() {}

	void save(Equation* eqn);
	Equation* undo();
	Equation* top();

private:
	std::vector<std::string> m_eqns;
};

namespace Log
{
	void msg(std::string m);
}

#define LOG_TRACE_MSG(m) Log::msg(string("") + __FILE__ + ": " + to_string(__LINE__) + ": " + (m))
#define LOG_TRACE_FILE "/tmp/milo.log"
#define LOG_TRACE_CLEAR() remove(LOG_TRACE_FILE)
#endif // __MILO_H
