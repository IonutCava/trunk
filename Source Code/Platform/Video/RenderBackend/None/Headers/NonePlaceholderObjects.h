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

#include "Platform/Video/Buffers/PixelBuffer/Headers/PixelBuffer.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RTAttachment.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {
    class noRenderTarget final : public RenderTarget {
      public:
          noRenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor)
            : RenderTarget(context, descriptor)
        {}

        void clear(const RTClearDescriptor& descriptor) override {}
        void setDefaultState(const RTDrawDescriptor& drawPolicy) override {}
        void readData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) const override {}
        void blitFrom(const RTBlitParams& params) override {}
    };

    class noIMPrimitive final : public IMPrimitive {
    public:
        noIMPrimitive(GFXDevice& context)
            : IMPrimitive(context)
        {}

        void draw(const GenericDrawCommand& cmd, U32 cmdBufferOffset) override {}

        void beginBatch(bool reserveBuffers, U32 vertexCount, U32 attributeCount) override {}

        void begin(PrimitiveType type) override {}
        void vertex(F32 x, F32 y, F32 z) override {}

        void attribute1i(U32 attribLocation, I32 value) override {}
        void attribute1f(U32 attribLocation, F32 value) override {}
        void attribute4ub(U32 attribLocation, U8 x, U8 y, U8 z, U8 w) override {}
        void attribute4f(U32 attribLocation, F32 x, F32 y, F32 z, F32 w) override {}

        void end() override {}
        void endBatch() override {}
        void clearBatch() override {}
        bool hasBatch() const override { return false; }

        GFX::CommandBuffer& toCommandBuffer() const override { return *_cmdBuffer; }
    };

    class noVertexBuffer final : public VertexBuffer {
      public:
          noVertexBuffer(GFXDevice& context)
            : VertexBuffer(context)
        {}

        void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) override {}
        bool queueRefresh() override { return refresh(); }

      protected:
        bool refresh() override { return true; }
    };


    class noPixelBuffer final : public PixelBuffer {
    public:
        noPixelBuffer(GFXDevice& context, const PBType type, const char* name)
            : PixelBuffer(context, type, name)
        {}

        bool create(
            U16 width, U16 height, U16 depth = 0,
            GFXImageFormat formatEnum = GFXImageFormat::RGBA,
            GFXDataFormat dataTypeEnum = GFXDataFormat::FLOAT_32,
            bool normalized = true) override
        {
            return true;
        }

        void updatePixels(const F32* pixels, U32 pixelCount) override {}
    };


    class noGenericVertexData final : public GenericVertexData {
    public:
        noGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
            : GenericVertexData(context, ringBufferLength, name)
        {}

        void create(U8 numBuffers = 1) override {}

        void draw(const GenericDrawCommand& command, U32 cmdBufferOffset) override {}

        void setBuffer(const SetBufferParams& params) override {}

        void updateBuffer(U32 buffer,
            U32 elementCount,
            U32 elementCountOffset,
            bufferPtr data) override {}

        void setBufferBindOffset(U32 buffer, U32 elementCountOffset) override {}

        void incQueryQueue() {}
    };

    class noTexture final : public Texture {
    public:
        noTexture(GFXDevice& context,
                  const size_t descriptorHash,
                  const Str256& name,
                  const ResourcePath& assetNames,
                  const ResourcePath& assetLocations,
                  const bool isFlipped,
                  const bool asyncLoad,
                  const TextureDescriptor& texDescriptor)
            : Texture(context, descriptorHash, name, assetNames, assetLocations, isFlipped, asyncLoad, texDescriptor)
        {}

        [[nodiscard]] size_t makeResident(size_t samplerHash) override { return 0; }
        void bindLayer(U8 slot, U8 level, U8 layer, bool layered, Image::Flag rw_flag) override {}
        void resize(const std::pair<Byte*, size_t>& ptr, const vec2<U16>& dimensions) override {}
        void loadData(const ImageTools::ImageData& imageData) override {}
        void loadData(const std::pair<Byte*, size_t>& data, const vec2<U16>& dimensions) override {}
        std::pair<std::shared_ptr<Byte[]>, size_t> readData(U16 mipLevel, GFXDataFormat desiredFormat) const override { ACKNOWLEDGE_UNUSED(mipLevel); ACKNOWLEDGE_UNUSED(desiredFormat); return { nullptr, 0 }; }
    };

    class noShaderProgram final : public ShaderProgram {
    public:
        noShaderProgram(GFXDevice& context, const size_t descriptorHash,
                        const Str256& name,
                        const Str256& assetName,
                        const ResourcePath& assetLocation,
                        const ShaderProgramDescriptor& descriptor,
                        const bool asyncLoad)
            : ShaderProgram(context, descriptorHash, name, assetName, assetLocation, descriptor, asyncLoad)
        {}

        bool isValid() const override { return true; }
    };

    class noUniformBuffer final : public ShaderBuffer {
    public:
        noUniformBuffer(GFXDevice& context, const ShaderBufferDescriptor& descriptor)
            : ShaderBuffer(context, descriptor)
        {}


        void writeData(U32 offsetElementCount, U32 rangeElementCount, bufferPtr data) override {}
        void writeBytes(ptrdiff_t offsetInBytes, ptrdiff_t rangeInBytes, bufferPtr data) override {}
        void writeData(bufferPtr data) override {}

        void clearData(U32 offsetElementCount, U32 rangeElementCount) override {}
        void readData(U32 offsetElementCount, U32 rangeElementCount, bufferPtr result) const override {}

        bool bindRange(U8 bindIndex, U32 offsetElementCount, U32 rangeElementCount) override { return true; }

        bool bind(U8 bindIndex) override { return true; }
    };
};  // namespace Divide
#endif //_NONE_PLACEHOLDER_OBJECTS_H_
