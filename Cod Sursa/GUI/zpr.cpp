#include "zpr.h"
#include "GUI.h"

#include "PhysX/PhysX.h"
#include "GLUIManager.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"
#include "Rendering/common.h"
#include "Managers/SceneManager.h"

namespace ZPR
{
ParamHandler &par = ParamHandler::getInstance();
static F32 ReferencePoint[4] = { 0,0,0,0};

D32 _left   = 0.0;
D32 _right  = 0.0;
D32 _bottom = 0.0;
D32 _top    = 0.0;
D32 _zNear  = 0.01f;
D32 _zFar   = 7000.0f;

static int		gMouseX = 0;
static int		gMouseY = 0;


static bool wasDebugView = false;

static int  _mouseX      = 0;
static int  _mouseY      = 0;
static bool _mouseLeft   = false;
static bool _mouseMiddle = false;
static bool _mouseRight  = false;

static D32 _dragPosX  = 0.0;
static D32 _dragPosY  = 0.0;
static D32 _dragPosZ  = 0.0;
static F32 ratio;
F32 x=0.0f,y=1.75f,z=5.0f;
F32 lx=0.0f,ly=0.0f,lz=-1.0f;

void Init()
{
    glutMotionFunc(Motion);
	glutPassiveMotionFunc(MouseMove); 
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
    glutMouseFunc(Mouse);
    glutReshapeFunc(Reshape);
	glutIgnoreKeyRepeat(1);
	glutSpecialFunc(SpecialKeyboard);
	glutSpecialUpFunc(SpecialUpKeyboard);
	par.setParam("zNear",_zNear);
	par.setParam("zFar",_zFar);
}

void Reshape(int w,int h)
{
	Engine::getInstance().setWindowWidth(w);
    Engine::getInstance().setWindowHeight(h);
    _top    =  1.0;
    _bottom = -1.0;
    _left   = -(D32)w/(D32)h;
    _right  = -_left;

	ratio = 1.0f * w / h;
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

static void Mouse(int button, int state, int x, int y)
{
    _mouseX = x;
    _mouseY = y;
	int window_height = Engine::getInstance().getWindowHeight();
	int window_width = Engine::getInstance().getWindowWidth();
	if (state==GLUT_UP)
        switch (button)
        {
            case GLUT_LEFT_BUTTON: 
			{
				_mouseLeft   = false;
				SceneManager::getInstance().findSelection(x,y);
				GUI::getInstance().clickReleaseCheck();
				break;

			}
            case GLUT_MIDDLE_BUTTON: _mouseMiddle = false; break;
            case GLUT_RIGHT_BUTTON:  _mouseRight  = false; break;
        }
    else
        switch (button)
        {
            case GLUT_LEFT_BUTTON:
			{
				_mouseLeft   = true;	
				GUI::getInstance().clickCheck();
				break;
			}
            case GLUT_MIDDLE_BUTTON: _mouseMiddle = true; break;
            case GLUT_RIGHT_BUTTON:  _mouseRight  = true; break;
        }
}

static void MouseMove(int x, int y)
{
	GUI::getInstance().checkItem(x,y);
}

static void Motion(int x, int y)
{
    const int dx = x - _mouseX;
    const int dy = y - _mouseY;

    if (dx==0 && dy==0)
        return;

    if (_mouseLeft && _mouseRight)
    {
    }
    else if (_mouseLeft)
    {
    }
    else if (_mouseMiddle)
    {
    }

    _mouseX = x;
    _mouseY = y;

	
}


};