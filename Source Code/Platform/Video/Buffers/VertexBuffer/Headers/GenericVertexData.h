/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _GENERIC_VERTEX_DATA_H
#define _GENERIC_VERTEX_DATA_H

#include "VertexDataInterface.h"
#include "Managers/Headers/FrameListenerManager.h"

/// This class is used to upload generic VB data to the GPU that can be rendered
/// directly or instanced.
/// Use this class to create precise VB data with specific usage (such as
/// particle systems)
/// Use IMPrimitive for on-the-fly geometry creation

namespace Divide {

class NOINITVTABLE GenericVertexData : public VertexDataInterface,
                                       public FrameListener {
   public:
    struct AttributeDescriptor {
        AttributeDescriptor()
            : _index(0),
              _divisor(0),
              _parentBuffer(0),
              _componentsPerElement(0),
              _elementCountOffset(0),
              _wasSet(false),
              _dirty(false),
              _normalized(false),
              _stride(0),
              _type(GFXDataFormat::UNSIGNED_INT)
        {
        }

        void set(U32 bufferIndex, U32 instanceDivisor, U32 componentsPerElement,
                 bool normalized, size_t stride, U32 elementCountOffset,
                 GFXDataFormat dataType) {
            this->bufferIndex(bufferIndex);
            this->instanceDivisor(instanceDivisor);
            this->componentsPerElement(componentsPerElement);
            this->normalized(normalized);
            this->stride(stride);
            this->offset(elementCountOffset);
            this->dataType(dataType);
        }

        inline void attribIndex(U32 index) {
            _index = index;
            _dirty = true;
        }

        inline void offset(U32 elementCountOffset) {
            _elementCountOffset = elementCountOffset;
            _dirty = true;
        }

        inline void bufferIndex(U32 bufferIndex) {
            _parentBuffer = bufferIndex;
            _dirty = true;
        }

        inline void instanceDivisor(U32 instanceDivisor) {
            _divisor = instanceDivisor;
            _dirty = true;
        }

        inline void componentsPerElement(U32 componentsPerElement) {
            _componentsPerElement = componentsPerElement;
            _dirty = true;
        }

        inline void normalized(bool normalized) {
            _normalized = normalized;
            _dirty = true;
        }

        inline void stride(size_t stride) {
            _stride = stride;
            _dirty = true;
        }

        inline void dataType(GFXDataFormat type) {
            _type = type;
            _dirty = true;
        }

        inline void wasSet(bool wasSet) { _wasSet = wasSet; }

        inline void clean() { _dirty = false; }

        inline U32 attribIndex() const { return _index; }
        inline U32 offset() const { return _elementCountOffset; }
        inline U32 bufferIndex() const { return _parentBuffer; }
        inline U32 instanceDivisor() const { return _divisor; }
        inline U32 componentsPerElement() const {
            return _componentsPerElement;
        }
        inline bool normalized() const { return _normalized; }
        inline size_t stride() const { return _stride; }
        inline GFXDataFormat dataType() const { return _type; }
        inline bool wasSet() const { return _wasSet; }
        inline bool dirty() const { return _dirty; }

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

    GenericVertexData(GFXDevice& context, bool persistentMapped)
        : VertexDataInterface(context),
          FrameListener(),
          _persistentMapped(persistentMapped &&
                            !Config::Profile::DISABLE_PERSISTENT_BUFFER)
    {
        REGISTER_FRAME_LISTENER(this, 4);
        _doubleBufferedQuery = true;
    }

    virtual ~GenericVertexData()
    {
        _attributeMapDraw.clear();
        _attributeMapFdbk.clear();
        UNREGISTER_FRAME_LISTENER(this);
    }

    virtual void setIndexBuffer(U32 indicesCount, bool dynamic, bool stream) = 0;
    virtual void updateIndexBuffer(const vectorImpl<U32>& indices) = 0;
    virtual void create(U8 numBuffers = 1, U8 numQueries = 1) = 0;
    virtual void setFeedbackBuffer(U32 buffer, U32 bindPoint) = 0;

    virtual void draw(const GenericDrawCommand& command,
                      bool useCmdBuffer = false) = 0;

    /// When reading and writing to the same buffer, we use a round-robin
    /// approach and
    /// offset the reading and writing to multiple copies of the data
    virtual void setBuffer(U32 buffer, U32 elementCount, size_t elementSize,
                           U8 sizeFactor, void* data, bool dynamic, bool stream,
                           bool persistentMapped = false) = 0;

    virtual void updateBuffer(U32 buffer, U32 elementCount,
                              U32 elementCountOffset, void* data) = 0;

    virtual void bindFeedbackBufferRange(U32 buffer, U32 elementCountOffset,
                                         size_t elementCount) = 0;

    virtual U32 getFeedbackPrimitiveCount(U8 queryID) = 0;
    /// Just before we render the frame
    virtual bool frameStarted(const FrameEvent& evt) { return true; }

    inline void toggleDoubleBufferedQueries(const bool state) {
        _doubleBufferedQuery = state;
    }

    inline AttributeDescriptor& getDrawAttribDescriptor(U32 attribIndex) {
        AttributeDescriptor& desc = _attributeMapDraw[attribIndex];
        desc.attribIndex(attribIndex);
        return desc;
    }

    inline AttributeDescriptor& getFdbkAttribDescriptor(U32 attribIndex) {
        AttributeDescriptor& desc = _attributeMapFdbk[attribIndex];
        desc.attribIndex(attribIndex);
        return desc;
    }

   protected:
    typedef hashMapImpl<U32, AttributeDescriptor> attributeMap;
    bool _doubleBufferedQuery;
    vectorImpl<U32> _feedbackBuffers;
    attributeMap _attributeMapDraw;
    attributeMap _attributeMapFdbk;

    const bool _persistentMapped;
};

};  // namespace Divide

#endif
