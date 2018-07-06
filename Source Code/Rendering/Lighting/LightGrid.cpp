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
#include "Headers/LightGrid.h"
#include "Rendering/Headers/ClipRegion.h"
#include "Utility/Headers/Colors.h"

namespace Divide {

LightGrid::ScreenRect LightGrid::findScreenSpaceBounds(const mat4<F32>& projection,
                                                       const vec3<F32>& pt,
                                                       F32 rad, 
                                                       I32 width, 
                                                       I32 height, 
                                                       F32 nearPlane) {
    vec4<F32> reg = computeClipRegion(pt, rad, nearPlane, projection);
    reg = -reg;

    std::swap(reg.x, reg.z);
    std::swap(reg.y, reg.w);
    reg *= 0.5f;
    reg += 0.5f;

    CLAMP<vec4<F32> >(reg, DefaultColors::BLACK(), DefaultColors::WHITE());

    LightGrid::ScreenRect result;
    result.min.x = I32(reg.x * F32(width));
    result.min.y = I32(reg.y * F32(height));
    result.max.x = I32(reg.z * F32(width));
    result.max.y = I32(reg.w * F32(height));

    assert(result.max.x <= U32(width));
    assert(result.max.y <= U32(height));

    return result;
}

inline bool testDepthBounds(const vec2<F32>& zRange, const LightGrid::LightInternal& light) {
    // Note that since in view space greater depth means _smaller_ z value (i.e. larger _negative_ Z values), 
    // it all gets turned inside out. 
    // Fairly easy to get confused...
    F32 lightMin = light.position.z + light.range;
    F32 lightMax = light.position.z - light.range;

    return (zRange.y < lightMin && zRange.x > lightMax);
}

void LightGrid::build(const vec2<U16>& tileSize, const vec2<U16>& resolution, const Lights& lights,
                      const mat4<F32>& modelView, const mat4<F32>& projection, F32 nearPlane, 
                      const vectorImpl<vec2<F32> >& gridMinMaxZ) {

    m_gridMinMaxZ = gridMinMaxZ;
    m_minMaxGridValid = !gridMinMaxZ.empty();

    const vec2<F32> *gridMinMaxZPtr = m_minMaxGridValid ? &m_gridMinMaxZ[0] : nullptr;

    m_tileSize = tileSize;
    m_gridDim.set((resolution.x + tileSize.x - 1) / tileSize.x, (resolution.y + tileSize.y - 1) / tileSize.y);
    m_maxTileLightCount = 0;

    buildRects(resolution, lights, modelView, projection, nearPlane);

    memset(m_gridOffsets, 0, sizeof(m_gridOffsets));
    memset(m_gridCounts, 0, sizeof(m_gridCounts));

    #define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])
    #define GRID_COUNTS(_x_,_y_)  (m_gridCounts[ _x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])

    I32 totalus = 0;
    {
        for (vectorAlg::vecSize i = 0; i < m_screenRects.size(); ++i) {
            ScreenRect r = m_screenRects[i];
            LightInternal light = m_viewSpaceLights[i];

            vec2<U16> temp(r.max + tileSize - vec2<U16>(1));
            vec2<U16> l(r.min.x / tileSize.x, r.min.y / tileSize.y);
            vec2<U16> u(temp.x / tileSize.x, temp.y / tileSize.y);

            CLAMP<vec2<U16> >(l, vec2<U16>(0, 0), m_gridDim + vec2<U16>(1));
            CLAMP<vec2<U16> >(u, vec2<U16>(0, 0), m_gridDim + vec2<U16>(1));

            for (U32 y = l.y; y < u.y; ++y) {
                for (U32 x = l.x; x < u.x; ++x) {
                    if (!m_minMaxGridValid || testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], light)) {
                        GRID_COUNTS(x, y) += 1;
                        ++totalus;
                    }
                }
            }
        }
    }
    m_tileLightIndexLists.resize(totalus);
#ifdef _DEBUG
    if (!m_tileLightIndexLists.empty())
    {
        memset(&m_tileLightIndexLists[0], 0, m_tileLightIndexLists.size() * sizeof(m_tileLightIndexLists[0]));
    }
#endif // _DEBUG
    U32 offset = 0;
    {
        for (U32 y = 0; y < m_gridDim.y; ++y) {
            for (U32 x = 0; x < m_gridDim.x; ++x) {
                U32 count = GRID_COUNTS(x, y);
                // set offset to be just past end, then decrement while filling in
                GRID_OFFSETS(x, y) = offset + count;
                offset += count;

                // for debug/profiling etc.
                m_maxTileLightCount = std::max(m_maxTileLightCount, count);
            }
        }
    }
    if (m_screenRects.size() && !m_tileLightIndexLists.empty()) {
        I32 *data = &m_tileLightIndexLists[0];
        for (vectorAlg::vecSize i = 0; i < m_screenRects.size(); ++i) {
            U32 lightId = U32(i);

            LightInternal light = m_viewSpaceLights[i];
            ScreenRect r = m_screenRects[i];

            vec2<U16> temp(r.max + tileSize - vec2<U16>(1));
            vec2<U16> l(r.min.x / tileSize.x, r.min.y / tileSize.y);
            vec2<U16> u(temp.x / tileSize.x, temp.y / tileSize.y);

            CLAMP<vec2<U16> >(l, vec2<U16>(0, 0), m_gridDim + vec2<U16>(1));
            CLAMP<vec2<U16> >(u, vec2<U16>(0, 0), m_gridDim + vec2<U16>(1));

            for (U32 y = l.y; y < u.y; ++y) {
                for (U32 x = l.x; x < u.x; ++x) {
                    if (!m_minMaxGridValid || testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], light)) {
                        // store reversely into next free slot
                        offset = GRID_OFFSETS(x, y) - 1;
                        data[offset] = lightId;
                        GRID_OFFSETS(x, y) = offset;
                    }
                }
            }
        }
    }
    #undef GRID_COUNTS
    #undef GRID_OFFSETS
}

void LightGrid::prune(const vectorImpl<vec2<F32> >& gridMinMaxZ) {
    m_gridMinMaxZ = gridMinMaxZ;
    m_minMaxGridValid = !gridMinMaxZ.empty();

    if (!m_minMaxGridValid || m_tileLightIndexLists.empty())
        return;

    const vec2<F32> *gridMinMaxZPtr = m_minMaxGridValid ? &m_gridMinMaxZ[0] : nullptr;
    I32 *lightInds = &m_tileLightIndexLists[0];

    #define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])
    #define GRID_COUNTS(_x_,_y_)  (m_gridCounts[ _x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])

    I32 totalus = 0;
    m_maxTileLightCount = 0;
    for (U32 y = 0; y < m_gridDim.y; ++y) {
        for (U32 x = 0; x < m_gridDim.x; ++x) {
            U32 count = GRID_COUNTS(x, y);
            U32 offset = GRID_OFFSETS(x, y);

            for (U32 i = 0; i < count; ++i) {
                const LightInternal &l = m_viewSpaceLights[lightInds[offset + i]];
                if (!testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], l)) {
                    std::swap(lightInds[offset + i], lightInds[offset + count - 1]);
                    --count;
                }
            }
            totalus += count;
            GRID_COUNTS(x, y) = count;
            m_maxTileLightCount = std::max(m_maxTileLightCount, count);
        }
    }
    #undef GRID_COUNTS
    #undef GRID_OFFSETS
}

void LightGrid::pruneFarOnly(F32 aNear, const vectorImpl<vec2<F32> >& gridMinMaxZ) {

    m_gridMinMaxZ = gridMinMaxZ;
    m_minMaxGridValid = !gridMinMaxZ.empty();

    if (!m_minMaxGridValid || m_tileLightIndexLists.empty())
        return;

    for (vectorImpl<vec2<F32> >::iterator it = m_gridMinMaxZ.begin(); it != m_gridMinMaxZ.end(); ++it)
        it->x = -aNear;

    const vec2<F32> *gridMinMaxZPtr = m_minMaxGridValid ? &m_gridMinMaxZ[0] : nullptr;
    I32 *lightInds = &m_tileLightIndexLists[0];

    #define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])
    #define GRID_COUNTS(_x_,_y_)  (m_gridCounts[ _x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])

    I32 totalus = 0;
    m_maxTileLightCount = 0;
    for (U32 y = 0; y < m_gridDim.y; ++y) {
        for (U32 x = 0; x < m_gridDim.x; ++x) {
            U32 count = GRID_COUNTS(x, y);
            U32 offset = GRID_OFFSETS(x, y);

            for (U32 i = 0; i < count; ++i) {
                const LightInternal &l = m_viewSpaceLights[lightInds[offset + i]];
                if (!testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], l)) {
                    std::swap(lightInds[offset + i], lightInds[offset + count - 1]);
                    --count;
                }
            }
            totalus += count;
            GRID_COUNTS(x, y) = count;
            m_maxTileLightCount = std::max(m_maxTileLightCount, count);
        }
    }
    #undef GRID_COUNTS
    #undef GRID_OFFSETS
}

void LightGrid::buildRects(const vec2<U16>& resolution, 
                           const Lights& lights, 
                           const mat4<F32>& modelView,
                           const mat4<F32>& projection,
                           F32 nearPlane) {

    m_viewSpaceLights.clear();
    m_viewSpaceLights.reserve(lights.size());

    m_screenRects.clear();
    m_screenRects.reserve(lights.size());

    for(U32 i = 0; i < lights.size(); ++i) {
        const LightInternal &l = lights[i];
        vec3<F32> vp = modelView.transform(l.position);
        ScreenRect rect = findScreenSpaceBounds(projection, vp, l.range, resolution.x, resolution.y, nearPlane);

        if (rect.min.x < rect.max.x && rect.min.y < rect.max.y)
        {
            m_screenRects.push_back(rect);
            // save light in model space
            m_viewSpaceLights.push_back(makeLight(vp, l));
        }
    }
}

}; //namespace Divide