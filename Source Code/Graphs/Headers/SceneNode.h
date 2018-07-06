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

#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "SceneNodeRenderState.h"
#include "Core/Resources/Headers/Resource.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"
#include "Hardware/Video/Headers/RenderAPIWrapper.h"

namespace Divide {
class Scene;
class Material;

enum SceneNodeType {
    TYPE_ROOT             = toBit(1), //< root node
    TYPE_OBJECT3D         = toBit(2), //< 3d objects in the scene
    TYPE_TRANSFORM        = toBit(3), //< dummy node to stack multiple transforms
    TYPE_WATER            = toBit(4), //< water node
    TYPE_LIGHT            = toBit(5), //< a scene light
    TYPE_TRIGGER          = toBit(6), //< a scene trigger (perform action on contact)
    TYPE_PARTICLE_EMITTER = toBit(7), //< a particle emitter
    TYPE_SKY              = toBit(8), //< sky node
    TYPE_VEGETATION_GRASS = toBit(9), //< grass node
    TYPE_VEGETATION_TREES = toBit(10), //< trees node (to do later)
    ///Place types above
    TYPE_PLACEHOLDER      = toBit(11)
};

class SceneState;
class SceneRenderState;
class ShaderProgram;
enum  RenderStage;

class SceneNode : public Resource {
    friend class SceneGraphNode;
    friend class RenderQueue;

public:
    SceneNode(const SceneNodeType& type);
    SceneNode(const stringImpl& name, const SceneNodeType& type);
    virtual ~SceneNode();

    /// Perform any pre-draw operations (this is after sort and transform updates)
    /// If the node isn't ready for rendering and should be skipped this frame, the return value is false
    virtual bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage) = 0;
    virtual	bool getDrawState() const { return _renderState.getDrawState(); }
    /// Some SceneNodes may need special case handling. I.E. water shouldn't render itself in REFLECTION_STAGE
    virtual	bool getDrawState(const RenderStage& currentStage);
    virtual void getDrawCommands(SceneGraphNode* const sgn, const RenderStage& currentRenderStage, SceneRenderState& sceneRenderState, vectorImpl<GenericDrawCommand>& drawCommandsOut) = 0;
    /*//Rendering/Processing*/

    virtual	bool	  unload();
    virtual bool	  isInView( const SceneRenderState& sceneRenderState, SceneGraphNode* const sgn, const bool distanceCheck = true );
    virtual	void	  setMaterialTpl(Material* const m);
    Material*   const getMaterialTpl();

    /// Every SceneNode computes a bounding box in it's own way.
    virtual	bool computeBoundingBox(SceneGraphNode* const sgn);
    virtual void postDrawBoundingBox(SceneGraphNode* const sgn) const;

    inline       void           setType(const SceneNodeType& type)        { _type = type; }
    inline const SceneNodeType& getType()					        const { return _type; }

    inline SceneNodeRenderState& renderState() { return _renderState; }

    inline void incLODcount() { _LODcount++; }
    inline void decLODcount() { _LODcount--; }
    inline U8   getLODcount()   const { return _LODcount; }

protected:
    friend class RenderBin;
    friend class RenderPass;
    friend class SceneGraph;
    friend class RenderingComponent;
    /// Perform any post-draw operations (this is after releasing object and shadow states)
    virtual void postDraw(SceneGraphNode* const sgn, const RenderStage& currentStage) {/*Nothing yet*/ }
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    /*Rendering/Processing*/
    virtual void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage) = 0; //Sounds are played, geometry is displayed etc.

    virtual void onCameraChange(SceneGraphNode* const sgn) {}

    // Post insertion calls (Use this to setup child objects during creation)
    virtual void postLoad( SceneGraphNode* const sgn ) { _hasSGNParent = ( sgn != nullptr ); };

    inline bool hasSGNParent() const { return _hasSGNParent;  }

protected:
    ///The various states needed for rendering
    SceneNodeRenderState  _renderState;
    ///Maximum available LOD levels
    U8                    _LODcount; 

private:
    SceneNodeType _type;
    bool          _hasSGNParent;
    Material*	  _materialTemplate;
};

}; //namespace Divide
#endif