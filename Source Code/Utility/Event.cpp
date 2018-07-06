#include "Headers/Event.h"

void Event::startEvent()
{
	_thisThread =  std::tr1::shared_ptr<boost::thread>(new boost::thread(&Event::run,boost::ref(*this)));
}

void Event::run()
{
	
	assert(_numberOfTicks);

	while(true)
	{
		try	{
			boost::this_thread::interruption_point();
		}
		catch(const boost::thread_interrupted&){break;}

		boost::this_thread::sleep(boost::posix_time::milliseconds((boost::int64_t)_tickInterval));
		_callback();

		if(_numberOfTicks > 0) _numberOfTicks--;
		if(_numberOfTicks = 0){
			boost::mutex::scoped_lock l(_mutex);
			_end = true;
		}

		boost::mutex::scoped_lock l(_mutex);
		if(_end) break;

		
	}
	Console::getInstance().printfn("EventManager: Deleting thread: %d", boost::this_thread::get_id());
	_end = false;
}





