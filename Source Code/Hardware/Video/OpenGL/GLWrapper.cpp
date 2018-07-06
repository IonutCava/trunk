#include "glResources.h"
#include "GLWrapper.h"
#include "glRenderStateBlock.h"
#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"
#include "GUI/Headers/GUIButton.h"
#include "GUI/Headers/GUIConsole.h"
#include "Core/Headers/Application.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/ImageTools.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/Headers/Frustum.h"

#define USE_FREEGLUT
bool _applicationClosing = false;
void GLCheckError(const std::string& File, U32 Line,char* operation) {
    // Get the last error
    GLenum ErrorCode = glGetError();
	while (ErrorCode != GL_NO_ERROR && !_applicationClosing) {

		ErrorCode = glGetError();

		if(ErrorCode == GL_NO_ERROR) return;
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
        ERROR_FN("An internal OpenGL call failed on call [ %s ]: [ %s ]",operation, ss.str().c_str());
	}
}

void resizeWindowCallback(I32 w, I32 h){
	GUI::getInstance().onResize(w,h);
	GFX_DEVICE.resizeWindow(w,h);
}

//Let's try and create a  valid OpenGL context.
//Due to university requirements, backwards compatibility with OpenGL 2.0 had to be added
void GL_API::initHardware(){
	ParamHandler& par = ParamHandler::getInstance();
	I32   argc   = 1; 
	//The Application's title can be set in the "config.xml" file, so no explanation here needed
	char *argv[] = {(char*)par.getParam<std::string>("appTitle").c_str(), NULL};
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
	_windowId = glutCreateWindow(par.getParam<std::string>("appTitle").c_str());
	Application::getInstance().setMainWindowId(_windowId);
	//Everything is set up as needed, so initialize the OpenGL API
	U32 err = glewInit();
	//Check for errors and print if any (They happen more than anyone thinks)
	//Bad drivers, old video card, corrupt GPU, anything can throw an error
	if (GLEW_OK != err){
		ERROR_FN("GFXDevice: %s \nTry switching to DX (version 9.0c required) or upgrade hardware.\nApplication will now exit!",glewGetErrorString(err));
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
	const GLubyte* glslVersionSupported = glGetString(GL_SHADING_LANGUAGE_VERSION);
	//Time to select our shaders.
	//We do not support OpenGL version lower than 2.0;
	if(major < 2){
		ERROR_FN("Your current hardware does not support the OpenGL 2.0 extension set!");
		PRINT_FN("Try switching to DX (version 9.0c required) or upgrade hardware.");
		PRINT_FN("Application will now exit!");
		exit(2);
	}else if(major == 2){
		//If we do start a 2.0 context, use only basic shaders
		par.setParam("shaderDetailToken",std::string("low"));
		setVersionId(OpenGL2x);
	}else if(major == 3 || major == 4){
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
		if(major == 3){
			setVersionId(OpenGL3x);
		}else{
			setVersionId(OpenGL4x);
		}
	}
	//Print all of the OpenGL functionality info to the console
	PRINT_FN("Max GLSL fragment uniform components supported: %d",max_frag_uniform);
	PRINT_FN("Max GLSL fragment varying floats supported: %d",max_varying_floats);
	PRINT_FN("Max GLSL vertex uniform components supported: %d",max_vertex_uniform);
	PRINT_FN("Max GLSL vertex attributes supported: %d",max_vertex_attrib);
	PRINT_FN("Max Combined Texture Units supported: %d",max_texture_units); 
	PRINT_FN("Hardware acceleration up to OpenGL %d.%d supported!",major,minor);
	PRINT_FN("GLSL version supported: [ %s ]",glslVersionSupported);
	GL_ENUM_TABLE::fill();
	//Set the clear color to a nice blue
	glClearColor(0.1f,0.1f,0.8f,1);
	//Smooth shading as GPU's are fast enough to support this as default
	glShadeModel(GL_SMOOTH);
	//keep normals in the [0..1] range despite scaling geometry
	glEnable(GL_NORMALIZE);
	//Correct texcoord generation during rasterization despite Perspective changes
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
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
	//That's it. Everything should be ready for draw calls
	PRINT_FN("OpenGL rendering system initialized!");
	
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
	RenderStateBlockDescriptor defaultGLStateDescriptor;
	GFX_DEVICE._defaultStateBlock = GFX_DEVICE.createStateBlock(defaultGLStateDescriptor);
	SET_DEFAULT_STATE_BLOCK();
}

void GL_API::closeApplication(){
	//glutLeaveMainLoop();
	_applicationClosing = true;
	Application::getInstance().killApplication();
	
}

///clear up stuff ...
void GL_API::closeRenderingApi(){
	SAFE_DELETE(_state2DRendering);
	glutDestroyWindow(Application::getInstance().getMainWindowId());
}

void GL_API::initDevice(U32 targetFPS){
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

//ToDo: convert to OpenGL 3.2 and GLSL 1.5 standards. No more matrix queries to GPU - Ionut
void GL_API::getProjectionMatrix(mat4<F32>& projMat){
	glGetFloatv( GL_PROJECTION_MATRIX, projMat.mat );	
}

void GL_API::getModelViewMatrix(mat4<F32>& mvMat){
	glGetFloatv( GL_MODELVIEW_MATRIX, mvMat.mat );	
}

void GL_API::clearBuffers(U8 buffer_mask){

	I32 buffers = 0;
	if((buffer_mask & GFXDevice::COLOR_BUFFER ) == GFXDevice::COLOR_BUFFER) buffers |= GL_COLOR_BUFFER_BIT;
	if((buffer_mask & GFXDevice::DEPTH_BUFFER) == GFXDevice::DEPTH_BUFFER) buffers |= GL_DEPTH_BUFFER_BIT;
	if((buffer_mask & GFXDevice::STENCIL_BUFFER) == GFXDevice::STENCIL_BUFFER) buffers |= GL_STENCIL_BUFFER_BIT;
	GLCheck(glClear(buffers));
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

void GL_API::drawTextToScreen(GUIElement* const textElement){
	GUIText* text = dynamic_cast<GUIText* >(textElement);
	assert(text != NULL);
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

void GL_API::drawFlash(GUIElement* const flash){
	assert(flash != NULL);
	dynamic_cast<GUIFlash* >(flash)->playMovie();
}

void GL_API::drawButton(GUIElement* const button){
	GUIButton* b = dynamic_cast<GUIButton* >(button);
	F32 fontx;
	F32 fonty;
	GUIText *t = NULL;	

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
			t = new GUIText(std::string("1"),b->_text,vec2<F32>(fontx,fonty),
							GLUT_BITMAP_HELVETICA_10,vec3<F32>(0,0,0));
		}
		t->_text = b->_text;
		if(b->_highlight){
			fontx--;
			fonty--;
			t->_color = vec3<F32>(0,0,0);
		}else{
			t->_color = vec3<F32>(1,1,1);
		}
		
		t->setPosition(vec2<F32>(fontx,fonty));
		drawTextToScreen(t);
		glPopAttrib();
	}
	SAFE_DELETE(t);

}
void GL_API::drawConsole(){
	GUIConsole& console = GUIConsole::getInstance();
	if(!console.isConsoleOpen()) return;

	ParamHandler& par = ParamHandler::getInstance();
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_SCISSOR_BIT | GL_TRANSFORM_BIT);
	setMaterial(console.getRenderQuad()->getMaterial());

	//get the width and heigtht of the viewport
	struct {GLint x, y, width, height;} viewport;
	glGetIntegerv(GL_VIEWPORT, &viewport.x);
	console.getViewportDimensions().x = viewport.width;
	console.getViewportDimensions().y = viewport.height;

	//reset matrices and switch to ortho view
						
	glMatrixMode(GL_PROJECTION);						
	glPushMatrix();										
	glLoadIdentity();									
	glOrtho(0,console.getViewportDimensions().x,0,console.getViewportDimensions().y,-1,1);							
				
		
	//set up a scissor region to draw the console in
	glScissor(1	,console.getViewportDimensions().y - console.getConsoleHeight(), //bottom coord
				console.getViewportDimensions().x, //width
				console.getViewportDimensions().y); //top coord
	glEnable(GL_SCISSOR_TEST);
		
	//render transparent background
	console.getRenderQuad()->setDimensions(vec4<F32>(0,0, console.getViewportDimensions().x - 1,
		                                                  console.getViewportDimensions().y - 1));
	GFX_DEVICE.renderModel(console.getRenderQuad());

	//draw text
	//RenderText();

	//restore old matrices and properties...	
			
	glPopMatrix();
	glPopAttrib();
}


void GL_API::toggleDepthMapRendering(bool state) {
	if(state){
		if(!_depthMapRendering){
			glShadeModel(GL_FLAT);
			_depthMapRendering = true;
		}
	}else{
		if(_depthMapRendering){
			glShadeModel(GL_SMOOTH);
			_depthMapRendering = false;
		}
	}
}

void GL_API::updateStateInternal(RenderStateBlock* block, bool force){
   assert(dynamic_cast<glRenderStateBlock*>(block));
   glRenderStateBlock* glBlock = static_cast<glRenderStateBlock*>(block);
   glRenderStateBlock* glCurrent = static_cast<glRenderStateBlock*>(GFX_DEVICE._currentStateBlock);
   if (force){
      glCurrent = NULL;
   }
   glBlock->activate(glCurrent);
   _currentGLRenderStateBlock = glBlock;
}

/// Applies matrix transformations to the vertices that are about to be drawn
/// If now shader is specified, fixed function multiplication are used for compatibility
void GL_API::setObjectState(Transform* const transform, bool force, ShaderProgram* const shader){
	if(transform){
		if(shader && shader->isBound()){
			shader->Uniform("transformMatrix",transform->getMatrix());
			shader->Uniform("parentTransformMatrix",transform->getParentMatrix());
		}
			
		glPushMatrix();
		if(force){
			glLoadIdentity();
		}
		glMultMatrixf(transform->getMatrix());
		glMultMatrixf(transform->getParentMatrix());
	}
}

void GL_API::releaseObjectState(Transform* const transform, ShaderProgram* const shader){
	if(transform){
		glPopMatrix();
	}
}

void GL_API::setMaterial(Material* mat){
	assert(mat != NULL);
	glColor4fv(&mat->getMaterialMatrix().getCol(1)[0]);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat->getMaterialMatrix().getCol(1));
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,mat->getMaterialMatrix().getCol(0));
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat->getMaterialMatrix().getCol(2));
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,mat->getMaterialMatrix().getCol(3).x);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,vec4<F32>(mat->getMaterialMatrix().getCol(3).y,mat->getMaterialMatrix().getCol(3).z,mat->getMaterialMatrix().getCol(3).w,1.0f));
}
 
void GL_API::renderInViewport(const vec4<F32>& rect, boost::function0<void> callback){
	
	glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(rect.x, rect.y, rect.z, rect.w);
	callback();
	glPopAttrib();

}

void GL_API::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset){

	glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);  
    glDisable(GL_LIGHTING);  
    glLineWidth(2.0f);
	glColor3f(0.0f,0.0f,1.0f);
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

void GL_API::drawLines(const std::vector<vec3<F32> >& pointsA,const std::vector<vec3<F32> >& pointsB,const std::vector<vec4<F32> >& colors, const mat4<F32>& globalOffset){
	/// We need a perfect pair of point A's to point B's
	if(pointsA.size() != pointsB.size()) return;
	/// We need a color for each line, even if it is the same one
	if(pointsA.size() != colors.size()) return;
	glPushMatrix();
	//glLoadIdentity();
	glMultMatrixf(globalOffset);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);  
    glDisable(GL_TEXTURE_2D);  
    glDisable(GL_LIGHTING);  
    glLineWidth(5.0f);
    glBegin(GL_LINES);
	for(U16 i = 0; i < pointsA.size(); i++){
		glColor4fv( &colors[i].r);
		glVertex3fv( &pointsA[i].x);
		glVertex3fv( &pointsB[i].x);
	}
    glEnd();
	glPopAttrib();
	glPopMatrix();
}

void GL_API::renderModel(Object3D* const model){
	PRIMITIVE_TYPE type = TRIANGLES;
	//render in the switch or after. hacky, but works -Ionut
	bool b_continue = true;
	switch(model->getType()){
		case TEXT_3D:{
			Text3D* text = dynamic_cast<Text3D*>(model);
			glPushAttrib(GL_ENABLE_BIT);	
			glLineWidth(text->getWidth());
			glutStrokeString(text->getFont(), (const U8*)text->getText().c_str());
			glPopAttrib();
			b_continue = false;
			}break;
		
		case BOX_3D:
		case SUBMESH:
		case GENERIC:
			type = TRIANGLES;
			break;
		case QUAD_3D:
		case SPHERE_3D:
			type = QUADS;
			break;
		//We should never enter the default case!
		default:
			ERROR_FN("GLWrapper: Invalid Object3D type received for object: [ %s ]",model->getName().c_str());
			b_continue = false;
			break;
	}

	if(b_continue){	

		VertexBufferObject* vbo = model->getGeometryVBO();
		assert(vbo != NULL);
		std::vector<U16>& hwIndices = vbo->getHWIndices();
		assert(!hwIndices.empty());
		vbo->Enable();
			GLCheck(glDrawRangeElements(type, (GLshort)model->getIndiceLimits().x, (GLshort)(model->getIndiceLimits().y + 1) /*half open*/ , (size_t)hwIndices.size(), GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)));
		vbo->Disable();
	}
}

void GL_API::renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element){
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
	GLCheck(glDrawElements(t, count, format, first_element ));
	
}

void GL_API::toggle2D(bool state){
	if(!_state2DRendering){
		RenderStateBlockDescriptor state2DRenderingDesc;
		state2DRenderingDesc.setCullMode(CULL_MODE_None);
		//state2DRenderingDesc.setZReadWrite(false,false);
		state2DRenderingDesc._fixedLighting = false;
		_state2DRendering = GFX_DEVICE.createStateBlock(state2DRenderingDesc);
	}

	if(state){ //2D
		SET_STATE_BLOCK(_state2DRendering);

		F32 width = Application::getInstance().getWindowDimensions().width;
		F32 height = Application::getInstance().getWindowDimensions().height;
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
	}
}

void GL_API::setAmbientLight(const vec4<F32>& light){
	GLCheck(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light));
}

///Update OpenGL light state
void GL_API::setLight(Light* const light){
	assert(light != NULL);
	U8 slot = light->getSlot();
	if(!light->getEnabled()){
	   glDisable(GL_LIGHT0 + light->getSlot());
      return;
   }
	if(slot >= 8){
		///We only have 8 slots, so replace
		slot = slot%8;
	}

	LIGHT_TYPE type = light->getLightType();
	glLightfv(GL_LIGHT0+slot, GL_POSITION, light->getVProperty(LIGHT_POSITION));
	glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  light->getVProperty(LIGHT_AMBIENT));
	glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  light->getVProperty(LIGHT_DIFFUSE));
	glLightfv(GL_LIGHT0+slot, GL_SPECULAR, light->getVProperty(LIGHT_SPECULAR));
	if(type == LIGHT_SPOT){
		glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, light->getVProperty(LIGHT_SPOT_DIRECTION));
		glLightf(GL_LIGHT0+slot, GL_SPOT_EXPONENT,light->getFProperty(LIGHT_SPOT_EXPONENT));
		glLightf(GL_LIGHT0+slot, GL_SPOT_CUTOFF, light->getFProperty(LIGHT_SPOT_CUTOFF));
	}
	if(type != LIGHT_DIRECTIONAL){
		glLightf(GL_LIGHT0+slot, GL_CONSTANT_ATTENUATION,light->getFProperty(LIGHT_CONST_ATT));
		glLightf(GL_LIGHT0+slot, GL_LINEAR_ATTENUATION,light->getFProperty(LIGHT_LIN_ATT));
		glLightf(GL_LIGHT0+slot, GL_QUADRATIC_ATTENUATION,light->getFProperty(LIGHT_QUAD_ATT));
	}
	glEnable(GL_LIGHT0+slot);
}


//Setting gluLookAt here for camera's or shadow projection
// -set the current matrix to GL_MODELVIEW
// -reset it to identity
// -invert the scene if needed by using, the now deprecated, I know, glScalef (but hey, so is gluLookAt)
void GL_API::lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up, bool invertx, bool inverty){
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
void GL_API::setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes){
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
void GL_API::setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes){
	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity ();
	gluPerspective(FoV, aspectRatio, planes.x, planes.y);
	glMatrixMode (GL_MODELVIEW);
}

//Save the area designated by the rectangle "rect" to a TGA file
void GL_API::Screenshot(char *filename, const vec4<F32>& rect){
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

	mat4<F32> nv_mvp;
	vec4<F32> transf;	
	
	// find the z-range of the current frustum as seen from the light
	// in order to increase precision
	glGetFloatv(GL_MODELVIEW_MATRIX, nv_mvp.mat);
	
	// note that only the z-component is need and thus
	// the multiplication can be simplified
	// transf.z = shad_modelview[2] * f.point[0].x + shad_modelview[6] * f.point[0].y + shad_modelview[10] * f.point[0].z + shad_modelview[14];
	transf = nv_mvp*vec4<F32>(f.point[0], 1.0f);
	minZ = transf.z;
	maxZ = transf.z;
	for(U8 i=1; i<8; i++){
		transf = nv_mvp*vec4<F32>(f.point[i], 1.0f);
		if(transf.z > maxZ) maxZ = transf.z;
		if(transf.z < minZ) minZ = transf.z;
	}
	// make sure all relevant shadow casters are included
	// note that these here are dummy objects at the edges of our scene
		//Unload every sub node recursively

	for_each(BoundingBox& b, sceneGraph->getBBoxes()){
		transf = nv_mvp*vec4<F32>(b.getCenter(), 1.0f);
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
		transf = nv_mvp*vec4<F32>(f.point[i], 1.0f);

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

RenderStateBlock* GL_API::newRenderStateBlock(const RenderStateBlockDescriptor& descriptor){
	return New glRenderStateBlock(descriptor);
}