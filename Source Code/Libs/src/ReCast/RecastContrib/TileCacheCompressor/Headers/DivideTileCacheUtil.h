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
#include "DetourTileCache/Include/DetourTileCacheBuilder.h"

/**
  * Allows a custom memory allocation technique to be implemented
  * for storing compressed tiles. This implementation does a linear
  * memory allocation.
  **/
struct LinearAllocator : public dtTileCacheAlloc
{
    unsigned char* buffer;
    int capacity;
    int top;
    int high;

    LinearAllocator(const int cap) : buffer(0), capacity(0), top(0), high(0)
    {
        resize(cap);
    }

    ~LinearAllocator()
    {
        dtFree(buffer);
    }

    void resize(const int cap)
    {
        if (buffer) dtFree(buffer);
        buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
        capacity = cap;
    }

    virtual void reset()
    {
        high = dtMax(high, top);
        top = 0;
    }

    virtual void* alloc(const int size)
    {
        if (!buffer)
            return 0;
        if (top+size > capacity)
            return 0;
        unsigned char* mem = &buffer[top];
        top += size;
        return mem;
    }

    virtual void free(void* /*ptr*/)
    {
        // Empty
    }
};

/**
  * Struct to store a request to add or remove
  * a convex obstacle to the tilecache as a deferred
  * command (currently unused).
  **/
struct DivideTileCacheConvexObstacle
{
    ConvexVolume *obstacle;
    dtCompressedTileRef touched[DT_MAX_TOUCHED_TILES];
    dtCompressedTileRef pending[DT_MAX_TOUCHED_TILES];
    unsigned short salt;
    unsigned char state;
    unsigned char ntouched;
    unsigned char npending;
    DivideTileCacheConvexObstacle* next;    // Single linked list
};

#endif