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

   /// Separate Alpha Blending
   bool _alphaBlendDefined;
   bool _alphaBlendEnable;
   BlendProperty _alphaBlendSrc;
   BlendProperty _alphaBlendDest;
   BlendOperation _alphaBlendOp;

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

   /// Color material?
   bool _vertexColorEnable;

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

   void setAlphaBlend( bool enable,
                        BlendProperty src = BLEND_PROPERTY_ONE,
                        BlendProperty dest = BLEND_PROPERTY_ZERO,
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