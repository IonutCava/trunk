/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "SceneNodeRenderState.h"
#include "Rendering/Camera/Headers/Frustum.h"
#include "Core/Resources/Headers/Resource.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

class Scene;
class Camera;
class SceneGraph;
class SceneState;
class WorldPacket;
class RenderPackage;
class RenderStagePass;
class SceneRenderState;
class NetworkingComponent;

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);
FWD_DECLARE_MANAGED_CLASS(Material);

namespace Attorney {
    class SceneNodeSceneGraph;
    class SceneNodeNetworkComponent;
};

/// Usage context affects lighting, navigation, physics, etc
enum class NodeUsageContext : U8 {
    NODE_DYNAMIC = 0,
    NODE_STATIC
};

enum class SceneNodeType : U16 {
    TYPE_ROOT = toBit(1),       //< root node
    TYPE_OBJECT3D = toBit(2),   //< 3d objects in the scene
    TYPE_TRANSFORM = toBit(3),  //< dummy node to stack multiple transforms
    TYPE_WATER = toBit(4),      //< water node
    TYPE_LIGHT = toBit(5),      //< a scene light
    TYPE_TRIGGER = toBit(6),    //< a scene trigger (perform action on contact)
    TYPE_PARTICLE_EMITTER = toBit(7),   //< a particle emitter
    TYPE_SKY = toBit(8),                //< sky node
    TYPE_VEGETATION_GRASS = toBit(9),   //< grass node
    TYPE_VEGETATION_TREES = toBit(10),  //< trees node (to do later)
    COUNT                               //< Place types above
                                        
};

class NOINITVTABLE SceneNode : public CachedResource {
    friend class Attorney::SceneNodeSceneGraph;
    friend class Attorney::SceneNodeNetworkComponent;

  public:
    explicit SceneNode(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const SceneNodeType& type);
    explicit SceneNode(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, const SceneNodeType& type);
    virtual ~SceneNode();

    /// Perform any pre-draw operations (this is after sort and transform updates)
    /// If the node isn't ready for rendering and should be skipped this frame,
    /// the return value is false
    virtual bool onRender(SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          const RenderStagePass& renderStagePass);
    virtual bool getDrawState() const { return _renderState.getDrawState(); }
    /// Some SceneNodes may need special case handling. I.E. water shouldn't
    /// render itself in REFLECTION
    virtual bool getDrawState(const RenderStagePass& renderStage);

    virtual void buildDrawCommands(SceneGraphNode& sgn,
                                   const RenderStagePass& renderStage,
                                   RenderPackage& pkgInOut);
    /*//Rendering/Processing*/

    virtual bool unload();
    virtual void setMaterialTpl(const Material_ptr& material);
    const Material_ptr& getMaterialTpl();

    inline void setType(const SceneNodeType& type) { _type = type; }
    inline const SceneNodeType& getType() const { return _type; }

    inline SceneNodeRenderState& renderState() { return _renderState; }

    inline void incLODcount() { _LODcount++; }
    inline void decLODcount() { _LODcount--; }
    inline U8   getLODcount() const { return _LODcount; }

    ResourceCache& parentResourceCache() { return _parentCache; }
    const ResourceCache& parentResourceCache() const { return _parentCache; }


    inline const BoundingBox& getBoundsInternal() const { return _boundingBox; }
   protected:
    friend class BoundsSystem;
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn,
                             SceneState& sceneState);

    // Post insertion calls (Use this to setup child objects during creation)
    virtual void postLoad(SceneGraphNode& sgn);
    virtual void updateBoundsInternal();

    virtual void onCameraUpdate(SceneGraphNode& sgn,
                                const U64 cameraNameHash,
                                const vec3<F32>& posOffset,
                                const mat4<F32>& rotationOffset);

    virtual void setBoundsChanged();

   protected:
     virtual void onNetworkSend(SceneGraphNode& sgn, WorldPacket& dataOut) const;
     virtual void onNetworkReceive(SceneGraphNode& sgn, WorldPacket& dataIn) const;

   protected:
    ResourceCache& _parentCache;
    /// The various states needed for rendering
    SceneNodeRenderState _renderState;
    /// Maximum available LOD levels
    U8 _LODcount;
    /// The initial bounding box as it was at object's creation (i.e. no transforms applied)
    BoundingBox _boundingBox;

   private:
    SceneNodeType _type;
    Material_ptr _materialTemplate;

    vector<SceneGraphNode*> _sgnParents;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(SceneNode);

namespace Attorney {
class SceneNodeSceneGraph {
   private:
    static void postLoad(SceneNode& node, SceneGraphNode& sgn) {
        node.postLoad(sgn);
    }
    
    static void sceneUpdate(SceneNode& node, const U64 deltaTimeUS,
                            SceneGraphNode& sgn, SceneState& sceneState) {
        node.sceneUpdate(deltaTimeUS, sgn, sceneState);
    }

    static void registerSGNParent(SceneNode& node, SceneGraphNode* sgn);

    static void unregisterSGNParent(SceneNode& node, SceneGraphNode* sgn);

    static size_t parentCount(const SceneNode& node) {
        return node._sgnParents.size();
    }

    static void onCameraUpdate(SceneGraphNode& sgn,
                               SceneNode& node,
                               const U64 cameraNameHash,
                               const vec3<F32>& posOffset,
                               const mat4<F32>& rotationOffset) {
        node.onCameraUpdate(sgn, cameraNameHash, posOffset, rotationOffset);
    }

    friend class Divide::SceneGraph;
    friend class Divide::SceneGraphNode;
};

class SceneNodeNetworkComponent {
  private:
    static void onNetworkSend(SceneGraphNode& sgn, SceneNode& node, WorldPacket& dataOut) {
        node.onNetworkSend(sgn, dataOut);
    }

    static void onNetworkReceive(SceneGraphNode& sgn, SceneNode& node, WorldPacket& dataIn) {
        node.onNetworkReceive(sgn, dataIn);
    }

    friend class Divide::NetworkingComponent;
};

};  // namespace Attorney
};  // namespace Divide
#endif
