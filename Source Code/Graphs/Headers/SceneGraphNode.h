/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#define SCENE_GRAPH_PROCESS(pointer) boost::bind(&SceneGraph::process(),pointer)

#include "SceneNode.h"
#include <boost/atomic.hpp>

class SceneGraph;
class SceneRoot : public SceneNode{
public:
	SceneRoot() : SceneNode("root",TYPE_ROOT){_renderState.useDefaultMaterial(false);}
	void render(SceneGraphNode* const sgn) {return;}

	bool load(const std::string& name) {return true;}
	void postLoad(SceneGraphNode* const sgn) {};	
	bool unload() {return true;}
	void onDraw() {};
	bool computeBoundingBox(SceneGraphNode* const sgn) {return true;}
	void updateTransform(SceneGraphNode* const sgn) {}
};

class SceneGraphNode{
public:
    ///Usage context affects lighting, navigation,etc
	enum UsageContext {
		NODE_DYNAMIC = 0, 
		NODE_STATIC
	};

    enum NavigationContext {
        NODE_OBSTACLE = 0,
        NODE_IGNORE
    };

	typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
	SceneGraphNode(SceneNode* node);

	~SceneGraphNode();
	bool unload();
	/// Recursivelly print information about this SGN and all it's children
	void print();
	/// Update bounding boxes
	void checkBoundingBoxes();
	/// Position, rotation, scale updates
	void updateTransforms();
    /// Apply current transform to the node's BB
    void updateBoundingBoxTransform();
	/// Culling and visibility checks
	void updateVisualInformation();
	/// Called from SceneGraph "sceneUpdate"
	void sceneUpdate(U32 sceneTime);
	/*Node Management*/
	template<class T>
	///Always use the level of redirection needed to reduce virtual function overhead
	///Use getNode<SceneNode> if you need material properties for ex. or getNode<SubMesh> for animation transforms
	inline T* getNode() {assert(_node != NULL); return dynamic_cast<T*>(_node);}

    SceneGraphNode* addNode(SceneNode* const node,const std::string& name = "");
	void			removeNode(SceneGraphNode* node);
	///Find a node in the graph based on the SceneGraphNode's name
	///If sceneNodeName = true, find a node in the graph based on the SceneNode's name
	SceneGraphNode* findNode(const std::string& name, bool sceneNodeName = false);
	///Find the graph node whom's bounding box intersects the given ray
	SceneGraphNode* Intersect(const Ray& ray, F32 start, F32 end);
const  std::string& getName(){return _name;}
/*Node Management*/

/*Parent <-> Children*/
inline       SceneGraphNode*  getParent(){return _parent;}
inline       SceneGraphNode*  getGrandParent(){return _grandParent;}
inline       NodeChildren&    getChildren() {return _children;}
	         void             setParent(SceneGraphNode* parent);
inline       void             setGrandParent(SceneGraphNode* grandParent) {_grandParent = grandParent;}
/*Parent <-> Children*/

/*Bounding Box Management*/
inline   	 void    		  setInitialBoundingBox(BoundingBox& initialBoundingBox){WriteLock w_lock(_queryLock); _initialBoundingBox = initialBoundingBox;}
inline const BoundingBox&     getInitialBoundingBox()		  {ReadLock r_lock(_queryLock); return _initialBoundingBox;}
inline       BoundingBox&     getBoundingBox()                {ReadLock r_lock(_queryLock); return _boundingBox;}
inline       void             updateBoundingBox(const BoundingBox& box) {WriteLock w_lock(_queryLock); _boundingBox = box;}  

vectorImpl<BoundingBox >&     getBBoxes(vectorImpl<BoundingBox >& boxes );
inline       void             updateBB(bool state) { _updateBB = state;}
inline       bool             updateBB()           { return _updateBB;}
inline       BoundingSphere&  getBoundingSphere()  {ReadLock r_lock(_queryLock); return _boundingSphere;}         
/*Bounding Box Management*/

/*Transform management*/
	          void			  setTransform(Transform* const t);
			 Transform*	const getTransform();
inline       void             useDefaultTransform(bool state) {_noDefaultTransform = !state;}
inline       void             silentDispose(bool state) {_silentDispose = state;}
/*Transform management*/

/*Node State*/
inline   	 void             setActive(bool state) {_wasActive = _active; _active = state;}
inline       bool             isActive() {return _active;}
inline       void             restoreActive() {_active = _wasActive;}
inline       void			  scheduleDeletion(){_shouldDelete = true;}
inline       bool             isReady(){return _isReady;}
/*Node State*/

inline       U32              getChildQueue() {return _childQueue;}
inline       void             incChildQueue() {_childQueue++;}
inline       void             decChildQueue() {_childQueue--;}

inline  UsageContext              getUsageContext() const {return _usageContext;}
inline  void                      setUsageContext(UsageContext newContext) {_usageContext = newContext;}
inline  NavigationContext         getNavigationContext() const {return _navigationContext;}
inline  void                      setNavigationContext(NavigationContext newContext) {_navigationContext = newContext;}

private:
	inline void setName(const std::string& name){_name = name;}

private:
	SceneNode* _node;
	NodeChildren _children;
	SceneGraphNode *_parent, *_grandParent;
    boost::atomic<bool> _active;
    bool _wasActive;
	bool _noDefaultTransform;
	bool _inView;
	bool _sorted;
	bool _silentDispose;
    boost::atomic<bool> _updateBB;
    boost::atomic<bool> _isReady; //<is created and has a valid transform
	bool _shouldDelete;
	///_initialBoundingBox is a copy of the initialy calculate BB for transformation
	///it should be copied in every computeBoungingBox call;
	BoundingBox _initialBoundingBox;
	BoundingBox _boundingBox; 
	BoundingSphere _boundingSphere; ///<For faster visibility culling

	Transform*	_transform;
	U32 _childQueue,_updateTimer;
	std::string _name;
	mutable SharedLock _queryLock;

    UsageContext _usageContext;
    NavigationContext _navigationContext;
};

#endif