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

#ifndef _OCCLUSION_QUERY_H
#define _OCCLUSION_QUERY_H
#include "core.h"

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
	virtual QueryResult getStatus(bool blockCPU, U32* visiblePixels = NULL) = 0;
	///Prepares the query. If the previous query is finished, returns true, else it returs false
	virtual bool begin() = 0;
	///Called after the geometry is drawn to process the query and prepare the result
	virtual end() = 0;
};

#endif