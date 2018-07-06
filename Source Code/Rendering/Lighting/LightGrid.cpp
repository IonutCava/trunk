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

LightGrid::ScreenRect LightGrid::findScreenSpaceBounds(
    const mat4<F32>& projection, const vec3<F32>& pt, F32 rad, I32 width,
    I32 height, F32 nearPlane) {
    vec4<F32> reg = computeClipRegion(pt, rad, nearPlane, projection);
    reg = -reg;

    std::swap(reg.x, reg.z);
    std::swap(reg.y, reg.w);
    reg *= 0.5f;
    reg += 0.5f;

    CLAMP<vec4<F32> >(reg, DefaultColors::BLACK(), DefaultColors::WHITE());

    LightGrid::ScreenRect result;
    result.min.x = static_cast<U16>(reg.x * to_float(width));
    result.min.y = static_cast<U16>(reg.y * to_float(height));
    result.max.x = static_cast<U16>(reg.z * to_float(width));
    result.max.y = static_cast<U16>(reg.w * to_float(height));

    assert(result.max.x <= U32(width));
    assert(result.max.y <= U32(height));

    return result;
}

inline bool testDepthBounds(const vec2<F32>& zRange,
                            const LightGrid::LightInternal& light) {
    // Note that since in view space greater depth means _smaller_ z value (i.e.
    // larger _negative_ Z values),
    // it all gets turned inside out.
    // Fairly easy to get confused...
    F32 lightMin = light.position.z + light.range;
    F32 lightMax = light.position.z - light.range;

    return (zRange.y < lightMin && zRange.x > lightMax);
}

void LightGrid::build(const vec2<U16>& tileSize, const vec2<U16>& resolution,
                      const Lights& lights, const mat4<F32>& modelView,
                      const mat4<F32>& projection, F32 nearPlane,
                      const vectorImpl<vec2<F32> >& gridMinMaxZ) {
    _gridMinMaxZ = gridMinMaxZ;
    _minMaxGridValid = !gridMinMaxZ.empty();

    const vec2<F32>* gridMinMaxZPtr =
        _minMaxGridValid ? &_gridMinMaxZ[0] : nullptr;

    _tileSize = tileSize;
    _gridDim.set((resolution.x + tileSize.x - 1) / tileSize.x,
                 (resolution.y + tileSize.y - 1) / tileSize.y);
    _maxTileLightCount = 0;

    buildRects(resolution, lights, modelView, projection, nearPlane);

    _gridOffsets.fill(0);
    _gridCounts.fill(0);

#define GRID_OFFSETS(_x_, _y_) \
    (_gridOffsets[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])
#define GRID_COUNTS(_x_, _y_) \
    (_gridCounts[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])

    I32 totalus = 0;
    {
        for (vectorAlg::vecSize i = 0; i < _screenRects.size(); ++i) {
            ScreenRect r = _screenRects[i];
            LightInternal light = _viewSpaceLights[i];

            vec2<U16> temp(r.max + tileSize - vec2<U16>(1));
            vec2<U16> l(r.min.x / tileSize.x, r.min.y / tileSize.y);
            vec2<U16> u(temp.x / tileSize.x, temp.y / tileSize.y);

            CLAMP<vec2<U16> >(l, vec2<U16>(0, 0), _gridDim + vec2<U16>(1));
            CLAMP<vec2<U16> >(u, vec2<U16>(0, 0), _gridDim + vec2<U16>(1));

            for (U32 y = l.y; y < u.y; ++y) {
                for (U32 x = l.x; x < u.x; ++x) {
                    if (!_minMaxGridValid ||
                        testDepthBounds(gridMinMaxZPtr[y * _gridDim.x + x],
                                        light)) {
                        GRID_COUNTS(x, y) += 1;
                        ++totalus;
                    }
                }
            }
        }
    }
    _tileLightIndexLists.resize(totalus);
#ifdef _DEBUG
    if (!_tileLightIndexLists.empty()) {
        memset(&_tileLightIndexLists[0], 0,
               _tileLightIndexLists.size() * sizeof(_tileLightIndexLists[0]));
    }
#endif  // _DEBUG
    U32 offset = 0;
    {
        for (U32 y = 0; y < _gridDim.y; ++y) {
            for (U32 x = 0; x < _gridDim.x; ++x) {
                U32 count = GRID_COUNTS(x, y);
                // set offset to be just past end, then decrement while filling
                // in
                GRID_OFFSETS(x, y) = offset + count;
                offset += count;

                // for debug/profiling etc.
                _maxTileLightCount = std::max(_maxTileLightCount, count);
            }
        }
    }
    if (_screenRects.size() && !_tileLightIndexLists.empty()) {
        I32* data = &_tileLightIndexLists[0];
        for (vectorAlg::vecSize i = 0; i < _screenRects.size(); ++i) {
            U32 lightID = U32(i);

            LightInternal light = _viewSpaceLights[i];
            ScreenRect r = _screenRects[i];

            vec2<U16> temp(r.max + tileSize - vec2<U16>(1));
            vec2<U16> l(r.min.x / tileSize.x, r.min.y / tileSize.y);
            vec2<U16> u(temp.x / tileSize.x, temp.y / tileSize.y);

            CLAMP<vec2<U16> >(l, vec2<U16>(0, 0), _gridDim + vec2<U16>(1));
            CLAMP<vec2<U16> >(u, vec2<U16>(0, 0), _gridDim + vec2<U16>(1));

            for (U32 y = l.y; y < u.y; ++y) {
                for (U32 x = l.x; x < u.x; ++x) {
                    if (!_minMaxGridValid ||
                        testDepthBounds(gridMinMaxZPtr[y * _gridDim.x + x],
                                        light)) {
                        // store reversely into next free slot
                        offset = GRID_OFFSETS(x, y) - 1;
                        data[offset] = lightID;
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
    _gridMinMaxZ = gridMinMaxZ;
    _minMaxGridValid = !gridMinMaxZ.empty();

    if (!_minMaxGridValid || _tileLightIndexLists.empty()) return;

    const vec2<F32>* gridMinMaxZPtr =
        _minMaxGridValid ? &_gridMinMaxZ[0] : nullptr;
    I32* lightInds = &_tileLightIndexLists[0];

#define GRID_OFFSETS(_x_, _y_) \
    (_gridOffsets[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])
#define GRID_COUNTS(_x_, _y_) \
    (_gridCounts[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])

    I32 totalus = 0;
    _maxTileLightCount = 0;
    for (U32 y = 0; y < _gridDim.y; ++y) {
        for (U32 x = 0; x < _gridDim.x; ++x) {
            U32 count = GRID_COUNTS(x, y);
            U32 offset = GRID_OFFSETS(x, y);

            for (U32 i = 0; i < count; ++i) {
                const LightInternal& l =
                    _viewSpaceLights[lightInds[offset + i]];
                if (!testDepthBounds(gridMinMaxZPtr[y * _gridDim.x + x], l)) {
                    std::swap(lightInds[offset + i],
                              lightInds[offset + count - 1]);
                    --count;
                }
            }
            totalus += count;
            GRID_COUNTS(x, y) = count;
            _maxTileLightCount = std::max(_maxTileLightCount, count);
        }
    }
#undef GRID_COUNTS
#undef GRID_OFFSETS
}

void LightGrid::pruneFarOnly(F32 aNear,
                             const vectorImpl<vec2<F32> >& gridMinMaxZ) {
    _gridMinMaxZ = gridMinMaxZ;
    _minMaxGridValid = !gridMinMaxZ.empty();

    if (!_minMaxGridValid || _tileLightIndexLists.empty()) {
        return;
    }
    for (vectorImpl<vec2<F32> >::iterator it = std::begin(_gridMinMaxZ);
         it != std::end(_gridMinMaxZ); ++it) {
        it->x = -aNear;
    }
    const vec2<F32>* gridMinMaxZPtr =
        _minMaxGridValid ? &_gridMinMaxZ[0] : nullptr;
    I32* lightInds = &_tileLightIndexLists[0];

#define GRID_OFFSETS(_x_, _y_) \
    (_gridOffsets[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])
#define GRID_COUNTS(_x_, _y_) \
    (_gridCounts[_x_ + _y_ * Config::Lighting::LIGHT_GRID_MAX_DIM_X])

    I32 totalus = 0;
    _maxTileLightCount = 0;
    for (U32 y = 0; y < _gridDim.y; ++y) {
        for (U32 x = 0; x < _gridDim.x; ++x) {
            U32 count = GRID_COUNTS(x, y);
            U32 offset = GRID_OFFSETS(x, y);

            for (U32 i = 0; i < count; ++i) {
                const LightInternal& l =
                    _viewSpaceLights[lightInds[offset + i]];
                if (!testDepthBounds(gridMinMaxZPtr[y * _gridDim.x + x], l)) {
                    std::swap(lightInds[offset + i],
                              lightInds[offset + count - 1]);
                    --count;
                }
            }
            totalus += count;
            GRID_COUNTS(x, y) = count;
            _maxTileLightCount = std::max(_maxTileLightCount, count);
        }
    }
#undef GRID_COUNTS
#undef GRID_OFFSETS
}

void LightGrid::buildRects(const vec2<U16>& resolution, const Lights& lights,
                           const mat4<F32>& modelView,
                           const mat4<F32>& projection, F32 nearPlane) {
    _viewSpaceLights.clear();
    _viewSpaceLights.reserve(lights.size());

    _screenRects.clear();
    _screenRects.reserve(lights.size());

    for (U32 i = 0; i < lights.size(); ++i) {
        const LightInternal& l = lights[i];
        vec3<F32> vp = modelView.transform(l.position);
        ScreenRect rect = findScreenSpaceBounds(
            projection, vp, l.range, resolution.x, resolution.y, nearPlane);

        if (rect.min.x < rect.max.x && rect.min.y < rect.max.y) {
            _screenRects.push_back(rect);
            // save light in model space
            _viewSpaceLights.push_back(makeLight(vp, l));
        }
    }
}

};  // namespace Divide