/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _OPCODES_H_
#define _OPCODES_H_

enum Opcodes{

   	MSG_HEARTBEAT							 = 0x000,
	SMSG_SEND_FILE                           = 0x001,
    CMSG_PING						         = 0x002,
	SMSG_PONG								 = 0x003,
	CMSG_REQUEST_DISCONNECT					 = 0x004,
	CMSG_GEOMETRY_LIST                       = 0x005,
	SMSG_GEOMETRY_APPEND					 = 0x006,
	CMSG_REQUEST_GEOMETRY					 = 0x007,
	SMSG_DISCONNECT							 = 0x008,
	NUM_MSG_TYPES							 = 0x009
};

#endif;