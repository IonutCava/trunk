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

/*Code references:
    http://www.cplusplus.com/forum/lounge/27570/
*/
#ifndef _GUID_WRAPPER_H_
#define _GUID_WRAPPER_H_
#include "Hardware/Platform/Headers/PlatformDefines.h"

namespace Divide {

///Utility class that adds basic GUID management to objects
class GUIDWrapper {
public:

    GUIDWrapper() : _GUID(_idGenerator++)
    {
    }

    virtual ~GUIDWrapper()
    {
        _idGenerator--;
    }

    GUIDWrapper(const GUIDWrapper& old) : _GUID(_idGenerator++)
    {
    }

    inline I64  getGUID()   const  {return _GUID;}

private:
    void operator=(const GUIDWrapper& old)
    {
    }

protected:
    const  I64 _GUID;

private:
    static I64 _idGenerator;
};

}; //namespace Divide

#endif