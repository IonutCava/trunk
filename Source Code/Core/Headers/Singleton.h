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

#ifndef SINGLETON_H_
#define SINGLETON_H_

#include "Hardware/Platform/Headers/Mutex.h"

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
	static Lock _singletonMutex;
};

template <class T> T* Singleton<T>::_instance = 0;
template <class T> Lock Singleton<T>::_singletonMutex;

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

