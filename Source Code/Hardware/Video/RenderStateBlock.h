/*“Copyright 2009-2012 DIVIDE-Studio”*/
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
#include "Hardware/Platform/PlatformDefines.h"

struct RenderStateBlockDescriptor {
   // Blending   
   bool _blendDefined;
   bool _blendEnable;
   BLEND_PROPERTY _blendSrc;
   BLEND_PROPERTY _blendDest;
   BLEND_OPERATION _blendOp;

   /// Separate Alpha Blending
   bool _alphaBlendDefined;
   bool _alphaBlendEnable;
   BLEND_PROPERTY _alphaBlendSrc;
   BLEND_PROPERTY _alphaBlendDest;
   BLEND_OPERATION _alphaBlendOp;

   /// Alpha test
   bool _alphaDefined;
   bool _alphaTestEnable;   
   I32  _alphaTestRef;
   COMPARE_FUNC _alphaTestFunc;

   /// Color Writes
   bool _enableColorWrite;
   bool _writeRedChannel;
   bool _writeBlueChannel;
   bool _writeGreenChannel;
   bool _writeAlphaChannel;

   /// Rasterizer
   bool _cullDefined;
   CULL_MODE _cullMode;

   /// Depth
   bool _zDefined;
   bool _zEnable;
   bool _zWriteEnable;
   COMPARE_FUNC _zFunc;
   F32 _zBias;
   F32 _zSlopeBias;

   /// Stencil
   bool _stencilDefined;
   bool _stencilEnable;
   STENCIL_OPERATION _stencilFailOp;
   STENCIL_OPERATION _stencilZFailOp;
   STENCIL_OPERATION _stencilPassOp;
   COMPARE_FUNC  _stencilFunc;
   U32 _stencilRef;
   U32 _stencilMask;
   U32 _stencilWriteMask;

   /// fixed pipeline lighting
   bool _fixedLighting;

   /// Color material?
   bool _vertexColorEnable;

   FILL_MODE _fillMode;

   ///Cached hash value;
   U32 _hash;

   RenderStateBlockDescriptor();

   U32  getHash() const;

   void fromDescriptor( const RenderStateBlockDescriptor& descriptor );

   void setCullMode(CULL_MODE mode ); 
   inline void setFillMode(FILL_MODE mode) { _fillMode = mode; }

   void setZEnable(const bool enable); 
   void setZReadWrite(bool read, bool write = true); 

   void setAlphaTest(   bool enable, 
                        COMPARE_FUNC func = COMPARE_FUNC_GreaterEqual, 
                        I32 alphaRef = 0 );

   void setBlend( bool enable, 
                  BLEND_PROPERTY src = BLEND_PROPERTY_SrcAlpha, 
                  BLEND_PROPERTY dest = BLEND_PROPERTY_InvSrcAlpha,
                  BLEND_OPERATION op = BLEND_OPERATION_Add );

   void setAlphaBlend( bool enable, 
                        BLEND_PROPERTY src = BLEND_PROPERTY_One, 
                        BLEND_PROPERTY dest = BLEND_PROPERTY_Zero,
                        BLEND_OPERATION op = BLEND_OPERATION_Add );

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