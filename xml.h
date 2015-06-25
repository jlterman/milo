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
    /*
	 * The State enum keeps track of the current state of the XML Finite State Machine.
	 * When the parser (input) or the stream (output) the FSM is advanced to a new state.
	 * This transition is checked to see if it is legal. FSM starts out at NEW state 
	 * whose only legal transition is HEADER. Each state corresponds to the following tokens:
	 *
	 * HEADER     => <{tag_name}       ELEMENT => Any string in between tags
     * HEADER_END => >                 =========================================================
	 * FOOTER     => <{tag_name}>      NEW     => State at beginning
	 * ATOM_END   => />                ILLEGAL => new state not legal transition from prev state
	 * NAME_VALUE => {name}="{value}"  FINISH  => Output only. Close all open headers
	 *
	 */
	enum State { HEADER=1, HEADER_END=2, FOOTER=4, ATOM_END=8, NAME_VALUE=16,
				 ELEMENT=32, NEW=64, ILLEGAL=128, FINISH=256 };

	/*
	 * Finite State Machine class used for both input and output to keep track of current
	 * state of XML and to make sure the next state is a legal transition
	 */
	class FSM
	{
	public:
		FSM() : m_state(NEW) {}

		// Return current state of FSM
		State getState() { return m_state; }

		// Return most recent header tag encountered
		const std::string& getTag() { return m_tags.top(); }

		// Advance FSM to new_state and new tag if new_state == HEADER
		void next(State new_state, const std::string& tag = std::string());

		// FSM is finished with last closing footer encountered
		bool finished() { return m_tags.empty(); }
	private:
		// Set FSM current state to new_state if legal transition, to illegal otherwise. 
		void advance(State new_state);

		// Current state. Initialized to NEW
		State m_state;

		// List of header name tags
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

	/*
	 * Class Stream acts as an output stream for XML. Use << operator 
	 * to send XML states and strings to Stream class. 
	 */
	class Stream
	{
	public:
		// os   - ouput stream where XML is sent to
		// step - number of spaces nested after each header
		// sep  - character seperating each tag
		Stream(std::ostream& os, int step = 2, std::string sep = "\n");
		~Stream();

		// output state to Stream. Legality of transistion checked
		void out(State state);

		// output string to Stream. Behavior is dependent on Pending state
		void out(const std::string& tag);
	private:
		// Some states expect a string next. These internal states
		// will handle the string correctly
		enum Pending { NONE, HEADER_TAG, NAME, VALUE, ELEMENT_TAG };

		std::string m_sep;
		std::ostream& m_os;
		const int m_indent_step; 
		Pending m_pending = NONE;
		std::string m_nv;
		int m_indent = 0; // current indention of XML line
		FSM fsm;          // current FSM state 
	};

	// Operator overloading of << operator for Stream
	Stream& operator<<(Stream& xml, State state);
	Stream& operator<<(Stream& xml, const std::string& tag);

	/*
	 * Class parser reads from an input stream and parses the XML.
	 * Inspector member functions getTag, getAttributes, getElement 
	 * allow you to extract the XML information read in.
	 */
	class Parser
	{
	public:
		
		Parser(std::istream& in);
		virtual ~Parser() {}

		// Check the current state and last header name tag read in
		bool check(State state, const std::string& tag = std::string());

		// Read in the next XML tag and check the state and the header name tag
		Parser& next(State state, const std::string& tag = std::string());

		// Read remaing XML assuming that only footers remain
		void finish();

		// get last header name tag read in
		const std::string& getTag() const { return m_tag; }

		// Return true if attribute name was found. Value returned in value
		bool getAttribute(const std::string& name, std::string& value);

		// Return true if unread attributes remain
		bool hasAttributes() { return m_attributes.size() != 0; }

		// Return true if XML element was read in
		bool hasElement() { return !m_element.empty(); }

		// Return XML element
		const std::string& getElement() { return m_element; }

		// Throw an exception that displays XML tokens read in
		void syntaxError(const std::string& msg);

	private:
		// Array of tokenized XML from input stream (see enum XML::State)
		std::vector<std::string> m_tokens;

		// current token being parsed
		unsigned int m_pos;

		// Attributes read in from last header tag
		std::map<std::string, std::string> m_attributes;

		FSM fsm;               // XML finite state machine
		std::string m_tag;     // last heder name tag processed
		std::string m_element; // last tag element processed

		// Return true if last token was processed
		bool EOL() const { return m_pos == m_tokens.size(); }

		// Process next token and check if transistion was legal
		void next();

		// Tokenize input stream with helper function tokenize(string)
		void tokenize(std::istream& in);

		// Helper function for tokenize(istream)
		void tokenize(std::string xml);

		// Take token with name, value pairs(s) and fill m_attributes map
		void parse_attributes(const std::string& nv);

		// Parse next XML token and put in arguments state and tag
		void parse(State& state, std::string& tag);
	};

	// Escape special characters to embed arbitrary text into XML
	void escape_tag(std::string& tag);

	// Unescape special characters to get back original text
	void unescape_tag(std::string& tag);
}

#endif // __XML_H
