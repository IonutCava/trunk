/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _TASKS_H_
#define _TASKS_H_

#include "SharedMutex.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

enum class CallbackParam : U32 {
    TYPE_INTEGER = 0,
    TYPE_MEDIUM_INTEGER,
    TYPE_SMALL_INTEGER,
    TYPE_UNSIGNED_INTEGER,
    TYPE_MEDIUM_UNSIGNED_INTEGER,
    TYPE_SMALL_UNSIGNED_INTEGER,
    TYPE_STRING,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR
};
/**
 *@brief Using std::atomic for thread-shared data to avoid locking
 */
class Task : public GUIDWrapper, public std::enable_shared_from_this<Task> {
   public:
       enum class TaskPriority : U32 {
           DONT_CARE = 0,
           LOW = 1,
           MEDIUM = 2,
           HIGH = 3,
           MAX = 4
       };
    /**
     * @brief Creates a new Task that runs in a separate thread
     * @param tickInterval The delay (in microseconds) between each callback
     * @param numberOfTicks The number of times to call the callback function
     * before the Task is deleted. 0 = run forever
     * @param f The callback function
     */
    Task(ThreadPool& tp, U64 tickInterval, I32 numberOfTicks,
         const DELEGATE_CBK<>& f);
    ~Task();

    // Minimum interval is 1 millisecond!
    void updateTickInterval(U64 tickInterval);

    void updateTickCounter(I32 numberOfTicks);
    void startTask(TaskPriority priority = TaskPriority::DONT_CARE);
    void stopTask();
    void pauseTask(bool state);

    inline bool isFinished() const { return _done; }

    inline void onCompletionCbk(const DELEGATE_CBK_PARAM<I64>& cbk) {
        _onCompletionCbk = cbk;
    }

    // Always is microseconds
    static void update(const U64 deltaTime) {
        _currentTime += deltaTime;
    }

   private:
    mutable SharedLock _lock;
    mutable std::atomic<U64> _tickInterval;
    mutable std::atomic_int _numberOfTicks;
    mutable std::atomic_bool _end;
    mutable std::atomic_bool _paused;
    mutable std::atomic_bool _done;

    DELEGATE_CBK<> _callback;
    DELEGATE_CBK_PARAM<I64> _onCompletionCbk;
    ThreadPool& _tp;

    // Always is microseconds
    static U64 _currentTime;
   protected:
    void run();
};

typedef std::shared_ptr<Task> Task_ptr;
typedef std::weak_ptr<Task> Task_weak_ptr;

};  // namespace Divide

#endif
