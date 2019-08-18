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

#include "stdafx.h"
#include "Headers/TileRing.h"

#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

    struct Adjacency
    {
        // These are the size of the neighbours along +/- x or y axes.  For interior tiles
        // this is 1.  For edge tiles it is 0.5 or 2.0.
        F32 neighbourMinusX;
        F32 neighbourMinusY;
        F32 neighbourPlusX;
        F32 neighbourPlusY;
    };

    struct InstanceData
    {
        F32 x, y;
        Adjacency adjacency;
    };

    TileRing::TileRing(GFXDevice& device, I32 holeWidth, I32 outerWidth, F32 tileSize) :
        _holeWidth(holeWidth),
        _outerWidth(outerWidth),
        _ringWidth((outerWidth - holeWidth) / 2),   // No remainder - see assert below.
        _nTiles(outerWidth* outerWidth - holeWidth * holeWidth),
        _tileSize(tileSize),
        _pVBData(nullptr)
    {
        _buffer = device.newGVD(1, Util::StringFormat("Terrain Tile Ring [ %d - %d - %5.2f ]", holeWidth, outerWidth, tileSize).c_str());
        assert((outerWidth - holeWidth) % 2 == 0);
        CreateInstanceDataVB();
    }

    TileRing::~TileRing()
    {
        MemoryManager::DELETE_ARRAY(_pVBData);
    }

    void TileRing::CreateInputLayout(const GenericVertexData::IndexBuffer& idxBuff)
    {
        _buffer->setIndexBuffer(idxBuff, BufferUpdateFrequency::ONCE);

        AttributeDescriptor& desc1 = _buffer->attribDescriptor(to_base(AttribLocation::POSITION));
        desc1.set(0,
                  2,
                  GFXDataFormat::FLOAT_32,
                  false,
                  offsetof(InstanceData, x));

        AttributeDescriptor& desc2 = _buffer->attribDescriptor(to_base(AttribLocation::TEXCOORD));
        desc2.set(0,
                  4,
                  GFXDataFormat::FLOAT_32,
                  false,
                  offsetof(InstanceData, adjacency));
    }

    void TileRing::ReleaseInputLayout()
    {
    }

    bool TileRing::InRing(I32 x, I32 y) const
    {
        assert(x >= 0 && x < _outerWidth);
        assert(y >= 0 && y < _outerWidth);
        return (x < _ringWidth || y < _ringWidth || x >= _outerWidth - _ringWidth || y >= _outerWidth - _ringWidth);
    }

    void TileRing::AssignNeighbourSizes(I32 x, I32 y, Adjacency* pAdj) const
    {
        pAdj->neighbourPlusX = 1.0f;
        pAdj->neighbourPlusY = 1.0f;
        pAdj->neighbourMinusX = 1.0f;
        pAdj->neighbourMinusY = 1.0f;

        // TBD: these aren't necessarily 2x different.  Depends on the relative tiles sizes supplied to ring ctors.
        const F32 innerNeighbourSize = 0.5f;
        const F32 outerNeighbourSize = 2.0f;

        // Inner edges abut tiles that are smaller.  (But not on the inner-most.)
        if (_holeWidth > 0)
        {
            if (y >= _ringWidth && y < _outerWidth - _ringWidth)
            {
                if (x == _ringWidth - 1) {
                    pAdj->neighbourPlusX = innerNeighbourSize;
                }
                if (x == _outerWidth - _ringWidth) {
                    pAdj->neighbourMinusX = innerNeighbourSize;
                }
            }
            if (x >= _ringWidth && x < _outerWidth - _ringWidth)
            {
                if (y == _ringWidth - 1) {
                    pAdj->neighbourPlusY = innerNeighbourSize;
                }
                if (y == _outerWidth - _ringWidth) {
                    pAdj->neighbourMinusY = innerNeighbourSize;
                }
            }
        }

        // Outer edges abut tiles that are larger.  We could skip this on the outer-most ring.  But it will
        // make almost zero visual or perf difference.
        if (x == 0) {
            pAdj->neighbourMinusX = outerNeighbourSize;
        }
        if (y == 0) {
            pAdj->neighbourMinusY = outerNeighbourSize;
        }
        if (x == _outerWidth - 1) {
            pAdj->neighbourPlusX = outerNeighbourSize;
        }
        if (y == _outerWidth - 1) {
            pAdj->neighbourPlusY = outerNeighbourSize;
        }
    }

    void TileRing::CreateInstanceDataVB()
    {
        I32 index = 0;
        _pVBData = MemoryManager_NEW InstanceData[_nTiles];

        const F32 halfWidth = 0.5f * (F32)_outerWidth;
        for (I32 y = 0; y < _outerWidth; ++y)
        {
            for (I32 x = 0; x < _outerWidth; ++x)
            {
                if (InRing(x, y))
                {
                    _pVBData[index].x = _tileSize * ((F32)x - halfWidth);
                    _pVBData[index].y = _tileSize * ((F32)y - halfWidth);
                    AssignNeighbourSizes(x, y, &(_pVBData[index].adjacency));
                    index++;
                }
            }
        }
        assert(index == _nTiles);


        GenericVertexData::SetBufferParams params = {};
        params._buffer = 0;
        params._elementCount = _nTiles;
        params._elementSize = sizeof(InstanceData);
        params._useRingBuffer = false;
        params._updateFrequency = BufferUpdateFrequency::ONCE;
        params._sync = false;
        params._data = (bufferPtr)_pVBData;
        params._instanceDivisor = 1;

        _buffer->create(1);
        _buffer->setBuffer(params);
    }
}; //namespace Divide