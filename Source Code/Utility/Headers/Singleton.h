/*�Copyright 2009-2011 DIVIDE-Studio�*/
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

#ifndef SINGLETON_H
#define SINGLETON_H

template <class T>
class Singleton
{
public :
	inline static T& getInstance() {
		if (!s_pInstance)
			s_pInstance = new T;

		return *s_pInstance;
	}

	inline static void DestroyInstance() {
		if(s_pInstance) delete s_pInstance;
		s_pInstance = 0;
	}

protected :
	Singleton() {}
	~Singleton() {}

private :
	static T* s_pInstance;	// Instance de la classe

	Singleton(Singleton&);
	void operator =(Singleton&);
};

template <class T> T* Singleton<T>::s_pInstance = 0;

#define SINGLETON_BEGIN(class_name) \
	class class_name : public Singleton<class_name> { \
		friend class Singleton<class_name>;

#define SINGLETON_BEGIN_EXT1(class_name, base_class) \
	class class_name : public Singleton<class_name> , public base_class{ \
		friend class Singleton<class_name>;

#define SINGLETON_BEGIN_EXT2(class_name, base_class1, base_class2) \
	class class_name : public Singleton<class_name> , public base_class1, public base_class2{ \
		friend class Singleton<class_name>;

#define SINGLETON_END() };

#endif // SINGLETON_H
