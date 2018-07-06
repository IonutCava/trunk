#include "Headers/GLWrapper.h"
#include "Headers/glImmediateModeEmulation.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Scenes/Headers/SceneState.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/OpenGL/glsw/Headers/glsw.h"

#include "Hardware/Video/OpenGL/Buffers/Framebuffer/Headers/glFramebuffer.h"
#include "Hardware/Video/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Hardware/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"

#include <glsl/glsl_optimizer.h>

#ifndef GLFONTSTASH_IMPLEMENTATION
#define GLFONTSTASH_IMPLEMENTATION
#define FONTSTASH_IMPLEMENTATION
#include "Text/Headers/fontstash.h"
#include "Text/Headers/glfontstash.h"
#endif

GLuint64 GL_API::FRAME_DURATION_GPU = 0;

#include <glim.h>

void GL_API::createFonsContext() {
    _fonsContext = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT);
}

void GL_API::deleteFonsContext() {
    glfonsDelete(_fonsContext);
    _fonsContext = nullptr;
}

void GL_API::beginFrame(){
#ifdef _DEBUG
    GLuint queryId = _queryID[_queryBackBuffer][0];
    glBeginQuery(GL_TIME_ELAPSED, queryId);
#endif
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE(), 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT/* | GL_STENCIL_BUFFER_BIT*/);
    GFX_DEVICE.registerDrawCall();
}

void GL_API::endFrame(){

    //unbind all states (shaders, textures, buffers)
    clearStates(false,false,false);

    glfwSwapBuffers(Divide::GLUtil::_mainWindow);

#ifdef _DEBUG
    glEndQuery(GL_TIME_ELAPSED);

    GLuint query = GFX_DEVICE.getFrameCount() % 4;
    
    _queryBackBuffer = query;
    _queryFrontBuffer = 3 - _queryBackBuffer;
    
#endif
}

void GL_API::flush(){
    clearStates(false,false,false);
}

bool GL_API::initShaders(){
    //Init glsw library
    GLint glswState = glswInit();
    glswAddDirectiveToken("","#version 430 core\n/*“Copyright 2009-2014 DIVIDE-Studio”*/");
#if defined(_DEBUG)
    glswAddDirectiveToken("", "#define _DEBUG");
#elif defined(_PROFILE)
    glswAddDirectiveToken("", "#define _PROFILE");
#else
    glswAddDirectiveToken("", "#define _RELEASE");
#endif

    glswAddDirectiveToken("Vertex",   "#define VERT_SHADER");
    glswAddDirectiveToken("Geometry", "#define GEOM_SHADER");
    glswAddDirectiveToken("Fragment", "#define FRAG_SHADER");
    glswAddDirectiveToken("TessellationE", "#define TESS_EVAL_SHADER");
    glswAddDirectiveToken("TessellationC", "#define TESS_CTRL_SHADER");
    glswAddDirectiveToken("Compute",  "#define COMPUTE_SHADER");

    glswAddDirectiveToken("", "//__CUSTOM_DEFINES__");

    if(getGPUVendor() == GPU_VENDOR_NVIDIA){ //nVidia specific
        glswAddDirectiveToken("","#pragma optionNV(fastmath on)");
        glswAddDirectiveToken("","#pragma optionNV(fastprecision on)");
        glswAddDirectiveToken("","//#pragma optionNV(inline all)");
        glswAddDirectiveToken("","//#pragma optionNV(ifcvt none)");
        glswAddDirectiveToken("","#pragma optionNV(strict on)");
        glswAddDirectiveToken("","//#pragma optionNV(unroll all)");
    }

    glswAddDirectiveToken("", std::string("#define MAX_CLIP_PLANES " + Util::toString(Config::MAX_CLIP_PLANES)).c_str());
    glswAddDirectiveToken("", std::string("#define MAX_SHADOW_CASTING_LIGHTS " + Util::toString(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS_PER_NODE)).c_str());
    glswAddDirectiveToken("", std::string("const uint MAX_SPLITS_PER_LIGHT = " + Util::toString(Config::Lighting::MAX_SPLITS_PER_LIGHT) + ";").c_str());
    glswAddDirectiveToken("", std::string("const uint MAX_LIGHTS_PER_SCENE = " + Util::toString(Config::Lighting::MAX_LIGHTS_PER_SCENE) + ";").c_str());
    glswAddDirectiveToken("", std::string("#define SHADER_BUFFER_LIGHT_NORMAL " + Util::toString(Divide::SHADER_BUFFER_LIGHT_NORMAL)).c_str());
    glswAddDirectiveToken("", std::string("#define SHADER_BUFFER_GPU_BLOCK " + Util::toString(Divide::SHADER_BUFFER_GPU_BLOCK)).c_str());
    glswAddDirectiveToken("", std::string("#define SHADER_BUFFER_NODE_INFO " + Util::toString(Divide::SHADER_BUFFER_NODE_INFO)).c_str());
    glswAddDirectiveToken("", "const float Z_TEST_SIGMA = 0.0001;");
    glswAddDirectiveToken("", "const float ALPHA_DISCARD_THRESHOLD = 0.25;");
   
    glswAddDirectiveToken("Fragment", std::string("#define VARYING in").c_str()); 
    glswAddDirectiveToken("Fragment", std::string("#define SHADER_BUFFER_LIGHT_SHADOW " + Util::toString(Divide::SHADER_BUFFER_LIGHT_SHADOW)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_UNIT0 " + Util::toString(Material::TEXTURE_UNIT0)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_UNIT1 " + Util::toString(Material::TEXTURE_UNIT1)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_PROJECTION " + Util::toString(Material::TEXTURE_PROJECTION)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_NORMALMAP " + Util::toString(Material::TEXTURE_NORMALMAP)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_OPACITY " + Util::toString(Material::TEXTURE_OPACITY)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_SPECULAR " + Util::toString(Material::TEXTURE_SPECULAR)).c_str());
    glswAddDirectiveToken("Fragment", "const uint DEPTH_EXP_WARP = 32;");
    
    // GLSL <-> VBO intercommunication 
    glswAddDirectiveToken("Vertex", std::string("#define VARYING out").c_str());
    glswAddDirectiveToken("Vertex", std::string("const uint MAX_BONE_COUNT_PER_NODE = " + Util::toString(Config::MAX_BONE_COUNT_PER_NODE) + ";").c_str());
    glswAddDirectiveToken("Vertex", std::string("#define SHADER_BUFFER_BONE_TRANSFORMS " + Util::toString(Divide::SHADER_BUFFER_BONE_TRANSFORMS)).c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_POSITION_LOCATION)    + ") in vec3  inVertexData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_COLOR_LOCATION)       + ") in vec4  inColorData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_NORMAL_LOCATION)      + ") in vec3  inNormalData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_TEXCOORD_LOCATION)    + ") in vec2  inTexCoordData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_TANGENT_LOCATION)     + ") in vec3  inTangentData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_BITANGENT_LOCATION)   + ") in vec3  inBiTangentData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_BONE_WEIGHT_LOCATION) + ") in vec4  inBoneWeightData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_BONE_INDICE_LOCATION) + ") in ivec4 inBoneIndiceData;").c_str());

    glswAddDirectiveToken("", std::string("#include \"nodeDataInput.cmn\"").c_str());

    glswAddDirectiveToken("", "//__CUSTOM_UNIFORMS__");

    GL_API::_GLSLOptContex = glslopt_initialize(GFX_DEVICE.getApi() == OpenGLES ? kGlslTargetOpenGLES30 : kGlslTargetOpenGL);

    return glswState == 1 && GL_API::_GLSLOptContex != nullptr;
}

bool GL_API::deInitShaders(){
    glslopt_cleanup(_GLSLOptContex);
    //Shut down glsw and clean memory
    return (glswShutdown() == 1);
}

I32 GL_API::getFont(const std::string& fontName){
    const FontCache::const_iterator& it = _fonts.find(fontName);
    if(it == _fonts.end()){
        std::string fontPath = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/";
        fontPath += "GUI/";
        fontPath += "fonts/";
        fontPath += fontName;
        I32 tempFont = fonsAddFont(_fonsContext, fontName.c_str(), fontPath.c_str());

        if (tempFont == FONS_INVALID) {
            ERROR_FN(Locale::get("ERROR_FONT_FILE"),fontName.c_str());
        }
        _fonts.insert(std::make_pair(fontName, tempFont));
        return tempFont;
    }
    
    return it->second;
}

void GL_API::drawText(const TextLabel& textLabel, const vec2<I32>& position){
    I32 font = getFont(textLabel._font);
    if(font == FONS_INVALID) return;

    F32 lh = 0;
    fonsClearState(_fonsContext);
    fonsSetSize(_fonsContext, textLabel._height);
    fonsSetFont(_fonsContext, font);
    fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);
    fonsSetColor(_fonsContext, textLabel._color.r * 255, 
                               textLabel._color.g * 255,
                               textLabel._color.b * 255, 
                               textLabel._color.a * 255);
    if(textLabel._blurAmount > 0.01f)
        fonsSetBlur(_fonsContext, textLabel._blurAmount );
    
    if(textLabel._spacing > 0.01f)
        fonsSetBlur(_fonsContext, textLabel._spacing );
    
    if(textLabel._alignFlag != 0)
        fonsSetAlign(_fonsContext, textLabel._alignFlag);
    
    fonsDrawText(_fonsContext, position.x, _cachedResolution.y - (position.y + lh), textLabel._text.c_str(), nullptr);
    GFX_DEVICE.registerDrawCall();
}

void GL_API::drawPoints(GLuint numPoints) {
    GL_API::setActiveVAO(_pointDummyVAO);
    glDrawArrays(GL_POINTS, 0, numPoints);
    GFX_DEVICE.registerDrawCall();
}

IMPrimitive* GL_API::newIMP() const {
    return New glIMPrimitive();
}

/// Creates a new frame buffer
Framebuffer* GL_API::newFB(bool multisampled) const {
    // if MSAA is disabled, this will be a simple color / depth buffer
    // if we requested a MSFB and msaa is enabled, create a resolve buffer
    // and create our FB adding the resolve buffer as it's child
    // else, return a regular frame buffer
     return New glFramebuffer((multisampled && GFX_DEVICE.MSAAEnabled()) ? New glFramebuffer() : nullptr);
}

VertexBuffer* GL_API::newVB() const {
    return New glVertexArray();
}

PixelBuffer* GL_API::newPB(const PBType& type) const {
    return New glPixelBuffer(type);
}

GenericVertexData* GL_API::newGVD(const bool persistentMapped) const {
    return New glGenericVertexData(persistentMapped);
}

ShaderBuffer* GL_API::newSB(const bool unbound, const bool persistentMapped) const {
    return New glUniformBuffer(unbound, persistentMapped);
}

size_t GL_API::getOrCreateSamplerObject(const SamplerDescriptor& descriptor){
    size_t hashValue = descriptor.getHash();

    samplerObjectMap::iterator it = _samplerMap.find(hashValue);
    if (it == _samplerMap.end())
        _samplerMap.insert(std::make_pair(hashValue, New glSamplerObject(descriptor)));
 
    return hashValue;
}

GLuint GL_API::getSamplerHandle(size_t samplerHash) {
    if(samplerHash == 0)
        return 0;

    samplerObjectMap::iterator it = _samplerMap.find(samplerHash);
    if (it == _samplerMap.end()) 
        return 0;

    return it->second->getObjectHandle();
}