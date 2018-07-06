/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SCENE_GRAPH_NODE_H_
#define _SCENE_GRAPH_NODE_H_

#define SCENE_GRAPH_PROCESS(pointer) DELEGATE_BIND(&SceneGraph::process(),pointer)

#include "SceneNode.h"
#include <boost/atomic.hpp>

class Transform;
class SceneGraph;
class SceneState;
// This is the scene root node. All scene node's are added to it as child nodes
class SceneRoot : public SceneNode{
public:
    SceneRoot() : SceneNode("root",TYPE_ROOT)
    {
        _renderState.useDefaultMaterial(false);
        setState(RES_LOADED);
    }

    void render(SceneGraphNode* const sgn)             {return;}
    void postLoad(SceneGraphNode* const sgn)           {return;}
    void onDraw(const RenderStage& currentStage)       {return;}
    void updateTransform(SceneGraphNode* const sgn)    {return;}
    bool unload()                                      {return true;}
    bool load(const std::string& name)                 {return true;}
    bool computeBoundingBox(SceneGraphNode* const sgn);
};
// Add as many SceneTransform nodes are needed as parent nodes for any scenenode to create complex transforms in the scene
class SceneTransform : public SceneNode {
public:
    SceneTransform() : SceneNode(TYPE_TRANSFORM)
    {
        _renderState.useDefaultMaterial(false);
        setState(RES_LOADED);
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

    ///Usage context affects lighting, navigation, physics, etc
    enum UsageContext {
        NODE_DYNAMIC = 0,
        NODE_STATIC
    };

    enum NavigationContext {
        NODE_OBSTACLE = 0,
        NODE_IGNORE
    };

    enum PhysicsGroup {
        NODE_COLLIDE_IGNORE = 0,
        NODE_COLLIDE_NO_PUSH,
        NODE_COLLIDE
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
    void updateBoundingBoxTransform(const mat4<F32>& transform);
    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);
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
    ///Find the graph nodes whom's bounding boxes intersects the given ray
    void Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits);

    ///Selection helper functions
            void setSelected(bool state);
    inline 	bool isSelected()	         const {return _selected;}
    inline bool  isSelectable()                 const {return _isSelectable;}
    inline void  setSelectable(const bool state)      {_isSelectable = state;}

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

           const BoundingBox&     getBoundingBoxTransformed();
    inline       BoundingBox&     getBoundingBox()               {ReadLock r_lock(_queryLock); return _boundingBox;}
    inline const BoundingBox&     getBoundingBoxConst()   const  {ReadLock r_lock(_queryLock); return _boundingBox;}
    inline const BoundingBox&     getInitialBoundingBox() const  {ReadLock r_lock(_queryLock); return _initialBoundingBox;}

    vectorImpl<BoundingBox >& getBBoxes(vectorImpl<BoundingBox >& boxes );
    inline   void             updateBB(const bool state)        { _updateBB = state;}
    inline   bool             updateBB()                 const  { return _updateBB;}
    inline   const BoundingSphere&  getBoundingSphere()  const  {ReadLock r_lock(_queryLock); return _boundingSphere;}
    /*Bounding Box Management*/

    /*Transform management*/
    Transform* const getTransform();
               void	 setTransform(Transform* const t);

    Transform* const getPrevTransform();
               void  setPrevTransform(Transform* const t);

    inline     void  silentDispose(const bool state)       {_silentDispose = state;}
    inline     void  useDefaultTransform(const bool state) {_noDefaultTransform = !state;}
    // Animations (if needed)
    const mat4<F32>& getBoneTransform(const std::string& name);
    inline void animationTransforms(const vectorImpl<mat4<F32> >& animationTransforms)       {_animationTransforms = animationTransforms;}
    inline const vectorImpl<mat4<F32> >& animationTransforms()                         const {return _animationTransforms;}
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
    
    inline const PhysicsGroup&      getPhysicsGroup()          const {return _physicsCollisionGroup;}
    inline const UsageContext&      getUsageContext()          const {return _usageContext;}
    inline const NavigationContext& getNavigationContext()     const {return _navigationContext;}
    inline       bool               getNavMeshDetailOverride() const {return _overrideNavMeshDetail;}

    inline void  setPhysicsGroup(const PhysicsGroup& newGroup)             {_physicsCollisionGroup = newGroup;}
    inline void  setUsageContext(const UsageContext& newContext)           {_usageContext = newContext;}
           void  setNavigationContext(const NavigationContext& newContext);
           void  setNavigationDetailOverride(const bool detailOverride);

    void cookCollisionMesh(const std::string& sceneName);
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
    bool _selected;
    bool _isSelectable;
    bool _wasActive;
    bool _noDefaultTransform;
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
    Transform*  _transformPrevious;

    U32 _childQueue;
    D32 _updateTimer;
    std::string _name;
    mutable SharedLock _queryLock;

    PhysicsGroup _physicsCollisionGroup;
    UsageContext _usageContext;
    NavigationContext _navigationContext;
    bool              _overrideNavMeshDetail;
    ///Animations (current and previous for interpolation)
    vectorImpl<mat4<F32> > _animationTransforms;
    vectorImpl<mat4<F32> > _animationTransformsPrev;
};

#endif