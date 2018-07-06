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
#ifndef _DIVIDE_TILE_CACHE_COMPRESSOR_H_
#define _DIVIDE_TILE_CACHE_COMPRESSOR_H_

#include "DetourTileCache/Include/DetourTileCache.h"
#include "RecastContrib/fastlz/fastlz.h"

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

#endif