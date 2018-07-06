/*�Copyright 2009-2013 DIVIDE-Studio�*/
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

#ifndef _UNORDERED_MAP_H_
#define _UNORDERED_MAP_H_

#if defined(UNORDERED_MAP_IMP) && UNORDERED_MAP_IMP == 0

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#define unordered_map    boost::unordered_map
#define for_each         BOOST_FOREACH
#define reverse_for_each BOOST_REVERSE_FOREACH

#else

#include <unordered_map>
#include <algorithm>
#define unordered_map std::tr1::unordered_map
///ToDo: fix these 3 to use std::for_each and lambda expressions
#include <boost/foreach.hpp>
#define for_each BOOST_FOREACH 
#define reverse_for_each BOOST_REVERSE_FOREACH


#endif

#endif