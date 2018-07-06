#include "Headers/Event.h"

void Event::startEvent()
{
	_end = false;
	_thrd.bind(boost::bind(&Event::eventThread,this));
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
	
	
	Con::getInstance().printfn("EventManager: Deleting thread: %d", boost::this_thread::get_id());
	delete this;
}





