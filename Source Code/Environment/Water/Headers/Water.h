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

#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "core.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/Reflector.h"

class ShaderProgram;

class Texture;
typedef Texture Texture2D;

class WaterPlane : public SceneNode, public Reflector{
public:
    WaterPlane();
    ~WaterPlane(){}

    /// Resource inherited "unload"
    bool unload();

    /// General SceneNode stuff
    void onDraw(const RenderStage& currentStage);
    void postDraw(const RenderStage& currentStage);
    void render(SceneGraphNode* const sgn);
    void postLoad(SceneGraphNode* const sgn);
    void prepareMaterial(SceneGraphNode* const sgn);
    void releaseMaterial();
    void prepareDepthMaterial(SceneGraphNode* const sgn){}
    void releaseDepthMaterial(){}
    bool getDrawState(const RenderStage& currentStage)  const;

    bool isInView(const BoundingBox& boundingBox,const BoundingSphere& sphere, const bool distanceCheck = true) {return true;}

    void setParams(F32 shininess, const vec2<F32>& noiseTile, const vec2<F32>& noiseFactor, F32 transparency);
    inline Quad3D*     getQuad()    {return _plane;}

    /// Reflector overwrite
    void updateReflection();
    void updatePlaneEquation();
    /// Used for many things, such as culling switches, and underwater effects
    inline bool isPointUnderWater(const vec3<F32>& pos) { return (pos.y < _waterLevel); }

protected:
    template<typename T>
    friend class ImplResourceLoader;
    inline void setWaterNormalMap(Texture2D* const waterNM){_texture = waterNM;}
    inline void setShaderProgram(ShaderProgram* const shaderProg){_shader = shaderProg;}
    inline void setGeometry(Quad3D* const waterPlane){_plane = waterPlane;}

private:
    /// Bounding Box computation overwrite from SceneNode
    bool computeBoundingBox(SceneGraphNode* const sgn);

private:
    /// number of lights in the scene
    U8                 _lightCount;
    /// the hw clip-plane index for the water
    I32                _clippingPlaneID;
    /// cached far plane value
    F32				   _farPlane;
    /// cached water level
    F32                _waterLevel;
    /// cached water depth
    F32                _waterDepth;
    /// Last known camera position
    vec3<F32>          _eyePos;
    /// Camera's position delta from the previous frame only on the XY plane
    vec2<F32>          _eyeDiff;
    /// The plane's transformed normal
    vec3<F32>          _absNormal;
    /// the water's "geometry"
    Quad3D*			   _plane;
    Texture2D*		   _texture;
    ShaderProgram* 	   _shader;
    Transform*         _planeTransform;
    SceneGraphNode*    _node;
    SceneGraphNode*    _planeSGN;
    bool               _reflectionRendering;
};

#endif