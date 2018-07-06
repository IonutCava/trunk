#include "core.h"

#include "Headers/Event.h"

void Event::startEvent(){
	_thisThread =  std::tr1::shared_ptr<boost::thread>(New boost::thread(&Event::run,boost::ref(*this)));
}

void Event::stopEvent(){
	if(_end == true) 
		return;
	_end = true;
	if(_thisThread.get()){
		if(_thisThread->joinable()){
			_thisThread->join();
		}
	}
}

void Event::run(){
	
	assert(_numberOfTicks);
	bool shouldEnd = false;
	while(true){
	try	{
		if(shouldEnd) break;
		boost::this_thread::interruption_point();

		if(_tickInterval > 0){
			boost::this_thread::sleep(boost::posix_time::milliseconds((boost::int64_t)_tickInterval));
		}

		_callback();
	
		if(_numberOfTicks > 0) _numberOfTicks--;
		if(_numberOfTicks == 0)	_end = true;
		
		shouldEnd = _end;
	}catch(const boost::thread_interrupted&){break;}
	}

	PRINT_FN(Locale::get("EVENT_DELETE_THREAD"), boost::this_thread::get_id());
	_end = false;
	
}





