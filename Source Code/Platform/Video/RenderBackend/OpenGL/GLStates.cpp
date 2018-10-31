#include "stdafx.h"

#include "config.h"

#include "Headers/GLWrapper.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

namespace Divide {

/// The following static variables are used to remember the current OpenGL state
GLuint GL_API::s_UBOffsetAlignment = 0;
GLuint GL_API::s_UBMaxSize = 0;
GLuint GL_API::s_SSBOffsetAlignment = 0;
GLuint GL_API::s_SSBMaxSize = 0;
GLint  GL_API::s_lineWidthLimit = 1;
GLuint GL_API::s_dummyVAO = 0;
GLint  GL_API::s_maxTextureUnits = 0;
GLint  GL_API::s_maxAttribBindings = 0;
GLint  GL_API::s_maxFBOAttachments = 0;
GLuint GL_API::s_anisoLevel = 0;

SharedMutex GL_API::s_mipmapQueueSetLock;
hashMap<GLuint, GLsync> GL_API::s_mipmapQueueSync;
GL_API::samplerObjectMap GL_API::s_samplerMap;
std::mutex GL_API::s_samplerMapLock;
GLUtil::glVAOPool GL_API::s_vaoPool;
glHardwareQueryPool* GL_API::s_hardwareQueryPool = nullptr;

GLStateTracker& GL_API::getStateTracker() {
    assert(s_activeStateTracker != nullptr);
    return *s_activeStateTracker;
}

/// Reset as much of the GL default state as possible within the limitations given
void GL_API::clearStates(const DisplayWindow& window, GLStateTracker& stateTracker, bool global) {
    if (global) {
        stateTracker.bindTextures(0, s_maxTextureUnits, nullptr, nullptr);
        stateTracker.setPixelPackUnpackAlignment();
        stateTracker.setActiveBuffer(GL_TEXTURE_BUFFER, 0);
        stateTracker.setActiveBuffer(GL_UNIFORM_BUFFER, 0);
        stateTracker.setActiveBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        stateTracker.setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        stateTracker.setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        stateTracker._activePixelBuffer = nullptr;
        stateTracker._activeViewport.set(-1);
    }
    stateTracker._previousStateBlockHash = 0;

    stateTracker.setActiveVAO(0);
    stateTracker.setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    stateTracker.setActiveFB(RenderTarget::RenderTargetUsage::RT_READ_WRITE, 0);
    stateTracker.setActiveTransformFeedback(0);
    stateTracker._activeClearColour.set(window.clearColour());
    for (vec_size i = 0; i < stateTracker._blendEnabled.size(); ++i) {
        stateTracker.setBlending((GLuint)i, BlendingProperties(), true);
    }
    stateTracker.setBlendColour(UColour(0u), true);

    const vec2<U16>& drawableSize = _context.getDrawableSize(window);
    stateTracker.setScissor(Rect<I32>(0, 0, drawableSize.width, drawableSize.height));

    stateTracker._activePipeline = nullptr;
    stateTracker._activeRenderTarget = nullptr;
    Attorney::GLAPIShaderProgram::unbind();
}

/// Clipping planes are only enabled/disabled if they differ from the current
/// state
void GL_API::updateClipPlanes(const FrustumClipPlanes& list) {
    // For every clip plane that we support (usually 6)
    for (U32 i = 0; i < to_base(ClipPlaneIndex::COUNT); ++i) {
        // Check its state and compare it with OpenGL's current state
        bool& activePlane = _activeClipPlanes[i];
        if (activePlane != list._active[i]) {
            // Update the clip plane if it differs internally
            activePlane = !activePlane;
            activePlane ? glEnable(GLenum((U32)GL_CLIP_DISTANCE0 + i))
                        : glDisable(GLenum((U32)GL_CLIP_DISTANCE0 + i));
        }
    }
}

bool GL_API::deleteBuffers(GLuint count, GLuint* buffers) {
    if (count > 0 && buffers != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            GLuint crtBuffer = buffers[i];
            for (auto it : s_stateTrackers) {
                for (GLuint& boundBuffer : it.second._activeBufferID) {
                    if (boundBuffer == crtBuffer) {
                        boundBuffer = GLUtil::_invalidObjectID;
                    }
                }
                for (auto boundBuffer : it.second._activeVAOIB) {
                    if (boundBuffer.second == crtBuffer) {
                        boundBuffer.second = GLUtil::_invalidObjectID;
                    }
                }
            }
        }

        glDeleteBuffers(count, buffers);
        memset(buffers, 0, count * sizeof(GLuint));
        return true;
    }

    return false;
}

bool GL_API::deleteVAOs(GLuint count, GLuint* vaos) {
    if (count > 0 && vaos != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            for (auto it : s_stateTrackers) {
                if (it.second._activeVAOID == vaos[i]) {
                    it.second._activeVAOID = GLUtil::_invalidObjectID;
                    break;
                }
            }
        }
        glDeleteVertexArrays(count, vaos);
        memset(vaos, 0, count * sizeof(GLuint));
        return true;
    }
    return false;
}

bool GL_API::deleteFramebuffers(GLuint count, GLuint* framebuffers) {
    if (count > 0 && framebuffers != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            GLuint crtFB = framebuffers[i];
            for (auto it : s_stateTrackers) {
                for (GLuint& activeFB : it.second._activeFBID) {
                    if (activeFB == crtFB) {
                        activeFB = GLUtil::_invalidObjectID;
                    }
                }
            }
        }
        glDeleteFramebuffers(count, framebuffers);
        memset(framebuffers, 0, count * sizeof(GLuint));
        return true;
    }
    return false;
}

bool GL_API::deleteShaderPrograms(GLuint count, GLuint* programs) {
    if (count > 0 && programs != nullptr) {
        for (GLuint i = 0; i < count; ++i) {
            for (auto it : s_stateTrackers) {
                if (it.second._activeShaderProgram == programs[i]) {
                    it.second._activeShaderProgram = 0;
                }
            }
            glDeleteProgram(programs[i]);
        }
        
        memset(programs, 0, count * sizeof(GLuint));
        return true;
    }
    return false;
}

bool GL_API::deleteTextures(GLuint count, GLuint* textures) {
    if (count > 0 && textures != nullptr) {
        
        for (GLuint i = 0; i < count; ++i) {
            GLuint crtTex = textures[i];
            if (crtTex != 0) {
                for (auto it : s_stateTrackers) {
                    for (GLuint& boundTex : it.second._textureBoundMap) {
                        if (boundTex == crtTex) {
                            boundTex = 0;
                        }
                    }
                    for (ImageBindSettings& settings : it.second._imageBoundMap) {
                        if (settings._texture == crtTex) {
                            settings.reset();
                        }
                    }
                }
            }
        }
        glDeleteTextures(count, textures);
        memset(textures, 0, count * sizeof(GLuint));
        return true;
    }

    return false;
}

bool GL_API::deleteSamplers(GLuint count, GLuint* samplers) {
    if (count > 0 && samplers != nullptr) {

        for (GLuint i = 0; i < count; ++i) {
            GLuint crtSampler = samplers[i];
            if (crtSampler != 0) {
                for (auto it : s_stateTrackers) {
                    for (GLuint& boundSampler : it.second._samplerBoundMap) {
                        if (boundSampler == crtSampler) {
                            boundSampler = 0;
                        }
                    }
                }
            }
        }
        glDeleteSamplers(count, samplers);
        memset(samplers, 0, count * sizeof(GLuint));
        return true;
    }

    return false;
}

};