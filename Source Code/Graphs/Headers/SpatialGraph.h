/*�Copyright 2009-2013 DIVIDE-Studio�*/
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

class SpatialGraph  {
public:
	SpatialGraph(){
		_root = New SceneGraphNode(new SceneRoot);
	}

	~SceneGraph(){
		_root->unload();
		/// Should recursivelly call delete on the entire scene
		SAFE_DELETE(_root);
	}

	inline SceneGraphNode* getRoot(){ return _root; }

	inline SceneGraphNode* findNode(const std::string& name){
		return _root->findNode(name);
	}
	
	inline void render() {	_root->render(); }

private:
	SceneGraphNode* _root;
	//SpatialHierarchyTree _spatialTree; ///< For HSR
};
#endif