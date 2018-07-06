
// 80% of this file is taken from:
// http://www.vitiy.info/manual-decoding-of-ico-file-format-small-c-cross-platform-decoder-lib/
// Original License:
/*
 *
 *	code by Victor Laskin (victor.laskin@gmail.com)
 *  rev 2 - 1bit color was added, fixes for bit mask
 *
 *
 *
 *	THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 *	OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 *	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *	IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *	IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

 // Encoder and 256x256 png decoder/encoder (through stb_image and stb_image_write) added by Flix (same license)

#ifndef TINY_ICO_H
#define TINY_ICO_H

/* MISSING STUFF:
 * Support for loading/saving icons (less than 256x256) with: (width*bitCount)%32!=0 [currently the bitmap stride is ignored in the XOR-Bitmap (both in the decoder and in the encoder): it shouldn't]
 * Support for .cur files
 * Support for icons with a color palette (if they exist)
*/


//#include "./addons/imguibindings/stb_image.h"	// To remove (intellisense)
//#define TINY_ICO_IMPLEMENTATION			// To remove (intellisense)


#ifdef TINY_ICO_STATIC
#define TINYICODEF static
#else
#define TINYICODEF extern
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifndef TINY_ICO_NO_DECODER
// 'comp' always returns 4
// 'icon_index=-1' loads the bigger icon
// returned memory is allocated using STBI_MALLOC(...)
TINYICODEF unsigned char* tiny_ico_load_from_memory(unsigned char const* buffer,int len,int *x,int *y,int *comp,int icon_index=0,int* num_icons=NULL);
#endif

#ifndef TINY_ICO_NO_ENCODER
// 'comp' must be 4, otherwise it returns NULL
// 'buffer' must contain x*y*comp bytes.
// if x>256 or y>256 it returns NULL (no other constraint is applied, but common applications may support only some specific dimensions)
// only one image can be saved
// returned memory is allocated using STBIW_MALLOC(...)
TINYICODEF unsigned char* tiny_ico_save_from_memory(unsigned char const* buffer,int x,int y,int comp,int* returned_buffer_size);
#endif

#ifdef __cplusplus
} //extern "C"
#endif

#ifdef TINY_ICO_IMPLEMENTATION

#pragma pack(push, 1)
// These next two structs represent how the icon information is stored
// in an ICO file.

// 6 bytes
typedef struct	{
    unsigned short  idReserved;		// Reserved
    unsigned short  idType;		// resource type (1 for icons, 2 for cursor)
    unsigned short  idCount;		// how many images?
    //ICONDIRENTRY  idEntries[1];	// the entries for each image
} TINYICONDIR, *LPTINYICONDIR;

// 16 bytes (foreach image,one ICONDIRENTRY after another)
typedef struct	{
    unsigned char   bWidth;		// Width of the image (if 0 is 256)
    unsigned char   bHeight;		// Height of the image (if 0 is 256)
    unsigned char   bColorCount;	// Number of colors in the color palette. Should be 0 if the image does not use a color palette.
    unsigned char   bReserved;		// Reserved
    unsigned short  wPlanes;		// Color Planes for .ico (unused: 0 or 1). For .cur it's HotSpot.x (from left)
    unsigned short  wBitCount;		// Bits per pixel for .ico. For .cur it's HotSpot.y (from the top)
    unsigned int    dwBytesInRes;	// how many bytes in this image?
    unsigned int    dwImageOffset;	// where in the file is this image (from the beginning of the file)
} TINYICONDIRENTRY, *LPTINYICONDIRENTRY;

// Now each image can be a mere .png (usually only for 256x256 images) or a bitmap.
// If it's a bitmap, it has a BITMAPINFOHEADER,
// followed by an XOR-Bitmap (= raw pixels of BITMAPINFOHEADER::biBitCount)
// and by an AND-Bitmap (= 1bpp bitmap that specify transparency)
// Usually the AND-Bitmap is used even if BITMAPINFOHEADER::biBitCount==32 (alpha present in the XOR-Bitmap)

// 40 bytes (Variant of BMP InfoHeader)
typedef struct {
    unsigned int    biSize;		// Size of InfoHeader structure = 40
    unsigned int    biWidth;		// Icon Width
    unsigned int    biHeight;		// Icon Height Doubled (because it's the added height of XOR-Bitmap and AND-Bitmap that follow)
    unsigned short  biPlanes;		// Number of planes = 1
    unsigned short  biBitCount;		// Bits per pixel = 1, 4, 8, 16, 32
    unsigned int    biCompression;	// Type of Compression = 0
    unsigned int    biSizeImage;	// Size of Image in Bytes = 0 (uncompressed)
    unsigned int    biXPelsPerMeter;	// unused = 0 (but I found 2834)
    unsigned int    biYPelsPerMeter;	// unused = 0 (but I found 2834)
    unsigned int    biClrUsed;		// unused = 0
    unsigned int    biClrImportant;	// unused = 0
} TINYICOBITMAPINFOHEADER, *PTINYICOBITMAPINFOHEADER;

// 46 bytes
typedef struct{
   TINYICOBITMAPINFOHEADER icHeader;		// DIB header
   unsigned int	    icColors[1];	// Color table (short 4 bytes) //RGBQUAD
   unsigned char    icXOR[1];		// DIB bits for XOR mask
   unsigned char    icAND[1];		// DIB bits for AND mask
} TINYICONIMAGE, *LPTINYICONIMAGE;

#pragma pack(pop)

#ifndef TINY_ICO_NO_DECODER
#   ifndef STBI_MALLOC
#	error Please include stb_image before this header, when you request TINY_ICO_IMPLEMENTATION
#   endif
#endif

#ifndef TINY_ICO_NO_ENCODER
#   if (!defined(STBIW_MALLOC) || !defined(STBIW_FREE))
#	error Please include stb_image_write before this header, when you request TINY_ICO_IMPLEMENTATION
#   endif
#endif

#ifndef TINY_ICO_ASSERT
#include <assert.h>
#define TINY_ICO_ASSERT(_EXPR)    assert(_EXPR)
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h> // printf [TO REMOVE]
#endif //STBI_NO_STDIO

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TINY_ICO_NO_DECODER
unsigned char* tiny_ico_load_from_memory(unsigned char const* buffer,int len,int *x,int *y,int *comp,int icon_index,int* num_icons)	{
    TINY_ICO_ASSERT(sizeof(unsigned int)==4);
    if (!buffer || len<=0 || !x || !y || !comp) return NULL;
    int& width = *x = 0;
    int& height = *y = 0;
    *comp = 4;

    LPTINYICONDIR icoDir = (LPTINYICONDIR)buffer;

    int iconsCount = icoDir->idCount;
    if (num_icons) *num_icons=iconsCount;

    if (icoDir->idReserved != 0) return NULL;
    if (icoDir->idType != 1) return NULL;   // Otherwise is a cursor
    if (iconsCount == 0) return NULL;
    if (iconsCount > 20) return NULL;

    const unsigned char* cursor = buffer;
    cursor += 6;
    TINYICONDIRENTRY* dirEntry = (TINYICONDIRENTRY*)(cursor);

    unsigned long maxSize = 0;
    unsigned long offset = 0;
    unsigned long size = 0;
    unsigned short bitCount=0;
    if (icon_index>iconsCount-1) icon_index = iconsCount-1;
    if (icon_index<0)	{
	// we choose icon with max resolution
	unsigned long tmpW=0,tmpH=0;
	for (int i = 0; i < iconsCount; i++)	{
	    tmpW = dirEntry->bWidth == 0 ? 256 : dirEntry->bWidth;
	    tmpH = dirEntry->bHeight == 0 ? 256 : dirEntry->bHeight;
	    if (tmpW * tmpH > maxSize)	{
		maxSize = tmpW * tmpH;
		icon_index = i;
	    }
	    dirEntry++;
	}
    }
    dirEntry = (TINYICONDIRENTRY*)(cursor);
    dirEntry+=icon_index;
    width = dirEntry->bWidth == 0 ? 256 : dirEntry->bWidth;
    height = dirEntry->bHeight == 0 ? 256 : dirEntry->bHeight;
    bitCount = dirEntry->wBitCount;
    offset = dirEntry->dwImageOffset;
    size = dirEntry->dwBytesInRes;

    if (offset == 0) return NULL;

    cursor = buffer;
    cursor += offset;

    unsigned long raw_size = width*height*bitCount/8;

    unsigned char* image = NULL;

#   if (defined(STBI_INCLUDE_STB_IMAGE_H) && !defined(STBI_NO_PNG))
    if (raw_size!=0 && raw_size>size) image = stbi_load_from_memory(&buffer[offset],size,x,y,comp,0);
    if (image) return image;
#   endif

    TINYICONIMAGE* icon = (TINYICONIMAGE*)(cursor);
    int realBitsCount = (int)icon->icHeader.biBitCount;
    bool hasAndMask = (realBitsCount < 32) && (height != (int)icon->icHeader.biHeight);

    cursor += 40;
    int numBytes = width * height * 4;
    image = (unsigned char*) STBI_MALLOC(numBytes);

    // rgba + vertical swap
    if (realBitsCount == 32)
    {
	int shift;
	int shift2;
	for (int x = 0; x < width; x++)
	    for (int y = 0; y < height; y++)
	    {
		shift = 4 * (x + y * width);
		shift2 = 4 * (x + (height - y - 1) * width);
		image[shift] = cursor[shift2 +2];
		image[shift+1] = cursor[shift2 +1];
		image[shift+2] = cursor[shift2 ];
		image[shift+3] = cursor[shift2 +3];
	    }
    }

    if (realBitsCount == 24)
    {
	int shift;
	int shift2;
	for (int x = 0; x < width; x++)
	    for (int y = 0; y < height; y++)
	    {
		shift = 4 * (x + y * width);
		shift2 = 3 * (x + (height - y - 1) * width);
		image[shift] = cursor[shift2 +2];
		image[shift+1] = cursor[shift2 +1];
		image[shift+2] = cursor[shift2 ];
		image[shift+3] = 255;
	    }
    }

    if (realBitsCount == 8)  /// 256 colors
    {
	// 256 color table
	unsigned char * colors = (unsigned char *)cursor;
	cursor += 256 * 4;
	int shift;
	int shift2;
	int index;
	for (int x = 0; x < width; x++)
	    for (int y = 0; y < height; y++)
	    {
		shift = 4 * (x + y * width);
		shift2 = (x + (height - y - 1) * width);
		index = 4 * cursor[shift2];
		image[shift] = colors[index + 2];
		image[shift+1] = colors[index + 1];
		image[shift+2] = colors[index ];
		image[shift+3] = 255;
	    }
    }

    if (realBitsCount == 4)  /// 16 colors
    {
	// 16 color table
	unsigned char * colors = (unsigned char *)cursor;
	cursor += 16 * 4;
	int shift;
	int shift2;
	unsigned char index;
	for (int x = 0; x < width; x++)
	    for (int y = 0; y < height; y++)
	    {
		shift = 4 * (x + y * width);
		shift2 = (x + (height - y - 1) * width);
		index = cursor[shift2 / 2];
		if (shift2 % 2 == 0)
		    index = (index >> 4) & 0xF;
		else
		    index = index & 0xF;
		index *= 4;

		image[shift] = colors[index + 2];
		image[shift+1] = colors[index + 1];
		image[shift+2] = colors[index ];
		image[shift+3] = 255;
	    }
    }

    if (realBitsCount == 1)  /// 2 colors
    {
	// 2 color table
	unsigned char * colors = (unsigned char *)cursor;
	cursor += 2 * 4;
	int shift;
	int shift2;
	unsigned char index;
	unsigned char bit;

	int boundary = width; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
	while (boundary % 32 != 0) boundary++;

	for (int x = 0; x < width; x++)
	    for (int y = 0; y < height; y++)
	    {
		shift = 4 * (x + y * width);
		shift2 = (x + (height - y - 1) * boundary);
		index = cursor[shift2 / 8];

		// select 1 bit only
		bit = 7 - (x % 8);
		index = (index >> bit) & 0x01;
		index *= 4;

		image[shift] = colors[index + 2];
		image[shift+1] = colors[index + 1];
		image[shift+2] = colors[index ];
		image[shift+3] = 255;
	    }
    }

    // Read AND mask after base color data - 1 BIT MASK
    if (hasAndMask)
    {
	int shift;
	int shift2;
	unsigned char bit;
	int mask;

	int boundary = width * realBitsCount; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
	while (boundary % 32 != 0) boundary++;
	cursor += boundary * height / 8;

	boundary = width;
	while (boundary % 32 != 0) boundary++;

	for (int y = 0; y < height; y++)
	    for (int x = 0; x < width; x++)
	    {
		shift = 4 * (x + y * width) + 3;
		bit = 7 - (x % 8);
		shift2 = (x + (height - y - 1) * boundary) / 8;
		mask = (0x01 & ((unsigned char)cursor[shift2] >> bit));
		//LOG << "Bit: " << bit << "Value: " << mask << " from byte: " << cursor[shift2] << " row: " << y << " index:" << shift2 << NL;
		image[shift] *= 1 - mask;

	    }
    }

    return image;
}
#endif //TINY_ICO_NO_DECODER

#ifndef TINY_ICO_NO_ENCODER
unsigned char* tiny_ico_save_from_memory(unsigned char const* buffer,int x,int y,int comp,int* returned_buffer_size)  {
    const int w = x;const int h=y;
    if (!buffer || x<=0 || y<=0 || x>256 || y>256 || comp!=4 || !returned_buffer_size) return NULL;
    *returned_buffer_size=0;
    unsigned char* png_image = NULL;
    int res_image_size = 0;
    if (w==256 && h==256) png_image = stbi_write_png_to_mem((unsigned char *)buffer,0,w,h,comp,&res_image_size);
    bool hasIntermediateTransparency = true;
    unsigned int stride_XOR_bitmap = 0;
    unsigned int size_XOR_bitmap = 0;
    unsigned int stride_AND_bitmap = 0;
    unsigned int size_AND_bitmap = 0;

    unsigned short bitCount = 32;
    unsigned int file_size = 6 + 16;
    if (png_image && res_image_size>0) {file_size+=res_image_size;}
    else {
	res_image_size= 40;	// BITMAPINFOHEADER

	const unsigned char* pbuf = &buffer[3];
	hasIntermediateTransparency = false;
	for (unsigned i=0,isz=(w*h);i<isz;i++) {
	    if (*pbuf>0 && (*pbuf)<255) {hasIntermediateTransparency=true;break;}
	    pbuf+=4;
	}
	if (!hasIntermediateTransparency) bitCount=24;

	// Add the size of XOR and AND Bitmaps
	stride_XOR_bitmap = (bitCount*w)/8;
	if(stride_XOR_bitmap & 3) stride_XOR_bitmap = (stride_XOR_bitmap + 4) & ~3;    // multiple of 4 bytes
	size_XOR_bitmap = stride_XOR_bitmap*h;
	res_image_size+= size_XOR_bitmap;			// XOR-Bitmap

	stride_AND_bitmap = w/8;
	if(stride_AND_bitmap & 3) stride_AND_bitmap = (stride_AND_bitmap + 4) & ~3;    // multiple of 4 bytes
	size_AND_bitmap = stride_AND_bitmap*h;
	res_image_size+= size_AND_bitmap;			// AND-Bitmap

	file_size+=res_image_size;
    }

    *returned_buffer_size = (int) file_size;
    unsigned char* image = (unsigned char*) STBIW_MALLOC(file_size);
    unsigned char* cursor = image;
    TINYICONDIR& iconDir = *((TINYICONDIR*)cursor);
    iconDir.idType=1;
    iconDir.idReserved=0;
    iconDir.idCount=1;
    cursor+=6;
    TINYICONDIRENTRY& iconDirEntry = *((TINYICONDIRENTRY*)cursor);
    iconDirEntry.bWidth=	(w == 256) ? 0 : (unsigned char) w;
    iconDirEntry.bHeight=	(h == 256) ? 0 : (unsigned char) h;
    iconDirEntry.bColorCount=	0;
    iconDirEntry.bReserved=	0;
    iconDirEntry.wPlanes=	1;
    iconDirEntry.wBitCount=	bitCount;
    iconDirEntry.dwBytesInRes=	res_image_size;
    iconDirEntry.dwImageOffset= 6+16;
    cursor+=16;

    if (png_image) {
	memcpy(cursor,png_image,res_image_size);
	STBIW_FREE(png_image);png_image=NULL;	
    }
    else {
	TINYICOBITMAPINFOHEADER& bmpInfoHeader = *((TINYICOBITMAPINFOHEADER*)cursor);
	bmpInfoHeader.biSize=		40;
	bmpInfoHeader.biWidth=		w;
	bmpInfoHeader.biHeight=		h*2;
	bmpInfoHeader.biPlanes=		1;
	bmpInfoHeader.biBitCount=	bitCount;
	bmpInfoHeader.biCompression=    0;
	bmpInfoHeader.biSizeImage=	0;
	bmpInfoHeader.biXPelsPerMeter=  2834;    // 0 ?
	bmpInfoHeader.biYPelsPerMeter=  2834;    // 0 ?
	bmpInfoHeader.biClrUsed=	0;
	bmpInfoHeader.biClrImportant=   0;
	cursor+=40;

//	fprintf(stderr,"stride_XOR_bitmap=%u size_XOR_bitmap=%u stride_AND_bitmap=%u size_AND_bitmap=%u\n",
//	stride_XOR_bitmap,
//	size_XOR_bitmap,
//	stride_AND_bitmap,
//	size_AND_bitmap
//	);

	const unsigned short numCh = bitCount/8;

	TINY_ICO_ASSERT(numCh==3 || numCh==4);

	// rgba + vertical swap (TODO: rewrite using stride_XOR_bitmap
	if (bitCount == 32)
	{
	    int shift;
	    int shift2;
	    for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
		    shift = 4 * (x + y * w);
		    shift2 = 4 * (x + (h - y - 1) * w);
		    cursor[shift2 +2]	= buffer[shift];
		    cursor[shift2 +1]	= buffer[shift+1];
		    cursor[shift2]	= buffer[shift+2];
		    cursor[shift2 +3]	= buffer[shift+3];
		}
	}
	else if (bitCount == 24)
	{
	    int shift;
	    int shift2;
	    for (int x = 0; x < w; x++)
		for (int y = 0; y < h; y++)
		{
		    shift = 4 * (x + y * w);
		    shift2 = 3 * (x + (h - y - 1) * w);
		    cursor[shift2 +2]	= buffer[shift];
		    cursor[shift2 +1]	= buffer[shift+1];
		    cursor[shift2 ]	= buffer[shift+2];
		}
	}
	else TINY_ICO_ASSERT(true);

	cursor+=size_XOR_bitmap;

	// Write AND mask after base color data - 1 BIT MASK
	{
	    int shift;
	    int shift2;
	    unsigned char bit;

	    for (unsigned int i=0;i<size_AND_bitmap;i++) cursor[i]=0;	// 255 makes all transparent

	    int boundary = w;
	    while (boundary % 32 != 0) boundary++;	// boundary should be: stride_AND_bitmap*8 AFAIK

	    for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
		{
		    shift = 4 * (x + y * w) + 3;
		    bit = 7 - (x % 8);
		    shift2 = (x + (h - y - 1) * boundary) / 8;

		    if (buffer[shift]==0) cursor[shift2]|=(1<<bit);   // setting the transparency bit

		}
	}

	// Check file size (optional)
	cursor+=size_AND_bitmap;
	unsigned int tmp = cursor-image;
	TINY_ICO_ASSERT(tmp==file_size);

    }

    return image;
}
#endif //TINY_ICO_NO_ENCODER

#ifdef __cplusplus
} //extern "C"
#endif

#endif //TINY_ICO_IMPLEMENTATION

#endif //TINY_ICO_H


