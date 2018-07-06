/*�Copyright 2009-2012 DIVIDE-Studio�*/
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
class Ray;
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

	inline  SceneGraphNode* getRoot(){ 
		return _root; 
	}

	inline  std::vector<BoundingBox >& getBBoxes(){
		return _root->getBBoxes(_boundingBoxes);
	}

	inline  SceneGraphNode* findNode(const std::string& name,bool sceneNodeName = false){
		return _root->findNode(name,sceneNodeName);
	}
	
	void render();
	/// Update transforms and bounding boxes
	void update();
	/// Update all nodes. Called from "updateSceneState" from class Scene
	void sceneUpdate(D32 sceneTime);

	void print();

	void startUpdateThread();

	void idle();

	SceneGraphNode* Intersect(const Ray& ray, F32 start, F32 end);
	void addToDeletionQueue(SceneGraphNode* node) {_pendingDeletionNodes.push_back(node);}

private:
	boost::mutex    _rootAccessMutex; 
	SceneGraphNode* _root;
	Scene*          _scene;
	bool            _updateRunning;
	std::vector<BoundingBox>        _boundingBoxes;
	std::vector<SceneGraphNode* >   _pendingDeletionNodes;
};
#endif