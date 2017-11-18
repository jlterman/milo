#ifndef __UI_H
#define __UI_H

/* Copyright (C) 2017 - James Terman
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
 * @file ui.h
 * This file contains the common declerations for the abstract user interface of 
 * milo. Key, menus and mouse events are defined in a neutral way so that it can
 * be ported.
 */

#include <unordered_map>
#include <string>
#include <memory>
#include "util.h"
#include "milo.h"

/**
 * User Interface for milo namespace.
 * This is the user interface for milo for porting to paticular interfaces.
 */
namespace UI {
	class Graphics;
	class GlobalContext;

	/** @name UI namespace Type Declerations  */
	//@{
	using GraphicsPtr = std::shared_ptr<Graphics>;           ///< Shared pointer for Graphics
	using GlobalContextPtr = std::shared_ptr<GlobalContext>; /// Shared pointer for GlobalContext
	//@}
	
	/**
	 * The values under 0x80 are just the ACCII character set. Above 0x80 are 
	 * the special keyboard keys.
	 */
	enum Keys {
		NO_KEY, CTRL_A, CTRL_B, CTRL_C, CTRL_D, CTRL_E, CTRL_F, CTRL_G, CTRL_H,
		TAB, ENTER, CTRL_K, CTRL_L, CTRL_M, CTRL_N, CTRL_O, CTRL_P, CTRL_Q,
		CTRL_R, CTRL_S, CTRL_T, CTRL_U,	CTRL_V, CTRL_W, CTRL_X, CTRL_Y, CTRL_Z, ESC,
		SPACE = 32, BANG, DBL_QUOTE, HASH, DOLLAR, PRCT, AMP, QUOTE, L_PAR, R_PAR,
		STAR, PLUS, COMMA, MINUS, DOT, DIVIDE, K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
		COLON, SEMI, LESS, EQUAL, GREATER, QUEST, AT, A, B, C, D, E, F, G, H, I, J, K,
		L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, L_BRACKET, B_SLASH, R_BRACKET,
		POWER, U_SCORE, ACCENT, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r,
		s, t, u, v, w, x, y, z, L_BRACE, PIPE, R_BRACE, TILDE,
		F1 = 0x80, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, UP, DOWN, LEFT, RIGHT, BSPACE
	};

	/**
	 * All possible mouse events.
	 */
	enum Mouse {
		NO_MOUSE, POSITION, PRESSED, RELEASED, CLICKED, DOUBLE
	};

	/**
	 * Modifier key states for mouse and key events.
	 */
	enum Modifiers {
		NO_MOD, ALT, SHIFT, ALT_SHIFT, CTRL, CRL_ALT, CTRL_SHIFT, CTRL_ALT_SHIFT
	};

	/**
	 * Milo UI Mouse Event.
	 * All milo UI mouse events are uniquely stored in this class. It is designed
	 * to uniquely designate an event for the unordered map of mouse events to 
	 * function calls.
	 */
	class MouseEvent
	{
	public:
		/** @name Constructors */
		//@{
		/**
		 * Universal event constructor.
		 * @param t       Mouse press type
		 * @param b       Button number pressed
		 * @param m       Modifiers key states.
		 * @param mouse_x mouse x coordinate.
		 * @param mouse_y mouse y coordinate.
		 */
	    MouseEvent(enum Mouse t, int b, enum Modifiers md = NO_MOD, int mouse_x = -1, int mouse_y = -1) :
		    m_type{t}, m_button{b}, m_mod{md}, m_x{mouse_x}, m_y{mouse_y} {}
		//@}

		enum Mouse getMouse() const { return m_type; }         ///< @return Mouse click type
		int getButton() const { return m_button; }             ///< @return button pressed
		enum Modifiers getModifiers() const { return m_mod; }  ///< @return Modifiers state.

		/**
		 * Get mouse coordinates
		 * @param[out] mouse_x x-coordinate of mouse event
		 * @param[out] mouse_y y-coordinate of mouse event
		 */
        void getCoords(int& mouse_x, int& mouse_y) const { mouse_x = m_x; mouse_y = m_y; }

		/**
		 * Set mouse coordinates
		 * @param mouse_x x-coordinate of mouse event
		 * @param mouse_y y-coordinate of mouse event
		 */
        void setCoords(int mouse_x, int mouse_y) { m_x = mouse_x; m_y = mouse_y; }
		
		/**
		 * Calculate hash for this class object. Used by unordered map.
		 * @return Hash value.
		 */
		std::size_t hash() const
		{
			return hash_calculate<int>({(int) m_type, m_button, (int) m_mod});
		}		

		/**
		 * Equal operator override.
		 * Used by unordered map.
		 */
		bool operator==(const MouseEvent& e) const
		{
			return e.m_type   == m_type &&
			       e.m_button == m_button &&
			       e.m_mod    == m_mod;
		}

		MouseEvent& operator=(const MouseEvent& e)
		{
			m_type   = e.m_type;
			m_button = e.m_button;
			m_mod    = e.m_mod;
			m_x      = e.m_x;
			m_y      = e.m_y;
			return *this;
		}
		
		/**
		 * Create string representing event.
		 * @return String representing event
		 */
		std::string toString() const;
		
	private:
		enum Mouse m_type;     ///< Mouse click type.
		int m_button;          ///< Number of button pressed
		enum Modifiers m_mod;  ///< Modifiers state.
		int m_x;               ///< mouse x-coordinate.
		int m_y;               ///< mouse y-coordinate.
	};
	
	/**
	 * Milo UI Key Event.
	 * All milo UI key events are uniquely stored in this class. It is designed
	 * to uniquely designate an event for the unordered map of events to 
	 * function calls.
	 */
	class KeyEvent
	{
	public:
		/** @name Constructors */
		//@{
		/**
		 * Universal event constructor.
		 * @param key Key code.
		 * @param m   Modifiers for key.
		 */
	    KeyEvent(enum Keys key, enum Modifiers md = NO_MOD) : m_key{key}, m_mod{md} {}
		
		/**
		 * Default key event constructor.
		 * Construct an event for ascii code with no modifier keys.
		 * @param key Ascii character.
		 */
        KeyEvent(char key) : KeyEvent((Keys) key) {}
		//@}
		
		enum Keys getKey() const { return m_key; }             ///< @return Key code.
		enum Modifiers getModifiers() const { return m_mod; }  ///< @return Modifiers state.
		
		/**
		 * Calculate hash for this class object. Used by unordered map.
		 * @return Hash value.
		 */
		std::size_t hash() const
		{
			return hash_calculate<int>({(int) m_key, (int) m_mod});
		}

		/**
		 * Equal operator override.
		 * Used by unordered map.
		 */
		bool operator==(const KeyEvent e) const
		{
			return e.m_key == m_key &&
			       e.m_mod == m_mod;
		}
		
		/**
		 * Create string representing event.
		 * @return String representing event
		 */
		std::string toString() const;
		
	private:
		enum Keys  m_key;     ///< Key code.
		enum Modifiers m_mod; ///< Modifiers state.
	};

	/**
	 * Abstract base class to provide a context free graphical interface.
	 * Provides an interface of helper functions that allow nodes to draw themselves
	 * on the current graphical interface.
	 */
	class Graphics
	{
	public:
		/** @name Constructor and Virtual Destructor */
		//@{
		/**
		 * Default constructor.
		 */
		Graphics() {}
		
		/**
		 * Abstract base class needs vertical destructor.
		 */
		virtual ~Graphics() {}
		//@}
		
		/**
		 * Predefined colors.
		 */
		enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };
		
		/**
		 * Predefined character attributes.
		 */
		enum Attributes { NONE=0, BOLD=1, ITALIC=2, BOLD_ITALIC=3 };
		
		/** @name Virtual Public Member Functions */
		//@{
		
		/**
		 * Draw differential of width x0 and height y0 with char variable name.
		 * @param x0 Horizontal origin of differential.
		 * @param y0 Vertical origin of differential.
		 * @param variable Name of variable of differential.
		 */
		virtual void differential(int x0, int y0, char variable)=0;
		
		/**
		 * Draw pair of parenthesis around a node of size x_size, y_size and origin x0,y0.
		 * @param x_size Horizontal size of both parenthesis.
		 * @param y_size Vertical size of parenthesis.
		 * @param x0 Horizontal origin of parenthesis.
		 * @param y0 Vertical origin of parenthesis.
		 */
		virtual void parenthesis(int x_size, int y_size, int x0, int y0)=0;
		
		/**
		 * Draw a horizontal line starting at x0,y0 length x_size.
		 * @param x_size Horizontal size of both line.
		 * @param x0 Horizontal origin of line.
		 * @param y0 Vertical origin of line.
		 */
		virtual void horiz_line(int x_size, int x0, int y0)=0;
		
		/**
		 * Draw a character at x,y with a color.
		 * @param x0 Horizontal origin of character.
		 * @param y0 Vertical origin of character.
		 * @param c  Character to be drawn at x0,y0.
		 * @param chrAttr Attribute of character.
		 * @param color Color of character.
		 */
		virtual void at(int x0, int y0, int c, Attributes chrAttr, Color color = BLACK)=0;
		
		/**
		 * Draw a string at x,y with a color.
		 * @param x0 Horizontal origin of line.
		 * @param y0 Vertical origin of line.
		 * @param s  String to be drawn at x0,y0.
		 * @param chrAttr Attribute of character.
		 * @param color Color of line.
		 */
		virtual void at(int x0, int y0, const std::string& s, Attributes chrAttr, Color color = BLACK)=0;
		
		/**
		 * Flush all drawing so far to graphic interface.
		 */
		virtual void out()=0;
		
		/**
		 * Get height of text in pixels.
		 * @return Height of text in pixels.
		 */
		virtual int getTextHeight()=0;
		
		/**
		 * Get length of string in pixels.
		 * @return Length of string in pixels.
		 */
		virtual int getTextLength(const std::string& s)=0;
		
		/**
		 * Get length of character in pixels.
		 * @return Length of character in pixels.
		 */
		virtual int getCharLength(char c)=0;
		
		/**
		 * Get width of a parenthesis for a given height in pixels.
		 * @return Width of parenthesis.
		 */
		virtual int getParenthesisWidth(int height = 1)=0;
		
		/**
		 * Get height of line in a division node.
		 * @return Height of division line.
		 */
		virtual int getDivideLineHeight()=0;
		
		/**
		 * Get height of differential.
		 * @param c Variable name of differential.
		 * @return Height of differential in pixels.
		 */
		virtual int getDifferentialHeight(char c)=0;
		
		/**
		 * Get width of differential.
		 * @param c Variable name of differential.
		 * @return Width of differential in pixels.
		 */
		virtual int getDifferentialWidth(char c)=0;
		
		/**
		 * Get vertical offset of differential.
		 * @param c Variable name of differential.
		 * @return Vertical offset of differential in pixels.
		 */
		virtual int getDifferentialBase(char c)=0;
		
		/**
		 * Set size and origin of this graphics window.
		 * @param x Horizontal size of both graphics window.
		 * @param y Vertical size of graphics window.
		 * @param x0 Horizontal origin of graphics window.
		 * @param y0 Vertical origin of graphics window.
		 */
		virtual void set(int x, int y, int x0 = 0, int y0 = 0) { 
			m_xSize = x; m_ySize = y; m_xOrig = x0, m_yOrig = y0;
		}

		/**
		 * Refresh screen
		 */
		virtual void refresh()=0;
		//@}
		
		/**
		 * Set size and origin of this graphics window from a rectangle.
		 * @param box Rectangle containing origin and size of window.
		 */
		void set(const Box& box) { 
			set(box.width(), box.height(), box.x0(), box.y0());
		}
		
		/**
		 * Select the area of size x,y at origin x0,y0.
		 * @param x Horizontal size of both selection area.
		 * @param y Vertical size of selection area.
		 * @param x0 Horizontal origin of selection area.
		 * @param y0 Vertical origin of selection area.
		 */
		virtual void setSelect(int x, int y, int x0, int y0) { m_select.set(x, y, x0, y0); }
		
		/**
		 * Set selection area from rectangle.
		 * @param box Rectangle containing origin and size of selection area.
		 */
		void setSelect(const Box& box) { 
			setSelect(box.width(), box.height(), box.x0(), box.y0());
		}
		
		/**
		 * Draw a pair of parenthesis inside the rectangle box.
		 * @param box Rectangle containing origin and size of parenthesis.
		 */
		void parenthesis(const Box& box) {
			parenthesis(box.width(), box.height(), box.x0(), box.y0());
		}
		
		/**
		 * Change the coordinates x,y to be relative to the graphic's window origin.
		 * @param[in,out] x Horizontal coordinate to be shifted.
		 * @param[in,out] y Vertical coordinate to be shifted.
		 */
		void relativeOrig(int& x, int& y) { x -= m_xOrig; y -= m_yOrig; }
	protected:
		int m_xSize;   ///< Horizontal size of graphic window.
		int m_ySize;   ///< Vertical size of graphic window.
		int m_xOrig;   ///< Horizontal origin of graphic window.
		int m_yOrig;   ///< Vertical origin of graphic window.
		Box m_select;  ///< Currently selected area.
	};

	/**
	 * Class that provides a Graphics class object, equation and equation
	 * undo list. The static member variable GlobalContext::current always
	 * points to the equation that has focus.
	 */
	class GlobalContext
	{
	public:
		/** @name Constructors */
		//@{

		/**
		 * Constructor with new equation.
		 */
	    GlobalContext(Graphics* gc);

		/**
		 * Constructor with input equation.
		 */
	    GlobalContext(Graphics* gc, std::istream& is);
		//@}

		/**
		 * Get graphics context.
		 * @return Graphics context object.
		 */
		Graphics& getGraphics() { return *m_gc; }

		/** 
		 * Get working equation.
		 * @return Working equation.
		 */
		Equation& getEqn() { return *m_eqn; }

		/**
		 * Save working equation and create new equation.
		 */
		void saveEqn();

		/**
		 * Undo working equation with last saved eqn in list.
		 */
		void undoEqn();
		
		static GlobalContextPtr current; ///< Shared pointer to current global context.
		
        Node* start_select = nullptr;    ///< If not null, node is selected.
        int start_mouse_x = -1;          ///< Horiz coord of start of mouse drag.
        int start_mouse_y = -1;          ///< Vertical coord of start of mouse drag
 
	private:
		GraphicsPtr m_gc;   ///< Shared pointer to current Graphics.
		EqnPtr      m_eqn;  ///< Shared pointer to current equation.
		EqnUndoList m_eqns; ///< Equation undo stack
	};

	/**
	 * Query if program should quit.
	 * @return If true, exit main loop.
	 */
	bool isRunning();
	
	/**
	 * Execute function based on its name. Used for menu handling.
	 * @return Equation has changed.
	 */
	bool doMenu(const std::string& menuFunctionName);

	/**
	 * Handle key event from main loop.
	 * @return Equation has changed.
	 */
	bool doKey(const KeyEvent& key);

	/**
	 * Handle mouse event from main loop.
	 * @return Equation has changed.
	 */
	bool doMouse(const MouseEvent& mouse);
}

namespace std
{
	/**
	 * Specialization hash for UI::KeyEvent.
	 */
	template<> struct hash<UI::KeyEvent>
	{
		size_t operator()(UI::KeyEvent const& key) const
		{
			return key.hash();
		}
	};

	/**
	 * Specialization hash for UI::MouseEvent.
	 */
	template<> struct hash<UI::MouseEvent>
	{
		size_t operator()(UI::MouseEvent const& key) const
		{
			return key.hash();
		}
	};
}

#endif // __UI_H
