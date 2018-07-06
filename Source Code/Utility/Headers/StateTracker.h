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

#ifndef UTIL_STATE_TRACKER_H_
#define UTIL_STATE_TRACKER_H_

#include "Platform/DataTypes/Headers/PlatformDefines.h"

namespace Divide {

template <typename T>
class StateTracker {
   public:
    enum class State : U32 {
        SKELETON_RENDERED = 0,
        COUNT
    };

    StateTracker()
    {
    }

    ~StateTracker()
    {
    }

    StateTracker& operator=(const StateTracker& other) {
        for (U32 i = 0; i < to_uint(State::COUNT); ++i) {
            _trackedValues[i] = other._trackedValues[i];
        }
        return *this;
    }

    inline T getTrackedValue(State state) {
        return _trackedValues[to_uint(state)].value;
    }

    inline void setTrackedValue(State state, T value) {
        _trackedValues[to_uint(state)].value = value;
        _trackedValues[to_uint(state)].initialized = true;
    }

    /// Init will not change an already initialized value
    inline void initTrackedValue(State state, const T value) {
        getTrackedValue(state);
        if (!_trackedValues[to_uint(state)].initialized) {
            setTrackedValue(state, value);
        }
    }

   protected:
    struct optionalValue {
        T value;
        bool initialized;

        optionalValue()
            : initialized(false)
        {
        }
    };

   protected:
    std::array<optionalValue, to_const_uint(State::COUNT)> _trackedValues;
};

};  // namespace Divide

#endif
