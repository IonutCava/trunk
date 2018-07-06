/* Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _SGN_EDITOR_COMPONENT_H_
#define _SGN_EDITOR_COMPONENT_H_

#include "Platform/Headers/PlatformDefines.h"

//Temp
#include "Platform/Video/Headers/PushConstant.h"

namespace Divide {
    class ByteBuffer;
    class SceneGraphNode;
    class PropertyWindow;

    namespace Attorney {
        class EditorComponentEditor;
        class EditorComponentSceneGraphNode;
    }; //namespace Attorney

    enum class EditorComponentFieldType : U8 {
        PUSH_TYPE = 0,
        BOUNDING_BOX,
        BOUNDING_SPHERE,
        TRANSFORM,
        MATERIAL,
        COUNT
    };

    struct EditorComponentField
    {
        GFX::PushConstantType _basicType = GFX::PushConstantType::COUNT;
        EditorComponentFieldType _type = EditorComponentFieldType::COUNT;
        bool _readOnly = false;
        stringImpl _name;
        void* _data = nullptr;
    };

    class EditorComponent : public GUIDWrapper
    {
        friend class Attorney::EditorComponentEditor;
        friend class Attorney::EditorComponentSceneGraphNode;

      public:

        EditorComponent(const stringImpl& name);
        virtual ~EditorComponent();

        inline const stringImpl& name() const { return _name; }

        inline void addHeader(const stringImpl& name) {
            registerField(name, nullptr, EditorComponentFieldType::PUSH_TYPE, true);
        }

        void registerField(const stringImpl& name, 
                           void* data,
                           EditorComponentFieldType type,
                           bool readOnly,
                           GFX::PushConstantType basicType = GFX::PushConstantType::COUNT);


        inline vector<EditorComponentField>& fields() { return _fields; }
        inline const vector<EditorComponentField>& fields() const { return _fields; }

        inline void onChangedCbk(const DELEGATE_CBK<void, EditorComponentField&>& cbk) {
            _onChangedCbk = cbk;
        }

        bool save(ByteBuffer& outputBuffer) const;
        bool load(ByteBuffer& inputBuffer);

      protected:
        void onChanged(EditorComponentField& field);
        virtual void saveToXML(boost::property_tree::ptree& pt) const;
        virtual void loadFromXML(const boost::property_tree::ptree& pt);

        void saveFieldToXML(const EditorComponentField& field, boost::property_tree::ptree& pt) const;
        void loadFieldFromXML(const EditorComponentField& field, const boost::property_tree::ptree& pt);

      protected:
        const stringImpl _name;
        DELEGATE_CBK<void, EditorComponentField&> _onChangedCbk;
        vector<EditorComponentField> _fields;
    };

    namespace Attorney {
        class EditorComponentEditor {
          private:
            static vector<EditorComponentField>& fields(EditorComponent& comp) {
                return comp._fields;
            }

            static const vector<EditorComponentField>& fields(const EditorComponent& comp) {
                return comp._fields;
            }

            static void onChanged(EditorComponent& comp, EditorComponentField& field) {
                comp.onChanged(field);
            }
            friend class Divide::PropertyWindow;
        };

        class EditorComponentSceneGraphNode {
           private:
            static void saveToXML(EditorComponent& comp, boost::property_tree::ptree& pt) {
                comp.saveToXML(pt);
            }
            static void loadFromXML(EditorComponent& comp, const boost::property_tree::ptree& pt) {
                comp.loadFromXML(pt);
            }

            friend class Divide::SceneGraphNode;
        };
    };  // namespace Attorney

};  // namespace Divide
#endif //_SGN_EDITOR_COMPONENT_H_
