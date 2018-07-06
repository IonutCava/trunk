/*********************************************************
*
*  Copyright (C) 2014 by Vitaliy Vitsentiy
*  https://github.com/vit-vit/CTPL
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*
*  October 2015, Jonathan Hadida:
*   - rename, reformat and comment some functions
*   - add a restart function
*  	- fix a few unsafe spots, eg:
*  		+ order of testing in setup_thread;
*  		+ atomic guards on pushes;
*  		+ make clear_queue private
*
*********************************************************/


#ifndef CTPL_H_INCLUDED
#define CTPL_H_INCLUDED

#include <functional>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <future>
#include <mutex>
#include <queue>


// thread pool to run user's functors with signature
//      ret func(size_t id, other_params)
// where id is the index of the thread that runs the functor
// ret is some return type


namespace ctpl {

    namespace detail {
        template <typename T>
        class Queue {
            public:
            bool push(T const & value) {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_queue.push(value);
                return true;
            }
            // deletes the retrieved element, do not use for non integral types
            bool pop(T & v) {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_queue.empty())
                    return false;
                v = m_queue.front();
                m_queue.pop();
                return true;
            }
            bool empty() {
                std::unique_lock<std::mutex> lock(m_mutex);
                return m_queue.empty();
            }
            private:
            std::queue<T> m_queue;
            std::mutex    m_mutex;
        };
    }

    class thread_pool {

        public:

        thread_pool();
        thread_pool(size_t nThreads);

        // the destructor waits for all the functions in the queue to be finished
        ~thread_pool();

        // get the number of running threads in the pool
        inline size_t size() const { return m_threads.size(); }

        // number of idle threads
        inline size_t n_idle() const { return ma_n_idle; }
        inline size_t n_pending() { return this->m_nPending; }
        // get a specific thread
        inline std::thread & get_thread(size_t i) { return *m_threads.at(i); }

        inline bool idle() { return m_queue.empty(); }

        // restart the pool
        void restart();

        // change the number of threads in the pool
        // should be called from one thread, otherwise be careful to not interleave, also with this->interrupt()
        void resize(size_t nThreads);

        // wait for all computing threads to finish and stop all threads
        // may be called asynchronously to not pause the calling thread while waiting
        // if kill == true, all the functions in the queue are run, otherwise the queue is cleared without running the functions
        void interrupt(bool kill = false);

        template<typename F, typename... Rest>
        auto push(F && f, Rest&&... rest) ->std::future<decltype(f(0, rest...))>
        {
            if (!ma_kill && !ma_interrupt)
            {
                auto pck = std::make_shared<std::packaged_task< decltype(f(0, rest...)) (size_t) >>(
                    std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Rest>(rest)...)
                    );
                auto _f = new std::function<void(size_t id)>([pck](size_t id) { (*pck)(id); });

                ++this->m_nPending;
                m_queue.push(_f);
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond.notify_one();
                return pck->get_future();
            } else return std::future<decltype(f(0, rest...))>();
        }

        // run the user's function that accepts argument size_t - id of the running thread. returned value is templatized
        // operator returns std::future, where the user can get the result and rethrow the catched exceptions
        template<typename F>
        auto push(F && f) ->std::future<decltype(f(0))>
        {
            if (!ma_kill && !ma_interrupt)
            {
                auto pck = std::make_shared<std::packaged_task< decltype(f(0)) (size_t) >>(std::forward<F>(f));
                auto _f = new std::function<void(size_t id)>([pck](size_t id) { (*pck)(id); });

                ++this->m_nPending;
                m_queue.push(_f);
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cond.notify_one();
                return pck->get_future();
            } else return std::future<decltype(f(0))>();
        }

        void clear_queue();

        private:

        // deleted
        thread_pool(const thread_pool &);// = delete;
        thread_pool(thread_pool &&);// = delete;
        thread_pool & operator=(const thread_pool &);// = delete;
        thread_pool & operator=(thread_pool &&);// = delete;

                                                // clear all tasks

        // reset all flags
        void init();

        // each thread pops jobs from the queue until:
        //  - the queue is empty, then it waits (idle)
        //  - its abort flag is set (terminate without emptying the queue)
        //  - a global interrupt is set, then only idle threads terminate
        void setup_thread(size_t i);

        // ----------  =====  ----------

        std::vector<std::unique_ptr<std::thread>>        m_threads;
        std::vector<std::shared_ptr<std::atomic<bool>>>  m_abort;
        detail::Queue<std::function<void(size_t id)> *>  m_queue;

        std::atomic<bool>    ma_interrupt, ma_kill;
        std::atomic<size_t>  ma_n_idle;
        std::atomic<size_t>  m_nThreads;
        std::atomic<size_t>  m_nPending;  // how many tasks are waiting
        std::mutex               m_mutex;
        std::condition_variable  m_cond;
    };
}

#endif