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

    BETTER_ENUM(ComponentType, U32,
        TRANSFORM = toBit(1),
        ANIMATION = toBit(2),
        INVERSE_KINEMATICS = toBit(3),
        RAGDOLL = toBit(4),
        NAVIGATION = toBit(5),
        BOUNDS = toBit(6),
        RENDERING = toBit(7),
        NETWORKING = toBit(8),
        UNIT = toBit(9),
        RIGID_BODY = toBit(10),
        SELECTION = toBit(11),
        DIRECTIONAL_LIGHT = toBit(12),
        POINT_LIGHT = toBit(13),
        SPOT_LIGHT = toBit(14),
        SCRIPT = toBit(15),
        COUNT = 15
    );

    enum class EditorComponentFieldType : U8 {
        PUSH_TYPE = 0,
        SLIDER_TYPE,
        SEPARATOR,
        BUTTON,
        DROPDOWN_TYPE,
        BOUNDING_BOX,
        BOUNDING_SPHERE,
        TRANSFORM,
        MATERIAL,
        COUNT
    };

    struct EditorComponentField
    {
        std::function<void(void*)> _dataGetter = {};
        std::function<void(const void*)> _dataSetter = {};
        Str128 _tooltip = "";
        void* _data = nullptr;
        vec2<F32> _range = { 0.0f, 0.0f }; //< Used by slider_type as a min / max range or dropdown as selected_index / count
        Str32  _name = "";
        F32 _step = 0.0f; //< 0.0f == no +- buttons
        const char* _format = "";
        GFX::PushConstantType _basicType = GFX::PushConstantType::COUNT;
        EditorComponentFieldType _type = EditorComponentFieldType::COUNT;
        // Use this to configure smaller data sizes for integers only (signed or unsigned) like:
        // U8: (PushConstantType::UINT, _byteCount=BYTE)
        // I16: (PushConstantType::INT, _byteCount=WORD)
        // etc
        // byteCount of 3 is currently NOT supported
        GFX::PushConstantSize _basicTypeSize = GFX::PushConstantSize::DWORD;

        bool _readOnly = false;
        bool _serialise = true;

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
        T get() const {
            if (_dataGetter) {
                T dataOut = {};
                _dataGetter(&dataOut);
                return dataOut;
            } 

            return *static_cast<T*>(_data);
        }

        template<typename T>
        void get(T& dataOut) const {
            if (_dataGetter) {
                _dataGetter(&dataOut);
            } else {
                dataOut = *static_cast<T*>(_data);
            }
        }

        template<typename T>
        void set(const T& dataIn) {
            if (_dataSetter) {
                _dataSetter(&dataIn);
            } else {
                *static_cast<T*>(_data) = dataIn;
            }
        }

        inline bool supportsByteCount() const noexcept {
            return _basicType == GFX::PushConstantType::INT ||
                   _basicType == GFX::PushConstantType::UINT ||
                   _basicType == GFX::PushConstantType::IVEC2 ||
                   _basicType == GFX::PushConstantType::IVEC3 ||
                   _basicType == GFX::PushConstantType::IVEC4 ||
                   _basicType == GFX::PushConstantType::UVEC2 ||
                   _basicType == GFX::PushConstantType::UVEC3 ||
                   _basicType == GFX::PushConstantType::UVEC4 ||
                   _basicType == GFX::PushConstantType::IMAT2 ||
                   _basicType == GFX::PushConstantType::IMAT3 ||
                   _basicType == GFX::PushConstantType::IMAT4 ||
                   _basicType == GFX::PushConstantType::UMAT2 ||
                   _basicType == GFX::PushConstantType::UMAT3 ||
                   _basicType == GFX::PushConstantType::UMAT4;
        }

        inline bool isMatrix() const noexcept {
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

        explicit EditorComponent(ComponentType parentComponentType, const Str128& name);
        explicit EditorComponent(const Str128& name);

        virtual ~EditorComponent() = default;

        inline void addHeader(const Str32& name) {
            EditorComponentField field = {};
            field._name = name;
            field._type = EditorComponentFieldType::PUSH_TYPE;
            field._readOnly = true;
            registerField(std::move(field));
        }

        void registerField(EditorComponentField&& field);

        inline vectorEASTL<EditorComponentField>& fields() noexcept { return _fields; }
        inline const vectorEASTL<EditorComponentField>& fields() const noexcept { return _fields; }

        inline void onChangedCbk(const DELEGATE<void, std::string_view>& cbk) { _onChangedCbk = cbk; }
        inline void onChangedCbk(DELEGATE<void, std::string_view>&& cbk) { _onChangedCbk = std::move(cbk); }

        bool saveCache(ByteBuffer& outputBuffer) const;
        bool loadCache(ByteBuffer& inputBuffer);

        PROPERTY_RW(Str128, name, "");
        PROPERTY_RW(ComponentType, parentComponentType, ComponentType::COUNT);

      protected:
        void onChanged(EditorComponentField& field) const;
        virtual void saveToXML(boost::property_tree::ptree& pt) const;
        virtual void loadFromXML(const boost::property_tree::ptree& pt);

        void saveFieldToXML(const EditorComponentField& field, boost::property_tree::ptree& pt) const;
        void loadFieldFromXML(EditorComponentField& field, const boost::property_tree::ptree& pt);

      protected:
        DELEGATE<void, std::string_view> _onChangedCbk;
        vectorEASTL<EditorComponentField> _fields;
    };

    namespace Attorney {
        class EditorComponentEditor {
          private:
            static vectorEASTL<EditorComponentField>& fields(EditorComponent& comp) noexcept {
                return comp._fields;
            }

            static const vectorEASTL<EditorComponentField>& fields(const EditorComponent& comp) noexcept {
                return comp._fields;
            }

            static void onChanged(const EditorComponent& comp, EditorComponentField& field) {
                comp.onChanged(field);
            }
            friend class Divide::PropertyWindow;
        };

        class EditorComponentSceneGraphNode {
           private:
            static bool saveCache(const EditorComponent& comp, ByteBuffer& outputBuffer) {
                return comp.saveCache(outputBuffer);
            }
            static bool loadCache(EditorComponent& comp, ByteBuffer& inputBuffer) {
                return comp.loadCache(inputBuffer);
            }

            static void saveToXML(const EditorComponent& comp, boost::property_tree::ptree& pt) {
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
