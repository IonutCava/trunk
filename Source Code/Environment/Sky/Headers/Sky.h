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

#ifndef _SKY_H
#define _SKY_H

#include "core.h"
#include "Graphs/Headers/SceneNode.h"

class Texture;
class Sphere3D;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;

enum RenderStage;

class Sky : public SceneNode {
public:
    Sky(const std::string& name);
    ~Sky();

    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage);
    void setRenderingOptions(bool drawSun = true, bool drawSky = true) ;
    void setSunVector(const vec3<F32>& sunVect);
    ///Skies are always visible (for now. Interiors will change that. Windows will require a occlusion query(?))
    bool isInView(const SceneRenderState& sceneRenderState, const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck = false) { return true; }
    void drawBoundingBox(SceneGraphNode* const sgn) const {}

protected:
    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState);
    bool prepareMaterial(SceneGraphNode* const sgn);
    void postLoad(SceneGraphNode* const sgn);
    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);

private:
    bool load();

private:
    bool			  _drawSky,_drawSun;
    ShaderProgram*	  _skyShader;
    Texture*   	      _skybox;
    vec3<F32>		  _sunVect;
    Sphere3D          *_sky,*_sun;
    SceneGraphNode    *_sunNode, *_skyGeom;
    U16				  _exclusionMask;
    RenderStateBlock* _skyboxRenderState;
	RenderStateBlock* _skyboxRenderStateReflected;
};

#endif
