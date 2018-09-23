#include "stdafx.h"

#include "Headers/Defines.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Time/Headers/ApplicationTimer.h"

#include <atomic>

namespace Divide {

TEST(TaskPoolContructionTest)
{
    Console::toggleErrorStream(false);
    TaskPool test;

    // Not enough workers
    bool init = test.init(0, TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_FALSE(init);

    // Valid
    init = test.init(1, TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    // Double init
    init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_FALSE(init);
}

TEST(ParallelForTest)
{
    Console::toggleErrorStream(false);

    TaskPool test;
    bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    const U32 partitionSize = 4;
    const U32 loopCount = partitionSize * 4 + 2;

    std::atomic_uint loopCounter = 0;
    std::atomic_uint totalCounter = 0;
    auto loop = [&totalCounter, &loopCounter](const Task& parentTask, U32 start, U32 end) {
        ++loopCounter;
        for (U32 i = start; i < end; ++i) {
            ++totalCounter;
        }
    };

    parallel_for(test, loop, loopCount, partitionSize);

    CHECK_EQUAL(loopCounter, 5u);
    CHECK_EQUAL(totalCounter, 18u);
}


TEST(TaskCallbackTest)
{
    TaskPool test;
    bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    std::atomic_bool testValue = false;

    auto task = [](const Task& parentTask) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    };

    auto callback = [&testValue]() {
        testValue = true;
    };

    TaskHandle job = CreateTask(test, task);
    job.startTask(callback);

    CHECK_FALSE(testValue);

    job.wait();

    CHECK_FALSE(testValue);

    test.flushCallbackQueue();

    CHECK_TRUE(testValue);
}

namespace {
    class ThreadedTest {
      public:
        ThreadedTest()
        {
            _testValue = false;
        }

        void setTestValue(bool state) {
            _testValue = state;
        }

        bool getTestValue() {
            return _testValue;
        }

        void threadedFunction(const Task& parentTask) {
            ACKNOWLEDGE_UNUSED(parentTask);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            _testValue = true;
        }
      private:
        std::atomic_bool _testValue;
    };
};

TEST(TaskClassMemberCallbackTest)
{
    TaskPool test;
    bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    ThreadedTest testObj;

    TaskHandle job = CreateTask(test, [&testObj](const Task& parentTask) { testObj.threadedFunction(parentTask); } );
    job.startTask( [&testObj]() { testObj.setTestValue(false); } );

    CHECK_FALSE(testObj.getTestValue());

    job.wait();

    CHECK_TRUE(testObj.getTestValue());

    test.flushCallbackQueue();

    CHECK_FALSE(testObj.getTestValue());
}

TEST(TaskSpeedTest)
{
    {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
        CHECK_TRUE(init);

        Time::ProfileTimer timer;

        timer.start();
        TaskHandle job = CreateTask(test,
            [](const Task& parentTask) {
            // NOP
        }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i)
        {
            CreateTask(test, &job,
                [](const Task& parentTask) {
                // NOP
            }
            ).startTask();
        }

        job.startTask().wait();
        F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.stop() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (blocking): 60K tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_LOCKFREE);
        CHECK_TRUE(init);

        Time::ProfileTimer timer;

        timer.start();
        TaskHandle job = CreateTask(test,
            [](const Task& parentTask) {
            // NOP
        }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i)
        {
            CreateTask(test, &job,
                [](const Task& parentTask) {
                // NOP
            }
            ).startTask();
        }

        job.startTask().wait();
        F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.stop() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (lockfree): 60K tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BOOST_ASIO);
        CHECK_TRUE(init);

        Time::ProfileTimer timer;

        timer.start();
        TaskHandle job = CreateTask(test,
            [](const Task& parentTask) {
            // NOP
        }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i)
        {
            CreateTask(test, &job,
                [](const Task& parentTask) {
                // NOP
            }
            ).startTask();
        }

        job.startTask().wait();
        F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.stop() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (boost::asio): 60K tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
        CHECK_TRUE(init);

        const U32 partitionSize = 256;
        const U32 loopCount = partitionSize * 8192 + 2;

        Time::ProfileTimer timer;
        timer.start();
        parallel_for(test,
                     [](const Task& parentTask, U32 start, U32 end) {
                        // NOP
                     },
                     loopCount,
                     partitionSize);
        F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.stop() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - blocking): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_LOCKFREE);
        CHECK_TRUE(init);

        const U32 partitionSize = 256;
        const U32 loopCount = partitionSize * 8192 + 2;

        Time::ProfileTimer timer;
        timer.start();
        parallel_for(test,
                    [](const Task& parentTask, U32 start, U32 end) {
                        // NOP
                    },
                    loopCount,
                    partitionSize);
        F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.stop() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - lockfree): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }
      {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BOOST_ASIO);
        CHECK_TRUE(init);

        const U32 partitionSize = 256;
        const U32 loopCount = partitionSize * 8192 + 2;

        Time::ProfileTimer timer;
        timer.start();
        parallel_for(test,
                    [](const Task& parentTask, U32 start, U32 end) {
                        // NOP
                    },
                    loopCount,
                    partitionSize);
        F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.stop() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - boost::asio): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }
}

TEST(TaskPriorityTest)
{
    TaskPool test;
    bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    U32 callbackValue = 0;
    auto testFunction = [&callbackValue](const Task& parentTask) {
        callbackValue++;
    };

    auto callback = [&callbackValue]() {
        callbackValue++;
    };

    TaskHandle job = CreateTask(test, testFunction);
    job.startTask(callback);
    job.wait();
    CHECK_EQUAL(callbackValue, 1u);
    test.flushCallbackQueue();
    CHECK_EQUAL(callbackValue, 2u);

    job = CreateTask(test, testFunction);
    job.startTask();
    job.wait();
    CHECK_EQUAL(callbackValue, 3u);
    test.flushCallbackQueue();
    CHECK_EQUAL(callbackValue, 3u);

    job = CreateTask(test, testFunction);
    job.startTask(TaskPriority::REALTIME, callback);
    job.wait();
    CHECK_EQUAL(callbackValue, 5u);
}

}; //namespace Divide
