/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _ATTRIBUTE_DESCRIPTOR_H_
#define _ATTRIBUTE_DESCRIPTOR_H_

#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Video/Headers/GraphicsResource.h"

namespace Divide {

    struct AttributeDescriptor {
        AttributeDescriptor();
        ~AttributeDescriptor();

        void set(U32 bufferIndex,
                 U32 instanceDivisor,
                 U32 componentsPerElement,
                 bool normalized,
                 U32 elementCountOffset,
                 GFXDataFormat dataType);

        void attribIndex(U32 index);
        void offset(U32 elementCountOffset);
        void bufferIndex(U32 bufferIndex);
        void instanceDivisor(U32 instanceDivisor);
        void componentsPerElement(U32 componentsPerElement);
        void normalized(bool normalized);
        void dataType(GFXDataFormat type);
        void wasSet(bool wasSet);
        void clean();
        void stride(size_t stride);

        inline U32 attribIndex() const { return _index; }
        inline U32 offset() const { return _elementCountOffset; }
        inline U32 bufferIndex() const { return _parentBuffer; }
        inline U32 instanceDivisor() const { return _divisor; }
        inline U32 componentsPerElement() const { return _componentsPerElement; }
        inline bool normalized() const { return _normalized; }
        inline GFXDataFormat dataType() const { return _type; }
        inline bool wasSet() const { return _wasSet; }
        inline bool dirty() const { return _dirty; }
        
        size_t stride() const;

    protected:
        U32 _index;
        U32 _divisor;
        U32 _parentBuffer;
        U32 _componentsPerElement;
        U32 _elementCountOffset;
        bool _wasSet;
        bool _dirty;
        bool _normalized;
        size_t _stride;
        GFXDataFormat _type;
    };
}; //namespace Divide

#endif //_ATTRIBUTE_DESCRIPTOR_H_
