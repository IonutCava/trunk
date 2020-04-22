/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _ENVIRONMENT_PROBE_H_
#define _ENVIRONMENT_PROBE_H_

#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class Scene;
class Pipeline;
class GFXDevice;
class IMPrimitive;
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);
FWD_DECLARE_MANAGED_CLASS(ImpostorSphere);

namespace GFX {
    class CommandBuffer;
}; //namespace GFX

enum class RefractorType : U8 {
    PLANAR = 0,
    COUNT
};

enum class ReflectorType : U8 {
    PLANAR = 0,
    CUBE,
    ENVIRONMENT,
    COUNT
};

FWD_DECLARE_MANAGED_CLASS(EnvironmentProbe);
class EnvironmentProbe : public GUIDWrapper {
public:
    enum class ProbeType {
        TYPE_INFINITE = 0,
        TYPE_LOCAL,
        COUNT
    };

public:
    EnvironmentProbe(Scene& parentScene, ProbeType type);
    ~EnvironmentProbe();

    static void onStartup(GFXDevice& context);
    static void onShutdown(GFXDevice& context);

    void refresh(GFX::CommandBuffer& bufferInOut);
    void setUpdateRate(U8 rate);
    void setBounds(const vec3<F32>& min, const vec3<F32>& max);
    void setBounds(const vec3<F32>& center, F32 radius);

    static RenderTargetHandle reflectionTarget();

    F32 distanceSqTo(const vec3<F32>& pos) const;
    U32 getRTIndex() const;

    void debugDraw(GFX::CommandBuffer& bufferInOut);

protected:
    void updateInternal();

    static U16 allocateSlice();

protected:
    BoundingBox _aabb;
    ImpostorSphere_ptr _impostor = nullptr;
    ShaderProgram_ptr _impostorShader = nullptr;
    Pipeline*    _bbPipeline = nullptr;
    IMPrimitive* _boundingBoxPrimitive = nullptr;
    U16 _currentArrayIndex = 0u;
    ProbeType _type = ProbeType::COUNT;
    U8 _updateRate = 1u;
    U8 _currentUpdateCall = 0u;

private:
    GFXDevice& _context;
    static vectorEASTL<bool> s_availableSlices;
    static RenderTargetHandle s_reflection;
};

using EnvironmentProbeList = vectorEASTL<EnvironmentProbe_ptr>;

}; //namespace Divide

#endif //_ENVIRONMENT_PROBE_H_
