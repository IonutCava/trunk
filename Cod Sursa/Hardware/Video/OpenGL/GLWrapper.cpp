#include "GLWrapper.h"
#include "Utility/Headers/DataTypes.h"
#include "Rendering/common.h"
#include "Utility/Headers/Guardian.h"
#include "GUI/GUI.h"

#define USE_FREEGLUT

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
	cout << "OpenGL rendering system initialized!" << endl;
	
}
void GL_API::closeRenderingApi()
{
	glutLeaveMainLoop();
	glutDestroyWindow(Engine::getInstance().getMainWindowId());
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

void GL_API::enable_PROJECTION()
{
	glMatrixMode( GL_PROJECTION );
}

void GL_API::enable_TEXTURE(int slot)
{
	glMatrixMode( GL_TEXTURE );
	if(slot > 0) glActiveTexture(GL_TEXTURE0 + slot);
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
		glutBitmapString(text->_font, (UBYTE *)(text->_text.c_str()));
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
		fontx = b->_position.x + (b->_dimensions.x - glutBitmapLength(GLUT_BITMAP_HELVETICA_10,(UBYTE*)b->_text.c_str())) / 2 ;
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

void GL_API::drawCube(F32 size)
{
	glutSolidCube(size);
}

void GL_API::drawSphere(F32 size, U32 resolution)
{
	glutSolidSphere(size,resolution,resolution);
}

void GL_API::drawQuad(vec3& _topLeft, vec3& _topRight, vec3& _bottomLeft, vec3& _bottomRight)
{
	glBegin(GL_TRIANGLE_STRIP); //GL_TRIANGLE_STRIP is slightly faster on newer HW than GL_QUAD,
								//as GL_QUAD converts into a GL_TRIANGLE_STRIP at the driver level anyway
		glNormal3f(0.0f, 1.0f, 0.0f);
		glTexCoord3f(1.0f, 0.0f, 0.0f);
		glVertex3f(_topLeft.x, _topLeft.y, _topLeft.z);
		glVertex3f(_topRight.x, _topRight.y, _topRight.z);
		glVertex3f(_bottomLeft.x, _bottomLeft.y, _bottomLeft.z);
		glVertex3f(_bottomRight.x, _bottomRight.y, _bottomRight.z);
	glEnd();
}

void GL_API::loadOrtographicView()
{
	glMatrixMode( GL_PROJECTION );
		pushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, Engine::getInstance().getWindowWidth(), 0,Engine::getInstance().getWindowHeight());
		glScalef(1, -1, 1);
		glTranslatef(0, (F32)-Engine::getInstance().getWindowHeight(), 0);
	glMatrixMode( GL_MODELVIEW );
}

void GL_API::loadModelView()
{
	glMatrixMode( GL_PROJECTION );
	popMatrix();
	glMatrixMode( GL_MODELVIEW );
}

void GL_API::toggle2D3D(bool _3D)
{
	if(!_3D)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,Engine::getInstance().getWindowWidth(),Engine::getInstance().getWindowHeight(),0,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
	}
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45,(Engine::getInstance().getWindowHeight()==0)?(1):((float)Engine::getInstance().getWindowWidth()/Engine::getInstance().getWindowHeight()),1,100);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
	}
}

void GL_API::renderMesh(const Mesh& mesh)
{
}

void GL_API::renderSubMesh(const SubMesh& subMesh)
{
}

void GL_API::setColor(F32 r, F32 g, F32 b)
{
	glColor3f(r,g,b);
}

void GL_API::setColor(D32 r, D32 g, D32 b)
{
	glColor3d(r,g,b);
}

void GL_API::setColor(int r, int g, int b)
{
	glColor3i(r,g,b);
}

void GL_API::setColor(F32 r, F32 g, F32 b, F32 alpha)
{
	glColor4f(r,g,b,alpha);
}

void GL_API::setColor(D32 r, D32 g, D32 b, D32 alpha)
{
	glColor4d(r,g,b,alpha);
}

void GL_API::setColor(int r, int g, int b, int alpha)
{
	glColor4i(r,g,b,alpha);
}

void GL_API::setColor(F32 *v)
{
	int number_elements = sizeof( v ) / sizeof( v[0] );
	if( number_elements == 3)	glColor3fv(v);
	else if( number_elements == 4)	glColor4fv(v);
	else glColor3i(0,0,0);
}

void GL_API::setColor(D32 *v)
{
	int number_elements = sizeof( v ) / sizeof( v[0] );
	if( number_elements == 3)	glColor3dv(v);
	else if( number_elements == 4)	glColor4dv(v);
	else glColor3i(0,0,0);
}

void GL_API::setColor(int *v)
{
	int number_elements = sizeof( v ) / sizeof( v[0] );
	if( number_elements == 3)	glColor3iv(v);
	else if( number_elements == 4)	glColor4iv(v);
	else glColor3i(0,0,0);
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