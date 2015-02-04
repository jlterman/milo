#ifndef __PARSER_H
#define __PARSER_H

#include <iostream>
#include <vector>
#include <map>
#include <string>

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

#endif // __PARSER_H
