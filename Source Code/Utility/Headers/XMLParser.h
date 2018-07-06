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

#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_

#if defined(_MSC_VER)
#	pragma warning( push )
#		pragma warning(disable:4103) ///<Boost alignment shouts
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		//pragma GCC diagnostic ignored "-Wall"
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

class Scene;
class Texture;
class Material;
class SceneManager;

namespace XML {

	///Parent Function
	std::string loadScripts(const std::string &file);

	///Child Functions
	void loadConfig(const std::string& file);
	void loadScene(const std::string& sceneName, SceneManager& sceneMgr);
	void loadGeometry(const std::string& file, Scene* const scene);
	void loadTerrain(const std::string& file, Scene* const scene);
	Material* loadMaterial(const std::string &file);
	void dumpMaterial(Material* const mat);

	Material* loadMaterialXML(const std::string& location);
	Texture*  loadTextureXML(const std::string& textureNode, const std::string& textureName);
	void      saveTextureXML(const std::string& textureNode, unsigned int operation, Texture* texture);
}

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#endif