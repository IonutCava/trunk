/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

class SceneGraph  {
	typedef unordered_map<std::string/*Shader Name*/, std::vector<SceneGraphNode*>/*Objects bound to this shader*/> object_shader_map;
	public:
	
	SceneGraph(){
		_root = New SceneGraphNode(New SceneRoot);
	}

	~SceneGraph(){
		_root->unload();
		delete _root; //Should recursivelly call delete on the entire scene
		_root = NULL;
	}

	SceneGraphNode* getRoot(){ return _root; }

	SceneGraphNode* findNode(const std::string& name){
		return _root->findNode(name);
	}
	
	void render();
	inline object_shader_map& getObjectShaderMap() {return _objectShaderMap;}
	void print();
private:
	SceneGraphNode* _root;
	 object_shader_map _objectShaderMap;
	//SpatialHierarchyTree _spatialTree; //For HSR
};
#endif