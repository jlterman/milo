#include <iostream>
#include <vector>
#include <iterator>
#include <exception>
#include <map>
#include <limits>
#include "milo.h"

using namespace std;

class Parser
{
public:
	Parser(string expr) : m_expr(expr), m_pos(0) {}

	char peek() { return m_expr[m_pos]; }
	char next();
	bool match(const string& s);
private:
	string m_expr;
	size_t m_pos;
};

class Binary : public Node
{
public:
	Binary(char op, NodePtr one, NodePtr two) : 
		Node(), m_op(op), m_first(one), m_second(two) {}

	~Binary() {}

	string toString() { return m_first->toString() + m_op + m_second->toString(); }

	static NodePtr parse(Parser& p, NodePtr one);

private:
	NodePtr m_first;
	NodePtr m_second;
	char m_op;
};

class Divide : public Binary
{
public:
	Divide(NodePtr one, NodePtr two) : Binary('/', one, two) {}
	~Divide() {}
};

class Power : public Binary
{
public:
	Power(NodePtr one, NodePtr two) : Binary('^', one, two) {}
	~Power() {}
};

class Function : public Node
{
public:
	typedef Complex (*func_ptr)(Complex);
	using func_map = map<string, func_ptr>;

	static NodePtr parse(Parser& p);
	string toString() { return "\033[32m" + m_name + "\033[30m" + m_arg->toString(); }

	Function(const string& name, func_ptr fp, NodePtr node) : 
		Node(), m_name(name), m_func(fp), m_arg(node) {}
	Function()=delete;
	~Function() {}
private:
	string m_name;
	func_ptr m_func;
	NodePtr m_arg;

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
	Variable(Parser& p) : Node(), m_name( p.next() ) {}
	~Variable() {}

	string toString() { return string(string("") + m_name); }

	static NodePtr parse(Parser& p);

private:
	char m_name;
};

class Number : public Node
{
public:
	Number(Parser& p) : Node(), m_isInteger(true) { getNumber(p); }
	~Number() {}

	double getReal(Parser& p);
	void getNumber(Parser& p);

	string toString();

	static NodePtr parse(Parser& p);

private:
	string getInteger(Parser& p);

	Complex m_value;
	bool m_isInteger;
};

class Term : public Node
{
public:
	Term(Parser& p) : Node() { while(add(p)); }

	bool add(Parser& p);
	
	static NodePtr parse(Parser& p);

	string toString();

private:
	vector<NodePtr> factors;
};

class Expression : public Node
{
public:
	Expression(Parser& p) : Node() { while(add(p));	}
	~Expression() {}

	bool add(Parser& p);

	string toString();

	static NodePtr parse(Parser& p);

private:
	vector<NodePtr> terms;	
};

Equation::Equation(string eq) { 
	Parser p(eq); 
	m_root = NodePtr(new Expression(p));
}

bool isZero(double x) {
	return abs(x)<1e-10;
}

bool isZero(Complex z) {
	return isZero(z.real()) && isZero(z.imag());
}

Complex Function::sinZ(Complex z) {
	double x = z.real(), y = z.imag();
	return Complex(sin(x)*cosh(y), cos(x)*sinh(y));
}

Complex Function::cosZ(Complex z) {
	double x = z.real(), y = z.imag();
	return Complex(cos(x)*cosh(y), -sin(x)*sinh(y));
}

Complex Function::tanZ(Complex z) {
	double x = z.real(), y = z.imag();
	double r = cos(x)*cos(x)*cosh(y)*cosh(y) + 
               sin(x)*sin(x)*sinh(y)*sinh(y);
	return Complex(sin(x)*cos(x)/r, sinh(y)*cosh(y)/r);
}

Complex Function::logZ(Complex z) {
	if (isZero(z)) {
		return Complex(-1*numeric_limits<float>::infinity(), 0);
	}
	double x = abs(z.real()), y = abs(z.imag());
	bool neg_x = signbit(z.real()), neg_y = signbit(z.imag());
	if (isZero(x)) return Complex(log(y), (neg_y ? -1 : 1)*M_PI/2);
	if (isZero(y)) return Complex(log(x),  neg_x ? -M_PI : 0);

	return Complex(log(x*x + y*y)/2, 
				   atan(y/x)*((neg_x == neg_y) ? 1 : -1));
}

Complex Function::expZ(Complex z) {
	double x = z.real(), y = z.imag();
	return Complex(exp(x)*cos(y), exp(x)*sin(y));
}

void Function::init_functions(func_map& fmap) {
	fmap.emplace("sin", &sinZ);
	fmap.emplace("cos", &cosZ);
	fmap.emplace("tan", &tanZ);
	fmap.emplace("log", &logZ);
	fmap.emplace("exp", &expZ);
}

NodePtr Function::parse(Parser& p) {
	static func_map functions;
	if (functions.empty()) { init_functions(functions);	}

	if (!isalpha(p.peek()) ) return nullptr;

	for ( auto m : functions ) { 
		if (p.match(m.first + "(")) {
			NodePtr node = NodePtr(new Expression(p));
			if (!node) throw logic_error("bad format");

			return NodePtr(new Function(m.first, m.second, node));
		}
	}
	return nullptr;
}

NodePtr Binary::parse(Parser& p, NodePtr one)
{
	char c = p.peek();
	if (c != '/' && c != '^') return one;

	c = p.next();
	NodePtr two = Term::parse(p);
	if (!two) throw logic_error("bad format");

	return ( c == '/' ) ? NodePtr(new Divide(one, two)) : 
		                  NodePtr(new Power(one, two));
}

char Parser::next()
{
	if (m_pos < m_expr.length()) return m_expr[m_pos++];
	                        else return '\0';
}

bool Parser::match(const string& s)
{
	size_t pos = m_expr.find(s, m_pos); 
	if ( pos != string::npos && pos == m_pos ) {
		m_pos += s.length();
		return true;
	}
	else
		return false;
}

NodePtr Variable::parse(Parser& p)
{
	char c = p.peek();
	if ( isalpha(c) && c != 'i' && c != 'e' ) {
		return NodePtr(new Variable(p));
	}
	else
		return nullptr;
}

NodePtr Number::parse(Parser& p)
{
	char c = p.peek();
	if (isdigit(c) || c == 'i' ) {
		return NodePtr(new Number(p));
	}
	else
		return nullptr;
}

string Number::getInteger(Parser& p)
{
	string n;
	while (isdigit(p.peek())) { n += p.next(); }
	return n;
}

double Number::getReal(Parser& p)
{
	string n = getInteger(p);
	if (p.peek() != '.' && p.peek() != 'E') {
		return stod(n);
	}
	char c = p.next();
	if (c == '.' && isdigit(p.peek())) {
		n += "." +  getInteger(p);
		if (p.peek() == 'E') c = p.next();
		                else c = '\0';
	}
	if (c == 'E') {
		n += 'E';
		if (p.peek() == '-' || p.peek() == '+') n += p.next();

		if (!isdigit(p.peek())) throw logic_error("bad format");
		n += getInteger(p);
	}
	m_isInteger = false;
	return stod(n);
}

void Number::getNumber(Parser& p)
{
	if (p.peek() == 'i') {
		char c = p.next();
		m_value = isdigit(p.peek()) ? Complex(0, getReal(p)) : Complex(0, 1);
	}
	else {
		m_value = Complex(getReal(p), 0);
	}
}

string Number::toString()
{
	double realValue = abs(m_value.real());
	bool realNeg = signbit(m_value.real());
	double imagValue = abs(m_value.imag());
	bool imagNeg = signbit(m_value.imag());

	if (isZero(realValue) && isZero(imagValue)) return "0";

	string result = "";
	if (m_isInteger && !isZero(realValue)) 
		result += std::to_string((int) (realValue + 0.2));

	if (!m_isInteger && !isZero(realValue)) 
		result += std::to_string(realValue);

	if (m_isInteger && !isZero(imagValue)) {
		if (!isZero(realValue)) result += realNeg ? "-" : "+";
		result += "\033[31mi\033[30m" + std::to_string((int) (imagValue + 0.2));
	}
	if (!m_isInteger && !isZero(imagValue)) {
		if (!isZero(realValue)) result += realNeg ? "-" : "+";
		result += "\033[31mi\033[30m" + std::to_string(imagValue);
	}
	return result;
}

NodePtr Term::parse(Parser& p)
{
	char c = p.peek();
	if ( c=='\0' || c=='+' || c=='-' || c==')' ) return false;

	NodePtr    node = Expression::parse(p);
	if (!node) node = Function::parse(p);
	if (!node) node = Variable::parse(p);
	if (!node) node = Number::parse(p);
	if (!node) return nullptr;

	node = Binary::parse(p, node);
	return node;
}

bool Term::add(Parser& p)
{
	NodePtr node = parse(p);
	if (!node) return false;

	factors.push_back(node);
	return true;
}

bool Expression::add(Parser& p)
{
	bool neg = false;
	if (p.peek() == '+' || p.peek() == '-') {
		char c = p.next();
		neg = (c == '-') ? true : false;
	}
	NodePtr term = NodePtr( new Term(p) );
	if (neg) term->negative();
	terms.push_back(term);
	return ( p.peek() != '\0' && p.peek() != ')' );
}

NodePtr Expression::parse(Parser& p) {
	if (p.peek() != '(') return nullptr;
	char c = p.next();
	NodePtr node = NodePtr(new Expression(p));
	c = p.next();
	if ( c != ')' ) throw logic_error("bad format");
	return node;
}


string Term::toString()
{
	string s = "";
	for ( auto f : factors ) { 
		s += ( f->getSign() ? "" : "(-" ) + f->toString() + ( f->getSign() ? "" : ")" );
	}
	return s; 	
}

string Expression::toString() { 
	string s = "(";
	for ( auto t : terms ) { 
		s += t->getSign() ? '+' : '-'; 
		s += t->toString();
	}
	return s += ")";
}

int main(int argc, char* argv[])
{
	Equation eqn(argv[1]);
	cout << eqn.toString() << endl;
}
