#include "resource.h"
#include <boost/any.hpp>
#include <boost/thread.hpp>
#include <boost/thread.hpp>
#include <unordered_map>

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
typedef void (*callback)(boost::any a, CallbackParam b);
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
		  callback f,
		  boost::any data,
		  CallbackParam param):

	  _tickInterval(tickInterval),
	  _numberOfTicks(numberOfTicks),
	  _callback(f),
	  _data(data),
	  _paramType(param)
	  {
		  if(startOnCreate) startEvent();
	  }

	Event(F32 tickInterval,
	      bool startOnCreate,
	      callback f,
		  boost::any data,
		  CallbackParam param):

	  _tickInterval(tickInterval),
      _numberOfTicks(1),
	  _callback(f),
	  _data(data),
	  _paramType(param)
	  {
		  if(startOnCreate) startEvent();
	  }
	
	void updateTickInterval(F32 tickInterval){_tickInterval = tickInterval;}
	void updateTickCounter(U32 numberOfTicks){_numberOfTicks = numberOfTicks;}
	void startEvent();
	

private:
	string _name;
	F32 _tickInterval;
	U32 _numberOfTicks;
	boost::thread *_thrd;
	boost::any _data;
	CallbackParam _paramType;

	void eventThread();
	callback _callback;

};


