#ifndef __XML_H
#define __XML_H

/* Copyright (C) 2018 - James Terman
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

/**
 * @file xml.h
 * This file is the interface to the XML input and output stream
 * The XML output stream is used to serialize the equation tree to an XML file.
 * The XML input stream is to deserialize the XML file into an equation tree
 *
 * Both streams are designed as state machines. Each class can set 
 * expectations on what they see in the input (and throw exceptions) and 
 * construct the class object appriopately. Classes also need to construct
 * their XML output correctly or XML state machinge will throw an exception.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <regex>

/** 
 * XML namespace processing interface.
 * XML namespace interface is the module that milo uses for XML processing.
 * It contains an input stream and an output stream.
 */
namespace XML {
    /** 
	 * enum State stores the current state of the XML Finite State Machine.
	 * The XML Finite State Machine contains the state either from the last input processed
	 * or the last output given to the stream. When the parser (input) or the stream (output) 
	 * the FSM is advanced to a new state.
	 * This transition is checked to see if it is legal. FSM starts out at NEW state 
	 * whose only legal transition is HEADER. Each state corresponds to the following tokens:
	 *
	 * HEADER     => <{tag_name}       ELEMENT => Any string in between tags
     * HEADER_END => >                 =========================================================
	 * FOOTER     => </{tag_name}>     NEW     => State at beginning
	 * ATOM_END   => />                ILLEGAL => new state not legal transition from prev state
	 * NAME_VALUE => {name}="{value}"  FINISH  => Output only. Close all open headers
	 *
	 */
	enum State { HEADER=1, HEADER_END=2, FOOTER=4, ATOM_END=8, NAME_VALUE=16,
				 ELEMENT=32, NEW=64, ILLEGAL=128, FINISH=256 };

	/** 
	 * XML Finite State Machine.
	 * This class used for both input and output to keep track of current
	 * state of XML and to make sure the next state is a legal transition
	 */
	class FSM
	{
	public:
		/**
		 * Default constructor for FSM class.
		 * Initialize FSM to state NEW. Only time FSM is in this state.
		 */
		FSM() : m_state(NEW) {}

		/**
		 * Return current state of FSM.
		 * @return Current state of FSM.
		 */
		State getState() { return m_state; }

		/**
		 * Return string with tag last added or next to be processed.
		 * @return Return top of tag stack.
		 */
		const std::string& getTag() { return m_tags.top(); }

		/**
		 * Advance FSM to new_state and check if this is a legal transition. 
		 * If tag is non-empty it also checked. Either m_state is set to new_state or ILLEGAL.
		 * @param new_state Expected next state after transition
		 * @param tag       Expected next tag (may be empty if not relevent).
		 */
		void next(State new_state, const std::string& tag = std::string());

		/**
		 * Check if FSM is finished when last closing footer is encountered.
		 * @return True if there no more tags to be processed.
		 */
		bool finished() { return m_tags.empty(); }
	private:
		/**
		 * Check the transition of old state (m_state) => new_state is legal. 
		 * If legal, m_state is new_state otherwise it is set to illegal.
		 */
		void advance(State new_state);

		State m_state; 		             ///< Current state. Initialized to NEW
		std::stack<std::string> m_tags;  ///< Stack of header name tags

		/**
		 * The array transitions implments the following state machine to check XML syntax.
		 * <PRE>
         *      +------+      NEW  +--------+
         *      ^      |       |   |        |
         *      |      V       V   V        |
         *    FOOTER<--+------>HEADER-----+ |
         *      ^      ^        |  |      | |
         *      |      |   +----+  |  +-+ | |
         *      |      |   |       |  | | | |
         *      |      |   V       V  V | | |
         *      |    ATOM_END<-NAME_VALUE | |
         *      |                 |       | |
         *      |                 V       | |
         *      +--ELEMENT<---HEADER_END<-+ |
         *                        |         |
         *                        +---------+
		 * </PRE>
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

	/** Default root xml tag.
	 */
	const std::string ROOT = "document";

	/**
	 * Output stream for XML. 
	 * Overloaded << operator is used to output XML states and tags.
	 * The constructor and destructor adds the root tag supplied ("document" in this example)
	 * level to the output. Destructor will also closes all open header tags.
	 * Example code:
	 * <PRE>
	 * {
	 *   Stream xml(std::cout, "document");  
	 *   xml << HEADER << "foo";
	 *   xml << HEADER << "bar" << NAME_VALUE << "flag" << "true" << ATOM_END;
	 *   xml << HEADER << "text" << HEADER_END << ELEMENT << "Example text & element" << FOOTER;
	 * }
	 * </PRE>
	 * This will give output:
	 * <PRE>
	 * <document>
	 *   <foo>
	 *     <bar flag="true"/>
	 *     <text>Example text &amp; element</text>
	 *   </foo>
	 * </document>
	 * </PRE>
	 */
	class Stream
	{
	public:
		/**
		 * Constructor for class Stream.
		 * Initialize private data members and add root header.
		 * @param os   Output character stream to write XML
		 * @param root Root tag of xml output
		 * @param step Number of spaces to add to indention
		 * @param sep  Character to add before each indention
		 */
		Stream(std::ostream& os,
			   const std::string& root = ROOT,
			   int step = 2,
			   std::string sep = "\n"
		      );

		/**
		 * Destructor for class stream.
		 * Close all open headers with a footer.
		 */
		~Stream();

		/**
		 * Advances the XML output stream to the given state.
		 * Some states like footers are complete onto themselves and others like
		 * headers create a pending state to complete the transition.
		 * @param state New state for the output stream.
		 */
		void out(State state);

		/**
		 * Called when XML stream has a pending state that needs another string.
		 * With the pending state satisfied, it it completes the transistion to the next state.
		 * @param tag String value for pending state.
		 */
		void out(const std::string& tag);
	private:
		/**
		 * Possible pending states.
		 * The HEADER tag leaves the stream in a HEADER_TAG pending state.
		 * The ELEMENT tag leaves thestream in an ELEMENT_TAG pending state.
		 * The NAME_VALUE tag leaves the stream in a NAME pending state. After the
		 * call to out(const std::string&), the stream is in a value pending state.
		 */
		enum Pending { NONE, HEADER_TAG, NAME, VALUE, ELEMENT_TAG };

		std::string m_sep;        ///< Character to output before indention.
		std::ostream& m_os;       ///< Ouput character stream to write XML.
		const int m_indent_step;  ///< Number of spaces in indention before HEADER and standalone FOOTER.
		Pending m_pending = NONE; ///< Current pending state.
		std::string m_nv;         ///< temp space for name tag value.
		int m_indent = 0;         ///< current indention of XML line.
		FSM fsm;                  ///< current FSM state.
	};

	/**
	 * Operator << overloading to send XML State to XML stream.
	 * Allows xml << XML::FOOTER equivalent to xml.out(XML::FOOTER).
	 * @param xml XML stream object.
	 * @param state XML state to output to stream.
	 * @return XML stream object.
	 */
	Stream& operator<<(Stream& xml, State state);

	/**
	 * Operator << overloading to send string to XML stream.
	 * Allows xml << "foo & bar" equivalent to xml.out("foo & bar").
	 * This ends up as "foo &amp; bar" in output
	 * @param xml XML stream object.
	 * @param tag XML tag to output to stream.
	 * @return XML stream object.
	 */
	Stream& operator<<(Stream& xml, const std::string& tag);

	template <class T> inline auto operator<<(XML::Stream& xml, T& t) -> decltype(t.out(xml))
	{
		t.out(xml);
		return xml;
	}
	
	/**
	 * Parsing of XML tree from input stream.
	 * Inspector member functions getTag, getAttribute, getElement 
	 * allow you to extract the XML information read in.
	 */
	class Parser
	{
	public:
		/**
		 * Constructor for class Parser.
		 * Initialize character input stream and read in 
		 * root header tag.
		 * @param in Input containing xml content
		 * @param root Root xml tag
		 */
		Parser(std::istream& in, const std::string& root = ROOT);

		/**
		 * Virtual desctructor for class Parser.
		 * Class Parser is often inherited to support 
		 * using this class as a paramter to class's constructor
		 * to deserialize.
		 */
		virtual ~Parser() {}

		/**
		 * Checks that the current XML token matches the state and if not empty, 
		 * the tag passed to it. 
		 * @param state XML state to be matched.
		 * @param tag   tag to be matched if non empty.
		 * @return True if match succeeded.
		 */
		bool check(State state, const std::string& tag = std::string());

		/**
		 * Checks that the next XML token matches the state and if not empty, 
		 * the tag passed to it. If match fails throw a syntax error exception.
		 * @param state XML state to be matched.
		 * @param tag   tag to be matched if non empty.
		 * @return Reference to this class object for chaining calls.
		 */
		Parser& next(State state, const std::string& tag = std::string());

		/**
		 * Finish parsing XML stream by classing all remaining footers.
		 * Assume that all remaining tags are footers and close them.
		 */
		void finish();

		/**
		 * Return the last tag processed by the stream.
		 * This is the current tag being processed. This is not cleared
		 * until a new header or footer is processed.
		 * @return Current tag
		 */
		const std::string& getTag() const { return m_tag; }

		/**
		 * Looks for a attribute name and store its value in the argument value. 
		 * All name/value pairs have been stores in m_attributes data member. 
		 * If string value in name is found, store value in argument value.
		 * @param name Name of attribute to be searched for.
		 * @param[out] value Value of attribute found.
		 @ @return Return true if name found in attribute map
		 */
		bool getAttribute(const std::string& name, std::string& value);

		/**
		 * Checks for any attributes in the last header processed.
		 * Checks for non-empty attributes map which is clear when new header is parsed.
		 * @return True if attributes have been processed.
		 */
		bool hasAttributes() { return m_attributes.size() != 0; }

		/**
		 * Throw an error if there are attributes left.
		 */
		void assertNoAttributes() {
			if (hasAttributes()) syntaxError("Unknown attribute");
		}

		/**
		 * Checks for any element in last header/footer tags processed.
		 * If an element was found in last header/footer tag it is stored in m_element.
		 * @return Return true if m_element is non-empty.
		 */
		bool hasElement() { return !m_element.empty(); }

		/**
		 * Return the last element in the last header/footer processed.
		 * Will be empty if no element was found.
		 * @return Return last element processed if any.
		 */
		const std::string& getElement() { return m_element; }

		/**
		 * Throws an exception display message string and XML trace.
		 * The exception string contain all the XML that has been 
		 * parsed so far plus the message string.
		 * @param msg Text for message in syntax error.
		 */
		void syntaxError(const std::string& msg);

	private:
		std::vector<std::string> m_tokens; ///< Storage for XML tokens to be processed.
		unsigned int m_pos;                ///< Index of current token being processed.

		/**
		 * Map of attributes of last header processed.
		 * The name/value pairs of the attributes from the last
		 * header processed, i.e. text="foo".
		 */
		std::unordered_map<std::string, std::string> m_attributes;

		FSM fsm;               ///< XML finite state machine
		std::string m_tag;     ///< last heder name tag processed
		std::string m_element; ///< last tag element processed

		/**
		 * Checks if at end of XML processing.
		 * @return Return true if last XML token was processed.
		 */
		bool EOL() const { return m_pos == m_tokens.size(); }

		/**
		 * Parses the current token.
		 * Populates the Parser class object with the XML information 
		 * and advances the FSM checking if the transition was legal.
		 */
		void next();

		/**
		 * Tokenize the entire XML input stream.
		 * Reads from the stream tokenizing it. It reads line by line 
		 * until it finds the character that will end the current XML 
		 * tag which is then tokenized. Carriage returns are perserved
		 * and can be in name, value pairs and element tags.
		 * @param in Character input stream containing XML.
		 */
		void tokenize(std::istream& in);

		/**
		 * Helper function for tokenize(std::istream&).
		 * This function is passed substrings which are further
		 * processed into tokens.
		 * @param xml Substring from XML input stream.
		 */
		void tokenize(std::string xml);

		/**
		 * Parses the attributes of a header tag.
		 * Expects a string containing name="value" pairs separated by white space. 
		 * It then populates the m_attributes map with these name value pairs.
		 * @param nv String containing mulitple name="value" pairs seperated by whitespace.
		 */	
		void parse_attributes(const std::string& nv);

		/**
		 * Parses the current token and save result in state and tag arguments.
		 * Will parse the current token, m_tokens[m_pos],  and return the corresponding 
		 * state and tag if appropiate. Mostly a helper function for Parser::next().
		 * @param[out] state Loaded with state of the current token.
		 * @param[out] tag   Loaded with tag of current token if appropiate otherwise empty.
		 */
		void parse(State& state, std::string& tag);
	};

	/**
	 * Convert characters in string to their escaped counterparts.
	 * { &, <, >, ", '} => { &amp;, &lt;, &gt;, &quot, &apos }
	 */
	void escape_tag(std::string& tag);

	/**
	 * Convert escaped character in string to regular text.
	 * { &amp;, &lt;, &gt;, &quot, &apos } => { &, <, >, ", '}
	 */
	void unescape_tag(std::string& tag);
}

#endif // __XML_H
