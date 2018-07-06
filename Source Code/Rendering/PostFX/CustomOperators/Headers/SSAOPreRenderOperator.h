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

#ifndef _SSAO_PRE_RENDER_OPERATOR_H_
#define _SSAO_PRE_RENDER_OPERATOR_H_

#include "Rendering/PostFX/Headers/PreRenderOperator.h"
///This Operator processes the entire geometry via the SSAOShader, generating a intermediate FB
///The intermediate FB contains each object's normals in the "rgb" components, and the linear depth in the "a" component
///The intermediate FB is passed as a 2D sampler in the 2nd stage shader, processed and produces a full screen texure as a result
///The result FB contains AO ambient values that should be added to the final fragment's ambient lighting value

namespace Divide {

class SSAOPreRenderOperator : public PreRenderOperator {
public:
    SSAOPreRenderOperator(Framebuffer* result, const vec2<U16>& resolution, SamplerDescriptor* const sampler);
    ~SSAOPreRenderOperator();

    void operation();
    void reshape(I32 width, I32 height);

private:
    ShaderProgram* _ssaoShader;
    Framebuffer*   _outputFB;
};

}; //namespace Divide

#endif