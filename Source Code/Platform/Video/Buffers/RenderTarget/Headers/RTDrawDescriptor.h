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
#include "Platform/Video/Headers/BlendingProperties.h"

namespace Divide {

// 4 should be more than enough even for batching multiple render targets together
constexpr U8 MAX_RT_COLOUR_ATTACHMENTS = 4;

class RTDrawMask {
  public:
    RTDrawMask();

    bool isEnabled(RTAttachmentType type, U8 index) const;
    void setEnabled(RTAttachmentType type, U8 index, const bool state);

    void enableAll();
    void disableAll();

    bool operator==(const RTDrawMask& other) const;
    bool operator!=(const RTDrawMask& other) const;

  private:
    bool _disabledDepth;
    bool _disabledStencil;
    std::array<bool, MAX_RT_COLOUR_ATTACHMENTS> _disabledColours;
};

struct RTBlendState {
    bool _blendEnable = false;
    vec4<U8> _blendColour = vec4<U8>(0);
    BlendingProperties _blendProperties;

    bool operator==(const RTBlendState& other) const;
    bool operator!=(const RTBlendState& other) const;
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

    inline RTBlendState& blendState(U8 index) { return _blendStates[index]; }
    inline const RTBlendState& blendState(U8 index) const{ return _blendStates[index]; }

    bool operator==(const RTDrawDescriptor& other) const;
    bool operator!=(const RTDrawDescriptor& other) const;

  protected:
    RTDrawMask _drawMask;
    U32 _stateMask;
    std::array<RTBlendState, MAX_RT_COLOUR_ATTACHMENTS> _blendStates;
};

}; //namespace Divide

#endif //_RENDER_TARGET_DRAW_DESCRIPTOR_H_
