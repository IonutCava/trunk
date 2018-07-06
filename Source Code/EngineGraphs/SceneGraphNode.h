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

#ifndef _SCENE_GRAPH_NODE_H_
#define _SCENE_GRAPH_NODE_H_

#include "SceneNode.h"
class SceneRoot : public SceneNode{
public:
	SceneRoot() : SceneNode("root"){}
	void render() {return;}

	bool load(const std::string& name) {return true;}
	bool unload() {return true;}

	bool computeBoundingBox(){return true;}
};


class SceneGraphNode{
	typedef std::tr1::unordered_map<std::string, SceneGraphNode*> NodeChildren;

public:
	SceneGraphNode(SceneNode* node) : _node(node), 
									  _parent(NULL),
									  _grandParent(NULL),
									  _wasActive(true),
									  _active(true){}
	~SceneGraphNode();
	bool unload();
	void render();

	SceneGraphNode* addNode(SceneNode* node);

	SceneGraphNode* findNode(const std::string& name);
	SceneGraphNode* getParent(){return _parent;}
	SceneGraphNode* getGrandParent(){return _grandParent;}
	inline NodeChildren& getChildren() {return _children;}

	void setParent(SceneGraphNode* parent) {_parent = parent;}
	void setGrandParent(SceneGraphNode* grandParent) {_grandParent = grandParent;}

	SceneNode* getNode() {return _node;}

	void setActive(bool state) {_wasActive = _active; _active = state;}
	void restoreActive() {_active = _wasActive;}
	template<class T>
	T* getNode();

private:
	SceneNode* _node;
	NodeChildren _children;
	SceneGraphNode *_parent, *_grandParent;
	bool _active, _wasActive;
};

#endif