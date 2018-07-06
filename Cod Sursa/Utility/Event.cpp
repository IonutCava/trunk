#include "Headers/Event.h"

void Event::startEvent()
{
	_thrd = new boost::thread(boost::bind(&Event::eventThread,this));
}

void Event::eventThread()
{
	boost::thread::id thrdId = boost::this_thread::get_id();
	assert(_numberOfTicks);
	while(_numberOfTicks > 0)
	{
		_numberOfTicks--;
		boost::this_thread::sleep(boost::posix_time::milliseconds((boost::int64_t)_tickInterval));

		(*_callback)(_data,_paramType);
		 
	}
	
	cout << "EventManager: Deleting thread: " << thrdId << endl;
	_thrd->join();
	delete this;
}





