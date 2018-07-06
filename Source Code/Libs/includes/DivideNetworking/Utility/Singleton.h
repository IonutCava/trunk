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

#ifndef SINGLETON_H_
#define SINGLETON_H_

#include "SharedMutex.h"

template <class T>
class Singleton{
public :
	inline static T& getInstance() {
		if (!_instance)   {
			UpgradableReadLock ur_lock(_singletonMutex);
			if (!_instance){ //double-checked lock
				UpgradeToWriteLock uw_lock(ur_lock);
				_instance = new T;
			}
		}
		return *_instance;
	}

	inline static void DestroyInstance() {
		WriteLock w_lock(_singletonMutex);
		SAFE_DELETE(_instance);
	}

protected :
	Singleton() {}
	virtual ~Singleton() {}

private :
	/// singleton instance
	static T* _instance;

	Singleton(Singleton&);
	void operator =(Singleton&);
	static SharedLock _singletonMutex;
};

template <class T> T* Singleton<T>::_instance = 0;
template <class T> SharedLock Singleton<T>::_singletonMutex;

#define DEFINE_SINGLETON(class_name) \
	class class_name : public Singleton<class_name> { \
		friend class Singleton<class_name>;

#define DEFINE_SINGLETON_EXT1(class_name, base_class) \
	class class_name : public Singleton<class_name> , public base_class{ \
		friend class Singleton<class_name>;

#define DEFINE_SINGLETON_EXT2(class_name, base_class1, base_class2) \
	class class_name : public Singleton<class_name> , public base_class1, public base_class2{ \
		friend class Singleton<class_name>;

#define END_SINGLETON };

#endif // SINGLETON_H
