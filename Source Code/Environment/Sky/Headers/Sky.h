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

#ifndef _SKY_H
#define _SKY_H

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

class Texture;
class Sphere3D;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;

typedef Texture TextureCubemap;

enum RenderStage;

class Sky : public SceneNode {
public:
    Sky(const std::string& name);
    ~Sky();

    void onDraw(const RenderStage& currentStage);
    void render(SceneGraphNode* const sgn);
    void setRenderingOptions(bool drawSun = true, bool drawSky = true) ;
    void prepareMaterial(SceneGraphNode* const sgn);
    void releaseMaterial();

    void setSunVector(const vec3<F32>& sunVect);

    void addToDrawExclusionMask(I32 stageMask);
    void removeFromDrawExclusionMask(I32 stageMask);
    ///Draw states are used to test if the current object should be drawn depending on the current render pass
    bool getDrawState(const RenderStage& currentStage) const;
    ///Skies are always visible (for now. Interiors will change that. Windows will reuqire a occlusion querry(?))
    bool isInView(const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck = false) {return true;}
    void postLoad(SceneGraphNode* const sgn);

private:
    bool load();

private:
    bool			  _init,_drawSky,_drawSun;
    ShaderProgram*	  _skyShader;
    TextureCubemap*	  _skybox;
    vec3<F32>		  _sunVect;
    Sphere3D          *_sky,*_sun;
    SceneGraphNode    *_sunNode, *_skyGeom;
    U8				  _exclusionMask;
    RenderStateBlock* _skyboxRenderState;
};

#endif
