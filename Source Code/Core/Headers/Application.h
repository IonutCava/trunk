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

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "core.h"
class Kernel;
///Lightweight singleton class that manages our application's kernel and window information
DEFINE_SINGLETON( Application )

public:
	///Startup and shutdown
	I8 Initialize(const std::string& entryPoint,I32 argc, char **argv);
	void Deinitialize();
	void run();

	///Application resolution (either fullscreen resolution or window dimensions)
	inline const vec2<U16>& getResolution() const {return _resolution;}
	inline void setResolutionWidth(U16 w){_resolution.x = w;}
	inline void setResolutionHeight(U16 h){_resolution.y = h;}
	inline void RequestShutdown()   {_requestShutdown = true;}
	inline void CancelShutdown()    {_requestShutdown = false;}
	inline bool ShutdownRequested() {return _requestShutdown;}
	///Application window ID
	inline I8 const&  getMainWindowId() {return _mainWindowId;}
	inline Kernel* const getKernel()    {return _kernel;}

	inline void setMemoryLogFile(const std::string& fileName) {memLogBuffer.open(fileName.c_str());}
	inline void logMemoryAllocation(const std::stringstream& buffer) {if(memLogBuffer.is_open()){ memLogBuffer << buffer.str();}}

private:
	Application();
	~Application();

private:
	bool _requestShutdown;
	I8 _mainWindowId;
	vec2<U16> _resolution;
	Kernel* _kernel;
	///buffer to register all of the memory allocations recorded via "New"
	std::ofstream memLogBuffer;
	
END_SINGLETON

#endif