/*“Copyright 2009-2012 DIVIDE-Studio”*/
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
	void Initialize(const std::string& entryPoint);
	void Deinitialize();
	void run();

	///Application window dimensions
	inline const vec2<F32>& getWindowDimensions() const {return _dimensions;}
	inline void setWindowWidth(U16 w){_dimensions.x = w;}
	inline void setWindowHeight(U16 h){_dimensions.y = h;}

	///Application window ID
	inline I8 const&  getMainWindowId() {return _mainWindowId;}
	inline Kernel* const getKernel()    {return _kernel;}

	inline void setMemoryLogFile(const std::string& fileName) {memLogBuffer.open(fileName.c_str());}
	inline void logMemoryAllocation(const std::stringstream& buffer) {if(memLogBuffer.is_open()){ memLogBuffer << buffer.str();}}

private:
	Application();
	~Application();

private:
	I8 _mainWindowId;
	vec2<F32> _dimensions;
	Kernel* _kernel;
	///buffer to register all of the memory allocations recorded via "New"
	std::ofstream memLogBuffer;
	
END_SINGLETON

#endif