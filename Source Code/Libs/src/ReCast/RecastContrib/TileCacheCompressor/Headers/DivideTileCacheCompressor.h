/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _DIVIDE_TILE_CACHE_COMPRESSOR_H_
#define _DIVIDE_TILE_CACHE_COMPRESSOR_H_

#include "DetourTileCache/Include/DetourTileCache.h"
#include "RecastContrib/fastlz/fastlz.h"

namespace Divide {
/**
  * FastLZ implementation of detour tile cache tile compressor.
  * You can define a custom implementation if you wish to use
  * a different compression algorithm for compressing your
  * detour heightfield tiles.
  * The result of compression runs is the data that detourTileCache
  * stores in memory (or can save out to disk).
  * The compressed heightfield tiles are stored in ram as they allow
  * to quickly generate a navmesh tile, possibly with obstacles added
  * to them, without the need for a full rebuild.
  **/
struct DivideTileCacheCompressor : public dtTileCacheCompressor
{
        virtual int maxCompressedSize(const int bufferSize)
        {
                return (int)(bufferSize* 1.05f);
        }

        virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
                                                          unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
        {
                *compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
                return DT_SUCCESS;
        }

        virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
                                                                unsigned char* buffer, const int maxBufferSize, int* bufferSize)
        {
                *bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
                return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
        }
};

}; //namespace Divide
#endif