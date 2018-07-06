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
/*
	Code source: http://www.gamedev.net/page/resources/_/technical/game-programming/enginuity-part-ii-r1954
*/

#ifndef TRACKED_OBJECT_H_
#define TRACKED_OBJECT_H_
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/atomic.hpp>
#include "Hardware/Platform/Headers/SharedMutex.h"

///A tracked object takes car of it's own reference counting and knows it's own size
///It also schedules it's own deletion (a pompous name for a smart pointer)
class TrackedObject : private boost::noncopyable  {
    public:
	  ///Increase reference count
	  void AddRef();
	  ///Decrease reference count
	  bool SubRef();
	  ///Add object dependency (dependent objects are ref counted with the parent object)
	  void addDependency(TrackedObject* const obj);
	  ///Remove an object dependency
	  void removeDependency(TrackedObject* const obj);
	  ///How many references does this object belong to
	  inline const long getRefCount() const {return _refCount;}
	  ///Did the object reach 0 refs? If so, it's marked for deleton
	  virtual const bool shouldDelete() const {return _shouldDelete;}
	  ///Do we have any parent object? (if so, do not delete the object. The parent should handle that)
	  virtual const bool hasParents()  const {return !_parentList.empty();}
	  ///We can manually force a deletion
	  virtual void scheduleDeletion(){_shouldDelete = true;}
	  ///Or we can cancel the deletion before it happens
	  virtual void cancelDeletion()  {_shouldDelete = false;}
	  ///For memory consumption later on
	  virtual const unsigned long size() const {return sizeof(*this);}

    protected:
	  TrackedObject();
	  virtual ~TrackedObject();
	  ///Keep track if we have a parent or not
	  virtual void addParent(TrackedObject* const obj);
	  ///Remove a parent
	  virtual void removeParent(TrackedObject* const obj);
   private:
      //mutable SharedLock _dependencyLock;
      boost::atomic<long> _refCount;
      boost::atomic<bool> _shouldDelete;
	  std::list<TrackedObject* > _dependencyList;
	  std::list<TrackedObject* > _parentList;
};
//define a quickmacro to make things easier on derived classes
#define AUTO_SIZE unsigned long size(){return sizeof(*this);}
///Registering a dependency
#define REGISTER_TRACKED_DEPENDENCY(X)   this->addDependency(X);
#define UNREGISTER_TRACKED_DEPENDENCY(X) this->removeDependency(X);

#endif
