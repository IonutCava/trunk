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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
class Material;
class Texture;

namespace XML {

	///Parent Function
	void loadScripts(const std::string &file);

	///Child Functions
	void loadConfig(const std::string& file);
	void loadScene(const std::string& sceneName);
	void loadGeometry(const std::string& file);
	void loadTerrain(const std::string& file);
	Material* loadMaterial(const std::string &file);
	void dumpMaterial(Material* const mat);
	///ToDo: ....... Add more

	Material* loadMaterialXML(const std::string& location);
	Texture*  loadTextureXML(const  std::string& textureName);
}