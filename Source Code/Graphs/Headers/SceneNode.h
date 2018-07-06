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
#include "Dynamics/Physics/Headers/PhysicsAsset.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"

enum SceneNodeType {
    TYPE_ROOT             = toBit(1), //< root node
    TYPE_OBJECT3D         = toBit(2), //< 3d objects in the scene
    TYPE_TRANSFORM        = toBit(3), //< dummy node to stack multiple transforms
    TYPE_TERRAIN          = toBit(4), //< terrain node
    TYPE_WATER            = toBit(5), //< water node
    TYPE_LIGHT            = toBit(6), //< a scene light
    TYPE_TRIGGER          = toBit(7), //< a scene trigger (perform action on contact)
    TYPE_PARTICLE_EMITTER = toBit(8), //< a particle emitter
    TYPE_SKY              = toBit(9), //< sky node
    TYPE_VEGETATION_GRASS = toBit(10), //< grass node
    TYPE_VEGETATION_TREES = toBit(11), //< trees node (to do later)
    ///Place types above
    TYPE_PLACEHOLDER      = toBit(12)
};

class Scene;
class Material;
class SceneState;
class ShaderProgram;
enum  RenderStage;

class SceneNode : public Resource {
    friend class SceneGraphNode;
    friend class RenderQueue;

public:
    SceneNode(const SceneNodeType& type);
    SceneNode(const std::string& name, const SceneNodeType& type);
    virtual ~SceneNode();

    /// Perform any pre-draw operations (this is after sort and transform updates)
    /// If the node isn't ready for rendering and should be skipped this frame, the return value is false
    virtual bool onDraw(const RenderStage& currentStage) = 0;
    virtual	bool getDrawState() const { return _renderState.getDrawState(); }
    /// Some scenenodes may need special case handling. I.E. water shouldn't render itself in REFLECTION_STAGE
    virtual	bool getDrawState(const RenderStage& currentStage)  const { return _renderState.getDrawState(currentStage); }
    /*//Rendering/Processing*/

    virtual	bool	  unload();
    virtual bool      isReadyForDraw(const RenderStage& currentStage);
    virtual bool	  isInView(const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck = true);
    virtual	void	  setMaterial(Material* const m);
    Material* getMaterial();
    /// A custom shader is used if we either don't have a material or we need to render in a special way.
    /// Either way, we are not using the fixed pipeline so a shader is always needed for rendering
    inline void  setCustomShader(ShaderProgram* const shader) { _customShader = shader; }

    /// Every SceneNode computes a bounding box in it's own way.
    virtual	bool computeBoundingBox(SceneGraphNode* const sgn);
    virtual void drawBoundingBox(SceneGraphNode* const sgn) const;

    inline       void           setType(const SceneNodeType& type)        { _type = type; }
    inline const SceneNodeType& getType()					        const { return _type; }

    inline SceneNodeRenderState& getSceneNodeRenderState() { return _renderState; }

    inline void incLODcount() { _LODcount++; }
    inline void decLODcount() { _LODcount--; }
    inline U8   getLODcount()   const { return _LODcount; }
    inline U8   getCurrentLOD() const { return (_lodLevel < (_LODcount - 1) ? _lodLevel : (_LODcount - 1)); }

protected:
    friend class RenderBin;
    friend class RenderPass;
    friend class SceneGraph;
    friend class SceneGraphNode;
    inline I8   getReferenceCount() const { return _sgnReferenceCount; }
    inline void incReferenceCount()       { _sgnReferenceCount++; }
    inline void decReferenceCount()       { _sgnReferenceCount--; }

    /// Perform any post-draw operations (this is after releasing object and shadow states)
    virtual void postDraw(const RenderStage& currentStage) {/*Nothing yet*/ }
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    /// Called if something changed for the SGN that affects rendering
    virtual void drawReset(SceneGraphNode* const sgn) {}
    /*Rendering/Processing*/
    virtual void render(SceneGraphNode* const sgn) = 0; //Sounds are played, geometry is displayed etc.

    /* Normal material */
    virtual	bool prepareMaterial(SceneGraphNode* const sgn);
    virtual	bool releaseMaterial();

    /* Depth map material */
    virtual	bool prepareDepthMaterial(SceneGraphNode* const sgn);
    virtual	bool releaseDepthMaterial();

    /// Perform any last minute operations before the frame drawing ends (this is after shader and shodawmap unbindng)
    virtual void preFrameDrawEnd(SceneGraphNode* const sgn);
    // Post insertion calls (Use this to setup child objects during creation)
    virtual void postLoad(SceneGraphNode* const sgn) { _nodeReady = (sgn != nullptr); }; 

protected:
    ShaderProgram*        _customShader;
    ///The various states needed for rendering
    SceneNodeRenderState  _renderState;
    ///Attach a physics asset to the node to make it physics enabled
    PhysicsAsset*         _physicsAsset;
    ///LOD level is updated at every visibility check (SceneNode::isInView(...));
    U8                    _lodLevel; ///<Relative to camera distance
    U8                    _LODcount; ///<Maximum available LOD levels
    I8                    _sgnReferenceCount;

private:
    //mutable SharedLock _materialLock;
    Material*	  _material;
    bool          _refreshMaterialData;
    bool          _nodeReady; //< make sure postload was called with a valid SGN
    SceneNodeType _type;
};

#endif