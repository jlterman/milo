#ifndef __UTIL_H
#define __UTIL_H

#include <string>
#include <vector>
#include <array>
#include <complex>
#include <algorithm>

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
 * @file util.h
 * This file contains miscellaneous functions and declerations.
 */

/** @name Global Utility Functions */
//@{
/**
 * Check if floating point is smaller than a limit.
 * @return If smaller than limit, return true for zero.
 */
bool isZero(double x);

/**
 * Check if complex floating point is smaller than a limit.
 * @return If smaller than limit, return true for zero.
 */
bool isZero(std::complex<double> z);

/**
 * Check if string contains integer value.
 * @return True, if integer.
 */
bool isInteger(const std::string& n);

/**
 * Check if double is integer value.
 * @return True is fractional value is smaller than a limit.
 */
bool isInteger(double value);

/**
 * Convert long integer to hex string no leading zeroes.
 * @return String containing converted hexidecimal string.
 */
inline std::string to_hexstring(unsigned long x)
{
	std::string hex;
	while (x != 0) {
		int n = x % 16; x /= 16;
		char d = (n < 10) ? '0' + n : 'A' + n - 10;
		hex.insert(0, 1, d);
	}
	return hex;
}

/**
 * Find an iterator to a value in a vector.
 * Template function will look for a value whose type is stored in the given vector.
 * Returns an iterator to the found value or the end iterator if not found.
 * @param v Vector containing type T.
 * @param val Value of type T.
 * @return Return iterator to found value or end iterator.
 */
template <class T>
inline auto find(const std::vector<T>& v, const T& val) -> decltype( v.cbegin() )
{
	return std::find(v.cbegin(), v.cend(), val);
}

/**
 * Delete all items in a vector that contains pointers
 * Template function delete each pointer in a vector.
 * @param v Vector containing pointer type T*.
 */
template <class T>
inline void freeVector(std::vector<T*>& v)
{
	for ( auto p : v ) { delete p; }
	v.clear();
}

/**
 * Merge vector two vectors.
 * @param a Vector ending up items of both vectors.
 * @param b Vector whose items are merged and cleared.
 */
template <class T>
inline void mergeVectors(std::vector<T>& a, std::vector<T>& b)
{
	a.reserve(a.size() + b.size());
	for ( auto e : b ) { a.insert(a.end(), e); }
	b.clear();
}

/**
 * Erase an element in a vector by index.
 * Helper function to convert index to iterator to erase element.
 * @param v Vector storing type T.
 * @param index Index of element to be erased.
 */
template <class T>
inline void eraseElement(std::vector<T>& v, int index)
{
	v.erase((index < 0 ? v.end() - 1 : v.begin()) + index);
}

/**
 * Insert an element after the element pointed to by index in vector.
 * Use vector.insert() with iterator created from index.
 * @param v Vector storing type T.
 * @param index Index of element where new element will be inserted.
 * @param e Element of type T.
 */
template <class T>
inline void insertElement(std::vector<T>& v, int index, T e)
{
	v.insert((index < 0 ? v.end() : v.begin()) + index, e);
}
//@}

/**
 * Rectangle template class.
 * Holds a rectange as an origin and a width and height, plus helper member functions.
 * Template type can be any numerical type.
 */
template <class T>
class Rectangle
{
public:
	/** @name Constructors */
	//@{
	/**
	 * Default constructor for Rectangle.
	 * Set origin and dimensions to zero.
	 */
    Rectangle() : m_rect{0, 0, 0, 0} {}

	/**
	 * Constructor for Rectangle from all 4 coordinates.
	 * @param x Width of rectangle.
	 * @param y Height of rectangle.
	 * @param x0 Horizontal origin of rectangle.
	 * @param y0 Vertical origin of rectangle.
	 */
    Rectangle(T x, T y, T x0, T y0) : m_rect{x, y, x0, y0} {}
	//@}

	/** @name Mutators */
	//@{
	/**
	 * Set origin and dimension of Rectangle.
	 * @param x Width of rectangle.
	 * @param y Height of rectangle.
	 * @param x0 Horizontal origin of rectangle.
	 * @param y0 Vertical origin of rectangle.
	 */
	void set(T x, T y, T x0, T y0) { m_rect = {x, y, x0, y0}; }

	/**
	 * Set origin of Rectangle.
	 * @param x0 Horizontal origin of rectangle.
	 * @param y0 Vertical origin of rectangle.
	 */
	void setOrigin(T x0, T y0)     { m_rect[X0] = x0; m_rect[Y0] = y0; }

	/**
	 * Set dimension of Rectangle.
	 * @param x Width of rectangle.
	 * @param y Height of rectangle.
	 */
	void setSize(T x, T y)         { m_rect[WIDTH] = x; m_rect[HEIGHT] = y; }
	//@}

	/** @name Accessors */
	//@{
	/**
	 *  reference to Rectangle's horizontal origin.
	 * @return Reference to Rectangle's horizontal origin.
	 */
	int& x0() { return m_rect[X0]; }

	/**
	 * Get reference to Rectangle's vertical origin.
	 * @return Reference to Rectangle's vertical origin.
	 */
	int& y0() { return m_rect[Y0]; }

	/**
	 * Get reference to Rectangle's width.
	 * @return Reference to Rectangle's width.
	 */
	int& width()   { return m_rect[WIDTH]; }

	/**
	 * Get reference to Rectangle's height.
	 * @return Reference to Rectangle's height.
	 */
	int& height()  { return m_rect[HEIGHT]; }

	/**
	 * Get Rectangle's horizontal origin
	 * @return Rectangle's horizontal origin
	 */
	int x0() const { return m_rect[X0]; }

	/**
	 * Get Rectangle's vertical origin
	 * @return Rectangle's vertical origin
	 */
	int y0() const { return m_rect[Y0]; }

	/**
	 * Get Rectangle's width
	 * @return Rectangle's width
	 */
	int width()  const { return m_rect[WIDTH]; }

	/**
	 * Get Rectangle's height
	 * @return Rectangle's height
	 */
	int height() const { return m_rect[HEIGHT]; }
	//@}

	/** @name Helper Member Functions */
	//@{
	/**
	 * Convert state of this class to string
	 * @return Return string representing this object
	 */
	std::string toString()
	{
		std::string s = "width: " + std::to_string(width()) + ", height: " + std::to_string(height()) +
			            ", x0: " + std::to_string(x0()) + ", y0: " + std::to_string(y0());
		return s;
	}

	/**
	 * Return area of rectangle.
	 * @return Area of rectangle.
	 */
	T area() { return width()*height(); }
	
	/**
	 * Check if point is inside Rectangle.
	 * @param x Horizontal origin of point.
	 * @param y Vertical origin of point.
	 * @return Return true if point (x,y) is inside Rectangle.
	 */
	bool inside(T x, T y) const { 
		return ( x >= x0() && x < (x0() + width()) && 
				 y >= y0() && y < (y0() + height()) );
	}

	/**
	 * Check if this rectangle is inside given Rectangle r.
	 * @param r Rectangle to be tested
	 * @return Return true if this rectangle is inside r.
	 */
	bool inside(Rectangle r) const {
		return ( x0() >= r.x0() && (x0() + width())  <= (r.x0() + r.width()) && 
				 y0() >= r.y0() && (y0() + height()) <= (r.y0() + r.height()) );
	}

	/**
	 * Detect overlap between this rectangle and another.
	 * @return True if rectangle r intersects with rectangle in class object.
	 */
	bool intersect(const Rectangle& r) const {
		bool noOverlap = (x0() - r.x0()) > r.width() ||
			             (r.x0() - x0()) > width() ||
			             (y0() - r.y0()) > r.height() ||
			             (r.y0() - y0()) > height();
		return !noOverlap;
	}

	/**
	 * Merge rectangle in class object with rectangle r
	 */
	void merge(const Rectangle& r) {
		T x0 = min(x0(), r.x0());
		T y0 = min(y0(), r.y0());
		T x1 = max(x0() + width(), r.x0() + r.width());
		T y1 = max(y0() + height(), r.y0() + r.height());
		set(x1 - x0, y1 - y0, x0, y0);
	}

	/**
	 * return rectangle that is merger of r1 and r2
	 */
	static Rectangle merge(const Rectangle& r1, const Rectangle& r2) {
		Rectangle r{r1};
		r.merge(r2);
		return r;
	}
	//@}

private:
	enum { WIDTH, HEIGHT, X0, Y0, SIZE }; ///< Name of rectangle parameters.
	std::array<T, SIZE> m_rect;           ///< Storage of rectangle parameters.
};

/** @name Global Type Declerations  */
//@{
using Box = Rectangle<int>; ///< @brief Specialization of integer rectangle.
//@}

namespace std
{
	/**
	 * Combine hash keys for multi data member classes,
	 */
	template <class T> inline void hash_combine(std::size_t& seed, T v)
	{
		seed ^= hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	}
}

/**
 * Calculate final hash from given list of values
 */
template <class T> inline std::size_t hash_calculate(std::initializer_list<T> l)
{
	std::size_t seed = 0;
	for (T n : l) {
		std::hash_combine<T>(seed, n * 0x9e3779b1);
	}
	return seed;
}

/**
 * Logging for milo program.
 */
namespace Log
{
	/**
	 * Pint message along with file and line number information to milo log.
	 */
	void msg(const std::string& m);
}

#define LOG_TRACE_MSG(m) Log::msg(string("") + __FILE__ + ": " + to_string(__LINE__) + ": " + (m))
#define LOG_TRACE_FILE "/tmp/milo.log"
#define LOG_TRACE_CLEAR() remove(LOG_TRACE_FILE)

#define QUOTE(arg) #arg
#define STR(macro) QUOTE(macro)

#ifndef INSTALL_PATH
#define INSTALL_PATH /usr/local/milo
#endif

#define INSTALL_PATH_STR STR(INSTALL_PATH)

#endif // __UTIL_H
