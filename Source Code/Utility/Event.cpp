#include "core.h"

#include "Headers/Event.h"

Event::~Event(){
	WriteLock w_lock(_lock);
	_end = true;
	w_lock.unlock();
	stopEvent();
}

void Event::startEvent(){
	_thisThread =  std::tr1::shared_ptr<boost::thread>(New boost::thread(&Event::run,boost::ref(*this)));
}

void Event::interruptEvent(){
	WriteLock w_lock(_lock);
	_end = true; 
	w_lock.unlock();
	_thisThread->interrupt();
}

void Event::stopEvent(){
	WriteLock w_lock(_lock);
	if(_thisThread.get()){
		_end = true;
		_lock.unlock();
		if(_thisThread->joinable()){
			_thisThread->join();
		}
	}
}

void Event::pauseEvent(bool state){
	if(_thisThread.get()){
		WriteLock w_lock(_lock);
		_paused = state;
	}
}

void Event::run(){
	
	while(true){
		UpgradableReadLock ur_lock(_lock);
		while(_paused) {boost::this_thread::sleep(boost::posix_time::milliseconds(_tickInterval));}
		if(_end) break;

		try	{
			boost::this_thread::interruption_point();
		
			if(_tickInterval > 0){
				boost::this_thread::sleep(boost::posix_time::milliseconds(_tickInterval));
			}

			_callback();
	
			if(_numberOfTicks > 0){
				UpgradeToWriteLock uw_lock(ur_lock);
				_numberOfTicks--;
			}else if(_numberOfTicks == 0){
				UpgradeToWriteLock uw_lock(ur_lock);
				_end = true;
			}else{
				//_numberOfTicks == -1, run forever
			}
		}catch(const boost::thread_interrupted&){
			break;
		}
	}

	D_PRINT_FN(Locale::get("EVENT_DELETE_THREAD"), boost::this_thread::get_id());
}





