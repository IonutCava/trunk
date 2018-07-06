#include "glResources.h"

#include "GLWrapper.h"
#include "Utility/Headers/DataTypes.h"
#include "Rendering/common.h"
#include "Utility/Headers/Guardian.h"
#include "GUI/GUI.h"
#include "Utility/Headers/ParamHandler.h"

#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Text3D.h"

#define USE_FREEGLUT

void resizeWindowCallback(int w, int h)
{
	GFXDevice::getInstance().resizeWindow(w,h);
}

void GL_API::initHardware()
{
	int   argc   = 1; 
	char *argv[] = {"DIVIDE Engine", NULL};
    glutInit(&argc, argv);
	glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
	glutInitWindowSize(Engine::getInstance().getWindowWidth(),Engine::getInstance().getWindowHeight());
	glutInitWindowPosition(10,50);
	Engine::getInstance().setMainWindowId(glutCreateWindow("DIVIDE Engine"));
	U32 err = glewInit();
	if (GLEW_OK != err)
	{
		cout << "Error: "<< glewGetErrorString(err) << endl
			  << "Try switching to DX (version 9.0c required) or upgrade hardware." << endl
			  << "Application will now exit!" << endl;
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
		cout << "ERROR: " << "Your current hardware does not support the OpenGL 2.0 extension set!" << endl
			 << "Try switching to DX (version 9.0c required) or upgrade hardware." << endl
			 << "Application will now exit!" << endl;
		exit(2);
	}
	cout << "Hardware acceleration up to OpenGL " << ver << " supported!" << endl;
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
	cout << "OpenGL rendering system initialized!" << endl;
	
}

void GL_API::closeRenderingApi()
{
	glutLeaveMainLoop();
	glutDestroyWindow(Engine::getInstance().getMainWindowId());
}

void GL_API::resizeWindow(U32 w, U32 h)
{
	D32 _zNear  = 0.01f;
	D32 _zFar   = 7000.0f;
	F32 x=0.0f,y=1.75f,z=5.0f;
	F32 lx=0.0f,ly=0.0f,lz=-1.0f;

	ParamHandler::getInstance().setParam("zNear",_zNear);
	ParamHandler::getInstance().setParam("zFar",_zFar);

	F32 ratio = 1.0f * w / h;
	// Reset the coordinate system before modifying
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(60,ratio,_zNear,_zFar);
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

F32 GL_API::getTime()  
{
	return (F32)(glutGet(GLUT_ELAPSED_TIME)/1000.0f);
}

F32 GL_API::getMSTime()
{
	return (F32)glutGet(GLUT_ELAPSED_TIME);
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

void GL_API::translate(vec3& pos)
{
	glTranslatef(pos.x,pos.y,pos.z);
}

void GL_API::rotate(F32 angle, vec3& weights)
{
	glRotatef(angle,weights.x,weights.y,weights.z);
}

void GL_API::scale(vec3& scale)
{
	glScalef(scale.x,scale.y,scale.z);
}

void GL_API::clearBuffers(int buffer_mask)
{
	glClear(buffer_mask);
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
	glFogi (GL_FOG_MODE, GL_EXP2); 
	glFogfv (GL_FOG_COLOR, color); 
	glFogf (GL_FOG_DENSITY, density); 
	glHint (GL_FOG_HINT, GL_NICEST); 
	glFogf (GL_FOG_START,  0.05f);
	glFogf (GL_FOG_END,    200.0f);
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
		glRasterPos2f(text->_position.x,text->_position.y);
		
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
	glutBitmapCharacter(font, (int)text);
#else
	//nothing yet
#endif
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
		if (b->_highlight) 
			glColor3f(b->_color.r + 0.1f,b->_color.g + 0.1f,b->_color.b + 0.2f);
		else 
			glColor3f(b->_color.r,b->_color.g,b->_color.b);

		/*
		 *	draw background for the button.
		 */
		glBegin(GL_QUADS);
			glVertex2f( b->_position.x     , b->_position.y      );
			glVertex2f( b->_position.x     , b->_position.y+b->_dimensions.y );
			glVertex2f( b->_position.x+b->_dimensions.x, b->_position.y+b->_dimensions.y );
			glVertex2f( b->_position.x+b->_dimensions.x, b->_position.y      );
		glEnd();

		/*
		 *	Draw an outline around the button with width 3
		 */
		glLineWidth(3);

		/*
		 *	The colours for the outline are reversed when the button. 
		 */
		if (b->_pressed) 
			glColor3f((b->_color.r + 0.1f)/2.0f,(b->_color.g+ 0.1f)/2.0f,(b->_color.b+ 0.1f)/2.0f);
		else 
			glColor3f(b->_color.r + 0.1f,b->_color.g+ 0.1f,b->_color.b+ 0.1f);

		glBegin(GL_LINE_STRIP);
			glVertex2f( b->_position.x+b->_dimensions.x, b->_position.y      );
			glVertex2f( b->_position.x     , b->_position.y      );
			glVertex2f( b->_position.x     , b->_position.y+b->_dimensions.y );
		glEnd();

		if (b->_pressed) 
			glColor3f(0.8f,0.8f,0.8f);
		else 
			glColor3f(0.4f,0.4f,0.4f);

		glBegin(GL_LINE_STRIP);
			glVertex2f( b->_position.x     , b->_position.y+b->_dimensions.y );
			glVertex2f( b->_position.x+b->_dimensions.x, b->_position.y+b->_dimensions.y );
			glVertex2f( b->_position.x+b->_dimensions.x, b->_position.y      );
		glEnd();

		glLineWidth(1);
		fontx = b->_position.x + (b->_dimensions.x - glutBitmapLength(GLUT_BITMAP_HELVETICA_10,(U8*)b->_text.c_str())) / 2 ;
		fonty = b->_position.y + (b->_dimensions.y+10)/2;

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

void GL_API::beginRenderStateProcessing()
{
	//ToDo: Enable these and fix texturing issues -Ionut
		glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT);
		if(_state.isEnabled())
		{
			if( !_state.blendingEnabled()) glDisable(GL_BLEND);
			//else glEnable(GL_BLEND);
			if(!_state.cullingEnabled() ) glDisable(GL_CULL_FACE);
			//else glEnable(GL_CULL_FACE);
			if(!_state.lightingEnabled()) glDisable(GL_LIGHTING);
			//else glEnable(GL_LIGHTING);
			if(!_state.texturesEnabled()) glDisable(GL_TEXTURE_2D);
			//else glEnable(GL_TEXTURE_2D);
		}
}

void GL_API::endRenderStateProcessing()
{
		glPopAttrib();
}

void GL_API::drawBox3D(vec3 min, vec3 max)
{
	//beginRenderStateProcessing();

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

	//endRenderStateProcessing();
}

void GL_API::drawBox3D(Box3D* const box)
{
	//beginRenderStateProcessing();

	pushMatrix();
	translate(box->getPosition());
	rotate(box->getOrientation().x,vec3(1.0f,0.0f,0.0f));
	rotate(box->getOrientation().y,vec3(0.0f,1.0f,0.0f));
	rotate(box->getOrientation().z,vec3(0.0f,0.0f,1.0f));
	scale(box->getScale());
	setColor(box->getColor());
	if(box->getTexture()) box->getTexture()->Bind(0);
	if(box->getShader()) 
	{
		box->getShader()->bind();
		box->getShader()->UniformTexture("texDiffuse",0);
	}
	glutSolidCube(box->getSize());
	if(box->getShader())  box->getShader()->unbind();
	if(box->getTexture()) box->getTexture()->Unbind(0);
	popMatrix();

	//endRenderStateProcessing();
}

void GL_API::drawSphere3D(Sphere3D* const sphere)
{
	//beginRenderStateProcessing();

	pushMatrix();
	translate(sphere->getPosition());
	rotate(sphere->getOrientation().x,vec3(1.0f,0.0f,0.0f));
	rotate(sphere->getOrientation().y,vec3(0.0f,1.0f,0.0f));
	rotate(sphere->getOrientation().z,vec3(0.0f,0.0f,1.0f));
	scale(sphere->getScale());
	setColor(sphere->getColor());
	if(sphere->getTexture()) sphere->getTexture()->Bind(0);
	if(sphere->getShader()) 
	{
		sphere->getShader()->bind();
		sphere->getShader()->UniformTexture("texDiffuse",0);
	}
	glutSolidSphere(sphere->getSize(),sphere->getResolution(),sphere->getResolution());
	if(sphere->getShader())  sphere->getShader()->unbind();
	if(sphere->getTexture()) sphere->getTexture()->Unbind(0);
	popMatrix();

	//endRenderStateProcessing();
	
}

void GL_API::drawQuad3D(Quad3D* const quad)
{
	//beginRenderStateProcessing();

	pushMatrix();
	translate(quad->getPosition());
	rotate(quad->getOrientation().x,vec3(1.0f,0.0f,0.0f));
	rotate(quad->getOrientation().y,vec3(0.0f,1.0f,0.0f));
	rotate(quad->getOrientation().z,vec3(0.0f,0.0f,1.0f));
	scale(quad->getScale());
	setColor(quad->getColor());

	if(quad->getTexture()) quad->getTexture()->Bind(0);
	if(quad->getShader()) 
	{
		quad->getShader()->bind();
		quad->getShader()->UniformTexture("texDiffuse",0);
	}
	glBegin(GL_TRIANGLE_STRIP); //GL_TRIANGLE_STRIP is slightly faster on newer HW than GL_QUAD,
								//as GL_QUAD converts into a GL_TRIANGLE_STRIP at the driver level anyway
		glNormal3f(0.0f, 1.0f, 0.0f);
		glTexCoord3f(1.0f, 0.0f, 0.0f);
		glVertex3f(quad->_tl.x, quad->_tl.y, quad->_tl.z);
		glVertex3f(quad->_tr.x, quad->_tr.y, quad->_tr.z);
		glVertex3f(quad->_bl.x, quad->_bl.y, quad->_bl.z);
		glVertex3f(quad->_br.x, quad->_br.y, quad->_br.z);
	glEnd();
	if(quad->getShader())  quad->getShader()->unbind();
	if(quad->getTexture()) quad->getTexture()->Unbind(0);
	
	popMatrix();

	//endRenderStateProcessing();
}

void GL_API::drawText3D(Text3D* const text)
{
	//beginRenderStateProcessing();

	pushMatrix();
	translate(text->getPosition());
	rotate(text->getOrientation().x,vec3(1.0f,0.0f,0.0f));
	rotate(text->getOrientation().y,vec3(0.0f,1.0f,0.0f));
	rotate(text->getOrientation().z,vec3(0.0f,0.0f,1.0f));
	scale(text->getScale());
	setColor(text->getColor());
	if(text->getTexture()) text->getTexture()->Bind(0);
	if(text->getShader()) 
	{
		text->getShader()->bind();
		text->getShader()->UniformTexture("texDiffuse",0);
	}
	glPushAttrib(GL_ENABLE_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(text->getWidth());
	glutStrokeString(text->getFont(), (const unsigned char*)text->getText().c_str());
	glPopAttrib();
	if(text->getShader())  text->getShader()->unbind();
	if(text->getTexture()) text->getTexture()->Unbind(0);
	popMatrix();

	//endRenderStateProcessing();
}

void GL_API::toggle2D(bool _2D)
{
	
	if(_2D)
	{
		F32 width = Engine::getInstance().getWindowWidth();
		F32 height = Engine::getInstance().getWindowHeight();
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

void GL_API::renderModel(DVDFile* const model)
{
	if(!model->isVisible()) return;

	//beginRenderStateProcessing();

	SubMesh *s;
	vector<SubMesh* >::iterator _subMeshIterator;
	
	pushMatrix();
	translate(model->getPosition());
	rotate(model->getOrientation().x,vec3(1.0f,0.0f,0.0f));
	rotate(model->getOrientation().y,vec3(0.0f,1.0f,0.0f));
	rotate(model->getOrientation().z,vec3(0.0f,0.0f,1.0f));
	scale(model->getScale());
	model->getShader()->bind();
	
	for(_subMeshIterator = model->getSubMeshes().begin(); 
		_subMeshIterator != model->getSubMeshes().end(); 
		_subMeshIterator++)
	{
		s = (*_subMeshIterator);
		s->getGeometryVBO()->Enable();
		s->getMaterial().texture->Bind(0);
			model->getShader()->UniformTexture("texDiffuse",0);
	
			glDrawElements(GL_TRIANGLES, s->getIndices().size(), GL_UNSIGNED_INT, &(s->getIndices()[0]));

		s->getMaterial().texture->Unbind(0);
		s->getGeometryVBO()->Disable();
		
	}
	model->getShader()->unbind();
	popMatrix();

	//endRenderStateProcessing();
}

void GL_API::renderElements(Type t, U32 count, const void* first_element)
{
	//beginRenderStateProcessing();

	glDrawElements(t, count, GL_UNSIGNED_INT, first_element );

	//endRenderStateProcessing();
}

void GL_API::setColor(vec4& color)
{
	glColor4f(color.r,color.g,color.b,color.a);
}

void GL_API::setColor(vec3& color)
{
	glColor3f(color.r,color.g,color.b);
}


void GL_API::initDevice()
{
	glutMainLoop();
}

void GL_API::setLight(U32 slot, tr1::unordered_map<string,vec4>& properties)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0+slot);
	if( properties.size() == 5)	glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, properties["spotDirection"]);
	glLightfv(GL_LIGHT0+slot, GL_POSITION, properties["position"]);
	glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  properties["ambient"]);
	glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  properties["diffuse"]);
	glLightfv(GL_LIGHT0+slot, GL_SPECULAR, properties["diffuse"]);
}

void GL_API::createLight(U32 slot)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0+slot);
}