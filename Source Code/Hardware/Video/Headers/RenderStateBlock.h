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

class RenderStateBlockDescriptor : public GUIDWrapper {
protected:
   friend class glRenderStateBlock;
   friend class d3dRenderStateBlock;
   /// Color Writes
   bool _enableColorWrite;
   bool _writeRedChannel;
   bool _writeBlueChannel;
   bool _writeGreenChannel;
   bool _writeAlphaChannel;

   // Blending
   bool _blendDefined;
   bool _blendEnable;
   BlendProperty _blendSrc;
   BlendProperty _blendDest;
   BlendOperation _blendOp;

    /// Rasterizer
   bool _cullDefined;
   CullMode _cullMode;

   /// Depth
   bool _zDefined;
   bool _zEnable;
   bool _zWriteEnable;

   /// Stencil
   bool _stencilDefined;
   bool _stencilEnable;
   StencilOperation _stencilFailOp;
   StencilOperation _stencilZFailOp;
   StencilOperation _stencilPassOp;
   ComparisonFunction  _stencilFunc;
   U32 _stencilRef;
   U32 _stencilMask;
   U32 _stencilWriteMask;

   FillMode   _fillMode;

public:
   ComparisonFunction _zFunc;
   F32 _zBias;
   F32 _zUnits;

   RenderStateBlockDescriptor();

   void fromDescriptor( const RenderStateBlockDescriptor& descriptor );

   inline void setFillMode(FillMode mode)      { _fillMode = mode;  }

   void setCullMode(CullMode mode );
   void setZEnable(bool enable);
   void setZReadWrite(bool read, bool write = true);

   void setBlend( bool enable,
                  BlendProperty src = BLEND_PROPERTY_SRC_ALPHA,
                  BlendProperty dest = BLEND_PROPERTY_INV_SRC_ALPHA,
                  BlendOperation op = BLEND_OPERATION_ADD );

   void setColorWrites( bool red, bool green, bool blue, bool alpha );
};

class RenderStateBlock{
public:
   virtual ~RenderStateBlock() { }

   virtual I64 getGUID() const = 0;

   virtual RenderStateBlockDescriptor& getDescriptor() = 0;

   bool operator == (RenderStateBlock& RSB){return Compare(RSB);}
   bool operator != (RenderStateBlock& RSB){return !Compare(RSB);}
   inline bool Compare(const RenderStateBlock& RSB) const {return getGUID() == RSB.getGUID();}
};

#endif