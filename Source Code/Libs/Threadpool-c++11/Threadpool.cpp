#include "Threadpool.hpp"

#include <functional>
#include <memory>
#include <exception>

namespace ctpl {

thread_pool::thread_pool()
    : q(_ctplThreadPoolLength_)
{
    this->init();
}

thread_pool::thread_pool(int nThreads, int queueSize)
    : q(queueSize)
{
    this->init();
    this->resize(nThreads);
}

// the destructor waits for all the functions in the queue to be finished
thread_pool::~thread_pool()
{
    this->stop(true);
}

// get the number of running threads in the pool
int thread_pool::size() {
    return static_cast<int>(this->threads.size());
}

// number of idle threads
int thread_pool::n_idle() {
    return this->nWaiting;
}

std::thread & thread_pool::get_thread(int i) {
    return *this->threads[i];
}

// change the number of threads in the pool
// should be called from one thread, otherwise be careful to not interleave, also with this->stop()
// nThreads must be >= 0
void thread_pool::resize(int nThreads) {
    if (!this->isStop && !this->isDone) {
        int oldNThreads = static_cast<int>(this->threads.size());
        if (oldNThreads <= nThreads) {  // if the number of threads is increased
            this->threads.resize(nThreads);
            this->flags.resize(nThreads);

            for (int i = oldNThreads; i < nThreads; ++i) {
                this->flags[i] = std::make_shared<std::atomic<bool>>(false);
                this->set_thread(i);
            }
        }
        else {  // the number of threads is decreased
            for (int i = oldNThreads - 1; i >= nThreads; --i) {
                *this->flags[i] = true;  // this thread will finish
                this->threads[i]->detach();
            }
            {
                // stop the detached threads that were waiting
                std::unique_lock<std::mutex> lock(this->mutex);
                this->cv.notify_all();
            }
            this->threads.resize(nThreads);  // safe to delete because the threads are detached
            this->flags.resize(nThreads);  // safe to delete because the threads have copies of shared_ptr of the flags, not originals
        }
    }
}

// empty the queue
void thread_pool::clear_queue() {
    std::function<void(int id)> * _f;
    while (this->q.pop(_f))
        delete _f;  // empty the queue
}

// pops a functional wraper to the original function
std::function<void(int)> thread_pool::pop() {
    std::function<void(int id)> * _f = nullptr;
    this->q.pop(_f);
    std::unique_ptr<std::function<void(int id)>> func(_f);  // at return, delete the function even if an exception occurred

    std::function<void(int)> f;
    if (_f)
        f = *_f;
    return f;
}


// wait for all computing threads to finish and stop all threads
// may be called asyncronously to not pause the calling thread while waiting
// if isWait == true, all the functions in the queue are run, otherwise the queue is cleared without running the functions
void thread_pool::stop(bool isWait) {
    if (!isWait) {
        if (this->isStop)
            return;
        this->isStop = true;
        for (int i = 0, n = this->size(); i < n; ++i) {
            *this->flags[i] = true;  // command the threads to stop
        }
        this->clear_queue();  // empty the queue
    }
    else {
        if (this->isDone || this->isStop)
            return;
        this->isDone = true;  // give the waiting threads a command to finish
    }
    {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->cv.notify_all();  // stop all waiting threads
    }
    for (int i = 0; i < static_cast<int>(this->threads.size()); ++i) {  // wait for the computing threads to finish
        if (this->threads[i]->joinable())
            this->threads[i]->join();
    }
    // if there were no threads in the pool but some functors in the queue, the functors are not deleted by the threads
    // therefore delete them here
    this->clear_queue();
    this->threads.clear();
    this->flags.clear();
}

void thread_pool::set_thread(int i) {
    std::shared_ptr<std::atomic<bool>> flag(this->flags[i]);  // a copy of the shared ptr to the flag
    auto f = [this, i, flag/* a copy of the shared ptr to the flag */]() {
        std::atomic<bool> & _flag = *flag;
        std::function<void(int id)> * _f;
        bool isPop = this->q.pop(_f);
        while (true) {
            while (isPop) {  // if there is anything in the queue
                std::unique_ptr<std::function<void(int id)>> func(_f);  // at return, delete the function even if an exception occurred
                (*_f)(i);

                if (_flag)
                    return;  // the thread is wanted to stop, return even if the queue is not empty yet
                else
                    isPop = this->q.pop(_f);
            }

            // the queue is empty here, wait for the next command
            std::unique_lock<std::mutex> lock(this->mutex);
            ++this->nWaiting;
            this->cv.wait(lock, [this, &_f, &isPop, &_flag]() { isPop = this->q.pop(_f); return isPop || this->isDone || _flag; });
            --this->nWaiting;

            if (!isPop)
                return;  // if the queue is empty and this->isDone == true or *flag then return
        }
    };
    this->threads[i].reset(new std::thread(f));  // compiler may not support std::make_unique()
}

void thread_pool::init() {
    this->nWaiting = 0;
    this->isStop = false;
    this->isDone = false;
}

}; //namespace ctpl