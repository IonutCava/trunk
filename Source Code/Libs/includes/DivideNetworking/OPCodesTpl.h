#ifndef _OPCODES_H_INFO_
#define _OPCODES_H_INFO_

#ifndef OPCODE_ENUM
#define OPCODE_ENUM
#endif

namespace Divide {
///Packet handling requires OPCodes to be defined. Use the following num structure to define them in each app:
enum OPCodes
{
    MSG_HEARTBEAT                            = 0x000,
    SMSG_SEND_FILE                           = 0x001,
    SMSG_DISCONNECT                          = 0x002,
    CMSG_REQUEST_DISCONNECT                  = 0x003
};

#define FIRST_FREE_OPCODE 0x004
#define LAST_OPCODE NUM_MSG_TYPES
#define OPCODE_ID(X) ((unsigned int)FIRST_FREE_OPCODE + (X))

/*To create new OPCodes follow this convention:
#include <DivideNetworking/Utility/InheritEnum.h>

enum OPCodesEx { //<Or whaterver name you wish
    CMSG_EXAMPLE  = OPCODE_ID(1),
    SMSG_EXAMPLE2 = OPCODE_ID(2),
                .....
    LAST_OPCODE
};

typedef InheritEnum< OPCodesEx, OPCodes > OPCodesImpl;

And use OPCodesImpl for switch statements and packet handling
*/

}; //namespace Divide
#endif
