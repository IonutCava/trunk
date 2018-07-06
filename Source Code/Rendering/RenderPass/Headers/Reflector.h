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

#ifndef REFLECTOR_H_
#define REFLECTOR_H_

#include "Core/Math/Headers/Plane.h"
#include "Managers/Headers/FrameListenerManager.h"

namespace Divide {

enum class ReflectorType : U32 {
    TYPE_MIRROR = 0,
    TYPE_WATER_SURFACE,
    TYPE_PORTAL,
    TYPE_MATERIAL,
    COUNT
};

class Quad3D;
class Framebuffer;
class ShaderProgram;
/// Virtual interface for reflective surfaces
class NOINITVTABLE Reflector : public FrameListener {
   public:
    Reflector(ReflectorType type, const vec2<U16>& resolution);
    virtual ~Reflector();

    /// This function should be unique to every reflector.
    /// Portals may need special effects, mirrors some special lighting, etc
    virtual void updateReflection() = 0;
    /// Each reflector has a certain plane equation.
    /// Update this after each transform to obtain proper reflections
    virtual void updatePlaneEquation() = 0;
    /// Retrieves the reflection texture
    inline Framebuffer* getReflectionFB() { return _reflectedTexture; }
    inline const Plane<F32>& getReflectionPlane() { return _reflectionPlane; }
    /// Rendering callback is that function in the scene used to render the
    /// reflection
    /// The SceneGraph is not the default rendering class for reflections as
    /// some elements
    // (i.e. Sky) are not part of it
    /// As the sky and other elements should be included in the reflection,
    /// we should set a custom callback
    /// Another example would be the player model. It is not rendered when in
    /// First Person,
    /// but it should show up in reflections
    inline void setRenderCallback(const DELEGATE_CBK<>& callback) {
        _renderCallback = callback;
    }

    void togglePreviewReflection() { _previewReflection = !_previewReflection; }

   protected:
    /// This is inherited from FrameListener and is used to queue up reflection
    /// on every frame start
    bool framePreRenderEnded(const FrameEvent& evt);
    virtual void previewReflection();
    /// Create FB
    bool build();

   protected:
    ReflectorType _type;
    /// How often do we need to update this instance's reflection?
    U32 _updateInterval;
    /// What is our current tick count
    F32 _updateTimer;
    /// What resolution should the generated texture have?
    vec2<U16> _resolution;
    /// Use this to force current reflector to draw itself in reflection
    bool _excludeSelfReflection;
    bool _previewReflection;

    DELEGATE_CBK<> _renderCallback;
    Framebuffer* _reflectedTexture;
    Plane<F32> _reflectionPlane;
    ShaderProgram* _previewReflectionShader;
    /// used for render exclusion. Do not render self in own reflection
    bool _updateSelf;
};

};  // namespace Divide

#endif