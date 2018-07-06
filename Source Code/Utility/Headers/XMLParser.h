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

#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_

#if defined(_MSC_VER)
#	pragma warning( push )
#		pragma warning(disable:4103) ///<Boost alignment shouts
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		//pragma GCC diagnostic ignored "-Wall"
#endif

#include "Utility/Headers/String.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace Divide {

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
	void dumpMaterial(Material& mat);

	Material* loadMaterialXML(const std::string& location, bool rendererDependent = true);
}; //namespace XML
}; //namespace Divide

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#endif