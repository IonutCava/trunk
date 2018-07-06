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
#include "Hardware/Platform/Headers/PlatformDefines.h"

struct RenderStateBlockDescriptor {
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

   /// Alpha test
   bool _alphaDefined;
   bool _alphaTestEnable;   
   I32  _alphaTestRef;
   ComparisonFunction _alphaTestFunc;

   /// Color Writes
   bool _enableColorWrite;
   bool _writeRedChannel;
   bool _writeBlueChannel;
   bool _writeGreenChannel;
   bool _writeAlphaChannel;

   /// Rasterizer
   bool _cullDefined;
   CullMode _cullMode;

   /// Depth
   bool _zDefined;
   bool _zEnable;
   bool _zWriteEnable;
   ComparisonFunction _zFunc;
   F32 _zBias;
   F32 _zUnits;

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

   /// fixed pipeline lighting
   bool _fixedLighting;

   /// Color material?
   bool _vertexColorEnable;

   FillMode _fillMode;

   ///Cached hash value;
   U32 _hash;

   RenderStateBlockDescriptor();

   U32  getHash() const;

   void fromDescriptor( const RenderStateBlockDescriptor& descriptor );

   void setCullMode(CullMode mode ); 
   inline void setFillMode(FillMode mode) { _fillMode = mode; }

   void setZEnable(const bool enable); 
   void setZReadWrite(bool read, bool write = true); 

   void setAlphaTest(   bool enable, 
                        ComparisonFunction func = CMP_FUNC_GEQUAL, 
                        I32 alphaRef = 0 );

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

   virtual U32 getHash() const = 0;

   virtual const RenderStateBlockDescriptor& getDescriptor() = 0;

};

#define SET_STATE_BLOCK(X) GFX_DEVICE.setStateBlock(X)
#define SET_DEFAULT_STATE_BLOCK() GFX_DEVICE.setDefaultStateBlock()
#endif