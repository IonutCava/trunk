﻿#include "Headers/GLWrapper.h"
#include "Headers/glIMPrimitive.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/glsw/Headers/glsw.h"
#include "Platform/Video/OpenGL/Buffers/Framebuffer/Headers/glFramebuffer.h"
#include "Platform/Video/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Platform/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

#include "GUI/Headers/GUIText.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/LightManager.h"
#include "Geometry/Material/Headers/Material.h"

#ifndef GLFONTSTASH_IMPLEMENTATION
#define GLFONTSTASH_IMPLEMENTATION
#define FONTSTASH_IMPLEMENTATION
#include "Text/Headers/fontstash.h"
#include "Text/Headers/glfontstash.h"
#endif
#include <glsl/glsl_optimizer.h>
#include <CEGUI/CEGUI.h>

namespace Divide {

GL_API::GL_API() : RenderAPIWrapper(),
                   _prevWidthNode(0),
                   _prevWidthString(0),
                   _prevSizeNode(0),
                   _prevSizeString(0),
                   _lineWidthLimit(1),
                   _pointDummyVAO(0),
				   _GUIGLrenderer(nullptr),
                   _fonsContext(nullptr),
                   _GLSLOptContex(nullptr),
                   _enableCEGUIRendering(false)
{
    // Only updated in Debug builds
    FRAME_DURATION_GPU = 0;
    // Atomic boolean use to signal the loading thread to stop
    _closeLoadingThread = false;
    // Initial values for performance queries
    for (U8 i = 0; i < PERFORMANCE_COUNTER_BUFFERS; ++i) {
        for (U8 j = 0; j < PERFORMANCE_COUNTERS; ++j) {
            _queryID[i][j] = 0;
        }
    }
    // All clip planes are disabled at first (default OpenGL state)
    for (U8 index = 0; index < Config::MAX_CLIP_PLANES; ++index) {
        _activeClipPlanes[index] = false;
    }
}

GL_API::~GL_API()
{
}

/// FontStash library initialization
void GL_API::createFonsContext() {
    // 512x512 atlas with bottom-left origin
    _fonsContext = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT);
}

/// FontStash library deinitialization
void GL_API::deleteFonsContext() {
    glfonsDelete(_fonsContext);
    _fonsContext = nullptr;
}

/// Prepare the GPU for rendering a frame
void GL_API::beginFrame() {
    // Start a duration query in debug builds
#   ifdef _DEBUG
        glBeginQuery(GL_TIME_ELAPSED, _queryID[_queryBackBuffer][0]);
#   endif
    // Restore the clear color (in case it changed)
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
    // Clear our buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT/* | GL_STENCIL_BUFFER_BIT*/);
    // Clears are registered as draw calls by most software, so we do the same to stay in sync with third party software
    GFX_DEVICE.registerDrawCall();
    GL_API::setActiveBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectDrawBuffer);
}

/// Finish rendering the current frame
void GL_API::endFrame() {
    // Revert back to the default OpenGL states
    clearStates(false, false, false);
    // CEGUI handles its own states, so render it after we clear our states but before we swap buffers
    if (_enableCEGUIRendering) {
        CEGUI::System::getSingleton().renderAllGUIContexts();
    }
    // Swap buffers
    glfwSwapBuffers(GLUtil::_mainWindow);
    // Poll for new events
    glfwPollEvents();
    // End the timing query started in beginFrame() in debug builds
#   ifdef _DEBUG
        glEndQuery(GL_TIME_ELAPSED);
        // Swap query objects. The current time will be available after 4 frames
        _queryBackBuffer   = GFX_DEVICE.getFrameCount() % 4;
        _queryFrontBuffer = 3 - _queryBackBuffer;
#   endif
}

/// Prepare our shader loading system
bool GL_API::initShaders() {
    // Initialize GLSW
    GLint glswState = glswInit();
    // Add our engine specific defines and various code pieces to every GLSL shader
    // Add version as the first shader statement, followed by copyright notice
    glswAddDirectiveToken("","#version 430 core\n/*“Copyright 2009-2014 DIVIDE-Studio”*/");
    glswAddDirectiveToken("","#extension GL_ARB_shader_draw_parameters : require");
    // Add current build environment information to the shaders
#   if defined(_DEBUG)
        glswAddDirectiveToken("", "#define _DEBUG");
#   elif defined(_PROFILE)
        glswAddDirectiveToken("", "#define _PROFILE");
#   else
        glswAddDirectiveToken("", "#define _RELEASE");
#   endif

    // Shader stage level reflection system. A shader stage must know what stage it's used for
    glswAddDirectiveToken("Vertex",   "#define VERT_SHADER");
    glswAddDirectiveToken("Geometry", "#define GEOM_SHADER");
    glswAddDirectiveToken("Fragment", "#define FRAG_SHADER");
    glswAddDirectiveToken("TessellationE", "#define TESS_EVAL_SHADER");
    glswAddDirectiveToken("TessellationC", "#define TESS_CTRL_SHADER");
    glswAddDirectiveToken("Compute",  "#define COMPUTE_SHADER");
    // This line gets replaced in every shader at load with the custom list of defines specified by the material
    glswAddDirectiveToken("", "//__CUSTOM_DEFINES__");
    // Add some nVidia specific pragma directives
    if (GFX_DEVICE.getGPUVendor() == GPU_VENDOR_NVIDIA) {
        glswAddDirectiveToken("","//#pragma optionNV(fastmath on)");
        glswAddDirectiveToken("","//#pragma optionNV(fastprecision on)");
        glswAddDirectiveToken("","//#pragma optionNV(inline all)");
        glswAddDirectiveToken("","//#pragma optionNV(ifcvt none)");
        glswAddDirectiveToken("","//#pragma optionNV(strict on)");
        glswAddDirectiveToken("","//#pragma optionNV(unroll all)");
    }
    // Try to sync engine specific data and values with GLSL
    glswAddDirectiveToken("", ("#define MAX_CLIP_PLANES " + 
                                Util::toString(Config::MAX_CLIP_PLANES)).c_str());
    glswAddDirectiveToken("", ("#define MAX_SHADOW_CASTING_LIGHTS " + 
                                Util::toString(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE)).c_str());
    glswAddDirectiveToken("", ("const uint MAX_SPLITS_PER_LIGHT = " + 
                                Util::toString(Config::Lighting::MAX_SPLITS_PER_LIGHT) + ";").c_str());
    glswAddDirectiveToken("", ("const uint MAX_LIGHTS_PER_SCENE = " + 
                                Util::toString(Config::Lighting::MAX_LIGHTS_PER_SCENE) + ";").c_str());
    glswAddDirectiveToken("", ("#define SHADER_BUFFER_LIGHT_NORMAL " + 
                                Util::toString(SHADER_BUFFER_LIGHT_NORMAL)).c_str());
    glswAddDirectiveToken("", ("#define SHADER_BUFFER_GPU_BLOCK " + 
                                Util::toString(SHADER_BUFFER_GPU_BLOCK)).c_str());
    glswAddDirectiveToken("", ("#define SHADER_BUFFER_NODE_INFO " + 
                                Util::toString(SHADER_BUFFER_NODE_INFO)).c_str());
    glswAddDirectiveToken("", "const float Z_TEST_SIGMA = 0.0001;");
    glswAddDirectiveToken("", "const float ALPHA_DISCARD_THRESHOLD = 0.25;");
    
    glswAddDirectiveToken("Fragment", "#define VARYING in"); 
    glswAddDirectiveToken("Fragment", ("#define SHADER_BUFFER_LIGHT_SHADOW " + 
                                        Util::toString(SHADER_BUFFER_LIGHT_SHADOW)).c_str());
	glswAddDirectiveToken("Fragment", ("#define MAX_TEXTURE_SLOTS " + 
                          Util::toString(ParamHandler::getInstance().getParam<I32>("rendering.maxTextureSlots", 16))).c_str());
    glswAddDirectiveToken("Fragment", ("#define TEXTURE_UNIT0 " +
                                        Util::toString(ShaderProgram::TEXTURE_UNIT0)).c_str());
    glswAddDirectiveToken("Fragment", ("#define TEXTURE_UNIT1 " + 
                                        Util::toString(ShaderProgram::TEXTURE_UNIT1)).c_str());
    glswAddDirectiveToken("Fragment", ("#define TEXTURE_NORMALMAP " + 
                                        Util::toString(ShaderProgram::TEXTURE_NORMALMAP)).c_str());
    glswAddDirectiveToken("Fragment", ("#define TEXTURE_OPACITY " + 
                                        Util::toString(ShaderProgram::TEXTURE_OPACITY)).c_str());
    glswAddDirectiveToken("Fragment", ("#define TEXTURE_SPECULAR " + 
                                        Util::toString(ShaderProgram::TEXTURE_SPECULAR)).c_str());
    glswAddDirectiveToken("Fragment", ("#define TEXTURE_PROJECTION " + 
                                        Util::toString(ShaderProgram::TEXTURE_PROJECTION)).c_str());
    glswAddDirectiveToken("Fragment", ("#define SHADOW_CUBE_START " + 
        Util::toString((U32)LightManager::getInstance().getShadowBindSlotOffset(LightManager::SHADOW_SLOT_TYPE_CUBE))).c_str());
    glswAddDirectiveToken("Fragment", ("#define SHADOW_NORMAL_START " + 
        Util::toString((U32)LightManager::getInstance().getShadowBindSlotOffset(LightManager::SHADOW_SLOT_TYPE_NORMAL))).c_str());
    glswAddDirectiveToken("Fragment", ("#define SHADOW_ARRAY_START " + 
        Util::toString((U32)LightManager::getInstance().getShadowBindSlotOffset(LightManager::SHADOW_SLOT_TYPE_ARRAY))).c_str());
    glswAddDirectiveToken("Fragment", "const uint DEPTH_EXP_WARP = 32;");

    // GLSL <-> VBO intercommunication 
    glswAddDirectiveToken("Vertex", "#define VARYING out");
    glswAddDirectiveToken("Vertex", ("const uint MAX_BONE_COUNT_PER_NODE = " + 
                                      Util::toString(Config::MAX_BONE_COUNT_PER_NODE) + ";").c_str());
    glswAddDirectiveToken("Vertex", ("#define SHADER_BUFFER_BONE_TRANSFORMS " + 
                                      Util::toString(SHADER_BUFFER_BONE_TRANSFORMS)).c_str());
    // Vertex data has a fixed format
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_POSITION_LOCATION) + 
                                     ") in vec3  inVertexData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_COLOR_LOCATION) + 
                                     ") in vec4  inColorData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_NORMAL_LOCATION) + 
                                     ") in vec3  inNormalData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_TEXCOORD_LOCATION) + 
                                     ") in vec2  inTexCoordData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_TANGENT_LOCATION) + 
                                     ") in vec3  inTangentData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_BITANGENT_LOCATION) + 
                                     ") in vec3  inBiTangentData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " +
                                      Util::toString(VERTEX_BONE_WEIGHT_LOCATION) + 
                                     ") in vec4  inBoneWeightData;").c_str());
    glswAddDirectiveToken("Vertex", ("layout(location = " + 
                                      Util::toString(VERTEX_BONE_INDICE_LOCATION) + 
                                     ") in ivec4 inBoneIndiceData;").c_str());
    // GPU specific data, such as GFXDevice's main uniform block and clipping planes 
    // are defined in an external file included in every shader
    glswAddDirectiveToken("", "#include \"nodeDataInput.cmn\"");
    // After we finish with all of the defines, add all of the custom uniform variables defined by each material
    glswAddDirectiveToken("", "//__CUSTOM_UNIFORMS__");
    // Create an optimisation context used for post-processing shaders 
    // (using Aras Pranckevičius's glsl-optimizer: https://github.com/aras-p/glsl-optimizer )
    _GLSLOptContex = glslopt_initialize(GFX_DEVICE.getApi() == OpenGLES ? kGlslTargetOpenGLES30 : kGlslTargetOpenGL);
    // Check initialization status for GLSL and glsl-optimizer
    return (glswState == 1 && _GLSLOptContex != nullptr);
}

/// Revert everything that was set up in "initShaders()"
bool GL_API::deInitShaders() {
    // Delete the glsl-optimizer context
    glslopt_cleanup(_GLSLOptContex);
    // Shutdown GLSW
    return (glswShutdown() == GL_TRUE);
}

/// Try to find the requested font in the font cache. Load on cache miss.
I32 GL_API::getFont(const stringImpl& fontName) {
    // Search for the requested font by name
    FontCache::const_iterator it = _fonts.find(fontName);
    // If we failed to find it, it wasn't loaded yet
    if (it == _fonts.end()) {
        // Fonts are stored in the general asset directory
		stringImpl fontPath(ParamHandler::getInstance().getParam<stringImpl>("assetsLocation", "assets") + "/");
        // In the GUI subfolder
        fontPath += "GUI/";
        // In the fonts subfolder
        fontPath += "fonts/";
        fontPath += fontName;
        // We use FontStash to load the font file
        I32 tempFont = fonsAddFont(_fonsContext, fontName.c_str(), fontPath.c_str());
        // If the font is invalid, inform the user, but map it anyway, to avoid loading an invalid font file on every request
        if (tempFont == FONS_INVALID) {
            ERROR_FN(Locale::get("ERROR_FONT_FILE"), fontName.c_str());
        }
        // Save the font in the font cache
        hashAlg::pair<FontCache::iterator, bool> result = hashAlg::emplace(_fonts, fontName, tempFont);
		assert(result.second);
        // Return the font
		return result.first->second;
    }
    // We found the font in cache, so return it
    return it->second;
}

/// Text rendering is handled exclusively by Mikko Mononen's FontStash library (https://github.com/memononen/fontstash)
/// with his OpenGL frontend adapted for core context profiles
void GL_API::drawText(const TextLabel& textLabel, const vec2<I32>& position) {
    // Retrieve the font from the font cache
    I32 font = getFont(textLabel._font);
    // The font may be invalid, so skip this text label
    if (font == FONS_INVALID) {
        return;
    }
    // See FontStash documentation for the following block
    {
        fonsClearState(_fonsContext);
        fonsSetSize(_fonsContext, textLabel._height);
        fonsSetFont(_fonsContext, font);
        F32 lh = 0;
        fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);
        fonsSetColor(_fonsContext, 
                     textLabel._color.r * 255, 
                     textLabel._color.g * 255, 
                     textLabel._color.b * 255, 
                     textLabel._color.a * 255);

        if (textLabel._blurAmount > 0.01f) {
            fonsSetBlur(_fonsContext, textLabel._blurAmount );
        }

        if (textLabel._spacing > 0.01f) {
            fonsSetBlur(_fonsContext, textLabel._spacing );
        }

        if (textLabel._alignFlag != 0) {
            fonsSetAlign(_fonsContext, textLabel._alignFlag);
        }

        fonsDrawText(_fonsContext, position.x, _cachedResolution.y - (position.y + lh), textLabel._text.c_str(), nullptr);
    }
    // Register each label rendered as a draw call
    GFX_DEVICE.registerDrawCall();
}

/// Rendering points is universally useful, so we have a function, and a VAO, dedicated to this process
void GL_API::drawPoints(GLuint numPoints) {
    GL_API::setActiveVAO(_pointDummyVAO);
    glDrawArrays(GL_POINTS, 0, numPoints);
    GFX_DEVICE.registerDrawCall();
}

void GL_API::uploadDrawCommands(const vectorImpl<IndirectDrawCommand>& drawCommands) const {
    glNamedBufferDataEXT(_indirectDrawBuffer, 
                         sizeof(IndirectDrawCommand) * drawCommands.size(), 
                         drawCommands.data(), 
                         GL_DYNAMIC_COPY);
}

/// Verify if we have a sampler object created and available for the given descriptor
size_t GL_API::getOrCreateSamplerObject(const SamplerDescriptor& descriptor) {
    // Get the descriptor's hash value
    size_t hashValue = descriptor.getHash();
    // Try to find the hash value in the sampler object map
    samplerObjectMap::const_iterator it = _samplerMap.find(hashValue);
    // If we fail to find it, we need to create a new sampler object
    if (it == _samplerMap.end()) {
        // Create and store the newly created sample object. GL_API is responsible for deleting these!
        hashAlg::emplace(_samplerMap, hashValue, MemoryManager_NEW glSamplerObject(descriptor));
    }
    // Return the sampler object's hash value
    return hashValue;
}

/// Return the OpenGL sampler object's handle for the given hash value
GLuint GL_API::getSamplerHandle(size_t samplerHash) {
    // If the hash value is 0, we assume the code is trying to unbind a sampler object
    if (samplerHash == 0) {
        return 0;
    }
    // If we fail to find the sampler object for the given hash, we print an error and return the default OpenGL handle
    samplerObjectMap::const_iterator it = _samplerMap.find(samplerHash);
    if (it == _samplerMap.end()) {
        ERROR_FN(Locale::get("ERROR_NO_SAMPLER_OBJECT_FOUND"), samplerHash);
        return 0;
    }
    // Return the OpenGL handle for the sampler object matching the specified hash value
    return it->second->getObjectHandle();
}

/// Create and return a new IM emulation primitive. The callee is responsible for it's deletion!
IMPrimitive* GL_API::newIMP() const {
    return MemoryManager_NEW glIMPrimitive();
}

/// Create and return a new framebuffer. The callee is responsible for it's deletion!
Framebuffer* GL_API::newFB(bool multisampled) const {
    // If MSAA is disabled, this will be a simple color / depth buffer
    // If we requested a MultiSampledFramebuffer and MSAA is enabled, we also allocate a resolve framebuffer
    // We set the resolve framebuffer as the requested framebuffer's child. 
    // The framebuffer is responsible for deleting it's own resolve child!
    return MemoryManager_NEW glFramebuffer((multisampled && GFX_DEVICE.MSAAEnabled()) ? MemoryManager_NEW glFramebuffer() : 
                                                                                        nullptr);
}

/// Create and return a new vertex array (VAO + VB + IB). The callee is responsible for it's deletion!
VertexBuffer* GL_API::newVB() const {
    return MemoryManager_NEW glVertexArray();
}

/// Create and return a new pixel buffer using the requested format. The callee is responsible for it's deletion!
PixelBuffer* GL_API::newPB(const PBType& type) const {
    return MemoryManager_NEW glPixelBuffer(type);
}

/// Create and return a new generic vertex data object and, optionally set it as persistently mapped. 
/// The callee is responsible for it's deletion!
GenericVertexData* GL_API::newGVD(const bool persistentMapped) const {
    return MemoryManager_NEW glGenericVertexData(persistentMapped);
}

/// Create and return a new shader buffer. The callee is responsible for it's deletion!
/// The OpenGL implementation creates either an 'Uniform Buffer Object' if unbound is false
/// or a 'Shader Storage Block Object' otherwise
ShaderBuffer* GL_API::newSB(const bool unbound, const bool persistentMapped) const {
    // The shader buffer can also be persistently mapped, if requested
    return MemoryManager_NEW glUniformBuffer(unbound, persistentMapped);
}

/// Create and return a new texture array (optionally, flipped vertically). The callee is responsible for it's deletion!
Texture* GL_API::newTextureArray(const bool flipped) const {
    return MemoryManager_NEW glTexture(TEXTURE_2D_ARRAY, flipped);
}

/// Create and return a new 2D texture (optionally, flipped vertically). The callee is responsible for it's deletion!
Texture* GL_API::newTexture2D(const bool flipped) const {
    return MemoryManager_NEW glTexture(TEXTURE_2D, flipped);
}

/// Create and return a new cube texture (optionally, flipped vertically). The callee is responsible for it's deletion!
Texture* GL_API::newTextureCubemap(const bool flipped) const {
    return MemoryManager_NEW glTexture(TEXTURE_CUBE_MAP, flipped);
}

/// Create and return a new shader program (optionally, post load optimised). The callee is responsible for it's deletion!
ShaderProgram* GL_API::newShaderProgram(const bool optimise) const {
    return MemoryManager_NEW glShaderProgram(optimise);
}

/// Create and return a new shader of the specified type by loading the specified name (optionally, post load optimised). 
/// The callee is responsible for it's deletion!
Shader* GL_API::newShader(const stringImpl& name, const ShaderType& type, const bool optimise) const {
    return MemoryManager_NEW glShader(name, type, optimise);
}

};