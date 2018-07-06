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

#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class Texture;
class ShaderProgram;

class WaterPlane : public SceneNode {
   public:
    explicit WaterPlane(const stringImpl& name, I32 sideLength);
    ~WaterPlane();

    /// Resource inherited "unload"
    bool unload() override;
    /// General SceneNode stuff
    bool onRender(RenderStage currentStage) override;

    bool getDrawState(RenderStage currentStage);

    void setParams(F32 shininess, const vec2<F32>& noiseTile,
                   const vec2<F32>& noiseFactor, F32 transparency);

    void sceneUpdate(const U64 deltaTime,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;

    inline const std::shared_ptr<Quad3D>& getQuad() const { return _plane; }

    void updatePlaneEquation(const SceneGraphNode& sgn,
                             Plane<F32>& reflectionPlane,
                             Plane<F32>& refractionPlane);

   protected:
    void initialiseDrawCommands(SceneGraphNode& sgn,
                                RenderStage renderStage,
                                GenericDrawCommands& drawCommandsInOut) override;
    void updateDrawCommands(SceneGraphNode& sgn,
                            RenderStage renderStage,
                            const SceneRenderState& sceneRenderState,
                            GenericDrawCommands& drawCommandsInOut) override;

    void postLoad(SceneGraphNode& sgn) override;

    inline void setSideLength(I32 length) { 
        _sideLength = std::max(length, 1);
        _paramsDirty = true;
    }

   private:
    void updateBoundsInternal(SceneGraphNode& sgn) override;
    void updateReflection(const SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          const RenderTargetID& renderTarget,
                          U32 passIndex);
    void updateRefraction(const SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          const RenderTargetID& renderTarget,
                          U32 passIndex);
    bool cameraUnderwater(const SceneGraphNode& sgn, const vec3<F32>& eyePos);

   private:
    /// cached far plane value
    I32 _sideLength;
    /// the water's "geometry"
    std::shared_ptr<Quad3D> _plane;
    bool _refractionRendering;
    bool _reflectionRendering;
    bool _paramsDirty;
    F32 _shininess;
    vec2<F32> _noiseTile;
    vec2<F32> _noiseFactor;

    /// used for render exclusion. Do not render self in own reflection
    bool _updateSelf;
    /// Use this to force current reflector to draw itself in reflection
    bool _excludeSelfReflection;

    Camera* _reflectionCam;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(WaterPlane);

};  // namespace Divide

#endif