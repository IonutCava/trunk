/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _RENDER_TARGET_DRAW_DESCRIPTOR_H_
#define _RENDER_TARGET_DRAW_DESCRIPTOR_H_

#include "RTAttachment.h"

namespace Divide {

class RTDrawMask {
  public:
    RTDrawMask();

    bool isEnabled(RTAttachment::Type type, U8 index) const;
    void setEnabled(RTAttachment::Type type, U8 index, const bool state);

    void enableAll();
    void disableAll();

    bool operator==(const RTDrawMask& other) const;
    bool operator!=(const RTDrawMask& other) const;

  private:
    bool _disabledDepth;
    bool _disabledStencil;
    vectorImpl<U8> _disabledColours;
};

class RTDrawDescriptor {
  public: 
    enum class State : U32 {
        CLEAR_COLOUR_BUFFERS = toBit(1),
        CLEAR_DEPTH_BUFFER = toBit(2),
        CHANGE_VIEWPORT = toBit(3),
        COUNT = 3
    };

  public:
    RTDrawDescriptor();

    void stateMask(U32 stateMask);
    void enableState(State state);
    void disableState(State state);
    bool isEnabledState(State state) const;

    inline RTDrawMask& drawMask() { return _drawMask; }
    inline const RTDrawMask& drawMask() const { return _drawMask; }
    inline U32 stateMask() const { return _stateMask; }

    bool operator==(const RTDrawDescriptor& other) const;
    bool operator!=(const RTDrawDescriptor& other) const;

  protected:
    RTDrawMask _drawMask;
    U32 _stateMask;
};

}; //namespace Divide

#endif //_RENDER_TARGET_DRAW_DESCRIPTOR_H_
