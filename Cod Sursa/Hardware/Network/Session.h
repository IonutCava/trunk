#ifndef _SESSION_H_
#define _SESSION_H_

#include "resource.h"
/// Player session in the World
class Session
{
    public:
        Session(U32 id, time_t mute_time);
        ~Session();
};

#endif