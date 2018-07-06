#include "SceneManager.h"

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