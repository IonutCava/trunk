#include "stdafx.h"

#include "Headers/GFXDevice.h"

#include "Utility/Headers/Localization.h"

#include "Platform/Video/RenderBackend/Vulkan/Headers/VKWrapper.h"
#include "Platform/Video/RenderBackend/None/Headers/NoneWrapper.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glIMPrimitive.h"
#include "Platform/Video/RenderBackend/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/RenderBackend/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/RenderTarget/Headers/glFramebuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"


namespace Divide {

namespace {
    GFXDevice& refThis(const GFXDevice* device) {
        return const_cast<GFXDevice&>(*device);
    }
};

RenderTarget* GFXDevice::newRT(const RenderTargetDescriptor& descriptor) const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    RenderTarget* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new framebuffer.
            /// The callee is responsible for it's deletion!
            temp =  new (_gpuObjectArena) glFramebuffer(refThis(this), nullptr, descriptor);
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkRenderTarget(refThis(this), descriptor);
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noRenderTarget(refThis(this), descriptor);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

IMPrimitive* GFXDevice::newIMP() const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    IMPrimitive* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new IM emulation primitive. The callee is responsible
            /// for it's deletion!
            temp = new (_gpuObjectArena) glIMPrimitive(refThis(this));
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkIMPrimitive(refThis(this));
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noIMPrimitive(refThis(this));
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

VertexBuffer* GFXDevice::newVB() const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    VertexBuffer* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new vertex array (VAO + VB + IB).
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glVertexArray(refThis(this));
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkVertexBuffer(refThis(this));
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noVertexBuffer(refThis(this));
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

PixelBuffer* GFXDevice::newPB(PBType type, const char* name) const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    PixelBuffer* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new pixel buffer using the requested format.
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glPixelBuffer(refThis(this), type, name);
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkPixelBuffer(refThis(this), type, name);
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noPixelBuffer(refThis(this), type, name);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

GenericVertexData* GFXDevice::newGVD(const U32 ringBufferLength, const char* name) const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    GenericVertexData* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new generic vertex data object
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glGenericVertexData(refThis(this), ringBufferLength, name);
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkGenericVertexData(refThis(this), ringBufferLength, name);
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noGenericVertexData(refThis(this), ringBufferLength, name);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
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
    bool locked = _gpuObjectArenaMutex.try_lock();
    

    // Texture is a resource! Do not use object arena!
    Texture* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new texture. The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glTexture(refThis(this), descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor);
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkTexture(refThis(this), descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor);
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noTexture(refThis(this), descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

Pipeline* GFXDevice::newPipeline(const PipelineDescriptor& descriptor) const {
    // Pipeline with not shader is no pipeline at all
    DIVIDE_ASSERT(descriptor._shaderProgramHandle != 0, "Missing shader handle during pipeline creation!");

    size_t hash = descriptor.getHash();

    UniqueLock lock(_pipelineCacheLock);
    hashMap<size_t, Pipeline>::iterator it = _pipelineCache.find(hash);
    if (it == std::cend(_pipelineCache)) {
        return &hashAlg::insert(_pipelineCache, hash, Pipeline(descriptor)).first->second;
    }

    return &it->second;
}

ShaderProgram* GFXDevice::newShaderProgram(size_t descriptorHash,
                                           const stringImpl& name,
                                           const stringImpl& resourceName,
                                           const stringImpl& resourceLocation,
                                           bool asyncLoad) const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    // ShaderProgram is a resource! Do not use object arena!
    ShaderProgram* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new shader program.
            /// The callee is responsible for it's deletion!
            temp = new (_gpuObjectArena) glShaderProgram(refThis(this), descriptorHash, name, resourceName, resourceLocation, asyncLoad);
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkShaderProgram(refThis(this), descriptorHash, name, resourceName, resourceLocation, asyncLoad);
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noShaderProgram(refThis(this), descriptorHash, name, resourceName, resourceLocation, asyncLoad);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

ShaderBuffer* GFXDevice::newSB(const ShaderBufferDescriptor& descriptor) const {
    bool locked = _gpuObjectArenaMutex.try_lock();

    ShaderBuffer* temp = nullptr;
    switch (_API_ID) {
        case RenderAPI::OpenGL:
        case RenderAPI::OpenGLES: {
            /// Create and return a new shader buffer. The callee is responsible for it's deletion!
            /// The OpenGL implementation creates either an 'Uniform Buffer Object' if unbound is false
            /// or a 'Shader Storage Block Object' otherwise
            /// The shader buffer can also be persistently mapped, if requested
            temp = new (_gpuObjectArena) glUniformBuffer(refThis(this), descriptor);
        } break;
        case RenderAPI::Vulkan: {
            temp = new (_gpuObjectArena) vkUniformBuffer(refThis(this), descriptor);
        } break;
        case RenderAPI::None: {
            temp = new (_gpuObjectArena) noUniformBuffer(refThis(this), descriptor);
        } break;
        default: {
            DIVIDE_UNEXPECTED_CALL(Locale::get(_ID("ERROR_GFX_DEVICE_API")));
        } break;
    };

    if (temp != nullptr) {
        _gpuObjectArena.DTOR(temp);
    }

    if (locked) {
        _gpuObjectArenaMutex.unlock();
    }

    return temp;
}

}; // namespace Divide