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
#include "Utility/Headers/StateTracker.h"
#include "Graphs/Components/Headers/SGNComponent.h"
#include "Graphs/Components/Headers/PhysicsComponent.h"
#include "Graphs/Components/Headers/AnimationComponent.h"
#include "Graphs/Components/Headers/NavigationComponent.h"

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

    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage)       { return true; }
    bool unload()                                      {return true;}
    bool load(const std::string& name)                 {return true;}
    bool computeBoundingBox(SceneGraphNode* const sgn);

protected:
    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState) {}
    void postLoad(SceneGraphNode* const sgn)           { SceneNode::postLoad(sgn); }
};

// Add as many SceneTransform nodes are needed as parent nodes for any scenenode to create complex transforms in the scene
class SceneTransform : public SceneNode {
public:
    SceneTransform() : SceneNode(TYPE_TRANSFORM)
    {
        _renderState.useDefaultMaterial(false);
        setState(RES_LOADED);
    }
    
    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState) { return; }
    void postLoad(SceneGraphNode* const sgn)           {return;}
    bool onDraw(const RenderStage& currentStage)       {return true;}
    bool unload()                                      {return true;}
    bool load(const std::string& name)                 {return true;}
    bool computeBoundingBox(SceneGraphNode* const sgn) {return true;}
};

class SceneGraphNode : public GUIDWrapper, private boost::noncopyable{
public:
    typedef Unordered_map<std::string, SceneGraphNode*> NodeChildren;
    typedef Unordered_map<SGNComponent::ComponentType, SGNComponent* > NodeComponents;

    ///Usage context affects lighting, navigation, physics, etc
    enum UsageContext {
        NODE_DYNAMIC = 0,
        NODE_STATIC
    };

    SceneGraphNode(SceneGraph* const sg, SceneNode* const node);
    ~SceneGraphNode();

    bool unload();
    /// Update bounding boxes
    void checkBoundingBoxes();
    /// Apply current transform to the node's BB
    void updateBoundingBoxTransform(const mat4<F32>& transform);
    /// Called if the current node is in view and is about to be rendered
    bool onDraw(RenderStage renderStage); 
    /// Called after the current node was rendered
    void postDraw(RenderStage renderStage);
    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);
    /*Node Management*/
    template<class T = SceneNode>
    ///Always use the level of redirection needed to reduce virtual function overhead
    ///Use getNode<SceneNode> if you need material properties for ex. or getNode<SubMesh> for animation transforms
    inline T* getNode() const {assert(_node != nullptr); return dynamic_cast<T*>(_node);}

    SceneGraphNode* addNode(SceneNode* const node,const std::string& name = "");
    void			removeNode(SceneGraphNode* node);
    ///Find a node in the graph based on the SceneGraphNode's name
    ///If sceneNodeName = true, find a node in the graph based on the SceneNode's name
    SceneGraphNode* findNode(const std::string& name, bool sceneNodeName = false);
    ///Find the graph nodes whom's bounding boxes intersects the given ray
    void Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits);

    ///Selection helper functions
           void setSelected(bool state);
    inline bool isSelected()	                const {return _selected;}
    inline bool isSelectable()                  const {return _isSelectable;}
    inline void setSelectable(const bool state)       {_isSelectable = state;}

    const  std::string& getName() const {return _name;}
    /*Node Management*/

    /*Parent <-> Children*/
                SceneGraphNode*  getRoot()        const;
    inline      SceneGraphNode*  getParent()      const {return _parent;}
    inline      NodeChildren&    getChildren()          {return _children;}
                void             setParent(SceneGraphNode* const parent);
    /*Parent <-> Children*/

    /*Bounding Box Management*/
    void setInitialBoundingBox(const BoundingBox& initialBoundingBox);

    inline       BoundingBox&     getBoundingBox()                {return _boundingBox;}
    inline const BoundingBox&     getBoundingBoxConst()    const  {return _boundingBox;}
    inline       BoundingSphere&  getBoundingSphere()             {return _boundingSphere;}
    inline const BoundingSphere&  getBoundingSphereConst() const  {return _boundingSphere; }
    inline const BoundingBox&     getInitialBoundingBox()  const  {return _initialBoundingBox; }

    void getBBoxes(vectorImpl<BoundingBox >& boxes) const;
    /*Bounding Box Management*/

    /*Transform management*/
    Transform* const getTransform();
               void	 setTransform(Transform* const t);

    Transform* const getPrevTransform();
               void  setPrevTransform(Transform* const t);

    inline     void  silentDispose(const bool state)       {_silentDispose = state;}
    inline     void  useDefaultTransform(const bool state) {_noDefaultTransform = !state;}

    /*Transform management*/

    /*Node State*/
    inline void setActive(bool state) {_wasActive = _active; _active = state;}
    inline void restoreActive()       {_active = _wasActive;}
    inline void	scheduleDeletion()    {_shouldDelete = true;}

    inline bool inView()   const {return _inView;}
    inline bool isReady()  const {return _isReady;}
    inline bool isActive() const {return _active;}
    /*Node State*/

    inline void setCastsShadows(const bool state)    {_castsShadows = state;}
    inline void setReceivesShadows(const bool state) {_receiveShadows = state;}
    inline bool getCastsShadows()    const {return _castsShadows;}
    inline bool getReceivesShadows() const {return _receiveShadows;}

    void getShadowCastersAndReceivers(vectorImpl<const SceneGraphNode* >& casters, vectorImpl<const SceneGraphNode* >& receivers, bool visibleOnly = false) const;

    inline U32  getInstanceID() const {return _instanceID;}
    inline U32  getChildQueue() const {return _childQueue;}
    inline void incChildQueue()       {_childQueue++;}
    inline void decChildQueue()       {_childQueue--;}

    inline const UsageContext&      getUsageContext()          const {return _usageContext;}
    inline void  setUsageContext(const UsageContext& newContext)           {_usageContext = newContext;}

    void addBoundingBox(const BoundingBox& bb, const SceneNodeType& type);
    void setBBExclusionMask(U32 bbExclusionMask) {_bbAddExclusionList = bbExclusionMask;}

    inline U64 getElapsedTime() const {return _elapsedTime;}

    inline void setComponent(SGNComponent::ComponentType type, SGNComponent* component) { SAFE_UPDATE(_components[type], component); }

    template<class T>
    inline T* getComponent() { assert(false && "INVALID COMPONENT"); return nullptr; }
    template<>
    inline AnimationComponent* getComponent() { return dynamic_cast<AnimationComponent*>(_components[SGNComponent::SGN_COMP_ANIMATION]); }
    template<>
    inline NavigationComponent* getComponent() { return dynamic_cast<NavigationComponent*>(_components[SGNComponent::SGN_COMP_NAVIGATION]); }
    template<>
    inline PhysicsComponent* getComponent() { return dynamic_cast<PhysicsComponent*>(_components[SGNComponent::SGN_COMP_PHYSICS]); }
    
    inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

protected:
    friend class RenderPassCuller;
    inline void inView(const bool isInView) { _inView = isInView; }

private:
    inline void setName(const std::string& name){_name = name;}
    inline void scheduleDrawReset(RenderStage currentStage) { _drawReset[currentStage] = true; }

private:
    SceneNode* _node;
    NodeChildren _children;
    SceneGraphNode *_parent;
    SceneGraph     *_sceneGraph;
    boost::atomic<bool> _active;
    boost::atomic<bool> _loaded;
    boost::atomic<bool> _inView;
    //Used to skip certain BB's (sky, ligts, etc);
    U32 _bbAddExclusionList;
    bool _selected;
    bool _isSelectable;
    bool _wasActive;
    bool _noDefaultTransform;
    bool _sorted;
    bool _silentDispose;
    bool _castsShadows;
    bool _receiveShadows;
    boost::atomic<bool> _boundingBoxDirty;
    boost::atomic<bool> _isReady; //<is created and has a valid transform
    bool _shouldDelete;
    ///_initialBoundingBox is a copy of the initialy calculate BB for transformation
    ///it should be copied in every computeBoungingBox call;
    BoundingBox _initialBoundingBox, _initialBoundingBoxCache;
    BoundingBox _boundingBox;
    BoundingSphere _boundingSphere; ///<For faster visibility culling

    Transform*	_transform;
    Transform*  _transformPrevious;
    mat4<F32>   _transformGlobalMatrixCache;

    U32 _instanceID;
    U32 _childQueue;
    D32 _updateTimer;
    U64 _elapsedTime;//< the total amount of time that passed since this node was created
    std::string _name;

    UsageContext _usageContext;
    NodeComponents _components;

    Unordered_map<RenderStage, bool> _drawReset;

    StateTracker<bool> _trackedBools;
};

#endif