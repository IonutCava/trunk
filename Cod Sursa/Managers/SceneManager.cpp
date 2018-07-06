#include "SceneManager.h"
#include "Rendering/Camera.h"
#include "Rendering/common.h"

Scene* SceneManager::findScene(const std::string& name)
{
	_sceneIter =  _scenes.find(name);

	if ( _sceneIter != _scenes.end() ) return _sceneIter->second;
    else return NULL;
}

void SceneManager::toggleBoundingBoxes()
{
	if(_scene->drawBBox() && _scene->drawObjects())
	{
		_scene->drawBBox() = false;
		_scene->drawObjects() = true;
	}
	else if (!_scene->drawBBox() && _scene->drawObjects())
	{
		_scene->drawBBox() = true;
		_scene->drawObjects() = false;
	}
	else
	{
		_scene->drawBBox() = true;
		_scene->drawObjects() = true;
	}
}

void SceneManager::findSelection(int x, int y)
{
	ParamHandler& par = ParamHandler::getInstance();
    float value_fov = 0.7853f;    //this is 45 degrees converted to radians
    float value_aspect = (F32)Engine::getInstance().getWindowWidth() / (F32)Engine::getInstance().getWindowHeight();
	float half_window_width = Engine::getInstance().getWindowWidth() / 2.0f;
	float half_window_height = Engine::getInstance().getWindowHeight() / 2.0f;

    float modifier_x;
    float modifier_y;
        //mathematical handling of the difference between
        //your mouse position and the 'center' of the window

    float point[3];
        //the untransformed ray will be put here

    float point_dist = par.getParam<F32>("zFar");
        //it'll be put this far on the Z plane

    float camera_origin[3];
        //this is where the camera sits, in 3dspace

    float point_xformed[3];
        //this is the transformed point

    GLfloat pulledMatrix[16];
        //pulledMatrix is the OpenGL matrix.

    float final_point[3];
    float color[4] = { 0.0, 1.0, 0.0, 1.0 };

    //These lines are the biggest part of this function.
    //This is where the mouse position is turned into a mathematical
    //'relative' of 3d space. The conversion to an actual point
    modifier_x = (F32)tan( value_fov * 0.5f )
        * (( 1.0f - x / half_window_width ) * ( value_aspect ) );
    modifier_y = (F32)tan( value_fov * 0.5f )
        * -( 1.0f - y / half_window_height );

    //These 3 take our modifier_x/y values and our 'casting' distance
    //to throw out a point in space that lies on the point_dist plane.
    //If we were using completely untransformed, untranslated space,
    //this would be fine - but we're not :)
    point[0] = modifier_x * point_dist;
    point[1] = modifier_y * point_dist;
    point[2] = point_dist;

    //Next we make an openGL call to grab our MODELVIEW_MATRIX -
    //This is the matrix that rasters 3d points to 2d space - which is
    //kinda what we're doing, in reverse
    glGetFloatv(GL_MODELVIEW_MATRIX, pulledMatrix);
    //Some folks would then invert the matrix - I invert the results.

    //First, to get the camera_origin, we transform the 12, 13, 14
    //slots of our pulledMatrix - this gets us the actual viewing
    //position we are 'sitting' at when the function is called
    camera_origin[0] = -(
        pulledMatrix[0] * pulledMatrix[12] +
        pulledMatrix[1] * pulledMatrix[13] +
        pulledMatrix[2] * pulledMatrix[14]);
    camera_origin[1] = -(
        pulledMatrix[4] * pulledMatrix[12] +
        pulledMatrix[5] * pulledMatrix[13] +
        pulledMatrix[6] * pulledMatrix[14]);
    camera_origin[2] = -(
        pulledMatrix[8] * pulledMatrix[12] +
        pulledMatrix[9] * pulledMatrix[13] +
        pulledMatrix[10] * pulledMatrix[14]);

    //Second, we transform the position we generated earlier - the '3d'
    //mouse position - by our viewing matrix.
    point_xformed[0] = -(
        pulledMatrix[0] * point[0] +
        pulledMatrix[1] * point[1] +
        pulledMatrix[2] * point[2]);
    point_xformed[1] = -(
        pulledMatrix[4] * point[0] +
        pulledMatrix[5] * point[1] +
        pulledMatrix[6] * point[2]);
    point_xformed[2] = -(
        pulledMatrix[8] * point[0] +
        pulledMatrix[9] * point[1] +
        pulledMatrix[10] * point[2]);

    final_point[0] = point_xformed[0] + camera_origin[0];
    final_point[1] = point_xformed[1] + camera_origin[1];
    final_point[2] = point_xformed[2] + camera_origin[2];

	vector<DVDFile* >::iterator it;
	vec3 origin = vec3(Camera::getInstance().getEye().x,Camera::getInstance().getEye().y,Camera::getInstance().getEye().z);
	vec3 dir = origin.direction(vec3(final_point[0],final_point[1],final_point[2]));
	
	Ray r(origin,dir);

	for(it = getModelArray().begin(); it != getModelArray().end(); it++)
	{
		if((*it)->getBoundingBox().intersect(r,0.1f,4500))
		{
			(*it)->setSelected(true);
			cout << "Selected: " << (*it)->getName() << endl;
			break;
		}
	}
}