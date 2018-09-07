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
 * @file ui.cpp
 * This file implements the milo user interface implementing user generated
 * events.
 */

#include "ui.h"

using namespace std;
using namespace UI;

static bool fRunning = true;                ///< When false, quit program.

static MiloApp& app = MiloApp::getGlobal(); ///< Reference to current app

unordered_map<string, menu_handler> MiloApp::menu_map = {
	{ "undo",   []() { if (app.hasPanel()) { app.getPanel().popUndo(); } } },
	{ "redraw", []() { app.redraw_screen(); } },
	{ "quit",   []() { fRunning = false; } }
};

unordered_map<string, panel_factory> MiloPanel::panel_map;

unordered_map<string, panel_xml> MiloPanel::panel_xml_map;

static const unordered_map<enum Mouse, string> mouse_string = {
	{ POSITION, "POSITION" }, { PRESSED, "PRESSED" }, { RELEASED, "RELEASED" },
	{ CLICKED, "CLICKED" }, { DOUBLE, "DOUBLE" }
};

static const unordered_map<enum Modifiers, string> mod_string = {
	{ ALT, "ALT-" }, { SHIFT, "SHIFT-" }, { ALT_SHIFT, "ALT-SHIFT-" }, { CTRL, "CTRL-" },
	{ CTRL_SHIFT, "CTRL-SHIFT-" }, { CTRL_ALT_SHIFT, "CTRL-ALT-SHIFT-" }, { NO_MOD, string() }
};

static const unordered_map<Keyboard, string> key_string = {
	{ Keys::F1, "F1" }, { Keys::F2, "F2" }, { Keys::F3, "F3" }, { Keys::F4, "F4" }, { Keys::F5, "F5" },
	{ Keys::F6, "F6" }, { Keys::F7, "F7" }, { Keys::F8, "F8" }, { Keys::F9, "F9" }, { Keys::F10, "F10" },
	{ Keys::F11, "F11" }, { Keys::F12, "F12" },	{ Keys::INS, "INS" }, { Keys::DEL, "DEL" }, { Keys::HOME, "HOME" },
	{ Keys::END, "END" }, { Keys::PAGE_UP, "PAGE_UP" },	{ Keys::PAGE_DOWN, "PAGE_DOWN" }, { Keys::UP, "UP" },
	{ Keys::DOWN, "DOWN" }, { Keys::LEFT, "LEFT" },	{ Keys::RIGHT, "RIGHT" }, { Keys::BSPACE, "BACKSPACE" },
	{ Keys::SPACE, "SPACE" }
};

static const unordered_map<string, enum Modifiers> stringToMod = {
	{ "ALT", ALT }, { "SHIFT", SHIFT }, { "ALT_SHIFT", ALT_SHIFT }, { "CTRL", CTRL },
	{ "CTRL_SHIFT", CTRL_SHIFT }, { "CTRL_ALT_SHIFT", CTRL_ALT_SHIFT }, { string(), NO_MOD }
};

static const unordered_map<string, Keyboard> stringToKey = {
	{ "F1", Keys::F1 }, { "F2", Keys::F2 }, { "F3", Keys::F3 }, { "F4", Keys::F4 }, { "F5", Keys::F5 },
	{ "F6", Keys::F6 }, { "F7", Keys::F7 }, { "F8", Keys::F8 }, { "F9", Keys::F9 }, { "F10", Keys::F10 },
	{ "F11", Keys::F11 }, { "F12", Keys::F12 },	{ "INS", Keys::INS }, { "DEL", Keys::DEL }, { "\u2302", Keys::HOME },
	{ "END", Keys::END }, { "PAGE\u25b2",Keys:: PAGE_UP }, { "PAGE\u25bc", Keys::PAGE_DOWN }, { "\u25b2", Keys::UP },
	{ "\u25bc", Keys::DOWN }, { "\u25c0", Keys::LEFT },	{ "\u25b6", Keys::RIGHT }, { "BSP", Keys::BSPACE },
	{ "SP", Keys::SPACE }, { "TAB", Keys::TAB }, { "ENTER", Keys::ENTER }, { "ESC", Keys::ESC }, { "HOME", Keys::HOME },
	{ "PAGE_UP", Keys::PAGE_UP }, { "PAGE_DOWN", Keys::PAGE_DOWN }, { "UP", Keys::UP }, { "DOWN", Keys::DOWN },
	{ "LEFT", Keys::LEFT }, { "RIGHT", Keys::RIGHT }, { "PLUS", Keys::PLUS }, { "MINUS", Keys::MINUS }
};

KeyEvent::KeyEvent(const string& key)
{
	auto dash = key.find("-");
	m_mod = NO_MOD;
	m_key = Keys::NONE;
	if (dash != string::npos) {
		auto it = stringToMod.find(key.substr(0, dash));
		if (it == stringToMod.end()) {
			return;
		}
		m_mod = it->second;
	}
	string letter = key.substr(dash+1);	 
	if (letter.length() == 1 && m_mod == NO_MOD) {
		m_key = Keyboard(letter[0]);
	}
	else if (letter.length() == 1 && isalpha(letter[0]) && m_mod == CTRL) {
		m_key = Keyboard(char(letter[0]) - '@');
		m_mod = NO_MOD;
	}
	else {
		auto it = stringToKey.find(letter);
		if (it == stringToKey.end()) {
			m_mod = NO_MOD;
			return;
		}
		m_key = it->second;
	}
}

string MouseEvent::toString() const
{
	return string("Mouse event: ") +
		   mod_string.at(m_mod) +
		   mouse_string.at(m_type) +
		   "-" + to_string(m_button);
}

string KeyEvent::toString() const
{

	if ( m_key < Keys::ESC )
		return string("Key event: " + mod_string.at(CTRL) + string(1, (char) '@' + m_key));
	else if ( m_key > Keys::SPACE && m_key < Keys::F1)
		return string("key event: " + mod_string.at(m_mod) +  string(1, (char) m_key));
	else
		return string("Key event: ") + mod_string.at(m_mod) + key_string.at(m_key);
}

bool MiloPanel::getActive()
{
	return getSharedPtr().get() == m_win->getPanelIter()->get();
}

void MiloPanel::setActive()
{
	m_win->setActivePanel(*this);
}

MiloPanel* MiloPanel::make(const string& name,
						   const string& init,
						   GraphicsPtr gc,
						   MiloWindow* win)
{
	auto panel_entry = panel_map.find(name);
	if (panel_entry != panel_map.end()) {
		return (panel_entry->second)(init, gc, win);
	}
	return nullptr;
}

MiloPanel* MiloPanel::make(XML::Parser& in,
						   bool& fActive,
						   GraphicsPtr gc,
						   MiloWindow* win)
{
	fActive = false;
	in.next(XML::HEADER, "panel").next(XML::NAME_VALUE).next(XML::HEADER_END);

	string name;
	if (!in.getAttribute("type", name)) {
		in.syntaxError("type missing from panel");
	}

	if (string value; in.getAttribute("active", value)) {
		if (value != "true" && value != "false") {
			in.syntaxError("bad boolean value");
		}
		fActive = (value == "true");
	}
	MiloPanel* panel = nullptr;
	auto panel_entry = panel_xml_map.find(name);
	if (panel_entry != panel_xml_map.end()) {
		panel = (panel_entry->second)(in, gc, win);
	}
	in.next(XML::FOOTER);
	return panel;
}

void MiloWindow::stepPanel(bool dir)
{
	if (dir) {
		++m_current_panel;
		if (m_current_panel == m_panels.end()) {
			m_current_panel = m_panels.begin();
		}
	} else {
		if (m_current_panel == m_panels.begin()) {
			m_current_panel = m_panels.end();
		}
		--m_current_panel;
	}
}

void MiloWindow::xml_in(XML::Parser& in)
{
	in.next(XML::HEADER, "title").next(XML::HEADER_END).next(XML::ELEMENT);
	if (in.hasElement()) {
		m_title = in.getElement();
	}
	else {
		in.syntaxError("Missing title");
	}	
	in.next(XML::FOOTER);
	in.assertNoAttributes();

	MiloPanel* active_panel = nullptr;
	while (in.check(XML::HEADER, "panel")) {
		bool fActive = false;
		MiloPanel* p = MiloPanel::make(in, fActive, MiloApp::getGlobal().makeGraphics(), this);
		m_panels.push_back(p);
		if (fActive) {
			active_panel = p;
		}
	}

	if (active_panel != nullptr) {
		setActivePanel(*active_panel);
	}
	else {
		m_current_panel = m_panels.begin();
	}	
}

void MiloWindow::out(XML::Stream& xml)
{
	xml << XML::HEADER << "title" << XML::ELEMENT << m_title << XML::FOOTER;
	for ( auto panel : m_panels ) {
		panel->out(xml);
	}
}

bool MiloApp::isRunning()
{
	return fRunning;
}

void MiloApp::doMenu(const string& menuFunctionName)
{
	if (!getWindow().getPanel().doMenu(menuFunctionName)) {
		auto menu_entry = menu_map.find(menuFunctionName);
		if (menu_entry != menu_map.end()) {
			(menu_entry->second)();
		}
	}
}

void MenuXML::parse_menu(XML::Parser& in)
{
	in.next(XML::HEADER, "menu").next(XML::NAME_VALUE);

	string type;
	if (!in.getAttribute("type", type)) throw logic_error("type not found");
	if (type == "line") {
		define_menu_line();
		in.next(XML::ATOM_END);
		return;
	}

	string value;
	string name;
	if (!in.getAttribute("name", name)) throw logic_error("name not found");
	if (type == "menu") {
	    StringMap attributes;
		attributes.emplace("type", "menu");
		attributes.emplace("name", name);
		if (!in.getAttribute("active", value)) throw logic_error("active tag not found");
		if (value != "true" && value != "false") throw logic_error("bad boolean value");
		attributes.emplace("active", value);
		if (in.getAttribute("title", value))
			attributes.emplace("title", value);
		else
			attributes.emplace("title", name);

		define_menu(attributes);
		in.next(XML::HEADER_END);
		while (in.check(XML::HEADER, "menu")) parse_menu(in);
		in.next(XML::FOOTER);
		define_menu_end(name);
	}
	else if (type == "item") {
		StringMap attributes;
		attributes.emplace("type", "item");
		attributes.emplace("name", name);
		for ( const string& s : { "active", "action", "key" } ) {
			if (!in.getAttribute(s, value)) throw logic_error(s + " not found");
			attributes.emplace(s, value);
		}
		if (in.getAttribute("title", value))
			attributes.emplace("title", value);
		else
			attributes.emplace("title", name);

		define_menu_item(attributes);
		in.next(XML::ATOM_END);
	}
	else
		throw logic_error("unknown type: " + value);
}
