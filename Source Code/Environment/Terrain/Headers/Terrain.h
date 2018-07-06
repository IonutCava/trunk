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

#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "Graphs/Headers/SceneNode.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Environment/Vegetation/Headers/Vegetation.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

struct TerrainTextureLayer {
    TerrainTextureLayer()
    {
        _lastOffset = 0;

        for (U8 i = 0; i < TEXTURE_USAGE_PLACEHOLDER; ++i)
            _texture[i] = nullptr;

        memset(_diffuseUVScale, 0.0f, (4) * sizeof(F32));
        memset(_detailUVScale,  0.0f, (4) * sizeof(F32));
    }

    ~TerrainTextureLayer();

    enum TerrainTextureUsage {
        TEXTURE_BLEND_MAP = 0,
        TEXTURE_ALBEDO_MAPS = 1,
        TEXTURE_DETAIL_MAPS = 2,
        TEXTURE_USAGE_PLACEHOLDER = 3
    };

    enum TerrainTextureChannel {
        TEXTURE_RED_CHANNEL = 0,
        TEXTURE_GREEN_CHANNEL = 1,
        TEXTURE_BLUE_CHANNEL = 2,
        TEXTURE_ALPHA_CHANNEL = 3
    };

    void bindTextures(U32 offset);
    void unbindTextures();

    inline void setTexture(TerrainTextureUsage textureUsage, Texture* texture){
        assert(textureUsage < TEXTURE_USAGE_PLACEHOLDER && textureUsage >= TEXTURE_BLEND_MAP);
        _texture[textureUsage] = texture;
    }

    inline void setTextureScale(TerrainTextureUsage textureUsage, TerrainTextureChannel textureChannel, F32 scale) {
        assert(textureUsage < TEXTURE_USAGE_PLACEHOLDER && textureUsage > TEXTURE_BLEND_MAP);

        if (textureUsage == TEXTURE_ALBEDO_MAPS) _diffuseUVScale[textureChannel] = scale;
        else                                     _detailUVScale[textureChannel] = scale;
    }

    inline F32 getTextureScale(TerrainTextureUsage textureUsage, TerrainTextureChannel textureChannel) {
        assert(textureUsage < TEXTURE_USAGE_PLACEHOLDER && textureUsage > TEXTURE_BLEND_MAP);

        if (textureUsage == TEXTURE_ALBEDO_MAPS) return _diffuseUVScale[textureChannel];
        else                                     return _detailUVScale[textureChannel];
    }

private:
    U32 _lastOffset;
    F32 _diffuseUVScale[4];
    F32 _detailUVScale[4];
    Texture* _texture[TEXTURE_USAGE_PLACEHOLDER];
};

class Quad3D;
class Quadtree;
class Transform;
class VertexBuffer;
class ShaderProgram;
class TerrainDescriptor;

class Terrain : public SceneNode {
    friend class TerrainLoader;
public:

    Terrain();
    ~Terrain();

    bool unload();

    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage);
    void drawBoundingBox(SceneGraphNode* const sgn) const;
    inline void toggleBoundingBoxes(){ _drawBBoxes = !_drawBBoxes; }

    vec3<F32>  getPositionFromGlobal(F32 x, F32 z) const;
    vec3<F32>  getPosition(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getNormal(F32 x_clampf, F32 z_clampf) const;
    vec3<F32>  getTangent(F32 x_clampf, F32 z_clampf) const;
    vec2<F32>  getDimensions(){return vec2<F32>((F32)_terrainWidth, (F32)_terrainHeight);}

           void  terrainSmooth(F32 k);
           void  initializeVegetation(TerrainDescriptor* const terrain,SceneGraphNode* const terrainSGN);

    inline VertexBuffer* const getGeometryVB() {return _groundVB;}
    inline Quadtree&           getQuadtree()   const {return *_terrainQuadtree;}

    bool computeBoundingBox(SceneGraphNode* const sgn);

	Vegetation* const getVegetation() const;
    void toggleVegetation(bool state);

protected:

    void postDraw(SceneGraphNode* const sgn, const RenderStage& currentStage);

    void drawGround(const SceneRenderState& sceneRenderState) const;
    void drawInfinitePlain(const SceneRenderState& sceneRenderState) const;

    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState);
    bool prepareMaterial(SceneGraphNode* const sgn);
    bool releaseMaterial();
    bool prepareDepthMaterial(SceneGraphNode* const sgn);
    bool releaseDepthMaterial();

    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);

    void postLoad(SceneGraphNode* const sgn);
    void setCausticsTex(Texture* causticTexture);
    void setUnderwaterAlbedoTex(Texture* underwaterAlbedoTexture);
    void setUnderwaterDetailTex(Texture* underwaterDetailTexture);

    inline void setUnderwaterDiffuseScale(F32 diffuseScale) {_underwaterDiffuseScale = diffuseScale;}

protected:
    friend class TerrainChunk;
    void renderChunkCallback(U8 lod);

protected:

    U8            _lightCount;
    U16			  _terrainWidth;
    U16           _terrainHeight;
    U32           _chunkSize;
    Quadtree*	  _terrainQuadtree;
    VertexBuffer* _groundVB;

    vec2<F32> _terrainScaleFactor;
    F32	 _farPlane;
    U64  _stateRefreshInterval;
    U64  _stateRefreshIntervalBuffer;
    bool _alphaTexturePresent;
    bool _drawBBoxes;
    bool _shadowMapped;

    Vegetation*       _vegetation;
    SceneGraphNode*   _vegetationGrassNode;
    BoundingBox       _boundingBox;
    Quad3D*		      _plane;
    Transform*		  _planeTransform;
    Transform*        _terrainTransform;
    SceneGraphNode*   _node;
    SceneGraphNode*	  _planeSGN;

    Texture*           _causticsTex;
    Texture*           _underwaterAlbedoTex;
    Texture*           _underwaterDetailTex;
    F32                _underwaterDiffuseScale;
    vectorImpl<TerrainTextureLayer* > _terrainTextures;
    ///Normal rendering state
    RenderStateBlock* _terrainRenderState;
    ///Depth map rendering state
    RenderStateBlock* _terrainDepthRenderState;
    ///PrePass rendering state
    RenderStateBlock* _terrainPrePassRenderState;
    ///Reflection rendering state
    RenderStateBlock*  _terrainReflectionRenderState;
};

#endif
