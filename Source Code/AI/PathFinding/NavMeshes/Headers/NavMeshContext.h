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

#ifndef _NAV_MESH_CONTEXT_H_
#define _NAV_MESH_CONTEXT_H_

#include "NavMeshDefines.h"

namespace Navigation{
	
	class rcContextDivide : public rcContext{
    public:
        rcContextDivide(bool state) : rcContext(state) {resetTimers();}
        ~rcContextDivide() {}

    private:
        /// Logs a message.
	    ///  @param[in]		category	The category of the message.
	    ///  @param[in]		msg			The formatted message.
	    ///  @param[in]		len			The length of the formatted message.
	    void doLog(const rcLogCategory /*category*/, const char* /*msg*/, const I32 /*len*/);
		void doResetTimers();
		void doStartTimer(const rcTimerLabel /*label*/);
		void doStopTimer(const rcTimerLabel /*label*/);
		I32 doGetAccumulatedTime(const rcTimerLabel /*label*/) const;

	private:
		U32 _startTime[RC_MAX_TIMERS];
		I32 _accTime[RC_MAX_TIMERS];
    };
};

#endif