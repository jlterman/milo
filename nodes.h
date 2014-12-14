#include <vector>
#include <exception>
#include <map>
#include "milo.h"

bool isZero(double x);
bool isZero(Complex z);
bool isInteger(const std::string& n);

class Expression;

class Input : public Node
{
public:
    Input(Parser& p, Node* parent = nullptr);
    Input(Equation& eqn, std::string txt = std::string(), bool current = false, Node* parent = nullptr,
		  bool neg = false, Node::Select s = Node::Select::NONE);
	~Input() {}

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	Complex getNodeValue() const;

	static Node* parse(Parser& p, Node* parent = nullptr);
	static Node* xml_in(XMLParser& in, Node* parent);

	friend class Equation;
private:
	std::string m_typed;
	int m_sn;
	bool m_active;
	bool m_current;
	Equation& m_eqn;

	void disable() { m_active = false; }
	void setCurrent(bool current) { m_current = current; }

	static int input_sn;
};

class Binary : public Node
{
public:
	Binary(char op, Node* one, Node* two, Node* parent, bool neg, Node::Select s) : 
	    Node(parent, neg, s), m_op(op), m_first(one), m_second(two) {}

	virtual ~Binary() { delete m_first; delete m_second; }

	std::string toString() const { return m_first->toString() + m_op + m_second->toString(); }
	bool isLeaf() const { return false; }

	static Node* parse(Parser& p, Node* one, Node* parent);
	static Node* xml_in(XMLParser& in, char op, const std::string& name, Node* parent);
	void xml_out(const std::string& tag, XML& xml) const;

protected:
	Node* m_first;  // Binary own this tree
	Node* m_second; // Binary own this tree
	char m_op;

private:
	Node* downLeft()  { return m_first; }
	Node* downRight() { return m_second; }
	Node* getLeftSibling(Node* node)  { return (m_second == node) ? m_first : nullptr; }
	Node* getRightSibling(Node* node) { return (m_first == node) ? m_second : nullptr; }

};

class Divide : public Binary
{
public:
    Divide(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('/', one, two, parent, neg, s) 
	{ 
		one->setParent(this); two->setParent(this); 
	}
	~Divide() {}

	void xml_out(XML& xml) const { Binary::xml_out("divide", xml); }
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	Complex getNodeValue() const;
};

class Power : public Binary
{
public:
    Power(Node* one, Node* two, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Binary('^', one, two, parent, neg, s) 
	{ 
		one->setParent(this); two->setParent(this);
	}
	~Power() {}

	void xml_out(XML& xml) const { Binary::xml_out("power", xml); }
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	Complex getNodeValue() const;
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = std::map<std::string, func_ptr>;

	static Node* parse(Parser& p, Node* parent);
	static Node* xml_in(XMLParser& in, Node* parent);

	std::string toString() const { return m_name + m_arg->toString(); }
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool drawParenthesis() const { return true; }
	bool isLeaf() const { return false; }
	Complex getNodeValue() const;

	Function(const std::string& name, func_ptr fp, Node* node, 
			 Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
    	Node(parent, neg, s), m_name(name), m_func(fp), m_arg(node) 
	{ 
		m_arg->setParent(this); 
	}

	Function()=delete;
	~Function() { delete m_arg; }

private:
	Node* downLeft()  { return m_arg; }
	Node* downRight() { return m_arg; }

private:
	Node* m_arg;       // Function own this tree
	std::string m_name;
	func_ptr m_func;

	static func_map functions;

	static void init_functions();
	static Complex sinZ(Complex z);
	static Complex cosZ(Complex z);
	static Complex tanZ(Complex z);
	static Complex logZ(Complex z);
	static Complex expZ(Complex z);
};

class Constant : public Node
{
public:
	Constant(Parser& p, Node* parent);
    Constant(char name, Complex value, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	    Node(parent, neg, s), m_name(name), m_value(value) {}
	~Constant() {}

	using const_map = std::map<char, Complex>;

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	Complex getNodeValue() const { return constants[m_name]; }

	static Node* parse(Parser& p, Node* parent);
	static Node* xml_in(XMLParser& in, Node* parent);

private:
	char m_name;
	Complex m_value;
	static const_map constants;

	static void init_constants();
};

class Variable : public Node
{
public:
	Variable(Parser& p, Node* parent);
    Variable(char name, Node* parent, bool neg = false, Node::Select s = Node::Select::NONE) : 
	Node(parent, neg, s), m_name(name) { values.emplace(name, Complex(0, 0)); }
	~Variable() {}

	using var_map = std::map<char, Complex>;

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	Complex getNodeValue() const { return values[m_name]; }

	static Node* parse(Parser& p, Node* parent);
	static Node* xml_in(XMLParser& in, Node* parent);
	static void setValue(char name, Complex value);

private:
	char m_name;

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
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	std::string toString(double value, bool real = true) const;
	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	Complex getNodeValue() const { return m_value; }

	static Node* parse(Parser& p, Node* parent);
	static Node* xml_in(XMLParser& in, Node* parent);

private:
	std::string getInteger(Parser& p);

	Complex m_value;
	bool m_isInteger;
	size_t m_imag_pos;
};

class Term : public Node
{
public:
    Term(Parser& p, Node* parent = nullptr) : Node(parent) { while(add(p)); }
	Term(XMLParser& in, Node* parent = nullptr);
    Term(NodeVector f, Node* parent) : Node(parent) { factors.swap(f); }
    Term(Node* node, Node* parent = nullptr, bool fNeg = false) : Node(parent, fNeg), factors(1, node) {}
	~Term() { freeVector(factors); }
	
	static Node* parse(Parser& p, Node* parent = nullptr);
	static Node* xml_in(XMLParser& in, Node* parent);

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool drawParenthesis() const { return true; }
	bool isLeaf() const { return false; }
	bool isFactor() const { return false; }
	Complex getNodeValue() const;

	Expression* getParentExpression();
	int getFactorIndex(Node* node) { return distance(factors.cbegin(), find(factors, node)); }

	friend class Equation;
private:
	Node* downLeft()  { return factors.front(); }
	Node* downRight() { return factors.back(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

	NodeVector factors; // Term owns this tree

	bool add(Parser& p);
	bool add(XMLParser& in);
};

class Expression : public Node
{
public:
	Expression(Parser& p, Node* parent = nullptr) : Node(parent) { while(add(p)); }
	Expression(XMLParser& in, Node* parent = nullptr);
    Expression(Term* term, Node* parent = nullptr) : Node(parent), terms(1, term) {}
	~Expression() { freeVector(terms); }

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool isLeaf() const { return false; }
	Complex getNodeValue() const;

	int getTermIndex(Node* term) { return distance(terms.cbegin(), find(terms, term)); }

	static Node* getTerm(Equation& eqn, std::string text, Node* parent);
	static Node* getTerm(Parser& p, Node* parent);
	static Node* parse(Parser& p, Node* parent);
	static Node* xml_in(XMLParser& in, Node* parent);

	friend class Equation;
protected:
	Node* downLeft()  { return terms.front(); } 
	Node* downRight() { return terms.back(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

private:
	NodeVector terms;	// Expression owns this tree

	bool add(Parser& p);
	bool add(XMLParser& in);
};
