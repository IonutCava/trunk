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

#ifndef _OPCODES_IMPL_H_
#define _OPCODES_IMPL_H_

#include <DivideNetworking/OPCodesTpl.h>
#include <DivideNetworking/Utility/InheritEnum.h>

enum OPCodesEx {
	CMSG_GEOMETRY_LIST                       = OPCODE_ID(1),
	SMSG_GEOMETRY_APPEND					 = OPCODE_ID(2),
	CMSG_REQUEST_GEOMETRY					 = OPCODE_ID(3),
	CMSG_PING						         = OPCODE_ID(4),
	SMSG_PONG								 = OPCODE_ID(5),
	LAST_OPCODE
};

typedef InheritEnum< OPCodesEx, OPCodes > OPCodesImpl;

#endif;