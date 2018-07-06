/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class Scene;

FWD_DECLARE_MANAGED_CLASS(Material);

namespace XML {
/// Parent Function
stringImpl loadScripts(const stringImpl& file);

/// Child Functions
void loadConfig(const stringImpl& file);
void loadDefaultKeybindings(const stringImpl &file, Scene* scene);
void loadScene(const stringImpl& sceneName, Scene* scene);
void loadGeometry(const stringImpl& file, Scene* const scene);
void loadTerrain(const stringImpl& file, Scene* const scene);
void loadMusicPlaylist(const stringImpl& file, Scene* const scene);

Material_ptr loadMaterial(const stringImpl& file);
void dumpMaterial(Material& mat);

Material_ptr loadMaterialXML(const stringImpl& location, bool rendererDependent = true);
};  // namespace XML
};  // namespace Divide

#endif
