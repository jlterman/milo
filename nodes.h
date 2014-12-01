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
};

class Divide : public Binary
{
public:
	Divide(NodePtr one, NodePtr two, bool fNeg = false, Node* parent = nullptr) : 
		Binary('/', one, two, parent) { if (fNeg) negative(); }
	~Divide() {}

	void xml_out(XML& xml) const { Binary::xml_out("divide", xml); }
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;
};

class Power : public Binary
{
public:
	Power(NodePtr one, NodePtr two, bool fNeg = false, Node* parent = nullptr) : 
		Binary('^', one, two, parent) { if (fNeg) negative(); }
	~Power() {}

	void xml_out(XML& xml) const { Binary::xml_out("power", xml); }
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = std::map<std::string, func_ptr>;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

	std::string toString() const { return "\033[32m" + m_name + "\033[30m" + m_arg->toString(); }
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool drawParenthesis() { return true; }

	Function(const std::string& name, func_ptr fp, NodePtr node, Node* parent, bool neg = false) : 
		Node(parent), m_name(name), m_func(fp), m_arg(node) { if (neg) negative(); }
	Function()=delete;
	~Function() {}
private:
	std::string m_name;
	func_ptr m_func;
	NodePtr m_arg;

	static func_map functions;

	static void init_functions(func_map& fmap);
	static Complex sinZ(Complex z);
	static Complex cosZ(Complex z);
	static Complex tanZ(Complex z);
	static Complex logZ(Complex z);
	static Complex expZ(Complex z);
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
	void calcTermSize();
	void calcTermOrig(int x, int y);
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
	Number(std::string real, std::string imag, Node* parent, bool neg) : 
		Node(parent), m_value(Complex(stod(real), stod(imag))), m_isInteger(isZero(m_value))
	{ 
		if (neg) negative();
	}
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	std::string toString() const;
	void xml_out(XML& xml) const;
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

private:
	std::string getInteger(Parser& p);

	Complex m_value;
	bool m_isInteger;
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
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;
	bool drawParenthesis() { return true; }

private:
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
	void calcTermSize();
	void calcTermOrig(int x, int y);
	void asciiArt(Draw& draw) const;

	bool firstTerm(const NodePtr n) const { return (terms.size() > 1) && (terms[0] == n); }

	static NodePtr getTerm(Parser& p, Node* parent);
	static NodePtr parse(Parser& p, Node* parent);
	static NodePtr xml_in(XMLParser& in, Node* parent);

private:
	std::vector<NodePtr> terms;	

	bool add(Parser& p);
	bool add(XMLParser& in);
};
