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

#ifndef _RENDER_STATE_BLOCK_H
#define _RENDER_STATE_BLOCK_H

#include "RenderAPIEnums.h"
#include "Utility/Headers/GUIDWrapper.h"
#include "Utility/Headers/CRC.h"

class RenderStateBlockDescriptor : public GUIDWrapper {

protected:
    friend class GL_API;
    friend class DX_API;
    /// Color Writes
    bool _writeRedChannel;
    bool _writeBlueChannel;
    bool _writeGreenChannel;
    bool _writeAlphaChannel;

    // Blending
    bool _blendEnable;
    BlendProperty _blendSrc;
    BlendProperty _blendDest;
    BlendOperation _blendOp;

    /// Rasterizer
    CullMode _cullMode;

    /// Depth
    bool _zEnable;
    bool _zWriteEnable;
    ComparisonFunction _zFunc;
    F32 _zBias;
    F32 _zUnits;

    /// Stencil
    bool _stencilEnable;
    U32  _stencilRef;
    U32  _stencilMask;
    U32  _stencilWriteMask;
    StencilOperation _stencilFailOp;
    StencilOperation _stencilZFailOp;
    StencilOperation _stencilPassOp;
    ComparisonFunction  _stencilFunc;
    
    FillMode   _fillMode;

private:
    U32  _cachedHash;
    bool _dirty;
    
    void clean();

public:

    RenderStateBlockDescriptor();

    void setDefaultValues();
    void fromDescriptor( const RenderStateBlockDescriptor& descriptor );

    void setFillMode(FillMode mode);
    void setZBias(F32 zBias, F32 zUnits, ComparisonFunction zFunc = CMP_FUNC_LEQUAL);
    void flipCullMode();
    void setCullMode(CullMode mode);
    void setZEnable(bool enable);
    void setZReadWrite(bool read, bool write = true);
    
    void setBlend( bool enable,
                   BlendProperty src = BLEND_PROPERTY_SRC_ALPHA,
                   BlendProperty dest = BLEND_PROPERTY_INV_SRC_ALPHA,
                   BlendOperation op = BLEND_OPERATION_ADD );

    void setStencil( bool enable, 
                     U32 stencilRef = 0, 
                     StencilOperation stencilFailOp = STENCIL_OPERATION_KEEP,
                     StencilOperation stencilZFailOp = STENCIL_OPERATION_KEEP,
                     StencilOperation stencilPassOp = STENCIL_OPERATION_KEEP,
                     ComparisonFunction stencilFunc = CMP_FUNC_NEVER);
    
    void setStencilReadWriteMask(U32 read, U32 write);

    void setColorWrites( bool red, bool green, bool blue, bool alpha );
    
    inline U32 getHash()       { clean(); return _cachedHash; }
    inline U32 getGUID() const { return getGUID(); }
};

class RenderStateBlock : public GUIDWrapper {
public:
    RenderStateBlock(const RenderStateBlockDescriptor& descriptor) : GUIDWrapper(),
                                                                     _descriptor(descriptor)
    {
    }

    virtual ~RenderStateBlock() 
    {
    }

    RenderStateBlockDescriptor& getDescriptor() {return _descriptor;}

    const RenderStateBlockDescriptor& getDescriptorConst() const { return _descriptor; }

    bool operator == (RenderStateBlock& RSB) const {
        assert(false);
        return false;
    }

    bool operator != (RenderStateBlock& RSB) const {
        assert(false);
        return false;
    }

    inline bool Compare(RenderStateBlock& RSB) { 
        return getDescriptor().getHash() == RSB.getDescriptor().getHash();
    }

    protected:
    RenderStateBlockDescriptor _descriptor;
};

#endif