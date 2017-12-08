#ifndef __SMART_H
#define __SMART_H
#include <vector>
#include <memory>
#include <initializer_list>

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
 * @file smart.h
 * This file templates that support SmartPtr and SmartVector. These classes
 * extend std::shared_ptr<T> so they more easily work with the pointer T*.
 */

/** @name Smart Pointer Templates */
//@{
/**
 * Extention of std::shared_ptr so that it can be assigned
 * compared to regular pointer.
 */
template <class T>
class SmartPtr : public std::shared_ptr<T>
{
public:
	/**
	 * Default constructor SmartPtr from regular pointer
	 */
	explicit SmartPtr() : std::shared_ptr<T>() {}

	/**
	 * Construct SmartPtr from regular pointer
	 */
	explicit SmartPtr(T* p) : std::shared_ptr<T>(p) {}

	/**
	 * Construct SmartPtr from shared pointer
	 */
	explicit SmartPtr(const std::shared_ptr<T>& sp) : std::shared_ptr<T>(sp) {}
	
	/**
	 * Assign raw pointer as managed pointer.
	 */
	SmartPtr& operator=(T* p) { SmartPtr a(p->getSharedPtr()); *this = a; return *this; }

	/**
	 * Compare raw pointer to managed pointer.
	 */
	bool operator==(T* p) const { return this->get() == p; }

	/**
	 * Compare raw pointer to managed pointer.
	 */
	bool operator!=(T* p) const { return this->get() != p; }

	/**
	 * Convert managed pointer to raw pointer for return values.
	 */
	operator T*() const { return this->get(); }
};

/**
 * SmartVector will hold objects with share pointers but can 
 * be used with raw pointers.
 */
template <class T>
class SmartVector : public std::vector< SmartPtr<T> >
{
public:
	/**
	 * Aliases for SmartVector
	 */
	using iterator = typename std::vector< SmartPtr<T> >::iterator;
	using const_iterator = typename std::vector< SmartPtr<T> >::const_iterator;
	using size_type = typename std::vector< std::shared_ptr<T> >::size_type;
	using base = typename std::vector< SmartPtr<T> >;

	/**
	 * Default Constructor for SmartVector
	 */
	SmartVector() {}

	/**
	 * Construct vector from initializer list of raw pointers
	 */
	explicit SmartVector(std::initializer_list<T*> li)
	{
		for ( T* n : li ) { this->push_back(n); }
	}
		
	/**
	 * Constructor for SmartVector initialized with a single object
	 */
	explicit SmartVector(T* p) { SmartPtr<T> a; a = p; this->base::push_back(a); } 

	/**
	 * Construct vector from initializer list of smart pointers
	 */
    SmartVector(std::initializer_list< SmartPtr<T> > li) : base(li) {}

	/**
	 * Push back object into vector managed by share pointer
	 */
	void push_back(T* p) { SmartPtr<T> a; a = p; this->base::push_back(a);  }

	/**
	 * Don't allow push back with shared pointer
	 */
	void push_back(const SmartPtr<T>&) = delete;

	/**
	 * Don't allow push back with shared pointer
	 */
	void push_back(const SmartPtr<T>&&) = delete;

	/**
	 * Insert object at position in vector
	 */
	iterator insert(const_iterator pos, T* val)
	{
		return this->base::insert(pos, SmartPtr<T>(val));
	}
	
	/**
	 * Insert object at position in vector n times
	 */
	iterator insert(const_iterator pos, size_type n, const T* val)
	{
		return this->base::insert(pos, n, SmartPtr<T>(val));
	}

	/**
	 * Insert objects at position in vector from iterator range
	 */
	iterator insert (const_iterator position, iterator first, iterator last)
	{
		return this->base::insert(position, first, last);
	}

	/**
	 * Insert smart_pointer at position in vector
	 */
	iterator insert(const_iterator pos, const SmartPtr<T> val)
	{
		return this->base::insert(pos, val);
	}

	/**
	 * Delete this member function that has shared pointer
	 */
	iterator insert(const_iterator pos, size_type n, const  SmartPtr<T> val) = delete;

	/**
	 * Delete this member function that has shared pointer
	 */
	iterator insert(const_iterator pos, std::initializer_list< SmartPtr<T> > il) = delete;

	/**
	 * Swap smart vector storage.
	 */
	void swap(SmartVector<T>& v) { this->base::swap(v); }

	/**
	 * Delete swap of vectors
	 */
	void swap(base&) = delete;

	/**
	 * Marge another SmartVector at arbitrary positon
	 */
	void merge(SmartVector<T>& v, iterator pos)
	{
		this->base::reserve(this->base::size() + v.size());
		insert(pos, v.base::begin(), v.base::end());
		v.base::clear();
	}

	/**
	 * Merge another SmartVector at end
	 */
	void merge(SmartVector<T>& v) { this->merge(v, this->base::begin()); }
	/**
	 * Insert element at index
	 */
	void insert_index(int index, T* e)
	{
		insert((index < 0 ? base::end() : base::begin()) + index, e);
	}

	/**
	 * Erase element at index.
	 */
	void erase_index(int index)
	{
		this->erase((index < 0 ? base::end() - 1 : base::begin()) + index);
	}

	/**
	 * Get index of element in vector
	 */
	int get_index(T* e) const
	{
		const base* base_this = this;
		int index = 0;
		for ( auto n : *base_this ) {
			if (n.get() == e) return index;
			++index;
		}
		return -1;
	}

	/**
	 * Get iterator of element in vector
	 */
	iterator find(T* e) { return this->base::begin() + get_index(e); }
};

/**
 * template operator== overload to allow comparison of SmartPtr to SmartVector::iterator.
 * @return True if both contain same pointer address.
 */
template<class T>
bool operator==(const SmartPtr<T>& p, const typename SmartVector<T>::iterator& it)
{
	return p.get() == it->get();
}

/**
 * template operator!= overload to allow comparison of SmartPtr to SmartVector::iterator.
 * @return True if both contain different pointer address.
 */
template<class T>
bool operator!=(const SmartPtr<T>& p, const typename SmartVector<T>::iterator& it)
{
	return p.get() != it->get();
}

/**
 * template operator== overload to allow comparison of SmartPtr to SmartVector::iterator.
 * @return True if both contain same pointer address.
 */
template<class T>
bool operator==(const typename SmartVector<T>::iterator& it, const SmartPtr<T>& p)
{
	return p.get() == it->get();
}

/**
 * template operator!= overload to allow comparison of SmartPtr to SmartVector::iterator.
 * @return True if both contain different pointer address.
 */
template<class T>
bool operator!=(const typename SmartVector<T>::iterator& it, const SmartPtr<T>& p)
{
	return p.get() != it->get();
}
//@}

#endif // __SMART_H
