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

#ifndef _BLOOM_PRE_RENDER_OPERATOR_H_
#define _BLOOM_PRE_RENDER_OPERATOR_H_
#include "Rendering/PostFX/Headers/PreRenderOperator.h"

class ShaderProgram;
class Quad3D;
class Texture;
typedef Texture Texture2D;
class FrameBufferObject;
class BloomPreRenderOperator : public PreRenderOperator {
public:
	BloomPreRenderOperator(ShaderProgram* const bloomShader, Quad3D* const target, FrameBufferObject* result, const vec2<U16>& resolution);
	~BloomPreRenderOperator();

	void operation();
	void reshape(I32 width, I32 height);

private:
	ShaderProgram* _blur;
	ShaderProgram* _bright;
	FrameBufferObject* _outputFBO;
	FrameBufferObject* _tempBloomFBO;

};

#endif