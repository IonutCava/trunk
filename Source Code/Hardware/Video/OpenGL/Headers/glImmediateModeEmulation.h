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

#ifndef _GL_IM_EMULATION_H_
#define _GL_IM_EMULATION_H_

#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/Headers/ImmediateModeEmulation.h"

namespace NS_GLIM {
	class GLIM_BATCH;
    enum GLIM_ENUM;
};

extern NS_GLIM::GLIM_ENUM glimPrimitiveType[PrimitiveType_PLACEHOLDER];

class glIMPrimitive : public IMPrimitive {

protected:
	friend class GLWrapper;
	glIMPrimitive();
	~glIMPrimitive();

public:
    void beginBatch();
    void begin(PrimitiveType type);
    void vertex(const vec3<F32>& vert);
    ///Specify each attribute at least once(even with dummy values) before calling begin!
    void attribute4ub(const std::string& attribName, const vec4<U8>& value);
    void attribute4f(const std::string& attribName, const vec4<F32>& value);
    void end();
    void endBatch();
    void clear();

protected:
    friend class GL_API;
    void renderBatch(bool wireframe = false);
    void renderBatchInstanced(I32 count, bool wireframe = false);
protected:
    NS_GLIM::GLIM_BATCH*  _imInterface;//< Rendering API specific implementation
};

#endif