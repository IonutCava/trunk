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

#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include "core.h"

#include "Utility/Headers/GUIDWrapper.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/Textures/Headers/TextureDescriptor.h"
#include <boost/noncopyable.hpp>

class FrameBuffer : private boost::noncopyable, public GUIDWrapper {
public:
    struct FrameBufferTarget {
        bool _depthOnly;
        bool _colorOnly;
        U32  _numColorChannels;
        bool _clearBuffersOnBind;
        FrameBufferTarget() : _depthOnly(false), _colorOnly(false), _clearBuffersOnBind(true), _numColorChannels(1)
        {
        }
    };

    inline static FrameBufferTarget& defaultPolicy() { static FrameBufferTarget _defaultPolicy; return _defaultPolicy; }

    virtual bool AddAttachment(const TextureDescriptor& descriptor, TextureDescriptor::AttachmentType slot);
    virtual bool Create(U16 width, U16 height) = 0;

    virtual void Destroy() = 0;
    virtual void DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth = true) = 0; ///<Use by multilayered FB's
    inline void DrawToFace(TextureDescriptor::AttachmentType slot, U8 nFace, bool includeDepth = true) {  ///<Used by cubemap FB's    
        DrawToLayer(slot, nFace, includeDepth);
    }

    virtual void Begin(const FrameBufferTarget& drawPolicy) = 0;
    virtual void End() = 0;

    virtual void Bind(U8 unit = 0, TextureDescriptor::AttachmentType slot = TextureDescriptor::Color0);
    virtual void Unbind(U8 unit = 0) const;
    virtual void BlitFrom(FrameBuffer* inputFB, TextureDescriptor::AttachmentType slot = TextureDescriptor::Color0, bool blitColor = true, bool blitDepth = false) = 0;
    //If true, array texture and/or cubemaps are bound to a single attachment and shader based layered rendering should be used
    virtual void toggleLayeredRendering(const bool state) {_layeredRendering = state;}
    //Enable/Disable color writes
    virtual void toggleColorWrites(const bool state) {_disableColorWrites = !state;}
    //Enable/Disable the presence of a depth renderbuffer
    virtual void toggleDepthBuffer(const bool state) {_useDepthBuffer = state;}
    //Set the color the FB will clear to when drawing to it
    inline void setClearColor(const vec4<F32>& clearColor) { _clearColor.set(clearColor); }

    inline bool isMultisampled() const { return _multisampled; }

    inline U16 getWidth()  const	{return _width;}
    inline U16 getHeight() const	{return _height;}
    inline U8  getHandle() const	{return _frameBufferHandle;}
    
    inline vec2<U16> getResolution() const {return vec2<U16>(_width, _height); }

    virtual const TextureDescriptor& GetAttachment(TextureDescriptor::AttachmentType slot) const { return _attachment[slot]; }

    inline void clearBuffers(bool state)       {_clearBuffersState = state;}
    inline bool clearBuffers()           const {return _clearBuffersState;}

    FrameBuffer(bool multiSample);
    virtual ~FrameBuffer();

protected:
    virtual bool checkStatus() const = 0;

protected:
    mutable bool _bound;
    bool        _layeredRendering;
    bool        _clearBuffersState;
    bool		_useDepthBuffer;
    bool        _disableColorWrites;
    bool        _multisampled;
    U16		    _width, _height;
    U32		    _frameBufferHandle;
    vec4<F32>   _clearColor;
    TextureDescriptor _attachment[5];
    bool              _attachmentDirty[5];
};

#endif
