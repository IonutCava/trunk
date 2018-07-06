/*
   Copyright (c) 2016 DIVIDE-Studio
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
class Material;

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
    /// Place types above
    COUNT
};

class Camera;
class SceneState;
class SceneRenderState;
class ShaderProgram;
class SceneGraphNode;
enum class RenderStage : U32;

typedef std::weak_ptr<SceneGraphNode> SceneGraphNode_wptr;

namespace Attorney {
    class SceneNodeSceneGraph;
};

class NOINITVTABLE SceneNode : public Resource {
    friend class Attorney::SceneNodeSceneGraph;
   public:
    SceneNode(const SceneNodeType& type);
    SceneNode(const stringImpl& name, const SceneNodeType& type);
    virtual ~SceneNode();

    /// Perform any pre-draw operations (this is after sort and transform
    /// updates)
    /// If the node isn't ready for rendering and should be skipped this frame,
    /// the return value is false
    virtual bool onRender(SceneGraphNode& sgn,
                        RenderStage currentStage) = 0;
    virtual bool getDrawState() const { return _renderState.getDrawState(); }
    /// Some SceneNodes may need special case handling. I.E. water shouldn't
    /// render itself in REFLECTION
    virtual bool getDrawState(RenderStage currentStage);

    virtual bool getDrawCommands(SceneGraphNode& sgn,
                                 RenderStage renderStage,
                                 const SceneRenderState& sceneRenderState,
                                 vectorImpl<GenericDrawCommand>& drawCommandsOut);
    /*//Rendering/Processing*/

    virtual bool unload();
    virtual void setMaterialTpl(Material* const m);
    Material* const getMaterialTpl();

    virtual void postRender(SceneGraphNode& sgn) const;

    inline void setType(const SceneNodeType& type) { _type = type; }
    inline const SceneNodeType& getType() const { return _type; }

    inline SceneNodeRenderState& renderState() { return _renderState; }

    inline void incLODcount() { _LODcount++; }
    inline void decLODcount() { _LODcount--; }
    inline U8   getLODcount() const { return _LODcount; }

   protected:
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                             SceneState& sceneState);

    // Post insertion calls (Use this to setup child objects during creation)
    virtual void postLoad(SceneGraphNode& sgn);
    virtual bool checkBoundingBox(const SceneGraphNode& sgn);

   protected:
    /// The various states needed for rendering
    SceneNodeRenderState _renderState;
    /// Maximum available LOD levels
    U8 _LODcount;
    /// The initial bounding box as it was at object's creation (i.e. no transforms applied)
    BoundingBox _boundingBox;

   private:
    vectorImpl<SceneGraphNode_wptr> _sgnParents;
    SceneNodeType _type;
    Material* _materialTemplate;
};

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

    friend class Divide::SceneGraphNode;
};
};  // namespace Attorney
};  // namespace Divide
#endif
