#include "stdafx.h"

#include "Headers/Defines.h"
#include "Core/Time/Headers/ProfileTimer.h"

#include <atomic>

namespace Divide {

bool TaskPool::USE_OPTICK_PROFILER = false;

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
    init = test.init(HardwareThreadCount(), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_FALSE(init);
}

TEST(ParallelForTest)
{
    Console::toggleErrorStream(false);

    TaskPool test;
    bool init = test.init(HardwareThreadCount(), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    const U32 partitionSize = 4;
    const U32 loopCount = partitionSize * 4 + 2;

    std::atomic_uint loopCounter = 0;
    std::atomic_uint totalCounter = 0;

    ParallelForDescriptor descriptor = {};
    descriptor._iterCount = loopCount;
    descriptor._partitionSize = partitionSize;
    descriptor._cbk = [&totalCounter, &loopCounter](const Task* parentTask, U32 start, U32 end) {
        ++loopCounter;
        for (U32 i = start; i < end; ++i) {
            ++totalCounter;
        }
    };

    parallel_for(test, descriptor);

    CHECK_EQUAL(loopCounter, 5u);
    CHECK_EQUAL(totalCounter, 18u);
}


TEST(TaskCallbackTest)
{
    TaskPool test;
    bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    std::atomic_bool testValue = false;

    Task* job = CreateTask(test, [](const Task& parentTask) {
        Time::ProfileTimer timer;
        timer.start();
        std::cout << "TaskCallbackTest: Thread sleeping for 500ms" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
        std::cout << "TaskCallbackTest: Thread waking up (" << durationMS << "ms )" << std::endl;
    });

    Start(*job, TaskPriority::DONT_CARE, [&testValue]() {
        std::cout << "TaskCallbackTest: Callback called!" << std::endl;
        testValue = true;
    });

    CHECK_FALSE(testValue);
    std::cout << "TaskCallbackTest: waiting for task!" << std::endl;
    Wait(*job);

    CHECK_FALSE(testValue);

    std::cout << "TaskCallbackTest: flushing queue!" << std::endl;
    test.flushCallbackQueue();

    CHECK_TRUE(testValue);
}

namespace {
    class ThreadedTest {
      public:
        ThreadedTest() noexcept
        {
            std::atomic_init(&_testValue, false);
        }

        void setTestValue(const bool state) noexcept {
            std::cout << "ThreadedTest: Setting value to - " << state << std::endl;
            _testValue = state;
        }

        bool getTestValue() const noexcept{
            return _testValue;
        }

        void threadedFunction(const Task& parentTask) {
            ACKNOWLEDGE_UNUSED(parentTask);
            Time::ProfileTimer timer;
            timer.start();
            std::cout << "ThreadedTest: Thread sleeping for 200ms" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            timer.stop();
            const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - Time::ProfileTimer::overhead());
            std::cout << "ThreadedTest: Thread waking up (" << durationMS << "ms )" << std::endl;
            setTestValue(true);
        }

      private:
        std::atomic_bool _testValue;
    };
};

TEST(TaskClassMemberCallbackTest)
{
    TaskPool test;
    bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    ThreadedTest testObj;

    Task* job = CreateTask(test, [&testObj](const Task& parentTask) {
        std::cout << "TaskClassMemberCallbackTest: Threaded called!" << std::endl;
        testObj.threadedFunction(parentTask);
    });

    Start(*job, TaskPriority::DONT_CARE, [&testObj]() {
        std::cout << "TaskClassMemberCallbackTest: Callback called!" << std::endl;
        testObj.setTestValue(false);
    });

    CHECK_FALSE(testObj.getTestValue());

    std::cout << "TaskClassMemberCallbackTest: Waiting for task!" << std::endl;
    Wait(*job);

    CHECK_TRUE(testObj.getTestValue());

    std::cout << "TaskClassMemberCallbackTest: Flushing callback queue!" << std::endl;
    test.flushCallbackQueue();

    CHECK_FALSE(testObj.getTestValue());
}

TEST(TaskSpeedTest)
{
    const U64 timerOverhead = Time::ProfileTimer::overhead();

    {
        TaskPool test;
        bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_BLOCKING);
        CHECK_TRUE(init);

        Time::ProfileTimer timer;

        timer.start();
        Task* job = CreateTask(test,
            [](const Task& parentTask) {
                NOP();
            }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i) {
            Start(*CreateTask(test, job,
                [](const Task& parentTask) {
                    NOP();
                }
            ));
        }

        Wait(Start(*job));
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - timerOverhead);
        std::cout << "Threading speed test (blocking): 60K tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_LOCKFREE);
        CHECK_TRUE(init);

        Time::ProfileTimer timer;

        timer.start();
        Task* job = CreateTask(test,
            [](const Task& parentTask) {
                NOP();
            }
        );

        for (std::size_t i = 0; i < 60 * 1000; ++i) {
            Start(*CreateTask(test, job,
                [](const Task& parentTask) {
                    NOP();
                }
            ));
        }

        Wait(Start(*job));
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - timerOverhead);
        std::cout << "Threading speed test (lockfree): 60K tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_BLOCKING);
        CHECK_TRUE(init);

        constexpr U32 partitionSize = 256;
        constexpr U32 loopCount = partitionSize * 8192 + 2;

        Time::ProfileTimer timer;
        timer.start();

        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = loopCount;
        descriptor._partitionSize = partitionSize;
        descriptor._useCurrentThread = false;
        descriptor._cbk = [](const Task* parentTask, U32 start, U32 end) {
            NOP();
        };

        parallel_for(test, descriptor);
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - timerOverhead);
        std::cout << "Threading speed test (parallel_for - blocking): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_BLOCKING);
        CHECK_TRUE(init);

        constexpr U32 partitionSize = 256;
        constexpr U32 loopCount = partitionSize * 8192 + 2;

        Time::ProfileTimer timer;
        timer.start();

        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = loopCount;
        descriptor._partitionSize = partitionSize;
        descriptor._useCurrentThread = true;
        descriptor._cbk = [](const Task* parentTask, U32 start, U32 end) {
            NOP();
        };

        parallel_for(test, descriptor);

        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - timerOverhead);
        std::cout << "Threading speed test (parallel_for - blocking - use current thread): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }
    {
        TaskPool test;
        bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_LOCKFREE);
        CHECK_TRUE(init);

        constexpr U32 partitionSize = 256;
        constexpr U32 loopCount = partitionSize * 8192 + 2;

        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = loopCount;
        descriptor._partitionSize = partitionSize;
        descriptor._useCurrentThread = false;
        descriptor._cbk = [](const Task* parentTask, U32 start, U32 end) {
            NOP();
        };

        Time::ProfileTimer timer;
        timer.start();
        parallel_for(test, descriptor);
        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - timerOverhead);
        std::cout << "Threading speed test (parallel_for - lockfree): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }   
    {
        TaskPool test;
        bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_LOCKFREE);
        CHECK_TRUE(init);

        constexpr U32 partitionSize = 256;
        constexpr U32 loopCount = partitionSize * 8192 + 2;

        Time::ProfileTimer timer;
        timer.start();

        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = loopCount;
        descriptor._partitionSize = partitionSize;
        descriptor._useCurrentThread = true;
        descriptor._cbk = [](const Task* parentTask, U32 start, U32 end) {
            NOP();
        };

        parallel_for(test, descriptor);

        timer.stop();
        const F32 durationMS = Time::MicrosecondsToMilliseconds<F32>(timer.get() - timerOverhead);
        std::cout << "Threading speed test (parallel_for - lockfree - use current thread): 8192 + 1 partitions tasks completed in: " << durationMS << " ms." << std::endl;
    }
}

TEST(TaskPriorityTest)
{
    TaskPool test;
    const bool init = test.init(to_U8(HardwareThreadCount()), TaskPool::TaskPoolType::TYPE_BLOCKING);
    CHECK_TRUE(init);

    U32 callbackValue = 0;

    Task* job = CreateTask(test, [&callbackValue](const Task& parentTask) {
        ACKNOWLEDGE_UNUSED(parentTask);
        std::cout << "TaskPriorityTest: threaded function call (DONT_CARE)" << std::endl;
        ++callbackValue;
    });

    Wait(Start(*job, TaskPriority::DONT_CARE, [&callbackValue]() {
        std::cout << "TaskPriorityTest: threaded callback (DONT_CARE)" << std::endl;
        ++callbackValue;
    }));
    CHECK_EQUAL(callbackValue, 1u);

    std::cout << "TaskPriorityTest: flushing queue" << std::endl;
    test.flushCallbackQueue();
    CHECK_EQUAL(callbackValue, 2u);

    job = CreateTask(test, [&callbackValue](const Task& parentTask) {
        std::cout << "TaskPriorityTest: threaded function call (NO_CALLBACK)" << std::endl;
        ++callbackValue;
    });
    Wait(Start(*job));
    CHECK_EQUAL(callbackValue, 3u);

    std::cout << "TaskPriorityTest: flushing queue" << std::endl;
    test.flushCallbackQueue();
    CHECK_EQUAL(callbackValue, 3u);

    job = CreateTask(test, [&callbackValue](const Task& parentTask) {
        std::cout << "TaskPriorityTest: threaded function call (REALTIME)" << std::endl;
        ++callbackValue;
    });
    Wait(Start(*job, TaskPriority::REALTIME, [&callbackValue]() {
        std::cout << "TaskPriorityTest: threaded callback (REALTIME)" << std::endl;
        ++callbackValue;
    }));
    CHECK_EQUAL(callbackValue, 5u);
}

}; //namespace Divide
