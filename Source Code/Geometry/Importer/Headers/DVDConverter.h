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

//Assimp License
//
//Copyright (c) 2006-2012 assimp team
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
//    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//    Neither the name of the assimp team nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
//INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
//IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
//OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef _DIVIDE_FORMAT_CONVERTER_H_
#define _DIVIDE_FORMAT_CONVERTER_H_

#include "resource.h"
#include "Geometry/Material/Headers/Material.h"
#include "Hardware/Video/Texture.h"

class  Mesh;
class  SubMesh;
struct aiScene;
struct aiMesh;
struct aiMaterial;
enum   aiTextureOp;
enum   aiTextureMapMode;
DEFINE_SINGLETON( DVDConverter )

public:
    Mesh* load(const std::string& file);
private:
	SubMesh* loadSubMeshGeometry(aiMesh* source, U8 count);
	Material* loadSubMeshMaterial(aiMaterial* source, const std::string& materialName);
	Material::TextureOperation aiTextureOpToTextureOperation(aiTextureOp op);
	Material::ShadingMode      shadingModeInternal(I32 mode);
	Texture::TextureWrap       aiTextureMapModeToInternal(aiTextureMapMode mode);

private:
	const aiScene* scene;
	U32   _ppsteps;
	std::string _fileLocation;
	std::string _modelName;

END_SINGLETON

#endif