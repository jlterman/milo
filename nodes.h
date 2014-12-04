#include <vector>
#include <exception>
#include <map>
#include "milo.h"

class Binary : public Node
{
public:
	Binary(char op, NodePtr one, NodePtr two, Node* parent) : 
		Node(parent), m_op(op), m_first(one), m_second(two) {}

	virtual ~Binary() {}

	std::string toString() const { return m_first->toString() + m_op + m_second->toString(); }

	static NodePtr parse(Parser& p, NodePtr one, Node* parent);
	static NodePtr xml_in(XMLParser& in, char op, const std::string& name, Node* parent);
	void xml_out(const std::string& tag, XML& xml) const;

protected:
	NodePtr m_first;
	NodePtr m_second;
	char m_op;

private:
	Node* downLeft() { return m_first.get(); }
	Node* downRight() { return m_second.get(); }
	Node* getLeftSibling(Node* node)  { return (m_second == node) ? m_first.get() : nullptr; }
	Node* getRightSibling(Node* node) { return (m_first == node) ? m_second.get() : nullptr; }

};

class Divide : public Binary
{
public:
    Divide(NodePtr one, NodePtr two, Node* parent, bool neg = false) : 
	    Binary('/', one, two, parent) 
	{ 
		one->setParent(this); two->setParent(this); 
	}
	~Divide() {}

	void xml_out(XML& xml) const { Binary::xml_out("divide", xml); }
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
};

class Power : public Binary
{
public:
    Power(NodePtr one, NodePtr two, Node* parent, bool neg = false) : 
		Binary('^', one, two, parent) 
	{ 
		if (neg) negative(); one->setParent(this); two->setParent(this);
	}
	~Power() {}

	void xml_out(XML& xml) const { Binary::xml_out("power", xml); }
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = std::map<std::string, func_ptr>;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

	std::string toString() const { return m_name + m_arg->toString(); }
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool drawParenthesis() { return true; }

	Function(const std::string& name, func_ptr fp, NodePtr node, Node* parent, bool neg = false) : 
	    Node(parent), m_name(name), m_func(fp), m_arg(node) 
	{ 
		if (neg) negative();
		m_arg->setParent(this); 
	}

	Function()=delete;
	~Function() {}

private:
	Node* downLeft()  { return m_arg.get(); }
	Node* downRight() { return m_arg.get(); }

private:
	std::string m_name;
	func_ptr m_func;
	NodePtr m_arg;

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
    Constant(char name, Complex value, Node* parent) : Node(parent), m_name(name), m_value(value) {}
	~Constant() {}

	using const_map = std::map<char, Complex>;

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

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
	Variable(char name, Node* parent, bool neg) : 
		Node(parent), m_name(name) { if (neg) negative(); }
	~Variable() {}

	std::string toString() const { return std::string() + m_name; }
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

private:
	char m_name;
};

class Number : public Node
{
public:
	Number(Parser& p, Node* parent) : Node(parent), m_isInteger(true) { getNumber(p); }
	Number(std::string real, std::string imaginary, Node* parent, bool neg) : 
	    Node(parent), m_value(Complex(stod(real), stod(imaginary))), m_imag_pos(-1),
		m_isInteger(isInteger(real) && isInteger(imaginary))
	{ 
		if (neg) negative();
	}
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	std::string toString(double value, bool real = true) const;
	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

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
	
	static NodePtr parse(Parser& p, Node* parent = nullptr);
	static NodePtr xml_in(XMLParser& in, Node* parent);

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool drawParenthesis() { return true; }

private:
	Node* downLeft()  { return factors.front().get(); }
	Node* downRight() { return factors.back().get(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

	std::vector<NodePtr> factors;

	bool add(Parser& p);
	bool add(XMLParser& in);
};

class Expression : public Node
{
public:
	Expression(Parser& p, Node* parent = nullptr) : Node(parent) { while(add(p)); }
	Expression(XMLParser& in, Node* parent = nullptr);
	~Expression() {}

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcSize();
	void calcOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	bool firstTerm(const NodePtr n) const { return (terms.size() > 1) && (terms[0] == n); }

	static NodePtr getTerm(Parser& p, Node* parent);
	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

protected:
	Node* downLeft()  { return terms.front().get(); }
	Node* downRight() { return terms.back().get(); }
	Node* getLeftSibling(Node* node);
	Node* getRightSibling(Node* node);

private:
	std::vector<NodePtr> terms;	

	bool add(Parser& p);
	bool add(XMLParser& in);
};
