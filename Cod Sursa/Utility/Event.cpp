#include "Headers/Event.h"

void Event::startEvent()
{
	_end = false;
	_thrd.bind(&Event::eventThread,this);
}

void Event::eventThread()
{
	
	assert(_numberOfTicks);

	while(!_end)
	{
		_thrd.sleep(_tickInterval);
		_callback();

		if(_numberOfTicks > 0) _numberOfTicks--;
		if(_numberOfTicks = 0) _end = true;
	}
	
	
	cout << "EventManager: Deleting thread: " <<  boost::this_thread::get_id() << endl;
	delete this;
}





