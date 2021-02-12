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
#ifndef _DOF_PRE_RENDER_OPERATOR_H_
#define _DOF_PRE_RENDER_OPERATOR_H_

#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/PostFX/Headers/PreRenderOperator.h"

namespace Divide {

class DoFPreRenderOperator final : public PreRenderOperator {
   public:
       struct Parameters {
           /// Autofocus point on screen (0.0,0.0 - left lower corner, 1.0,1.0 - upper right)
           vec2<F32> _focalPoint = { 0.5f };
           /// Focal distance value in meters
           F32 _focalDepth = 10.0f;
           /// Focal length in mm
           F32 _focalLength = 12.0f;
           /// f-stop value
           FStops _fStop = FStops::F_1_4;
           /// Near dof blur start
           F32 _ndofstart = 1.0f;
           /// Near dof blur falloff distance
           F32 _ndofdist = 2.0f; 
           /// Far dof blur start
           F32 _fdofstart = 1.0f;
           /// Far dof blur falloff distance
           F32 _fdofdist = 3.0f;
           /// Vignetting outer border
           F32 _vignout = 1.3f;
           /// Vignetting inner border
           F32 _vignin = 0.0f; 
           /// Use autofocus in shader? disable if you use external focalDepth value
           bool _autoFocus = true;
           /// Use optical lens vignetting
           bool _vignetting = false;
           /// Show debug focus point and focal range (red = focal point, green = focal range)
           bool _debugFocus = false;
           /// Manual dof calculation
           bool _manualdof = false;
       };

   public:
    DoFPreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache* cache);
    ~DoFPreRenderOperator() = default;

    [[nodiscard]] bool execute(const Camera* camera, const RenderTargetHandle& input, const RenderTargetHandle& output, GFX::CommandBuffer& bufferInOut) override;
    void reshape(U16 width, U16 height) override;

    PROPERTY_R(Parameters, parameters);

    void parameters(const Parameters& params) noexcept;

    [[nodiscard]] bool ready() const override;

   private:
     ShaderProgram_ptr _dofShader = nullptr;
     Pipeline* _pipeline = nullptr;
     PushConstants _constants;
     vec2<F32> _cachedZPlanes = VECTOR2_UNIT;
     bool _constantsDirty = true;
};

}  // namespace Divide

#endif //_DOF_PRE_RENDER_OPERATOR_H_
