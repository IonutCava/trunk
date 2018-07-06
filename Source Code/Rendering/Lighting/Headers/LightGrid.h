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
* This class contains the representation, and construction of the light grid
* used for
* tiled deferred shading.
* The grid is stored in statically sized arrays, the size is determined by
* LIGHT_GRID_MAX_DIM_X and LIGHT_GRID_MAX_DIM_Y
* (see Config.h).
*/

namespace Divide {

class LightGrid {
   public:
    struct LightInternal {
       public:
        I64 guid;
        vec3<F32> position;
        vec3<F32> color;
        F32 range;
    };

    inline static LightInternal makeLight(I64 guid,
                                          const vec3<F32>& position,
                                          const vec3<F32>& color,
                                          F32 range) {
        LightInternal l = {guid, position, color, range};
        return l;
    }

    inline static LightInternal makeLight(const vec3<F32>& position,
                                          const LightInternal& l) {
        return makeLight(l.guid, position, l.color, l.range);
    }

    typedef vectorImpl<LightInternal> Lights;

    struct ScreenRect {
        vec2<U16> min;
        vec2<U16> max;
    };

    typedef vectorImpl<ScreenRect> ScreenRects;

    LightGrid() {}

    /**
    * Call to build the grid structure.
    * The tile size and (screen) resolution must combine to be less than the
    *maximum defined byt
    * LIGHT_GRID_MAX_DIM_X and LIGHT_GRID_MAX_DIM_Y.
    *
    * The 'gridMinMaxZ' pointer is optional, if provided, the min and max range
    *is used to prune lights.
    * The values are expected to be in view space.
    * It must have the dimensions LIGHT_GRID_MAX_DIM_X x LIGHT_GRID_MAX_DIM_Y.
    */
    void build(const vec2<U16>& tileSize, const vec2<U16>& resolution,
               const Lights& light, const mat4<F32>& modelView,
               const mat4<F32>& projection, F32 near,
               const vectorImpl<vec2<F32> >& gridMinMaxZ);

    void prune(const vectorImpl<vec2<F32> >& gridMinMaxZ);

    void pruneFarOnly(F32 aNear, const vectorImpl<vec2<F32> >& aGridMinMaxZ);

    /**
    * Access to grid wide properties, set at last call to 'build'.
    */
    const vec2<U16>& getTileSize() const { return _tileSize; }

    const vec2<U16>& getGridDim() const { return _gridDim; }

    U32 getMaxTileLightCount() const { return _maxTileLightCount; }

    U32 getTotalTileLightIndexListLength() const {
        return U32(_tileLightIndexLists.size());
    }

    /**
    * tile accessor functions, returns data for an individual tile.
    */
    I32 tileLightCount(U32 x, U32 y) const {
        return _gridCounts[x + y * Config::Lighting::LIGHT_GRID_MAX_DIM_X];
    }

    const I32* tileLightIndexList(U32 x, U32 y) const {
        return &_tileLightIndexLists
            [_gridOffsets[x + y * Config::Lighting::LIGHT_GRID_MAX_DIM_X]];
    }

    const vec2<F32> getTileMinMax(U32 x, U32 y) const {
        return _minMaxGridValid ? _gridMinMaxZ[x + y * _gridDim.x]
                                 : vec2<F32>(0.0f, 0.0f);
    }

    /**
    * Grid data pointer accessors, used for uploading to GPU.
    */
    const I32* tileDataPtr() const { return _gridOffsets.data(); }

    const I32* tileCountsDataPtr() const { return _gridCounts.data(); }

    const I32* tileLightIndexListsPtr() const {
        return &_tileLightIndexLists[0];
    }
    /**
    * Returns the list of lights, transformed to view space, that are present in
    * the grid,
    * i.e. produces an on screen rectangle. The light indices in the tile light
    * lists index
    * into this list.
    */
    const Lights& getViewSpaceLights() const { return _viewSpaceLights; }
    /**
    * Indicated whether there are any lights visible, i.e. with on-screen rects.
    */
    bool empty() const { return _screenRects.empty(); }
    /**
    * if true, then the contents of the min/max grid is valid,
    * i.e. the pointer was not 0 last time build was called.
    */
    bool minMaxGridValid() const { return _minMaxGridValid; }

    static ScreenRect findScreenSpaceBounds(const mat4<F32>& projection,
                                            const vec3<F32>& pt, F32 rad,
                                            I32 width, I32 height,
                                            F32 nearPlane);

   protected:
    /**
    * Builds rects list (m_screenRects) and view space lights list
    * (m_viewSpaceLights).
    */
    void buildRects(const vec2<U16>& resolution, const Lights& lights,
                    const mat4<F32>& modelView, const mat4<F32>& projection,
                    F32 nearPlane);

   protected:
    vec2<U16> _tileSize;
    vec2<U16> _gridDim;

    vectorImpl<vec2<F32> > _gridMinMaxZ;
    std::array<I32, Config::Lighting::LIGHT_GRID_MAX_DIM_X *
                        Config::Lighting::LIGHT_GRID_MAX_DIM_Y> _gridOffsets;
    std::array<I32, Config::Lighting::LIGHT_GRID_MAX_DIM_X *
                        Config::Lighting::LIGHT_GRID_MAX_DIM_Y> _gridCounts;
    vectorImpl<I32> _tileLightIndexLists;
    U32 _maxTileLightCount;
    bool _minMaxGridValid;

    Lights _viewSpaceLights;
    ScreenRects _screenRects;
};

};  // namespace Divide

#endif  // _LightGrid_h_
