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

#ifndef _TASKS_H_
#define _TASKS_H_

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
