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

#ifndef _D3D_RESOURCES_H_
#define _D3D_RESOURCES_H_

#if TARGET_D3D_VERSION == D3D10
#define D3D10_IGNORE_SDK_LAYERS
//#include <D3D10.h>
#include <CEGUI/RendererModules/Direct3D10/Renderer.h>
#else
//#include <D3D11.h>
#include <CEGUI/RendererModules/Direct3D11/Renderer.h>
#endif

#endif