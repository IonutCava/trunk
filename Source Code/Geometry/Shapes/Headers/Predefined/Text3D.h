/*
   Copyright (c) 2013 DIVIDE-Studio
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