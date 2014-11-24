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
	virtual std::string toString() const=0;
	virtual void calcTermSize()=0;
	virtual void calcTermOrig(int x, int y)=0;
	virtual char asciiArt(int x, int y) const=0;
	char asciiArtParenthesis(bool right, int y) const;

	void negative() { m_sign = !m_sign; }
	bool getSign() const { return m_sign; }

	int getTermSizeX() const { return termSize_x; }
	int getTermSizeY() const { return termSize_y; }
	int getTermOrigX() const { return termOrig_x; }
	int getTermOrigY() const { return termOrig_y; }
	bool insideNodeTerm(int x, int y) const;

	void setTermSize(int x, int y) { termSize_x = x; termSize_y = y; }
	void setTermOrig(int x, int y) { termOrig_x = x; termOrig_y = y; }

private:
	int termSize_x;
	int termSize_y;
	int termOrig_x;
	int termOrig_y;
	bool m_sign;
};

using NodePtr = std::shared_ptr<Node>;

class Equation
{
public:
	Equation(std::string eq);

	std::string toString() const { return m_root->toString(); }
	
	void xml_out(XML& xml) const;

	void asciiArt(std::ostream& os) const;
private:
	NodePtr m_root;
};

