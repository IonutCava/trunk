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

/*Code references:
	http://www.cplusplus.com/forum/lounge/27570/
*/
#ifndef UTIL_CRC_H_
#define UTIL_CRC_H_

#include "Platform/DataTypes/Headers/PlatformDefines.h"

namespace Divide {
namespace Util {
	class CRC32	{
	public:
		//=========================================
		//  ctors
		inline CRC32()                                   { Reset();                  }
		inline CRC32(const void* buf, size_t siz)        { Reset(); Hash(buf,siz);   }

		//=========================================
		/// implicit cast, so that you can do something like foo = CRC(dat,siz);
		inline operator U32 () const                    { return Get();             }

		//=========================================
		/// getting the crc
		inline U32          Get() const                 { return ~mCrc;             }

		//=========================================
		// HashBase stuff
		virtual void        Reset()                     { mCrc = (U32)~0;                }
		virtual void        Hash(const void* buf,size_t siz);

	private:
		U32         mCrc;
		static U32  mTable[0x100];

	private:
		//=========================================
		// internal support
		static U32          Reflect(U32 v,I32 bits);
	};
}; //namespace Util
}; //namespace Divide

#endif