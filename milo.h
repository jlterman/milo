#include <string>
#include <memory>
#include <complex>
#include <ostream>

using Complex = std::complex<double>;

bool isZero(double x);
bool isZero(Complex z);

class XML;

class Node
{
public:
	Node() : m_sign(true) {}
	~Node() {}

	virtual void xml_out(XML& xml) const=0;
	virtual std::string toString()=0;
	virtual void calcTermSize()=0;

	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }
	int getTermSizeX() { return termSize_x; }
	int getTermSizeY() { return termSize_y; }

protected:
	int termSize_x;
	int termSize_y;

private:
	bool m_sign;
};

using NodePtr = std::shared_ptr<Node>;

class Equation
{
public:
	Equation(std::string eq);

	std::string toString()  { return m_root->toString(); }
	
	void xml_out(XML& xml);
	static void indent(std::ostream& os);
	static void inc_indent() { xml_indent += 2; }
	static void dec_indent() { xml_indent -= 2; }
private:
	NodePtr m_root;

	static int xml_indent;
	static void clear_indent() { xml_indent = 0; }
};

