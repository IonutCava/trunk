// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

//----------------------------------------------------------------------------------
// Defines and draws one ring of tiles in a concentric, nested set of rings.  Each ring
// is a different LOD and uses a different tile/patch size.  These are actually square
// rings.  (Is there a term for that?)  But they could conceivably change to circular.
// The inner-most LOD is a square, represented by a degenerate ring.
//

/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _TERRAIN_TILE_RING_H_
#define _TERRAIN_TILE_RING_H_

#include "Rendering/Camera/Headers/Frustum.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

namespace Divide {
    class GenericVertexData;
    struct InstanceData;
    // Int dimensions specified to the ctor are in numbers of tiles.  It's symmetrical in
    // each direction.  (Don't read much into the exact numbers of #s in this diagram.)
    //
    //    <-   outerWidth  ->
    //    ###################
    //    ###################
    //    ###             ###
    //    ###<-holeWidth->###
    //    ###             ###
    //    ###    (0,0)    ###
    //    ###             ###
    //    ###             ###
    //    ###             ###
    //    ###################
    //    ###################
    //
    class TileRing : private NonCopyable
    {
    public:
        // holeWidth & outerWidth are nos. of tiles. tileSize is a world-space length
        explicit TileRing(GFXDevice& device, I32 holeWidth, I32 outerWidth, F32 tileSize);
        ~TileRing();

        void CreateInputLayout(const GenericVertexData::IndexBuffer& idxBuff);
        void ReleaseInputLayout();

        inline I32 outerWidth() const { return _outerWidth; }
        inline I32 nTiles()     const { return _nTiles; }
        inline F32 tileSize()   const { return _tileSize; }

        inline GenericVertexData* getBuffer() const { return _buffer; }

    private:
        void CreateInstanceDataVB();
        bool InRing(I32 x, I32 y) const;
        void AssignNeighbourSizes(I32 x, I32 y, InstanceData*) const;

        const I32 _holeWidth, _outerWidth, _ringWidth;
        const I32 _nTiles;
        const F32 _tileSize;
        GenericVertexData* _buffer;
    };
}; //namespace Divide
#endif //_TERRAIN_TILE_RING_H_