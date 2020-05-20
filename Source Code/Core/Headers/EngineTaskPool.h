/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _ENGINE_TASK_POOL_H_
#define _ENGINE_TASK_POOL_H_

namespace Divide {

class PlatformContext;

struct Task;
struct ParallelForDescriptor;

/**
* @brief Creates a new Task that runs in a separate thread
* @param threadedFunction The callback function to call in a separate thread = the job to execute
*/
template<class Predicate>
Task* CreateTask(PlatformContext& context, Predicate&& threadedFunction, bool allowedInIdle = true);

/**
* @brief Creates a new Task that runs in a separate thread
* @param context The parent task that would need to wait for our newly created task to complete before finishing
* @param threadedFunction The callback function to call in a separate thread = the job to execute
*/

template<class Predicate>
Task* CreateTask(PlatformContext& context, Task* parentTask, Predicate&& threadedFunction, bool allowedInIdle = true);

void parallel_for(PlatformContext& context, const ParallelForDescriptor& descriptor);

void WaitForAllTasks(PlatformContext& context, bool yield, bool flushCallbacks);

}; //namespace Divide

#endif //_ENGINE_TASK_POOL_H_

#include "EngineTaskPool.inl"