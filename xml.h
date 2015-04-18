#ifndef __XML_H
#define __XML_H

/*
 * xml.h
 * This file is part of milo
 *
 * Copyright (C) 2015 - James Terman
 *
 * milo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * milo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>     // std::cout
#include <vector>       // std::vector
#include <map>
#include <stack>
#include <regex>

namespace XML {
	enum State { HEADER=1, HEADER_END=2, FOOTER=4, ATOM_END=8, NAME_VALUE=16,
				 ELEMENT=32, NEW=64, ILLEGAL=128, FINISH=256 };
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

#endif // __XML_H
