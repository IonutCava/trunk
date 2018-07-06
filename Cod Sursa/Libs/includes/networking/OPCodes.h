#ifndef _OPCODES_H_
#define _OPCODES_H_

enum Opcodes
{
   	MSG_HEARTBEAT							 = 0x000,
    CMSG_PING						         = 0x001,
	SMSG_PONG								 = 0x002,
	CMSG_REQUEST_DISCONNECT					 = 0x003,
	CMSG_GEOMETRY_LIST                       = 0x004,
	SMSG_GEOMETRY_APPEND					 = 0x005,
	CMSG_REQUEST_GEOMETRY					 = 0x006,
	SMSG_DISCONNECT							 = 0x007,
	NUM_MSG_TYPES							 = 0x008
};

#endif;