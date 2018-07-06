/*
   Copyright (c) 2014 DIVIDE-Studio
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

/*
    Code source: http://www.gamedev.net/page/resources/_/technical/game-programming/enginuity-part-ii-r1954
*/

#ifndef TRACKED_OBJECT_H_
#define TRACKED_OBJECT_H_
#include <list>
#include "Hardware/Platform/Headers/SharedMutex.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

///A tracked object takes car of it's own reference counting and knows it's own size
///It also schedules it's own deletion (a pompous name for a smart pointer)
class TrackedObject : private NonCopyable, public GUIDWrapper {
    public:
      ///Increase reference count
      void AddRef();
      ///Decrease reference count
      bool SubRef(bool recursive = true);
      ///Add object dependency (dependent objects are ref counted with the parent object)
      void addDependency(TrackedObject* const obj);
      ///Remove an object dependency
      void removeDependency(TrackedObject* const obj);
      ///How many references does this object belong to
      inline const long getRefCount() const {return _refCount;}
      ///For memory consumption later on
      virtual const unsigned long size() const {return sizeof(*this);}

    protected:
      TrackedObject();
      virtual ~TrackedObject();
      ///A callback that's called when the ref count is modified. If increase is true, the ref count was incremented, else it was decremented
      virtual void refModifyCallback(bool increase);

   private:
      //mutable SharedLock _dependencyLock;
      std::atomic<long> _refCount;
      std::list<TrackedObject* > _dependencyList;
};
//define a quickmacro to make things easier on derived classes
#define AUTO_SIZE unsigned long size(){return sizeof(*this);}
///Registering a dependency
#define REGISTER_TRACKED_DEPENDENCY(X)   this->addDependency(X);
#define UNREGISTER_TRACKED_DEPENDENCY(X) this->removeDependency(X);

}; //namespace Divide

#endif
