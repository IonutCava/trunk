#include "stdafx.h"

#include "Headers/GFXDevice.h"

#include "Utility/Headers/Localization.h"

#include "Platform/Video/OpenGL/Buffers/RenderTarget/Headers/glFramebuffer.h"
#include "Platform/Video/Direct3D/Buffers/RenderTarget/Headers/d3dRenderTarget.h"

#include "Platform/Video/OpenGL/Headers/glIMPrimitive.h"

#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dVertexBuffer.h"

#include "Platform/Video/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
#include "Platform/Video/Direct3D/Buffers/PixelBuffer/Headers/d3dPixelBuffer.h"

#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Platform/Video/Direct3D/Buffers/VertexBuffer/Headers/d3dGenericVertexData.h"

#include "Platform/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/Direct3D/Textures/Headers/d3dTexture.h"

#include "Platform/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/Direct3D/Shaders/Headers/d3dShaderProgram.h"

#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Platform/Video/Direct3D/Buffers/ShaderBuffer/Headers/d3dConstantBuffer.h"

namespace Divide {

namespace {
    GFXDevice& refThis(const GFXDevice* device) {
        return const_cast<GFXDevice&>(*device);
    }
};

RenderTarget* GFXDevice::newRT(const vec2<U16>& resolution, const stringImpl& name) const {
    std::unique_lock<std::mutex> lk(_gpuObjectArenaMutex);

    RenderTarget* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new framebuffer.
            /// The callee is responsible for it's deletion!
            temp =  new (_gpuObjectArena) glFramebuffer(refThis(this), resolution, name);
        } break;
        case RenderAPI::Direct3D: {
            temp = new (_gpuObjectArena) d3dRenderTarget(refThis(this), resolution, name);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    return temp;
}

IMPrimitive* GFXDevice::newIMP() const {
    std::unique_lock<std::mutex> lk(_gpuObjectArenaMutex);

    IMPrimitive* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new IM emulation primitive. The callee is responsible
            /// for it's deletion!
            temp = new (_gpuObjectArena) glIMPrimitive(refThis(this));
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    return temp;
}

VertexBuffer* GFXDevice::newVB() const {
    std::unique_lock<std::mutex> lk(_gpuObjectArenaMutex);

    VertexBuffer* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new vertex array (VAO + VB + IB).
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glVertexArray(refThis(this));
        } break;
        case RenderAPI::Direct3D: {
            temp = new (_gpuObjectArena) d3dVertexBuffer(refThis(this));
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    return temp;
}

PixelBuffer* GFXDevice::newPB(PBType type) const {
    std::unique_lock<std::mutex> lk(_gpuObjectArenaMutex);

    PixelBuffer* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new pixel buffer using the requested format.
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glPixelBuffer(refThis(this), type);
        } break;
        case RenderAPI::Direct3D: {
            temp = new (_gpuObjectArena) d3dPixelBuffer(refThis(this), type);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    return temp;
}

GenericVertexData* GFXDevice::newGVD(const U32 ringBufferLength) const {
    std::unique_lock<std::mutex> lk(_gpuObjectArenaMutex);

    GenericVertexData* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new generic vertex data object
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glGenericVertexData(refThis(this), ringBufferLength);
        } break;
        case RenderAPI::Direct3D: {
            temp = new (_gpuObjectArena) d3dGenericVertexData(refThis(this), ringBufferLength);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    return temp;
}

Texture* GFXDevice::newTexture(size_t descriptorHash,
                               const stringImpl& name,
                               const stringImpl& resourceName,
                               const stringImpl& resourceLocation,
                               bool isFlipped,
                               bool asyncLoad,
                               const TextureDescriptor& texDescriptor) const {
    // Texture is a resource! Do not use object arena!
    Texture* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new texture. The callee is responsible for it's deletion!
            temp = MemoryManager_NEW glTexture(refThis(this), descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor);
        } break;
        case RenderAPI::Direct3D: {
            temp = MemoryManager_NEW d3dTexture(refThis(this), descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return temp;
}

Pipeline GFXDevice::newPipeline(const PipelineDescriptor& descriptor) const {
    // Hack for now. Cache and lookup later (e.g. for Vulkan/D3D12)
    return Pipeline(descriptor);
}

ShaderProgram* GFXDevice::newShaderProgram(size_t descriptorHash,
                                           const stringImpl& name,
                                           const stringImpl& resourceName,
                                           const stringImpl& resourceLocation,
                                           bool asyncLoad) const {
    // ShaderProgram is a resource! Do not use object arena!
    ShaderProgram* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new shader program.
            /// The callee is responsible for it's deletion!
            temp = MemoryManager_NEW glShaderProgram(refThis(this), descriptorHash, name, resourceName, resourceLocation, asyncLoad);
        } break;
        case RenderAPI::Direct3D: {
            temp = MemoryManager_NEW d3dShaderProgram(refThis(this), descriptorHash, name, resourceName, resourceLocation, asyncLoad);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    return temp;
}

ShaderBuffer* GFXDevice::newSB(const ShaderBufferDescriptor& descriptor) const {
    std::unique_lock<std::mutex> lk(_gpuObjectArenaMutex);

    ShaderBuffer* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new shader buffer. The callee is responsible for it's deletion!
            /// The OpenGL implementation creates either an 'Uniform Buffer Object' if unbound is false
            /// or a 'Shader Storage Block Object' otherwise
            // The shader buffer can also be persistently mapped, if requested
            temp = new (_gpuObjectArena) glUniformBuffer(refThis(this), descriptor);
        } break;
        case RenderAPI::Direct3D: {
            temp = new (_gpuObjectArena) d3dConstantBuffer(refThis(this), descriptor);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    return temp;
}

}; // namespace Divide