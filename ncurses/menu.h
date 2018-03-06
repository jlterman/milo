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
 * @file ncurses/menu.h
 * This file defines the interface for the ncurses menu classes.
 */

#include <ncursesw/ncurses.h>
#include <locale.h>
#include <unordered_map>
#include <memory>
#include <utf8.h>
#include "ui.h"


/**
 * Abstract base menu class that all menu classes inherit from.
 */
class MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	MenuBaseItem(const StringMap& tags) :
		m_name(tags.at("name")),
		m_type(tags.at("type")),
		m_width(-1) {}
	virtual ~MenuBaseItem() {}
	//@}

	/** 
	 * Get name of the menu item
	 * @return Menu item name
	 */
	const std::string& getName() { return m_name; }

	/** 
	 * Get type of the menu item
	 * @return Menu item type
	 */
	const std::string& getType() { return m_type; }

	/** 
	 * Get width of menu item
	 * @return Menu item width
	 */
	int getWidth() { if (m_width == -1) m_width = calc_width(); return m_width; }	
	
	/** @name Pure Virtual Public Member Functions */
	//@{
	/**
	 * Select this menu item
	 */
	virtual void select()=0;
	
	/**
	 * Get title of menu
	 * @return Return title
	 */
	virtual const std::string& getTitle()=0;

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	virtual bool getActive()=0;

	/** 
	 * Draw menu title in drop down box at given coordinates
	 * @param fHighlight Highlight this item if true
	 * @param y0 y origin of item
	 * @param x0 x origin of item
	 * @param w width of item
	 */
	virtual void draw(bool fHighlight, const int y0, const int x0, const int w) = 0;
	//@}

protected:
	/** 
	 * Draw menu title in drop down box at given coordinates
	 * @param fHighlight Highlight this item if true
	 * @param y0 y origin of item
	 * @param x0 x origin of item
	 * @param w width of item
	 * @param key Key modifier (default none)
	 */
	void draw_menu(bool fHighlight, const int y0, const int x0, const int w, const std::string& key = std::string());

private:
	/** Private pure virtual member function to calc width of menu
	 */
	virtual int calc_width()=0;

	const std::string m_name; ///< menu name
	const std::string m_type; ///< menu type
	int m_width;              ///< menu width
};

using MenuBasePtr = std::shared_ptr<MenuBaseItem>; ///< Shared menu pointer for all menu classes
class MenuBar;

/**
 * Submenu class also for top menu.
 */
class Menu : public MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	Menu(const StringMap& tags) :
		MenuBaseItem(tags),
		m_active(tags.at("active") == "true"),
		m_title(tags.at("title")),
	    m_parent(nullptr) {}
	~Menu() {}
	//@}

	/** @name Overriden Pure Virtual Member Functions */
	//@{
	/**
	 * Draw menu title in menu bar
	 * @param Horizontal location of menu title
	 */
	void drawInBar(int x, bool fHighlight = false);

	/**
	 * Handle key event
	 * @param code Key code.
	 * @return Event was handled.
	 */
	bool handleKey(int code);

	/**
	 * Select this menu item
	 */
	void select();

	/**
	 * Get title of menu
	 * @return Return title
	 */
	const std::string& getTitle() { return m_title; }

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	bool getActive() { return m_active; }

	/** Calc width of menu
	 */
	int calc_width();

	/** 
	 * Draw menu title in drop down box at given coordinates
	 * @param fHighlight Highlight this item if true
	 * @param y0 y origin of item
	 * @param x0 x origin of item
	 * @param w width of item
	 */
	void draw(bool fHighlight, const int y0, const int x0, const int w) {
		draw_menu(fHighlight, y0, x0, w);
	}
	//@}

	/** 
	 * Add menu item to menu
	 * @param m Menu item to add menu
	 */
	void addItem(MenuBaseItem* m) { m_items.push_back(MenuBasePtr(m)); }

	/**
	 * Get parent menu
	 * @return Parent menu
	 */
	Menu* getParent() { return m_parent; }

	/**
	 * Highlight next item
	 * @param step direction of change
	 */
	void highlight_next(int step);

	/**
	 * Refresh menu window containing menu items.
	 */
	void refresh_window();

	/**
	 * Is this menu the current menu?
	 * @return True, if menu is current active menu
	 */
	bool isCurrent();
	
	friend class MenuBar;
private:
	bool m_active;                             ///< True if active
	const std::string m_title;                 ///< Title of menu
	std::vector<MenuBasePtr> m_items;          ///< List of menu items
	Menu* m_parent;                            ///< Parent menu. Menu bar if null.
	int m_xbar;                                ///< Position on menubar if needed.
	int m_wbar;                                ///< Width on menubar if needed.
	std::vector<MenuBasePtr>::iterator m_highlight; ///< Index of item highlighted.
	int m_x0;     ///< Horiz origin of menu
	int m_y0;     ///< Vertical origin of menu
	int m_width;  ///< Width of where menu
	int m_height; ///< Height of menu

	static Menu* m_current;      ///< current active menu
};

using MenuPtr = std::shared_ptr<Menu>; ///< shared pointer for class Menu

/**
 * Menu item class
 */
class MenuItem : public MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	MenuItem(const StringMap& tags) :
		MenuBaseItem(tags),
		m_active(tags.at("active") == "true"),
		m_title(tags.at("title")),
		m_action(tags.at("action")),
		m_key((tags.at("key") == "NONE") ? std::string() : tags.at("key")) {}
		
	~MenuItem() {}
	//@}

	/** @name Overriden Pure Virtual Member Functions */
	//@{
	/**
	 * Select this menu item
	 */
	void select() { UI::doMenu(m_action); }

	/**
	 * Get title of menu
	 * @return Return title
	 */
	const std::string& getTitle() { return m_title; }

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	bool getActive() { return m_active; }

	/** Calc width of menu
	 */
	int calc_width();

	/** 
	 * Draw menu title in drop down box at given coordinates
	 * @param fHighlight Highlight this item if true
	 * @param y0 y origin of item
	 * @param x0 x origin of item
	 * @param w width of item
	 */
	void draw(bool fHighlight, const int y0, const int x0, const int w) {
		draw_menu(fHighlight, y0, x0, w, m_key);
	}
	//@}

private:
	bool m_active;              ///< True if active
	const std::string m_title;  ///< Title of menu item
	const std::string m_action; ///< name of action of menu item
	const std::string m_key;    ///< Key binding of menu item
};

/**
 * Menu line item class
 */
class MenuLine : public MenuBaseItem
{
public:
	/** @name Constructor and Virtual Destructor */
	//@{
	MenuLine() : MenuBaseItem({ {"type", "line"}, {"name", "line"} }) {}
	~MenuLine() {}
	//@}

	/** @name Overriden Pure Virtual Member Functions */
	//@{
	/** 
	 * Draw menu on window at given coordinates
	 * @param fHighlight Highlight this item if true
	 * @param y0 y origin of item
	 * @param x0 x origin of item
	 * @param w width of item
	 */
	void draw(bool fHighlight, const int y0, const int x0, const int w);

	/**
	 * Select this menu item. Shouldn't be called.
	 */
	void select() {}

	/**
	 * Get title of menu
	 * @return Return title
	 */
	const std::string& getTitle() { return m_line_title; }

	/**
	 * Get active state of menu
	 * @return Return active state
	 */
	bool getActive() { return false; }

	/** Calc width of menu
	 */
	int calc_width() { return -1; }
	//@}
private:
	static const std::string m_line_title;
};

/**
 * Menubar class
 */

class MenuBar : public UI::MenuXML
{
public:
	/**
	 * Constructor for class MenuBar
	 * @param xml Filename of xml file for menubar
	 */
	MenuBar(const std::string& xml);

	/**
	 * Add menu to bar
	 * @param menu Pointer to new menu
	 */
	void add(Menu* menu);

	/**
	 * Draw menu bar
	 */
	void draw();

	/**
	 * Handle event loop for menu bar
	 * @param mouse_x Menu clicked (default first menu)
	 * @return Menu was selected
	 */
	bool select(int mouse_x = 1);

	/**
	 * Returns if menus are active
	 * @return m_root not null
	 */
	bool active() { return m_root != 0; }

	/** 
	 * Send key code to active menu
	 * @param code Key code
	 * @return True if key code was handled.
	 */
	bool handleKey(int code);

	/** @name Overridden Public Member Functions */
	//@{
	/**
	 * Start of menu with attributes
	 * @param attributes All menu settings in map.
	 */
	void define_menu(const StringMap& attributes);
	
	/**
	 * No more menu items for current menu
	 * @param name Name of menu coming to end
	 */
	void define_menu_end(const std::string& name);
	
	/**
	 * Menu item for current menu
	 * @param attributes All menu item settings in map.
	 */
	void define_menu_item(const StringMap& attributes);
	
	/**
	 * Menu line for current menu
	 */
	void define_menu_line();
	//@}

	/**
	 * Map ncurses keys to a name representing a function to call.
	 */
	static std::unordered_map<UI::KeyEvent, std::string> key_menu_map;
	
private:
    MenuPtr m_root;                ///< current menu on bar open
	int m_level;                   ///< Keep track of menu level
	std::vector<MenuPtr> m_menus;  ///< vector of menus in menu bar
	std::stack<Menu*> m_menu_heap; ///< Stack of menus being created
};
