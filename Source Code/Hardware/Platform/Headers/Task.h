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

#ifndef _TASKS_H_
#define _TASKS_H_

#include "SharedMutex.h"
#include <boost/any.hpp>
#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Utility/Headers/GUIDWrapper.h"

using boost::any_cast;

enum CallbackParam
{
    TYPE_INTEGER,
    TYPE_MEDIUM_INTEGER,
    TYPE_SMALL_INTEGER,
    TYPE_UNSIGNED_INTEGER,
    TYPE_MEDIUM_UNSIGNED_INTEGER,
    TYPE_SMALL_UNSIGNED_INTEGER,
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR
};
///Using std::atomic / boost::atomic for thread-shared data to avoid locking
class Task : public GUIDWrapper, public boost::enable_shared_from_this<Task>
{
public:
    /// <summary>
    /// Creates a new Task that runs in a separate thread
    /// </summary>
    /// <remarks>
    /// The class uses Boost::thread library (http://www.boost.org)
    /// </remarks>
    /// <param name="tickInterval">The delay (in milliseconds) between each callback</param>
    /// <param name="startOnCreate">The Task begins processing as soon as it is created (no need to call 'startTask()')</param>
    /// <param name="numberOfTicks">The number of times to call the callback function before the Task is deleted</param>
    /// <param name="*f">The callback function</param>
    Task(boost::threadpool::pool* tp,
          U32 tickInterval,
          bool startOnCreate,
          I32 numberOfTicks,
          boost::function0<void> f) : GUIDWrapper(),
      _tp(tp),
      _tickInterval(tickInterval),
      _numberOfTicks(numberOfTicks),
      _callback(f),
      _end(false),
      _paused(false),
      _done(false){
          if(startOnCreate) startTask();
      }

    Task(boost::threadpool::pool* tp,
          U32 tickInterval,
          bool startOnCreate,
          bool runOnce,
          boost::function0<void> f) : GUIDWrapper(),
      _tp(tp),
      _tickInterval(tickInterval),
      _numberOfTicks(1),
      _callback(f),
      _end(false),
      _paused(false),
      _done(false){
          ///If runOnce is true, then we only run the Task once (# of ticks is 1)
          ///If runOnce is false, then we run the Task until stopTask() is called
          runOnce ? _numberOfTicks = 1 : _numberOfTicks = -1;
          if(startOnCreate) startTask();
      }
    ~Task();
    void updateTickInterval(U32 tickInterval){_tickInterval = tickInterval;}
    void updateTickCounter(I32 numberOfTicks){_numberOfTicks = numberOfTicks;}
    void startTask();
    void stopTask();
    void pauseTask(bool state);

    inline bool isFinished() const {return _done;}

private:
    mutable SharedLock _lock;
    mutable boost::atomic<U32> _tickInterval;
    mutable boost::atomic<I32> _numberOfTicks;
    mutable boost::atomic<bool> _end;
    mutable boost::atomic<bool> _paused;
    mutable boost::atomic<bool> _done;
    boost::function0<void> _callback;
    boost::threadpool::pool* _tp;

protected:
    void run();
};

typedef std::tr1::shared_ptr<Task> Task_ptr;

#endif
