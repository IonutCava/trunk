#include "Headers/NavMeshContext.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Navigation {

	void rcContextDivide::doLog(const rcLogCategory category, const char* msg, const I32 len){
        switch(category){
            default:
            case RC_LOG_PROGRESS:
                PRINT_FN(Locale::get("RECAST_CONTEXT_LOG_PROGRESS"),msg);
                break;
            case RC_LOG_WARNING:
                PRINT_FN(Locale::get("RECAST_CONTEXT_LOG_WARNING"),msg);
                break;
            case RC_LOG_ERROR:
                ERROR_FN(Locale::get("RECAST_CONTEXT_LOG_ERROR"),msg);
                break;
        }
    }
};