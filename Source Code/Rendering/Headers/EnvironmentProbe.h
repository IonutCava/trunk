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

#ifndef _ENVIRONMENT_PROBE_H_
#define _ENVIRONMENT_PROBE_H_

#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class IMPrimitive;
class ShaderProgram;
class ImpostorSphere;

struct RenderSubPassCmd;
typedef vectorImpl<RenderSubPassCmd> RenderSubPassCmds;

FWD_DECLARE_MANAGED_CLASS(EnvironmentProbe);
class EnvironmentProbe : public GUIDWrapper {
public:
    enum class ProbeType {
        TYPE_INFINITE = 0,
        TYPE_LOCAL,
        COUNT
    };

public:
    EnvironmentProbe(ProbeType type);
    ~EnvironmentProbe();

    static void onStartup();
    static void onShutdown();

    void refresh();
    void setUpdateRate(U8 rate);
    void setBounds(const vec3<F32>& min, const vec3<F32>& max);
    void setBounds(const vec3<F32>& center, F32 radius);

    static RenderTargetHandle reflectionTarget();

    F32 distanceSqTo(const vec3<F32>& pos) const;
    U32 getRTIndex() const;

    void debugDraw(RenderSubPassCmds& subPassesInOut);

protected:
    void updateInternal();

    static U32 allocateSlice();

protected:
    ProbeType _type;
    U8 _updateRate;
    U8 _currentUpdateCall;
    U32 _currentArrayIndex;
    BoundingBox _aabb;
    IMPrimitive* _boundingBoxPrimitive;
    std::shared_ptr<ImpostorSphere> _impostor;
    std::shared_ptr<ShaderProgram> _impostorShader;

private:
    static vectorImpl<bool> s_availableSlices;
    static RenderTargetHandle s_reflection;
};

typedef vectorImpl<EnvironmentProbe_ptr> EnvironmentProbeList;

}; //namespace Divide

#endif //_ENVIRONMENT_PROBE_H_
