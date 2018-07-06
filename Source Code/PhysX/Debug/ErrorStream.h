/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _ERROR_STREAM_H
#define _ERROR_STREAM_H

#ifdef _WIN32

#define NOMINMAX
#include <windows.h>
#endif

class ErrorStream : public NxUserOutputStream
	{
	public:
	void reportError(NxErrorCode e, const char* message, const char* file, I32 line)
		{
		printf("%s (%d) :", file, line);
		switch (e)
			{
			case NXE_INVALID_PARAMETER:
				printf( "invalid parameter");
				break;
			case NXE_INVALID_OPERATION:
				printf( "invalid operation");
				break;
			case NXE_OUT_OF_MEMORY:
				printf( "out of memory");
				break;
			case NXE_DB_INFO:
				printf( "info");
				break;
			case NXE_DB_WARNING:
				printf( "warning");
				break;
			default:
				printf("unknown error");
			}

		printf(" : %s\n", message);
		}

	NxAssertResponse reportAssertViolation(const char* message, const char* file, I32 line)
		{
		printf("access violation : %s (%s line %d)\n", message, file, line);

#ifdef WIN32
		switch (MessageBox(0, message, "AssertViolation, see console for details.", MB_ABORTRETRYIGNORE))
			{
			case IDRETRY:
				return NX_AR_CONTINUE;
			case IDIGNORE:
				return NX_AR_IGNORE;
			case IDABORT:
			default:
				return NX_AR_BREAKPOINT;
			}
#elif LINUX
		assert(0);
#elif _XBOX
		return NX_AR_BREAKPOINT;
#elif __CELLOS_LV2__
		return NX_AR_BREAKPOINT;
#elif __PPCGEKKO__
		return NX_AR_BREAKPOINT;		
#endif
		}

	void print(const char* message)
		{
		printf(message);
		}
	};
#endif