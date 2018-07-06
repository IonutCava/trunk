/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCENE_GRAPH_H_
#define _SCENE_GRAPH_H_

#include "SceneGraphNode.h"

class Scene;
class SceneGraph  {
	public:
	
	SceneGraph();

	~SceneGraph(){
		PRINT_FN("Deleting SceneGraph");

		_root->unload(); ///< Should recursivelly call unload on the entire scene graph
		///Should recursivelly call delete on the entire scene graph
		SAFE_DELETE(_root);
	}

	SceneGraphNode* getRoot(){ return _root; }
	std::vector<BoundingBox >& getBBoxes(){
		return _root->getBBoxes(_boundingBoxes);
	}
	SceneGraphNode* findNode(const std::string& name,bool sceneNodeName = false){
		return _root->findNode(name,sceneNodeName);
	}
	
	void render();
	void update();
	void print();
	void startUpdateThread();

private:
	boost::mutex    _rootAccessMutex; 
	SceneGraphNode* _root;
	Scene*          _scene;
	bool            _updateRunning;
	std::vector<BoundingBox> _boundingBoxes;
};
#endif