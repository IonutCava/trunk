#include "Headers/glImmediateModeEmulation.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Kernel.h"
#include "core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include <glim.h>

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
        void printGLInitError(GLuint err) {
            ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),glewGetErrorString(err));
            PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
            PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));
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
                printGLInitError(err);
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
                printGLInitError(err);
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
                printGLInitError(err);
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

#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
#if defined(_DEBUG)
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
#endif
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
#endif
    
    if(GFX_DEVICE.MSAAEnabled())
        glfwWindowHint(GLFW_SAMPLES, GFX_DEVICE.MSAASamples());

    if (getId() == OpenGLES) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    }else{
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    }

    glfwWindowHint(GLFW_RESIZABLE, par.getParam<bool>("runtime.allowWindowResize",false));
    ///32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
    glfwWindowHint(GLFW_RED_BITS,8);
    glfwWindowHint(GLFW_GREEN_BITS,8);
    glfwWindowHint(GLFW_BLUE_BITS,8);
    glfwWindowHint(GLFW_ALPHA_BITS,8);
    glfwWindowHint(GLFW_DEPTH_BITS,24);
    glfwWindowHint(GLFW_STENCIL_BITS,8);
    
    //Store the main window ID for future reference
    // Open an OpenGL window; resolution is specified in the external XML files
    bool window = par.getParam<bool>("runtime.windowedMode", true);
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
    if(!GLEW_VERSION_4_3){
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE"),"The OpenGL version supported by the current GPU is too old!");
        return GLEW_OLD_HARDWARE;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    Divide::GLUtil::_loaderWindow = glfwCreateWindow(1, 1, "divide-res-loader", nullptr, Divide::GLUtil::_mainWindow);
    if (!Divide::GLUtil::_loaderWindow){
        glfwTerminate();
        return( GLFW_WINDOW_INIT_ERROR );
    }

#if defined(_DEBUG) || defined(_PROFILE) || defined(_GLDEBUG_IN_RELEASE)
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
    GLint maxAnisotropy = 0, maxTextureSlots = 0;
    GLint maxSubroutines = 0, maxSubroutineUniforms = 0;
    GLint maxUBOBindings = 0, maxUBOBlockSize = 0, UBOalignment = 0;
    GLint maxSSBOBindings = 0, maxSSBOBlockSize = 0, maxSSBOCombinedBlocks = 0, SSBOalignment = 0;
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
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UBOalignment);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &maxSSBOCombinedBlocks);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBOBindings);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOBlockSize);
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &SSBOalignment);
    glGetIntegerv(GL_MAX_SUBROUTINES, &maxSubroutines);
    glGetIntegerv(GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS, &maxSubroutineUniforms);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureSlots);
    GFX_DEVICE.setMaxTextureSlots(maxTextureSlots);
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
    par.setParam("shaderDetailToken", par.getParam<GLubyte>("rendering.detailLevel"));
    par.setParam("GFX_DEVICE.maxTextureCombinedUnits", max_texture_units);
    //Cap max aniso to what the hardware supports
    if (maxAnisotropy < par.getParam<GLint>("rendering.anisotropicFilteringLevel", 1)){
        par.setParam("rendering.anisotropicFilteringLevel", maxAnisotropy);
    }
    //Time to select our shaders.
    //We do not support OpenGL version lower than 4.0;
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
    PRINT_FN(Locale::get("GL_MAX_TEX_UNITS"), max_texture_units, maxTextureSlots);
    PRINT_FN(Locale::get("GL_MAX_VERSION"),major,maxMinor);
    PRINT_FN(Locale::get("GL_GLSL_SUPPORT"),glslVersionSupported);
    PRINT_FN(Locale::get("GL_VENDOR_STRING"), gpuVendorByte.c_str(), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    PRINT_FN(Locale::get("GL_MULTI_SAMPLE_INFO"),samplesEffective,buffers);
    PRINT_FN(Locale::get("GL_UBO_INFO"), maxUBOBindings, maxUBOBlockSize / 1024, UBOalignment);
    PRINT_FN(Locale::get("GL_SSBO_INFO"), maxSSBOBindings, (maxSSBOBlockSize / 1024) / 1024, maxSSBOCombinedBlocks, SSBOalignment);
    PRINT_FN(Locale::get("GL_SUBROUTINE_INFO"), maxSubroutines, maxSubroutineUniforms);
        
    GL_ENUM_TABLE::fill();

    //Set the clear color to a nice blue
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE(), 0);

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
        DIVIDE_ASSERT(_queryID[i][0] != 0, "GLFWWrapper error: Invalid performance counter query ID!");
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

    //That's it. Everything should be ready for draw calls
    PRINT_FN(Locale::get("START_OGL_API_OK"));

    CEGUI::OpenGL3Renderer& GUIGLrenderer = CEGUI::OpenGL3Renderer::create();

    GUIGLrenderer.enableExtraStateSettings(par.getParam<bool>("GUI.CEGUI.ExtraStates"));
    GUI::getInstance().bindRenderer(GUIGLrenderer);

    createFonsContext();  

    if (_fonsContext == nullptr) {
        ERROR_FN(Locale::get("ERROR_FONT_INIT"));
        return GLFW_WINDOW_INIT_ERROR;
    }
    NS_GLIM::glim.SetVertexAttribLocation(Divide::VERTEX_POSITION_LOCATION);

    glGenVertexArrays(1, &_pointDummyVAO);

    return 0;
}

void GL_API::exitRenderLoop(bool killCommand) {
    deleteFonsContext();
   
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

    FOR_EACH(samplerObjectMap::value_type& it, _samplerMap)
        SAFE_DELETE(it.second);
    
    _samplerMap.clear();

    _fonts.clear();

    glfwDestroyWindow(Divide::GLUtil::_loaderWindow);
    boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    glfwDestroyWindow(Divide::GLUtil::_mainWindow);
    glfwTerminate();
}

void GL_API::initDevice(GLuint targetFrameRate){
    while (!glfwWindowShouldClose(Divide::GLUtil::_mainWindow))
        Kernel::mainLoopStatic();
}

void GL_API::changeResolutionInternal(GLushort w, GLushort h) {
    glfwSetWindowSize(Divide::GLUtil::_mainWindow, w, h);
    _cachedResolution.set(w, h);
}

void GL_API::setWindowPos(GLushort w, GLushort h) const {
    glfwSetWindowPos(Divide::GLUtil::_mainWindow, w, h);
}

void GL_API::setMousePosition(GLushort x, GLushort y) const {
    glfwSetCursorPos(Divide::GLUtil::_mainWindow, (GLdouble)x, (GLdouble)y);
}

void GL_API::idle() {
    glfwPollEvents();
}

void GL_API::loadInContextInternal() {
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    assert(Divide::GLUtil::_loaderWindow != nullptr);
    glfwMakeContextCurrent(Divide::GLUtil::_loaderWindow);
#ifdef GLEW_MX
    Divide::GLUtil::initGlew();
#   if defined(_DEBUG) || defined(_PROFILE)
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&Divide::GLUtil::DebugCallback, (GLvoid*)(1));
#   endif
#endif
    _loaderThreadAvailable = true;
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
    _loaderThreadAvailable = false;
}