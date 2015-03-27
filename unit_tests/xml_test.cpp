/*
 * xml_test.cpp
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

#include "xml.h"

using namespace std;

const string test1 =
"<document>\n"
"  <equation>\n"
"    <variable/>\n"
"  </equation>\n"
"</document>";

const string test2 =
"<document>\n"
"  <equation>\n"
"    <variable name=\"a\" negative=\"true\"/>\n"
"  </equation>\n"
"</document>";

const string test3 =
"<document>\n"
"  <plans>\n"
"    <test plan=\"foo&lt;bar&gt;\">frick &amp;\n frack</test>\n"
"  </plans>\n"
"</document>";

const string test5 =
"<document>\n"
"  <equation>\n"
"    <variable/>\n"
"</document>";

const string test6 =
"<document>\n"
"  <equation>\n"
"    <variable name=\"a negative=\"true\"/>\n"
"  </equation>\n"
"</document>";


const string test_n =
"<document>\n"
"  <equation>\n"
"    <expression>\n"
"      <term>\n"
"        <variable name=\"x\"/>\n"
"        <divide nth=\"2\">\n"
"          <expression>\n"
"            <term>\n"
"              <variable name=\"a\" nth=\"3\" negative=\"true\">\n"
"            </term>\n"
"          </expression>\n"
"          <expression>\n"
"            <term>\n"
"              <variable name=\"b\"/>\n"
"            </term>\n"
"          </expression>\n"
"        </divide>\n"
"        <power nth=\"-2\">\n"
"          <expression>\n"
"            <term>\n"
"              <variable name=\"f\"/>\n"
"            </term>\n"
"          </expression>\n"
"          <expression>\n"
"            <term>\n"
"              <variable name=\"good bar\"/>\n"
"              <text>The quick brown fox</text>\n"
"            </term>\n"
"          </expression>\n"
"        </power>\n"
"      </term>\n"
"    </expression>\n"
"  </equation>\n"
"</document>";


int main()
{
	bool noThrow = true;
	ostringstream store1;
	try {
		XML::Stream xml(store1);
		xml << XML::HEADER << "equation" << XML::HEADER_END;
		xml << XML::HEADER << "variable" << XML::ATOM_END;
		xml << XML::FOOTER;
	}
	catch (std::exception& e) {
		cout << "Exception caught: " << e.what() << endl;
		noThrow = false;
	}
	cout << "Test #1a " << ( (noThrow && store1.str() == test1) ? "passed" : "failed" ) << endl;
	if (store1.str() != test1) cout << store1.str() << endl;

	noThrow = true;
	istringstream in1(store1.str());	
	try {
		XML::Parser p(in1);
		p.next(XML::HEADER, "equation").next(XML::HEADER_END);
		p.next(XML::HEADER, "variable").next(XML::ATOM_END);
		p.finish();
	}
	catch (std::exception& e) {
		cout << "Exception caught: " << e.what() << endl;
		noThrow = false;
	}
	cout << "Test #1b " << ( noThrow ? "passed" : "failed" ) << endl;
		
	noThrow = true;
	ostringstream store2;
	try {
		XML::Stream xml(store2);
		xml << XML::HEADER << "equation" << XML::HEADER_END;
		xml << XML::HEADER << "variable" << XML::NAME_VALUE << "name" << "a";
		xml << XML::NAME_VALUE << "negative" << "true" << XML::ATOM_END;
		xml << XML::FOOTER;
	}
	catch (std::exception& e) {
		cout << "Exception caught: " << e.what() << endl;
		noThrow = false;
	}
	cout << "Test #2a " << ( (noThrow && store2.str() == test2) ? "passed" : "failed" ) << endl;
	if (store2.str() != test2) cout << store2.str() << endl;

	noThrow = true;
	istringstream in2(store2.str());
	try {
		XML::Parser p(in2);
		p.next(XML::HEADER, "equation").next(XML::HEADER_END);
		p.next(XML::HEADER, "variable");
		if (p.check(XML::NAME_VALUE)) p.next(XML::NAME_VALUE);
		p.next(XML::ATOM_END);
		string value;
		if (!p.getAttribute("name", value) || value != "a") throw logic_error("test did not pass");
		if (!p.getAttribute("negative", value) || value != "true") throw logic_error("test did not pass");
		p.finish();
	}
	catch (std::exception& e) {
		cout << "Exception caught: " << e.what() << endl;
		noThrow = false;
	}
	cout << "Test #2b " << ( noThrow ? "passed" : "failed") << endl;

	noThrow = true;
	ostringstream store3;
	try {
		XML::Stream xml(store3);
		xml << XML::HEADER << "plans" << XML::HEADER_END;
		xml << XML::HEADER << "test" << XML::NAME_VALUE << "plan" << "foo<bar>" << XML::HEADER_END;
		xml << XML::ELEMENT << "frick &\n frack";
	}
	catch (std::exception& e) {
		cout << "Exception caught: " << e.what() << endl;
		noThrow = false;
	}
	cout << "Test #3a " << ( ( noThrow && store3.str() == test3 ) ? "passed" : "failed" ) << endl;
	if (store3.str() != test3) cout << store3.str() << endl;

	noThrow = true;
	istringstream in3(store3.str());
	try {
		XML::Parser p(in3);
		p.next(XML::HEADER, "plans").next(XML::HEADER_END);
		p.next(XML::HEADER, "test").next(XML::NAME_VALUE).next(XML::HEADER_END);
		p.next(XML::ELEMENT).finish();
		if (p.getElement().empty() || p.getElement() != "frick &\n frack") 
			throw logic_error("Element: " + p.getElement());
	}
	catch (std::exception& e) {
		cout << "Exception caught: " << e.what() << endl;
		noThrow = false;
	}
	cout << "Test #3b " << ( noThrow ? "passed" : "failed") << endl;

	bool passed = false;
	ostringstream store4;
	try {
		XML::Stream xml(store4);
		xml << XML::HEADER << "equation" << XML::HEADER_END;
		xml << XML::HEADER << "variable" << XML::ATOM_END;
		xml << XML::FOOTER << XML::FOOTER << XML::FOOTER;
	}
	catch (std::exception& e) {
		string err = e.what();
		if (err == "Too many footers") passed = true;
		else cout << "Unexpected exception: " << "'" << e.what() << "'" << endl;
	}
	cout << "Test #4  " << ( passed ? "passed" : "failed") << endl;

	passed = false;
	istringstream in5(test5);
	try {
		XML::Parser p(in5);
		p.next(XML::HEADER, "equation").next(XML::HEADER_END);
		p.next(XML::HEADER, "variable").next(XML::ATOM_END);
		p.finish();
	}
	catch (std::exception& e) {
		string err = e.what();
		if (err == "Bad xml syntax: FOOTER, document\n<document><equation><variable/></document><<<<<") passed = true;
		else cout << "Unexpected exception: " << "'" << e.what() << "'" << endl;
	}
	cout << "Test #5  " << ( passed ? "passed" : "failed") << endl;

	passed = false;
	istringstream in6(test6);
	try {
		XML::Parser p(in6);
		p.next(XML::HEADER, "equation").next(XML::HEADER_END);
		p.next(XML::HEADER, "variable");
		if (p.check(XML::NAME_VALUE)) p.next(XML::NAME_VALUE);
		p.next(XML::ATOM_END);
		p.finish();
	}
	catch (std::exception& e) {
		string err = e.what();
		if (err == "Bad name, value pair:\n<document><equation><variablename=\"a negative=\"true\"<<<<<") passed = true;
		else cout << "Unexpected exception: " << "'" << e.what() << "'" << endl;
	}
	cout << "Test #6  " << ( noThrow ? "passed" : "failed") << endl;
}
