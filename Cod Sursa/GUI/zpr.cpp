#include "zpr.h"
#include "PhysX/PhysX.h"
#include "GLUIManager.h"
#include "Utility/Headers/MathHelper.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/ParamHandler.h"

namespace ZPR
{
ParamHandler &par = ParamHandler::getInstance();
static F32 ReferencePoint[4] = { 0,0,0,0};

D32 _left   = 0.0;
D32 _right  = 0.0;
D32 _bottom = 0.0;
D32 _top    = 0.0;
D32 _zNear  = 0.1f;
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

static D32 _matrix[16];
static D32 _matrixInverse[16];
static F32 ratio;
F32 x=0.0f,y=1.75f,z=5.0f;
F32 lx=0.0f,ly=0.0f,lz=-1.0f;

void Init()
{
	getMatrix();
    glutMotionFunc(Motion);
	glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutReshapeFunc(Reshape);
	glutIgnoreKeyRepeat(1);
	glutSpecialFunc(SpecialKeyboard);
	glutSpecialUpFunc(SpecialUpKeyboard);
	par.setParam("zNear",_zNear);
	par.setParam("zFar",_zFar);
}

static void Reshape(int w,int h)
{
	Engine::getInstance().setWindowWidth(w);
    Engine::getInstance().setWindowHeight(h);
    _top    =  1.0;
    _bottom = -1.0;
    _left   = -(D32)w/(D32)h;
    _right  = -_left;

	ratio = 1.0f * w / h;
	// Reset the coordinate system before modifying
	GFXDevice::getInstance().enable_PROJECTION();
	GFXDevice::getInstance().loadIdentityMatrix();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set the clipping volume
	gluPerspective(60,ratio,_zNear,_zFar);
	GFXDevice::getInstance().enable_MODELVIEW();
	GFXDevice::getInstance().loadIdentityMatrix();

	gluLookAt(x, y, z, 
		  	x + lx,y + ly,z + lz,
			0.0f,1.0f,0.0f);
}

static void Mouse(int button, int state, int x, int y)
{
	if(PhysX::getInstance().getDebugRender())
	{
		wasDebugView = true;
		PhysX::getInstance().setDebugRender(false);
	}
   int viewport[4];

   /* Do picking */
   if (state==GLUT_DOWN)
      Pick(x,glutGet(GLUT_WINDOW_HEIGHT)-1-y,3,3);

    _mouseX = x;
    _mouseY = y;

    if (state==GLUT_UP)
        switch (button)
        {
            case GLUT_LEFT_BUTTON: 
				_mouseLeft   = false;
		         
				break;
            case GLUT_MIDDLE_BUTTON: _mouseMiddle = false; break;
            case GLUT_RIGHT_BUTTON:  _mouseRight  = false; break;
        }
    else
        switch (button)
        {
            case GLUT_LEFT_BUTTON:   
				_mouseLeft   = true;
				break;
            case GLUT_MIDDLE_BUTTON: _mouseMiddle = true; break;
            case GLUT_RIGHT_BUTTON:  _mouseRight  = true; break;
        }

    glGetIntegerv(GL_VIEWPORT,viewport);
	pos(&_dragPosX,&_dragPosY,&_dragPosZ,x,y,viewport);

	if(wasDebugView)
	{
		PhysX::getInstance().setDebugRender(true);
		wasDebugView = false;
	}
}

static void Motion(int x, int y)
{
	if(PhysX::getInstance().getDebugRender())
	{
		wasDebugView = true;
		PhysX::getInstance().setDebugRender(false);
	}

    bool changed = false;

    const int dx = x - _mouseX;
    const int dy = y - _mouseY;

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);

    if (dx==0 && dy==0)
        return;

    if (_mouseLeft && _mouseRight)
    {
        F32 s = exp((F32)dy*0.01f);
        GFXDevice::getInstance().translate( ReferencePoint[0], ReferencePoint[1], ReferencePoint[2]);
        GFXDevice::getInstance().scale(s,s,s);
        GFXDevice::getInstance().translate(-ReferencePoint[0],-ReferencePoint[1],-ReferencePoint[2]);

        changed = true;
    }
    else if (_mouseLeft)
    {
            D32 ax,ay,az;
            D32 bx,by,bz;
            D32 angle;

            ax = dy;
            ay = dx;
            az = 0.0;
			angle = Math::vlen(ax,ay,az)/(F32)(viewport[2]+1)*180.0;

            /* Use inverse matrix to determine local axis of rotation */

            bx = _matrixInverse[0]*ax + _matrixInverse[4]*ay + _matrixInverse[8] *az;
            by = _matrixInverse[1]*ax + _matrixInverse[5]*ay + _matrixInverse[9] *az;
            bz = _matrixInverse[2]*ax + _matrixInverse[6]*ay + _matrixInverse[10]*az;

            GFXDevice::getInstance().translate( ReferencePoint[0], ReferencePoint[1], ReferencePoint[2]);
			GFXDevice::getInstance().rotate((F32)angle,(F32)bx,(F32)by,(F32)bz);
            GFXDevice::getInstance().translate(-ReferencePoint[0],-ReferencePoint[1],-ReferencePoint[2]);

            changed = true;
        }
	    else if (_mouseMiddle)
        {
                D32 px,py,pz;

				pos(&px,&py,&pz,x,y,viewport);

                GFXDevice::getInstance().loadIdentityMatrix();
                GFXDevice::getInstance().translate((F32)px-(F32)_dragPosX,(F32)py-(F32)_dragPosY,(F32)pz-(F32)_dragPosZ);
                glMultMatrixd(_matrix);

                _dragPosX = px;
                _dragPosY = py;
                _dragPosZ = pz;

                changed = true;
        }

    _mouseX = x;
    _mouseY = y;

    if (changed)
    {
        getMatrix();
        glutPostRedisplay();
    }
	if(wasDebugView)
	{
		PhysX::getInstance().setDebugRender(true);
	    wasDebugView = false;
    }
	
}

/*****************************************************************
 * Utility functions
 *****************************************************************/

void getMatrix()
{
    glGetDoublev(GL_MODELVIEW_MATRIX,_matrix);
	Math::invertMatrix(_matrix,_matrixInverse);
}



/***************************************** Picking ****************************************************/

void (*selection)(void) = NULL;
void (*pick)(int name) = NULL;

void SelectionFunc(void (*f)(void))
{
    selection = f;
}

void PickFunc(void (*f)(int name))
{
    pick = f;
}

/* Draw in selection mode */

static void
Pick(D32 x, D32 y,D32 delX, D32 delY)
{
   return;
   U32 *buffer = New U32[1024];
   const int bufferSize = sizeof(buffer)/sizeof(U32);

   int    viewport[4];
   D32 projection[16];

   int hits;
   int i,j;

   int  min  = -1;
   U32  minZ = -1;

   glSelectBuffer(bufferSize,buffer);              /* Selection buffer for hit records */
   glRenderMode(GL_SELECT);                        /* OpenGL selection mode            */
   glInitNames();                                  /* Clear OpenGL name stack          */

   GFXDevice::getInstance().enable_PROJECTION();
   GFXDevice::getInstance().pushMatrix();          /* Push current projection matrix   */
   glGetIntegerv(GL_VIEWPORT,viewport);            /* Get the current viewport size    */
   glGetDoublev(GL_PROJECTION_MATRIX,projection);  /* Get the projection matrix        */
   GFXDevice::getInstance().loadIdentityMatrix();  /* Reset the projection matrix      */
   gluPickMatrix(x,y,delX,delY,viewport);          /* Set the picking matrix           */
   glMultMatrixd(projection);                      /* Apply projection matrix          */

   GFXDevice::getInstance().enable_MODELVIEW();

   if (selection)
      selection();                                 /* Draw the scene in selection mode */

   hits = glRenderMode(GL_RENDER);                 /* Return to normal rendering mode  */

   /* Determine the nearest hit */

   if (hits)
   {
      for (i=0,j=0; i<hits; i++)
      {
         if (buffer[j+1]<minZ)
         {
            /* If name stack is empty, return -1                */
            /* If name stack is not empty, return top-most name */

            if (buffer[j]==0)
               min = -1;
            else
               min  = buffer[j+2+buffer[j]];

            minZ = buffer[j+1];
         }

         j += buffer[j] + 3;
      }
   }
   delete[] buffer;
   GFXDevice::getInstance().enable_PROJECTION();
   GFXDevice::getInstance().popMatrix();                    /* Restore projection matrix           */
   GFXDevice::getInstance().enable_MODELVIEW();

   if (pick)
      pick(min);                          /* Pass pick event back to application */
   
}

	void pos(D32 *px,D32 *py,D32 *pz,const int x,const int y,const int *viewport)
	{
		/*
		Use the ortho projection and viewport information
		to map from mouse co-ordinates back into world
		co-ordinates
		*/

		*px = (D32)(x-viewport[0])/(D32)(viewport[2]);
		*py = (D32)(y-viewport[1])/(D32)(viewport[3]);

		*px = _left + (*px)*(_right-_left);
		*py = _top  + (*py)*(_bottom-_top);
		*pz = _zNear;
	}

}
