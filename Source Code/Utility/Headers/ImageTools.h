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

namespace Divide {
enum class GFXImageFormat : U8;
enum class GFXDataFormat : U8;
enum class TextureType : U8;

namespace ImageTools {

struct LayerData {
    virtual ~LayerData() = default;
    virtual bufferPtr data() const = 0;

    /// the image data as it was read from the file / memory.
    size_t _size = 0u;
    /// with and height
    vec3<U16> _dimensions = { 0, 0, 1 };
};

template<typename T>
struct ImageMip final : LayerData {

    explicit ImageMip(T* data, size_t len, U16 width, U16 height, U16 depth)
        : _data(len, nullptr)
    {
        if (data != nullptr) {
            std::memcpy(_data.data(), data, len * sizeof(T));
        }

        _size = len;
        _dimensions.set(width, height, depth);
    }

    ~ImageMip() = default;

    bufferPtr data() const final { return (bufferPtr)_data.data(); }

protected:
    vectorEASTL<T> _data;
};

struct ImageLayer {
    template<typename T>
    inline T* allocateMip(T* data, size_t len, U16 width, U16 height, U16 depth) {
        assert(_mips.size() < std::numeric_limits<U8>::max() - 1);

        _mips.emplace_back(std::make_unique<ImageMip<T>>(data, len, width, height, depth));
        return (T*)_mips.back()->data();
    }

    template<typename T>
    inline T* allocateMip(size_t len, U16 width, U16 height, U16 depth) {
        return allocateMip<T>(nullptr, len, width, height, depth);
    }

    inline bufferPtr data(U8 mip) const { 
        if (_mips.size() <= mip) {
            return nullptr;
        }

        return _mips[mip]->data();
    }

    inline LayerData* getMip(U8 mip) const noexcept {
        if (mip < _mips.size()) {
            return _mips[mip].get();
        }

        return nullptr;
    }

    inline U8 mipCount() const noexcept {
        return to_U8(_mips.size());
    }

private:
    vectorEASTL<std::unique_ptr<LayerData>> _mips;
};

class ImageData : private NonCopyable {
   public:
     ImageData() noexcept;
    ~ImageData();

    /// image origin information
    inline void flip(bool state) noexcept { _flip = state; }
    inline bool flip() const noexcept { return _flip; }

    inline void set16Bit(bool state) noexcept { _16Bit = state; }
    inline bool is16Bit() const noexcept { return _16Bit; }

    inline bool isHDR() const noexcept { return _isHDR; }

    /// set and get the image's actual data 
    inline const bufferPtr data(U32 layer, U8 mipLevel) const { 
        if (layer < _layers.size() &&  mipLevel < mipCount()) {
            // triple data-ception
            return _layers[layer].data(mipLevel);
        }

        return nullptr;
    }

    inline const vectorEASTL<ImageLayer>& imageLayers() const noexcept {
        return _layers;
    }
    /// image width, height and depth
    inline const vec3<U16>& dimensions(U32 layer, U8 mipLevel = 0u) const { 
        assert(mipLevel < mipCount());
        assert(layer < _layers.size());

        return _layers[layer].getMip(mipLevel)->_dimensions;
    }

    /// set and get the image's compression state
    inline bool compressed() const noexcept { return _compressed; }
    /// get the number of pre-loaded mip maps (same number for each layer)
    inline U8 mipCount() const noexcept { return _layers.empty() ? 0u : _layers.front().mipCount(); }
    /// get the total number of image layers
    inline U32 layerCount() const noexcept { return to_U32(_layers.size()); }
    /// image transparency information
    inline bool alpha() const noexcept { return _alpha; }
    /// image depth information
    inline U8 bpp() const noexcept { return _bpp; }
    /// the filename from which the image is created
    inline const stringImpl& name() const noexcept { return _name; }
    /// the image format as given by STB/NV_DDS
    inline GFXImageFormat format() const noexcept { return _format; }

    inline GFXDataFormat dataType() const noexcept { return _dataType; }
    /// get the texel colour at the specified offset from the origin
    UColour4 getColour(I32 x, I32 y, U32 layer = 0u, U8 mipLevel = 0u) const;
    void getColour(I32 x, I32 y, U8& r, U8& g, U8& b, U8& a, U32 layer = 0u, U8 mipLevel = 0u) const;

    void getRed(I32 x, I32 y, U8& r, U32 layer = 0u, U8 mipLevel = 0u) const;
    void getGreen(I32 x, I32 y, U8& g, U32 layer = 0u, U8 mipLevel = 0u) const;
    void getBlue(I32 x, I32 y, U8& b, U32 layer = 0u, U8 mipLevel = 0u) const;
    void getAlpha(I32 x, I32 y, U8& a, U32 layer = 0u, U8 mipLevel = 0u) const;

    inline TextureType compressedTextureType() const noexcept { return _compressedTextureType; }

    bool addLayer(Byte* data, size_t size, U16 width, U16 height, U16 depth);
    /// creates this image instance from the specified data
    bool addLayer(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& fileName);

  protected:
    friend class ImageDataInterface;
    bool loadDDS_IL(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename);
    bool loadDDS_NV(bool srgb, U16 refWidth, U16 refHeight, const stringImpl& filename);

   private:
    //Each entry is a separate mip map.
    vectorEASTL<ImageLayer> _layers;
    vectorEASTL<U8> _decompressedData;
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
    static bool CreateImageData(const stringImpl& filename, U16 refWidth, U16 refHeight, bool srgb, ImageData& imgOut);
protected:
    friend class ImageData;
    /// used to lock image loader in a sequential operating mode in a multithreaded environment
    static Mutex _loadingMutex;
};

/// save a single file to TGA
I8 SaveToTGA(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth, U8* imageData) noexcept;
/// save a single file to tga using a sequential naming pattern
I8 SaveSeries(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth, U8* imageData);

};  // namespace ImageTools
};  // namespace Divide

#endif
