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
#ifndef _VK_PLACEHOLDER_OBJECTS_H_
#define _VK_PLACEHOLDER_OBJECTS_H_

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
    class vkRenderTarget final : public RenderTarget {
      public:
        vkRenderTarget(GFXDevice& context, const RenderTargetDescriptor& descriptor)
            : RenderTarget(context, descriptor)
        {}

        bool resize(U16 width, U16 height) override { return true; }
        void setDefaultState(const RTDrawDescriptor& drawPolicy) override {}
        void readData(const vec4<U16>& rect, GFXImageFormat imageFormat, GFXDataFormat dataType, bufferPtr outData) override {}
        void blitFrom(const RTBlitParams& params) override {}
    };

    class vkIMPrimitive final : public IMPrimitive {
    public:
        vkIMPrimitive(GFXDevice& context)
            : IMPrimitive(context)
        {}

        void draw(const GenericDrawCommand& cmd, I32 passIdx = -1) override {}

        void beginBatch(bool reserveBuffers, U32 vertexCount, U32 attributeCount) override {}

        void begin(PrimitiveType type) override {}
        void vertex(F32 x, F32 y, F32 z) override {}

        void attribute1i(U32 attribLocation, I32 value) override {}
        void attribute1f(U32 attribLocation, F32 value) override {}
        void attribute4ub(U32 attribLocation, U8 x, U8 y, U8 z, U8 w) override {}
        void attribute4f(U32 attribLocation, F32 x, F32 y, F32 z, F32 w) override {}

        void end() override {}
        void endBatch() override {}

        GFX::CommandBuffer& toCommandBuffer() const override { return *_cmdBuffer; }

    private:
        GFX::CommandBuffer* _cmdBuffer = nullptr;
    };

    class vkVertexBuffer final : public VertexBuffer {
      public:
        vkVertexBuffer(GFXDevice& context)
            : VertexBuffer(context)
        {}

        void draw(const GenericDrawCommand& command, I32 passIdx = -1) override {}
        bool queueRefresh() override { return refresh(); }

      protected:
        bool refresh() override { return true; }
    };


    class vkPixelBuffer final : public PixelBuffer {
    public:
        vkPixelBuffer(GFXDevice& context, PBType type, const char* name)
            : PixelBuffer(context, type, name)
        {}

        bool create(
            U16 width, U16 height, U16 depth = 0,
            GFXImageFormat formatEnum = GFXImageFormat::RGBA,
            GFXDataFormat dataTypeEnum = GFXDataFormat::FLOAT_32) override
        {
            return true;
        }

        void updatePixels(const F32* const pixels, U32 pixelCount) override {}
    };


    class vkGenericVertexData final : public GenericVertexData {
    public:
        vkGenericVertexData(GFXDevice& context, const U32 ringBufferLength, const char* name)
            : GenericVertexData(context, ringBufferLength, name)
        {}

        void setIndexBuffer(const IndexBuffer& indices, BufferUpdateFrequency updateFrequency) override {}
        void updateIndexBuffer(const IndexBuffer& indices) override {}
        void create(U8 numBuffers = 1) override {}

        void draw(const GenericDrawCommand& command, I32 passIdx = -1) override {}

        void setBuffer(const SetBufferParams& params) override {}

        void updateBuffer(U32 buffer,
            U32 elementCount,
            U32 elementCountOffset,
            const bufferPtr data) override {}

        void setBufferBindOffset(U32 buffer, U32 elementCountOffset) override {}


        void incQueryQueue() {}
    };

    class vkTexture final : public Texture {
    public:
        vkTexture(GFXDevice& context,
                  size_t descriptorHash,
                  const stringImpl& name,
                  const stringImpl& resourceName,
                  const stringImpl& resourceLocation,
                  bool isFlipped,
                  bool asyncLoad,
                  const TextureDescriptor& texDescriptor)
            : Texture(context, descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor)
        {}

        void bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) override {}
        void resize(const bufferPtr ptr, const vec2<U16>& dimensions) override {}
        void loadData(const TextureLoadInfo& info, const vector<ImageTools::ImageLayer>& imageLayers) override {}
        void loadData(const TextureLoadInfo& info, const bufferPtr data, const vec2<U16>& dimensions) override {}
        void copy(const Texture_ptr& other)  override {}
    };

    class vkShaderProgram final : public ShaderProgram {
    public:
        vkShaderProgram(GFXDevice& context, size_t descriptorHash,
                        const stringImpl& name,
                        const stringImpl& resourceName,
                        const stringImpl& resourceLocation,
                        const ShaderProgramDescriptor& descriptor,
                        bool asyncLoad)
            : ShaderProgram(context, descriptorHash, name, resourceName, resourceLocation, descriptor, asyncLoad)
        {}

        bool isValid() const override { return true; }
        U32 GetSubroutineIndex(ShaderType type, const char* name) override { return 0; }
        U32 GetSubroutineUniformCount(ShaderType type) override { return 0;  }

    protected:
        bool recompileInternal() override { return true; }
    };


    class vkUniformBuffer final : public ShaderBuffer {
    public:
        vkUniformBuffer(GFXDevice& context, const ShaderBufferDescriptor& descriptor)
            : ShaderBuffer(context, descriptor)
        {}


        void writeData(ptrdiff_t offsetElementCount, ptrdiff_t rangeElementCount, const bufferPtr data) override {}
        void writeBytes(ptrdiff_t offsetInBytes, ptrdiff_t rangeInBytes, const bufferPtr data) override {}
        void writeData(const bufferPtr data) {}

        void clearData(ptrdiff_t offsetElementCount, ptrdiff_t rangeElementCount) override {}

        void readData(ptrdiff_t offsetElementCount, ptrdiff_t rangeElementCount, bufferPtr result) const override {}

        bool bindRange(U8 bindIndex, U32 offsetElementCount, U32 rangeElementCount) override { return true; }

        bool bind(U8 bindIndex) override { return true; }
    };
};  // namespace Divide
#endif //_VK_PLACEHOLDER_OBJECTS_H_
