#include <vector>
#include <exception>
#include <map>
#include "milo.h"

bool isZero(double x);
bool isZero(Complex z);
bool isInteger(const std::string& n);

class Input : public Node
{
public:
    Input(Parser& p, Node* parent = nullptr);
    Input(Equation& eqn, std::string txt = std::string(), bool current = false, Node* parent = nullptr,
		  bool neg = false, Node::Select s = Node::Select::NONE);
	Input(XMLParser& in, Node* parent);
	~Input() {}

	std::string toString() const;
	void xml_out(XML& xml) const;

	static Node* parse(Parser& p, Node* parent = nullptr);

	static const std::string name;

	friend class Equation;
private:
	std::string m_typed;
	int m_sn;
	bool m_current;
	Equation& m_eqn;
	Frame m_internal;

	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const;

	void setCurrent(bool current) { m_current = current; }

	static int input_sn;
};

class Binary : public Node
{
public:
	Binary(char op, Node* one, Node* two, Node* parent, bool neg, Node::Select s) : 
	    Node(parent, neg, s), m_op(op), m_first(one), m_second(two) {}
	Binary(XMLParser& in, Node* parent, const std::string& name);
	virtual ~Binary() { delete m_first; delete m_second; }

	std::string toString() const { return m_first->toString() + std::string(1, m_op) + m_second->toString(); }
	bool isLeaf() const { return false; }
	Node* findNode(int x, int y);
	int numFactors() const { return m_first->numFactors() + m_second->numFactors(); }
	virtual const std::string& getName() const=0;

	static Node* parse(Parser& p, Node* one, Node* parent);
	void xml_out(XML& xml) const;

protected:
	Node* m_first;  // Binary own this tree
	Node* m_second; // Binary own this tree
	char m_op;
	Frame m_internal;

private:
	Node* downLeft()  { return m_first; }
	Node* downRight() { return m_second; }
	Node* getLeftSibling(Node* node)  { return (m_second == node) ? m_first->last() : nullptr; }
	Node* getRightSibling(Node* node) { return (m_first == node) ? m_second->first() : nullptr; }

};

class Divide : public Binary
{
public:
    Divide(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('/', one, two, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this); 
		m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}
    Divide(XMLParser& in, Node* parent) : Binary(in, parent, name)
	{ 
		m_op = '/'; m_first->setDrawParenthesis(false); m_second->setDrawParenthesis(false);
	}
	~Divide() {}

	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const;
	const std::string& getName() const { return name; }

	static const std::string name;
};

class Power : public Binary
{
public:
    Power(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('^', one, two, parent, neg, s) 
	{ 
		m_first->setParent(this); m_second->setParent(this);
		m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}
    Power(XMLParser& in, Node* parent) : Binary(in, parent, name)
	{ 
		m_op = '^'; m_first->setDrawParenthesis(m_first->numFactors()>1); m_second->setDrawParenthesis(false);
	}
	~Power() {}

	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const;
	const std::string& getName() const { return name; }

	static const std::string name;
};

class Constant : public Node
{
public:
	Constant(Parser& p, Node* parent);
    Constant(char name, Complex value, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name), m_value(value) {}
	Constant(XMLParser& in, Node* parent);
	~Constant() {}

	using const_map = std::map<char, Complex>;

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return constants.at(m_name); }

	static const std::string name;

	static Node* parse(Parser& p, Node* parent);

private:
	char m_name;
	Complex m_value;
	Frame m_internal;
	static const const_map constants;
};

class Variable : public Node
{
public:
	Variable(Parser& p, Node* parent);
    Variable(char name, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name)
    { 
		values.emplace(name, Complex(0, 0)); 
	}
	Variable(XMLParser& in, Node* parent);
	~Variable() {}

	using var_map = std::map<char, Complex>;

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return values[m_name]; }

	static const std::string name;

	static Node* parse(Parser& p, Node* parent);
	static void setValue(char name, Complex value);

private:
	char m_name;
	Frame m_internal;

	static var_map values;
};

class Number : public Node
{
public:
    Number(Parser& p, Node* parent) : Node(parent), m_isInteger(true) { getNumber(p); }
	Number(std::string real, std::string imaginary, Node* parent, 
		   bool neg = false, Node::Select s = Node::Select::NONE) :
	    Node(parent, neg, s), m_value(Complex(stod(real), stod(imaginary))), m_imag_pos(-1),
		m_isInteger(isInteger(real) && isInteger(imaginary)) {}
	Number(XMLParser& in, Node* parent);
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	std::string toString(double value, bool real = true) const;
	std::string toString() const;
	void xml_out(XML& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	Complex getNodeValue() const { return m_value; }

	static const std::string name;

	static Node* parse(Parser& p, Node* parent);

private:
	std::string getInteger(Parser& p);

	Complex m_value;
	bool m_isInteger;
	size_t m_imag_pos;
	std::string m_number;
	Frame m_internal;
};

class Term : public Node
{
public:
    Term(Parser& p, Expression* parent = nullptr) : Node((Node*) parent) { while(add(p)); }
	Term(XMLParser& in, Node* parent = nullptr);
    Term(NodeVector f, Expression* parent) : Node((Node*) parent) { factors.swap(f); }
    Term(Node* node, Expression* parent = nullptr, bool fNeg = false) : 
	    Node((Node*) parent, fNeg), factors(1, node) 
	{
		node->setParent(this);
	}
	~Term() { freeVector(factors); }
	
	static Node* parse(Parser& p, Node* parent = nullptr);

	std::string toString() const;
	void xml_out(XML& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	bool isLeaf() const { return false; }
	bool isFactor() const { return false; }
	Complex getNodeValue() const;
	Node* findNode(int x, int y);
	int numFactors() const;

	static const std::string name;

	int getFactorIndex(Node* node) { return distance(factors.cbegin(), find(factors, node)); }
	void setParent() { for ( auto f : factors ) f->setParent(this); }
	void setParent(Expression* parent) { Node::setParent((Node*) parent); }

	friend class Equation;
	friend class FactorIterator;
private:
	Node* downLeft()  { return factors.front(); }
	Node* downRight() { return factors.back(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

	NodeVector factors; // Term owns this tree
	Frame m_internal;

	bool add(Parser& p);
};

class Expression : public Node
{
public:
	Expression(XMLParser& in, Node* parent = nullptr);

    Expression(Parser& p, Node* parent = nullptr) : Node(parent)
	{
		while(add(p)); setDrawParenthesis(true);
	}

    Expression(Term* term, Node* parent = nullptr) : Node(parent), terms(1, term)
	{ 
		term->setParent(this); setDrawParenthesis(true);
	}

    Expression(Node* factor, Node* parent = nullptr) : 
	    Node(parent), terms(1, new Term(factor, this))
	{ 
		factor->setParent(terms[0]); setDrawParenthesis(true);
	}
	~Expression() { freeVector(terms); }

	std::string toString() const;
	void xml_out(XML& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	bool isLeaf() const { return false; }
	Complex getNodeValue() const;
	Node* findNode(int x, int y);
	int numFactors() const;

	static const std::string name;

	int getTermIndex(Term* term) { return distance(terms.cbegin(), find(terms, term)); }
	void setParent() { for ( auto t : terms ) t->setParent(this); }

	static Term* getTerm(Equation& eqn, std::string text, Expression* parent = nullptr);
	static Term* getTerm(Parser& p, Expression* parent);
	static Node* parse(Parser& p, Node* parent);

	friend class Equation;
	friend class FactorIterator;

	using TermVector = std::vector<Term*>;
private:
	Node* downLeft()  { return terms.front(); } 
	Node* downRight() { return terms.back(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

	TermVector terms;	// Expression owns this tree
	Frame m_internal;

	bool add(Parser& p);
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = std::map<std::string, func_ptr>;

	static Node* parse(Parser& p, Node* parent);

	std::string toString() const { return m_name + m_arg->toString(); }
	void xml_out(XML& xml) const;
	Frame calcSize(Graphics& gc);
	void calcOrig(Graphics& gc, int x, int y);
	void drawNode(Graphics& gc) const;
	bool isLeaf() const { return false; }
	Complex getNodeValue() const;
	Node* findNode(int x, int y);
	int numFactors() const { return m_arg->numFactors() + 1; }

	static const std::string name;

    Function(const std::string& name, func_ptr fp, Parser& p, Node* parent, 
			 bool neg = false, Node::Select s = Node::Select::NONE) : 
	             Node(parent, neg, s), m_name(name), m_func(fp), m_arg(new Expression(p, this)) {}

	Function(XMLParser& in, Node* parent);
	~Function() { delete m_arg; }

private:
	Node* downLeft()  { return m_arg->first(); }
	Node* downRight() { return m_arg->last(); }

	Node* m_arg;       // Function own this tree
	std::string m_name;
	func_ptr m_func;
	Frame m_internal;

	static const func_map functions;

	static Complex sinZ(Complex z);
	static Complex cosZ(Complex z);
	static Complex tanZ(Complex z);
	static Complex logZ(Complex z);
	static Complex expZ(Complex z);
};
