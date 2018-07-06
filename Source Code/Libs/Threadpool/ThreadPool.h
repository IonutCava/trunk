#ifndef CONCURRENT_THREADPOOL_H
#define CONCURRENT_THREADPOOL_H

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

#include <ConcurrentQueue/blockingconcurrentqueue.h>

/**
 *  Simple ThreadPool that creates `threadCount` threads upon its creation,
 *  and pulls from a queue to get new jobs.
 *
 *  This class requires a number of c++11 features be present in your compiler.
 */
class ThreadPool
{
public:
    typedef std::function<void()> Job;

    explicit ThreadPool(const int threadCount) :
        _isRunning(true)
    {
        _jobsLeft.store(0);
        _threads.reserve(threadCount);
        for (int index = 0; index < threadCount; ++index)
        {
            _threads.push_back(std::thread([&]
            {
                /**
                *  Take the next job in the queue and run it.
                *  Notify the main thread that a job has completed.
                */
                while (_isRunning)
                {
                    Job job;
                    _queue.wait_dequeue(job);
                    job();

                    _jobsLeft.fetch_sub(1);
                    _waitVar.notify_one();
                }
            }));
        }
    }

    /**
     *  JoinAll on deconstruction
     */
    ~ThreadPool()
    {
        JoinAll();
    }

    /**
     *  Add a new job to the pool. If there are no jobs in the queue,
     *  a thread is woken up to take the job. If all threads are busy,
     *  the job is added to the end of the queue.
     */
    void AddJob(const Job& job)
    {
        _queue.enqueue(job);
        _jobsLeft.fetch_add(1);
        _jobAvailableVar.notify_one();
    }

    /**
     *  Join with all threads. Block until all threads have completed.
     *  The queue may be filled after this call, but the threads will
     *  be done. After invoking `ThreadPool::JoinAll`, the pool can no
     *  longer be used.
     */
    void JoinAll()
    {
        if (!_isRunning)
        {
            return;
        }
        _isRunning = false;

        // add empty jobs to wake up threads
        const int threadCount = (int)(_threads.size());
        for (int index = 0; index < threadCount; ++index)
        {
            AddJob([]
            {
            });
        }

        // note that we're done, and wake up any thread that's
        // waiting for a new job
        _jobAvailableVar.notify_all();

        for (std::thread& thread : _threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    /**
     *  Wait for the pool to empty before continuing.
     *  This does not call `std::thread::join`, it only waits until
     *  all jobs have finished executing.
     */
    void WaitAll()
    {
        if (_jobsLeft.load() > 0)
        {
            std::unique_lock<std::mutex> lock(_jobsLeftMutex);
            _waitVar.wait(lock, [&]
            {
                return _jobsLeft.load() == 0;
            });
        }
    }

    /**
     *  Get the vector of threads themselves, in order to set the
     *  affinity, or anything else you might want to do
     */
    std::vector<std::thread>& GetThreads()
    {
        return _threads;
    }

    /**
     *  Return the next job in the queue to run it in the main thread
     */
    Job GetNextJob()
    {
        Job job;

        if (!_queue.try_dequeue(job)) {
            return nullptr;
        }

        _jobsLeft.fetch_sub(1);
        _waitVar.notify_one();

        return job;
    }

private:
    std::vector<std::thread> _threads;
    moodycamel::BlockingConcurrentQueue<Job> _queue;

    std::atomic_int _jobsLeft;
    bool _isRunning;
    std::condition_variable _jobAvailableVar;
    std::condition_variable _waitVar;
    std::mutex _jobsLeftMutex;
};

#undef CONTIGUOUS_JOBS_MEMORY
#endif //CONCURRENT_THREADPOOL_H
