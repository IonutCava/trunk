/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SSAO_PRE_RENDER_OPERATOR_H_
#define _SSAO_PRE_RENDER_OPERATOR_H_
#include "Rendering/PostFX/Headers/PreRenderOperator.h"
///This Operator processes the entire geometry via the SSAOShader, generating a intermediate FBO
///The intermidiate FBO contains each object's normals in the "rgb" components, and the linear depth in the "a" component
///The intermidiate FBO is then passed as a 2D sampler in the second stage shader, processed and produces a full screen texure as a result
///The result FBO contains AO ambient values that should be added to the final fragment's ambient lighting value
class ShaderProgram;
class Quad3D;
class Texture;
typedef Texture Texture2D;
class FrameBufferObject;
class SSAOPreRenderOperator : public PreRenderOperator {
public:
	SSAOPreRenderOperator(ShaderProgram* const SSAOShader, Quad3D* const target, FrameBufferObject* result, const vec2<U16>& resolution);
	~SSAOPreRenderOperator();

	void operation();
	void reshape(I32 width, I32 height);

private:
	ShaderProgram* _stage1Shader;
	ShaderProgram* _stage2Shader;
	FrameBufferObject* _outputFBO;
	FrameBufferObject* _normalsFBO;
	Texture2D*		   _colorNoise;

};

#endif