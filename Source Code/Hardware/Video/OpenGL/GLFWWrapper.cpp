#include "Headers/glImmediateModeEmulation.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "Core/Headers/Kernel.h"
#include "core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/OpenGL/glsw/Headers/glsw.h"
#include "Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include <glsl/glsl_optimizer.h>
#include <gtc/type_ptr.hpp>
#include <glim.h>

#ifndef FONTSTASH_IMPLEMENTATION
#define FONTSTASH_IMPLEMENTATION
#include "Text/Headers/fontstash.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "Text/Headers/glfontstash.h"
#endif

#include <CEGUI/CEGUI.h>

#ifdef GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_MX_d.lib")
#		pragma comment(lib, "glew32mxsd.lib")
#	else //_DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_MX.lib")
#		pragma comment(lib, "glew32mxs.lib")
#	endif //_DEBUG
#else //GLEW_MX
#	ifdef _DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer_d.lib")
#		pragma comment(lib, "glew32sd.lib")
#	else//_DEBUG
#		pragma comment(lib, "CEGUIOpenGLRenderer.lib")
#		pragma comment(lib, "glew32s.lib")
#	endif //_DEBUG
#endif //GLEW_MX

glslopt_ctx* GL_API::_GLSLOptContex = nullptr;

#ifdef GLEW_MX
///GLEW_MX requirement
static boost::thread_specific_ptr<GLEWContext> _GLEWContextPtr;
GLEWContext * glewGetContext(){ return _GLEWContextPtr.get(); }
#if defined( OS_WINDOWS )
static boost::thread_specific_ptr<WGLEWContext> _WGLEWContextPtr;
WGLEWContext* wglewGetContext(){ return _WGLEWContextPtr.get(); }
#else
static boost::thread_specific_ptr<GLXEWContext> _GLXEWContextPtr;
GLXEWContext* glxewGetContext(){ return _GLXEWContextPtr.get(); }
#endif
#endif//GLEW_MX

namespace Divide {
    namespace GLUtil {
        void glfw_focus_callback(GLFWwindow *window, I32 focusState) {
            Application::getInstance().hasFocus(focusState == GL_TRUE);
        }

        void initGlew(){
#ifdef GLEW_MX
            GLEWContext * currentGLEWContextsPtr =  _GLEWContextPtr.get();
            if (currentGLEWContextsPtr == nullptr)	{
                currentGLEWContextsPtr = New GLEWContext;
                _GLEWContextPtr.reset(currentGLEWContextsPtr);
                ZeroMemory(currentGLEWContextsPtr, sizeof(GLEWContext));
            }
#endif //GLEW_MX
            glewExperimental=TRUE;
            //Everything is set up as needed, so initialize the OpenGL API
            GLuint err = glewInit();
            //Check for errors and print if any (They happen more than anyone thinks)
            //Bad drivers, old video card, corrupt GPU, anything can throw an error
            if (GLEW_OK != err){
                ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
                PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
                PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));;
                //No need to continue, as switching to DX should be set before launching the application!
                exit(GLEW_INIT_ERROR);
            }
#ifdef GLEW_MX
    #if defined( OS_WINDOWS )

            WGLEWContext * currentWGLEWContextsPtr =  _WGLEWContextPtr.get();
            if (currentWGLEWContextsPtr == nullptr)	{
                currentWGLEWContextsPtr = New WGLEWContext;
                _WGLEWContextPtr.reset(currentWGLEWContextsPtr);
                ZeroMemory(currentWGLEWContextsPtr, sizeof(WGLEWContext));
            }

            err = wglewInit();
            if (GLEW_OK != err){
                ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
                PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
                PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));;
                exit(GLEW_INIT_ERROR);
            }
#	else //_WIN32

            GLXEWContext * currentGLXEWContextsPtr =  _GLXEWContextPtr.get();
            if (currentGLXEWContextsPtr == nullptr)	{
                currentGLXEWContextsPtr = New GLXEWContext;
                _GLXEWContextPtr.reset(currentGLXEWContextsPtr);
                ZeroMemory(currentGLXEWContextsPtr, sizeof(GLXEWContext));
            }

            err = glxewInit();
            if (GLEW_OK != err){
                ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
                PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
                PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));;
                exit(GLEW_INIT_ERROR);
            }
#	endif //_WIN32
#endif //GLEW_MX
        }
    }
}

//Let's try and create a  valid OpenGL context.
GLbyte GL_API::initHardware(const vec2<GLushort>& resolution, GLint argc, char **argv){
    ParamHandler& par = ParamHandler::getInstance();

    glfwSetErrorCallback(Divide::GLUtil::glfw_error_callback);

    if( !glfwInit() )	return GLFW_INIT_ERROR;

#if defined(_DEBUG) || defined(_PROFILE)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
#endif
    
    Divide::GLUtil::_initStacks();
    if(GFX_DEVICE.MSAAEnabled())
        glfwWindowHint(GLFW_SAMPLES, GFX_DEVICE.MSAASamples());

    if (getId() == OpenGLES) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    }else{
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    }

    glfwWindowHint(GLFW_RESIZABLE,par.getParam<bool>("runtime.allowWindowResize",false));
    ///32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
    glfwWindowHint(GLFW_RED_BITS,8);
    glfwWindowHint(GLFW_GREEN_BITS,8);
    glfwWindowHint(GLFW_BLUE_BITS,8);
    glfwWindowHint(GLFW_ALPHA_BITS,8);
    glfwWindowHint(GLFW_DEPTH_BITS,24);
    glfwWindowHint(GLFW_STENCIL_BITS,8);
    

    //Store the main window ID for future reference
    // Open an OpenGL window; resolution is specified in the external XML files
    bool window = par.getParam<bool>("runtime.windowedMode",true);
    Divide::GLUtil::_mainWindow = glfwCreateWindow(resolution.width, resolution.height,
                                                   par.getParam<std::string>("appTitle").c_str(),
                                                   window ? nullptr : glfwGetPrimaryMonitor(),
                                                   nullptr);
    boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    if (!Divide::GLUtil::_mainWindow){
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }
    glfwMakeContextCurrent(Divide::GLUtil::_mainWindow);
    //Init glew for main context
    Divide::GLUtil::initGlew();
    //Keep track of window focus so no input is processed if the window isn't selected
    glfwSetWindowFocusCallback(Divide::GLUtil::_mainWindow, Divide::GLUtil::glfw_focus_callback);
    //Geometry shaders became core in version 3.3
    if(!GLEW_VERSION_3_3){
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),"The OpenGL version supported by the current GPU is too old!");
        return GLEW_OLD_HARDWARE;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    Divide::GLUtil::_loaderWindow = glfwCreateWindow(1, 1, "divide-res-loader", nullptr, Divide::GLUtil::_mainWindow);
    if (!Divide::GLUtil::_loaderWindow){
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }

#if defined(_DEBUG) || defined(_PROFILE)
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&Divide::GLUtil::DebugCallback, (GLvoid*)(0));
    //Disable shader compiler errors (shader class handles that)
    glDebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER,
                            GL_DEBUG_TYPE_ERROR,
                            GL_DONT_CARE,
                            0,
                            nullptr,
                            GL_FALSE);
#endif

    const GLFWvidmode* return_struct = glfwGetVideoMode(glfwGetPrimaryMonitor());

    GLint height = return_struct->height;
    GLint width = return_struct->width;
    glfwSetWindowPos(Divide::GLUtil::_mainWindow, (width - resolution.width)*0.5f, (height - resolution.height)*0.5f);

    //If we got here, let's figure out what capabilities we have available
    GLint max_frag_uniform = 0, max_varying_floats = 0;
    GLint max_vertex_uniform = 0, max_vertex_attrib = 0,max_texture_units = 0;
    GLint buffers = 0,samplesEffective = 0;
    GLint major = 0, minor = 0, maxMinor = 0;
    GLint maxAnisotropy = 0;
    GLint maxUBOBindings = 0, maxUBOBlockSize = 0;
    GLint maxSubroutines = 0, maxSubroutineUniforms = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major); // OpenGL major version
    glGetIntegerv(GL_MINOR_VERSION, &minor); // OpenGL minor version
    glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
    glGetIntegerv(GL_SAMPLES, &samplesEffective);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_frag_uniform); //How many uniforms can we send to fragment shaders
    //glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats)); //How many varying floats can we use inside a shader program
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform); //How many uniforms can we send to vertex shaders
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib); //How many attributes can we send to a vertex shader
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units); //Maximum number of texture units we can address in shaders
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);//Maximum supporter anisotropic filtering level
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUBOBindings);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOBlockSize);
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSubroutines);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubroutineUniforms);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &(GFX_DEVICE._maxTextureSlots));
    const GLubyte* glslVersionSupported;
    glslVersionSupported = glGetString(GL_SHADING_LANGUAGE_VERSION);
    std::string gpuVendorByte(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    setGPUVendor(GPU_VENDOR_OTHER);
    if(!gpuVendorByte.empty()){
             if(gpuVendorByte.compare(0,5,"Intel") == 0)  { setGPUVendor(GPU_VENDOR_INTEL);  }
        else if(gpuVendorByte.compare(0,6,"NVIDIA") == 0) { setGPUVendor(GPU_VENDOR_NVIDIA); }
        else if(gpuVendorByte.compare(0,3,"ATI") == 0)    { setGPUVendor(GPU_VENDOR_AMD);    }
        else if(gpuVendorByte.compare(0,3,"AMD") == 0)    { setGPUVendor(GPU_VENDOR_AMD);    }
    }else{
        gpuVendorByte = "Unknown GPU Vendor";
    }

    //Shader selection based on detail
    par.setParam("shaderDetailToken",par.getParam<GLubyte>("rendering.detailLevel"));
    par.setParam("GFX_DEVICE.maxTextureCombinedUnits",max_texture_units);
    //Cap max aniso to what the hardware supports
    if (maxAnisotropy < par.getParam<GLint>("rendering.anisotropicFilteringLevel", 1)){
        par.setParam("rendering.anisotropicFilteringLevel", maxAnisotropy);
    }
    //Time to select our shaders.
    //We do not support OpenGL version lower than 3.0;
    if(major < 4){
        ERROR_FN(Locale::get("ERROR_OPENGL_NOT_SUPPORTED"));
        PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
        PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));
        return GLEW_OLD_HARDWARE;
    }else {
              if(GLEW_VERSION_4_4) maxMinor = 4;
        else  if(GLEW_VERSION_4_3) maxMinor = 3;
        else  if(GLEW_VERSION_4_2) maxMinor = 2;
        else  if(GLEW_VERSION_4_1) maxMinor = 1;
    }

    //Print all of the OpenGL functionality info to the console
    PRINT_FN(Locale::get("GL_MAX_UNIFORM"),max_frag_uniform);
    PRINT_FN(Locale::get("GL_MAX_FRAG_VARYING"),max_varying_floats);
    PRINT_FN(Locale::get("GL_MAX_VERT_UNIFORM"),max_vertex_uniform);
    PRINT_FN(Locale::get("GL_MAX_VERT_ATTRIB"),max_vertex_attrib);
    PRINT_FN(Locale::get("GL_MAX_TEX_UNITS"), max_texture_units, GFX_DEVICE._maxTextureSlots);
    PRINT_FN(Locale::get("GL_MAX_VERSION"),major,maxMinor);
    PRINT_FN(Locale::get("GL_GLSL_SUPPORT"),glslVersionSupported);
    PRINT_FN(Locale::get("GL_VENDOR_STRING"), gpuVendorByte.c_str(), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    PRINT_FN(Locale::get("GL_MULTI_SAMPLE_INFO"),samplesEffective,buffers);
    PRINT_FN(Locale::get("GL_UBO_INFO"),maxUBOBindings, maxUBOBlockSize);
    PRINT_FN(Locale::get("GL_SUBROUTINE_INFO"), maxSubroutines, maxSubroutineUniforms);
        
    GL_ENUM_TABLE::fill();

    //Set the clear color to a nice blue
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE(),0,true);

    if(glewIsSupported("GL_seamless_cube_map")) glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    if(GFX_DEVICE.MSAAEnabled()) glEnable(GL_MULTISAMPLE);

    if(Config::USE_HARDWARE_AA_LINES){
        glEnable(GL_LINE_SMOOTH);
        glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, &_lineWidthLimit);
    }else{
        glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, &_lineWidthLimit);
        
    }

    glEnable(GL_CULL_FACE);

    glPrimitiveRestartIndex(Config::PRIMITIVE_RESTART_INDEX_S);
    glfwSwapInterval(par.getParam<bool>("runtime.enableVSync",false) ? 1 : 0);
    GLint numberOfDisplayModes;
    const GLFWvidmode* modes = glfwGetVideoModes(glfwGetPrimaryMonitor(), &numberOfDisplayModes );
    if(modes) PRINT_FN(Locale::get("AVAILABLE_VIDEO_MODES"),numberOfDisplayModes);

    //start background loading thread
    _closeLoadingThread = false;

    //OpenGL is up and ready
    Divide::GLUtil::_contextAvailable = true;

    for(U8 i = 0; i < PERFORMANCE_COUNTER_BUFFERS; ++i)
        for(U8 j = 0; j < PERFORMANCE_COUNTERS; ++j)
            _queryID[i][j] = 0;

#ifdef _DEBUG
    for (U8 i = 0; i < PERFORMANCE_COUNTER_BUFFERS; ++i){
        glGenQueries(PERFORMANCE_COUNTERS, _queryID[i]);
        assert(_queryID[i][0] != 0);
    }

    _queryBackBuffer = 4;
    glBeginQuery(GL_TIME_ELAPSED, _queryID[0][0]);
    glEndQuery(GL_TIME_ELAPSED);
    // wait until the results are available
    GLint stopTimerAvailable = 0;
    while (!stopTimerAvailable) {
        glGetQueryObjectiv(_queryID[0][0], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
    }
#endif

    for(GLuint index = 0; index < Config::MAX_CLIP_PLANES; ++index){
        _activeClipPlanes[index] = false;
    }

    if (textureBoundMap.empty()){
        for (U8 i = 0; i < 31; i++){
            //Set all 16 texture slots to 0
            textureBoundMap[i] = std::make_pair(i, GL_NONE);
        }
    }
    //That's it. Everything should be ready for draw calls
    PRINT_FN(Locale::get("START_OGL_API_OK"));

    //Create an immediate mode shader
    ShaderManager::getInstance().init(GFX_DEVICE._kernel);
    _imShader = ShaderManager::getInstance().getDefaultShader();

    _glimInterfaces.reserve(4); //<For Bounding boxes, Skeletons, Debug Axis and Nav Meshes

    CEGUI::OpenGL3Renderer& GUIGLrenderer = CEGUI::OpenGL3Renderer::create();

    GUIGLrenderer.enableExtraStateSettings(par.getParam<bool>("GUI.CEGUI.ExtraStates"));
    GUI::getInstance().bindRenderer(GUIGLrenderer);

     _fonsContext = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT);
    if (_fonsContext == nullptr) {
        ERROR_FN(Locale::get("ERROR_FONT_INIT"));
        return GLFW_WINDOW_INIT_ERROR;
    }
    NS_GLIM::glim.SetVertexAttribLocation(Divide::VERTEX_POSITION_LOCATION);

    glGenVertexArrays(1, &_pointDummyVAO);

    return 0;
}

void GL_API::exitRenderLoop(bool killCommand) {
    glfonsDelete(_fonsContext);
    _fonsContext = nullptr;
    Divide::GLUtil::_applicationClosing = true;
    glfwSetWindowShouldClose(Divide::GLUtil::_mainWindow, true);
    glfwSetWindowShouldClose(Divide::GLUtil::_loaderWindow, true);
}

///clear up stuff ...
void GL_API::closeRenderingApi(){
    Divide::GLUtil::_applicationClosing = true;
    glfwSetWindowShouldClose(Divide::GLUtil::_mainWindow, true);
    glfwSetWindowShouldClose(Divide::GLUtil::_loaderWindow, true);

    _closeLoadingThread = true;

    try { CEGUI::System::destroy();}
    catch( ... ){ D_ERROR_FN(Locale::get("ERROR_CEGUI_DESTROY")); }

    for(glIMPrimitive*& priv : _glimInterfaces){
        SAFE_DELETE(priv);
    }

    _fonts.clear();
    _glimInterfaces.clear(); //<Should call all destructors
    glfwDestroyWindow(Divide::GLUtil::_loaderWindow);
    boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    glfwDestroyWindow(Divide::GLUtil::_mainWindow);
    glfwTerminate();
}

void GL_API::initDevice(GLuint targetFrameRate){
    assert(_imShader != nullptr);
    _imShader->Uniform("tex",0);
    while (!glfwWindowShouldClose(Divide::GLUtil::_mainWindow)) {
        Kernel::mainLoopStatic();
    }
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

    if(getGPUVendor() == GPU_VENDOR_NVIDIA){ //nVidia specific
        glswAddDirectiveToken("","#pragma optionNV(fastmath on)");
        glswAddDirectiveToken("","#pragma optionNV(fastprecision on)");
        glswAddDirectiveToken("","#pragma optionNV(inline all)");
        glswAddDirectiveToken("","#pragma optionNV(ifcvt none)");
        glswAddDirectiveToken("","#pragma optionNV(strict on)");
        glswAddDirectiveToken("","#pragma optionNV(unroll all)");
    }

    glswAddDirectiveToken("", std::string("#define MAX_INSTANCES " + Util::toString(Config::MAX_INSTANCE_COUNT)).c_str());
    glswAddDirectiveToken("", std::string("#define MAX_CLIP_PLANES " + Util::toString(Config::MAX_CLIP_PLANES)).c_str());
    glswAddDirectiveToken("", std::string("#define MAX_LIGHTS_PER_NODE " + Util::toString(Config::MAX_LIGHTS_PER_SCENE_NODE)).c_str());
    glswAddDirectiveToken("", std::string("#define MAX_SHADOW_CASTING_LIGHTS " + Util::toString(Config::MAX_SHADOW_CASTING_LIGHTS_PER_NODE)).c_str());
    glswAddDirectiveToken("", std::string("#define MAX_SPLITS_PER_LIGHT " + Util::toString(Config::MAX_SPLITS_PER_LIGHT)).c_str());
    glswAddDirectiveToken("", std::string("const int MAX_LIGHTS_PER_SCENE = " + Util::toString(Config::MAX_LIGHTS_PER_SCENE) + ";").c_str());
    glswAddDirectiveToken("", std::string("#define SHADER_BUFFER_LIGHT_NORMAL " + Util::toString(Divide::SHADER_BUFFER_LIGHT_NORMAL)).c_str());
    glswAddDirectiveToken("", std::string("#define SHADER_BUFFER_CAM_MATRICES " + Util::toString(Divide::SHADER_BUFFER_CAM_MATRICES)).c_str());
    glswAddDirectiveToken("", "const float Z_TEST_SIGMA = 0.0001;");
    glswAddDirectiveToken("", "const float ALPHA_DISCARD_THRESHOLD = 0.1;");
    glswAddDirectiveToken("", "//__CUSTOM_DEFINES__");
    glswAddDirectiveToken("", "//__CUSTOM_UNIFORMS__");
    glswAddDirectiveToken("Fragment", std::string("#define SHADER_BUFFER_LIGHT_SHADOW " + Util::toString(Divide::SHADER_BUFFER_LIGHT_SHADOW)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_UNIT0 " + Util::toString(Material::TEXTURE_UNIT0 + 0)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_UNIT1 " + Util::toString(Material::TEXTURE_UNIT0 + 1)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_UNIT2 " + Util::toString(Material::TEXTURE_UNIT0 + 2)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_NORMALMAP " + Util::toString(Material::TEXTURE_NORMALMAP)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_OPACITY " + Util::toString(Material::TEXTURE_OPACITY)).c_str());
    glswAddDirectiveToken("Fragment", std::string("#define TEXTURE_SPECULAR " + Util::toString(Material::TEXTURE_SPECULAR)).c_str());

    glswAddDirectiveToken("Fragment", "const int DEPTH_EXP_WARP = 32;");
    
    // GLSL <-> VBO intercommunication 
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_POSITION_LOCATION) + ") in vec3  inVertexData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_COLOR_LOCATION) + ") in vec4  inColorData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_NORMAL_LOCATION) + ") in vec3  inNormalData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_TEXCOORD_LOCATION) + ") in vec2  inTexCoordData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_TANGENT_LOCATION) + ") in vec3  inTangentData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_BITANGENT_LOCATION) + ") in vec3  inBiTangentData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_BONE_WEIGHT_LOCATION) + ") in vec4  inBoneWeightData;").c_str());
    glswAddDirectiveToken("Vertex", std::string("layout(location = " + Util::toString(Divide::VERTEX_BONE_INDICE_LOCATION) + ") in ivec4 inBoneIndiceData;").c_str());
    GL_API::_GLSLOptContex = glslopt_initialize(GFX_DEVICE.getApi() == OpenGLES ? kGlslTargetOpenGLES30 : kGlslTargetOpenGL);

    return glswState == 1 && GL_API::_GLSLOptContex != nullptr;
}

bool GL_API::deInitShaders(){
    glslopt_cleanup(_GLSLOptContex);
    //Shut down glsw and clean memory
    return (glswShutdown() == 1);
}

void GL_API::changeResolutionInternal(GLushort w, GLushort h){
    glfwSetWindowSize(Divide::GLUtil::_mainWindow, w, h);
    _cachedResolution.width = w;
    _cachedResolution.height = h;
    //Inform the Kernel
    Kernel::updateResolutionCallback(w,h);
}

void GL_API::setWindowPos(GLushort w, GLushort h) const {
    glfwSetWindowPos(Divide::GLUtil::_mainWindow, w, h);
}

void GL_API::setMousePosition(GLushort x, GLushort y) const {
    glfwSetCursorPos(Divide::GLUtil::_mainWindow, (GLdouble)x, (GLdouble)y);
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

    F32 dx, dy, lh = 0;
    fonsClearState(_fonsContext);
    fonsSetSize(_fonsContext, textLabel._height);
    fonsSetFont(_fonsContext, font);
    fonsVertMetrics(_fonsContext, nullptr, nullptr, &lh);
    dx = position.x;
    dy = _cachedResolution.y - (position.y + lh);
    fonsSetColor(_fonsContext, textLabel._color.r * 255, textLabel._color.g * 255, textLabel._color.b * 255, textLabel._color.a * 255);
    if(textLabel._blurAmount > 0.01f){
        fonsSetBlur(_fonsContext, textLabel._blurAmount );
    }
    if(textLabel._spacing > 0.01f){
        fonsSetBlur(_fonsContext, textLabel._spacing );
    }
    if(textLabel._alignFlag != 0){
        fonsSetAlign(_fonsContext, textLabel._alignFlag);
    }
    fonsDrawText(_fonsContext, dx,dy,textLabel._text.c_str(),nullptr);
    GL_API::registerDrawCall();
}

void GL_API::idle(){
    glfwPollEvents();
}

void GL_API::loadInContextInternal(){
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    glfwMakeContextCurrent(Divide::GLUtil::_loaderWindow);
#ifdef GLEW_MX
    Divide::GLUtil::initGlew();
#   if defined(_DEBUG) || defined(_PROFILE)
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&Divide::GLUtil::DebugCallback, (GLvoid*)(1));
#   endif
#endif
    while(!_closeLoadingThread){
        if(GFX_DEVICE.getLoadQueue().empty()){
            boost::this_thread::sleep(boost::posix_time::milliseconds(20));//<Avoid burning the CPU - Ionut
            continue;
        }
        DELEGATE_CBK callback;
        while (GFX_DEVICE.getLoadQueue().pop(callback)){
            callback();
            glFlush();
        }
    }
}