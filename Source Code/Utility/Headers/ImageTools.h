/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _UTILITY_IMAGETOOLS_H
#define _UTILITY_IMAGETOOLS_H

#include "Core/Math/Headers/MathVectors.h"
#include "Core/Headers/NonCopyable.h"
#include <mutex>

namespace Divide {
enum class GFXImageFormat : U32;
namespace ImageTools {

class ImageData : private NonCopyable {
   public:
     ImageData();
    ~ImageData();

    /// image origin information
    inline void flip(bool state) { _flip = state; }
    inline bool flip() const { return _flip; }
    /// set and get the image's actual data
    inline const bufferPtr data() const { return (bufferPtr)_data.data(); }
    /// width * height * bpp
    inline const U32 imageSize() const { return _imageSize; }
    /// set and get the image's compression state
    inline bool compressed() const { return _compressed; }
    /// image transparency information
    inline bool alpha() const { return _alpha; }
    /// image depth information
    inline U8 bpp() const { return _bpp; }
    /// image width and height
    inline const vec2<U16>& dimensions() const { return _dimensions; }
    /// the filename from which the image is created
    inline const stringImpl& name() const { return _name; }
    /// the image format as given by DevIL
    inline GFXImageFormat format() const { return _format; }
    /// get the texel color at the specified offset from the origin
    vec4<U8> getColor(I32 x, I32 y) const;
    void getColor(I32 x, I32 y, U8& r, U8& g, U8& b, U8& a) const;

  protected:
    friend class ImageDataInterface;
    /// creates this image instance from the specified data
    bool create(const stringImpl& fileName);

   private:
    /// outputs a generic error and sets DevIL's image handle back to 0 so it
    /// can be reused on the next "create" call
    void throwLoadError(const stringImpl& fileName);

   private:
    /// the image data as it was read from the file / memory
    //U8* _data;
    vectorImpl<U8> _data;
    /// is the image stored as a regular image or in a compressed format? (eg.
    /// DXT1 / DXT3 / DXT5)
    bool _compressed;
    /// should we flip the image's origin on load?
    bool _flip;
    /// does the image have transparency?
    bool _alpha;
    /// with and height
    vec2<U16> _dimensions;
    /// image's bits per pixel
    U8 _bpp;
    /// number of data elements (w * h * bpp)
    U32 _imageSize;
    /// the image format
    GFXImageFormat _format;
    /// the actual image filename
    stringImpl _name;
};

class ImageDataInterface {
public:
    static void CreateImageData(const stringImpl& filename, ImageData& imgOut);
private:
    /// used to lock DevIL in a sequential operating mode in a multithreaded
    /// environment
    static std::mutex _loadingMutex;
};

/// save a single file to TGA
I8 SaveToTGA(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth,
    U8* imageData);
/// save a single file to tga using a sequential naming pattern
I8 SaveSeries(const stringImpl& filename, const vec2<U16>& dimensions, U8 pixelDepth,
    U8* imageData);

};  // namespace ImageTools
};  // namespace Divide

#endif
