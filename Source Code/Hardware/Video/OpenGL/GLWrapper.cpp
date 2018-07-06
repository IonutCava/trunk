#include "glResources.h"
#include "GLWrapper.h"
#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIFlash.h"
#include "Utility/Headers/Guardian.h"
#include "Core/Headers/Application.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"
using namespace std;

#define USE_FREEGLUT
void GLCheckError(const std::string& File, U32 Line) {

    // Get the last error
    GLenum ErrorCode = glGetError();

    if (ErrorCode != GL_NO_ERROR)  {
        std::string Error = "unknown error";
        std::string Desc  = "no description";

        // Decode the error code
        switch (ErrorCode) {

            case GL_INVALID_ENUM : {

                Error = "GL_INVALID_ENUM";
                Desc  = "an unacceptable value has been specified for an enumerated argument";
                break;
            }

            case GL_INVALID_VALUE : {

                Error = "GL_INVALID_VALUE";
                Desc  = "a numeric argument is out of range";
                break;
            }

            case GL_INVALID_OPERATION : {

                Error = "GL_INVALID_OPERATION";
                Desc  = "the specified operation is not allowed in the current state";
                break;
            }

            case GL_STACK_OVERFLOW : {

                Error = "GL_STACK_OVERFLOW";
                Desc  = "this command would cause a stack overflow";
                break;
            }

            case GL_STACK_UNDERFLOW : {

                Error = "GL_STACK_UNDERFLOW";
                Desc  = "this command would cause a stack underflow";
                break;
            }

            case GL_OUT_OF_MEMORY : {

                Error = "GL_OUT_OF_MEMORY";
                Desc  = "there is not enough memory left to execute the command";
                break;
            }

            case GL_INVALID_FRAMEBUFFER_OPERATION_EXT : {

                Error = "GL_INVALID_FRAMEBUFFER_OPERATION_EXT";
                Desc  = "the object bound to FRAMEBUFFER_BINDING_EXT is not \"framebuffer complete\"";
                break;
            }
        }
		
		std::stringstream ss;
        ss << File.substr(File.find_last_of("\\/") + 1) << " (" << Line << ") : "
           << Error << ", " 
		   << Desc;
        Console::getInstance().errorfn("An internal OpenGL call failed: [ %s ]", ss.str().c_str());
    }
}

void resizeWindowCallback(I32 w, I32 h){
	GUI::getInstance().onResize(w,h);
	GFXDevice::getInstance().resizeWindow(w,h);
}

//Let's try and create a  valid OpenGL context.
//Due to university requirements, backwards compatibility with OpenGL 2.0 had to be added
void GL_API::initHardware(){
	ParamHandler& par = ParamHandler::getInstance();
	I32   argc   = 1; 
	//The Application's title can be set in the "config.xml" file, so no explanation here needed
	char *argv[] = {(char*)par.getParam<string>("appTitle").c_str(), NULL};
	//Using freeglut, because ... why not? Crossplatform window creation in a single line? Sure
    glutInit(&argc, argv);
	//So, if someone selected High detail level, try to use pure 3.x API
	if(par.getParam<U8>("detailLevel") == HIGH){
		glutInitContextProfile(GLUT_CORE_PROFILE);
	}else{
		//Else, they probably don't support 3.x so try a compatibility mode to run 2.x
		glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
	}
	//Standard stuff: RGBA window mode, Double buffering (triple from drivers, no problem)
	//Also, the depth map is a must!
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE/* |  GLUT_MULTISAMPLE*/);
	//No comments needed. Window dimensions are specified in the external XML files
	glutInitWindowSize(Application::getInstance().getWindowDimensions().width,Application::getInstance().getWindowDimensions().height);
	//Try to offset the window position just slightly. Found it useful
	glutInitWindowPosition(10,50);
	//For a posibile multi-window support (as seen the the OBJ PhysX Simulator video
	//Store the main window ID for future reference
	_windowId = glutCreateWindow(par.getParam<string>("appTitle").c_str());
	Application::getInstance().setMainWindowId(_windowId);
	//Everything is set up as needed, so initialize the OpenGL API
	U32 err = glewInit();
	//Check for errors and print if any (They happen more than anyone thinks)
	//Bad drivers, old video card, corrupt GPU, anything can throw an error
	if (GLEW_OK != err){
		Console::getInstance().errorfn("GFXDevice: %s \nTry switching to DX (version 9.0c required) or upgrade hardware.\nApplication will now exit!",glewGetErrorString(err));
		//No need to continue, as switching to DX should be set before launching the application!
		exit(1);
	}
	//If we got here, let's figure out what capabilities we have available
	I32 major = 0, minor = 0, max_frag_uniform = 0, max_varying_floats = 0;
	I32 max_vertex_uniform = 0, max_vertex_attrib = 0,max_texture_units = 0;

	glGetIntegerv(GL_MAJOR_VERSION, &major); // OpenGL major version
    glGetIntegerv(GL_MINOR_VERSION, &minor); // OpenGL minor version 
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_frag_uniform); //How many uniforms can we send to fragment shaders
	glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats); //How many varying floats can we use inside a shader program
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform); //How many uniforms can we send to vertex shaders
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib); //How many attributes can we send to a vertex shader
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units); //Maximum number of texture units we can address in shaders
	//Time to select our shaders.
	//We do not support OpenGL version lower than 2.0;
	if(major < 2){
		Console::getInstance().errorfn("Your current hardware does not support the OpenGL 2.0 extension set!");
		Console::getInstance().printfn("Try switching to DX (version 9.0c required) or upgrade hardware.");
		Console::getInstance().printfn("Application will now exit!");
		exit(2);
	}else if(major == 2){
		//If we do start a 2.0 context, use only basic shaders
		par.setParam("shaderDetailToken",std::string("low"));
	}else{
		//OpenGL 3.1 introduced v140 for GLSL, so use a maximum of "medium" shaders
		//some "high" shaders need v150
		if(minor < 2){
			//If settings are set to low, use "low" shaders
			switch(par.getParam<U8>("detailLevel")){
				case LOW:
					par.setParam("shaderDetailToken",std::string("low"));
				break;
				//cap at "medium" level
				case MEDIUM:
					par.setParam("shaderDetailToken",std::string("medium"));
				break;

			};
		//If we got here, full OpenGL 3.2/3.3 is supported so use full shaders
		}else{
			//Same shader selection based on detail level as above
			switch(par.getParam<U8>("detailLevel")){
				case LOW:
					par.setParam("shaderDetailToken",std::string("low"));
				break;
				case MEDIUM:
					par.setParam("shaderDetailToken",std::string("medium"));
				break;
				case HIGH:
					par.setParam("shaderDetailToken",std::string("high"));
				break;
			};
		}
		
	}
	//Print all of the OpenGL functionality info to the console
	Console::getInstance().printfn("Max GLSL fragment uniform components supported: %d",max_frag_uniform);
	Console::getInstance().printfn("Max GLSL fragment varying floats supported: %d",max_varying_floats);
	Console::getInstance().printfn("Max GLSL vertex uniform components supported: %d",max_vertex_uniform);
	Console::getInstance().printfn("Max GLSL vertex attributes supported: %d",max_vertex_attrib);
	Console::getInstance().printfn("Max Combined Texture Units supported: %d",max_texture_units); 
	Console::getInstance().printfn("Hardware acceleration up to OpenGL %d.%d supported!",major,minor);
	//Set the clear color to a nice blue
	glClearColor(0.1f,0.1f,0.8f,1);
	//Use depth testing!
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); 
	//Smooth shading as GPU's are fast enough to support this as default
	glShadeModel(GL_SMOOTH);
	//A global blend function (good enough for fences, water, grass) for now
	//This will be moved to RenderState's if needed
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//keep normals in the [0..1] range despite scaling geometry
	glEnable(GL_NORMALIZE);
	//Don't use color materials, as we set all parameters via the Material class
	glDisable(GL_COLOR_MATERIAL);
	//Correct texcoord generation during rasterization despite Perspective changes
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	//Cull back faces. If needed, change this via the RenderState component in an object's Material
	glCullFace(GL_BACK);
	//We can't use LINE_SMOOTH do to our MRT style rendering and Deferred Shading if need be
	//glEnable(GL_LINE_SMOOTH);
	//This isn't needed due to the line found after this
	//glutCloseFunc(closeApplication);
	//If we end the render loop, continue processing the engine code found after (glutMainLoop)
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	//Our Resize/Draw/Idle callback setup
	glutReshapeFunc(resizeWindowCallback);
	glutDisplayFunc(Application::getInstance().DrawSceneStatic);
	glutIdleFunc(Application::getInstance().Idle);
	//Set a default render state for all objects. These global values are quite good for our needs
	_defaultRenderState.cullingEnabled() = true;
	_defaultRenderState.blendingEnabled() = false;
	_defaultRenderState.lightingEnabled() = false;
	_defaultRenderState.texturesEnabled() = true;
	GFXDevice::getInstance().setRenderState(_defaultRenderState,true);
	//That's it. Everything should be ready for draw calls
	Console::getInstance().printfn("OpenGL rendering system initialized!");
	
/*  //The following code should enable vSync
	int (*SwapInterval)(int);

	// main init code :
	SwapInterval = getProcAddress("glXSwapInterval");
	if (!SwapInterval)
	SwapInterval = getProcAddress("glXSwapIntervalEXT");
	if (!SwapInterval)
	SwapInterval = getProcAddress("glXSwapIntervalSGI");
	if (!SwapInterval)
	SwapInterval = getProcAddress("wglSwapInterval");
	if (!SwapInterval)
	SwapInterval = getProcAddress("wglSwapIntervalEXT");
	if (!SwapInterval)
	SwapInterval = getProcAddress("wglSwapIntervalSGI");
	// actual vsync activation
	SwapInterval(1);

	// no vsync, if needed :
	SwapInterval(0);
*/
}

void GL_API::closeApplication(){
	//glutLeaveMainLoop();
	Guardian::getInstance().TerminateApplication();
	
}

//clear up stuff ...
void GL_API::closeRenderingApi(){
	glutDestroyWindow(Application::getInstance().getMainWindowId());
}

void GL_API::initDevice(){
	glutMainLoop();
	closeApplication();
}

void GL_API::resizeWindow(U16 w, U16 h){
	ParamHandler& par = ParamHandler::getInstance();
	F32 zNear  = par.getParam<F32>("zNear");
	F32 zFar   = par.getParam<F32>("zFar");
	F32 fov    = par.getParam<F32>("verticalFOV");
	F32 ratio  = par.getParam<F32>("aspectRatio");

	// Reset the coordinate system before modifying
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(fov,ratio,zNear,zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}


void GL_API::idle(){

	glutSetWindow(_windowId); 
	glutPostRedisplay();
}

//ToDo: convert to OpenGL 3.3 and GLSL 1.5 standards. No more matrix queries to GPU - Ionut
void GL_API::getProjectionMatrix(mat4& projMat){
	glGetFloatv( GL_PROJECTION_MATRIX, projMat.mat );	
}

void GL_API::getModelViewMatrix(mat4& mvMat){
	glGetFloatv( GL_MODELVIEW_MATRIX, mvMat.mat );	
}

void GL_API::clearBuffers(U8 buffer_mask){

	I32 buffers = 0;
	if((buffer_mask & GFXDevice::COLOR_BUFFER ) == GFXDevice::COLOR_BUFFER) buffers |= GL_COLOR_BUFFER_BIT;
	if((buffer_mask & GFXDevice::DEPTH_BUFFER) == GFXDevice::DEPTH_BUFFER) buffers |= GL_DEPTH_BUFFER_BIT;
	if((buffer_mask & GFXDevice::STENCIL_BUFFER) == GFXDevice::STENCIL_BUFFER) buffers |= GL_STENCIL_BUFFER_BIT;
	glClear(buffers);
}

void GL_API::swapBuffers(){

#ifdef USE_FREEGLUT
	glutSwapBuffers();
#else
	//nothing yet
#endif
}

void GL_API::enableFog(F32 density, F32* color){
	glFogi (GL_FOG_MODE, GL_EXP2); 
	glFogfv(GL_FOG_COLOR, color); 
	glFogf (GL_FOG_DENSITY, density); 
	glHint (GL_FOG_HINT, GL_NICEST); 
	glFogf (GL_FOG_START,  3100.0f);
	glFogf (GL_FOG_END,    8000.0f);
}

void GL_API::drawTextToScreen(Text* text){
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushMatrix();
	glLoadIdentity();
	glColor3f(text->_color.x,text->_color.y,text->_color.z);
	glRasterPos2f(text->getPosition().x,text->getPosition().y);
		
#ifdef USE_FREEGLUT
		glutBitmapString(text->_font, (U8*)(text->_text.c_str()));
#else
		//nothing yet
#endif
	glPopMatrix();
	glColor3i(1,1,1);
	glPopAttrib();
}

void GL_API::drawCharacterToScreen(void* font,char text){

#ifdef USE_FREEGLUT
	glutBitmapCharacter(font, (I32)text);
#else
	//nothing yet
#endif
}

void GL_API::drawFlash(GuiFlash* flash){

	flash->playMovie();
}

void GL_API::drawButton(Button* b){
	
	F32 fontx;
	F32 fonty;
	Text *t = NULL;	

	if(b){
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		/*
		 *	We will indicate that the mouse cursor is over the button by changing its
		 *	colour.
		 */
		if(!b->isVisible()) return;
		if (b->_highlight) {
			glColor3f(b->_color.r + 0.1f,b->_color.g + 0.1f,b->_color.b + 0.2f);
		}else {
			glColor3f(b->_color.r,b->_color.g,b->_color.b);
		}

		/*
		 *	draw background for the button.
		 */
		glBegin(GL_QUADS);
			glVertex2f( b->getPosition().x     , b->getPosition().y      );
			glVertex2f( b->getPosition().x     , b->getPosition().y+b->_dimensions.y );
			glVertex2f( b->getPosition().x+b->_dimensions.x, b->getPosition().y+b->_dimensions.y );
			glVertex2f( b->getPosition().x+b->_dimensions.x, b->getPosition().y      );
		glEnd();

		/*
		 *	Draw an outline around the button with width 3
		 */
		glLineWidth(3);

		/*
		 *	The colours for the outline are reversed when the button. 
		 */
		if (b->_pressed && b->isActive()) {
			glColor3f((b->_color.r + 0.1f)/2.0f,(b->_color.g+ 0.1f)/2.0f,(b->_color.b+ 0.1f)/2.0f);
		}else {
			glColor3f(b->_color.r + 0.1f,b->_color.g+ 0.1f,b->_color.b+ 0.1f);
		}

		glBegin(GL_LINE_STRIP);
			glVertex2f( b->getPosition().x+b->_dimensions.x, b->getPosition().y      );
			glVertex2f( b->getPosition().x     , b->getPosition().y      );
			glVertex2f( b->getPosition().x     , b->getPosition().y+b->_dimensions.y );
		glEnd();

		if(b->_pressed) {
			glColor3f(0.8f,0.8f,0.8f);
		}else {
			glColor3f(0.4f,0.4f,0.4f);
		}

		glBegin(GL_LINE_STRIP);
			glVertex2f( b->getPosition().x     , b->getPosition().y+b->_dimensions.y );
			glVertex2f( b->getPosition().x+b->_dimensions.x, b->getPosition().y+b->_dimensions.y );
			glVertex2f( b->getPosition().x+b->_dimensions.x, b->getPosition().y      );
		glEnd();

		glLineWidth(1);
		fontx = b->getPosition().x + (b->_dimensions.x - glutBitmapLength(GLUT_BITMAP_HELVETICA_10,(U8*)b->_text.c_str())) / 2 ;
		fonty = b->getPosition().y + (b->_dimensions.y+10)/2;

		/*
		 *	if the button is pressed, make it look as though the string has been pushed
		 *	down. It's just a visual thing to help with the overall look....
		 */
		if (b->_pressed) {
			fontx+=2;
			fonty+=2;
		}

		/*
		 *	If the cursor is currently over the button we offset the text string and draw a shadow
		 */
		if(!t){/* delete t;*/
			t = new Text(string("1"),b->_text,vec2(fontx,fonty),GLUT_BITMAP_HELVETICA_10,vec3(0,0,0));
		}
		t->_text = b->_text;
		if(b->_highlight){
			fontx--;
			fonty--;
			t->_color = vec3(0,0,0);
		}else{
			t->_color = vec3(1,1,1);
		}
		
		t->setPosition(vec2(fontx,fonty));
		drawTextToScreen(t);
		glPopAttrib();
	}
	delete t;
	t = NULL;
}

void GL_API::toggleDepthMapRendering(bool state) {
	if(state){
		GLCheck(glCullFace(GL_FRONT));
		glShadeModel(GL_FLAT);
		glColorMask(0, 0, 0, 0);
		glPolygonOffset( 1.0f, 4096.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
		if(_currentRenderState.lightingEnabled()) GLCheck(glDisable(GL_LIGHTING));
	}else{
		GLCheck(glCullFace(GL_BACK));
		glShadeModel(GL_SMOOTH);
		glColorMask(1, 1, 1, 1);
		glDisable(GL_POLYGON_OFFSET_FILL);
		if(_currentRenderState.lightingEnabled()) GLCheck(glEnable(GL_LIGHTING));
	}
}

void GL_API::setRenderState(RenderState& state,bool force){

	if(_currentRenderState.blendingEnabled() != state.blendingEnabled() || force || _ignoreStates){
		state.blendingEnabled() ? GLCheck(glEnable(GL_BLEND)) : GLCheck(glDisable(GL_BLEND));
	}
	if(_currentRenderState.lightingEnabled() != state.lightingEnabled() || force || _ignoreStates){
		state.lightingEnabled() ? GLCheck(glEnable(GL_LIGHTING)) : GLCheck(glDisable(GL_LIGHTING));
	}
	if(_currentRenderState.cullingEnabled() != state.cullingEnabled() || force || _ignoreStates){
		state.cullingEnabled() ?  /*GLCheck(*/glEnable(GL_CULL_FACE)/*)*/ :	/*GLCheck(*/glDisable(GL_CULL_FACE)/*)*/;
	}
	if(_currentRenderState.texturesEnabled() != state.texturesEnabled() || force || _ignoreStates){
		state.texturesEnabled() ? GLCheck(glEnable(GL_TEXTURE_2D)) : GLCheck(glDisable(GL_TEXTURE_2D));
	}
}

void GL_API::ignoreStateChanges(bool state){
	if(state){
		glPushAttrib(GL_ALL_ATTRIB_BITS);
	}else{
		glPopAttrib();
	}
	_ignoreStates = state;
}

void GL_API::setObjectState(Transform* const transform){
	if(_wireframeRendering)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if(transform){
		//ToDo: Create separate shaders for this!!!! -Ionut 
		//Performance is abismal
		glPushMatrix();
		glMultMatrixf(transform->getMatrix());
		glMultMatrixf(transform->getParentMatrix());
	}
}

void GL_API::releaseObjectState(Transform* const transform){
	if(transform) glPopMatrix();
}

void GL_API::setMaterial(Material* mat){
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat->getMaterialMatrix().getCol(1));
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,mat->getMaterialMatrix().getCol(0));
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat->getMaterialMatrix().getCol(2));
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,mat->getMaterialMatrix().getCol(3).x);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,vec4(mat->getMaterialMatrix().getCol(3).y,mat->getMaterialMatrix().getCol(3).z,mat->getMaterialMatrix().getCol(3).w,1.0f));
}
 
void GL_API::renderInViewport(const vec4& rect, boost::function0<void> callback){
	
	glPushAttrib(GL_VIEWPORT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
    glViewport(rect.x, rect.y, rect.z, rect.w);
	callback();
	glPopAttrib();

}

void GL_API::drawBox3D(vec3 min, vec3 max){
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glColor3i(0,0,0);
	glBegin(GL_LINE_LOOP);
		glVertex3f( min.x, min.y, min.z );
		glVertex3f( max.x, min.y, min.z );
		glVertex3f( max.x, min.y, max.z );
		glVertex3f( min.x, min.y, max.z );
	glEnd();

	glBegin(GL_LINE_LOOP);
		glVertex3f( min.x, max.y, min.z );
		glVertex3f( max.x, max.y, min.z );
		glVertex3f( max.x, max.y, max.z );
		glVertex3f( min.x, max.y, max.z );
	glEnd();

	glBegin(GL_LINES);
		glVertex3f( min.x, min.y, min.z );
		glVertex3f( min.x, max.y, min.z );
		glVertex3f( max.x, min.y, min.z );
		glVertex3f( max.x, max.y, min.z );
		glVertex3f( max.x, min.y, max.z );
		glVertex3f( max.x, max.y, max.z );
		glVertex3f( min.x, min.y, max.z );
		glVertex3f( min.x, max.y, max.z );
	glEnd();
	glPopAttrib();
}

void GL_API::renderModel(Object3D* const model){
	Type type = TRIANGLES;
	//render in the switch or after. hacky, but works -Ionut
	bool b_continue = true;
	switch(model->getType()){
		case TEXT_3D:{
			Text3D* text = dynamic_cast<Text3D*>(model);
			glPushAttrib(GL_ENABLE_BIT);	
			GLCheck(glEnable(GL_LINE_SMOOTH));
			glLineWidth(text->getWidth());
			glutStrokeString(text->getFont(), (const U8*)text->getText().c_str());
			glPopAttrib();
			b_continue = false;
			}break;
		
		case BOX_3D:
		case SUBMESH:
			type = TRIANGLES;
			break;
		case QUAD_3D:
		case SPHERE_3D:
			type = QUADS;
			break;
		//We should never enter the default case!
		default:
			Console::getInstance().errorfn("GLWrapper: Invalid Object3D type received for object: [ %s ]",model->getName().c_str());
			b_continue = false;
			break;
	}

	if(b_continue){	
		model->getGeometryVBO()->Enable();
			renderElements(type,_U16,model->getIndices().size(), &(model->getIndices()[0]));
		model->getGeometryVBO()->Disable();
	}
}

void GL_API::renderElements(Type t, Format f, U32 count, const void* first_element){
	U32 format = 0;
	switch(f){
		case _I8:
			format = GL_BYTE;
			break;
		case _I16:
			format = GL_SHORT;
			break;
		case _I32:
			format = GL_INT;
			break;
		case _U8:
			format = GL_UNSIGNED_BYTE;
			break;
		case _U16:
			format = GL_UNSIGNED_SHORT;
			break;
		default:
		case _U32:
			format = GL_UNSIGNED_INT;
			break;
	}
	glDrawElements(t, count, format, first_element );
}

bool _depthTestWasEnabled = false;
void GL_API::toggle2D(bool state){
	
	if(state){ //2D
		F32 width = Application::getInstance().getWindowDimensions().width;
		F32 height = Application::getInstance().getWindowDimensions().height;
		if(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE){
			_depthTestWasEnabled = true;
			GLCheck(glDisable(GL_DEPTH_TEST));
		}else{
			_depthTestWasEnabled = false;
		}
		glMatrixMode(GL_PROJECTION);
		glPushMatrix(); //1
		glLoadIdentity();
		glOrtho(0,width,height,0,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix(); //2
		glLoadIdentity();

	}else{ //3D
		glPopMatrix(); //2 
		glMatrixMode(GL_PROJECTION);
		glPopMatrix(); //1
		if(glIsEnabled(GL_DEPTH_TEST) == GL_FALSE && _depthTestWasEnabled){
			glEnable(GL_DEPTH_TEST);
		}
	}
}

void GL_API::setAmbientLight(const vec4& light){
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light);
}

void GL_API::setLight(U8 slot, unordered_map<std::string,vec4>& properties_v,unordered_map<std::string,F32>& properties_f, LIGHT_TYPE type){
	glLightfv(GL_LIGHT0+slot, GL_POSITION, properties_v["position"]);
	glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  properties_v["ambient"]);
	glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  properties_v["diffuse"]);
	glLightfv(GL_LIGHT0+slot, GL_SPECULAR, properties_v["specular"]);
	if(type == LIGHT_SPOT){
		glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, properties_v["spotDirection"]);
		glLightf(GL_LIGHT0+slot, GL_SPOT_EXPONENT,properties_f["spotExponent"]);
		glLightf(GL_LIGHT0+slot, GL_SPOT_CUTOFF, properties_f["spotCutoff"]);
	}
	if(type != LIGHT_DIRECTIONAL){
		glLightf(GL_LIGHT0+slot, GL_CONSTANT_ATTENUATION,properties_f["constAtt"]);
		glLightf(GL_LIGHT0+slot, GL_LINEAR_ATTENUATION,properties_f["linearAtt"]);
		glLightf(GL_LIGHT0+slot, GL_QUADRATIC_ATTENUATION,properties_f["quadAtt"]);
	}
}


void GL_API::toggleWireframe(bool state){
	_wireframeRendering = state;
}

//Setting gluLookAt here for camera's or shadow projection
// -set the current matrix to GL_MODELVIEW
// -reset it to identity
// -invert the scene if needed by using, the now deprecated, I know, glScalef (but hey, so is gluLookAt)
void GL_API::lookAt(const vec3& eye,const vec3& center,const vec3& up, bool invertx, bool inverty){

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	if(invertx){
		glScalef(-1.0f,1.0f,1.0f);
	}
	gluLookAt(	eye.x,		eye.y,		eye.z,
				center.x,	center.y,	center.z,
				up.x,		up.y,		up.z	);
}

void GL_API::lockProjection(){
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
}

void GL_API::releaseProjection(){
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void GL_API::lockModelView(){
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
}

void GL_API::releaseModelView(){
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

//Setting ortho projection:
// -sets the current matrix to GL_PROJECTION
// -resets it to identity
// -sets an ortho perspective based on the input rect and limits
// -and sets the matrix mode back to GL_MODELVIEW
void GL_API::setOrthoProjection(const vec4& rect, const vec2& planes){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
	glMatrixMode(GL_MODELVIEW);
}

//Setting perspective projection:
// -sets the current matrix to GL_PRJECTION
// -resets it to identity
// -sets a perspective projection based on the input FoV, aspect ration and limits
// -and sets the matrix mode back to GL_MODELVIEW
void GL_API::setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2& planes){
	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity ();
	gluPerspective(FoV, aspectRatio, planes.x, planes.y);
	glMatrixMode (GL_MODELVIEW);
}

//Save the area designated by the rectangle "rect" to a TGA file
void GL_API::Screenshot(char *filename, const vec4& rect){
	// compute width and heidth of the image
	U16 w = rect.z - rect.x; //maxX - minX
	U16 h = rect.w - rect.y; //maxY - minY

	// allocate memory for the pixels
	U8 *imageData = New U8[w * h * 4];
	// read the pixels from the frame buffer
	glReadPixels(rect.x,rect.y,rect.z,rect.w,GL_RGBA,GL_UNSIGNED_BYTE, (void*)imageData);
	//save to file
	ImageTools::SaveSeries(filename,w,h,32,imageData);
}

// this function builds a projection matrix for rendering from the shadow's POV.
// First, it computes the appropriate z-range and sets an orthogonal projection.
// Then, it translates and scales it, so that it exactly captures the bounding box
// of the current frustum slice
F32 GL_API::applyCropMatrix(frustum &f,SceneGraph* sceneGraph){
	F32 shad_modelview[16];
	F32 shad_proj[16];
	F32 maxX = -1000.0f;
    F32 maxY = -1000.0f;
	F32 maxZ;
    F32 minX =  1000.0f;
    F32 minY =  1000.0f;
	F32 minZ;

	mat4 nv_mvp;
	vec4 transf;	
	
	// find the z-range of the current frustum as seen from the light
	// in order to increase precision
	glGetFloatv(GL_MODELVIEW_MATRIX, nv_mvp.mat);
	
	// note that only the z-component is need and thus
	// the multiplication can be simplified
	// transf.z = shad_modelview[2] * f.point[0].x + shad_modelview[6] * f.point[0].y + shad_modelview[10] * f.point[0].z + shad_modelview[14];
	transf = nv_mvp*vec4(f.point[0], 1.0f);
	minZ = transf.z;
	maxZ = transf.z;
	for(U8 i=1; i<8; i++){
		transf = nv_mvp*vec4(f.point[i], 1.0f);
		if(transf.z > maxZ) maxZ = transf.z;
		if(transf.z < minZ) minZ = transf.z;
	}
	// make sure all relevant shadow casters are included
	// note that these here are dummy objects at the edges of our scene
		//Unload every sub node recursively

	for_each(BoundingBox& b, sceneGraph->getBBoxes()){
		transf = nv_mvp*vec4(b.getCenter(), 1.0f);
		if(transf.z + b.getHalfExtent().z > maxZ) maxZ = transf.z + b.getHalfExtent().z;
	//	if(transf.z - b.radius < minZ) minZ = transf.z - b.radius;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set the projection matrix with the new z-bounds
	// note the inversion because the light looks at the neg. z axis
	// gluPerspective(LIGHT_FOV, 1.0, maxZ, minZ); // for point lights
	glOrtho(-1.0, 1.0, -1.0, 1.0, -maxZ, -minZ);
	glGetFloatv(GL_PROJECTION_MATRIX, shad_proj);
	glPushMatrix();
	glMultMatrixf(shad_modelview);
	glGetFloatv(GL_PROJECTION_MATRIX, nv_mvp.mat);
	glPopMatrix();

	// find the extends of the frustum slice as projected in light's homogeneous coordinates
	for(U8 i=0; i<8; i++){
		transf = nv_mvp*vec4(f.point[i], 1.0f);

		transf.x /= transf.w;
		transf.y /= transf.w;

		if(transf.x > maxX) maxX = transf.x;
		if(transf.x < minX) minX = transf.x;
		if(transf.y > maxY) maxY = transf.y;
		if(transf.y < minY) minY = transf.y;
	}

	F32 scaleX = 2.0f/(maxX - minX);
	F32 scaleY = 2.0f/(maxY - minY);
	F32 offsetX = -0.5f*(maxX + minX)*scaleX;
	F32 offsetY = -0.5f*(maxY + minY)*scaleY;

	// apply a crop matrix to modify the projection matrix we got from glOrtho.
	nv_mvp.identity();
	nv_mvp.element(0,0) = scaleX;
	nv_mvp.element(1,1) = scaleY;
	nv_mvp.element(0,3) = offsetX;
	nv_mvp.element(1,3) = offsetY;
	nv_mvp.transpose();
	glLoadMatrixf(nv_mvp.mat);
	glMultMatrixf(shad_proj);

	return minZ;
}