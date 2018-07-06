/*“Copyright 2009-2012 DIVIDE-Studio”*/
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
/*
	Code source: http://www.gamedev.net/page/resources/_/technical/game-programming/enginuity-part-ii-r1954
*/

#ifndef TRACKED_OBJECT_H_
#define TRACKED_OBJECT_H_
#include <list>
///A tracked object takes car of it's own reference counting and knows it's own size
///It also schedules it's own deletion (a pompous name for a smart pointer)
class TrackedObject {
  
    public:
	  ///Increase reference count
	  void AddRef();
	  ///Decrease reference count
	  void Release();
	  ///Add object dependency (dependent objects are ref counted with the parent object)
	  void addDependency(TrackedObject* obj);
	  ///Remove an object dependency
	  void removeDependency(TrackedObject* obj);
	  ///How many references does this object belong to
	  inline long getRefCount() {return _refCount;}
	  ///Did the object reach 0 refs? If so, it's marked for deleton
	  virtual bool shouldDelete(){return _shouldDelete;}
	  ///We can manually force a deletion
	  virtual void scheduleDeletion(){_shouldDelete = true;}
	  ///Or we can cancel the deletion before it happens
	  virtual void cancelDeletion(){_shouldDelete = false;}
	  ///For memory consumption later on
	  virtual unsigned long size() {return sizeof(*this);}
    protected:
	  TrackedObject();
	  virtual ~TrackedObject();

   private:
      long _refCount;
	  bool	_shouldDelete;
	  std::list<TrackedObject* > _dependencyList;

};
//define a quickmacro to make things easier on derived classes
#define AUTO_SIZE unsigned long size(){return sizeof(*this);}
///Registering a dependency
#define REGISTER_TRACKED_DEPENDENCY(X)   this->addDependency(X); PRINT_FN("DEP [ %s ] TO [ %s ]",this->getName().c_str(), X->getName().c_str());
#define UNREGISTER_TRACKED_DEPENDENCY(X) this->removeDependency(X);

#endif
