/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _UTILITY_IMAGETOOLS_H
#define _UTILITY_IMAGETOOLS_H

#include "Utility/Headers/String.h"
#include "Core/Math/Headers/MathVectors.h"
#include "Hardware/Platform/Headers/SharedMutex.h"

namespace Divide {
    enum GFXImageFormat;
    namespace ImageTools {

    class ImageData {
        public:
            ImageData()  : _flip(false),
                           _alpha(false),
                           _bpp(0),
                           _ilTexture(0),
                           _compressed(false),
                           _data(nullptr)
            {
                _dimensions.set(0,0);
            }

            ~ImageData()
            {
                destroy();
            }

            /// creates this image instance from the specified data
            bool create(const stringImpl& fileName);
            /// creates this image by reading a portion of memory from sistem RAM defined by a starting position and a size
            bool create(const void* ptr, U32 size);
            /// destroy the image data allocated by the create call
            void destroy();
            /// change a image's width and height
            void resize(U16 width, U16 height);
            /// image origin information
            inline void flip(bool state)               {_flip = state;}
            inline bool flip()                   const {return _flip;}
            /// set and get the image's actual data
            inline const U8* const data()        const {return _data;}
            /// width * height * bpp
            inline const U32       imageSize()   const {return _imageSize;}
            /// set and get the image's compression state
            inline bool compressed()             const {return _compressed;}
            /// image transparency information
            inline bool alpha()                  const {return _alpha;}
            /// image depth information
            inline U8   bpp()                    const {return _bpp;}
            /// image width and height
            inline const vec2<U16>& dimensions() const {return _dimensions;}
            /// the filename from which the image is created
            inline const stringImpl& name()     const {return _name;}
            /// the image handle as given by DevIL
            inline U32  handle()                 const {return _ilTexture;}
            /// the image format as given by DevIL
            inline GFXImageFormat  format()      const {return _format;}
            /// get the texel color at the specified offset from the origin
            vec4<U8> getColor(U16 x, U16 y)   const;

        private:
            /// this is called by either of the create functions to prepare DevIL for loading
            bool prepareInternalData();
            /// this is called by either of the create functions to set the image info (depth, resolution, pixel data, etc)
            bool setInternalData();
            /// outputs a generic error and sets DevIL's image handle back to 0 so it can be reused on the next "create" call
            void throwLoadError(const stringImpl& fileName);

        private:
            /// the image data as it was read from the file / memory
            U8*	 _data;
            /// is the image stored as a regular image or in a compressed format? (eg. DXT1 / DXT3 / DXT5)
            bool _compressed;
            /// should we flip the image's origin on load?
            bool _flip;
            /// does the image have transparency?
            bool _alpha;
            /// with and height
            vec2<U16> _dimensions;
            /// image's bits per pixel
            U8  _bpp;
            /// number of data elements (w * h * bpp)
            U32 _imageSize;
            /// the image format
            GFXImageFormat _format;
            /// the DevIL texture handle
            U32 _ilTexture;
            /// the actual image filename
            stringImpl _name;
    };

    /// prepares the image loading system
    void initialize();
    /// open an image file and save it's contents in the ImageData object
    inline void OpenImage(const stringImpl& filename, ImageData& img) {img.create(filename);}
    /// read an image file from a block of memory and save it's contents in the ImageData object
    inline void OpenImage(const void* ptr, U32 size, ImageData& img)   { img.create(ptr, size); }
    /// save a single file to TGA
    I8   SaveToTGA(char *filename,  const vec2<U16>& dimensions, U8 pixelDepth, U8 *imageData);
    /// save a single file to tga using a sequential naming pattern
    I8   SaveSeries(char *filename, const vec2<U16>& dimensions, U8 pixelDepth, U8 *imageData);

    /// used to lock DevIL in a sequential operating mode in a multithreaded environment
    static SharedLock _loadingMutex;

    }; //namespace ImageTools
}; //namespace Divide

#endif
