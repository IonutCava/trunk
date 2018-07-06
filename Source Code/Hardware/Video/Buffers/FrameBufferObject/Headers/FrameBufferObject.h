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

#ifndef _FRAME_BUFFER_OBJECT_H
#define _FRAME_BUFFER_OBJECT_H

#include "core.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/Textures/Headers/TextureDescriptor.h"
#include <boost/noncopyable.hpp>

class FrameBufferObject : private boost::noncopyable{
public:
    bool AddAttachment(const TextureDescriptor& decriptor, TextureDescriptor::AttachmentType slot);

    virtual bool Create(U16 width, U16 height, U8 imageLayers = 0) = 0;

    virtual void Destroy() = 0;
    virtual void DrawToLayer(TextureDescriptor::AttachmentType slot, U8 layer, bool includeDepth = true) const = 0; ///<Use by multilayerd FBO's
    virtual void Begin(U8 nFace=0) const = 0;
    virtual void End(U8 nFace=0) const = 0;

    virtual void Bind(U8 unit = 0, TextureDescriptor::AttachmentType slot = TextureDescriptor::Color0) const;
    virtual void Unbind(U8 unit = 0) const;
    // regenerates mip maps for the currently bound color attachement (must be called  between Bind() / Unbind() to take effect)
    virtual void UpdateMipMaps(TextureDescriptor::AttachmentType slot) const = 0;
    virtual void BlitFrom(FrameBufferObject* inputFBO) const = 0;
    //Enable/Disable color writes
    virtual void toggleColorWrites(bool state) {_disableColorWrites = !state;}
    //Enable/Disable the presence of a depth renderbuffer
    virtual void toggleDepthBuffer(bool state, bool useFloatingPoint = false) {_useDepthBuffer = state; _fpDepth = useFloatingPoint;}
    //Set the color the FBO will clear to when drawing to it
    inline void setClearColor(const vec4<F32>& clearColor) { _clearColor.set(clearColor); }

    inline U16 getWidth()  const	{return _width;}
    inline U16 getHeight() const	{return _height;}
    inline U8  getType()   const	{return _fboType;}
    inline U8  getHandle() const	{return _frameBufferHandle;}

    inline void clearBuffers(bool state)       {_clearBuffersState = state;}
    inline bool clearBuffers()           const {return _clearBuffersState;}
    inline void clearColor(bool state)         {_clearColorState = state;}
    inline bool clearColor()             const {return _clearColorState;}

    FrameBufferObject(FBOType type);
    virtual ~FrameBufferObject();

protected:
    virtual bool checkStatus() const = 0;

protected:
    typedef Unordered_map<TextureDescriptor::AttachmentType, TextureDescriptor >  TextureAttachements;

    mutable bool _bound;
    bool        _clearBuffersState;
    bool        _clearColorState;
    bool		_useDepthBuffer;
    bool        _fpDepth;
    bool        _disableColorWrites;
    U16		    _width, _height;
    U32		    _frameBufferHandle;
    U32		    _textureType;
    U8          _fboType;
    vec4<F32>   _clearColor;
    TextureAttachements _attachement;
    Unordered_map<TextureDescriptor::AttachmentType, bool > _attachementDirty;
};

#endif
