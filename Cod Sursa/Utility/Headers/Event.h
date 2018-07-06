#ifndef _EVENT_H_
#define _EVENT_H_

#include "resource.h"
#include <boost/any.hpp>
#include  "MultiThreading/threadHandler.h"

using boost::any_cast;

enum CallbackParam
{
	TYPE_INTEGER,
	TYPE_STRING,
	TYPE_FLOAT,
	TYPE_DOUBLE,
	TYPE_CHAR

};


class Event
{
public:
	/// <summary>
	/// Creates a new event that runs in a separate thread
	/// </summary>
	/// <remarks>
	/// The class uses Boost::thread library (http://www.boost.org)
	/// </remarks>
	/// <param name="tickInterval">The delay (in milliseconds) between each callback</param>
	/// <param name="repeatable">True: the callback function is called once every 'tickInterval' milliseconds for 'numberOfTicks' times
	///                          False: the callback function is called only once after 'tickInterval' milliseconds</param>

	/// <param name="startOnCreate">The event begins processing as soon as it is created (no need to call 'startEvent()')</param>
	/// <param name="numberOfTicks">The number of times to call the callback function before the event is deleted</param>
	/// <param name="*f">The callback function</param>
	Event(F32 tickInterval, 
		  bool startOnCreate,
		  U32 numberOfTicks,
		  boost::function0<void> f):

	  _tickInterval(tickInterval),
	  _numberOfTicks(numberOfTicks),
	  _callback(f)
	  {
		  if(startOnCreate) startEvent();
	  }

	Event(F32 tickInterval,
	      bool startOnCreate,
		  bool runOnce,
	      boost::function0<void> f):

	  _tickInterval(tickInterval),
      _numberOfTicks(1),
	  _callback(f)
	  {
		  //If runOnce is true, then we only run the event once (# of ticks is 1)
		  //If runOnce is false, then we run the event untill stopEvent() is called
		  runOnce? _numberOfTicks = 1 : _numberOfTicks = -1;
		  if(startOnCreate) startEvent();
	  }
	~Event(){_end=true;}
	void updateTickInterval(F32 tickInterval){_tickInterval = tickInterval;}
	void updateTickCounter(U32 numberOfTicks){_numberOfTicks = numberOfTicks;}
	void startEvent();
	void stopEvent(){_end = true;};
	

private:
	std::string _name;
	F32 _tickInterval;
	U32 _numberOfTicks;
	Thread _thrd;
	bool _end;
	void eventThread();
	boost::function0<void> _callback;

};

#endif
