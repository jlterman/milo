#ifndef __PARSER_H
#define __PARSER_H

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stack>

class Parser;
class Node;
class Equation;

class XMLParser
{
public:
	enum { CLEAR=0, HEADER=1, FOOTER=2, ATOM=4 };

	XMLParser(std::istream& in);
	virtual ~XMLParser();

	void next();
	bool next(int state, const std::string& tag = std::string()) { next(); return getState(state, tag); }
	bool getState(int state, const std::string& tag = std::string()) const;
	const std::string& getTag() const { return m_tag; }
	bool getAttribute(const std::string& name, std::string& value);
	bool hasAttributes() { return m_attributes.size() != 0; }

private:
	std::vector<std::string> m_tags;
	size_t m_pos;
	std::map<std::string, std::string> m_attributes;
	int m_state;
	std::string m_tag;

	bool EOL() const { return m_pos == m_tags.size(); }
	void parse(std::istream& in);
};

class EqnXMLParser : public XMLParser
{
public:
	typedef Node* (*createPtr)(EqnXMLParser&, Node*);

    EqnXMLParser(std::istream& in, Equation& eqn) : XMLParser(in), m_eqn(eqn) {}
	~EqnXMLParser() {}

	Equation& getEqn() const { return m_eqn; }
	Node* getFactor(Node* parent);
	
private:
	Equation& m_eqn;

	static const std::map<std::string, createPtr> create_factors;
};

namespace XML
{
	class Stream
	{
	public:
		enum State { END, HEADER_START, NAME, VALUE, HEADER_END, ATOM_END, FOOTER };

		Stream(std::ostream& os, int step = 2, std::string sep = "\n");
		~Stream();

		friend Stream& operator<<(Stream& xml, State state);
		friend Stream& operator<<(Stream& xml, const std::string& tag);
	private:
		std::ostream& m_os;
		const int m_indent_step;
		int m_indent;
		std::string m_sep;
		State m_state;
		std::stack<std::string> tags;
	};
}

#endif // __PARSER_H
