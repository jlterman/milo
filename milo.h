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

using Complex = std::complex<double>;

bool isZero(double x);
bool isZero(Complex z);
bool isInteger(const std::string& n);

class Draw;
class XML;
class XMLParser;
class Node;
using NodePtr = std::shared_ptr<Node>;

inline bool operator==(NodePtr& a, Node*& b) { return a.get() == b; }

class Node
{
public:
    Node(Node* parent = nullptr) : m_parent(parent), m_sign(true) {}
	virtual ~Node() {}

	virtual void xml_out(XML& xml) const=0;
	virtual std::string toString() const=0;
	virtual void calcSize()=0;
	virtual void calcOrig(int x, int y)=0;
	virtual void asciiArt(Draw& draw) const=0;
	virtual bool drawParenthesis() { return false; }

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
};

enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

class Draw
{
public:
    Draw() {}
	virtual ~Draw() {}

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
class Input;
using InputPtr = std::shared_ptr<Input>;

class Input : public Node
{
public:
    Input(Parser& p, Node* parent = nullptr);
    Input(Equation& eqn, std::string txt, Node* parent, bool neg, bool current);
	~Input() {}

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	void disable() { m_active = false; }
	void setCurrent(bool current) { m_current = current; }
	void addTyped(char c) { m_typed += c; }
	void delTyped() { m_typed.pop_back(); }

	bool handleChar(int ch);

	static NodePtr parse(Parser& p, Node* parent = nullptr);
	static NodePtr xml_in(XMLParser& in, Node* parent);

	friend class Equation;
private:
	std::string m_typed;
	int m_sn;
	bool m_active;
	bool m_current;

	static int input_sn;
};

class Equation
{
public:
	Equation(std::string eq);
    Equation(std::istream& is);
	std::string toString() const { return m_root->toString(); }
	void xml_out(XML& xml) const;
	void xml_out(std::ostream& os) const;
	void xml_out(std::string& str) const;
	void asciiArt(Draw& draw) const;
	NodePtr getRoot() { return m_root; }

	Input* getCurrentInput() { return m_inputs[m_input_index]; }
	void nextInput();
	bool disableCurrentInput();

	void setCurrentInput(int in_sn);
	void addInput(Input* in) { m_inputs.push_back(in); }

private:
	std::vector<Input*> m_inputs;
	int m_input_index = -1;
	NodePtr m_root;

	static NodePtr xml_in(XMLParser& in);
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
