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
class SceneGraphNode;
class PlatformContext;

struct Configuration;

FWD_DECLARE_MANAGED_CLASS(Material);

namespace XML {

namespace detail {
    struct LoadSave {
        bool _saveFileOK = false;
        stringImpl _loadPath = "";

        mutable stringImpl _savePath = "";
        mutable boost::property_tree::iptree XmlTree;

        bool read(const stringImpl& path);
        bool prepareSaveFile(const stringImpl& path) const;
        void write() const;
    };
};

#ifndef GET_PARAM
#define CONCAT(first, second) first second

#define GET_PARAM(X) GET_TEMP_PARAM(X, X)

#define GET_TEMP_PARAM(X, TEMP) \
    TEMP = LoadSave.XmlTree.get(TO_STRING(X), TEMP)

#define GET_PARAM_ATTRIB(X, Y) \
    X.Y = LoadSave.XmlTree.get(CONCAT(CONCAT(TO_STRING(X), ".<xmlattr>."), TO_STRING(Y)), X.Y)

#define PUT_PARAM(X) PUT_TEMP_PARAM(X, X)
#define PUT_TEMP_PARAM(X, TEMP) \
    LoadSave.XmlTree.put(TO_STRING(X), TEMP);
#define PUT_PARAM_ATTRIB(X, Y) \
    LoadSave.XmlTree.put(CONCAT(CONCAT(TO_STRING(X), ".<xmlattr>."), TO_STRING(Y)), X.Y)
#endif

class IXMLSerializable {
protected:
    friend bool loadFromXML(IXMLSerializable& object, const char* file);
    friend bool saveToXML(const IXMLSerializable& object, const char* file);

    virtual bool fromXML(const char* xmlFile) = 0;
    virtual bool toXML(const char* xmlFile) const = 0;

protected:
    XML::detail::LoadSave LoadSave;
};

bool loadFromXML(IXMLSerializable& object, const char* file);
bool saveToXML(const IXMLSerializable& object, const char* file);

/// Child Functions
void loadDefaultKeyBindings(const stringImpl &file, Scene* scene);


struct SceneNode {
    Str64 name;
    Str32 type;

    vectorSTD<SceneNode> children;
};

void loadSceneGraph(const Str256& scenePath, const Str64& fileName, Scene* const scene);
void loadMusicPlaylist(const Str256& scenePath, const Str64& fileName, Scene* const scene, const Configuration& config);

};  // namespace XML
};  // namespace Divide

#endif
