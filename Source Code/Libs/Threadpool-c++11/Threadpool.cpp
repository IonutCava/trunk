#include "stdafx.h"

#include "Threadpool.hpp"

#include <functional>
#include <memory>
#include <exception>

namespace ctpl {
    thread_pool::thread_pool() { this->init(); }
    thread_pool::thread_pool(size_t nThreads) { this->init(); m_nThreads = nThreads; this->resize(nThreads); }

    // the destructor waits for all the functions in the queue to be finished
    thread_pool::~thread_pool() { this->interrupt(false); }

    // restart the pool
    void thread_pool::restart()
    {
        this->interrupt(false); // finish all existing tasks but prevent new ones
        this->init(); // reset atomic flags
        this->resize(m_nThreads);
    }

    // change the number of threads in the pool
    // should be called from one thread, otherwise be careful to not interleave, also with this->interrupt()
    void thread_pool::resize(size_t nThreads)
    {
        if (!ma_kill && !ma_interrupt) {

            size_t oldNThreads = m_threads.size();
            if (oldNThreads <= nThreads) {  // if the number of threads is increased

                m_threads.resize(nThreads);
                m_abort.resize(nThreads);

                for (size_t i = oldNThreads; i < nThreads; ++i)
                {
                    m_abort[i] = std::make_shared<std::atomic<bool>>(false);
                    this->setup_thread(i);
                }
            } else {  // the number of threads is decreased

                for (size_t i = oldNThreads - 1; i >= nThreads; --i)
                {
                    *m_abort[i] = true;  // this thread will finish
                    m_threads[i]->detach();
                }
                {
                    // stop the detached threads that were waiting
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cond.notify_all();
                }
                m_threads.resize(nThreads); // safe to delete because the threads are detached
                m_abort.resize(nThreads); // safe to delete because the threads have copies of shared_ptr of the flags, not originals
            }
        }
    }

    // wait for all computing threads to finish and stop all threads
    // may be called asynchronously to not pause the calling thread while waiting
    // if kill == true, all the functions in the queue are run, otherwise the queue is cleared without running the functions
    void thread_pool::interrupt(bool kill)
    {
        if (kill) {
            if (ma_kill) return;
            ma_kill = true;

            for (size_t i = 0, n = this->size(); i < n; ++i)
                *m_abort[i] = true;  // command the threads to stop
        } else {
            if (ma_interrupt || ma_kill) return;
            ma_interrupt = true;  // give the waiting threads a command to finish
        }

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.notify_all();  // stop all waiting threads
        }
        // wait for the computing threads to finish
        for (size_t i = 0; i < m_threads.size(); ++i)
        {
            if (m_threads[i]->joinable())
                m_threads[i]->join();
        }

        this->clear_queue();

        m_threads.clear();
        m_abort.clear();
    }

   
    void thread_pool::clear_queue()
    {
        std::function<void(size_t id)> * _f = nullptr;
        while (m_queue.pop(_f))
            delete _f; // empty the queue
    }

    // reset all flags
    void thread_pool::init() { ma_n_idle = 0; ma_kill = false; ma_interrupt = false; }

    // each thread pops jobs from the queue until:
    //  - the queue is empty, then it waits (idle)
    //  - its abort flag is set (terminate without emptying the queue)
    //  - a global interrupt is set, then only idle threads terminate
    void thread_pool::setup_thread(size_t i)
    {
        // a copy of the shared ptr to the abort
        std::shared_ptr<std::atomic<bool>> abort_ptr(m_abort[i]);

        auto f = [this, i, abort_ptr]()
        {
            std::atomic<bool> & abort = *abort_ptr;
            std::function<void(size_t id)> * _f = nullptr;
            bool more_tasks = m_queue.pop(_f);

            while (true)
            {
                while (more_tasks) // if there is anything in the queue
                {
                    // at return, delete the function even if an exception occurred
                    std::unique_ptr<std::function<void(size_t id)>> func(_f);
                    (*_f)(i);
                    if (abort)
                        return; // return even if the queue is not empty yet
                    else
                        more_tasks = m_queue.pop(_f);
                }

                // the queue is empty here, wait for the next command
                std::unique_lock<std::mutex> lock(m_mutex);
                ++ma_n_idle;
                m_cond.wait(lock, [this, &_f, &more_tasks, &abort]() { more_tasks = m_queue.pop(_f); return abort || ma_interrupt || more_tasks; });
                --ma_n_idle;
                if (!more_tasks) return; // we stopped waiting either because of interruption or abort
            }
        };

        m_threads[i].reset(new std::thread(f));
    }
}; //namespace ctpl