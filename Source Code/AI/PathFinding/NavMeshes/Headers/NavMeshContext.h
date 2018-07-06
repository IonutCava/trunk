#ifndef _NAV_MESH_CONTEXT_H_
#define _NAV_MESH_CONTEXT_H_

#ifndef RECAST_UTIL_PROPERTIES
#define RECAST_UTIL_PROPERTIES
#endif
#include <ReCast/Include/Recast.h>

namespace Navigation{
	
	class rcContextDivide : public rcContext{
    public:
        rcContextDivide(bool state) : rcContext(state) {}
        ~rcContextDivide() {}
    private:
        /// Logs a message.
	    ///  @param[in]		category	The category of the message.
	    ///  @param[in]		msg			The formatted message.
	    ///  @param[in]		len			The length of the formatted message.
	    void doLog(const rcLogCategory /*category*/, const char* /*msg*/, const I32 /*len*/);
    };
};

#endif