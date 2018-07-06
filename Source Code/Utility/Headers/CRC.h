/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */
/*Code references:
	http://www.cplusplus.com/forum/lounge/27570/
*/
#ifndef UTIL_CRC_H_
#define UTIL_CRC_H_

#include "core.h"

namespace Util {

	class CRC32	{
	public:
		//=========================================
		//  ctors
		inline CRC32()                                  { Reset();                  }
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

		static const U32        POLYNOMIAL = 0x04C11DB7;

	private:
		//=========================================
		// internal support
		static void         BuildTable();
		static U32          Reflect(U32 v,int bits);
	};
};

#endif