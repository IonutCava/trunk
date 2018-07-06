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

#ifndef _GENERIC_VERTEX_DATA_H
#define _GENERIC_VERTEX_DATA_H
#include <boost/noncopyable.hpp>
#include "Utility/Headers/Vector.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Managers/Headers/FrameListenerManager.h"
/// This class is used to upload generic VB data to the GPU that can be rendered directly or instanced.
/// Use this class to create precise VB data with specific usage (such as particle systems)
/// Use IMPrimitive for on-the-fly geometry creation

class ShaderProgram;
class GenericVertexData : private boost::noncopyable, public FrameListener {
public:
    struct GenericDrawCommand {
        U32 _min;
        U32 _max;
        U8  _queryID;
        I64 _stateHash;
        U32 _instanceCount;
        bool _drawToBuffer;
        U8   _lod;
        PrimitiveType _type;

        inline void setLoD(U8 lod) { _lod = lod; }
        GenericDrawCommand(const PrimitiveType& type, I64 stateHash, U32 min, U32 max, U32 instanceCount = 1,
                           U8 queryID = 0, bool drawToBuffer = false) : _type(type),
                                                                        _stateHash(stateHash),
                                                                        _min(min),
                                                                        _max(max),
                                                                        _lod(0),
                                                                        _queryID(queryID),
                                                                        _drawToBuffer(drawToBuffer),                         
                                                                        _instanceCount(instanceCount)
                                                                    
        {
        }
    };

    struct AttributeDescriptor {
        AttributeDescriptor() : _index(0), _divisor(0), _parentBuffer(0), 
                                _componentsPerElement(0), _normalized(false),
                                _stride(0), _type(UNSIGNED_INT), 
                                _wasSet(false), _elementCountOffset(0), _dirty(false)
        {
        }

        void set(U32 bufferIndex, U32 instanceDivisor, U32 componentsPerElement, bool normalized, size_t stride, U32 elementCountOffset, GFXDataFormat dataType){
            setBufferIndex(bufferIndex);
            setInstanceDivisor(instanceDivisor);
            setComponentsPerElement(componentsPerElement);
            setNormalized(normalized);
            setStride(stride);
            setOffset(elementCountOffset);
            _type = dataType;
        }

        inline void setOffset(U32 elementCountOffset)                 { _elementCountOffset = elementCountOffset; _dirty = true; }
        inline void setBufferIndex(U32 bufferIndex)                   { _parentBuffer = bufferIndex; _dirty = true; }
        inline void setInstanceDivisor(U32 instanceDivisor)           { _divisor = instanceDivisor; _dirty = true; }
        inline void setComponentsPerElement(U32 componentsPerElement) { _componentsPerElement = componentsPerElement; _dirty = true; }
        inline void setNormalized(bool normalized)                    { _normalized = normalized; _dirty = true; }
        inline void setStride(size_t stride)                          { _stride = stride; _dirty = true; }

    protected:
        friend class GenericVertexData;
        friend class glGenericVertexData;
        friend class d3dGenericVertexData;
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

    GenericVertexData(bool persistentMapped) : FrameListener(), _persistentMapped(persistentMapped)
    {
        REGISTER_FRAME_LISTENER(this, 4);
        _doubleBufferedQuery = true;
        _currentShader = nullptr;
    }

    virtual ~GenericVertexData()
    {
        _attributeMapDraw.clear();
        _attributeMapFdbk.clear();
    }

    virtual void Create(U8 numBuffers = 1, U8 numQueries = 1) = 0;
    virtual void SetFeedbackBuffer(U32 buffer, U32 bindPoint) = 0;

    virtual void Draw(const GenericDrawCommand& command) = 0;

    virtual void SetBuffer(U32 buffer, U32 elementCount, size_t elementSize, void* data, bool dynamic, bool stream, bool persistentMapped = false) = 0;
    virtual void UpdateBuffer(U32 buffer, U32 elementCount, void* data, U32 offset, bool dynamic, bool steam) = 0;

    virtual void BindFeedbackBufferRange(U32 buffer, size_t elementCountOffset, size_t elementCount) = 0;

    virtual U32  GetFeedbackPrimitiveCount(U8 queryID) = 0;
    /// Just before we render the frame
    virtual bool frameStarted(const FrameEvent& evt) { return true; }

    inline void toggleDoubleBufferedQueries(const bool state) { _doubleBufferedQuery = state; }

    inline AttributeDescriptor& getDrawAttribDescriptor(U32 attribIndex) { AttributeDescriptor& desc = _attributeMapDraw[attribIndex]; desc._index = attribIndex; return desc; }
    inline AttributeDescriptor& getFdbkAttribDescriptor(U32 attribIndex) { AttributeDescriptor& desc = _attributeMapFdbk[attribIndex]; desc._index = attribIndex; return desc; }

    inline void setShaderProgram(ShaderProgram* const shaderProgram) { _currentShader = shaderProgram;}

protected:
    typedef Unordered_map<U32, AttributeDescriptor > attributeMap;
    bool _persistentMapped;
    bool _doubleBufferedQuery;
    vectorImpl<U32 > _feedbackBuffers;
    vectorImpl<U32 > _bufferObjects;
    attributeMap _attributeMapDraw;
    attributeMap _attributeMapFdbk;
    ShaderProgram* _currentShader;
};

#endif
