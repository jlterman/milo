#include <string>
#include <memory>
#include <complex>
#include <ostream>
#include <vector>

using Complex = std::complex<double>;

bool isZero(double x);
bool isZero(Complex z);

class DrawText;
class XML;
class Node;
using NodePtr = std::shared_ptr<Node>;

class Node
{
public:
    Node(NodePtr parent = nullptr) : m_parent(parent), m_sign(true) {}
	virtual ~Node() {}

	virtual void xml_out(XML& xml) const=0;
	virtual std::string toString() const=0;
	virtual void calcTermSize()=0;
	virtual void calcTermOrig(int x, int y)=0;
	virtual void asciiArt(DrawText& draw) const=0;
	void asciiArtParenthesis(DrawText& draw) const;

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

	void setParent(NodePtr parent) { m_parent = parent; }
	NodePtr getParent() const { return m_parent; }
private:
	int termSize_x;
	int termSize_y;
	int termOrig_x;
	int termOrig_y;
	bool m_sign;
	NodePtr m_parent;
	int m_base;
};

class Equation
{
public:
	Equation(std::string eq, DrawText& draw);

	std::string toString() const { return m_root->toString(); }
	
	void xml_out(XML& xml) const;

	void asciiArt() const;

	NodePtr getRoot() { return m_root; }
private:
	NodePtr m_root;
	DrawText& m_draw;
};

class DrawText
{
public:
    DrawText() {}
	virtual ~DrawText() {}

	virtual void at(int x, int y, char c)=0;
	virtual void out()=0;
   
	virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
		m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
	}
protected:
	int m_xSize;
	int m_ySize;
	int m_xOrig;
	int m_yOrig;
};

class XML
{
public:
    XML(std::ostream& os) : 
	m_os(os), m_indent(2) { m_os << "<document>" << std::endl; }
	~XML() { m_os << "</document>" << std::endl; }

	void header(const std::string& tag, bool atomic, 
				std::initializer_list<std::string>);

	void header(const std::string& tag, bool atomic = false);

	void footer(const std::string& tag);
private:
	std::vector<std::string> m_tags;
	int m_indent;
	std::ostream& m_os;

	void print_indent(std::ostream& os) {
		for (int i = 0; i < m_indent; ++i) m_os << ' ';
	}
};
