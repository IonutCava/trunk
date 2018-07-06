#ifndef _OPCODES_H_
#define _OPCODES_H_

enum Opcodes
{
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