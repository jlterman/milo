#include "xml.h"

using namespace std;

namespace XML
{
	void FSM::advance(State new_state)
	{
		for ( auto t : transitions ) {
			if ((t.at(0)&m_state) != 0 && (t.at(1)&new_state) != 0) {
				m_state = new_state;
				return;
			}
		}
		m_state = ILLEGAL;
		return;
	}

	void FSM::next(State new_state, const std::string& tag)
	{
		advance(new_state); if (m_state == ILLEGAL) return;
		if (!tag.empty() && (new_state&(HEADER|FOOTER)) == 0) {
			m_state = ILLEGAL; return;
		}
		if (!tag.empty() && (new_state&HEADER) != 0) {
			m_tags.push(tag);
		}
		else if (!tag.empty() && (new_state&FOOTER) != 0) {
			if (m_tags.top() != tag) m_state = ILLEGAL;
			m_tags.pop();
		}
		else if ((new_state&(FOOTER|ATOM_END)) != 0) {
			m_tags.pop();
		}
		return;
	}

	string to_string(State state)
	{
		static const map<State, string> state_strings = {
			{ END,        "END"        },
			{ HEADER,     "HEADER"     }, 
			{ HEADER_END, "HEADER_END" }, 
			{ FOOTER,     "FOOTER"     }, 
			{ FOOTER_END, "FOOTER_END" }, 
			{ ATOM_END,   "ATOM_END"   }, 
			{ NAME_VALUE, "NAME_VALUE" }, 
			{ ELEMENT,    "ELEMENT"    }, 
			{ NEW,        "NEW"        }, 
			{ ILLEGAL,    "ILLEGAL"    },
		};
		return state_strings.at(state);
	}

	void Stream::out(State state)
	{
		switch (state) {
			if (m_pending != NONE) state = ILLEGAL;

		    case NAME_VALUE:
				m_pending = NAME;
				break;
		    case ELEMENT:
				m_pending = ELEMENT_TAG;
				break;
		    case HEADER:
				m_pending = HEADER_TAG;
				break;
	        case HEADER_END: {
				m_os << ">"; 
				m_indent += m_indent_step; 
				fsm.next(state);
				break;
			}
		    case ATOM_END: {
				m_os << "/>";
				fsm.next(state);
				break;
			}
	        case FOOTER: {
				m_indent -= m_indent_step; 
				if (fsm.getState() != ELEMENT) {
					m_os << m_sep;
					if (m_indent > 0) m_os << string(m_indent, ' ');
				}
				if (fsm.finished()) throw logic_error("Too many footers");
				m_os << "</" << fsm.getTag() << ">";
				fsm.next(FOOTER);
				fsm.next(FOOTER_END);
				break;
			}
		    case FINISH: {
				while (!fsm.finished()) out(FOOTER);
				break;
			}
		    case FOOTER_END:
				if (fsm.getState() != FOOTER_END) fsm.next(state);
				break;
		    default: {
				throw logic_error("Bad xml state: " + to_string(state));
				break;
			}
		}
		if (fsm.getState() == ILLEGAL) 
			throw logic_error("Bad next xml state: " + to_string(state));
	}

	void Stream::out(const string& tag)
	{
		string tag_str = tag;
		switch (m_pending) {
		    case HEADER_TAG:
				m_pending = NONE;
				if (fsm.getState() != NEW) m_os << m_sep;
				if (m_indent > 0) m_os << string(m_indent, ' ');
				m_os << "<" << tag;
				fsm.next(HEADER, tag);
				break;
		    case ELEMENT_TAG:
				m_pending = NONE;
				escape_tag(tag_str);
				m_os << tag_str;
				fsm.next(ELEMENT);
				break;
		    case NAME:
				m_pending = VALUE;
				m_nv = tag;
				break;
		    case VALUE:
				m_pending = NONE;
				escape_tag(tag_str);
				m_os << " " << m_nv << "=\"" << tag_str << "\"";
				fsm.next(NAME_VALUE);
				break;
		    default: {
				throw logic_error("Bad pending xml tag: " + tag);
				break;
			}
		}
	}

	Stream& operator<<(Stream& xml, State state)
	{
		xml.out(state);
		return xml;
	}

	Stream& operator<<(Stream& xml, const string& tag)
	{
		xml.out(tag);
		return xml;
	}


	Stream::Stream(ostream& os, int step, string sep) : 
		m_sep(sep), m_os(os), m_indent_step(step)
	{
		*this << HEADER << "document" << HEADER_END;
	}

	Stream::~Stream()
	{ 
		*this << FINISH;
	}

	void escape_tag(string& tag)
	{
		const map<char, string> escape = {
			{ '<',  "&lt;"   },
			{ '>',  "&gt;"   },
			{ '"',  "&quot;" },
			{ '\'', "&apos;" },
		};
		size_t pos = 0;
		while ((pos = tag.find("&", pos)) != string::npos) {
			tag.replace(pos, 1, "&amp;"); ++pos;
		}

		while ((pos = tag.find_first_of("<>\"'")) != string::npos) {
			tag.replace(pos, 1, escape.at(tag.at(pos)));
		}
	}
	
	void unescape_tag(string& tag)
	{
		const map<string, string> unescape = {
			{ "&lt;",   "<", },
			{ "&gt;",   ">", },
			{ "&quot;", "\"" },
			{ "&apos;", "'"  },
			{ "&amp;",  "&"  }
		};
		for ( auto& u : unescape ) {
			size_t pos;
			while ((pos = tag.find(u.first)) != string::npos) {
				tag.replace(pos, u.first.length(), u.second);
			}
		}
	}

	Parser::Parser(istream& in) : m_pos(0)
	{ 
		tokenize(in);  
		next(HEADER, "document").next(HEADER_END);
	}
	
	void Parser::tokenize(const string& xml)
	{
		const regex re_xml("(</?\\w+)\\s*([^/>]*)\\s*(/?>)([^<]*)");
		const regex re_nws("\\S");
		smatch matches;
		
		string s = xml;
		while (regex_search (s, matches, re_xml)) {
			for ( auto it = matches.begin() + 1; it != matches.end(); ++it) {
				string tag = *it;
				if (!regex_search(tag, re_nws)) continue;
				m_tokens.push_back(tag);
			}
			s = matches.suffix().str();
		}
	}

	void Parser::tokenize(istream& in)
	{
		const regex re_end(">\\s*$");
		char buffer[1024] = "\0";
		string xml;
		while (in.getline(buffer, 1024)) {
			xml = buffer;
			xml += "\n";
			buffer[0] = '\0';
			while (!regex_search(xml, re_end) && in.getline(buffer, 1024)) {
				xml += buffer;
				xml += "\n";
				buffer[0] = '\0';
			}
			tokenize(xml);
		}
	}


	bool Parser::getAttribute(const string& name, string& value)
	{
		if (m_attributes.find(name) == m_attributes.end()) return false;
		
		value = m_attributes.at(name);
		m_attributes.erase(name);
		return true;
	}
	
	void Parser::parse_attributes(const string& nv)
	{
		const regex re_nv("(\\w+)=\"([^\"]+)\"");
		smatch matches;
		
		string s = nv;
		while (regex_search (s, matches, re_nv)) {
			string name = *(matches.begin() + 1);
			string value = *(matches.begin() + 2);
			unescape_tag(value);
			m_attributes.emplace(name, value);
			
			s = matches.suffix().str();
		}
	}
	
	void Parser::syntaxError(const string& msg)
	{
		string error = msg + "\n";
		for (int i = 0; i <= m_pos; ++i) { error += m_tokens[i]; }
		error += "<<<<<";
		throw logic_error(error);
	}

	void Parser::parse(State& state, string& tag)
	{
		const map<State, regex> parser_matches = {
			{ HEADER,     regex("^<\\w+$")                  },
			{ FOOTER,     regex("^</\\w+$")                 },
			{ NAME_VALUE, regex("^(\\w+=\"[^\"]+\"\\s*)+$") },
			{ ATOM_END,   regex("^/>$")                     },
			{ END,        regex("^>$")                      },
		};

		if (EOL()) throw logic_error("Unexpected end of xml in parser");
		state = ELEMENT;
		for ( auto& m : parser_matches ) { 
			if (regex_search(m_tokens[m_pos], m.second)) {
				state = m.first;
				break;
			}
		}
		switch (state) {
		    case END:
				state = (fsm.getState() == FOOTER) ? FOOTER_END : HEADER_END;
				break;
		    case HEADER:
				tag = m_tokens[m_pos].substr(1);
				break;
		    case FOOTER:
				tag = m_tokens[m_pos].substr(2);
				break;
		    default:
				break;
		}
	}

	void Parser::next()
	{
		State state;
		string tag;
		parse(state, tag);

		switch (state) {
		    case ELEMENT: {
				if (m_tokens[m_pos].find_first_of("<>\"'") != string::npos) 
					syntaxError("Element expected:");
				m_element = m_tokens[m_pos];
				unescape_tag(m_element);
				fsm.next(ELEMENT);
				break;
			}
		    case HEADER: {
				m_attributes.clear();
				m_element.clear();
			}
		    case FOOTER: {
				m_tag = tag;
				fsm.next(state, m_tag);
				break;
			}
		    case NAME_VALUE: {
				fsm.next(NAME_VALUE);
				if (fsm.getState() != ILLEGAL) parse_attributes(m_tokens[m_pos]);
				break;
			}
		    case ATOM_END:
		    case FOOTER_END:
		    case HEADER_END: {
				fsm.next(state);
				break;
			}
		    default: {
				throw logic_error("Illegal state value");
				break;
			}
		}
		if (fsm.getState() == ILLEGAL) 
			syntaxError("Bad xml syntax: " + to_string(state) + (tag.empty() ? "" : ", " + tag));
		++m_pos;
		if (state == FOOTER) next(FOOTER_END);
	}

	bool Parser::check(State ref_state, const string& ref_tag)
	{
		State state;
		string tag;
		parse(state, tag);
		return (ref_state == state) && (ref_tag.empty() || ref_tag == tag);
	}

	Parser& Parser::next(State state, const string& tag)
	{
		if (check(state, tag)) 
			next(); 
		else 
			syntaxError("Expected xml: " + to_string(state) + ", " + (tag.empty() ? "" : tag));
		return *this;
	}

	void Parser::finish()
	{
		while (m_pos < m_tokens.size()) next(FOOTER);
		if (!fsm.finished()) syntaxError("Missing footer:");
	}
}
