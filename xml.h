#ifndef __XML_H
#define __XML_H

#include <iostream>     // std::cout
#include <vector>       // std::vector
#include <map>
#include <stack>
#include <regex>

namespace XML {
	enum State { END=1, HEADER=2, HEADER_END=4, FOOTER=8, ATOM_END=16, NAME_VALUE=32,
				 ELEMENT=64, NEW=128, ILLEGAL=256, FINISH=512 };
	class FSM
	{
	public:
		FSM() : m_state(NEW) {}

		State getState() { return m_state; }
		const std::string& getTag() { return m_tags.top(); }
		void next(State new_state, const std::string& tag = std::string());
		bool finished() { return m_tags.empty(); }
	private:
		void advance(State new_state);
		State m_state;
		std::stack<std::string> m_tags;

		/*
		 * The array transitions implments the following state machine
		 * to check XML syntax

                +------+      NEW  +--------+
                ^      |       |   |        |
                |      V       V   V        |
              FOOTER<--+------>HEADER-----+ |
                ^      ^        |  |      | |
                |      |   +----+  |  +-+ | |
                |      |   |       |  | | | |
                |      |   V       V  V | | |
                |    ATOM_END<-NAME_VALUE | |
                |                 |       | |
                |                 V       | |
                +--ELEMENT<---HEADER_END<-+ |
                                  |         |
                                  +---------+
		*/

		const std::vector<std::array<int, 2>> transitions = {
			{ NEW,             HEADER                         },
			{ ATOM_END|FOOTER, FOOTER|HEADER                  },
			{ HEADER,          NAME_VALUE|HEADER_END|ATOM_END },
			{ NAME_VALUE,      HEADER_END|ATOM_END|NAME_VALUE },
			{ HEADER_END,      ELEMENT|HEADER                 },
			{ ELEMENT,         FOOTER                         }
		};
	};

	class Stream
	{
	public:
		Stream(std::ostream& os, int step = 2, std::string sep = "\n");
		~Stream();

		void out(State state);
		void out(const std::string& tag);
	private:
		enum Pending { NONE, HEADER_TAG, NAME, VALUE, ELEMENT_TAG };

		Pending m_pending = NONE;
		std::string m_nv;
		std::ostream& m_os;
		const int m_indent_step;
		int m_indent = 0;
		std::string m_sep;
		FSM fsm;
	};

	Stream& operator<<(Stream& xml, State state);
	Stream& operator<<(Stream& xml, const std::string& tag);

	class Parser
	{
	public:
		
		Parser(std::istream& in);
		virtual ~Parser() {}
		
		bool check(State state, const std::string& tag = std::string());
		Parser& next(State state, const std::string& tag = std::string());
		void finish();
		const std::string& getTag() const { return m_tag; }

		bool getAttribute(const std::string& name, std::string& value);
		bool hasAttributes() { return m_attributes.size() != 0; }

		bool hasElement() { return !m_element.empty(); }
		const std::string& getElement() { return m_element; }

		void syntaxError(const std::string& msg);
	private:
		std::vector<std::string> m_tokens;
		size_t m_pos;
		std::map<std::string, std::string> m_attributes;

		FSM fsm;
		std::string m_tag;
		std::string m_element;
		
		bool EOL() const { return m_pos == m_tokens.size(); }
		void next();
		void tokenize(std::istream& in);
		void tokenize(std::string xml);
		void parse_attributes(const std::string& nv);
		void parse(State& state, std::string& tag);

		static const std::map<State, std::regex> xml_matches;
	};

	void escape_tag(std::string& tag);
	void unescape_tag(std::string& tag);
}

#endif // __PARSER_H
