/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RESOURCE_POINTER_H_
#define RESOURCE_POINTER_H_

template<class T>
class ResPointer {
public:
   //Constructors - basic
   ResPointer()  {
      obj=0;
   }
   //Constructing with a pointer
   ResPointer(T *o)    {
      obj=0;
      *this=o;
   }
   //Constructing with another smart pointer (copy constructor)
   ResPointer(const ResPointer<T> &p)  {
      obj=0;
      *this=p;
   }

   //Destructor
   ~ResPointer()  {
      subRef();
   }

   //Assignement operators - assigning a plain pointer
   inline operator =(T *o) {
      subRef();
      obj=o;
      addRef();
   }
   //Assigning another smart pointer
   inline operator =(const ResPointer<T> &p)  {
      subRef();
      obj=p.obj;
      addRef();
   }

   //Access as a reference
   inline T& operator *() const    {
      assert(obj!=0 && "Tried to * on a NULL smart pointer");
      return *obj;
   }
   //Access as a pointer
   inline T* operator ->() const   {
      assert(obj!=0 && "Tried to -> on a NULL smart pointer");
      return obj;
   }
   //Conversion - allow the smart pointer to be automatically
   //converted to type T*
   inline operator T*() const   {
      return obj;
   }

   inline bool isValid() const  {
      return (obj!=0);
   }

   inline bool operator !()  {
      return !(obj);
   }

   inline bool operator ==(const ResPointer<T> &p) const   {
      return (obj==p.obj);
   }

   inline bool operator ==(const T* o) const   {
      return (obj==o);
   }
private:
  inline void addRef() {
        // Only change if non-null
        if (obj) obj->addRef();
  }

  inline void subRef() {
        // Only change if non-null
        if (obj) {
			// Subtract and test if this was the last pointer.
			if (obj->subRef()) {
				delete obj;
				obj=0;
			}
        }
  }

protected:
   T* obj;
};

#endif