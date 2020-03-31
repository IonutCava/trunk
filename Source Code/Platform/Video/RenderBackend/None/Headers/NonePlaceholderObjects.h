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
#ifndef _NONE_PLACEHOLDER_OBJECTS_H_
#define _NONE_PLACEHOLDER_OBJECTS_H_

#include "config.h"

#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RTAttachment.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

namespace Divide {
    class noRenderTarget final : public RenderTarget {
      public:
          noRenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor)
            : RenderTarget(context, descriptor)
        {}

        bool resize(U16 width, U16 height) final { return true; }
        void clear(const RTClearDescriptor& descriptor) final {}
        void setDefaultState(const RTDrawDescriptor& drawPolicy) final {}
        void readData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) final {}
        void blitFrom(const RTBlitParams& params) final {}
    };

    class noIMPrimitive final : public IMPrimitive {
    public:
        noIMPrimitive(GFXDevice& context)
            : IMPrimitive(context)
        {}

        void draw(const GenericDrawCommand& cmd, U32 cmdBufferOffset) final {}

        void beginBatch(bool reserveBuffers, U32 vertexCount, U32 attributeCount) final {}

        void begin(PrimitiveType type) final {}
        void vertex(F32 x, F32 y, F32 z) final {}

        void attribute1i(U32 attribLocation, I32 value) final {}
        void attribute1f(U32 attribLocation, F32 value) final {}
        void attribute4ub(U32 attribLocation, U8 x, U8 y, U8 z, U8 w) final {}
        void attribute4f(U32 attribLocation, F32 x, F32 y, F32 z, F32 w) final {}

        void end() final {}
        void endBatch() final {}
        void clearBatch() final {}
        bool hasBatch() const final { return false; }

        GFX::CommandBuffer& toCommandBuffer() const final { return *_cmdBuffer; }

    private:
        GFX::CommandBuffer* _cmdBuffer = nullptr;
    };

    class noVertexBuffer final : public VertexBuffer {
      public:
          noVertexBuffer(GFXDevice& context)
            : VertexBuffer(context)
        {}

        void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) final {}
        bool queueRefresh() final { return refresh(); }

      protected:
        bool refresh() final { return true; }
    };


    class noPixelBuffer final : public PixelBuffer {
    public:
        noPixelBuffer(GFXDevice& context, PBType type, const char* name)
            : PixelBuffer(context, type, name)
        {}

        bool create(
            U16 width, U16 height, U16 depth = 0,
            GFXImageFormat formatEnum = GFXImageFormat::RGBA,
            GFXDataFormat dataTypeEnum = GFXDataFormat::FLOAT_32) final
        {
            return true;
        }

        void updatePixels(const F32* const pixels, U32 pixelCount) final {}
    };


    class noGenericVertexData final : public GenericVertexData {
    public:
        noGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
            : GenericVertexData(context, ringBufferLength, name)
        {}

        void create(U8 numBuffers = 1) final {}

        void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) final {}

        void setBuffer(const SetBufferParams& params) final {}

        void updateBuffer(U32 buffer,
            U32 elementCount,
            U32 elementCountOffset,
            const bufferPtr data) final {}

        void setBufferBindOffset(U32 buffer, U32 elementCountOffset) final {}

        void incQueryQueue() {}
    };

    class noTexture final : public Texture {
    public:
        noTexture(GFXDevice& context,
                  size_t descriptorHash,
                  const Str128& name,
                  const stringImpl& assetNames,
                  const stringImpl& assetLocations,
                  bool isFlipped,
                  bool asyncLoad,
                  const TextureDescriptor& texDescriptor)
            : Texture(context, descriptorHash, name, assetNames, assetLocations, isFlipped, asyncLoad, texDescriptor)
        {}

        void bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) final {}
        void resize(const bufferPtr ptr, const vec2<U16>& dimensions) final {}
        void loadData(const TextureLoadInfo& info, const std::vector<ImageTools::ImageLayer>& imageLayers) final {}
        void loadData(const TextureLoadInfo& info, const bufferPtr data, const vec2<U16>& dimensions) final {}
    };

    class noShaderProgram final : public ShaderProgram {
    public:
        noShaderProgram(GFXDevice& context, size_t descriptorHash,
                        const Str128& name,
                        const Str128& assetName,
                        const stringImpl& assetLocation,
                        const ShaderProgramDescriptor& descriptor,
                        bool asyncLoad)
            : ShaderProgram(context, descriptorHash, name, assetName, assetLocation, descriptor, asyncLoad)
        {}

        bool isValid() const final { return true; }

        U32 GetSubroutineIndex(ShaderType type, const char* name) final { return 0; }
        U32 GetSubroutineUniformCount(ShaderType type) final { return 0;  }

        void update(const U64 deltaTimeUS) final { ACKNOWLEDGE_UNUSED(deltaTimeUS); }
    protected:
        bool recompileInternal(bool force) final { ACKNOWLEDGE_UNUSED(force);  return true; }
    };


    class noUniformBuffer final : public ShaderBuffer {
    public:
        noUniformBuffer(GFXDevice& context, const ShaderBufferDescriptor& descriptor)
            : ShaderBuffer(context, descriptor)
        {}


        void writeData(U32 offsetElementCount, U32 rangeElementCount, const bufferPtr data) final {}
        void writeBytes(ptrdiff_t offsetInBytes, ptrdiff_t rangeInBytes, const bufferPtr data) final {}
        void writeData(const bufferPtr data) final {}

        void clearData(U32 offsetElementCount, U32 rangeElementCount) final {}
        void readData(U32 offsetElementCount, U32 rangeElementCount, bufferPtr result) const final {}

        bool bindRange(U8 bindIndex, U32 offsetElementCount, U32 rangeElementCount) final { return true; }

        bool bind(U8 bindIndex) final { return true; }
    };
};  // namespace Divide
#endif //_NONE_PLACEHOLDER_OBJECTS_H_
