/*
   Copyright (c) 2017 DIVIDE-Studio
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
class SceneState;
class WorldPacket;
class SceneRenderState;
class NetworkingComponent;
struct RenderStagePass;

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);
FWD_DECLARE_MANAGED_CLASS(Material);
namespace Attorney {
    class SceneNodeSceneGraph;
    class SceneNodeNetworkComponent;
};

enum class SceneNodeType : U32 {
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
      enum class UpdateFlag : U32 {
        BOUNDS_CHANGED = 0,
        COUNT
      };

   protected:
    class SGNParentData {
    public:
        explicit SGNParentData(I64 sgnGUID) : _GUID(sgnGUID)
        {
            _updateFlags.fill(true);
        }

        inline I64 sgnGUID() const { return _GUID; }

        inline bool getFlag(UpdateFlag flag) const { return _updateFlags[to_uint(flag)]; }
        inline void clearFlag(UpdateFlag flag) { _updateFlags[to_uint(flag)] = false; }
        inline void setFlag(UpdateFlag flag) { _updateFlags[to_uint(flag)] = true; }

    private:
        I64 _GUID;
        std::array<bool, to_const_uint(UpdateFlag::COUNT)> _updateFlags;
    };

   public:
    explicit SceneNode(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const SceneNodeType& type);
    explicit SceneNode(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, const SceneNodeType& type);
    virtual ~SceneNode();

    /// Perform any pre-draw operations (this is after sort and transform updates)
    /// If the node isn't ready for rendering and should be skipped this frame,
    /// the return value is false
    virtual bool onRender(const RenderStagePass& renderStagePass);
    virtual bool getDrawState() const { return _renderState.getDrawState(); }
    /// Some SceneNodes may need special case handling. I.E. water shouldn't
    /// render itself in REFLECTION
    virtual bool getDrawState(const RenderStagePass& renderStage);

    virtual void initialiseDrawCommands(SceneGraphNode& sgn,
                                        const RenderStagePass& renderStage,
                                        GenericDrawCommands& drawCommandsInOut);
    virtual void updateDrawCommands(SceneGraphNode& sgn,
                                    const RenderStagePass& renderStagePass,
                                    const SceneRenderState& sceneRenderState,
                                    GenericDrawCommands& drawCommandsInOut);
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

   protected:
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                             SceneState& sceneState);
    /// Called as a second pass after sceneUpdate
    virtual void sgnUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                           SceneState& sceneState);
    // Post insertion calls (Use this to setup child objects during creation)
    virtual void postLoad(SceneGraphNode& sgn);
    virtual void updateBoundsInternal(SceneGraphNode& sgn);

    inline void setFlag(UpdateFlag flag) {
        for (SGNParentData& data : _sgnParents) {
            data.setFlag(flag);
        }
    }

    inline vectorImpl<SceneNode::SGNParentData>::iterator getSGNData(I64 sgnGUID) {
        vectorImpl<SceneNode::SGNParentData>::iterator it;
        it = std::find_if(std::begin(_sgnParents), std::end(_sgnParents),
            [&sgnGUID](SceneNode::SGNParentData sgnIter) {
                return (sgnIter.sgnGUID() == sgnGUID);
            });
        return it;
    }

    inline vectorImpl<SceneNode::SGNParentData>::const_iterator getSGNData(I64 sgnGUID) const {
        return getSGNData(sgnGUID);
    }

    virtual void onCameraUpdate(SceneGraphNode& sgn,
                                const I64 cameraGUID,
                                const vec3<F32>& posOffset,
                                const mat4<F32>& rotationOffset);
    virtual void onCameraChange(SceneGraphNode& sgn,
                                const Camera& cam);

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

    vectorImpl<SGNParentData> _sgnParents;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(SceneNode);

namespace Attorney {
class SceneNodeSceneGraph {
   private:
    static void postLoad(SceneNode& node, SceneGraphNode& sgn) {
        node.postLoad(sgn);
    }
    
    static void sceneUpdate(SceneNode& node, const U64 deltaTime,
                            SceneGraphNode& sgn, SceneState& sceneState) {
        node.sceneUpdate(deltaTime, sgn, sceneState);
    }

    static void sgnUpdate(SceneNode& node, const U64 deltaTime,
                          SceneGraphNode& sgn, SceneState& sceneState) {
        node.sgnUpdate(deltaTime, sgn, sceneState);
    }
    
    static void registerSGNParent(SceneNode& node, I64 sgnGUID) {
        // prevent double add
        vectorImpl<SceneNode::SGNParentData>::const_iterator it;
        it = node.getSGNData(sgnGUID);
        assert(it == std::cend(node._sgnParents));

        node._sgnParents.push_back(SceneNode::SGNParentData(sgnGUID));
    }

    static void unregisterSGNParent(SceneNode& node, I64 sgnGUID) {
        // prevent double remove
        vectorImpl<SceneNode::SGNParentData>::const_iterator it;
        it = node.getSGNData(sgnGUID);
        assert(it != std::cend(node._sgnParents));

        node._sgnParents.erase(it);
    }

    static size_t parentCount(const SceneNode& node) {
        return node._sgnParents.size();
    }


    static void onCameraUpdate(SceneGraphNode& sgn,
                               SceneNode& node,
                               const I64 cameraGUID,
                               const vec3<F32>& posOffset,
                               const mat4<F32>& rotationOffset) {
        node.onCameraUpdate(sgn, cameraGUID, posOffset, rotationOffset);
    }

    static void onCameraChange(SceneGraphNode& sgn,
                               SceneNode& node,
                               const Camera& cam) {
        node.onCameraChange(sgn, cam);
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
