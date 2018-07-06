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

#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/Reflector.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {

class Texture;
class CameraManager;
class ShaderProgram;
class WaterPlane : public SceneNode, public Reflector {
   public:
    /// Resource inherited "unload"
    bool unload() override;
    /// General SceneNode stuff
    bool onDraw(SceneGraphNode& sgn, RenderStage currentStage) override;

    bool getDrawState(RenderStage currentStage);

    void setParams(F32 shininess, const vec2<F32>& noiseTile,
                   const vec2<F32>& noiseFactor, F32 transparency);

    void sceneUpdate(const U64 deltaTime,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;

    inline Quad3D* getQuad() const { return _plane; }

    inline const ClipPlaneIndex getReflectionPlaneID() {
        return _reflectionPlaneID;
    }
    inline const ClipPlaneIndex getRefractionPlaneID() {
        return _refractionPlaneID;
    }
    /// Reflector overwrite
    void updateReflection();
    void updateRefraction();
    void updatePlaneEquation();
    /// Used for many things, such as culling switches, and underwater effects
    inline bool isPointUnderWater(const vec3<F32>& pos) {
        return (pos.y < _waterLevel);
    }

    inline void setReflectionCallback(const DELEGATE_CBK<>& callback) {
        Reflector::setRenderCallback(callback);
    }
    inline void setRefractionCallback(const DELEGATE_CBK<>& callback) {
        _refractionCallback = callback;
    }

   protected:
    SET_DELETE_FRIEND

    template <typename T>
    friend class ImplResourceLoader;

    WaterPlane();

    ~WaterPlane();

    bool getDrawCommands(SceneGraphNode& sgn,
                         RenderStage renderStage,
                         const SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut) override;

    void postLoad(SceneGraphNode& sgn) override;

    void previewReflection();

    inline const Plane<F32>& getRefractionPlane() { return _refractionPlane; }

   private:
    void computeBoundingBox();

   private:
    /// the hw clip-plane index for the water
    ClipPlaneIndex _reflectionPlaneID;
    ClipPlaneIndex _refractionPlaneID;
    /// cached far plane value
    F32 _farPlane;
    /// cached water level
    F32 _waterLevel;
    /// cached water depth
    F32 _waterDepth;
    /// Last used orientation
    Quaternion<F32> _orientation;
    /// the water's "geometry"
    Quad3D* _plane;
    Framebuffer* _refractionTexture;
    Plane<F32> _refractionPlane;
    DELEGATE_CBK<> _refractionCallback;
    bool _refractionRendering;
    bool _reflectionRendering;
    bool _dirty, _paramsDirty;
    bool _cameraUnderWater;
    F32 _shininess;
    vec2<F32> _noiseTile;
    vec2<F32> _noiseFactor;
    F32 _transparency;
    CameraManager& _cameraMgr;
};

};  // namespace Divide

#endif