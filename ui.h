#ifndef __UI_H
#define __UI_H

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
 * @file ui.h
 * This file contains the common declerations for the abstract user interface of 
 * milo. Key, menus and mouse events are defined in a neutral way so that it can
 * be ported.
 */

#include <fstream>
#include <unordered_map>
#include <string>
#include <memory>
#include "util.h"
#include "xml.h"

/**
 * User Interface for milo namespace.
 * This is the user interface for milo for porting to paticular interfaces.
 */
namespace UI {
	// Forward class declerations
	class Graphics;
	class MiloApp;
	class MiloWindow;
	class MiloPanel;

	namespace Keys {
		/**
		 * The values under 0x80 are just the ACCII character set. Above 0x80 are 
		 * the special keyboard keys.
		 */
		enum Key_Values {
			NONE, CTRL_A, CTRL_B, CTRL_C, CTRL_D, CTRL_E, CTRL_F, CTRL_G, CTRL_H,
			TAB, ENTER, CTRL_K, CTRL_L, CTRL_M, CTRL_N, CTRL_O, CTRL_P, CTRL_Q,
			CTRL_R, CTRL_S, CTRL_T, CTRL_U,	CTRL_V, CTRL_W, CTRL_X, CTRL_Y, CTRL_Z, ESC,
			SPACE = 32, BANG, DBL_QUOTE, HASH, DOLLAR, PERCENT, AMP, QUOTE, L_PAR, R_PAR,
			STAR, PLUS, COMMA, MINUS, DOT, DIVIDE, K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
			COLON, SEMI, LESS, EQUAL, GREATER, QUESTION, AT, A, B, C, D, E, F, G, H, I, J, K,
			L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, L_BRACKET, B_SLASH, R_BRACKET,
			POWER, U_SCORE, ACCENT, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r,
			s, t, u, v, w, x, y, z, L_BRACE, PIPE, R_BRACE, TILDE,
		    F1 = 0x80, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
			INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, UP, DOWN, LEFT, RIGHT, BSPACE
		};
	}

	/** @name UI namespace Type Declerations  */
	//@{
    using Keyboard = Keys::Key_Values;

	/**
	 * Function pointer to handle a menu item.
	 * Return true, if event changed panel.
	 */
	using menu_handler = void (*)(void);
	
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
	//@}

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

		/**
		 * Set mouse coordinates
		 * @param mouse_x x-coordinate of mouse event
		 * @param mouse_y y-coordinate of mouse event
		 */
        void setCoords(int mouse_x, int mouse_y) { m_x = mouse_x; m_y = mouse_y; }

		/** @name MouseEvent class accessors */
		//@{
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
		 * Check mouse event for no mouse modifiers.
		 * @return True if there are no mouse modifiers.
		 */
		bool noMod() const { return (m_mod == 0) ? true : false; }

		/**
		 * Check mouse event for shift mouse modifier.
		 * @return True if there is shift mouse modifier.
		 */
		bool shiftMod() const { return (m_mod && Modifiers::SHIFT != 0) ? true : false; }

		/**
		 * Check mouse event for alt mouse modifier.
		 * @return True if there is alt mouse modifier.
		 */
		bool altMod() const { return (m_mod && Modifiers::ALT != 0) ? true : false; }
		
		/**
		 * Return true if valid mouse
		 */
		operator bool() const { return m_type !=NO_MOUSE; }
		//@}

		/** @name Help member functions */
		//@{
		/**
		 * Calculate hash for this class object. Used by unordered map.
		 * @return Hash value.
		 */
		std::size_t hash() const
		{
			return hash_calculate<int>({(int) m_type, m_button, (int) m_mod});
		}		

		/**
		 * Return true is this mouse event is equal to parameter
		 * @param e Mouse event to compare
		 * @return True if equal
		 */
		bool equals(const MouseEvent& e) const
		{
			return e.m_type   == m_type &&
			       e.m_button == m_button &&
			       e.m_mod    == m_mod;
		}

		/**
		 * Assign value of mouse event paramter
		 * @param e Mouse event to assign
		 * @return Reference to this mouse event
		 */
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
		//@}
		
	private:
		enum Mouse m_type;     ///< Mouse click type.
		int m_button;          ///< Number of button pressed
		enum Modifiers m_mod;  ///< Modifiers state.
		int m_x;               ///< mouse x-coordinate.
		int m_y;               ///< mouse y-coordinate.
	};

	/**
	 * Overloading == operator for class MouseEvent
	 * @param m1 First mouse event
	 * @param m2 Second mouse event
	 * @return True if m1 == m2
	 */
	inline bool operator==(const MouseEvent& m1, const MouseEvent& m2)
	{
		return m1.equals(m2);
	}
	
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
	    KeyEvent(Keyboard key, enum Modifiers md = NO_MOD) : m_key{key}, m_mod{md} {}
		
		/**
		 * Default key event constructor.
		 * Construct an event for ascii code with no modifier keys.
		 * @param key Ascii character.
		 */
	    KeyEvent(char key, enum Modifiers md = NO_MOD) : KeyEvent((Keyboard) key, md) {}

		/**
		 * Key event constructor that reads key event from string of format
		 * [[[CTRL]_ALT]_SHIFT]-letter.
		 */
		KeyEvent(const std::string& key);

		/**
		 * Copy constructor that can be a constexpr
		 */
	    constexpr KeyEvent(const KeyEvent& e) : m_key(e.m_key), m_mod(e.m_mod) {}
		//@}
		
		/** @name KeyEvent class accessors */
		//@{
		Keyboard getKey() const { return m_key; }              ///< @return Key code.
		enum Modifiers getModifiers() const { return m_mod; }  ///< @return Modifiers state.

		/**
		 * Check key event for no key modifiers.
		 * @return True if there are no key modifiers.
		 */
		bool noMod() const { return (m_mod == 0) ? true : false; }

		/**
		 * Check key event for shift key modifier.
		 * @return True if there is shift key modifier.
		 */
		bool shiftMod() const { return (m_mod && Modifiers::SHIFT != 0) ? true : false; }

		/**
		 * Check key event for alt key modifier.
		 * @return True if there is alt key modifier.
		 */
		bool altMod() const { return (m_mod && Modifiers::ALT != 0) ? true : false; }
		
		/**
		 * Return true if valid key
		 */
		operator bool() const { return m_key != Keys::NONE; }
		//@}

		/** @name Help member functions */
		//@{
		/**
		 * Calculate hash for this class object. Used by unordered map.
		 * @return Hash value.
		 */
		std::size_t hash() const
		{
			return hash_calculate<int>({(int) m_key, (int) m_mod});
		}

		/**
		 * Return true is this key event is equal to parameter
		 * @param e Key event to compare
		 * @return True if equal
		 */
		bool equals(const KeyEvent e) const
		{
			return e.m_key == m_key &&
			       e.m_mod == m_mod;
		}

		/**
		 * Assign value of key event paramter
		 * @param e Key event to assign
		 * @return Reference to this key event
		 */
		KeyEvent& operator=(const KeyEvent& e)
		{
			m_key = e.m_key;
			m_mod = e.m_mod;
			return *this;
		}
		
		/**
		 * Create string representing event.
		 * @return String representing event
		 */
		std::string toString() const;
		//@}
	
	private:
		Keyboard m_key;       ///< Key code.
		enum Modifiers m_mod; ///< Modifiers state.
	};

	/**
	 * Overloading == operator for class KeyEvent
	 * @param m1 First key event
	 * @param m2 Second key event
	 * @return True if k1 == k2
	 */
	inline bool operator==(const KeyEvent& k1, const KeyEvent& k2)
	{
		return k1.equals(k2);
	}

	/**
	 * Abstract base class to provide a context free graphical interface.
	 * Provides an interface of helper functions that allow nodes to draw themselves
	 * on the current graphical interface.
	 */
	class Graphics
	{
	public:
		using Ptr = std::unique_ptr<Graphics>;  ///< Unique pointer for Graphics.
		
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
		virtual void differential(int x0, int y0, char variable) = 0;
		
		/**
		 * Draw pair of parenthesis around a node of size x_size, y_size and origin x0,y0.
		 * @param x_size Horizontal size of both parenthesis.
		 * @param y_size Vertical size of parenthesis.
		 * @param x0 Horizontal origin of parenthesis.
		 * @param y0 Vertical origin of parenthesis.
		 */
		virtual void parenthesis(int x_size, int y_size, int x0, int y0) = 0;
		
		/**
		 * Draw a horizontal line starting at x0,y0 length x_size.
		 * @param x_size Horizontal size of both line.
		 * @param x0 Horizontal origin of line.
		 * @param y0 Vertical origin of line.
		 */
		virtual void horiz_line(int x_size, int x0, int y0) = 0;
		
		/**
		 * Draw a character at x,y with a color.
		 * @param x0 Horizontal origin of character.
		 * @param y0 Vertical origin of character.
		 * @param c  Character to be drawn at x0,y0.
		 * @param chrAttr Attribute of character.
		 * @param color Color of character.
		 */
		virtual void at(int x0, int y0, int c, Attributes chrAttr, Color color = BLACK) = 0;
		
		/**
		 * Draw a string at x,y with a color.
		 * @param x0 Horizontal origin of line.
		 * @param y0 Vertical origin of line.
		 * @param s  String to be drawn at x0,y0.
		 * @param chrAttr Attribute of character.
		 * @param color Color of line.
		 */
		virtual void at(int x0, int y0, const std::string& s, Attributes chrAttr, Color color = BLACK) = 0;

		/**
		 * Clear screen.
		 */
		virtual void clear_screen() = 0;
		
		/**
		 * Flush all drawing so far to graphic interface.
		 */
		virtual void out() = 0;
		
		/**
		 * Get height of text in pixels.
		 * @return Height of text in pixels.
		 */
		virtual int getTextHeight() = 0;
		
		/**
		 * Get length of string in pixels.
		 * @return Length of string in pixels.
		 */
		virtual int getTextLength(const std::string& s) = 0;
		
		/**
		 * Get length of character in pixels.
		 * @return Length of character in pixels.
		 */
		virtual int getCharLength(char c) = 0;
		
		/**
		 * Get width of a parenthesis for a given height in pixels.
		 * @return Width of parenthesis.
		 */
		virtual int getParenthesisWidth(int height = 1) = 0;
		
		/**
		 * Get height of line in a division node.
		 * @return Height of division line.
		 */
		virtual int getDivideLineHeight() = 0;
		
		/**
		 * Get height of differential.
		 * @param c Variable name of differential.
		 * @return Height of differential in pixels.
		 */
		virtual int getDifferentialHeight(char c) = 0;
		
		/**
		 * Get width of differential.
		 * @param c Variable name of differential.
		 * @return Width of differential in pixels.
		 */
		virtual int getDifferentialWidth(char c) = 0;
		
		/**
		 * Get vertical offset of differential.
		 * @param c Variable name of differential.
		 * @return Vertical offset of differential in pixels.
		 */
		virtual int getDifferentialBase(char c) = 0;
		
		/**
		 * Select the area of size x,y at origin x0,y0.
		 * @param x Horizontal size of both selection area.
		 * @param y Vertical size of selection area.
		 * @param x0 Horizontal origin of selection area.
		 * @param y0 Vertical origin of selection area.
		 */
		virtual void setSelect(int x, int y, int x0, int y0) { m_select.set(x, y, x0, y0); }
		//@}

		/** @name Public helper member functions. */
		//@{
		/**
		 * Get size of graphics area..
		 * @return Box containing drawing area.
		 */
		Box getBox() const { return m_frame; }

		/**
		 * Set size and origin of this graphics window.
		 * @param x Horizontal size of both graphics window.
		 * @param y Vertical size of graphics window.
		 * @param x0 Horizontal origin of graphics window.
		 * @param y0 Vertical origin of graphics window.
		 */
		void set(int x, int y, int x0 = 0, int y0 = 0) {
			m_frame.set(x, y, x0, y0);
		}

		/**
		 * Set size and origin of this graphics window from a rectangle.
		 * @param box Rectangle containing origin and size of window.
		 */
		void set(const Box& box) { 
			m_frame = box;
		}
		
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
		 * Change the coordinates x,y to be relative to the graphic context frame
		 * @param[in,out] x Horizontal coordinate to be shifted.
		 * @param[in,out] y Vertical coordinate to be shifted.
		 */
		void localOrig(int& x, int& y) const { x -= m_frame.x0(); y -= m_frame.y0(); }
		
		/**
		 * Change the coordinates x,y to be relative to the window 
		 * containing the graphic context;
		 * @param[in,out] x Horizontal coordinate to be shifted.
		 * @param[in,out] y Vertical coordinate to be shifted.
		 */
		void globalOrig(int& x, int& y) const { x += m_frame.x0(); y += m_frame.y0(); }
		//@}

	protected:
		Box m_frame;   ///< Frame of this panel.
		Box m_select;  ///< Currently selected area.
	};

	/**
	 * EventBox is an pure abstract base class interface that
	 * provides hooks into the UI for event handling and drawing.
	 */
	class EventBox
	{
	public:
		using Ptr    = std::unique_ptr<EventBox>;  ///< Unique pointer for EventBox
		using Vector = std::vector<EventBox::Ptr>; ///< Storage of EventBox pointers
		using Iter   = EventBox::Vector::iterator; ///< Iterator of EventBox ptr vector

		/** Constructor for EventBox base class.
		 */
	    EventBox();

		/** @name Pure Virtual Public Member Functions */
		//@{		
		/**
		 * Handle key event.
		 * @param key Key event.
		 */
		virtual void doKey(const UI::KeyEvent& key) = 0;

		/**
		 * Handle mouse event.
		 * @param mouse Mouse event.
		 */
		virtual void doMouse(const UI::MouseEvent& mouse) = 0;

		/**
		 * Execute specific function based on its name. Used for menu handling.
		 * @param menuFunctionName Name of menu function to be executed.
		 * @return True if menuFunctionName found.
		 */
		virtual bool doMenu(const std::string& menuFunctionName) = 0;
		
		/** Draw contents on window.
		 */
		virtual void doDraw() = 0;

		/** 
		 * Calculate size needed for doDraw(). .
		 * @return Calculated size for drawing.
		 */
		virtual Box calculateSize() = 0;

		/** 
		 * Get last calculated size.
		 * @return Last calculated size.
		 */
		virtual Box getSize() = 0;

		/**
		 * Get base line relative to top of box.
		 * @return Base line.
		 */
		virtual int getBase() = 0;

		/**
		 * Check where there is an active input.
		 * @return If true, there is an active input.
		 */
		virtual bool blink() = 0;

		/**
		 * Get coordinates of current active input.
		 * @param[out] x Horizontal origin of curosr.
		 * @param[out] y Vertical origin cursor.
		 */
		virtual void getCursorOrig(int& x, int& y) = 0;
		//@}

		/** @name Public helper member functions */
		//@{
		/**
		 * Set origin of this Event Box
		 * @param x0 Horizontal origin of graphics window.
		 * @param y0 Vertical origin of graphics window.
		 */
		void setOrigin(int x0, int y0) {
			Box b = m_gc->getBox();
			b.x0() = x0; b.y0() = y0;
			m_gc->set(b);
		}

		/**
		 * Get graphics box
		 * @return Rectangle containing origin and size of graphics
		 */
		Box getGraphicsBox() {
			return m_gc->getBox();
		}

		/**
		 * Check if event box has changed.
		 * @return True if event box changed.
		 */
		bool hasChanged() { return m_fChange; }
		//@}

	protected:
		Graphics::Ptr m_gc;  ///< Owned Graphics object for event box.
		bool m_fChange;      ///< True if state of event box has changed.
	};
	
	/** MiloPanel is an abstract base class for all milo panels.
	 */
	class MiloPanel : public EventBox
	{
	public:
		friend class MiloApp;

		/**
		 * Template function to create a panel of a particular type
		 *
		 * @param init String to initialize panel
		 * @return Pointer to panel
		 */
		template <class T>
		static MiloPanel* create(const std::string& init)
		{
			return new T(init);
		}

		/**
		 * Template function to create a panel of a particular type from XML
		 *
		 * @param in XML parser object.
		 * @return Pointer to panel
		 */
		template <class T>
		static MiloPanel* createXML(XML::Parser& in)
		{
			return new T(in);
		}

		/** Function pointer to return panel object with initilization string and Graphics object
		 */
		using factory = MiloPanel* (*)(const std::string&);

		/** Function pointer to return panel object with xml parser and Graphics object
		 */
		using factory_xml = MiloPanel* (*)(XML::Parser&);

		using Ptr    = std::unique_ptr<MiloPanel>;  ///< Unique pointer for MiloPanel
		using Vector = std::vector<MiloPanel::Ptr>; ///< Storage of MiloPanel pointers
		using Iter   = MiloPanel::Vector::iterator; ///< Iterator of MiloPanel ptr vector

		static constexpr const char* tag = "panel"; ///< xml tag for panel.
		const std::string type_tag = "type";        ///< xml type for <panel>.
		const std::string active_tag = "active";    ///< name/value tag.
		
		/**
		 * Function pointer to handle menu event.
		 * Return true, if event changed panel.
		 */
		using menu_handler = void (*)(MiloPanel&);

		/** @name Constructors and Virtual Destructor */
		//@{
		/** Constructor for MiloPanel base class.
		 */
	    MiloPanel() : EventBox() {}

		/** Virtual deconstructor for MiloPanel initializing graphics context
		 */
        virtual ~MiloPanel() {}
		//@}

		/** @name Pure Virtual Public Member Functions */
		//@{
		/** 
		 * Copy panel sate from XML paraser
		 * @param in XML parser object.
		 */
	    virtual void copy(XML::Parser& in) = 0;

		/**
		 * Output contents as xml to XML stream.
		 * @param XML stream class object.
		 */
		virtual void xml_out(XML::Stream& xml) = 0;

		/**
		 * Execute specific function based on its name. Used for menu handling.
		 * @param menuFunctionName Name of menu function to be executed.
		 * @return True if menuFunctionName found.
		 */
		virtual bool doPanelMenu(const std::string& menuFunctionName) = 0;

		/**
		 * Set size and origin of the graphics of this panel
		 * @param x Horizontal size ofgraphics.
		 * @param y Vertical size of graphics.
		 * @param x0 Horizontal origin of graphics.
		 * @param y0 Vertical origin of graphics.
		 */
		virtual void setBox(int x, int y, int x0, int y0) = 0;

		/**
		 * Get type name of panel.
		 * @return String containing type of panel for xml tag.
		 */
		virtual const std::string& getType() = 0;
		//@}
		
		/** @name Public helper member functions */
		//@{
		/**
		 * Get box containing graphics context for this panel.
		 * @return Box enclosing graphics context.
		 */
		Box getBox() { return m_gc->getBox(); }
		
		/**
		 * Serialize contents of panel to a string.
		 * @param str String to store contents.
		 */
		void xml_out(std::string& str);

		/**
		 * Copy panel state from string containing serialized contents.
		 * @param str Serialized new contents for panel.
		 */
		void copy(const std::string& str);
		
		/**
		 * Push state to undo stack.
		 */
		void pushUndo();
		
		/**
		 * Restore state from top of undo stack.
		 */
		void doUndo();

		/** 
		 * Undo a previous undo.
		 */
		void doRedo();

		/**
		 * Execute specific function based on its name. Used for menu handling.
		 * @param menuFunctionName Name of menu function to be executed.
		 * @return True if menuFunctionName found.
		 */
		bool doMenu(const std::string& menuFunctionName);

		/**
		 * Get graphics context.
		 * @return Graphics context object.
		 */
		Graphics& getGraphics() { return *m_gc; }

		/**
		 * Output panel as xml to XML stream.
		 * @param XML stream class object.
		 * @return XML stream object.
		 */
		XML::Stream& out(XML::Stream& xml);
		//@}
		
		/** 
		 * Get a smart pointer to a panel class object by name.
		 * @param name Name of panel class object to create
		 * @param init Initilization xml string
		 * @return Pointer to panel class object
		 */
		static MiloPanel::Ptr make(const std::string& name,
								   const std::string& init);
		
		/** 
		 * Get a smart pointer to a panel class object by name with xml.
		 * @param in XML parser to load panel.
		 * @param[out] fActive True, if this is active panel
		 * @return Pointer to panel class object.
		 */
		static MiloPanel::Ptr make(XML::Parser& in);
		
	protected:
		/** Map of name to panel create functions.
		 */
		static std::unordered_map<std::string, factory> panel_map;

		/** Map of name to panel create functions with xml.
		 */
		static std::unordered_map<std::string, factory_xml> panel_xml_map;

	private:
		StringVector m_undo; ///< Stack of undo history.
		int m_current = -1;  ///< Current place in undo history.

		/** Handle menu items for all panels.
		 */
		static std::unordered_map<std::string, menu_handler> menu_map;
	};

	/**
	 * Milo Window class that contains a list of panels.
	 */
	class MiloWindow
	{
	public:
		friend class MiloApp;

		using Ptr    = std::unique_ptr<MiloWindow>;  ///< Unique pointer for MiloWindow
		using Vector = std::vector<MiloWindow::Ptr>; ///< Storage of MiloWindow pointers
		using Iter   = MiloWindow::Vector::iterator; ///< Iterator of MiloWindows ptr vector
		
		const std::string title_tag = "title";   ///< title xml tag.
		const std::string active_tag = "active"; ///< active xml tag.

		/** @name Constructors and Virtual Destructor */
		//@{

		/**
		 * Constructor with new panel.
		 * @param name Name of panel class.
		 * @param init Initialization string.
		 */
	    MiloWindow(const std::string& name,
				   const std::string& init);

		/**
		 * Constructor from XML::Parser.
		 * @param in XML::Parser object
		 */
		MiloWindow(XML::Parser& in, const std::string& fname);
			
		/**
		 * Abstract base class needs virtual destructor.
		 */
		virtual ~MiloWindow() {}
		//@}

		/** @name Public helper member functions. */
		//@{
		/** 
		 * Get active panel.
		 * @return Active panel.
		 */
		MiloPanel& getPanel() { return *(m_current_panel->get()); }

		/** 
		 * Get active panel iterator.
		 * @return Active panel.
		 */
		MiloPanel::Iter getPanelIter() { return m_current_panel; }

		/**
		 * Add new panel to window after current panel.
		 * @param panel New panel added to window.
		 */
		void addPanel(MiloPanel* panel) {
			m_current_panel = AddPtrAfter(m_panels, m_current_panel, panel);
		}

		/**
		 * Delete panel from window.
		 * @param it Iterator to panel to be deleted.
		 */
		void deletePanel(MiloPanel::Iter it) {
			m_current_panel = m_panels.erase(it);
			if (m_current_panel == m_panels.end()) {
				m_current_panel = m_panels.end() - 1;
			}
		}

		/**
		 * Delete current panel from window.
		 */
		void deletePanel() { deletePanel(m_current_panel); }

		/**
		 * Get iterator of panel
		 * @param panel MiloPanel reference.
		 * @return Iterator of panel.
		 */
		MiloPanel::Iter getPanelIterator(const MiloPanel& panel) {
			for ( auto it = m_panels.begin(); it != m_panels.end(); ++it ) {
				if (&panel == it->get()) {
					return it;
				}
			}
			return m_panels.end();;
		}

		/**
		 * Set panel to be current active panel.
		 * @param panel MiloPanel reference.
		 */
		void setActivePanel(MiloPanel& panel) {
			m_current_panel = getPanelIterator(panel);
		}

		/**
		 * Step through panel list
		 * @param dir Postive if true.
		 */
		void stepPanel(bool dir = true);

		/**
		 * Load window from input xml stream.
		 * @param in Input XML stream.
		 */
		void xml_in(XML::Parser& in);

		/**
		 * Output window as xml to XML stream.
		 * @param XML stream class object.
		 * @return XML stream object.
		 */
		XML::Stream& out(XML::Stream& xml);

		/** Save window as xml to a new filename.
		 * @param fname Name of new file.
		 */
		void save(const std::string& fname) {
			m_filename = fname;
			save();
		}
		
		/** Save window as xml to file m_filename.
		 */
		void save() {
			std::ofstream ofs(m_filename);
			XML::Stream xml(ofs, "document");
			xml << *this;
		}
		 
		/**
		 * Return iterator of first panel.
		 * @return Iterator of first panel.
		 */
		MiloPanel::Iter begin() { return m_panels.begin(); }

		/**
		 * Return iterator of end panel iterator.
		 * @return End panel iterator.
		 */
		MiloPanel::Iter end() { return m_panels.end(); }
		//@}
	protected:
		MiloPanel::Vector m_panels;      ///< List of panels for this window.
		MiloPanel::Iter m_current_panel; ///< Current active panel.
		std::string m_title;             ///< Title of window.
		std::string m_filename;          ///< Filename of window's xml.
	};

	/**
	 * Singleton class that provides a global context for menu and current window.
	 */
	class MiloApp
	{
	public:
		/** @name Virtual Public Member Functions */
		//@{		
		/**
		 * Redraw entire screen.
		 */
		virtual void redraw_screen() = 0;

		/**
		 * Get new graphics object.
		 * @return New graphics object.
		 */
		virtual Graphics* makeGraphics() = 0;
		//@}

		/** @name Public helper member functions. */
		//@{
		/**
		 * Execute function based on its name. Used for menu handling.
		 */
		void doMenu(const std::string& menuFunctionName);
	
		/**
		 * Query if program should quit.
		 * @return If true, exit main loop.
		 */
		static bool isRunning();

		/** 
		 * Get active window.
		 * @return Active window.
		 */
		MiloWindow& getWindow() { return *(m_current_window->get()); }

		/**
		 * Get active panel from active window
		 * @return Active panel
		 */
		MiloPanel& getPanel() { return *((*m_current_window)->m_current_panel->get()); }

		/**
		 * Add new window to application after current window.
		 * @param win New window added to application
		 */
		void addWindow(MiloWindow* win) {
			m_current_window = AddPtrAfter(m_windows, m_current_window, win);
			makeTopWindow();
		}

		/**
		 * Create a new window and add it to application.
		 */
		void addNewWindow() { addWindow(makeWindow()); }

		/**
		 * Create a new window from xml and add it to application.
		 * @param in XML parser object.
		 */
		void addNewWindow(XML::Parser& in) { addWindow(makeWindow(in, std::string())); }

		/**
		 * Create a new window from xml in a file and add it to application.
		 * @param fname Name of file to load.
		 */
		void addNewWindow(const std::string& fname) {
			std::ifstream ifs(fname);
			XML::Parser in(ifs, "document");
			addWindow(makeWindow(in, fname));
		}

		/**
		 * Close current window from application.
		 */
		void closeWindow() { closeWindow(m_current_window); }

		/**
		 * Close window from application.
		 * @param it Iterator to window to be deleted.
		 */
		void closeWindow(MiloWindow::Iter it) {
			m_current_window = m_windows.erase(it);
			if (m_current_window == m_windows.end()) {
				m_current_window = m_windows.end() - 1;
			}
		}

		/**
		 * Check for existing window.
		 * @return True if there is a window.
		 */
		bool hasWindow() { return !m_windows.empty(); }

		/**
		 * Check for existing panel in current window
		 * @return True if there is a panel.
		 */
		bool hasPanel() { return !(m_windows.empty() || (*m_current_window)->m_panels.empty()); }

		/**
		 * Get iterator of window
		 * @param win MiloWindow reference.
		 * @return Iterator of window.
		 */
		MiloWindow::Iter getWindowIterator(const MiloWindow& win) {
			for ( auto it = m_windows.begin(); it != m_windows.end(); ++it ) {
				if (&win == it->get()) {
					return it;
				}
			}
			return m_windows.end();
		}

		/**
		 * Set window to be current top window.
		 * @param win MiloWindow reference.
		 */
		void setTopWindow(const MiloWindow& win) {
			m_current_window = getWindowIterator(win);
			makeTopWindow();
		}

		/**
		 * Step through window list
		 * @param dir Postive if true.
		 */
		void stepWindow(bool dir = true) {
			if (dir) {
				++m_current_window;
				if (m_current_window == m_windows.end()) {
					m_current_window = m_windows.begin();
				}
			} else {
				if (m_current_window == m_windows.begin()) {
					m_current_window = m_windows.end();
				}
				--m_current_window;
			}
		}

		/**
		 * Return iterator of first window.
		 * @return Iterator of first window.
		 */
		MiloWindow::Iter begin() { return m_windows.begin(); }

		/**
		 * Return iterator of end window iterator.
		 * @return End window iterator.
		 */
		MiloWindow::Iter end() { return m_windows.end(); }
		//@}

		/**
		 * Return current global context.
		 * @return current global context
		 */
		static MiloApp& getGlobal() { return m_current; }

	protected:
		/** @name Constructors and Virtual Destructor */
		//@{

		/**
		 * Protected constructor for singleton base class for windowless context.
		 */
		MiloApp() {}

		/**
		 * Protected constructor for singleton base class.
		 * @param win Default window
		 */
	    MiloApp(MiloWindow* win);

		/**
		 * Abstract base class needs virtual destructor.
		 */
		~MiloApp() {}
		//@}
		
		MiloWindow::Vector m_windows;        ///< List of windows for this application.
		MiloWindow::Iter   m_current_window; ///< Current active window.

		static MiloApp& m_current; ///< Reference to current application singleton

	private:
		/** @name GUI specific Virtual Private Member Functions */
		//@{		
		/** Put current window on top.
		 */
		virtual void makeTopWindow() = 0;

		/** Create default window.
		 */
		virtual MiloWindow* makeWindow() = 0;

		/**
		 * Create window from xml.
		 * @param in XML parser object.
		 */
		virtual MiloWindow* makeWindow(XML::Parser& in, const std::string& fname) = 0;
		//@}
		
		/**
		 * Map of names to functions that handle menu items.
		 */
		static std::unordered_map<std::string, menu_handler> menu_map;
	};

	/**
	 * Abstract Base Class to parse menu xml file
	 */
	class MenuXML
	{
	public:
		/** @name Virtual Public Member Functions */
		//@{
		/**
		 * Start of menu with attributes
		 * @param attributes All menu settings in map.
		 */
		virtual void define_menu(const StringMap& attributes) = 0;

		/**
		 * No more menu items for current menu
		 * @param name Name of menu coming to end
		 */
		virtual void define_menu_end(const std::string& name) = 0;

		/**
		 * Menu item for current menu
		 * @param attributes All menu item settings in map.
		 */
		virtual void define_menu_item(const StringMap& attributes) = 0;

		/**
		 * Menu line for current menu
		 */
		virtual void define_menu_line() = 0;
		//@}

		/** @name Public helper member functions. */
		//@{
		/**
		 * Parse menu tag in XML calling the virtual menu member functions
		 * define_menu, define_menu_end, define_menu_item and define_menu_line.
		 * @param in XML::Parser already initialized with xml file
		 */
		void parse_menu(XML::Parser& in);
		//@}
	};
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
