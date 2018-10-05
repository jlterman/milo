#ifndef __PANEL_H
#define __PANEL_H

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
 * @file panel.h
 * This file contains the decleration of the different panels for the UI.
 */

#include <iostream>
#include <unordered_map>
#include <string>
#include <memory>
#include "milo.h"
#include "ui.h"

/**
 * User Interface for milo namespace.
 * This is the user interface for milo for porting to paticular interfaces.
 */
namespace UI {
	/**
	 * Event Box class for Equation class.
	 */
	class EqnBox : public EventBox
	{
	public:
		using Ptr = std::unique_ptr<EqnBox>; ///< Unique pointer for EqnBox
		
		/** @name EqnBox function pointer declerations. */
		//@{
		/**
		 * Function pointer to handle key event. Pointer to event passed as argument.
		 * Return true, if event changed equation.
		 */
		using key_handler = bool (*)(EqnBox&, const KeyEvent&);
		
		/**
		 * Function pointer to handle mouse event. Pointer to event passed as argument.
		 * Return true, if event changed equation.
		 */
		using mouse_handler = bool (*)(EqnBox&, const MouseEvent&);
		
		/**
		 * Function pointer to handle mouse event.
		 * Return true, if event changed equation.
		 */
		using menu_handler = bool (*)(EqnBox&);
		//@}

		/** @name Constructors and Virtual Desctructor */
		//@{
		/** 
		 * Constructor for EqnBox initializing Equation with
		 * single initialization string.
		 * @param init Initialization string.
		 */
	    EqnBox(const std::string& init) : EventBox(), m_eqn(new Equation(init))	{ pushUndo(); }

		/** 
		 * Constructor for EqnBox passing equation directly.
		 * @param eqn Equation for this panel.
		 */
	    EqnBox(Equation* eqn) :	EventBox(), m_eqn(eqn) { pushUndo(); }

		/** 
		 * Constructor for EqnBox getting equation from XML paraser
		 * @param in  XML parser object.
		 */
	    EqnBox(XML::Parser& in) : EventBox(), m_eqn(new Equation(in)) { pushUndo(); }

		~EqnBox() {} ///< Virtual desctructor.
		//@}
		
		/** @name Overriden Pure Virtual Public Member Functions from Event Box */
		//@{
		/**
		 * Handle key event for panel
		 * @param key Key event
		 */
		void doKey(const KeyEvent& key);
	
		/**
		 * Handle mouse event for panel
		 */
		void doMouse(const MouseEvent& mouse);

		/**
		 * Execute panel specific function based on its name. Used for menu handling.
		 * @param menuFunctionName Name of menu function to be executed.
		 * @return True if menuFunctionName found.
		 */
		bool doMenu(const std::string& menuFunctionName);
		
		/** Handle redraw event
		 */
		void doDraw();

		/** 
		 * Calculate size of panel.
		 * @return Calculated size of panel.
		 */
		Box calculateSize();

		/** 
		 * Get last calculated size of equation panel.
		 * @return Last calculated size of equation panel.
		 */
		Box getSize() { return m_eqn->getRoot()->getFrame().box; }

		/**
		 * Get base line relative to top of box.
		 * @return Base line.
		 */
		int getBase() { return  m_eqn->getRoot()->getFrame().base; }
		
		/**
		 * Push state of panel to undo stack.
		 */
		void pushUndo();
		
		/**
		 * Restore state of panel on top of undo stack.
		 */
		void popUndo();

		/**
		 * Output panel contents as xml to XML stream.
		 * @param XML stream class object.
		 */
		void xml_out(XML::Stream& xml) { xml << *m_eqn; }
		
		/**
		 * Check where there is an active input in this equation object.
		 * @return If true, there is an active input.
		 */
		bool blink();
		
		/**
		 * Get coordinates of current active input.
		 * @param[out] x Horizontal origin of curosr.
		 * @param[out] y Vertical origin cursor.
		 */
		void getCursorOrig(int& x, int& y);
		//@}

		/** @name Helper Public Member functions */
		//@{
		/**
		 * Get reference to current equation.
		 * @return Reference to current equation.
		 */
		Equation& getEqn() { return *m_eqn; }

		/**
		 * New equation from string such as 'a+b/c'
		 * @param eq String containing equation to be created.
		 * @return Refrence to new equation
		 */
		Equation& newEqn(std::string eq) {
			m_eqn.reset(new Equation(eq));
			return *m_eqn;
		}

		/**
		 * New equation from xml stream.
		 * @param is Input stream containing xml.
		 * @return Refrence to new equation
		 */
		Equation& newEqn(std::istream& is) {
			m_eqn.reset(new Equation(is));
			return *m_eqn;
		}
		//@}

	private:
		EqnPtr        m_eqn;            ///< Shared pointer to current equation.
		EqnUndoList   m_eqns;           ///< Equation undo stack
		Node* m_start_select = nullptr; ///< If not null, node is selected.
		int m_start_mouse_x  = -1;      ///< Horiz coord of start of mouse drag.
		int m_start_mouse_y  = -1;      ///< Vertical coord of start of mouse drag

		/** @name Static member functions */
		//@{
		/**
		 * Static function to emit a key event to a panel.
		 * @param eqn EqnBox to handle key event.
		 * @param key Key event.
		 * @return True if key event was handled.
		 */
		static bool emit_key(EqnBox& eqn, const KeyEvent& key);
		//@}

		/** @name Static member objects */
		//@{
		/**
		 * Map of events to functions that handle mouse events.
		 */
		static const std::unordered_map<MouseEvent, mouse_handler> mouse_event_map;
		
		/**
		 * Map of events to functions that handle key events.
		 */
		static const std::unordered_map<KeyEvent, key_handler> key_event_map;
		
		/**
		 * Map of names to functions that handle menu calls.
		 */
		static const std::unordered_map<std::string, menu_handler> menu_map;
		//@}

		/** @name Private member functions to handle UI events */
		//@{
		/**
		 * Handle letter or number key event.
		 * Insert letter or number key into equation if there is a current input or selection.
		 * @param event Key event.
		 * @return True if key event is processed.
		 */
		bool do_alphaNumber(const KeyEvent& event);

		/**
		 * Handle backspace key event.
		 * Erase selection if it exists, or previous factor before current input.
		 * @param event Key event.
		 * @return True if backspace is processed.
		 */
		bool do_backspace(const KeyEvent& event);

		/**
		 * Handle left arrow key event.
		 * Move to node to the left of selection or current input.
		 * @param event Key event.
		 * @return True if left arrow proccessed.
		 */
		bool do_left(const KeyEvent& event);

		/**
		 * Handle right arrow key event.
		 * Move to node to the right of selection or current input.
		 * @param event Key event.
		 * @return True if right arrow proccessed.
		 */
		bool do_right(const KeyEvent& event);

		/**
		 * Handle left arrow with shift key event.
		 * Either add factor on the left to selection or 
		 * select factor to left of cursor.
		 * @param event Key event.
		 * @return True if shift left arrow is processed.
		 */
		bool do_shift_left(const KeyEvent& event);

		/**
		 * Handle right arrow with shift key event.
		 * Either add factor on the right to selection or 
		 * select factor to right of cursor.
		 * @param event Key event.
		 * @return True if shift right arrow is processed.
		 */
		bool do_shift_right(const KeyEvent& event);
		
		/**
		 * Handle up arrow key event.
		 * Transverse the entire equation tree to the left 
		 * of the selection or the current input. Select first
		 * node if there is no selection or input.
		 * @param event Key event.
		 * @return Always true.
		 */
		bool do_up(const KeyEvent& event);

		/**
		 * Handle down arrow key event.
		 * Transverse the entire equation tree to the right
		 * of the selection or the current input. Select last
		 * node if there is no selection or input.
		 * @param event Key event.
		 * @return Always true.
		 */
		bool do_down(const KeyEvent& event);

		/**
		 * Handle up arrow with shift key event.
		 * Select the expression to the left of the current input
		 * or add it to the existing selection.
		 * @param event Key event.
		 * @return True if up arrow with shift is processed.
		 */
		bool do_shift_up(const KeyEvent& event);

		/**
		 * Handle down arrow with shift key event.
		 * Select the expression to the right of the current input
		 * or add it to the existing selection.
		 * @param event Key event.
		 * @return True if down arrow with shift is processed.
		 */
		bool do_shift_down(const KeyEvent& event);

		/**
		 * Handle enter key event.
		 * Either disable current input or replace 
		 * current selection with a new input.
		 * @param event Key event.
		 * @return True if enter is proccessed.
		 */
		bool do_enter(const KeyEvent& event);

		/**
		 * Handle tab key event.
		 * Move to next input. If shift down move to previous input.
		 * @param event Key event.
		 * @return Always true
		 */
		bool do_tab(const KeyEvent& event);

		/**
		 * Handle +/- key event.
		 * Add a new term with a new input. New term is positive 
		 * or negative depending on whether character is +/-.
		 * @param event Key event.
		 * @return True if input is active.
		 */
		bool do_plus_minus(const KeyEvent& event);

		/**
		 * Insert divide term.
		 * @param event Key event.
		 * @return True if successful.
		 */
		bool do_divide(const KeyEvent& event);

		/**
		 * Insert power factor.
		 * @param event Key event.
		 * @return True if successfull.
		 */
		bool do_power(const KeyEvent& event);

		/**
		 * Handle left parenthesis key event.
		 * Add "(*)". Side effect will be to add any functions
		 * that matches or an expression as a new factor.
		 * @param event Key event.
		 * @return True if successful.
		 */
		bool do_left_parenthesis(const KeyEvent& event);

		/**
		 * Handle space key event.
		 * Either select factor at input or select one level higher.
		 * @param event Key event.
		 * @return True if successful.
		 */
		bool do_space(const KeyEvent& event);		

		/**
		 * Handle mouse press event.
		 * If node has been selected, remember it and mouse coordinates.
		 * @param mouse Mouse Event
		 * @return Always false.
		 */
		bool do_mouse_pressed(const MouseEvent& mouse);

		/**
		 * Handle mouse release event.
		 * If mouse released where it was pressed, select node.
		 * Otherwise, end mouse dragging state.
		 * @param mouse Mouse Event
		 * @return Always true.
		 */
		bool do_mouse_released(const MouseEvent& mouse);

		/**
		 * Handle mouse click.
		 * If mouse clicks on node select it or activate input.
		 * @param mouse Mouse Event
		 * @return True if node selected.
		 */
		bool do_mouse_clicked(const MouseEvent& mouse);
		
		/**
		 * Handle mouse double click.
		 * If no node found, return false. Otherwise activate if input selected
		 * or add input before selected node.
		 * @param mouse Mouse Event
		 * @return True, if node found.
		 */
		bool do_mouse_double(const MouseEvent& mouse);
		
		/**
		 * Handle mouse position reports,
		 * Update part of equation selected by mouse dragging.
		 * @param mouse Mouse Event
		 * @return Always false.
		 */
		bool do_mouse_position(const MouseEvent& mouse);
		//@}
	};

	/** EqnPanel is a panel class for EqnBox
	 */
	class EqnPanel : public MiloPanel
	{
	public:
		/** @name Constructors and Virtual Desctructor */
		//@{
		/** 
		 * Constructor for EqnPanel initializing Equation with
		 * single initialization string.
		 * @param init Initialization string.
		 */
	    EqnPanel(const std::string& init) :
		    MiloPanel(),
			m_eqnBox(init) {}

		/** 
		 * Constructor for EqnPanel passing equation directly.
		 * @param eqn Equation for this panel.
		 */
	    EqnPanel(Equation* eqn) :
		    MiloPanel(),
			m_eqnBox(eqn) {}

		/** 
		 * Constructor for EqnPanel getting equation from XML paraser
		 * @param in  XML parser object.
		 */
	    EqnPanel(XML::Parser& in) :
			MiloPanel(),
			m_eqnBox(in) {}

		~EqnPanel() {} ///< Virtual desctructor.
		//@}

		/** @name Overridden Pure Virtual Public Member Functions from EventBox */
		//@{		
		/**
		 * Handle key event.
		 * @param key Key event.
		 */
		void doKey(const UI::KeyEvent& key) { m_eqnBox.doKey(key); }

		/**
		 * Handle mouse event.
		 * @param mouse Mouse event.
		 */
		void doMouse(const UI::MouseEvent& mouse) { m_eqnBox.doMouse(mouse); }

		/**
		 * Execute specific function based on its name. Used for menu handling.
		 * @param menuFunctionName Name of menu function to be executed.
		 * @return True if menuFunctionName found.
		 */
		bool doMenu(const std::string& menuFunctionName) { return m_eqnBox.doMenu(menuFunctionName); }
		
		/** Draw contents on window.
		 */
		void doDraw() { m_eqnBox.doDraw(); }

		/** 
		 * Calculate size needed for doDraw(). .
		 * @return Calculated size for drawing.
		 */
		Box calculateSize() { return m_eqnBox.calculateSize(); }

		/** 
		 * Get last calculated size.
		 * @return Last calculated size.
		 */
		Box getSize() { return m_eqnBox.getSize(); }

		/**
		 * Get base line relative to top of box.
		 * @return Base line.
		 */
		int getBase() { return m_eqnBox.getBase(); }
		
		/**
		 * Push state to undo stack.
		 */
		void pushUndo() { m_eqnBox.pushUndo(); }
		
		/**
		 * Restore state from top of undo stack.
		 */
		void popUndo() { m_eqnBox.popUndo(); }

		/**
		 * Output contents as xml to XML stream.
		 * @param XML stream class object.
		 */
		void xml_out(XML::Stream& xml) { m_eqnBox.xml_out(xml); }

		/**
		 * Check where there is an active input.
		 * @return If true, there is an active input.
		 */
		bool blink() { return m_eqnBox.blink(); }

		/**
		 * Get coordinates of current active input.
		 * @param[out] x Horizontal origin of curosr.
		 * @param[out] y Vertical origin cursor.
		 */
		void getCursorOrig(int& x, int& y) { m_eqnBox.getCursorOrig(x, y); }
		//@}		
		
		/** @name Overridden Pure Virtual Public Member Functions from MiloPanel */
		//@{
		/**
		 * Get type name of panel.
		 * @return String containing type of panel for xml tag.
		 */
		const std::string& getType() { return EqnPanel::name; }

		/**
		 * Set size and origin of the graphics of this panel
		 * @param x Horizontal size of graphics.
		 * @param y Vertical size of graphics.
		 * @param x0 Horizontal origin of graphics.
		 * @param y0 Vertical origin of graphics.
		 */
		void setBox(int x, int y, int x0, int y0);
		//@}

		static const std::string name; ///< Name of this panel
	private:
		EqnBox m_eqnBox;  ///< Equation Event Box handled by EqnPanel.

		static bool init; ///< Should be true after static initilization.

		/**
		 * Static initialization for class.
		 */
		static bool do_init();
	};

	/** MiloPanel derived class for two equations in alegebraic relatiohship.
	 */
	class AlgebraPanel : public MiloPanel
	{
	public:
		/** EventBox subclass to draw equal between equations.
		 */
		class EqualBox : public EventBox
		{
		public:
			/** Constructor for EqualBox class
			 */
		    EqualBox() : EventBox() {}

			/** @name Pure Virtual Public Member Functions */
			//@{		
			/**
			 * Handle key event.
			 * @param key Key event.
			 */
			void doKey(const UI::KeyEvent&) {}
			
			/**
			 * Handle mouse event.
			 * @param mouse Mouse event.
			 */
			void doMouse(const UI::MouseEvent&) {}
			
			/**
			 * Execute specific function based on its name. Used for menu handling.
			 * @param menuFunctionName Name of menu function to be executed.
			 * @return True if menuFunctionName found.
			 */
			bool doMenu(const std::string&) { return false; }
			
			/** Draw contents on window.
			 */
			void doDraw() { m_gc->at(m_gc->getTextLength("="), 0, '=', Graphics::NONE); }
			
			/** 
			 * Calculate size needed for doDraw(). .
			 * @return Calculated size for drawing.
			 */
			Box calculateSize() { m_gc->set(getSize()); return getSize(); }
			
			/** 
			 * Get last calculated size.
			 * @return Last calculated size.
			 */
			Box getSize() { return Box(m_gc->getTextLength("==="), m_gc->getTextHeight(), 0, 0); }
			
			/**
			 * Get base line relative to top of box.
			 * @return Base line.
			 */
			int getBase() { return 0; }
			
			/**
			 * Push state to undo stack.
			 */
			void pushUndo() {}
			
			/**
			 * Restore state from top of undo stack.
			 */
			void popUndo() {}
			
			/**
			 * Output contents as xml to XML stream.
			 * @param XML stream class object.
			 */
			void xml_out(XML::Stream&) {}
			
			/**
			 * Check where there is an active input.
			 * @return If true, there is an active input.
			 */
			bool blink() { return false; }
			
			/**
			 * Get coordinates of current active input.
			 * @param[out] x Horizontal origin of curosr.
			 * @param[out] y Vertical origin cursor.
			 */
			void getCursorOrig(int&, int&) {}
		//@}
		};
		
		/** Left or right side.
		 */
		enum Side { LEFT, RIGHT };
		
		/** @name Constructors and Virtual Desctructor */
		//@{
		/** 
		 * Constructor for AlgebraPanel initializing Equation with
		 * single initialization string.
		 * @param init Initialization string for equation1+'='+equation2.
		 */
	    AlgebraPanel(const std::string& init);

		/** 
		 * Constructor for AlgebraPanel getting equation from XML paraser
		 * @param in  XML parser object.
		 */
	    AlgebraPanel(XML::Parser& in);

		~AlgebraPanel() {} ///< Virtual desctructor.
		//@}

		/** @name Public Member Functions */
		//@{		
		/**
		 * Handle key event for panel
		 * @param key Key event
		 */
		void doKey(const UI::KeyEvent& key) { getCurrentSide().doKey(key); }

		/**
		 * Handle mouse event for panel
		 * @param mouse Mouse event
		 */
		void doMouse(const UI::MouseEvent& mouse) { getCurrentSide().doMouse(mouse); }

		/**
		 * Execute panel specific function based on its name. Used for menu handling.
		 * @param menuFunctionName Name of menu function to be executed.
		 * @return True if menuFunctionName found.
		 */
		bool doMenu(const std::string& menuFunctionName) {
			return getCurrentSide().doMenu(menuFunctionName);
		}
		
		/** Handle redraw event
		 */
		void doDraw();

		/** 
		 * Calculate size of panel.
		 * @return Calculated size of panel.
		 */
		Box calculateSize();

		/** 
		 * Get last calculated size of equation panel.
		 * @return Last calculated size of equation panel.
		 */
		Box getSize() { return m_frame.box; }

		/**
		 * Get base line relative to top of box.
		 * @return Base line.
		 */
		int getBase() { return 0; }
		
		/**
		 * Push state of panel to undo stack.
		 */
		void pushUndo() { getCurrentSide().pushUndo(); }
		
		/**
		 * Restore state of panel on top of undo stack.
		 */
		void popUndo() { getCurrentSide().popUndo(); }

		/**
		 * Output panel contents as xml to XML stream.
		 * @param XML stream class object.
		 */
		void xml_out(XML::Stream& xml);

		/**
		 * Get type name of panel.
		 * @return String containing type of panel for xml tag.
		 */
		const std::string& getType() { return AlgebraPanel::name; }

		/**
		 * Check where there is an active input in this panel.
		 * @return If true, there is an active input.
		 */
		bool blink() { return getCurrentSide().blink(); }

		/**
		 * Set size and origin of the graphics of this panel
		 * @param x Horizontal size ofgraphics.
		 * @param y Vertical size of graphics.
		 * @param x0 Horizontal origin of graphics.
		 * @param y0 Vertical origin of graphics.
		 */
		void setBox(int x, int y, int x0, int y0);

		/**
		 * Get coordinates of current active input.
		 * @param[out] x Horizontal origin of curosr.
		 * @param[out] y Vertical origin cursor.
		 */
		void getCursorOrig(int& x, int& y) { getCurrentSide().getCursorOrig(x, y); }
		//@}

		/** @name Public helper member functions. */
		//@{
		/**
		 * Get current side of equation.
		 * @return Current side.
		 */
		EqnBox& getCurrentSide() { return (m_side == LEFT) ? m_left : m_right; }

		/**
		 * Read side from XML::Parser.
		 * @param in XML parser object.
		 * @return Side read from xml.
		 */
		Side readSide(XML::Parser& in);
		//@}

		static const std::string name; ///< Name of this panel
	private:
		Side     m_side;    ///< Side that is active.
		EqnBox   m_left;    ///< Equation of left of algebraic equality.
		EqnBox   m_right;   ///< Equation of right of algebraic equality.
		EqualBox m_equal;   ///< Equal sign between the two equations.
		Node::Frame m_frame; ///< Form containing both equations.

		static const std::string side_tag;    ///< Side tag.
		static const std::string left_value;  ///< Element value for left.
		static const std::string right_value; ///< Element value for right.
		
		static bool init;    ///< Should be true after static initilization.

		/**
		 * Static initialization for class.
		 */
		static bool do_init();
	};
	
}

#endif // __PANEL_H
