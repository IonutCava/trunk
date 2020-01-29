#include "stdafx.h"

#include "Headers/Defines.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/TaskPool.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Time/Headers/ApplicationTimer.h"

#include <atomic>

namespace Divide {

bool Divide::TaskPool::USE_OPTICK_PROFILER = false;

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

    Task* job = CreateTask(test, task);
    Start(*job, TaskPriority::DONT_CARE, callback);

    CHECK_FALSE(testValue);

    Wait(*job);

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

    Task* job = CreateTask(test, [&testObj](const Task& parentTask) { testObj.threadedFunction(parentTask); } );
    Start(*job, TaskPriority::DONT_CARE, [&testObj]() { testObj.setTestValue(false); } );

    CHECK_FALSE(testObj.getTestValue());

    Wait(*job);

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
        Task* job = CreateTask(test,
            [](const Task& parentTask) {
            // NOP
        }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i)
        {
            Start(*CreateTask(test, job,
                [](const Task& parentTask) {
                // NOP
            }
            ));
        }

        Wait(Start(*job));
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (blocking): 60K tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HARDWARE_THREAD_COUNT()), TaskPool::TaskPoolType::TYPE_LOCKFREE);
        CHECK_TRUE(init);

        Time::ProfileTimer timer;

        timer.start();
        Task* job = CreateTask(test,
            [](const Task& parentTask) {
            // NOP
        }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i)
        {
            Start(*CreateTask(test, job,
                [](const Task& parentTask) {
                // NOP
            }
            ));
        }

        Wait(Start(*job));
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (lockfree): 60K tasks completed in: " << durationMS << " ms." << std::endl;
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
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - blocking): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
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
            [](const Task & parentTask, U32 start, U32 end) {
                // NOP
            },
            loopCount,
            partitionSize,
            TaskPriority::DONT_CARE,
            false, 
            true);
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - blocking - use current thread): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
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
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - lockfree): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
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
            [](const Task & parentTask, U32 start, U32 end) {
                // NOP
            },
            loopCount,
            partitionSize,
            TaskPriority::DONT_CARE,
            false,
            true);
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "Threading speed test (parallel_for - lockfree - use current thread): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
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

    Task* job = CreateTask(test, testFunction);
    Wait(Start(*job, TaskPriority::DONT_CARE, callback));
    CHECK_EQUAL(callbackValue, 1u);
    test.flushCallbackQueue();
    CHECK_EQUAL(callbackValue, 2u);

    job = CreateTask(test, testFunction);
    Wait(Start(*job));
    CHECK_EQUAL(callbackValue, 3u);
    test.flushCallbackQueue();
    CHECK_EQUAL(callbackValue, 3u);

    job = CreateTask(test, testFunction);
    Wait(Start(*job, TaskPriority::REALTIME, callback));
    CHECK_EQUAL(callbackValue, 5u);
}

}; //namespace Divide
