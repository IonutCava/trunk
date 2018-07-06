#ifndef _OPCODES_H_
#define _OPCODES_H_

#include "WorldPacket.h"
#include "Session.h"

enum Opcodes
{
    CMSG_PING       = 0x000,
	SMSG_PONG       = 0x001,
	NUM_MSG_TYPES   = 0x002
};

enum SessionStatus
{
    STATUS_AUTHED = 0,
    STATUS_LOGGEDIN, 
    STATUS_TRANSFER, 
    STATUS_LOGGEDIN_OR_RECENTLY_LOGGEDOUT,
    STATUS_NEVER,
    STATUS_UNHANDLED
};

enum PacketProcessing
{
    PROCESS_INPLACE = 0,
    PROCESS_THREADUNSAFE,
    PROCESS_THREADSAFE
};

class WorldPacket;

struct OpcodeHandler
{
    char const* name;
    SessionStatus status;
    PacketProcessing packetProcessing;
    void (Session::*handler)(WorldPacket& recvPacket);
};

extern OpcodeHandler opcodeTable[NUM_MSG_TYPES];

/// Lookup opcode name for human understandable logging
inline const char* LookupOpcodeName(U16 id)
{
    if (id >= NUM_MSG_TYPES)
        return "Received unknown opcode, it's more than max!";
    return opcodeTable[id].name;
}
#endif;