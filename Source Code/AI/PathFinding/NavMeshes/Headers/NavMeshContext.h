/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */
#ifndef _NAV_MESH_CONTEXT_H_
#define _NAV_MESH_CONTEXT_H_

#include "NavMeshDefines.h"

namespace AI {
namespace Navigation {
    class rcContextDivide : public rcContext{
    public:
        rcContextDivide(bool state) : rcContext(state) {resetTimers();}
        ~rcContextDivide() {}

    private:
        /// Logs a message.
        ///  @param[in]		category	The category of the message.
        ///  @param[in]		msg			The formatted message.
        ///  @param[in]		len			The length of the formatted message.
        void doLog(const rcLogCategory /*category*/, const char* /*msg*/, const I32 /*len*/);
        void doResetTimers();
        void doStartTimer(const rcTimerLabel /*label*/);
        void doStopTimer(const rcTimerLabel /*label*/);
        I32 doGetAccumulatedTime(const rcTimerLabel /*label*/) const;

    private:
        D32 _startTime[RC_MAX_TIMERS];
        I32 _accTime[RC_MAX_TIMERS];
    };
};

}; //namespace AI

#endif