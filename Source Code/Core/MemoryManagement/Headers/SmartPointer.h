/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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