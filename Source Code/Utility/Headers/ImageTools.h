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
#ifndef _UTILITY_IMAGETOOLS_H
#define _UTILITY_IMAGETOOLS_H

#include "Core/Math/Headers/MathVectors.h"
#include <mutex>

namespace Divide {
enum class GFXImageFormat : U8;
enum class GFXDataFormat : U8;
enum class TextureType : U8;

namespace ImageTools {

class ImageLayer {
   public:
    ImageLayer() noexcept : _size(0)
    {
        _dimensions.set(0, 0, 1);
    }

    inline void writeData(U8* data, U32 len) {
        _data.assign(data, data + len);
    }

    inline void writeData(U16* data, U32 len) {
        _data16.assign(data, data + len);
    }

    inline void writeData(F32* data, U32 len) {
        _dataf.assign(data, data + len);
    }
    /// the image data as it was read from the file / memory.
    size_t _size;
    vector<U8> _data;
    vector<U16> _data16;
    vector<F32> _dataf;
    /// with and height
    vec3<U16> _dimensions;
};

class ImageData : private NonCopyable {
   public:
     ImageData() noexcept;
    ~ImageData();

    /// image origin information
    inline void flip(bool state) { _flip = state; }
    inline bool flip() const { return _flip; }

    inline void set16Bit(bool state) { _16Bit = state; }
    inline bool is16Bit() const { return _16Bit; }

    inline bool isHDR() const { return _isHDR; }
    /// set and get the image's actual data 
    inline const bufferPtr data(U32 mipLevel = 0) const { 
        bufferPtr data = nullptr;
        if (mipLevel < mipCount()) {
            // triple data-ception
            data = (bufferPtr)_data[mipLevel]._data.data();
        }

        return data;
    }
    inline const bufferPtr data16(U32 mipLevel = 0) const {
        bufferPtr data = nullptr;
        if (mipLevel < mipCount()) {
            // triple data-ception
            data = (bufferPtr)_data[mipLevel]._data16.data();
        }

        return data;
    }

    inline const bufferPtr dataf(U32 mipLevel = 0) const {
        bufferPtr data = nullptr;
        if (mipLevel < mipCount()) {
            // triple data-ception
            data = (bufferPtr)_data[mipLevel]._dataf.data();
        }

        return data;
    }

    inline const vector<ImageLayer>& imageLayers() const {
        return _data;
    }
    /// image width, height and depth
    inline const vec3<U16>& dimensions(U32 mipLevel = 0) const { 
        assert(mipLevel < mipCount());

        return _data[mipLevel]._dimensions;
    }
    /// set and get the image's compression state
    inline bool compressed() const { return _compressed; }
    /// get the number of pre-loaded mip maps
    inline U32 mipCount() const { return to_U32(_data.size()); }
    /// image transparency information
    inline bool alpha() const { return _alpha; }
    /// image depth information
    inline U8 bpp() const { return _bpp; }
    /// the filename from which the image is created
    inline const stringImpl& name() const { return _name; }
    /// the image format as given by STB/NV_DDS
    inline GFXImageFormat format() const { return _format; }

    inline GFXDataFormat dataType() const { return _dataType; }
    /// get the texel colour at the specified offset from the origin
    UColour getColour(I32 x, I32 y, U32 mipLevel = 0) const;
    void getColour(I32 x, I32 y, U8& r, U8& g, U8& b, U8& a, U32 mipLevel = 0) const;

    void getRed(I32 x, I32 y, U8& r, U32 mipLevel = 0) const;
    void getGreen(I32 x, I32 y, U8& g, U32 mipLevel = 0) const;
    void getBlue(I32 x, I32 y, U8& b, U32 mipLevel = 0) const;
    void getAlpha(I32 x, I32 y, U8& a, U32 mipLevel = 0) const;

    inline TextureType compressedTextureType() const {
        return _compressedTextureType;
    }

  protected:
    friend class ImageDataInterface;
    /// creates this image instance from the specified data
    bool create(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& fileName);
    bool loadDDS_IL(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename);
    bool loadDDS_NV(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename);

   private:
    //Each entry is a separate mip map.
    vector<ImageLayer> _data;
    vector<U8> _decompressedData;
    /// is the image stored as a regular image or in a compressed format? (eg. DXT1 / DXT3 / DXT5)
    bool _compressed;
    /// should we flip the image's origin on load?
    bool _flip;
    /// 16bit data
    bool _16Bit;
    /// HDR data
    bool _isHDR;
    /// does the image have transparency?
    bool _alpha;
    /// the image format
    GFXImageFormat _format;
    /// the image date type
    GFXDataFormat _dataType;
    /// used by compressed images to load 2D/3D/cubemap textures etc
    TextureType _compressedTextureType;
    /// the actual image filename
    stringImpl _name;
    /// image's bits per pixel
    U8 _bpp;
};

class ImageDataInterface {
public:
    //refWidth/Height = if not 0, we will attempt to resize the texture to the specified dimensions
    static void CreateImageData(const stringImpl& filename, U16 refWidth, U16 refHeight, bool srgb, ImageData& imgOut);
protected:
    friend class ImageData;
    /// used to lock image loader in a sequential operating mode in a multithreaded environment
    static std::mutex _loadingMutex;
};

/// save a single file to TGA
I8 SaveToTGA(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth, U8* imageData) noexcept;
/// save a single file to tga using a sequential naming pattern
I8 SaveSeries(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth, U8* imageData);

};  // namespace ImageTools
};  // namespace Divide

#endif
