/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _OCCLUSION_QUERY_H
#define _OCCLUSION_QUERY_H
#include "core.h"

namespace Divide {

class OcclusionQuery {
public:
	OcclusionQuery(){}
	virtual ~OcclusionQuery(){}

	enum QueryResult{
		OC_VISIBLE = 0,
		OC_OCCLUDED,
		OC_WAITING,
		OC_ERROR,
		OC_PLACEHOLDER
	};

	///Returns the status of the last query
	virtual QueryResult getStatus(bool blockCPU, U32* visiblePixels = nullptr) = 0;
	///Prepares the query. If the previous query is finished, returns true, else it returs false
	virtual bool begin() = 0;
	///Called after the geometry is drawn to process the query and prepare the result
	virtual end() = 0;
};

}; //namespace Divide

#endif