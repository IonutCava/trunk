#ifndef _OPCODES_H_
#define _OPCODES_H_

enum Opcodes
{
   	MSG_HEARTBEAT							 = 0x000,
    CMSG_PING						         = 0x001,
	SMSG_PONG								 = 0x002,
	CMSG_REQUEST_DISCONNECT					 = 0x003,
	CMSG_GEOMERTY_LIST                       = 0x004,
	SMSG_GEOMETRY_APPEND					 = 0x005,
	SMSG_DISCONNECT							 = 0x006,
	NUM_MSG_TYPES							 = 0x007
};

#endif;