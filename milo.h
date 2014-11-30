#ifndef __MILO_H
#define __MILO_H

#include <string>
#include <memory>
#include <complex>
#include <iostream>
#include <sstream>
#include <vector>

using Complex = std::complex<double>;

bool isZero(double x);
bool isZero(Complex z);

class Draw;
class XML;
class XMLParser;
class Node;
using NodePtr = std::shared_ptr<Node>;

class Node
{
public:
    Node(Node* parent = nullptr) : m_parent(parent), m_sign(true) {}
	virtual ~Node() {}

	virtual void xml_out(XML& xml) const=0;
	virtual std::string toString() const=0;
	virtual void calcTermSize()=0;
	virtual void calcTermOrig(int x, int y)=0;
	virtual void asciiArt(Draw& draw) const=0;
	virtual bool drawParenthesis() { return false; }

	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }

	int getTermSizeX() const { return termSize_x; }
	int getTermSizeY() const { return termSize_y; }
	int getTermOrigX() const { return termOrig_x; }
	int getTermOrigY() const { return termOrig_y; }
	int  getBaseLine() const { return m_base; }

	void setTermSize(int x, int y) { termSize_x = x; termSize_y = y; }
	void setTermOrig(int x, int y) { termOrig_x = x; termOrig_y = y; }
	void setBaseLine(int base) { m_base = base; }

	void setParent(Node* parent) { m_parent = parent; }
	Node* getParent() const { return m_parent; }
private:
	int termSize_x;
	int termSize_y;
	int termOrig_x;
	int termOrig_y;
	bool m_sign;
	Node* m_parent;
	int m_base;
};

class Draw
{
public:
    Draw() {}
	virtual ~Draw() {}

	virtual void parenthesis(int x_size, int y_size, int x0, int y0)=0;
	virtual void at(int x, int y, char c)=0;
	virtual void at(int x, int y, const std::string& s)=0;
	virtual void out()=0;
   
	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
	}

	void parenthesis(const Node* node) { 
		parenthesis(node->getTermSizeX(), node->getTermSizeY(), 
					node->getTermOrigX(), node->getTermOrigY());
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
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	void disable() { m_active = false; }
	void addTyped(char c) { m_typed += c; }
	void delTyped() { m_typed.pop_back(); }

	void handleChar(char ch);

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
	Equation(std::string eq, Draw& draw);
    Equation(std::istream& is, Draw& draw);
	std::string toString() const { return m_root->toString(); }
	void xml_out(XML& xml) const;
	void xml_out(std::ostream& os) const;
	void xml_out(std::string& str) const;
	void asciiArt() const;
	NodePtr getRoot() { return m_root; }

	Input* getCurrentInput() { return m_inputs[m_input_index]; }
	void nextInput() { m_input_index = (++m_input_index)%m_inputs.size(); }

	void setCurrentInput(int in_sn);
	void addInput(Input* in) { m_inputs.push_back(in); }

private:
	std::vector<Input*> m_inputs;
	int m_input_index = -1;

	NodePtr m_root;
	Draw& m_draw;

	static NodePtr xml_in(XMLParser& in);
};

#endif // __MILO_H
