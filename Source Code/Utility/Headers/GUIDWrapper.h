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
#ifndef _GUID_WRAPPER_H_
#define _GUID_WRAPPER_H_
#include "Hardware/Platform/Headers/PlatformDefines.h"
///Utility class that adds basic GUID management to objects
class GUIDWrapper {
public:

    GUIDWrapper() : _GUID(_idGenerator++)
    {
    }

    virtual ~GUIDWrapper()
    {
        _idGenerator--;
    }

    GUIDWrapper(const GUIDWrapper& old) : _GUID(_idGenerator++)
    {
    }

    inline I64  getGUID()   const  {return _GUID;}

private:
	void operator=(const GUIDWrapper& old)
	{
	}

protected:
    const  I64 _GUID;

private:
    static I64 _idGenerator;
};

#endif