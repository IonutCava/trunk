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

#ifndef _D3D_FRAME_BUFFER_OBJECT_H_
#define _D3D_FRAME_BUFFER_OBJECT_H_

#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {
    
class d3dRenderTarget : public RenderTarget {
    USE_CUSTOM_ALLOCATOR
   public:
    explicit d3dRenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor);
    ~d3dRenderTarget();
    
    bool resize(U16 width, U16 height) override;

    void drawToLayer(RTAttachmentType type,
                     U8 index,
                     U16 layer,
                     bool includeDepth = true) override;
    void setMipLevel(U16 writeLevel) override;

    void bind(U8 unit,
              RTAttachmentType type,
              U8 index) override;

    void readData(const vec4<U16>& rect,
                  GFXImageFormat imageFormat,
                  GFXDataFormat dataType,
                  bufferPtr outData) override;

    void blitFrom(RenderTarget* inputFB,
                  bool blitColour = true,
                  bool blitDepth = false) override;

    void blitFrom(RenderTarget* inputFB,
                  U8 index,
                  bool blitColour = true,
                  bool blitDepth = false) override;
};

};  // namespace Divide
#endif