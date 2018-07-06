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

template <class T>
class Singleton{
public :
	inline static T& getInstance() {
		if (!_instance){
			_instance = new T;
		}

		return *_instance;
	}

	inline static void DestroyInstance() {
		if(_instance){
			delete _instance;
			_instance = 0;
		}
	}

protected :
	Singleton() {}
	virtual ~Singleton() {}

private :
	///C++11 standard assures a static instance should be thread safe:
	//"if control enters the declaration concurrently while the variable is being initialized,
	//the concurrent execution shall wait for completion of the initialization."
	static T* _instance;
	Singleton(Singleton&);
	void operator =(Singleton&);
};

template <class T> T* Singleton<T>::_instance = 0;

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
