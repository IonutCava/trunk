#include "Headers/Event.h"

void Event::startEvent(){
	_thisThread =  std::tr1::shared_ptr<boost::thread>(New boost::thread(&Event::run,boost::ref(*this)));
}

void Event::run(){
	
	assert(_numberOfTicks);
	bool shouldEnd = false;
	while(true){
		if(shouldEnd) break;
		try	{
			boost::this_thread::interruption_point();
		}
		catch(const boost::thread_interrupted&){break;}

		if(_tickInterval > 0){
			boost::this_thread::sleep(boost::posix_time::milliseconds((boost::int64_t)_tickInterval));
		}
		_callback();

		if(_numberOfTicks > 0) _numberOfTicks--;
		if(_numberOfTicks == 0){
			_mutex.lock();
			_end = true;
			_mutex.unlock();
		}

		_mutex.lock();
		shouldEnd = _end;
		_mutex.unlock();

		
	}
	PRINT_FN("EventManager: Deleting thread: %d", boost::this_thread::get_id());
	_mutex.lock();
	_end = false;
	_mutex.unlock();
}





