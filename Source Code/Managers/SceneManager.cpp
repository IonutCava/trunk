#include "Headers/SceneManager.h"
#include "Headers//CameraManager.h"
#include "Core/Headers/Application.h"
#include "Rendering/Headers/Frustum.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "SceneList.h"

SceneManager::SceneManager() : _scene(NULL), _currentSelection(NULL){}

SceneManager::~SceneManager(){
	PRINT_FN("Deleting Scene Manager ...");
	PRINT_FN("Removing all scenes and destroying scene manager ...");
	SceneMap::iterator& it = _sceneMap.begin();
	for_each(SceneMap::value_type& it, _sceneMap){
		it.second->unload();
		SAFE_DELETE(it.second);
	}
	_sceneMap.clear();
}

Scene* SceneManager::loadScene(const std::string& name){
	Scene* scene = NULL;
	if(name.compare("MainScene") == 0){
		scene = New MainScene();
		_sceneMap.insert(std::make_pair("MainScene", scene ));	
	}else if(name.compare("CubeScene") == 0){
		scene = New CubeScene();
		_sceneMap.insert(std::make_pair("CubeScene", scene));
	}else if(name.compare("NetworkScene") == 0){
		scene = New NetworkScene();
		_sceneMap.insert(std::make_pair("NetworkScene", scene));
	}else if(name.compare("PingPongScene") == 0){
		scene = New PingPongScene();
		_sceneMap.insert(std::make_pair("PingPongScene", scene));
	}else if(name.compare("FlashScene") == 0){
		scene = New FlashScene();
		_sceneMap.insert(std::make_pair("FlashScene", scene));
	}else if(name.compare("AITenisScene") == 0){
		scene = New AITenisScene();
		_sceneMap.insert(std::make_pair("AITenisScene", scene));
	}else if(name.compare("PhysXScene") == 0){
		scene = New PhysXScene();
		_sceneMap.insert(std::make_pair("PhysXScene", scene));
	}else if(name.compare("WarScene") == 0){
		scene = New WarScene();
		_sceneMap.insert(std::make_pair("WarScene", scene));
	}else{
		scene = NULL;
	}
	return scene;
}

void SceneManager::registerScene(Scene* scenePointer){
	assert(scenePointer != NULL);
	std::pair<unordered_map<std::string, Scene*>::iterator, bool > result;
	///try and add the new scene
	result = _sceneMap.insert(std::make_pair(scenePointer->getName(), scenePointer));
	///if we fail ...
	if(!result.second){
		///unload and delete the old scene with the same name and register the new one
		(result.first)->second->unload();
		SAFE_UPDATE((result.first)->second, scenePointer);
	}
}

void SceneManager::toggleBoundingBoxes(){
	if(!_scene->drawBBox() && _scene->drawObjects())	{
		_scene->drawBBox(true);
		_scene->drawObjects(true);
	}else if (_scene->drawBBox() && _scene->drawObjects()){
		_scene->drawBBox(true);
		_scene->drawObjects(false);
	}else if (_scene->drawBBox() && !_scene->drawObjects()){
		_scene->drawBBox(false);
		_scene->drawObjects(false);
	}else{
		_scene->drawBBox(false);
		_scene->drawObjects(true);
	}
}

void SceneManager::deleteSelection(){
	if(_currentSelection != NULL){
		_currentSelection->scheduleDeletion();
	}
}

void SceneManager::findSelection(U32 x, U32 y){
	ParamHandler& par = ParamHandler::getInstance();
    F32 value_fov = 0.7853f;    //this is 45 degrees converted to radians
    F32 value_aspect = par.getParam<F32>("aspectRatio");
	F32 half_window_width = Application::getInstance().getWindowDimensions().width / 2.0f;
	F32 half_window_height = Application::getInstance().getWindowDimensions().height / 2.0f;

    F32 modifier_x, modifier_y;
        //mathematical handling of the difference between
        //your mouse position and the 'center' of the window

    vec3<F32> point;
        //the untransformed ray will be put here

    F32 point_dist = par.getParam<F32>("zFar");
        //it'll be put this far on the Z plane

    vec3<F32> camera_origin;
        //this is where the camera sits, in 3dspace

    vec3<F32> point_xformed;
        //this is the transformed point

    vec3<F32> final_point;
    vec4<F32> color(0.0, 1.0, 0.0, 1.0);

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
    point.x = modifier_x * point_dist;
    point.y = modifier_y * point_dist;
    point.z = point_dist;

    //Next we make an openGL call to grab our MODELVIEW_MATRIX -
    //This is the matrix that rasters 3d points to 2d space - which is
    //kinda what we're doing, in reverse
	mat4<F32> temp = Frustum::getInstance().getModelviewMatrix();
	
    //Some folks would then invert the matrix - I invert the results.

    //First, to get the camera_origin, we transform the 12, 13, 14
    //slots of our pulledMatrix - this gets us the actual viewing
    //position we are 'sitting' at when the function is called
    camera_origin.x = -(
        temp.mat[0] * temp.mat[12] +
        temp.mat[1] * temp.mat[13] +
        temp.mat[2] * temp.mat[14]);
    camera_origin.y = -(
        temp.mat[4] * temp.mat[12] +
        temp.mat[5] * temp.mat[13] +
        temp.mat[6] * temp.mat[14]);
    camera_origin.z = -(
        temp.mat[8] * temp.mat[12] +
        temp.mat[9] * temp.mat[13] +
        temp.mat[10] * temp.mat[14]);

    //Second, we transform the position we generated earlier - the '3d'
    //mouse position - by our viewing matrix.
    point_xformed.x = -(
        temp.mat[0] * point[0] +
        temp.mat[1] * point[1] +
        temp.mat[2] * point[2]);
    point_xformed.y = -(
        temp.mat[4] * point[0] +
        temp.mat[5] * point[1] +
        temp.mat[6] * point[2]);
    point_xformed.z = -(
        temp.mat[8] * point[0] +
        temp.mat[9] * point[1] +
        temp.mat[10] * point[2]);

    final_point = point_xformed + camera_origin;

	vec3<F32> origin(CameraManager::getInstance().getActiveCamera()->getEye());
	vec3<F32> dir = origin.direction(final_point);
	
	//ToDo: fix this!!!!! -Ionut
	/*Ray r(origin,dir);
	_currentSelection = NULL;
	for(unordered_map<string, Object3D* >::iterator it = getGeometryArray().begin(); 
													 it != getGeometryArray().end(); ++it){
		assert(it->second != NULL);
		(it->second)->setSelected(false);
		
		if((it->second)->getBoundingBox().intersect(r,par.getParam<F32>("zNear"),par.getParam<F32>("zFar")/2.0f)){
			(it->second)->setSelected(true);
			_currentSelection = it->second;
			break;
		}
	}*/
}

void SceneManager::render(RENDER_STAGE stage) {
	GFX_DEVICE.setRenderStage(stage);
	_scene->render();
}