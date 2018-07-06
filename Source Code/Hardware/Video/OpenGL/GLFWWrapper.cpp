#include "Headers/glRenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Shaders/Headers/glLightUniformBufferObject.h"
#include "Shaders/Headers/glMatrixUniformBufferObject.h"
#include "Shaders/Headers/glMaterialUniformBufferObject.h"

#define FTGL_LIBRARY_STATIC
#include <FTGL/ftgl.h>

bool GL_API::_applicationClosing = false;
bool GL_API::_contextAvailable = false;
bool GL_API::_useDebugOutputCallback = false;

namespace GL_CALLBACK{
	I32 GLFWCALL windowCloseListener() {GL_API::_applicationClosing = true; return 0;}
	void windowResizeListener(I32 w, I32 h) {GFX_DEVICE.changeResolution(w,h);}
}

//Let's try and create a  valid OpenGL context.
GLbyte GL_API::initHardware(const vec2<GLushort>& resolution, I32 argc, char **argv){

	ParamHandler& par = ParamHandler::getInstance();
    if( !glfwInit() ){
		return GLFW_INIT_ERROR;
	}
	_projectionMatrix.push(mat4<GLfloat>());
	_modelViewMatrix.push(mat4<GLfloat>());

	//So, if someone selected High detail level, try to use pure 3.x API
	_msaaSamples = par.getParam<U8>("rendering.FSAAsamples",2);
	_useMSAA = (par.getParam<U8>("rendering.FSAAmethod",FS_MSAA) == FS_MSAA || 
				par.getParam<U8>("rendering.FSAAmethod",FS_MSAA) == FS_MSwFXAA);
	if(_msaaSamples > 1 && _useMSAA){
		glfwOpenWindowHint(GLFW_FSAA_SAMPLES, _msaaSamples);
	}
	if(par.getParam<bool>("rendering.overrideRefreshRate",false)){
		glfwOpenWindowHint(GLFW_REFRESH_RATE,par.getParam<U8>("rendering.targetRefreshRate",75));
	}
	GLint glMinorVer = static_cast<GLint>(par.getParam<GLubyte>("runtime.GLminorVer",1));
	glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE,!par.getParam<bool>("runtime.allowWindowResize",true));
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR,3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR,glMinorVer);
#ifdef _DEBUG
	glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);
#endif
	if(glMinorVer > 1 && par.getParam<bool>("runtime.useGLCompatProfile",true)){
		glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	}
	//Store the main window ID for future reference
	// Open an OpenGL window; resolution is specified in the external XML files
	bool window = par.getParam<bool>("runtime.windowedMode",true);
	///32Bit RGBA (R8G8B8A8), 24bit Depth, 8bit Stencil
	if( !glfwOpenWindow( resolution.width, resolution.height, 8,8,8,8,24,8, window ? GLFW_WINDOW : GLFW_FULLSCREEN) ){
		glfwTerminate();
		return( GLFW_WINDOW_INIT_ERROR );
	}
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
		return GLEW_INIT_ERROR;
	}
#ifdef _DEBUG
	if(GLEW_ARB_debug_output) {
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageCallbackARB(&Divide::GL::DebugCallbackARB, NULL);
		///Disable shader compiler errors (shader class handles that)
		glDebugMessageControlARB(GL_DEBUG_SOURCE_SHADER_COMPILER_ARB, 
			                     GL_DEBUG_TYPE_ERROR_ARB,
								 GL_DONT_CARE,
								 0,
								 NULL,
								 GL_FALSE);
		_useDebugOutputCallback = true;
	}else if(GLEW_AMD_debug_output){
		glDebugMessageCallbackAMD(&Divide::GL::DebugCallbackAMD, NULL);
		glDebugMessageEnableAMD(GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD, 
								GL_DONT_CARE,
								0,
								NULL,
								GL_FALSE);
		_useDebugOutputCallback = true;
	}else{
		_useDebugOutputCallback = false;
	}
#endif
	
	glfwSetWindowTitle(par.getParam<std::string>("appTitle").c_str());
	GLFWvidmode return_struct;
	glfwGetDesktopMode( &return_struct );
	I32 height = return_struct.Height;
	I32 width = return_struct.Width;
	glfwSetWindowPos((width - resolution.width)*0.5f,(height - resolution.height)*0.5f);	
	//Our Resize/Draw/Idle callback setup
	glfwSetWindowSizeCallback(GL_CALLBACK::windowResizeListener);
	glfwSetWindowCloseCallback(GL_CALLBACK::windowCloseListener);
#if defined( __WIN32__ ) || defined( _WIN32 )
	_hwnd = FindWindow(NULL,par.getParam<std::string>("appTitle").c_str());
	_hdc = GetDC(_hwnd);
#elif defined( __APPLE_CC__ ) // Apple OS X
///??
#else //Linux
	_dpy = XOpenDisplay();
	_drawable = XCreateWindow( ...
#endif
	GL_API::_contextAvailable = true;
	//If we got here, let's figure out what capabilities we have available
	GLint max_frag_uniform = 0, max_varying_floats = 0;
	GLint max_vertex_uniform = 0, max_vertex_attrib = 0,max_texture_units = 0;
	GLint buffers = 0,samplesEffective = 0;
	GLint major = 0, minor = 0;
	GLint maxAnisotrophy = 0;
	GLCheck(glGetIntegerv(GL_MAJOR_VERSION, &major)); // OpenGL major version
    GLCheck(glGetIntegerv(GL_MINOR_VERSION, &minor)); // OpenGL minor version 
	GLCheck(glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers));
	GLCheck(glGetIntegerv(GL_SAMPLES, &samplesEffective));
	GLCheck(glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_frag_uniform)); //How many uniforms can we send to fragment shaders
	GLCheck(glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats)); //How many varying floats can we use inside a shader program
	GLCheck(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform)); //How many uniforms can we send to vertex shaders
	GLCheck(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib)); //How many attributes can we send to a vertex shader
	GLCheck(glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units)); //Maximum number of texture units we can address in shaders
	GLCheck(glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotrophy));//Maximum supporter anisotropic filtering level
	const GLubyte* glslVersionSupported;
	GLCheck(glslVersionSupported = glGetString(GL_SHADING_LANGUAGE_VERSION));
	std::string gpuVendorByte(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	if(!gpuVendorByte.empty()){
		if(gpuVendorByte.compare(0,5,"Intel") == 0) { setGPUVendor(GPU_VENDOR_INTEL); }
		else if(gpuVendorByte.compare(0,6,"NVIDIA") == 0) { setGPUVendor(GPU_VENDOR_NVIDIA); }
		else if (gpuVendorByte.compare(0,3,"ATI") == 0) { setGPUVendor(GPU_VENDOR_AMD); }
	}else{
		gpuVendorByte = "Unknown GPU Vendor";
	}
	//Cap max aniso to what the hardware supports
	if(maxAnisotrophy < par.getParam<U8>("rendering.anisotropicFilteringLevel",1)){
		par.setParam("rendering.anisotropicFilteringLevel",maxAnisotrophy);
	}
	//Time to select our shaders.
	//We do not support OpenGL version lower than 3.0;
	if(major < 3){
		ERROR_FN(Locale::get("ERROR_OPENGL_NOT_SUPPORTED"));
		PRINT_FN(Locale::get("WARN_SWITCH_D3D"));
		PRINT_FN(Locale::get("WARN_APPLICATION_CLOSE"));
		return GLEW_OLD_HARDWARE;
	}else {
		//OpenGL 3.1 introduced v140 for GLSL, so use a maximum of "medium" shaders
		//some "high" shaders need v150
		if(minor < 2){
			//If settings are set to low, use "low" shaders else cap at "medium" level
			par.setParam("shaderDetailToken",(par.getParam<GLubyte>("rendering.detailLevel") == DETAIL_LOW) ? DETAIL_LOW : DETAIL_MEDIUM);
		}else{	//If we got here, full OpenGL 3.2/3.3 is supported so use full shaders
			//Same shader selection based on detail level as above
			par.setParam("shaderDetailToken",par.getParam<GLubyte>("rendering.detailLevel"));
		}
		major == 3 ? setVersionId(OpenGL3x) : setVersionId(OpenGL4x); 
	}
	par.setParam("GFX_DEVICE.maxTextureCombinedUnits",max_texture_units);
	//Print all of the OpenGL functionality info to the console
	PRINT_FN(Locale::get("GL_MAX_UNIFORM"),max_frag_uniform);
	PRINT_FN(Locale::get("GL_MAX_FRAG_VARYING"),max_varying_floats);
	PRINT_FN(Locale::get("GL_MAX_VERT_UNIFORM"),max_vertex_uniform);
	PRINT_FN(Locale::get("GL_MAX_VERT_ATTRIB"),max_vertex_attrib);
	PRINT_FN(Locale::get("GL_MAX_TEX_UNITS"),max_texture_units); 
	PRINT_FN(Locale::get("GL_MAX_VERSION"),major,minor);
	PRINT_FN(Locale::get("GL_GLSL_SUPPORT"),glslVersionSupported);
	PRINT_FN(Locale::get("GL_VENDOR_STRING"),gpuVendorByte.c_str(), glGetString(GL_RENDERER));
	PRINT_FN(Locale::get("GL_MULTI_SAMPLE_INFO"),samplesEffective,buffers);
	GL_ENUM_TABLE::fill();
	//Set the clear color to a nice blue
	GLCheck(glClearColor(0.1f,0.1f,0.8f,1));
	//Smooth shading as GPU's are fast enough to support this as default
	//glShadeModel(GL_SMOOTH);
	if(glewIsSupported("GL_ARB_seamless_cube_map")){ 
		GLCheck(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
	}
	//keep normals in the [0..1] range despite scaling geometry
	GLCheck(glEnable(GL_NORMALIZE));
	if(_msaaSamples > 1 && _useMSAA){
		GLCheck(glEnable(GL_MULTISAMPLE));
	}
	//Correct texcoord generation during rasterization despite Perspective changes
	GLCheck(glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST));
	//Fog detail
	switch(par.getParam<U8>("rendering.fogDetailLevel", 2)){
		case 0: glHint (GL_FOG_HINT, GL_FASTEST); break;
		case 1: glHint (GL_FOG_HINT, GL_DONT_CARE); break;
		default: 
		case 2: glHint (GL_FOG_HINT, GL_NICEST); break;
	}
	//MipMap detail
	switch(par.getParam<U8>("rendering.mipMapDetailLevel", 2)){
		case 0: glHint (GL_GENERATE_MIPMAP_HINT, GL_FASTEST); break;
		case 1: glHint (GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE); break;
		default: 
		case 2: glHint (GL_GENERATE_MIPMAP_HINT, GL_NICEST); break;
	}
	
	//We can't use LINE_SMOOTH do to our MRT style rendering and Deferred Shading if need be
	//glEnable(GL_LINE_SMOOTH);
	//This isn't needed due to the line found after this
	glfwSwapInterval(par.getParam<bool>("runtime.enableVSync",false) ? 1 : 0);
	
	I32 numberOfDisplayModes = glfwGetVideoModes(_modes, 20 );
	PRINT_FN(Locale::get("AVAILABLE_VIDEO_MODES"),numberOfDisplayModes);
/*
	_lightUBO = New glLightUniformBufferObject();
	_materialsUBO = New glMaterialUniformBufferObject();
	_transformsUBO = New glMatrixUniformBufferObject();
*/
	//That's it. Everything should be ready for draw calls
	PRINT_FN(Locale::get("START_OGL_API_OK"));
	
	RenderStateBlockDescriptor defaultGLStateDescriptor;
	GFX_DEVICE._defaultStateBlock = GFX_DEVICE.createStateBlock(defaultGLStateDescriptor);
	SET_DEFAULT_STATE_BLOCK();
    GLCheck(glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE));
	//Create an immediate mode shader
	ResourceDescriptor immediateModeShader("ImmediateModeEmulation");
	_imShader = CreateResource<ShaderProgram>(immediateModeShader);

//Create a second context to aid in loading
#if defined( __WIN32__ ) || defined( _WIN32 )
	_loaderContext = wglCreateContext(_hdc);
	assert(_loaderContext != NULL);
	wglShareLists(_loaderContext, wglGetCurrentContext());
#elif defined( __APPLE_CC__ ) // Apple OS X
///??
#else //Linux
	_loaderContext = glXCreateContext(_display, _visual, glXGetCurrentContext(), TRUE); 
#endif
	
	CEGUI::OpenGLRenderer& renderer = CEGUI::OpenGLRenderer::create();
	renderer.enableExtraStateSettings(par.getParam<bool>("GUI.CEGUI.ExtraStates"));
	GUI::getInstance().bindRenderer(renderer);
	return 0;
}

///clear up stuff ...
void GL_API::closeRenderingApi(){
	SAFE_DELETE(_state2DRendering);
	///Clean font cache
	for_each(FontCache::value_type it, _fonts){
		SAFE_DELETE(it.second);
	}
	for_each(FontCache::value_type it, _3DFonts){
		SAFE_DELETE(it.second);
	}
	for_each(IMPrimitive* priv, _glimInterfaces){
		SAFE_DELETE(priv);
	}
	//SAFE_DELETE(_lightUBO);
	//SAFE_DELETE(_materialsUBO);
	//SAFE_DELETE(_transformsUBO);
	_fonts.clear();
	_3DFonts.clear();
	_glimInterfaces.clear(); //<Should call all destructors
}

void GL_API::initDevice(GLuint targetFrameRate){
	assert(_imShader != NULL);
	while(!_applicationClosing) { 
		Kernel::MainLoopStatic(); 
	}
		
///Delete secondary context
#if defined( __WIN32__ ) || defined( _WIN32 )
	wglDeleteContext(_loaderContext);
#elif defined( __APPLE_CC__ ) // Apple OS X
///??
#else //Linux
	glXDestroyContext(_loaderContext)
#endif
	glfwTerminate();
}

void GL_API::setWindowSize(U16 w, U16 h) {
	glfwSetWindowSize(w,h);
}

void GL_API::setWindowPos(U16 w, U16 h){
	glfwSetWindowPos(w,h);
}

void GL_API::changeResolution(GLushort w, GLushort h){
	ParamHandler& par = ParamHandler::getInstance();
	GLfloat zNear  = par.getParam<GLfloat>("runtime.zNear");
	GLfloat zFar   = par.getParam<GLfloat>("runtime.zFar");
	GLfloat fov    = par.getParam<GLfloat>("runtime.verticalFOV");
	GLfloat ratio  = par.getParam<GLfloat>("runtime.aspectRatio");

	// Reset the coordinate system before modifying
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	// Set the viewport to be the entire window
    GLCheck(glViewport(0, 0, w, h));

	// Set the clipping volume
	_projectionMatrix.top() = Divide::GL::_perspective(fov,ratio,zNear,zFar);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	_modelViewMatrix.top().identity(); ///glLoadIdentity();
	_cachedResolution.width = w;
	_cachedResolution.height = h;
	///Update view frustum
	Frustum::getInstance().setZPlanes(vec2<GLfloat>(zNear, zFar));
	///Inform the Kernel
	Kernel::updateResolutionCallback(w,h);
}


void GL_API::idle(){
}

void GL_API::swapBuffers(){
	glfwSwapBuffers();
}


#if defined( __WIN32__ ) || defined( _WIN32 )

bool GL_API::loadInContext(const CurrentContext& context, boost::function0<GLvoid> callback) {
	if(callback.empty()) return false;
	if(context == GFX_LOADING_CONTEXT){
		 boost::thread(&GL_API::loadInContextInternal, this, context, callback);
	}else{
		callback();
	}
	return true;
}

void GL_API::loadInContextInternal(const CurrentContext& context, boost::function0<GLvoid> callback){
	wglMakeCurrent(_hdc, _loaderContext);
	callback();
	glFlush();
	wglMakeCurrent(NULL, NULL);
}

#elif defined( __APPLE_CC__ ) // Apple OS X
///??
#else //Linux
bool GL_API::loadInContext(const CurrentContext& context, boost::function0<GLvoid> callback) {
	if(callback.empty()) return false;
	if(context == GFX_LOADING_CONTEXT){
		 boost::thread(&GL_API::loadInContextInternal, this, context, callback);
	}else{
		callback();
	}
	return loadInContextInternal(context,callback);
}

bool GL_API::loadInContextInternal(const CurrentContext& context, boost::function0<GLvoid> callback ){
	boost::thread([=] () {
		glxMakeCurrent(_dpy,_drawable, +loaderContext);
		callback();
		glFlush();
		glxMakeCurrent(nullptr, nullptr);
		});
	return true;
}
#endif