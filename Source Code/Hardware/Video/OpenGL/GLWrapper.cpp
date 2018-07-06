#include "glResources.h"

#include "GLWrapper.h"
#include "Rendering/common.h"
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

void resizeWindowCallback(I32 w, I32 h){
	GUI::getInstance().onResize(w,h);
	GFXDevice::getInstance().resizeWindow(w,h);
}

void GL_API::initHardware(){
	I32   argc   = 1; 
	char *argv[] = {"DIVIDE Engine", NULL};
    glutInit(&argc, argv);
	//glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
	glutInitWindowSize(Engine::getInstance().getWindowDimensions().width,Engine::getInstance().getWindowDimensions().height);
	glutInitWindowPosition(10,50);
	Engine::getInstance().setMainWindowId(glutCreateWindow("DIVIDE Engine"));
	U32 err = glewInit();
	if (GLEW_OK != err){
		Console::getInstance().errorfn("GFXDevice: %s \nTry switching to DX (version 9.0c required) or upgrade hardware.\nApplication will now exit!",glewGetErrorString(err));
		exit(1);
	}


	I32 major = 0, minor = 0, max_frag_uniform = 0, max_varying_floats = 0,max_vertex_uniform = 0, max_vertex_attrib = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_frag_uniform);
	glGetIntegerv(GL_MAX_VARYING_FLOATS, &max_varying_floats);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_vertex_uniform);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attrib);

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
	Console::getInstance().printfn("Hardware acceleration up to OpenGL %d.%d supported!",major,minor);
	
	glClearColor(0.1f,0.1f,0.8f,0.2f);
	glDepthFunc(GL_LEQUAL); 
    glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_NORMALIZE);
	glDisable(GL_COLOR_MATERIAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glutCloseFunc(Guardian::getInstance().TerminateApplication);
	glutReshapeFunc(resizeWindowCallback);
	glutDisplayFunc(Engine::getInstance().DrawSceneStatic);
	glutIdleFunc(Engine::getInstance().Idle);
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

void GL_API::closeRenderingApi()
{
	glutLeaveMainLoop();
	glutDestroyWindow(Engine::getInstance().getMainWindowId());
}

void GL_API::resizeWindow(U16 w, U16 h)
{
	D32 zNear  = 0.01f;
	D32 zFar   = 7000.0f;
	F32 fov    = 60;
	F32 ratio = 1.0f * w / h;
	F32 x=0.0f,y=1.75f,z=5.0f;
	F32 lx=0.0f,ly=0.0f,lz=-1.0f;

	ParamHandler::getInstance().setParam("zNear",zNear);
	ParamHandler::getInstance().setParam("zFar",zFar);
	ParamHandler::getInstance().setParam("FOV",fov);
	ParamHandler::getInstance().setParam("aspectRatio",ratio);

	// Reset the coordinate system before modifying
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(fov,ratio,zNear,zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(x, y, z, 
		  	x + lx,y + ly,z + lz,
			0.0f,1.0f,0.0f);
}

void GL_API::lookAt(const vec3& eye,const vec3& center,const vec3& up)
{
		gluLookAt(	eye.x,		eye.y,		eye.z,
					center.x,	center.y,	center.z,
					up.x,		up.y,		up.z	);
}

void GL_API::idle()
{
	glutSetWindow(Engine::getInstance().getMainWindowId()); 
	glutPostRedisplay();
}


mat4 GL_API::getProjectionMatrix()
{
	mat4 projection;
	glGetFloatv( GL_PROJECTION_MATRIX, projection.mat );	
	return projection;
}

mat4 GL_API::getModelViewMatrix()
{
	mat4 modelview;
	glGetFloatv( GL_MODELVIEW_MATRIX, modelview.mat );	
	return modelview;
}

void GL_API::translate(const vec3& pos)
{
	glTranslatef(pos.x,pos.y,pos.z);
}

void GL_API::rotate(F32 angle, const vec3& weights)
{
	glRotatef(angle,weights.x,weights.y,weights.z);
}

void GL_API::scale(const vec3& scale)
{
	glScalef(scale.x,scale.y,scale.z);
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

void GL_API::pushMatrix()
{
	glPushMatrix();
}

void GL_API::popMatrix()
{
	glPopMatrix();
}

void GL_API::enable_MODELVIEW()
{
	glMatrixMode( GL_MODELVIEW );
}

void GL_API::loadIdentityMatrix(){
	glLoadIdentity();
}

void GL_API::setTextureMatrix(U16 slot, const mat4& transformMatrix){
	glMatrixMode(GL_TEXTURE);
	glActiveTexture(GL_TEXTURE0+slot);
	glLoadMatrixf( transformMatrix );
	glMatrixMode(GL_MODELVIEW);
}

void GL_API::restoreTextureMatrix(U16 slot){
	glMatrixMode(GL_TEXTURE);
	glActiveTexture(GL_TEXTURE0+slot);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

void GL_API::setOrthoProjection(const vec4& rect, const vec2& planes){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
	glMatrixMode(GL_MODELVIEW);
}

void GL_API::drawTextToScreen(Text* text){
	pushMatrix();
		glLoadIdentity();
		glColor3f(text->_color.x,text->_color.y,text->_color.z);
		glRasterPos2f(text->getPosition().x,text->getPosition().y);
		
#ifdef USE_FREEGLUT
		glutBitmapString(text->_font, (U8*)(text->_text.c_str()));
#else
		//nothing yet
#endif
	popMatrix();
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
		/*
		 *	We will indicate that the mouse cursor is over the button by changing its
		 *	colour.
		 */
		if(!b->isVisible()) return;
		if (b->_highlight) 
			glColor3f(b->_color.r + 0.1f,b->_color.g + 0.1f,b->_color.b + 0.2f);
		else 
			glColor3f(b->_color.r,b->_color.g,b->_color.b);

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
		if (b->_pressed && b->isActive()) 
			glColor3f((b->_color.r + 0.1f)/2.0f,(b->_color.g+ 0.1f)/2.0f,(b->_color.b+ 0.1f)/2.0f);
		else 
			glColor3f(b->_color.r + 0.1f,b->_color.g+ 0.1f,b->_color.b+ 0.1f);

		glBegin(GL_LINE_STRIP);
			glVertex2f( b->getPosition().x+b->_dimensions.x, b->getPosition().y      );
			glVertex2f( b->getPosition().x     , b->getPosition().y      );
			glVertex2f( b->getPosition().x     , b->getPosition().y+b->_dimensions.y );
		glEnd();

		if (b->_pressed) 
			glColor3f(0.8f,0.8f,0.8f);
		else 
			glColor3f(0.4f,0.4f,0.4f);

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
		if(b->_highlight)
		{
			if(t) delete t;
			t = new Text(string("1"),b->_text,vec3(fontx,fonty,0),GLUT_BITMAP_HELVETICA_10,vec3(0,0,0));
			fontx--;
			fonty--;
		}
		if(t) delete t;
		t = new Text(string("1"),b->_text,vec3(fontx,fonty,0),GLUT_BITMAP_HELVETICA_10,vec3(1,1,1));
		drawTextToScreen(t);
	}
	delete t;
	t = NULL;

}

void GL_API::setRenderState(RenderState& state){
	state.blendingEnabled() ? glEnable(GL_BLEND)      : glDisable(GL_BLEND);
	state.lightingEnabled() ? glEnable(GL_LIGHTING)   : glDisable(GL_LIGHTING);
	state.cullingEnabled()  ? glEnable(GL_CULL_FACE)  : glDisable(GL_CULL_FACE);
	state.texturesEnabled() ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
}

void GL_API::ignoreStateChanges(bool state){
	if(state){
		glPushAttrib(GL_ENABLE_BIT);
	}else{
		glPopAttrib();
	}
}

//ToDo: Sort meshes by material!!!!! - Ionut
//Maybe create custom materials and manually assign them to submeshes?
//Bind once, draw all meshes affected by material, unbind?
void GL_API::prepareMaterial(SceneNode* model, Material* mat, Shader* prevShader){
	if(!mat) return;
	Shader* s = mat->getShader();
	F32 windX = SceneManager::getInstance().getActiveScene()->getWindDirX();
	F32 windZ = SceneManager::getInstance().getActiveScene()->getWindDirX();
	F32 windS = SceneManager::getInstance().getActiveScene()->getWindSpeed();
	Texture2D* baseTexture = mat->getTexture(Material::TEXTURE_BASE);
	Texture2D* bumpTexture = mat->getTexture(Material::TEXTURE_BUMP);
	Texture2D* secondTexture = mat->getTexture(Material::TEXTURE_SECOND);
	U8 count = 0;
	if(baseTexture){
		baseTexture->Bind(0);
		count++;
	}
	if(bumpTexture)	bumpTexture->Bind(1);
	
	if(secondTexture){
		secondTexture->Bind(2);
		count++;
	}
	setMaterial(mat);

	if(s) {
		//Console::getInstance().printfn("Model [ %s ] uses shader [ %s ]",model->getName().c_str(), s->getName().c_str());
		if(prevShader){ //If we specified a previous shader
			if(s->getName().compare(prevShader->getName()) != 0) //Only bind/unbind if we have a new shader;
				s->bind();
		}else{
			s->bind();
		}
		if(model->getTransform()){
			s->Uniform("scale",model->getTransform()->getScale());
		}else{
			s->Uniform("scale",vec3(1,1,1));
		}
		s->Uniform("material",mat->getMaterialMatrix());
		if(baseTexture){
			s->UniformTexture("texDiffuse0",0);
		}

		if(!GFXDevice::getInstance().getDeferredShading()){
			if(bumpTexture){
				s->UniformTexture("texBump",1);
				s->Uniform("mode", 1);
			}else{
				s->Uniform("mode", 0);
			}
			if(secondTexture){
				s->UniformTexture("texDiffuse1",2);
			}
			s->Uniform("textureCount",count);
			s->Uniform("enable_shadow_mapping", 0);
			s->Uniform("tile_factor", 1.0f);
		}

		s->Uniform("time", GETTIME());
		s->Uniform("windDirectionX", windX);
		s->Uniform("windDirectionZ", windZ);
		s->Uniform("windSpeed", windS);
	}
}

void GL_API::releaseMaterial(Material* mat, Shader* prevShader){
	if(!mat) return;
	Shader* s = mat->getShader();
	Texture2D* baseTexture = mat->getTexture(Material::TEXTURE_BASE);
	Texture2D* bumpTexture = mat->getTexture(Material::TEXTURE_BUMP);
	Texture2D* secondTexture = mat->getTexture(Material::TEXTURE_SECOND);

	if(s)
		if(prevShader){ //If we specified a previous shader
			if(s->getName().compare(prevShader->getName()) != 0) //Only bind/unbind if we have a new shader;
				s->unbind();
		}else{
			s->unbind();
		}

	if(secondTexture) secondTexture->Unbind(2);
	if(bumpTexture) bumpTexture->Unbind(1);
	if(baseTexture) baseTexture->Unbind(0);

}

void GL_API::drawBox3D(vec3 min, vec3 max)
{

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

void GL_API::drawBox3D(Box3D* const model)
{
	setObjectState(model);

	glutSolidCube(model->getSize());

	popMatrix();

}

void GL_API::drawSphere3D(Sphere3D* const model)
{
	setObjectState(model);
	
	glutSolidSphere(model->getSize(), model->getResolution(),model->getResolution());

	releaseObjectState(model);

}
void GL_API::setObjectState(SceneNode* const model){
	pushMatrix();
	if(model->getTransform()){
		glMultMatrixf(model->getTransform()->getMatrix());
		glMultMatrixf(model->getTransform()->getParentMatrix());
	}

	prepareMaterial(model, model->getMaterial());
}

void GL_API::releaseObjectState(SceneNode* const model){
	releaseMaterial(model->getMaterial());
	popMatrix();
}

void GL_API::drawQuad3D(Quad3D* const model)
{
	setObjectState(model);

	glBegin(GL_TRIANGLE_STRIP); //GL_TRIANGLE_STRIP is slightly faster on newer HW than GL_QUAD,
								//as GL_QUAD converts into a GL_TRIANGLE_STRIP at the driver level anyway
		glNormal3f(0.0f, 1.0f, 0.0f);
		glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
		glVertex3f(model->getCorner(Quad3D::TOP_LEFT).x, model->getCorner(Quad3D::TOP_LEFT).y, model->getCorner(Quad3D::TOP_LEFT).z);
		glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
		glVertex3f(model->getCorner(Quad3D::TOP_RIGHT).x, model->getCorner(Quad3D::TOP_RIGHT).y, model->getCorner(Quad3D::TOP_RIGHT).z);
		glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
		glVertex3f(model->getCorner(Quad3D::BOTTOM_LEFT).x, model->getCorner(Quad3D::BOTTOM_LEFT).y, model->getCorner(Quad3D::BOTTOM_LEFT).z);
		glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
		glVertex3f(model->getCorner(Quad3D::BOTTOM_RIGHT).x, model->getCorner(Quad3D::BOTTOM_RIGHT).y, model->getCorner(Quad3D::BOTTOM_RIGHT).z);
	glEnd();

	releaseObjectState(model);

}

void GL_API::drawText3D(Text3D* const model)
{
	setObjectState(model);

	glPushAttrib(GL_ENABLE_BIT);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(model->getWidth());
	setMaterial(model->getMaterial());
	glutStrokeString(model->getFont(), (const U8*)model->getText().c_str());
	glPopAttrib();

	releaseObjectState(model);

}

void GL_API::renderModel(Object3D* const model)
{
	Mesh* tempModel = dynamic_cast<Mesh*>(model);

	pushMatrix();
	//ToDo: Per submesh transforms!!!!!!!!!!!!!!!!!!! - Ionut
	if(tempModel->getTransform()){
		glMultMatrixf(tempModel->getTransform()->getMatrix());
	}
	if(model->getTransform()){
		glMultMatrixf(model->getTransform()->getParentMatrix());
	}
	Shader* prevShader = NULL;

	for(vector<SubMesh* >::iterator subMeshIterator = tempModel->getSubMeshes().begin(); 
		subMeshIterator != tempModel->getSubMeshes().end(); 
		++subMeshIterator)	{

		SubMesh *s = (*subMeshIterator);
		prepareMaterial(s, s->getMaterial(), prevShader);

		s->getGeometryVBO()->Enable();
			glDrawElements(GL_TRIANGLES, s->getIndices().size(), GL_UNSIGNED_INT, &(s->getIndices()[0]));
		s->getGeometryVBO()->Disable();

		releaseMaterial(s->getMaterial(), prevShader);
		prevShader = s->getMaterial()->getShader();
		
	}
	popMatrix();
}

void GL_API::renderElements(Type t, U32 count, const void* first_element){
	glDrawElements(t, count, GL_UNSIGNED_INT, first_element );
}

void GL_API::toggle2D(bool _2D){
	
	if(_2D)
	{
		F32 width = Engine::getInstance().getWindowDimensions().width;
		F32 height = Engine::getInstance().getWindowDimensions().height;
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix(); //1
		glLoadIdentity();
		glOrtho(0,width,height,0,-1,1);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix(); //2
		glLoadIdentity();


	}
	else
	{
		glPopMatrix(); //2 
		glMatrixMode(GL_PROJECTION);
		glPopMatrix(); //1
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
	}
}

void GL_API::setMaterial(Material* mat)
{
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat->getMaterialMatrix().getCol(1));
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,mat->getMaterialMatrix().getCol(0));
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat->getMaterialMatrix().getCol(2));
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,mat->getMaterialMatrix().getCol(3).x);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,vec4(mat->getMaterialMatrix().getCol(3).y,mat->getMaterialMatrix().getCol(3).z,mat->getMaterialMatrix().getCol(3).w,1.0f));
}

void GL_API::setColor(const vec4& color)
{
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
}

void GL_API::setColor(const vec3& color)
{
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
}


void GL_API::initDevice()
{
	//F32 global_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	glutMainLoop();
}

void GL_API::setLight(U8 slot, tr1::unordered_map<string,vec4>& properties){
	if(!_state.lightingEnabled()) return;
	glEnable(GL_LIGHT0+slot);
	glLightfv(GL_LIGHT0+slot, GL_POSITION, properties["position"]);
	glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  properties["ambient"]);
	glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  properties["diffuse"]);
	glLightfv(GL_LIGHT0+slot, GL_SPECULAR, properties["specular"]);

	if( properties.size() == 5)	glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, properties["spotDirection"]);

}

void GL_API::createLight(U8 slot)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0+slot);
}

void GL_API::toggleWireframe(bool state)
{
	if(state)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GL_API::setLightCameraMatrices(const vec3& lightVector)
{
	vec3 eye = CameraManager::getInstance().getActiveCamera()->getEye();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(	eye.x - 500*lightVector.x,		eye.y - 500*lightVector.y,		eye.z - 500*lightVector.z,
				eye.x,							eye.y,							eye.z,
				0.0,							1.0,							0.0	);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
}


void GL_API::restoreLightCameraMatrices()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
