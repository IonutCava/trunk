/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _DIVIDE_TILE_CACHE_COMPRESSOR_H_
#define _DIVIDE_TILE_CACHE_COMPRESSOR_H_

namespace Divide {

#include "DetourTileCache/Include/DetourTileCache.h"
#include "DetourTileCache/Include/DetourTileCacheBuilder.h"
#include "Detour/Include/DetourCommon.h"
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

}; //namespace Divide

#endif