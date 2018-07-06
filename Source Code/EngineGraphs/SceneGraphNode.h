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
#include "Hardware/Video/GFXDevice.h"

class SceneGraph;
class SceneRoot : public SceneNode{
public:
	SceneRoot() : SceneNode("root"){useDefaultMaterial(false);}
	void render(SceneGraphNode* node) {return;}

	bool load(const std::string& name) {;return true;}
	void postLoad(SceneGraphNode* node) {};	
	bool unload() {return true;}
	void onDraw() {};
	bool computeBoundingBox(SceneGraphNode* node){return true;}
	void createCopy(){}
	void removeCopy(){}
};
//Each render state could be attributed to any number of SceneGraphNodes.
//After sorting by shader, we start rendring each VisualNodePropertie's parents as long as they are visible
class VisualNodeProperties{
	RenderState& getRenderState() {return _s;}
	void         addParent(SceneGraphNode* node) {_parents.push_back(node);}
	void         removeParent(const std::string& name);
private:
	RenderState _s;
	std::vector<SceneGraphNode*> _parents;
};

class SceneGraphNode{
	typedef unordered_map<std::string, SceneGraphNode*> NodeChildren;
public:

	SceneGraphNode(SceneNode* node);

	~SceneGraphNode();
	bool unload();
	void print();
	void updateTransformsAndBounds();
	void updateVisualInformation();
/*Node Management*/
template<class T>
       T*                     getNode();
inline SceneNode*             getNode() {return _node;}
       SceneGraphNode*        addNode(SceneNode* node,const std::string& name = "");
	   void			          removeNode(SceneNode* node);
	   SceneGraphNode*        findNode(const std::string& name);
const  std::string&           getName(){return _name;}
/*Node Management*/

/*Parent <-> Children*/
inline       SceneGraphNode*  getParent(){return _parent;}
inline       SceneGraphNode*  getGrandParent(){return _grandParent;}
inline       NodeChildren&    getChildren() {return _children;}
	         void             setParent(SceneGraphNode* parent);
inline       void             setGrandParent(SceneGraphNode* grandParent) {_grandParent = grandParent;}
/*Parent <-> Children*/

/*Bounding Box Management*/
inline   	 void    		  setInitialBoundingBox(BoundingBox& initialBoundingBox){_initialBoundingBox = initialBoundingBox;}
inline const BoundingBox&     getInitialBoundingBox()		  {return _initialBoundingBox;}
inline       BoundingBox&	  getBoundingBox()                {return _boundingBox;}
/*Bounding Box Management*/

/*Transform management*/
inline       void			  setTransform(Transform* t) { if(_transform) delete _transform; _transform = t;}
			 Transform*		  getTransform();
inline       void             useDefaultTransform(bool state) {_noDefaultTransform = !state;}
inline       void             silentDispose(bool state) {_silentDispose = state;}
/*Transform management*/

/*Node State*/
inline   	 void             setActive(bool state) {_wasActive = _active; _active = state;}
inline       bool             isActive() {return _active;}
inline       void             restoreActive() {_active = _wasActive;}
/*Node State*/

inline       U32              getChildQueue() {return _childQueue;}
inline       void             incChildQueue() {_childQueue++;}
inline       void             decChildQueue() {_childQueue--;}

private:
	inline void setName(const std::string& name){_name = name;}

private:
	SceneNode* _node;
	NodeChildren _children;
	SceneGraphNode *_parent, *_grandParent;
	bool _active, _wasActive,_noDefaultTransform,_inView, _sorted, _silentDispose;

	//_initialBoundingBox is a copy of the initialy calculate BB for transformation
	//it should be copied in every computeBoungingBox call;
	BoundingBox _initialBoundingBox, _boundingBox; 
	Transform*	_transform;
	SceneGraph* _sceneGraph;
	U32 _childQueue,_updateTimer;
	std::string _name;
};

#endif