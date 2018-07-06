#include "GLWrapper.h"
#include "Utility/Headers/DataTypes.h"
#include "Rendering/common.h"
#include "Utility/Headers/Guardian.h"

#define USE_FREEGLUT

void GL_API::initHardware()
{
	int   argc   = 1; 
	char *argv[] = {"DIVIDE Engine", NULL};
    glutInit(&argc, argv);
	glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
	glutInitWindowSize(Engine::getInstance().getWindowWidth(),Engine::getInstance().getWindowHeight());
	glutInitWindowPosition(10,10);
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
	glEnable(GL_RESCALE_NORMAL);
	glEnable(GL_COLOR_MATERIAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable (GL_COLOR_MATERIAL );
	glutCloseFunc(Guardian::getInstance().TerminateApplication);
	cout << "OpenGL rendering system initialized!" << endl;
	
}
void GL_API::closeRenderingApi()
{
	glutLeaveMainLoop();
	glutDestroyWindow(Engine::getInstance().getMainWindowId());
}

void GL_API::translate(F32 x, F32 y, F32 z)
{
	glTranslatef(x,y,z);
}

void GL_API::translate(D32 x, D32 y, D32 z)
{
	glTranslated(x,y,z);
}

void GL_API::rotate(F32 angle, F32 x, F32 y, F32 z)
{
	glRotatef(angle,x,y,z);
}

void GL_API::rotate(D32 angle, D32 x, D32 y, D32 z)
{
	glRotated(angle,x,y,z);
}

void GL_API::scale(F32 x, F32 y, F32 z)
{
	glScalef(x,y,z);
}

void GL_API::scale(D32 x, D32 y, D32 z)
{
	glScaled(x,y,z);
}

void GL_API::scale(int x, int y, int z)
{
	glScalef((F32)x,(F32)y,(F32)z);
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
	glEnable (GL_FOG);
	glFogi (GL_FOG_MODE, GL_EXP2); 
	glFogfv (GL_FOG_COLOR, color); 
	glFogf (GL_FOG_DENSITY, density); 
	glHint (GL_FOG_HINT, GL_NICEST); 
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
		glColor3f(text->_color.x,text->_color.y,text->_color.z);
		glLoadIdentity();
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

void GL_API::drawButton(Button* button)
{
	pushMatrix();
		glTranslatef(button->_position.x,button->_position.y,button->_position.z);
		glColor3f(button->_color.x,button->_color.y,button->_color.z);
		glBegin(GL_TRIANGLE_STRIP );
			glVertex2f( 0, 0 );
			glVertex2f( button->_dimensions.x, 0 );
			glVertex2f( 0, button->_dimensions.y );
			glVertex2f( button->_dimensions.x, button->_dimensions.y );
		glEnd();
		glBegin( GL_TRIANGLE_STRIP );
			glVertex2f( 1, 1 );
			glVertex2f( button->_dimensions.x-1, 1 );
			glVertex2f( 1, button->_dimensions.y-1 );
			glVertex2f( button->_dimensions.x-1, button->_dimensions.y-1 );  
		glEnd();
	popMatrix();
	
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