#ifndef __UI_H
#define __UI_H

#include <unordered_map>
#include <string>
#include "util.h"

class Graphics;

namespace UI {
	enum Keys {
		NO_KEY, CTRL_A, CTRL_B, CTRL_C, CTRL_D, CTRL_E, CTRL_F, CTRL_G, CTRL_H,
		TAB, CTRL_J, CTRL_K, CTRL_L, ENTER, CTRL_N, CTRL_O, CTRL_P, CTRL_Q,
		CTRL_R, CTRL_S, CTRL_T, CTRL_U,	CTRL_V, CTRL_W, CTRL_X, CTRL_Y, CTRL_Z, ESC,
		SPACE = 32, BANG, DBL_QUOTE, HASH, DOLLAR, PRCT, AMP, QUOTE, L_PAR, R_PAR,
		STAR, PLUS, COMMA, MINUS, DOT, DIVIDE, K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
		COLON, SEMI, LESS, EQUAL, GREATER, QUEST, AT, A, B, C, D, E, F, G, H, I, J, K,
		L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, L_BRACKET, B_SLASH, R_BRACKET,
		POWER, U_SCORE, ACCENT, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r,
		s, t, u, v, w, x, y, z, L_BRACE, PIPE, R_BRACE, TILDE,
		F1 = 0x80, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, BACK_TAB,
		INS, DEL, HOME, END, PAGE_UP, PAGE_DOWN, UP, DOWN, LEFT, RIGHT, BSPACE
	};
	
	enum Mouse {
		NO_MOUSE, POSITION, PRESSED, RELEASED, CLICKED, DOUBLE
	};

	enum Modifiers {
		NO_MOD, ALT, SHIFT, ALT_SHIFT, CTRL, CRL_ALT, CTRL_SHIFT, CLTRL_ALT_SHIFT
	};
	
	class Event
	{
	public:
		enum Kind { Mouse, Key };
		
        Event(char k) :
		    m_kind{Key}, m_key{(Keys)k}, m_mouse{NO_MOUSE}, m_mod{NO_MOD}, m_button{0} {}
		
        Event(enum Keys k, enum Modifiers m = NO_MOD) :
	        m_kind{Key}, m_key{k}, m_mouse{NO_MOUSE}, m_mod{m}, m_button{0} {}
		
	    Event(enum Mouse m, int b, enum Modifiers md = NO_MOD) :
	        m_kind{Mouse}, m_key{NO_KEY}, m_mouse{m}, m_mod{md}, m_button{b} {}

		Kind getKind() const { return m_kind; }
		enum Keys getKey() const { return m_key; }
		enum Mouse getMouse() const { return m_mouse; }
		enum Modifiers getModifiers() const { return m_mod; }
		int getButton() const { return m_button; }

		std::size_t hash() const
		{
			using std::hash;
			
			return   hash<int>()((int) m_kind) ^
				   ((hash<int>()((int) m_key) << 1) >> 1) ^
				   ((hash<int>()((int) m_mouse) << 2) >> 2) ^
				    (hash<int>()((int) m_mod) << 2) ^
				    (hash<int>()(m_button) << 1);
		}

		bool operator==(const Event e) const
		{
			return  e.m_kind  == m_kind &&
			        e.m_key   == m_key &&
			        e.m_mouse == m_mouse &&
			        e.m_mod   == m_mod &&
			        e.m_button == m_button;
		}

	private:
		Kind m_kind;
		enum Keys  m_key;
		enum Mouse m_mouse;
		enum Modifiers m_mod;
		int  m_button;
	};

	void doMainLoop(int argc, char* argv[], Graphics& gc);
}

class Graphics;

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
	 * Get mouse poition from Graphics context
	 * @param[out] xMouse Horizontal coordinate of mouse
	 * @param[out] yMouse Vertical coordinate of mouse
	 */
	virtual void getMouseCoords(int& xMouse, int& yMouse)=0;
  
	/**
	 * Get next event.
	 * Blocking function that returns a reference to an base class that defines
	 * an interface to get event info.
	 * @param xCursor horiz coord of cursor
	 * @param yCursor vertical coord of cursor
	 * @param fBlink  If true, blink cursor
	 * @return Reference to Event object
	 */
	virtual const UI::Event& getNextEvent(int xCursor, int yCursor, bool fBlink)=0;
	
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

namespace std {
	template<> struct hash<UI::Event>
	{
		size_t operator()(UI::Event const& key) const
		{
			return key.hash();
		}
	};

	template<> struct hash<Graphics::Attributes>
	{
		size_t operator()(Graphics::Attributes const& a) const
		{
			using std::hash;
			
			return hash<int>()((int) a);
		}
	};
}

#endif // __UI_H
