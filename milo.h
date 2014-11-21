#include <string>
#include <memory>
#include <complex>

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

	virtual std::string toString()=0;

	void negative() { m_sign = !m_sign; }
	bool getSign() { return m_sign; }
private:
	bool m_sign;
};

class Equation
{
public:
	Equation(std::string eq);

	std::string toString()  { return m_root->toString(); }
private:
	NodePtr m_root;
};

