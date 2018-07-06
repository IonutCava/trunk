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

#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"
///For now, the name of the Text3D object is the text itself
class Text3D : public Object3D
{
public:
	Text3D(const std::string& text,const std::string& font) :  Object3D(TEXT_3D),
															   _text(text),
															   _font(font),
															   _height(16),
                                                               _width(1.0f)
	{
		///Dummy
		_geometry->useHWIndices(false);
		_geometry->queueRefresh();
	}

	inline std::string&  getText()    {return _text;}
	inline std::string&	 getFont()    {return _font;}
	inline F32&			 getWidth()   {return _width;}
	inline U32&          getHeight()  {return _height;}

	virtual bool computeBoundingBox(SceneGraphNode* const sgn){
		if(sgn->getBoundingBox().isComputed()) return true;
		vec3<F32> min(-_width*2,0,-_width*0.5f);
		vec3<F32> max(_width*1.5f*_text.length()*10,_width*_text.length()*1.5f,_width*0.5f);
		sgn->getBoundingBox().set(min,max);
		return SceneNode::computeBoundingBox(sgn);
	}

private:
	std::string _text;
	std::string _font;
	F32   _width;
	U32   _height;
};

#endif