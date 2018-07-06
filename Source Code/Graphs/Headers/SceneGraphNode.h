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

#define SCENE_GRAPH_PROCESS(pointer) DELEGATE_BIND(&SceneGraph::process(),pointer)

#include "SceneNode.h"
#include <boost/atomic.hpp>

class Transform;
class SceneGraph;
class SceneRoot : public SceneNode{
public:
	SceneRoot() : SceneNode("root",TYPE_ROOT)
	{
		_renderState.useDefaultMaterial(false);
	}

	void render(SceneGraphNode* const sgn)             {return;}
	void postLoad(SceneGraphNode* const sgn)           {return;}
	void onDraw(const RenderStage& currentStage)       {return;}
	void updateTransform(SceneGraphNode* const sgn)    {return;}
	bool unload()                                      {return true;}
	bool load(const std::string& name)                 {return true;}
    bool computeBoundingBox(SceneGraphNode* const sgn) {return true;}

};

class SceneGraphNode{
public:
	typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;

    ///Usage context affects lighting, navigation,etc
	enum UsageContext {
		NODE_DYNAMIC = 0,
		NODE_STATIC
	};

    enum NavigationContext {
        NODE_OBSTACLE = 0,
        NODE_IGNORE
    };

	SceneGraphNode(SceneNode* const node);
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
	void sceneUpdate(const U32 sceneTime);
	/*Node Management*/
	template<class T>
	///Always use the level of redirection needed to reduce virtual function overhead
	///Use getNode<SceneNode> if you need material properties for ex. or getNode<SubMesh> for animation transforms
	inline T* getNode() const {assert(_node != NULL); return dynamic_cast<T*>(_node);}

    SceneGraphNode* addNode(SceneNode* const node,const std::string& name = "");
	void			removeNode(SceneGraphNode* node);
	///Find a node in the graph based on the SceneGraphNode's name
	///If sceneNodeName = true, find a node in the graph based on the SceneNode's name
	SceneGraphNode* findNode(const std::string& name, bool sceneNodeName = false);
	///Find the graph node whom's bounding box intersects the given ray
	SceneGraphNode* Intersect(const Ray& ray, F32 start, F32 end);

	const  std::string& getName() const {return _name;}
	/*Node Management*/

	/*Parent <-> Children*/
				SceneGraphNode*  getRoot()        const;
    inline      SceneGraphNode*  getParent()      const {return _parent;}
	inline      SceneGraphNode*  getGrandParent() const {return _grandParent;}
	inline      NodeChildren&    getChildren()          {return _children;}
	inline      void             setGrandParent(SceneGraphNode* const grandParent) {_grandParent = grandParent;}
				void             setParent(SceneGraphNode* const parent);
	/*Parent <-> Children*/

	/*Bounding Box Management*/
	inline void setInitialBoundingBox(const BoundingBox& initialBoundingBox){
		WriteLock w_lock(_queryLock);
		_initialBoundingBox = initialBoundingBox;
	}

	inline void updateBoundingBox(const BoundingBox& box) {
		WriteLock w_lock(_queryLock);
		_boundingBox = box;
	}

	inline       BoundingBox&     getBoundingBox()         {ReadLock r_lock(_queryLock); return _boundingBox;}
	inline const BoundingBox&     getInitialBoundingBox()  {ReadLock r_lock(_queryLock); return _initialBoundingBox;}
	
	vectorImpl<BoundingBox >& getBBoxes(vectorImpl<BoundingBox >& boxes );
	inline   void             updateBB(const bool state)        { _updateBB = state;}
	inline   bool             updateBB()                 const  { return _updateBB;}
	inline   const BoundingSphere&  getBoundingSphere()  const  {ReadLock r_lock(_queryLock); return _boundingSphere;}
	/*Bounding Box Management*/

	/*Transform management*/
	Transform* const getTransform();
	           void	 setTransform(Transform* const t);
	inline     void  silentDispose(const bool state)       {_silentDispose = state;}
	inline     void  useDefaultTransform(const bool state) {_noDefaultTransform = !state;}
	/// Animations (if needed)
	inline     void  animationTransforms(const vectorImpl<mat4<F32> >& animationTransforms) {_animationTransforms = animationTransforms;}
    vectorImpl<mat4<F32> >& animationTransforms()                                           {return _animationTransforms;}
	/*Transform management*/

	/*Node State*/
	inline void setActive(bool state) {_wasActive = _active; _active = state;}
	inline void restoreActive()       {_active = _wasActive;}
	inline void	scheduleDeletion()    {_shouldDelete = true;}

	inline bool isReady()  const {return _isReady;}
	inline bool isActive() const {return _active;}
	/*Node State*/

	inline U32  getChildQueue() const {return _childQueue;}
	inline void incChildQueue()       {_childQueue++;}
	inline void decChildQueue()       {_childQueue--;}

	inline const UsageContext&      getUsageContext()      const {return _usageContext;}
	inline const NavigationContext& getNavigationContext() const {return _navigationContext;}

	inline void  setUsageContext(const UsageContext& newContext)           {_usageContext = newContext;}
	       void  setNavigationContext(const NavigationContext& newContext);
	
    void addBoundingBox(const BoundingBox& bb, const SceneNodeType& type);
    void setBBExclusionMask(U32 bbExclusionMask) {_bbAddExclusionList = bbExclusionMask;}


private:
	inline void setName(const std::string& name){_name = name;}

protected:
    friend class SceneGraph;
    void setSceneGraph(SceneGraph* const sg) {_sceneGraph = sg;}

private:
	SceneNode* _node;
	NodeChildren _children;
	SceneGraphNode *_parent, *_grandParent;
    SceneGraph     *_sceneGraph;

    boost::atomic<bool> _active;
	//Used to skip certain BB's (sky, ligts, etc);
    U32 _bbAddExclusionList;
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

	///Animations
	vectorImpl<mat4<F32> > _animationTransforms;
};

#endif