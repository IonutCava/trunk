/*
   Copyright (c) 2013 DIVIDE-Studio
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

#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "Graphs/Headers/SceneNode.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Vegetation/Headers/Vegetation.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

#define TERRAIN_STRIP_RESTART_INDEX std::numeric_limits<U32>::max()

class Quad3D;
class Quadtree;
class Transform;
class ShaderProgram;
class TerrainDescriptor;
class VertexBufferObject;

class Terrain : public SceneNode {
   enum TerrainTextureUsage{
      TERRAIN_TEXTURE_DIFFUSE = 0,
      TERRAIN_TEXTURE_NORMALMAP = 1,
      TERRAIN_TEXTURE_CAUSTICS = 2,
      TERRAIN_TEXTURE_RED = 3,
      TERRAIN_TEXTURE_GREEN = 4,
      TERRAIN_TEXTURE_BLUE = 5,
      TERRAIN_TEXTURE_ALPHA = 6
  };

  typedef Unordered_map<TerrainTextureUsage, Texture2D*> TerrainTextureMap;

public:

    Terrain();
    ~Terrain();

    bool unload();

    void drawGround() const;
    void drawInfinitePlain();
    void render(SceneGraphNode* const sgn);
    void onDraw(const RenderStage& currentStage);
    void postDraw(const RenderStage& currentStage);
    void prepareMaterial(SceneGraphNode* const sgn);
    void releaseMaterial();
    void prepareDepthMaterial(SceneGraphNode* const sgn);
    void releaseDepthMaterial();
    void sceneUpdate(const D32 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    void drawBoundingBox(SceneGraphNode* const sgn);

    inline void toggleBoundingBoxes(){ _drawBBoxes = !_drawBBoxes; }
    vec3<F32>  getPositionFromGlobal(F32 x, F32 z) const;
    vec3<F32>  getPosition(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getNormal(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getTangent(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getBiTangent(F32 x_clampf, F32 z_clampf) const;
    vec2<F32>  getDimensions(){return vec2<F32>((F32)_terrainWidth, (F32)_terrainHeight);}

           void  terrainSmooth(F32 k);
           void  postLoad(SceneGraphNode* const sgn);
           void  initializeVegetation(TerrainDescriptor* const terrain,SceneGraphNode* const terrainSGN);

    inline VertexBufferObject* const getGeometryVBO() {return _groundVBO;}
    inline Quadtree&         getQuadtree()   const {return *_terrainQuadtree;}
    inline Vegetation* const getVegetation() const {return _veg;}
    inline void addVegetation(Vegetation* veg, std::string grassShader){_veg = veg; _grassShader = grassShader;}
    inline void toggleVegetation(bool state){ _veg->toggleRendering(state); }
    inline void toggleReflection(bool state){ _drawReflected = state;}
    bool computeBoundingBox(SceneGraphNode* const sgn);
    inline bool isInView(const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck = true) {return true;}

    void addTexture(TerrainTextureUsage channel, Texture2D* const texture);
    inline Texture2D* getTexture(TerrainTextureUsage channel) {return _terrainTextures[channel];}

protected:
    template<typename T>
    friend class ImplResourceLoader;
    void loadVisualResources();
    bool loadThreadedResources(TerrainDescriptor* const terrain);

private:

    U8                      _lightCount;
    U16						_terrainWidth, _terrainHeight;
    Quadtree*				_terrainQuadtree;
    VertexBufferObject*		_groundVBO;

    F32  _diffuseUVScale;
    F32  _normalMapUVScale;
    F32  _terrainScaleFactor;
    F32  _terrainHeightScaleFactor;
    F32	 _farPlane;
    F32  _stateRefreshInterval;
    F32  _stateRefreshIntervalBuffer;
    bool _alphaTexturePresent;
    bool _drawBBoxes;
    bool _shadowMapped;
    bool _drawReflected;
    TerrainTextureMap       _terrainTextures;
    Vegetation*             _veg;
    std::string             _grassShader;
    BoundingBox             _boundingBox;
    vec3<F32>               _eyePos;
    Quad3D*					_plane;
    Transform*				_planeTransform;
    Transform*              _terrainTransform;
    SceneGraphNode*         _node;
    SceneGraphNode*			_planeSGN;
    ///Normal rendering state
    RenderStateBlock*       _terrainRenderState;
    ///Depth map rendering state
    RenderStateBlock*       _terrainDepthRenderState;
    ///Reflection rendering state
    RenderStateBlock*       _terrainReflectionRenderState;
};

#endif
