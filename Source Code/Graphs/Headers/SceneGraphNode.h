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
#include "Utility/Headers/StateTracker.h"
#include "Graphs/Components/Headers/SGNComponent.h"
#include "Graphs/Components/Headers/PhysicsComponent.h"
#include "Graphs/Components/Headers/AnimationComponent.h"
#include "Graphs/Components/Headers/NavigationComponent.h"

namespace Divide {

class Transform;
class SceneGraph;
class SceneState;
struct TransformValues;
// This is the scene root node. All scene node's are added to it as child nodes
class SceneRoot : public SceneNode {
public:
    SceneRoot() : SceneNode("root",TYPE_ROOT)
    {
        _renderState.useDefaultMaterial(false);
        setState(RES_LOADED);
    }

    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage) { return true; }
    bool unload()                                      {return true;}
    bool load(const stringImpl& name)                 {return true;}
    bool computeBoundingBox(SceneGraphNode* const sgn);

protected:
    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage) {}
    void postLoad(SceneGraphNode* const sgn)  { SceneNode::postLoad(sgn); }
};

// Add as many SceneTransform nodes are needed as parent nodes for any scenenode to create complex transforms in the scene
class SceneTransform : public SceneNode {
public:
    SceneTransform() : SceneNode(TYPE_TRANSFORM)
    {
        _renderState.useDefaultMaterial(false);
        setState(RES_LOADED);
    }
    
    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage) {}
    bool onDraw(const RenderStage& currentStage) {return true;}

    void postLoad(SceneGraphNode* const sgn)           {return;}
    bool unload()                                      {return true;}
    bool load(const stringImpl& name)                 {return true;}
    bool computeBoundingBox(SceneGraphNode* const sgn) {return true;}
};

class IMPrimitive;
class SceneGraphNode : public GUIDWrapper, private NonCopyable {
public:
    typedef hashMapImpl<stringImpl, SceneGraphNode*> NodeChildren;

     ///Usage context affects lighting, navigation, physics, etc
    enum UsageContext {
        NODE_DYNAMIC = 0,
        NODE_STATIC
    };

    SceneGraphNode(SceneGraph* const sg, SceneNode* const node);
    ~SceneGraphNode();

    bool unload();
    /// Draw the current scene graph node
    void render(const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage);
    /// Update bounding boxes
    void checkBoundingBoxes();
    /// Apply current transform to the node's BB. Return true if the bounding extents changed
    bool updateBoundingBoxTransform(const mat4<F32>& transform);
    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneState& sceneState);
    /// Called when the camera updates the view matrix and/or the projection matrix
    void onCameraChange();
    /*Node Management*/
    ///Always use the level of redirection needed to reduce virtual function overhead
    ///Use getNode<SceneNode> if you need material properties for ex. or getNode<SubMesh> for animation transforms
    template<typename T = SceneNode>
    inline T* getNode() const {assert(_node != nullptr); return dynamic_cast<T*>(_node);}

    SceneGraphNode* addNode(SceneNode* const node,const stringImpl& name = "");
    void			removeNode(SceneGraphNode* node);
    ///Find a node in the graph based on the SceneGraphNode's name
    ///If sceneNodeName = true, find a node in the graph based on the SceneNode's name
    SceneGraphNode* findNode(const stringImpl& name, bool sceneNodeName = false);
    ///Find the graph nodes whom's bounding boxes intersects the given ray
    void Intersect(const Ray& ray, F32 start, F32 end, vectorImpl<SceneGraphNode* >& selectionHits);

    ///Selection helper functions
           void setSelected(const bool state);
    inline bool isSelected()	                const {return _selected;}
    inline bool isSelectable()                  const {return _isSelectable;}
    inline void setSelectable(const bool state)       {_isSelectable = state;}

    const  stringImpl& getName() const {return _name;}
    /*Node Management*/

    /*Parent <-> Children*/
                SceneGraphNode*  getRoot()        const;
    inline      SceneGraphNode*  getParent()      const {return _parent;}
    inline      NodeChildren&    getChildren()          {return _children;}
                void             setParent(SceneGraphNode* const parent);

    /// Alias for changing parent
    inline void attachToNode(SceneGraphNode* const target) {
        setParent(target);
    }

    inline void attachToRoot() {
        setParent(getRoot());
    }
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

    inline     void  silentDispose(const bool state)       {_silentDispose = state;}
    inline     void  useDefaultTransform(const bool state) {
        getComponent<PhysicsComponent>()->useDefaultTransform(!state);
    }

    const mat4<F32>& getWorldMatrix(D32 interpolationFactor = 1.0);
    /*Transform management*/

    /*Node State*/
    inline void setActive(const bool state) {_wasActive = _active; _active = state;}
    inline void restoreActive()       {_active = _wasActive;}
    inline void	scheduleDeletion()    {_shouldDelete = true;}

    inline bool inView()   const {return _inView;}
    inline bool isActive() const {return _active;}

    void renderWireframe(const bool state);
    inline bool renderWireframe() const { return _renderWireframe; }
    void renderBoundingBox(const bool state);
    inline bool renderBoundingBox() const { return _renderBoundingBox; }

    void renderSkeleton(const bool state);
    inline bool renderSkeleton() const { return _renderSkeleton; }
    /*Node State*/

    void castsShadows(const bool state);
    void receivesShadows(const bool state);

    bool castsShadows()    const;
    bool receivesShadows() const;

    void getShadowCastersAndReceivers(vectorImpl<const SceneGraphNode* >& casters, vectorImpl<const SceneGraphNode* >& receivers, bool visibleOnly = false) const;

    inline U32  getInstanceID() const {return _instanceID;}
    inline U32  getChildQueue() const {return _childQueue;}
    inline void incChildQueue()       {_childQueue++;}
    inline void decChildQueue()       {_childQueue--;}

    inline const UsageContext& usageContext()                 const {return _usageContext;}
    inline void  usageContext(const UsageContext& newContext)       {_usageContext = newContext;}

    void addBoundingBox(const BoundingBox& bb, const SceneNodeType& type);
    void setBBExclusionMask(U32 bbExclusionMask) {_bbAddExclusionList = bbExclusionMask;}

    inline U64 getElapsedTime() const {return _elapsedTime;}

    inline void setComponent(SGNComponent::ComponentType type, SGNComponent* component) { SAFE_UPDATE(_components[type], component); }

    template<typename T>
    inline T* getComponent() const { assert(false && "INVALID COMPONENT"); return nullptr; }
    template<>
    inline AnimationComponent*  getComponent() const { return static_cast<AnimationComponent*>(_components[SGNComponent::SGN_COMP_ANIMATION]); }
    template<>
    inline NavigationComponent* getComponent() const { return static_cast<NavigationComponent*>(_components[SGNComponent::SGN_COMP_NAVIGATION]); }
    template<>
    inline PhysicsComponent*    getComponent() const { return static_cast<PhysicsComponent*>(_components[SGNComponent::SGN_COMP_PHYSICS]); }
    
    inline StateTracker<bool>& getTrackedBools() { return _trackedBools; }

    inline const mat4<F32>& getMaterialColorMatrix()    const { return _materialColorMatrix; }
    inline const mat4<F32>& getMaterialPropertyMatrix() const { return _materialPropertyMatrix; }

	inline void registerdeletionCallback(const DELEGATE_CBK<>& cbk) {
        _deletionCallbacks.push_back(cbk);
    }
#ifdef _DEBUG
    void drawDebugAxis();
#endif

    inline bool compare(const SceneGraphNode* const other) const {
        return *this == *other;
    }

    bool operator==(const SceneGraphNode &other) const { 
        return this->getGUID() == other.getGUID(); 
    }

protected:
    friend class RenderPassCuller;
    inline void inView(const bool isInView) { _inView = isInView; isInViewCallback(); }

private:
    inline void setName(const stringImpl& name){_name = name;}
    inline void scheduleDrawReset(RenderStage currentStage) { _drawReset[currentStage] = true; }
           void isInViewCallback();

    /// Called if the current node is in view and is about to be rendered
    bool onDraw(const SceneRenderState& sceneRenderState, RenderStage renderStage); 
    /// Called after the current node was rendered
    void postDraw(const SceneRenderState& sceneRenderState, RenderStage renderStage);

private:
    SceneNode* _node;
    NodeChildren _children;
    SceneGraphNode *_parent;
    SceneGraph     *_sceneGraph;
    std::atomic<bool> _active;
    std::atomic<bool> _loaded;
    std::atomic<bool> _inView;
	std::atomic<bool> _boundingBoxDirty;
    //Used to skip certain BB's (sky, lights, etc);
    U32 _bbAddExclusionList;
    bool _selected;
    bool _isSelectable;
    bool _wasActive;
    bool _sorted;
    bool _silentDispose;
    bool _castsShadows;
    bool _receiveShadows;
    bool _renderWireframe;
    bool _renderBoundingBox;
    bool _renderSkeleton;
    bool _shouldDelete;
    ///_initialBoundingBox is a copy of the initialy calculate BB for transformation
    ///it should be copied in every computeBoungingBox call;
    BoundingBox _initialBoundingBox, _initialBoundingBoxCache;
    BoundingBox _boundingBox;
    BoundingSphere _boundingSphere; ///<For faster visibility culling

    TransformValues* _prevTransformValues;

    ///The interpolated matrix cache. Represents an intermediate matrix between the current matrix and another, external matrix, interpolated by the given factor
    mat4<F32> _worldMatrixInterp;

    U32 _instanceID;
    U32 _childQueue;
    D32 _updateTimer;
    U64 _elapsedTime;//< the total amount of time that passed since this node was created
    stringImpl _name;

    UsageContext _usageContext;
    SGNComponent* _components[SGNComponent::ComponentType_PLACEHOLDER];
	vectorImpl<DELEGATE_CBK<> > _deletionCallbacks;
    hashMapImpl<RenderStage, bool, hashAlg::hash<I32>> _drawReset;

    StateTracker<bool> _trackedBools;

    mat4<F32> _materialColorMatrix;
    mat4<F32> _materialPropertyMatrix;

#ifdef _DEBUG
    vectorImpl<Line > _axisLines;
    IMPrimitive*      _axisGizmo;
#endif
};

}; //namespace Divide
#endif