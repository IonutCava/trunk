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

    bool isEnabled(RTAttachmentType type) const;
    bool isEnabled(RTAttachmentType type, U8 index) const;
    void setEnabled(RTAttachmentType type, U8 index, const bool state);

    void enableAll();
    void disableAll();

    inline bool operator==(const RTDrawMask& other) const;
    inline bool operator!=(const RTDrawMask& other) const;

  private:
    std::array<bool, MAX_RT_COLOUR_ATTACHMENTS> _disabledColours;
    bool _disabledDepth;
    bool _disabledStencil;
};

struct RTBlendState {
    UColour4 _blendColour = {0u, 0u, 0u, 0u};
    BlendingProperties _blendProperties;

    inline bool operator==(const RTBlendState& other) const;
    inline bool operator!=(const RTBlendState& other) const;
};

class RTClearDescriptor {
public:
    RTClearDescriptor();

    inline void clearColour(U8 index, const bool state) noexcept { _clearColourAttachment[index] = state; }
    inline bool clearColour(U8 index) const noexcept { return _clearColourAttachment[index]; }

    PROPERTY_RW(bool, clearDepth, true);
    PROPERTY_RW(bool, clearColours, true);
    PROPERTY_RW(bool, clearExternalColour, false);
    PROPERTY_RW(bool, clearExternalDepth, false);
    PROPERTY_RW(bool, resetToDefault, true);

protected:
    std::array<bool, MAX_RT_COLOUR_ATTACHMENTS> _clearColourAttachment;
};

class RTDrawDescriptor {
  public:
    RTDrawDescriptor();

    inline RTDrawMask& drawMask() noexcept { return _drawMask; }
    inline const RTDrawMask& drawMask() const noexcept { return _drawMask; }

    inline RTBlendState& blendState(U8 index) noexcept { return _blendStates[index]; }
    inline const RTBlendState& blendState(U8 index) const noexcept { return _blendStates[index]; }

    inline bool operator==(const RTDrawDescriptor& other) const;
    inline bool operator!=(const RTDrawDescriptor& other) const;

    PROPERTY_RW(bool, setViewport, true);

  protected:
    RTDrawMask _drawMask;
    std::array<RTBlendState, MAX_RT_COLOUR_ATTACHMENTS> _blendStates;
};

}; //namespace Divide

#endif //_RENDER_TARGET_DRAW_DESCRIPTOR_H_


#include "RTDrawDescriptor.inl"