/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class Scene;
class GFXDevice;
class Configuration;
class SceneGraphNode;
class PlatformContext;
FWD_DECLARE_MANAGED_CLASS(Material);

namespace XML {

#ifndef GET_PARAM
#define LOAD_FILE(X) boost::property_tree::iptree pt; \
                     read_xml(X, pt);

#define PREPARE_FILE_FOR_WRITING(X) boost::property_tree::iptree pt; \
                                    createFile(X, true);

#define SAVE_FILE(X) write_xml(X, \
                               pt, \
                               std::locale(), \
                               boost::property_tree::xml_writer_make_settings<boost::property_tree::iptree::key_type>('\t', 1));

#define FILE_VALID() (!pt.empty())
#define CONCAT(first, second) first second
#define GET_PARAM(X) GET_TEMP_PARAM(X, X)
#define GET_TEMP_PARAM(X, TEMP) TEMP = pt.get(TO_STRING(X), TEMP)
#define GET_PARAM_ATTRIB(X, Y) \
    X.Y = pt.get(CONCAT(CONCAT(TO_STRING(X), \
                               ".<xmlattr>."),\
                        TO_STRING(Y)), \
                 X.Y)

#define PUT_PARAM(X) PUT_TEMP_PARAM(X, X)
#define PUT_TEMP_PARAM(X, TEMP) pt.put(TO_STRING(X), TEMP);
#define PUT_PARAM_ATTRIB(X, Y) pt.put(CONCAT(CONCAT(TO_STRING(X), \
                                                    ".<xmlattr>."),\
                                              TO_STRING(Y)), \
                                      X.Y)
#endif

class IXMLSerializable {
protected:
    friend bool loadFromXML(IXMLSerializable& object, const char* file);
    friend bool saveToXML(const IXMLSerializable& object, const char* file);

    virtual bool fromXML(const char* xmlFile) = 0;
    virtual bool toXML(const char* xmlFile) const = 0;
};

bool loadFromXML(IXMLSerializable& object, const char* file);
bool saveToXML(const IXMLSerializable& object, const char* file);

/// Child Functions
void loadDefaultKeybindings(const stringImpl &file, Scene* scene);

void loadScene(const stringImpl& scenePath, const stringImpl& sceneName, Scene* scene, const Configuration& config);

void loadGeometry(const stringImpl& file, Scene* const scene);
void loadTerrain(const stringImpl& file, Scene* const scene);
void loadMusicPlaylist(const stringImpl& file, Scene* const scene, const Configuration& config);

};  // namespace XML
};  // namespace Divide

#endif
