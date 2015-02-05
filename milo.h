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
#include <typeindex>

#include "parser.h"

class Graphics;
class Node;
class Input;
class Term;
class Expression;
class Equation;

using Complex = std::complex<double>;
using NodeVector = std::vector<Node*>;
using NodeIter = NodeVector::iterator;

template <class T>
inline auto find(const std::vector<T>& v, const T& val) -> decltype( v.cbegin() )
{
	return std::find(v.cbegin(), v.cend(), val);
}

template <class T>
inline void freeVector(std::vector<T*>& v)
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
    Rectangle(T x, T y, T x0, T y0) : m_rect{x, y, x0, y0} {}
    Rectangle() : m_rect{0, 0, 0, 0} {}

	void set(T x, T y, T x0, T y0) { m_rect = {x, y, x0, y0}; }
	void setOrigin(T x0, T y0)     { m_rect[X0] = x0; m_rect[Y0] = y0; }
	void setSize(T x, T y)         { m_rect[WIDTH] = x; m_rect[HEIGHT] = y; }

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

	bool intersect(const Rectangle& r) const {
		bool noOverlap = (x0() - r.x0()) > r.width() ||
			             (r.x0() - x0()) > width() ||
			             (y0() - r.y0()) > r.height() ||
			             (r.y0() - y0()) > height();
		return !noOverlap;
	}

	void merge(const Rectangle& r) {
		T x0 = min(x0(), r.x0());
		T y0 = min(y0(), r.y0());
		T x1 = max(x0() + width(), r.x0() + r.width());
		T y1 = max(y0() + height(), r.y0() + r.height());
		set(x1 - x0, y1 - y0, x0, y0);
	}

	static Rectangle merge(const Rectangle& r1, const Rectangle& r2) {
		Rectangle r{r1};
		r.merge(r2);
		return r;
	}

private:
	enum { WIDTH, HEIGHT, X0, Y0, SIZE };
	std::array<T, SIZE> m_rect;
};

using Box = Rectangle<int>;

class Node 
{
public:
	enum Select { NONE, START, END, ALL };
	static const std::vector<std::string> select_tags;

	struct Frame { Box box; int base; };

    Node(Node* parent = nullptr, bool fNeg = false, Select s = NONE ) : 
	    m_parent(parent), m_sign(!fNeg), m_select(s) {}
	Node(EqnXMLParser& in, Node* parent, const std::string& name);

	virtual ~Node() {}
	Node(const Node&)=delete;
	Node& operator=(const Node&)=delete;

	virtual std::string toString() const=0;
	virtual bool isLeaf() const { return true; }
	virtual bool isFactor() const { return true; }
	virtual Node* findNode(int x, int y);
	virtual int numFactors() const { return 1; }
	virtual void normalize() {}
	virtual bool simplify() { return false; }
	virtual bool less(Node* b) const { return this->toString() < b->toString(); }
	virtual const std::string& getName() const=0;
	virtual std::type_index getType() const=0;

	void calculateSize(Graphics& gc);
	void calculateOrigin(Graphics& gc, int x, int y);
	void draw(Graphics& gc) const;

	Node* first();
	Node* last();
	Node* getNextLeft();
	Node* getNextRight();
	int getNth() const { return m_nth; }
	void addNth(int n) { m_nth += n; }
	void multNth(int n) { m_nth *= n; }
	void setNth(int n) { m_nth = n; }
	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }
	Complex getValue() const;
	void out(XML::Stream& xml);

	const Frame& getFrame() const { return m_frame; }

	void setParent(Node* parent) { m_parent = parent; }
	Node* getParent() const { return m_parent; }

	void setSelect(Select s) { m_select = s; }
	Select getSelect() const { return m_select; }

	void setDrawParenthesis(bool fDrawPar) { m_fDrawParenthesis = fDrawPar; }
	bool getDrawParenthesis() const { return m_fDrawParenthesis; }

private:
	virtual Node* downLeft() { return nullptr; }
	virtual Node* downRight() { return nullptr; }
	virtual Node* getLeftSibling(Node* node) { return nullptr; }
	virtual Node* getRightSibling(Node* node) { return nullptr; }

	virtual Frame calcSize(Graphics& gc)=0;
	virtual void calcOrig(Graphics& gc, int x, int y)=0;
	virtual void drawNode(Graphics& gc) const=0;
	virtual Complex getNodeValue() const=0;
	virtual void xml_out(XML::Stream& xml) const=0;

	Frame m_frame;
	Box m_parenthesis;
	bool m_sign;
	int m_nth = 1;
	Node* m_parent;
	Select m_select;
	bool m_fDrawParenthesis = false;
};

class Graphics
{
public:
    Graphics() {}
	virtual ~Graphics() {}

	enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

	virtual void parenthesis(int x_size, int y_size, int x0, int y0)=0;
	virtual void horiz_line(int x_size, int x0, int y0)=0;
	virtual void at(int x, int y, int c, Color color = BLACK)=0;
	virtual void at(int x, int y, const std::string& s, Color color = BLACK)=0;
	virtual void out()=0;
   
	virtual int getTextHeight()=0;
	virtual int getTextLength(const std::string& s)=0;
	virtual int getCharLength(char c)=0;
	virtual int getParenthesisWidth(int height = 1)=0;
	virtual int getDivideLineHeight()=0;

	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
	}

	void set(const Box& box) { 
		set(box.width(), box.height(), box.x0(), box.y0());
	}

	virtual void setSelect(int x, int y, int x0, int y0) { m_select.set(x, y, x0, y0); }

	void setSelect(const Box& box) { 
		setSelect(box.width(), box.height(), box.x0(), box.y0());
	}

	void parenthesis(const Box& box) {
		parenthesis(box.width(), box.height(), box.x0(), box.y0());
	}

	void relativeOrig(int& x, int& y) { x -= m_xOrig; y -= m_yOrig; }
protected:
	int m_xSize;
	int m_ySize;
	int m_xOrig;
	int m_yOrig;
	Box m_select;
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
    FactorIterator(Expression* expr);

	Node*& operator*() { return m_node; }
	Node*& operator->() { return m_node; }
	bool operator==(const FactorIterator& rhs) { return m_node == rhs.m_node; }
	bool operator!=(const FactorIterator& rhs) { return m_node != rhs.m_node; }

	FactorIterator& operator++()   { next(); return *this; }
	FactorIterator operator++(int) { FactorIterator tmp(*this); next(); return tmp; }
	FactorIterator& operator--()   { prev(); return *this; }
	FactorIterator operator--(int) { FactorIterator tmp(*this); prev(); return tmp; }

	void erase();
	void insert(Node* node);

	bool isBegin() { return m_factor_index == 0 && m_term_index == 0; }
	bool isEnd()   { return m_node == nullptr; }
	bool isBeginTerm() { return m_factor_index == 0; }
	bool isEndTerm();

	void getNode(int factor, int term);

	FactorIterator getBegin() { FactorIterator tmp(*this); tmp.getNode(0, 0); return tmp; }
	FactorIterator getLast() { FactorIterator tmp(*this); tmp.getNode(-1, -1); return tmp; }
	
	friend class Equation;
};

class Equation
{
public:
	Equation(std::string eq);
    Equation(std::istream& is);
	Equation(const Equation& eqn) { *this = eqn; }
	Equation& operator=(const Equation& eqn);
	Equation(Equation&& eqn)=delete;
	Equation& operator=(Equation&& eqn)=delete;
	~Equation() { delete m_root; }

	std::string toString() const { return m_root->toString(); }
	void xml_out(std::ostream& os) const;
	void xml_out(std::string& str) const;
	void draw(Graphics& gc);

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
	void setSelectFromNode(Node* node);
	Node* findNode(Graphics& gc, int x, int y);
	bool simplify() { m_root->normalize(); return m_root->simplify(); }
	void normalize() { m_root->normalize(); }

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
	void setSelect(Graphics& gc);
	void factor(std::string text, NodeVector& factors, Node* parent);
	void xml_out(XML::Stream& xml) const;
	void eraseSelection(Node* node);

	static FactorIterator insert(FactorIterator it, Node* node);
	static FactorIterator insert(FactorIterator it, Term* term, bool sign = true);
	FactorIterator back_erase(FactorIterator it);
	FactorIterator forward_erase(FactorIterator it);
	static void replace(FactorIterator it, Node* node, bool free = true);
	static void replace(FactorIterator it, Term* term, bool free = true);
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
