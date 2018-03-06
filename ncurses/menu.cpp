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
 * @file ncurses/menu.cpp
 * This file implements the ncurses menu classes.
 */
#include "ncurses/menu.h"

using namespace std;
using namespace UI;

unordered_map<KeyEvent, string> MenuBar::key_menu_map;

void MenuBar::define_menu(const StringMap& attributes)
{
	Menu* new_menu = new Menu(attributes);
	if (m_level == 0) {
		new_menu->m_parent = nullptr;
		add(new_menu);
	} else {
		new_menu->m_parent = m_menu_heap.top();
		m_menu_heap.top()->addItem(new_menu);
	}
	m_menu_heap.push(new_menu);
	++m_level;
}

void MenuBar::define_menu_end(const std::string& name)
{
	--m_level;
	if (m_menu_heap.top()->getName() != name) throw logic_error("Unexpected menu name: " + name);
	m_menu_heap.pop();
}

void MenuBar::define_menu_line()
{
	m_menu_heap.top()->addItem(new MenuLine());
}

void MenuBar::define_menu_item(const StringMap& attributes)
{
	auto menu_key = attributes.find("key");
	if ( menu_key == attributes.end()) {
		throw logic_error("No key attribute in menu item");
	}
	auto menu_action = attributes.find("action");
	if ( menu_action == attributes.end()) {
		throw logic_error("No action attribute in menu item");
	}
	KeyEvent key(menu_key->second);
	if (key) {
		key_menu_map.emplace(key, menu_action->second);	}
	m_menu_heap.top()->addItem(new MenuItem(attributes));
}

MenuBar::MenuBar(const string& xml) : m_root(0), m_level(0)
{
	ifstream is(xml);
	XML::Parser in(is, "menubar");
	while (in.check(XML::HEADER, "menu")) parse_menu(in);
	in.next(XML::FOOTER);
}

void MenuBar::add(Menu* menu)
{
	menu->m_wbar = utf8::distance(menu->getTitle().begin(), menu->getTitle().end());
	if (m_menus.empty()) {
		menu->m_xbar = 1;
	}
	else {
		menu->m_xbar = m_menus.back()->m_xbar + m_menus.back()->m_wbar + 2;
	}
	m_menus.emplace_back(menu);
}

void MenuBar::draw()
{
	for ( auto m : m_menus ) {
		m->getWidth();
	}
	int x_width = -1;
	getmaxyx(stdscr, x_width, x_width);
	int x = -1;
	while (x < x_width) { mvaddch(0, x++, ' '); }
	for ( auto m : m_menus ) {
		m->drawInBar(m->m_xbar, m == m_root);
	}
	if (Menu::m_current) {
		Menu::m_current->refresh_window();
	}
}

bool MenuBar::select(int mouse_x)
{
	m_root = 0;
	for ( auto m : m_menus ) {
		if (mouse_x >= m->m_xbar && mouse_x < m->m_xbar + m->m_wbar) {
			m_root = m;
			m->select();
			break;
		}
	}
	return m_root != 0;
}

bool MenuBar::handleKey(int code)
{
	if (Menu::m_current->handleKey(code)) {
		if (Menu::m_current == nullptr) {
			m_root = 0;
		}
		return true;
	}
	else {
		switch(code) {
		    case KEY_SLEFT: {
				m_root->handleKey(UI::Keys::ESC);
				if (m_root == *m_menus.begin()) {
					m_root = *(m_menus.end() - 1);
				}
				else {
					for ( auto m = m_menus.begin() + 1; m != m_menus.end(); ++m ) {
						if ( m_root == *m ) {
							m_root = *(--m);
							break;
						}
					}
				}
				m_root->select();
				break;
		    }
		    case KEY_SRIGHT: {
				m_root->handleKey(UI::Keys::ESC);
				if (m_root == *(m_menus.end() - 1)) {
					m_root = *m_menus.begin();
				}
				else {
					for ( auto m = m_menus.begin(); m != m_menus.end(); ++m ) {
						if ( m_root == *m ) {
							m_root = *(++m);
							break;
						}
					}
				}
				m_root->select();
				break;
		    }
		}
	}
	return false;
}

bool Menu::handleKey(int code)
{
	switch (code) {
	    case KEY_UP: {
			do
			{
				if (m_highlight == m_items.begin()) {
					m_highlight = m_items.end();
				}
			}
			while (!(*(--m_highlight))->getActive());
			break;
	    }
	    case KEY_DOWN: {
			do
			{
				if (m_highlight == m_items.end() - 1) {
					m_highlight = m_items.begin();
				}
				else {
					++m_highlight;
				}
			}
			while (!(*m_highlight)->getActive());
			break;
	    }
	    case UI::Keys::ESC: {
			m_current = m_parent;
			return true;
			break;
		}
	    case UI::Keys::ENTER: {
			(*m_highlight)->select();
			return true;
			break;
	    }
	    default: {
			return false;
			break;
	    }
	}
	return false;
}


Menu* Menu::m_current = 0;

void Menu::drawInBar(int x, bool fHighlight)
{
	if (fHighlight) attron(A_REVERSE);
	mvaddstr(0, x, getTitle().c_str());
	if (fHighlight) attroff(A_REVERSE);
}

void MenuBaseItem::draw_menu(bool fHighlight, const int y0, const int x0, const int w, const string& key)
{
	int x = x0;
	if (fHighlight) {
		mvaddstr(y0, x++, "\u2590");
		attron(A_REVERSE);
	} else {
		mvaddch(y0, x++, ACS_VLINE);
	}
	mvaddstr(y0, x, getTitle().c_str());
	x += utf8::distance(getTitle().begin(), getTitle().end());
	if (!key.empty()) {
		int key_width = utf8::distance(key.begin(), key.end());
		while (x < x0 + w - key_width - 1) {
			mvaddch(y0, x++, ' ');
		}
		mvaddstr(y0, x, key.c_str());
		x += key_width;
	}
	while (x < x0 + w - 1) {
		mvaddch(y0, x++, ' ');
	}
	if (fHighlight) {
		attroff(A_REVERSE);
		mvaddstr(y0, x++, "\u258c");
	} else {
		mvaddch(y0, x, ACS_VLINE);
	}
}

void Menu::select()
{
	Menu::m_current = this;

	if (m_parent) {
		m_x0 = m_parent->m_x0 + m_parent->m_width + 2;
		m_y0 = distance(m_parent->m_items.begin(),
						find(m_parent->m_items.begin(),
							 m_parent->m_items.end(),
							 MenuBasePtr(this))) + 1;
	}
	else {
		m_x0 = m_xbar;
		m_y0 = 1;
	}

	m_highlight = m_items.end();
	for ( auto it = m_items.begin(); it != m_items.end(); ++it) {
		if ((*it)->getActive()) {
			m_highlight = it;
			break;
		}
	}
	m_width = -1;
	for ( auto m : m_items ) {
		m_width = max(m_width, m->getWidth());
	}
	m_width += 2;
	m_height += m_items.size() + 2;
}

bool Menu::isCurrent()
{
	return this == m_current;
}

void Menu::refresh_window()
{
	int y = m_y0;
	mvaddch(y, m_x0, ACS_ULCORNER);
	for (int x = m_x0 + 1; x < m_x0 + m_width - 1; ++x) mvaddch(y, x, ACS_HLINE);
	mvaddch(y, m_x0 + m_width - 1, ACS_URCORNER);
	++y;
	for ( auto it = m_items.begin(); it != m_items.end(); ++it) {
		(*it)->draw((it == m_highlight), y++, m_x0, m_width);
	}
	mvaddch(y, m_x0, ACS_LLCORNER);
	for (int x = m_x0 + 1; x < m_x0 + m_width - 1; ++x) mvaddch(y, x, ACS_HLINE);
	mvaddch(y, m_x0 + m_width - 1, ACS_LRCORNER);
}

void MenuLine::draw(bool, const int y0, const int x0, const int w)
{
	int x = x0;
	mvaddch(y0, x++, ACS_LTEE);
	while (x < x0 + w - 1) mvaddch(y0, x++, ACS_HLINE);
	mvaddch(y0, x, ACS_RTEE);
}

int Menu::calc_width()
{
	return utf8::distance(m_title.begin(), m_title.end()) + 2;
}

int MenuItem::calc_width()
{
	return utf8::distance(m_title.begin(), m_title.end()) +
		   utf8::distance(m_key.begin(), m_key.end()) + 2;
}

const string MenuLine::m_line_title = "_line_";
