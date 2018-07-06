/****************************************************************************/
/* Copyright (c) 2011, Ola Olsson
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
/****************************************************************************/
#ifndef _LightGrid_h_
#define _LightGrid_h_

#include "Light.h"

/**
* This class contains the representation, and construction of the light grid used for tiled deferred shading.
* The grid is stored in statically sized arrays, the size is determined by LIGHT_GRID_MAX_DIM_X and LIGHT_GRID_MAX_DIM_Y
* (see Config.h).
*/

class LightGrid
{
public:
    struct LightInternal
    {
    public:
        vec3<F32> position;
        vec3<F32> color;
        F32 range;
    };


    inline static LightInternal make_light(const vec3<F32>& position, const vec3<F32>& color, F32 range)
    {
        LightInternal l = { position, color, range };
        return l;
    }

    inline static LightInternal make_light(const vec3<F32>& position, const LightInternal &l)
    {
        return make_light(position, l.color, l.range);
    }


    typedef vectorImpl<LightInternal> Lights;

    struct ScreenRect {
        vec2<U16> min;
        vec2<U16> max;
    };

    typedef vectorImpl<ScreenRect> ScreenRects;

    LightGrid() {};
    /**
    * Call to build the grid structure.
    * The tile size and (screen) resolution must combine to be less than the maximum defined byt
    * LIGHT_GRID_MAX_DIM_X and LIGHT_GRID_MAX_DIM_Y.
    *
    * The 'gridMinMaxZ' pointer is optional, if provided, the min and max range is used to prune lights.
    * The values are expected to be in view space.
    * It must have the dimensions LIGHT_GRID_MAX_DIM_X x LIGHT_GRID_MAX_DIM_Y.
    */
    void build(const vec2<U16>& tileSize, const vec2<U16>& resolution, const Lights& light,
               const mat4<F32>& modelView, const mat4<F32>& projection, F32 near, const vectorImpl<vec2<F32> >& gridMinMaxZ);

    void prune(const vectorImpl<vec2<F32> >& gridMinMaxZ);
    void pruneFarOnly(F32 aNear, const vectorImpl<vec2<F32> >& aGridMinMaxZ);

    /**
    * Access to grid wide properties, set at last call to 'build'.
    */
    const vec2<U16>& getTileSize() const { return m_tileSize; }
    const vec2<U16>& getGridDim()  const { return m_gridDim; }
    U32 getMaxTileLightCount()     const { return m_maxTileLightCount; }
    U32 getTotalTileLightIndexListLength() const { return U32(m_tileLightIndexLists.size()); }

    /**
    * tile accessor functions, returns data for an individual tile.
    */
    I32 tileLightCount(U32 x, U32 y) const { return m_gridCounts[x + y * Config::Lighting::LIGHT_GRID_MAX_DIM_X]; }
    const I32 *tileLightIndexList(U32 x, U32 y) const { return &m_tileLightIndexLists[m_gridOffsets[x + y * Config::Lighting::LIGHT_GRID_MAX_DIM_X]]; }
    const vec2<F32> getTileMinMax(U32 x, U32 y) const { return m_minMaxGridValid ? m_gridMinMaxZ[x + y * m_gridDim.x] : vec2<F32>(0.0f, 0.0f); }


    /**
    * Grid data pointer accessors, used for uploading to GPU.
    */
    const I32 *tileDataPtr() const { return m_gridOffsets; }
    const I32 *tileCountsDataPtr() const { return m_gridCounts; }
    const I32 *tileLightIndexListsPtr() const { return &m_tileLightIndexLists[0]; }

    /**
    * Returns the list of lights, transformed to view space, that are present in the grid,
    * i.e. produces an on screen rectangle. The light indices in the tile light lists index
    * into this list.
    */
    const Lights& getViewSpaceLights() const { return m_viewSpaceLights; }

    /**
    * Indicated whether there are any lights visible, i.e. with on-screen rects.
    */
    bool empty() const { return m_screenRects.empty(); }
    /**
    * if true, then the contents of the min/max grid is valid,
    * i.e. the pointer was not 0 last time build was called.
    */
    bool minMaxGridValid() const { return m_minMaxGridValid; }

    static ScreenRect findScreenSpaceBounds(const mat4<F32>& projection, const vec3<F32>& pt, F32 rad, I32 width, I32 height, F32 nearPlane);

protected:

    /**
    * Builds rects list (m_screenRects) and view space lights list (m_viewSpaceLights).
    */
    void buildRects(const vec2<U16>& resolution, const Lights& lights, const mat4<F32>& modelView, const mat4<F32>& projection, F32 nearPlane);

    vec2<U16> m_tileSize;
    vec2<U16> m_gridDim;

    vectorImpl<vec2<F32> > m_gridMinMaxZ;
    I32 m_gridOffsets[Config::Lighting::LIGHT_GRID_MAX_DIM_X * Config::Lighting::LIGHT_GRID_MAX_DIM_Y];
    I32 m_gridCounts[Config::Lighting::LIGHT_GRID_MAX_DIM_X * Config::Lighting::LIGHT_GRID_MAX_DIM_Y];
    vectorImpl<I32> m_tileLightIndexLists;
    U32 m_maxTileLightCount;
    bool m_minMaxGridValid;

    Lights m_viewSpaceLights;
    ScreenRects m_screenRects;
};


#endif // _LightGrid_h_
