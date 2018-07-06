#include "glResources.h"
#include "GLWrapper.h"
#include "Rendering/Application.h"
#include "Rendering/Frustum.h"
#include "Utility/Headers/Guardian.h"
#include "GUI/GUI.h"
#include "GUI/GuiFlash.h"
#include "Utility/Headers/ParamHandler.h"
#include "Utility/Headers/MathHelper.h"
#include "Managers/SceneManager.h"
#include "Managers/CameraManager.h"

#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Text3D.h"
using namespace std;

#define USE_FREEGLUT
void GLCheckError(const std::string& File, unsigned int Line)
{
    // Get the last error
    GLenum ErrorCode = glGetError();

    if (ErrorCode != GL_NO_ERROR)
    {
        std::string Error = "unknown error";
        std::string Desc  = "no description";

        // Decode the error code
        switch (ErrorCode)
        {
            case GL_INVALID_ENUM :
            {
                Error = "GL_INVALID_ENUM";
                Desc  = "an unacceptable value has been specified for an enumerated argument";
                break;
            }

            case GL_INVALID_VALUE :
            {
                Error = "GL_INVALID_VALUE";
                Desc  = "a numeric argument is out of range";
                break;
            }

            case GL_INVALID_OPERATION :
            {
                Error = "GL_INVALID_OPERATION";
                Desc  = "the specified operation is not allowed in the current state";
                break;
            }

            case GL_STACK_OVERFLOW :
            {
                Error = "GL_STACK_OVERFLOW";
                Desc  = "this command would cause a stack overflow";
                break;
            }

            case GL_STACK_UNDERFLOW :
            {
                Error = "GL_STACK_UNDERFLOW";
                Desc  = "this command would cause a stack underflow";
                break;
            }

            case GL_OUT_OF_MEMORY :
            {
                Error = "GL_OUT_OF_MEMORY";
                Desc  = "there is not enough memory left to execute the command";
                break;
            }

            case GL_INVALID_FRAMEBUFFER_OPERATION_EXT :
            {
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

void GL_API::initHardware(){
	I32   argc   = 1; 
	char *argv[] = {"DIVIDE Framework", NULL};
    glutInit(&argc, argv);
	//glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE/* |  GLUT_MULTISAMPLE*/);
	glutInitWindowSize(Application::getInstance().getWindowDimensions().width,Application::getInstance().getWindowDimensions().height);
	glutInitWindowPosition(10,50);
	_windowId = glutCreateWindow("DIVIDE Framework");
	Application::getInstance().setMainWindowId(_windowId);
	U32 err = glewInit();
	if (GLEW_OK != err){
		Console::getInstance().errorfn("GFXDevice: %s \nTry switching to DX (version 9.0c required) or upgrade hardware.\nApplication will now exit!",glewGetErrorString(err));
		exit(1);
	}


	I32 major = 0, minor = 0, max_frag_uniform = 0, max_varying_floats = 0;
	I32 max_vertex_uniform = 0, max_vertex_attrib = 0,max_texture_units = 0;

	glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_frag_uniform);
	glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);

	if(major < 2){
		Console::getInstance().errorfn("Your current hardware does not support the OpenGL 2.0 extension set!");
		Console::getInstance().printfn("Try switching to DX (version 9.0c required) or upgrade hardware.");
		Console::getInstance().printfn("Application will now exit!");
		exit(2);
	}
	Console::getInstance().printfn("Max GLSL fragment uniform components supported: %d",max_frag_uniform);
	Console::getInstance().printfn("Max GLSL fragment varying floats supported: %d",max_varying_floats);
	Console::getInstance().printfn("Max GLSL vertex uniform components supported: %d",max_vertex_uniform);
	Console::getInstance().printfn("Max GLSL vertex attributes supported: %d",max_vertex_attrib);
	Console::getInstance().printfn("Max Combined Texture Units supported: %d",max_texture_units); 
	Console::getInstance().printfn("Hardware acceleration up to OpenGL %d.%d supported!",major,minor);
	
	glClearColor(0.1f,0.1f,0.8f,0.2f);
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); 
	glShadeModel(GL_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_NORMALIZE);
	glDisable(GL_COLOR_MATERIAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glCullFace(GL_BACK);
	//glutCloseFunc(closeApplication);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutReshapeFunc(resizeWindowCallback);
	glutDisplayFunc(Application::getInstance().DrawSceneStatic);
	glutIdleFunc(Application::getInstance().Idle);
	_defaultRenderState.cullingEnabled() = true;
	_defaultRenderState.blendingEnabled() = false;
	_defaultRenderState.lightingEnabled() = false;
	_defaultRenderState.texturesEnabled() = true;
	GFXDevice::getInstance().setRenderState(_defaultRenderState,true);
	Console::getInstance().printfn("OpenGL rendering system initialized!");
/*
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
	Guardian::getInstance().TerminateApplication();
	glutDestroyWindow(Application::getInstance().getMainWindowId());
}

//clear up stuff ...
void GL_API::closeRenderingApi(){
	
}

void GL_API::initDevice(){
	//F32 global_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	glutMainLoop();
	closeApplication();
}
void GL_API::resizeWindow(U16 w, U16 h){

	F32 zNear  = ParamHandler::getInstance().getParam<F32>("zNear");
	F32 zFar   = ParamHandler::getInstance().getParam<F32>("zFar");
	F32 fov    = ParamHandler::getInstance().getParam<F32>("verticalFOV");
	F32 ratio  = ParamHandler::getInstance().getParam<F32>("aspectRatio");
	F32 x=0.0f,y=1.75f,z=5.0f;


	// Reset the coordinate system before modifying
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(fov,ratio,zNear,zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(x,   y,   z, 
		  	  x,   y,   z-1.0f,
			  0.0f,1.0f,0.0f);
}

void GL_API::lookAt(const vec3& eye,const vec3& center,const vec3& up, bool invertx, bool inverty)
{
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	if(invertx){
		glScalef(-1.0f,1.0f,1.0f);
	}
	gluLookAt(	eye.x,		eye.y,		eye.z,
				center.x,	center.y,	center.z,
				up.x,		up.y,		up.z	);
}

void GL_API::idle()
{
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

void GL_API::clearBuffers(U8 buffer_mask)
{
	I32 buffers = 0;
	if((buffer_mask & GFXDevice::COLOR_BUFFER ) == GFXDevice::COLOR_BUFFER) buffers |= GL_COLOR_BUFFER_BIT;
	if((buffer_mask & GFXDevice::DEPTH_BUFFER) == GFXDevice::DEPTH_BUFFER) buffers |= GL_DEPTH_BUFFER_BIT;
	if((buffer_mask & GFXDevice::STENCIL_BUFFER) == GFXDevice::STENCIL_BUFFER) buffers |= GL_STENCIL_BUFFER_BIT;
	glClear(buffers);
}

void GL_API::swapBuffers()
{
#ifdef USE_FREEGLUT
	glutSwapBuffers();
#else
	//nothing yet
#endif
}

void GL_API::enableFog(F32 density, F32* color)
{
	return; //Fog ... disabled. Really buggy - Ionut
	glFogi (GL_FOG_MODE, GL_EXP2); 
	glFogfv(GL_FOG_COLOR, color); 
	glFogf (GL_FOG_DENSITY, density); 
	glHint (GL_FOG_HINT, GL_NICEST); 
	glFogf (GL_FOG_START,  6700.0f);
	glFogf (GL_FOG_END,    7000.0f);
	glEnable(GL_FOG);
}

void GL_API::setOrthoProjection(const vec4& rect, const vec2& planes){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
	glMatrixMode(GL_MODELVIEW);
}

void GL_API::drawTextToScreen(Text* text){
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
}

void GL_API::drawCharacterToScreen(void* font,char text)
{
#ifdef USE_FREEGLUT
	glutBitmapCharacter(font, (I32)text);
#else
	//nothing yet
#endif
}

void GL_API::drawFlash(GuiFlash* flash)
{
	flash->playMovie();
}

void GL_API::drawButton(Button* b)
{

	F32 fontx;
	F32 fonty;
	Text *t = NULL;	

	if(b)
	{
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
			t = New Text(string("1"),b->_text,vec2(fontx,fonty),GLUT_BITMAP_HELVETICA_10,vec3(0,0,0));
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
void GL_API::setDepthMapRendering(bool state) {
	if(state){
		//glCullFace(GL_FRONT);
		if(_currentRenderState.lightingEnabled()) GLCheck(glDisable(GL_LIGHTING));
		if(_currentRenderState.cullingEnabled()) GLCheck(glDisable(GL_CULL_FACE));
	}else{
		//glCullFace(GL_BACK);
		if(_currentRenderState.lightingEnabled()) GLCheck(glEnable(GL_LIGHTING));
		if(_currentRenderState.cullingEnabled()) GLCheck(glEnable(GL_CULL_FACE));
	}
}

void GL_API::setRenderState(RenderState& state,bool force){

	if(_currentRenderState.blendingEnabled() != state.blendingEnabled() || force){
		state.blendingEnabled() ? GLCheck(glEnable(GL_BLEND)) : GLCheck(glDisable(GL_BLEND));
	}
	if(_currentRenderState.lightingEnabled() != state.lightingEnabled() || force){
		state.lightingEnabled() ? GLCheck(glEnable(GL_LIGHTING)) : GLCheck(glDisable(GL_LIGHTING));
	}
	if(_currentRenderState.cullingEnabled() != state.cullingEnabled() || force){
		state.cullingEnabled() ?  /*GLCheck(*/glEnable(GL_CULL_FACE)/*)*/ :	/*GLCheck(*/glDisable(GL_CULL_FACE)/*)*/;
	}
	if(_currentRenderState.texturesEnabled() != state.texturesEnabled() || force){
		state.texturesEnabled() ? GLCheck(glEnable(GL_TEXTURE_2D)) : GLCheck(glDisable(GL_TEXTURE_2D));
	}
}

void GL_API::ignoreStateChanges(bool state){
	if(state){
		//glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);
		glPushAttrib(GL_ALL_ATTRIB_BITS);
	}else{
		glPopAttrib();
	}
}

void GL_API::setObjectState(Transform* const transform){
	if(_wireframeRendering)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glPushMatrix();
	if(transform){
		glMultMatrixf(transform->getMatrix());
		glMultMatrixf(transform->getParentMatrix());
	}
}

void GL_API::releaseObjectState(){
	glPopMatrix();
}

void GL_API::setMaterial(Material* mat){
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat->getMaterialMatrix().getCol(1));
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,mat->getMaterialMatrix().getCol(0));
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat->getMaterialMatrix().getCol(2));
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,mat->getMaterialMatrix().getCol(3).x);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,vec4(mat->getMaterialMatrix().getCol(3).y,mat->getMaterialMatrix().getCol(3).z,mat->getMaterialMatrix().getCol(3).w,1.0f));
}
 
void GL_API::drawBox3D(vec3 min, vec3 max){

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

}
void GL_API::drawBox3D(Box3D* const box){
	SceneGraphNode temp(box);
	temp.useDefaultTransform(false);
	temp.silentDispose(true);
	drawBox3D(&temp);
}

void GL_API::drawSphere3D(Sphere3D* const sphere){
	SceneGraphNode temp(sphere);
	temp.useDefaultTransform(false);
	temp.silentDispose(true);
	drawSphere3D(&temp);
}

void GL_API::drawQuad3D(Quad3D* const quad){
	SceneGraphNode temp(quad);
	temp.useDefaultTransform(false);
	temp.silentDispose(true);
	drawQuad3D(&temp);
	
}

void GL_API::drawText3D(Text3D* const text){
	SceneGraphNode temp(text);
	temp.useDefaultTransform(false);
	temp.silentDispose(true);
	drawText3D(&temp);
}

void GL_API::drawBox3D(SceneGraphNode* node){
	Box3D* model = dynamic_cast<Box3D*>(node->getNode<Object3D>());
	setObjectState(node->getTransform());

	glutSolidCube(model->getSize());

	releaseObjectState();

}

void GL_API::drawSphere3D(SceneGraphNode* node){
	Sphere3D* model = dynamic_cast<Sphere3D*>(node->getNode<Object3D>());
	setObjectState(node->getTransform());
	
	glutSolidSphere(model->getSize(), model->getResolution(),model->getResolution());

	releaseObjectState();

}

void GL_API::drawQuad3D(SceneGraphNode* node){
	Quad3D* model = dynamic_cast<Quad3D*>(node->getNode<Object3D>());

	setObjectState(node->getTransform());

	glBegin(GL_TRIANGLE_STRIP); //GL_TRIANGLE_STRIP is slightly faster on newer HW than GL_QUAD,
								//as GL_QUAD converts into a GL_TRIANGLE_STRIP at the driver level anyway
		glNormal3f(0.0f, 1.0f, 0.0f);
		glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
		glVertex3fv(&model->getCorner(Quad3D::TOP_LEFT)[0]);
		glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
		glVertex3fv(&model->getCorner(Quad3D::TOP_RIGHT)[0]);
		glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
		glVertex3fv(&model->getCorner(Quad3D::BOTTOM_LEFT)[0]);
		glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
		glVertex3fv(&model->getCorner(Quad3D::BOTTOM_RIGHT)[0]);
	glEnd();

	releaseObjectState();

}

void GL_API::drawText3D(SceneGraphNode* node){
	Text3D* model = dynamic_cast<Text3D*>(node->getNode<Object3D>());

	setObjectState(node->getTransform());

	glPushAttrib(GL_ENABLE_BIT);
	GLCheck(glEnable(GL_LINE_SMOOTH));
	glLineWidth(model->getWidth());

	glutStrokeString(model->getFont(), (const U8*)model->getText().c_str());
	glPopAttrib();

	releaseObjectState();

}

void GL_API::renderModel(SceneGraphNode* node){

	SubMesh* s = dynamic_cast<SubMesh*>(node->getNode<Object3D>());
	setObjectState(node->getTransform());
	s->getGeometryVBO()->Enable();
		renderElements(TRIANGLES,_U32,s->getIndices().size(), &(s->getIndices()[0]));
	s->getGeometryVBO()->Disable();
	releaseObjectState();
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
	
	if(state){
		F32 width = Application::getInstance().getWindowDimensions().width;
		F32 height = Application::getInstance().getWindowDimensions().height;
		if(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE){
			_depthTestWasEnabled = true;
			GLCheck(glDisable(GL_DEPTH_TEST));
		}else{
			_depthTestWasEnabled = false;
		}
		GLCheck(glDisable(GL_LIGHTING));
		if(GFXDevice::getInstance().getDepthMapRendering()){
			GLCheck(glCullFace(GL_BACK));
		}
		glMatrixMode(GL_PROJECTION);
		glPushMatrix(); //1
		glLoadIdentity();
		glOrtho(0,width,height,0,-1,1);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix(); //2
		glLoadIdentity();

	}else{

		glPopMatrix(); //2 
		glMatrixMode(GL_PROJECTION);
		glPopMatrix(); //1
		if(_currentRenderState.lightingEnabled()) GLCheck(glEnable(GL_LIGHTING));
		if(GFXDevice::getInstance().getDepthMapRendering()){
			GLCheck(glCullFace(GL_FRONT));
		}
		if(glIsEnabled(GL_DEPTH_TEST) == GL_FALSE && _depthTestWasEnabled){
				//GLCheck(glEnable(GL_DEPTH_TEST));
			glEnable(GL_DEPTH_TEST);
		}
	}
}


void GL_API::setLight(U8 slot, unordered_map<string,vec4>& properties){
	if(_currentRenderState.lightingEnabled()){
		GLCheck(glEnable(GL_LIGHTING));
		GLCheck(glEnable(GL_LIGHT0+slot));
	}
	glLightfv(GL_LIGHT0+slot, GL_POSITION, properties["position"]);
	glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  properties["ambient"]);
	glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  properties["diffuse"]);
	glLightfv(GL_LIGHT0+slot, GL_SPECULAR, properties["specular"]);
	
	if( properties.size() == 5)	glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, properties["spotDirection"]);
	if(!_currentRenderState.lightingEnabled()) GLCheck(glDisable(GL_LIGHTING));
}

void GL_API::createLight(U8 slot){
	if(_currentRenderState.lightingEnabled()){
		GLCheck(glEnable(GL_LIGHTING));
		GLCheck(glEnable(GL_LIGHT0+slot));
	}
}

void GL_API::toggleWireframe(bool state){
	_wireframeRendering = state;
}

void GL_API::setLightCameraMatrices(const vec3& lightPosVector, const vec3& lightTargetVector,bool directional){
	if(!directional){
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluPerspective(90.0, 1.0, ParamHandler::getInstance().getParam<F32>("zNear"),ParamHandler::getInstance().getParam<F32>("zFar"));
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(	lightPosVector.x,		lightPosVector.y,		lightPosVector.z,
				lightTargetVector.x,	lightTargetVector.y,	lightTargetVector.z,
				0.0,					1.0,					0.0	);

	if(directional){
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
	}

}


void GL_API::restoreLightCameraMatrices(bool directional){
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void GL_API::Screenshot(char *filename, U16 xmin, U16 ymin, U16 xmax, U16 ymax){
	// compute width and heidth of the image
	U16 w = xmax - xmin;
	U16 h = ymax - ymin;

	// allocate memory for the pixels
	U8 *imageData = new U8[w * h * 4];

	// read the pixels from the frame buffer
	glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (void*)imageData);


//---------------SAVE SERIES-----------------------//
	static I8 savedImages=0;
	char *newFilename;
	
// compute the new filename by adding the 
// series number and the extension
	newFilename = new char[strlen(filename)+8];

	sprintf_s(newFilename,strlen(filename)+8,"%s%d.tga",filename,savedImages);
//----------------SAVE IMAGE----------------------------//
	U8 cGarbage = 0, type = 2 ,mode = 4,aux;
	I16 iGarbage = 0;
	I32 i;
	FILE *file;
	U8 pixelDepth = 32;
// open file and check for errors
	file = fopen(newFilename, "wb");
	if (file == NULL) {
		Console::getInstance().errorfn("Can't save screenshot to file [ %s ]",newFilename);
		return;
	}
// write the header
	fwrite(&cGarbage, sizeof(U8), 1, file);
	fwrite(&cGarbage, sizeof(U8), 1, file);

	fwrite(&type, sizeof(U8), 1, file);

	fwrite(&iGarbage, sizeof(I16), 1, file);
	fwrite(&iGarbage, sizeof(I16), 1, file);
	fwrite(&cGarbage, sizeof(U8), 1, file);
	fwrite(&iGarbage, sizeof(I16), 1, file);
	fwrite(&iGarbage, sizeof(I16), 1, file);

	fwrite(&w, sizeof(I16), 1, file);
	fwrite(&h, sizeof(I16), 1, file);
	fwrite(&pixelDepth, sizeof(U8), 1, file);

	fwrite(&cGarbage, sizeof(U8), 1, file);


	for (i=0; i < w * h * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

	fwrite(imageData, sizeof(U8), 
			w * h * mode, file);
	fclose(file);

	// release the memory
	delete imageData;
	imageData = NULL;

//----------------SAVE IMAGE----------------------------//

//increase the counter
	savedImages++;
	delete[] newFilename;
	newFilename = NULL;

//---------------SAVE SERIES-----------------------//
	delete[] imageData;
	imageData = NULL;
}