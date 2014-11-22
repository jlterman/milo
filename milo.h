#include <string>
#include <memory>
#include <complex>
#include <ostream>

using Complex = std::complex<double>;

bool isZero(double x);
bool isZero(Complex z);

class Node;
using NodePtr = std::shared_ptr<Node>;

class Node
{
public:
	Node() : m_sign(true) {}
	~Node() {}

	virtual std::ostream& xml_out(std::ostream& os) const=0;
	virtual std::string toString()=0;

	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }
private:
	bool m_sign;
};

class Equation
{
public:
	Equation(std::string eq);

	std::string toString()  { return m_root->toString(); }
	
	void xml_out(std::ostream& os);
	static void indent(std::ostream& os);
	static void inc_indent() { xml_indent += 2; }
	static void dec_indent() { xml_indent -= 2; }
private:
	NodePtr m_root;

	static int xml_indent;
	static void clear_indent() { xml_indent = 0; }
};

std::ostream& operator<<(std::ostream& os, const NodePtr& node) { return node->xml_out(os); }

