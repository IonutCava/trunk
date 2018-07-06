#include "stdafx.h"

#include "Headers/Defines.h"
#include "Core/Headers/TaskPool.h"

#include <atomic>

namespace Divide {

namespace {
    const U32 g_TestTaskPoolSize = 4;
};

TEST(TaskPoolContructionTest)
{
    for (U8 i = 0; i < to_U8(TaskPool::TaskPoolType::COUNT); ++i) {
        TaskPool test(g_TestTaskPoolSize);
        bool init = test.init(0, static_cast<TaskPool::TaskPoolType>(i));
        CHECK_FALSE(init);

        init = test.init(1, static_cast<TaskPool::TaskPoolType>(i));
        CHECK_FALSE(init);
        init = test.init(HARDWARE_THREAD_COUNT(), static_cast<TaskPool::TaskPoolType>(i));
        CHECK_TRUE(init);
    }
}

TEST(ParallelForTest)
{
    for (U8 i = 0; i < to_U8(TaskPool::TaskPoolType::COUNT); ++i) {
        TaskPool test(g_TestTaskPoolSize * 4);
        bool init = test.init(HARDWARE_THREAD_COUNT(), static_cast<TaskPool::TaskPoolType>(i));
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
}


TEST(TaskCallbackTest)
{
    for (U8 i = 0; i < to_U8(TaskPool::TaskPoolType::COUNT); ++i) {
        TaskPool test(g_TestTaskPoolSize);
        bool init = test.init(HARDWARE_THREAD_COUNT(), static_cast<TaskPool::TaskPoolType>(i));
        CHECK_TRUE(init);

        std::atomic_bool testValue = false;

        auto task = [](const Task& parentTask) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        };

        auto callback = [&testValue]() {
            testValue = true;
        };

        TaskHandle job = CreateTask(test, task, callback);
        job.startTask();

        CHECK_FALSE(testValue);

        job.wait();

        CHECK_FALSE(testValue);

        test.flushCallbackQueue();

        CHECK_TRUE(testValue);
    }
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
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      private:
        std::atomic_bool _testValue;
    };
};

TEST(TaskClassMemberCallbackTest)
{
    for (U8 i = 0; i < to_U8(TaskPool::TaskPoolType::COUNT); ++i) {
        TaskPool test(g_TestTaskPoolSize);
        bool init = test.init(HARDWARE_THREAD_COUNT(), static_cast<TaskPool::TaskPoolType>(i));
        CHECK_TRUE(init);

        ThreadedTest testObj;

        TaskHandle job = CreateTask(test,
            DELEGATE_BIND(&ThreadedTest::threadedFunction, &testObj, std::placeholders::_1),
            DELEGATE_BIND(&ThreadedTest::setTestValue, &testObj, true));
        job.startTask();

        CHECK_FALSE(testObj.getTestValue());

        job.wait();

        CHECK_FALSE(testObj.getTestValue());

        test.flushCallbackQueue();

        CHECK_TRUE(testObj.getTestValue());
    }
}

TEST(TaskPriorityTest)
{
    for (U8 i = 0; i < to_U8(TaskPool::TaskPoolType::COUNT); ++i) {
        TaskPool test(g_TestTaskPoolSize);
        bool init = test.init(HARDWARE_THREAD_COUNT(), static_cast<TaskPool::TaskPoolType>(i));
        CHECK_TRUE(init);

        U32 callbackValue = 0;
        auto testFunction = [&callbackValue](const Task& parentTask) {
            callbackValue++;
        };

        auto callback = [&callbackValue]() {
            callbackValue++;
        };

        TaskHandle job = CreateTask(test, testFunction, callback);
        job.startTask(Task::TaskPriority::MAX);
        job.wait();
        CHECK_EQUAL(callbackValue, 1u);
        test.flushCallbackQueue();
        CHECK_EQUAL(callbackValue, 2u);

        job.startTask(Task::TaskPriority::REALTIME);
        job.wait();
        CHECK_EQUAL(callbackValue, 3u);
        test.flushCallbackQueue();
        CHECK_EQUAL(callbackValue, 3u);
        test.setTaskCallback(job, callback);
        job.startTask(Task::TaskPriority::REALTIME_WITH_CALLBACK);
        job.wait();
        CHECK_EQUAL(callbackValue, 5u);
        test.flushCallbackQueue();
        CHECK_EQUAL(callbackValue, 5u);
    }
}

}; //namespace Divide
