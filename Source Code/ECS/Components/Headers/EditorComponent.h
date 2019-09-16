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
#include "Core/Math/Headers/MathVectors.h"
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
        SLIDER_TYPE,
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
        vec2<F32> _range = { 0.0f, 1.0f }; //< Used only by slider_type
        F32 _step = 0.1f;

        std::function<void(void*)> _dataGetter = {};
        std::function<void(const void*)> _dataSetter = {};

        template<typename T>
        T* getPtr() const {
            if (_dataGetter) {
                T* ret = nullptr;
                _dataGetter(ret);
                return ret;
            }
            return static_cast<T*>(_data);
        }
        template<typename T>
        /*FORCE_INLINE */T get() const {
            if (_dataGetter) {
                T dataOut = {};
                _dataGetter(&dataOut);
                return dataOut;
            } 

            return *static_cast<T*>(_data);
        }

        template<typename T>
        /*FORCE_INLINE */void get(T& dataOut) const {
            if (_dataGetter) {
                _dataGetter(&dataOut);
            } else {
                dataOut = *static_cast<T*>(_data);
            }
        }

        template<typename T>
        /*FORCE_INLINE */void set(const T& dataIn) {
            if (_dataSetter) {
                _dataSetter(&dataIn);
            } else {
                *static_cast<T*>(_data) = dataIn;
            }
        }

        inline bool isMatrix() const {
            return _basicType == GFX::PushConstantType::IMAT2 ||
                   _basicType == GFX::PushConstantType::IMAT3 ||
                   _basicType == GFX::PushConstantType::IMAT4 ||
                   _basicType == GFX::PushConstantType::UMAT2 ||
                   _basicType == GFX::PushConstantType::UMAT3 ||
                   _basicType == GFX::PushConstantType::UMAT4 ||
                   _basicType == GFX::PushConstantType::DMAT2 ||
                   _basicType == GFX::PushConstantType::DMAT3 ||
                   _basicType == GFX::PushConstantType::DMAT4 ||
                   _basicType == GFX::PushConstantType::MAT2 ||
                   _basicType == GFX::PushConstantType::MAT3 ||
                   _basicType == GFX::PushConstantType::MAT4;
        }
    };

    class EditorComponent : public GUIDWrapper
    {
        friend class Attorney::EditorComponentEditor;
        friend class Attorney::EditorComponentSceneGraphNode;

      public:

        EditorComponent(const stringImpl& name);
        virtual ~EditorComponent();

        inline void name(const stringImpl& nameStr) { _name = nameStr; }
        inline const stringImpl& name() const { return _name; }

        inline void addHeader(const stringImpl& name) {
            registerField(name, nullptr, EditorComponentFieldType::PUSH_TYPE, true);
        }

        void registerField(const stringImpl& name, 
                           void* data,
                           EditorComponentFieldType type,
                           bool readOnly,
                           GFX::PushConstantType basicType = GFX::PushConstantType::COUNT,
                           const vec2<F32>& range = {0.0f, 1.0f},
                           F32 step = 1.0f);


        void registerField(const stringImpl& name,
                           std::function<void(void*)> dataGetter,
                           std::function<void(const void*)> dataSetter,
                           EditorComponentFieldType type,
                           bool readOnly,
                           GFX::PushConstantType basicType = GFX::PushConstantType::COUNT,
                           const vec2<F32>& range = {0.0f, 1.0f},
                           F32 step = 1.0f);

        inline vector<EditorComponentField>& fields() { return _fields; }
        inline const vector<EditorComponentField>& fields() const { return _fields; }

        inline void onChangedCbk(const DELEGATE_CBK<void, const char*> cbk) {
            _onChangedCbk = cbk;
        }

        bool saveCache(ByteBuffer& outputBuffer) const;
        bool loadCache(ByteBuffer& inputBuffer);

      protected:
        void onChanged(EditorComponentField& field);
        virtual void saveToXML(boost::property_tree::ptree& pt) const;
        virtual void loadFromXML(const boost::property_tree::ptree& pt);

        void saveFieldToXML(const EditorComponentField& field, boost::property_tree::ptree& pt) const;
        void loadFieldFromXML(EditorComponentField& field, const boost::property_tree::ptree& pt);

      protected:
        stringImpl _name;
        DELEGATE_CBK<void, const char*> _onChangedCbk;
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
            static bool saveCache(EditorComponent& comp, ByteBuffer& outputBuffer) {
                return comp.saveCache(outputBuffer);
            }
            static bool loadCache(EditorComponent& comp, ByteBuffer& inputBuffer) {
                return comp.loadCache(inputBuffer);
            }

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
