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

#ifndef _EVENT_H_
#define _EVENT_H_

#include <boost/any.hpp>
#include <boost/function.hpp>  
#include <boost/enable_shared_from_this.hpp> 
#include "Hardware/Platform/Headers/SharedMutex.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

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


class Event : public boost::enable_shared_from_this<Event>
{
public:
	/// <summary>
	/// Creates a new event that runs in a separate thread
	/// </summary>
	/// <remarks>
	/// The class uses Boost::thread library (http://www.boost.org)
	/// </remarks>
	/// <param name="tickInterval">The delay (in milliseconds) between each callback</param>
	/// <param name="startOnCreate">The event begins processing as soon as it is created (no need to call 'startEvent()')</param>
	/// <param name="numberOfTicks">The number of times to call the callback function before the event is deleted</param>
	/// <param name="*f">The callback function</param>
	Event(U32 tickInterval, 
		  bool startOnCreate,
		  U32 numberOfTicks,
		  boost::function0<void> f):

	  _tickInterval(tickInterval),
	  _numberOfTicks(numberOfTicks),
	  _callback(f),
	  _end(false),
	  _paused(false){
		  if(startOnCreate) startEvent();
	  }

	Event(U32 tickInterval,
	      bool startOnCreate,
		  bool runOnce,
	      boost::function0<void> f):

	  _tickInterval(tickInterval),
      _numberOfTicks(1),
	  _callback(f),
	  _end(false),
	  _paused(false){
		  ///If runOnce is true, then we only run the event once (# of ticks is 1)
		  ///If runOnce is false, then we run the event until stopEvent() is called
		  runOnce ? _numberOfTicks = 1 : _numberOfTicks = -1;
		  if(startOnCreate) startEvent();
	  }
	~Event();
	void updateTickInterval(U32 tickInterval){WriteLock w_lock(_lock);_tickInterval = tickInterval;}
	void updateTickCounter(U32 numberOfTicks){WriteLock w_lock(_lock);_numberOfTicks = numberOfTicks;}
	void startEvent();
	void stopEvent();
	void interruptEvent();
	void pauseEvent(bool state);

private:
	std::string _name;
	U32 _tickInterval;
	U32 _numberOfTicks;
	std::tr1::shared_ptr<boost::thread> _thisThread;
	mutable SharedLock _lock;
	volatile bool _end;
	volatile bool _paused;
	boost::function0<void> _callback;

private:
	void run();

};

typedef std::tr1::shared_ptr<Event> Event_ptr;

#endif
