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
#include "ECS/Components/Headers/EditorComponent.h"

namespace Divide {

class Scene;
class Camera;
class Player;
class SceneGraph;
class SceneState;
class WorldPacket;
class RenderPackage;
class SceneRenderState;
class BoundsSystem;
class BoundsComponent;
class NetworkingComponent;

class Light;
class SpotLightComponent;
class PointLightComponent;
class DirectionalLightComponent;

struct RenderStagePass;

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);
FWD_DECLARE_MANAGED_CLASS(Material);

namespace Attorney {
    class SceneNodePlayer;
    class SceneNodeSceneGraph;
    class SceneNodeLightComponent;
    class SceneNodeBoundsComponent;
    class SceneNodeNetworkComponent;
};

/// Usage context affects lighting, navigation, physics, etc
enum class NodeUsageContext : U8 {
    NODE_DYNAMIC = 0,
    NODE_STATIC
};

enum class SceneNodeType : U16 {
    TYPE_ROOT = 0,
    TYPE_EMPTY,              //< empty node
    TYPE_OBJECT3D,           //< 3d objects in the scene
    TYPE_TRANSFORM,          //< dummy node to stack multiple transforms
    TYPE_WATER,              //< water node
    TYPE_TRIGGER,            //< a scene trigger (perform action on contact)
    TYPE_PARTICLE_EMITTER,   //< a particle emitter
    TYPE_SKY,                //< sky node
    TYPE_INFINITEPLANE,      //< the infinite plane that sits beneath everything in the world
    TYPE_VEGETATION,         //< grass node
    COUNT
};

class SceneNode : public CachedResource {
    friend class Attorney::SceneNodePlayer;
    friend class Attorney::SceneNodeSceneGraph;
    friend class Attorney::SceneNodeLightComponent;
    friend class Attorney::SceneNodeBoundsComponent;
    friend class Attorney::SceneNodeNetworkComponent;

  public:
    explicit SceneNode(ResourceCache& parentCache, size_t descriptorHash, const Str128& name, const SceneNodeType& type = SceneNodeType::TYPE_EMPTY);
    explicit SceneNode(ResourceCache& parentCache, size_t descriptorHash, const Str128& name, const Str128& resourceName, const stringImpl& resourceLocation, const SceneNodeType& type);
    virtual ~SceneNode();

    /// Perform any pre-draw operations PRE-command build
    /// If the node isn't ready for rendering and should be skipped this frame, the return value is false
    virtual bool preRender(SceneGraphNode& sgn,
                            const Camera& camera,
                            RenderStagePass renderStagePass,
                            bool refreshData,
                            bool& rebuildCommandsOut);

    /// Perform any pre-draw operations POST-command build
    /// If the node isn't ready for rendering and should be skipped this frame, the return value is false
    virtual bool onRender(SceneGraphNode& sgn,
                          const Camera& camera,
                          RenderStagePass renderStagePass,
                          bool refreshData);

    virtual void onRefreshNodeData(SceneGraphNode& sgn,
                                   RenderStagePass renderStagePass,
                                   const Camera& camera,
                                   bool quick,
                                   GFX::CommandBuffer& bufferInOut);

    virtual void buildDrawCommands(SceneGraphNode& sgn,
                                   RenderStagePass renderStage,
                                   RenderPackage& pkgInOut);
    /*//Rendering/Processing*/

    bool unload() noexcept override;
    virtual void setMaterialTpl(const Material_ptr& material);
    const Material_ptr& getMaterialTpl() const;

    inline void type(const SceneNodeType& type) { _type = type; }
    inline const SceneNodeType& type() const { return _type; }

    inline SceneNodeRenderState& renderState() { return _renderState; }

    virtual const char* getTypeName() const;

    inline ResourceCache& parentResourceCache() { return _parentCache; }
    inline const ResourceCache& parentResourceCache() const { return _parentCache; }

    inline const BoundingBox& getBounds() const { return _boundingBox; }

    virtual void saveCache(ByteBuffer& outputBuffer) const;
    virtual void loadCache(ByteBuffer& inputBuffer);

    virtual void saveToXML(boost::property_tree::ptree& pt) const;
    virtual void loadFromXML(const boost::property_tree::ptree& pt);

   protected:
    virtual void frameStarted(SceneGraphNode& sgn);
    virtual void frameEnded(SceneGraphNode& sgn);
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState);

    // Post insertion calls (Use this to setup child objects during creation)
    virtual void postLoad(SceneGraphNode& sgn);

    virtual void setBounds(const BoundingBox& aabb);

    EditorComponent& getEditorComponent() { return _editorComponent; }
    const EditorComponent& getEditorComponent() const { return _editorComponent; }

    virtual size_t maxReferenceCount() const { return 1; }

    virtual const char* getResourceTypeName() const override { return "SceneNode"; }

   protected:
     virtual void editorFieldChanged(const char* field);
     virtual void onNetworkSend(SceneGraphNode& sgn, WorldPacket& dataOut) const;
     virtual void onNetworkReceive(SceneGraphNode& sgn, WorldPacket& dataIn) const;

   protected:
    EditorComponent _editorComponent;

    ResourceCache& _parentCache;
    /// The various states needed for rendering
    SceneNodeRenderState _renderState;

    /// The initial bounding box as it was at object's creation (i.e. no transforms applied)
    BoundingBox _boundingBox;
    bool _boundsChanged = false;

   private:
    SceneNodeType _type;
    Material_ptr _materialTemplate;

    std::atomic_size_t _sgnParentCount = 0;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(SceneNode);

namespace Attorney {
class SceneNodeSceneGraph {
   private:
    static void postLoad(SceneNode& node, SceneGraphNode& sgn) {
        node.postLoad(sgn);
    }
    static void frameStarted(SceneNode& node, SceneGraphNode& sgn) {
        node.frameStarted(sgn);
    }

    static void frameEnded(SceneNode& node, SceneGraphNode& sgn) {
        node.frameEnded(sgn);
    }

    static void sceneUpdate(SceneNode& node, const U64 deltaTimeUS,
                            SceneGraphNode& sgn, SceneState& sceneState) {
        OPTICK_EVENT();
        node.sceneUpdate(deltaTimeUS, sgn, sceneState);
    }

    static void registerSGNParent(SceneNode& node, SceneGraphNode* sgn) {
        ACKNOWLEDGE_UNUSED(sgn);

        node._sgnParentCount.fetch_add(1);
    }

    static void unregisterSGNParent(SceneNode& node, SceneGraphNode* sgn) {
        ACKNOWLEDGE_UNUSED(sgn);

        node._sgnParentCount.fetch_sub(1);
    }

    static size_t parentCount(const SceneNode& node) {
        return node._sgnParentCount.load();
    }

    static size_t maxReferenceCount(const SceneNode& node) {
        return node.maxReferenceCount();
    }

    static EditorComponent& getEditorComponent(SceneNode& node) {
        return node._editorComponent;
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

class SceneNodeBoundsComponent {
  private:
    static void setBounds(SceneNode& node, const BoundingBox& aabb) {
        node.setBounds(aabb);
    }

    static bool boundsChanged(const SceneNode& node) {
        return node._boundsChanged;
    }

    static void setBoundsChanged(SceneNode& node) {
        node._boundsChanged = true;
    }

    static void clearBoundsChanged(SceneNode& node) {
        node._boundsChanged = false;
    }

    friend class Divide::BoundsComponent;
    friend class Divide::BoundsSystem;
};

class SceneNodeLightComponent {
private:
    static void setBounds(SceneNode& node, const BoundingBox& aabb) {
        node.setBounds(aabb);
    }

    friend class Divide::Light;
    friend class Divide::SpotLightComponent;
    friend class Divide::PointLightComponent;
    friend class Divide::DirectionalLightComponent;
};

class SceneNodePlayer {
private:
    static void setBounds(SceneNode& node, const BoundingBox& aabb) {
        node.setBounds(aabb);
    }

    friend class Divide::Player;
};

};  // namespace Attorney
};  // namespace Divide
#endif
