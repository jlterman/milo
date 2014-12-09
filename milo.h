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

bool isZero(double x);
bool isZero(Complex z);
bool isInteger(const std::string& n);

class Draw;
class XML;
class XMLParser;
class Node;
using NodeVector = std::vector<Node*>;

template <class T>
inline auto find(const std::vector<T>& v, const T& val) -> decltype( v.begin() )
{
	return std::find(v.begin(), v.end(), val);
}

template <class T>
inline void freeVector(std::vector<T*> v)
{
	for ( auto p : v ) { delete p; }
	v.clear();
}

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

	Node* begin();
	Node* end();
	Node* getNextLeft();
	Node* getNextRight();
	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }

	int getSizeX() const { return termSize_x; }
	int getSizeY() const { return termSize_y; }
	int getOrigX() const { return termOrig_x; }
	int getOrigY() const { return termOrig_y; }
	int  getBaseLine() const { return m_base; }

	void setSize(int x, int y) { termSize_x = x; termSize_y = y; }
	void setOrig(int x, int y) { termOrig_x = x; termOrig_y = y; }
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

	int termSize_x;
	int termSize_y;
	int termOrig_x;
	int termOrig_y;
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

	void parenthesis(const Node* node) { 
		parenthesis(node->getSizeX(), node->getSizeY(), 
					node->getOrigX(), node->getOrigY());
	}

protected:
	int m_xSize;
	int m_ySize;
	int m_xOrig;
	int m_yOrig;
};

class Equation;
class Parser;

class Input : public Node
{
public:
    Input(Parser& p, Node* parent = nullptr);
    Input(Equation& eqn, std::string txt, bool current, Node* parent, bool neg, Node::Select s);
	~Input() { freeVector(m_left); freeVector(m_right); }

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	bool handleChar(int ch);

	static Node* parse(Parser& p, Node* parent = nullptr);
	static Node* xml_in(XMLParser& in, Node* parent);

	friend class Equation;
private:
	NodeVector m_left;  // own these factors
	NodeVector m_right; // own these factors

	std::string m_typed;
	int m_sn;
	bool m_active;
	bool m_current;
	Equation& m_eqn;

	void disable() { m_active = false; }
	void setCurrent(bool current) { m_current = current; }
	void addTyped(char c) { m_typed += c; }
	bool handleBackspace();
	void swapLeftTerm(Node* term);
	static int input_sn;
};

class Equation
{
public:
	Equation(std::string eq);
    Equation(std::istream& is);
	~Equation() { delete m_root; }
	std::string toString() const { return m_root->toString(); }
	void xml_out(XML& xml) const;
	void xml_out(std::ostream& os) const;
	void xml_out(std::string& str) const;
	void asciiArt(Draw& draw) const;
	Node* getRoot() { return m_root; }

	Input* getCurrentInput() { 
		return (m_input_index < 0) ? nullptr : m_inputs[m_input_index];
	}
	void nextInput();
	bool disableCurrentInput();

	void setCurrentInput(int in_sn);
	void addInput(Input* in) { m_inputs.push_back(in); }
	bool handleChar(int ch);
	bool handleBackspace();

	void setSelectStart(Node* node) { m_selectStart = node; }
	void setSelectEnd(Node* node)   { m_selectEnd = node; }

private:
	Node* m_root = nullptr; // Equation owns this tree

	std::vector<Input*> m_inputs;
	int m_input_index = -1;
	Node* m_selectStart = nullptr;
	Node* m_selectEnd = nullptr;

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
#endif // __MILO_H
