#ifndef _OPCODES_H_
#define _OPCODES_H_

enum Opcodes
{
   	MSG_HEARTBEAT   = 0x000,
    CMSG_PING       = 0x001,
	SMSG_PONG       = 0x002,
	NUM_MSG_TYPES   = 0x003
};

#endif;