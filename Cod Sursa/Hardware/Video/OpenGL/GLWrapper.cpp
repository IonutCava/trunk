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

void resizeWindowCallback(I32 w, I32 h)
{
	GUI::getInstance().onResize(w,h);
	GFXDevice::getInstance().resizeWindow(w,h);
}

void GL_API::initHardware()
{
	I32   argc   = 1; 
	char *argv[] = {"DIVIDE Engine", NULL};
    glutInit(&argc, argv);
	glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
	glutInitWindowSize(Engine::getInstance().getWindowDimensions().width,Engine::getInstance().getWindowDimensions().height);
	glutInitWindowPosition(10,50);
	Engine::getInstance().setMainWindowId(glutCreateWindow("DIVIDE Engine"));
	U32 err = glewInit();
	if (GLEW_OK != err)
	{
		Con::getInstance().errorfn("GFXDevice: %s \nTry switching to DX (version 9.0c required) or upgrade hardware.\nApplication will now exit!",glewGetErrorString(err));
		exit(1);
	}
	string ver;
	if      (glewIsSupported("GL_VERSION_3_2")) ver = "3.2";
	else if	(glewIsSupported("GL_VERSION_3_1"))	ver = "3.1";
	else if	(glewIsSupported("GL_VERSION_3_0")) ver = "3.0";
	else if	(glewIsSupported("GL_VERSION_2_1")) ver = "2.1";
	else if	(glewIsSupported("GL_VERSION_2_0")) ver = "2.0";
	else
	{
		Con::getInstance().errorfn("Your current hardware does not support the OpenGL 2.0 extension set!");
		Con::getInstance().printfn("Try switching to DX (version 9.0c required) or upgrade hardware.");
		Con::getInstance().printfn("Application will now exit!");
		exit(2);
	}
	Con::getInstance().printfn("Hardware acceleration up to OpenGL %s supported!", ver.c_str());
	ver.empty();
	
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
	Con::getInstance().printfn("OpenGL rendering system initialized!");
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

void GL_API::loadIdentityMatrix()
{
	glLoadIdentity();
}

void GL_API::drawTextToScreen(Text* text)
{
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

void GL_API::setRenderState(RenderState& state)
{
	state.blendingEnabled() ? glEnable(GL_BLEND)      : glDisable(GL_BLEND);
	state.lightingEnabled() ? glEnable(GL_LIGHTING)   : glDisable(GL_LIGHTING);
	state.cullingEnabled()  ? glEnable(GL_CULL_FACE)  : glDisable(GL_CULL_FACE);
	state.texturesEnabled() ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
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

void GL_API::drawBox3D(Box3D* const box)
{
	box->onDraw();
	pushMatrix();

	Shader* s = NULL;
	if(!box->getShaders().empty()){
		if(box->getShaders()[0]) 
			s = box->getShaders()[0];

	glMultMatrixf(box->getTransform()->getMatrix());
	setMaterial(box->getMaterial());
	if(!box->getMaterial().textures.empty()) box->getMaterial().textures.front()->Bind(0);
	
		if(s) {
			s->bind();
			s->Uniform("color",box->getMaterial().diffuse);
			if(!box->getMaterial().textures.empty())
				s->UniformTexture("texDiffuse",0);
		}
	}
	glutSolidCube(box->getSize());
	if(s) s->unbind();
	if(!box->getMaterial().textures.empty()) box->getMaterial().textures.front()->Unbind(0);

	popMatrix();

}

void GL_API::drawSphere3D(Sphere3D* const sphere)
{
	
	sphere->onDraw();
	pushMatrix();
	glMultMatrixf(sphere->getTransform()->getMatrix());

	Shader* s = NULL;
	if(!sphere->getShaders().empty()){
		if(sphere->getShaders()[0]) 
			s = sphere->getShaders()[0];

	setMaterial(sphere->getMaterial());
	if(!sphere->getMaterial().textures.empty()) sphere->getMaterial().textures.front()->Bind(0);
	if(s) {
			s->bind();
			s->Uniform("color",sphere->getMaterial().diffuse);
			if(!sphere->getMaterial().textures.empty())
				s->UniformTexture("texDiffuse",0);
		}
	}
	glutSolidSphere(sphere->getSize(), sphere->getResolution(),sphere->getResolution());

	if(s) s->unbind();
	if(!sphere->getMaterial().textures.empty()) sphere->getMaterial().textures.front()->Unbind(0);
	popMatrix();

}

void GL_API::drawQuad3D(Quad3D* const quad)
{
	quad->onDraw();
	pushMatrix();

	glMultMatrixf(quad->getTransform()->getMatrix());
	setMaterial(quad->getMaterial());

	Shader* s = NULL;
	if(!quad->getShaders().empty()){
		if(quad->getShaders()[0]) 
			s = quad->getShaders()[0];

	if(!quad->getMaterial().textures.empty()) quad->getMaterial().textures.front()->Bind(0);
	if(s){
			s->bind();
			s->Uniform("color",quad->getMaterial().diffuse);
			if(!quad->getMaterial().textures.empty())
				s->UniformTexture("texDiffuse",0);
		}
	}

	glBegin(GL_TRIANGLE_STRIP); //GL_TRIANGLE_STRIP is slightly faster on newer HW than GL_QUAD,
								//as GL_QUAD converts into a GL_TRIANGLE_STRIP at the driver level anyway
		glNormal3f(0.0f, 1.0f, 0.0f);
		glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
		glVertex3f(quad->getCorner(Quad3D::TOP_LEFT).x, quad->getCorner(Quad3D::TOP_LEFT).y, quad->getCorner(Quad3D::TOP_LEFT).z);
		glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
		glVertex3f(quad->getCorner(Quad3D::TOP_RIGHT).x, quad->getCorner(Quad3D::TOP_RIGHT).y, quad->getCorner(Quad3D::TOP_RIGHT).z);
		glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
		glVertex3f(quad->getCorner(Quad3D::BOTTOM_LEFT).x, quad->getCorner(Quad3D::BOTTOM_LEFT).y, quad->getCorner(Quad3D::BOTTOM_LEFT).z);
		glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
		glVertex3f(quad->getCorner(Quad3D::BOTTOM_RIGHT).x, quad->getCorner(Quad3D::BOTTOM_RIGHT).y, quad->getCorner(Quad3D::BOTTOM_RIGHT).z);
	glEnd();

	if(s)	s->unbind();
	if(!quad->getMaterial().textures.empty()) quad->getMaterial().textures.front()->Unbind(0);
	
	popMatrix();

}

void GL_API::drawText3D(Text3D* const text)
{
	text->onDraw();
	pushMatrix();
	glMultMatrixf(text->getTransform()->getMatrix());

	Shader* s = NULL;
	if(!text->getShaders().empty()){
		if(text->getShaders()[0]) 
			s = text->getShaders()[0];

	if(!text->getMaterial().textures.empty()) text->getMaterial().textures.front()->Bind(0);
	if(s) {
			s->bind();
			s->Uniform("color",text->getMaterial().diffuse);
			if(!text->getMaterial().textures.empty())
				s->UniformTexture("texDiffuse",0);
		}
	}
	glPushAttrib(GL_ENABLE_BIT);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(text->getWidth());
	setMaterial(text->getMaterial());
	glutStrokeString(text->getFont(), (const U8*)text->getText().c_str());
	glPopAttrib();

	if(s) s->unbind();

	if(!text->getMaterial().textures.empty()) text->getMaterial().textures.front()->Unbind(0);
	popMatrix();

}

void GL_API::toggle2D(bool _2D)
{
	
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

void GL_API::renderModel(Mesh* const model)
{
	SubMesh *s;
	vector<SubMesh* >::iterator _subMeshIterator;
	
	pushMatrix();
	//ToDo: Per submesh transforms
	glMultMatrixf(model->getTransform()->getMatrix());

	for(_subMeshIterator = model->getSubMeshes().begin(); 
		_subMeshIterator != model->getSubMeshes().end(); 
		++_subMeshIterator)	{

		s = (*_subMeshIterator);

		setMaterial(s->getMaterial());
		
		U8 count = s->getMaterial().textures.size();
		U8 i = 0;
		if(count)
			for(U8 j = 0; j < count; j++,i++)
				s->getMaterial().textures[j]->Bind(i);

		
		
		if(s->getMaterial().bumpMap)
			s->getMaterial().bumpMap->Bind(i++);
				
		U8 n = 0;
		
		if(!model->getShaders().empty()){

			Shader* shader = NULL;
			//for(U8 n = 0; n < model->getShaders().size(); n++)
			if(model->getShaders()[n]){
				shader = model->getShaders()[n];
				shader->bind();
				if(!GFXDevice::getInstance().getDeferredShading()){
     				shader->Uniform("enable_shadow_mapping", 0);
					shader->Uniform("tile_factor", 1.0f);
					shader->Uniform("textureCount",count <= 2 ? count : 2);
					shader->UniformTexture("texDiffuse0",0);
					if(count > 1) shader->UniformTexture("texDiffuse1",1);
					if(s->getMaterial().bumpMap){
						shader->Uniform("mode", 1);
						shader->UniformTexture("texNormalHeightMap",i);
					}else{
						shader->Uniform("mode", 0);
					}
				}else{
					shader->Uniform("tDiffuse",0);
				}
			}
		}

		s->getGeometryVBO()->Enable();
			glDrawElements(GL_TRIANGLES, s->getIndices().size(), GL_UNSIGNED_INT, &(s->getIndices()[0]));
		s->getGeometryVBO()->Disable();

		if(!model->getShaders().empty()){
			//for(U8 n = 0; n < model->getShaders().size(); n++){
				if(model->getShaders()[n])
					model->getShaders()[n]->unbind();
			//}
		}

		if(s->getMaterial().bumpMap)
			s->getMaterial().bumpMap->Unbind(i);

		if(count) 
			for(U8 j = count-1; j > 0; j--) s->getMaterial().textures[j]->Unbind(i--);
				
	}


	
	popMatrix();

}

void GL_API::renderElements(Type t, U32 count, const void* first_element)
{
	glDrawElements(t, count, GL_UNSIGNED_INT, first_element );
}

void GL_API::setMaterial(Material& mat)
{
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,mat.ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat.specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,mat.shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,mat.emmissive);
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
	F32 global_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
	glutMainLoop();
}

void GL_API::setLight(U8 slot, tr1::unordered_map<string,vec4>& properties)
{
	glEnable(GL_LIGHTING);
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
