/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _SKY_H
#define _SKY_H

#include "Graphs/Headers/SceneNode.h"

namespace Divide {

class Texture;
class Sphere3D;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;

enum class RenderStage : U32;

class Sky : public SceneNode {
   public:
    Sky(const stringImpl& name);
    ~Sky();

    bool onDraw(SceneGraphNode& sgn, RenderStage currentStage);
    void setSunProperties(const vec3<F32>& sunVect, const vec4<F32>& sunColor);
    /// Skies are always visible (for now. Interiors will change that. Windows
    /// will require a occlusion query(?))
    bool isInView(const SceneRenderState& sceneRenderState,
                  SceneGraphNode& sgn, const bool distanceCheck = false) {
        return true;
    }

   protected:
    void postLoad(SceneGraphNode& sgn);
    void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                     SceneState& sceneState);
    void getDrawCommands(SceneGraphNode& sgn,
                         RenderStage currentRenderStage,
                         SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut);

   private:
    bool load();

   private:
    ShaderProgram* _skyShader;
    Texture* _skybox;
    Sphere3D* _sky;
    U16 _exclusionMask;
    size_t _skyboxRenderStateHash;
    size_t _skyboxRenderStateReflectedHash;
};

};  // namespace Divide

#endif
