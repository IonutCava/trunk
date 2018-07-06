/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef UTIL_STATE_TRACKER_H_
#define UTIL_STATE_TRACKER_H_

#include "Vector.h"
#include "Platform/DataTypes/Headers/PlatformDefines.h"

namespace Divide {

template<typename T>
class StateTracker {
public:
    StateTracker() 
    {
        _trackedValues.reserve(16);
    }

    ~StateTracker()
    {
        _trackedValues.clear();
    }

    StateTracker& operator=(const StateTracker& other){
        _trackedValues.clear();
        for (const optionalValue& val: other._trackedValues){
            _trackedValues.push_back(val);
        }
        return *this;
    }

    inline T getTrackedValue(U32 index) { 
        while(index >= _trackedValues.size()){
            _trackedValues.push_back(optionalValue());
        }
        return _trackedValues[index].value;  
    }

    inline void setTrackedValue(U32 index, T value)       { 
        _trackedValues[index].value = value; 
        _trackedValues[index].initialized = true;
    }

    /// Init will not change an already initialized value
    inline void initTrackedValue(U32 index, const T value) {
        getTrackedValue(index);
        if(!_trackedValues[index].initialized)
            setTrackedValue(index, value);
    }

protected:
    struct optionalValue{
        T value;
        bool initialized;
        optionalValue() : initialized(false) 
        {
        }
    };

protected:
    vectorImpl<optionalValue > _trackedValues;

};

}; //namespace Divide

#endif