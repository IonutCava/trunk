#ifndef THREAD_HANDLER_H_
#define THREAD_HANDLER_H_

#include "resource.h"
#include <boost/thread.hpp>

class Thread
{
public:
	
	Thread(){}
	~Thread() {join(); delete _thisThread; _thisThread = NULL;}

	//template< class A1, class M, class T >
	void bind(boost::function0<void> f) {_thisThread = new boost::thread(f);}

	void sleep(F32 _duration) { boost::this_thread::sleep(boost::posix_time::milliseconds((boost::int64_t)_duration));}

	void join() {_thisThread->join();}
private:
	boost::thread* _thisThread;
};
#endif