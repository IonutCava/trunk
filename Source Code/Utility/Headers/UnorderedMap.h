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

#ifndef _UNORDERED_MAP_H_
#define _UNORDERED_MAP_H_

#ifndef _CONFIG_HEADER_
#include "config.h"
#endif

#if defined(UNORDERED_MAP_IMP) && UNORDERED_MAP_IMP == 0

#include <boost/Unordered_map.hpp>

#define Unordered_map    boost::unordered_map

#include <boost/foreach.hpp>
#define for_each BOOST_FOREACH
#define reverse_for_each BOOST_REVERSE_FOREACH

#else

#include <Unordered_map>
#include <algorithm>
#define Unordered_map std::unordered_map
#pragma message("ToDo: fix std::for_each and lambda expressions -Ionut")
#include <boost/foreach.hpp>
#define for_each BOOST_FOREACH
#define reverse_for_each BOOST_REVERSE_FOREACH
#endif

#endif