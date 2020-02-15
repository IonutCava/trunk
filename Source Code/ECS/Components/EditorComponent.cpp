#include "stdafx.h"

#include "Headers/EditorComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/Headers/Quaternion.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Material/Headers/Material.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {
    namespace {
        FORCE_INLINE stringImpl GetFullFieldName(const char* componentName, Str32 fieldName) {
            Util::ReplaceStringInPlace(fieldName, " ", "__");
            return Util::StringFormat("%s.%s", componentName, fieldName.c_str());
        }
    };

    EditorComponent::EditorComponent(const Str128& name)
        : GUIDWrapper(),
          _name(name)
    {
    }

    EditorComponent::~EditorComponent()
    {
    }

    void EditorComponent::registerField(EditorComponentField&& field) {
        _fields.erase(
            std::remove_if(std::begin(_fields), std::end(_fields),
                [&field](const EditorComponentField& it)
                -> bool { return it._name == field._name &&
                                 it._type == field._type; }),
            std::end(_fields));

        assert(field._basicTypeSize == GFX::PushConstantSize::DWORD || field.supportsByteCount());

        _fields.push_back(field);
    }

    void EditorComponent::onChanged(EditorComponentField& field) const {
        if (_onChangedCbk) {
            _onChangedCbk(field._name.c_str());
        }
    }


    // May be wrong endpoint
    bool EditorComponent::saveCache(ByteBuffer& outputBuffer) const {
        return true;
    }

    // May be wrong endpoint
    bool EditorComponent::loadCache(ByteBuffer& inputBuffer) {
        return true;
    }


    void EditorComponent::saveToXML(boost::property_tree::ptree& pt) const {
        pt.put(_name.c_str(), "");

        for (const EditorComponentField& field : _fields) {
            auto entryName = GetFullFieldName(_name.c_str(), field._name);
            if (!field._serialise) {
                continue;
            }

            switch(field._type) {
                case EditorComponentFieldType::PUSH_TYPE: {
                    saveFieldToXML(field, pt);
                } break;
                case EditorComponentFieldType::TRANSFORM: {
                    TransformComponent* transform = field.getPtr<TransformComponent>();

                    vec3<F32> scale = transform->getLocalScale();
                    vec3<F32> position = transform->getLocalPosition();

                    vec3<Angle::DEGREES<F32>> orientationEuler;
                    Quaternion<F32> orientation = transform->getLocalOrientation();
                    orientation.getEuler(orientationEuler);
                    orientationEuler = Angle::to_DEGREES(orientationEuler);

                    pt.put(entryName + ".position.<xmlattr>.x", position.x);
                    pt.put(entryName + ".position.<xmlattr>.y", position.y);
                    pt.put(entryName + ".position.<xmlattr>.z", position.z);

                    pt.put(entryName + ".orientation.<xmlattr>.x", orientationEuler.pitch);
                    pt.put(entryName + ".orientation.<xmlattr>.y", orientationEuler.yaw);
                    pt.put(entryName + ".orientation.<xmlattr>.z", orientationEuler.roll);

                    pt.put(entryName + ".scale.<xmlattr>.x", scale.x);
                    pt.put(entryName + ".scale.<xmlattr>.y", scale.y);
                    pt.put(entryName + ".scale.<xmlattr>.z", scale.z);
                }break;
                case EditorComponentFieldType::MATERIAL: {
                    field.getPtr<Material>()->saveToXML(entryName, pt);
                }break;
                default:
                case EditorComponentFieldType::BUTTON:
                case EditorComponentFieldType::BOUNDING_BOX:
                case EditorComponentFieldType::BOUNDING_SPHERE: {
                    //Skip
                }break;
            }
        }
    }

    void EditorComponent::loadFromXML(const boost::property_tree::ptree& pt) {
        if (!pt.get(_name.c_str(), "").empty()) {
            for (EditorComponentField& field : _fields) {
                auto entryName = GetFullFieldName(_name.c_str(), field._name);
                if (!field._serialise) {
                    continue;
                }

                switch (field._type) {
                    case EditorComponentFieldType::PUSH_TYPE: {
                        loadFieldFromXML(field, pt);
                    } break;
                    case EditorComponentFieldType::TRANSFORM: {
                        TransformComponent* transform = field.getPtr<TransformComponent>();

                        vec3<F32> scale;
                        vec3<F32> position;
                        vec3<Angle::DEGREES<F32>> orientationEuler;

                        position.set(pt.get<F32>(entryName + ".position.<xmlattr>.x", 0.0f),
                                     pt.get<F32>(entryName + ".position.<xmlattr>.y", 0.0f),
                                     pt.get<F32>(entryName + ".position.<xmlattr>.z", 0.0f));

                        orientationEuler.pitch = pt.get<F32>(entryName + ".orientation.<xmlattr>.x", 0.0f);
                        orientationEuler.yaw   = pt.get<F32>(entryName + ".orientation.<xmlattr>.y", 0.0f);
                        orientationEuler.roll  = pt.get<F32>(entryName + ".orientation.<xmlattr>.z", 0.0f);

                        scale.set(pt.get<F32>(entryName + ".scale.<xmlattr>.x", 1.0f),
                                  pt.get<F32>(entryName + ".scale.<xmlattr>.y", 1.0f),
                                  pt.get<F32>(entryName + ".scale.<xmlattr>.z", 1.0f));

                        Quaternion<F32> rotation;
                        rotation.fromEuler(orientationEuler);
                        transform->setScale(scale);
                        transform->setRotation(rotation);
                        transform->setPosition(position);
                    }break;
                    case EditorComponentFieldType::MATERIAL: {
                        Material* mat = field.getPtr<Material>();
                        mat->loadFromXML(entryName, pt);
                    }break;
                    default:
                    case EditorComponentFieldType::BUTTON:
                    case EditorComponentFieldType::BOUNDING_BOX:
                    case EditorComponentFieldType::BOUNDING_SPHERE: {
                        //Skip
                    }break;
                };
            }
        }
    }

    void EditorComponent::saveFieldToXML(const EditorComponentField& field, boost::property_tree::ptree& pt) const {
        auto entryName = GetFullFieldName(_name.c_str(), field._name);

        switch (field._basicType) {
            case GFX::PushConstantType::BOOL: {
                pt.put(entryName.c_str(), field.get<bool>());
            } break;
            case GFX::PushConstantType::INT: {
                pt.put(entryName.c_str(), field.get<I32>());
            } break;
            case GFX::PushConstantType::UINT: {
                pt.put(entryName.c_str(), field.get<U32>());
            } break;
            case GFX::PushConstantType::FLOAT: {
                pt.put(entryName.c_str(), field.get<F32>());
            } break;
            case GFX::PushConstantType::DOUBLE: {
                pt.put(entryName.c_str(), field.get<D64>());
            } break;
            case GFX::PushConstantType::IVEC2: {
                vec2<I32> data = {};
                field.get<vec2<I32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::IVEC3: {
                vec3<I32> data = {};
                field.get<vec3<I32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::IVEC4: {
                vec4<I32> data = {};
                field.get<vec4<I32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::UVEC2: {
                vec2<U32> data = {};
                field.get<vec2<U32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::UVEC3: {
                vec3<U32> data = {};
                field.get<vec3<U32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::UVEC4: {
                vec4<U32> data = {};
                field.get<vec4<U32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::VEC2: {
                vec2<F32> data = {};
                field.get<vec2<F32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::VEC3: {
                vec3<F32> data = {};
                field.get<vec3<F32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::VEC4: {
                vec4<F32> data = {};
                field.get<vec4<F32>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::DVEC2: {
                vec2<D64> data = {};
                field.get<vec2<D64>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::DVEC3: {
                vec3<D64> data = {};
                field.get<vec3<D64>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::DVEC4: {
                vec4<D64> data = {};
                field.get<vec4<D64>>(data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::IMAT2: {
                mat2<I32> data = {};
                field.get<mat2<I32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::IMAT3: {
                mat3<I32> data = {};
                field.get<mat3<I32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::IMAT4: {
                mat4<I32> data = {};
                field.get<mat4<I32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                pt.put((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                pt.put((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                pt.put((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                pt.put((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                pt.put((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
            } break;
            case GFX::PushConstantType::UMAT2: {
                mat2<U32> data = {};
                field.get<mat2<U32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::UMAT3: {
                mat3<U32> data = {};
                field.get<mat3<U32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::UMAT4: {
                mat4<U32> data = {};
                field.get<mat4<U32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                pt.put((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                pt.put((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                pt.put((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                pt.put((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                pt.put((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
            } break;
            case GFX::PushConstantType::MAT2: {
                mat2<F32> data = {};
                field.get<mat2<F32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::MAT3: {
                mat3<F32> data = {};
                field.get<mat3<F32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::MAT4: {
                mat4<F32> data = {};
                field.get<mat4<F32>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                pt.put((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                pt.put((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                pt.put((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                pt.put((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                pt.put((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
            } break;
            case GFX::PushConstantType::DMAT2: {
                mat2<D64> data = {};
                field.get<mat2<D64>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::DMAT3: {
                mat3<D64> data = {};
                field.get<mat3<D64>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::DMAT4: {
                mat4<D64> data = {};
                field.get<mat4<D64>>(data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                pt.put((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                pt.put((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                pt.put((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                pt.put((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                pt.put((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                pt.put((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                pt.put((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                pt.put((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                pt.put((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                pt.put((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                pt.put((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
            } break;
            case GFX::PushConstantType::FCOLOUR3: {
                FColour3 data = {};
                field.get<FColour3>(data);
                pt.put((entryName + ".<xmlattr>.r").c_str(), data.r);
                pt.put((entryName + ".<xmlattr>.g").c_str(), data.g);
                pt.put((entryName + ".<xmlattr>.b").c_str(), data.b);
            } break;
            case GFX::PushConstantType::FCOLOUR4: {
                FColour4 data = {};
                field.get<FColour4>(data);
                pt.put((entryName + ".<xmlattr>.r").c_str(), data.r);
                pt.put((entryName + ".<xmlattr>.g").c_str(), data.g);
                pt.put((entryName + ".<xmlattr>.b").c_str(), data.b);
                pt.put((entryName + ".<xmlattr>.a").c_str(), data.a);
            } break;
        }
    }

    void EditorComponent::loadFieldFromXML(EditorComponentField& field, const boost::property_tree::ptree& pt) {
        auto entryName = GetFullFieldName(_name.c_str(), field._name);

        switch (field._basicType) {
            case GFX::PushConstantType::BOOL:
            {
                bool val = pt.get(entryName.c_str(), field.get<bool>());
                field.set<bool>(val);
            } break;
            case GFX::PushConstantType::INT:
            {
               I32 val = pt.get(entryName.c_str(), field.get<I32>());
               field.set<I32>(val);
            } break;
            case GFX::PushConstantType::UINT:
            {
                U32 val = pt.get(entryName.c_str(), field.get<U32>());
                field.set<U32>(val);
            } break;
            case GFX::PushConstantType::FLOAT:
            {
                F32 val = pt.get(entryName.c_str(), field.get<F32>());
                field.set<F32>(val);
            } break;
            case GFX::PushConstantType::DOUBLE:
            {
                D64 val = pt.get(entryName.c_str(), field.get<D64>());
                field.set<D64>(val);
            } break;
            case GFX::PushConstantType::IVEC2:
            {
                vec2<I32> data = field.get<vec2<I32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
                field.set<vec2<I32>>(data);
            } break;
            case GFX::PushConstantType::IVEC3:
            {
                vec3<I32> data = field.get<vec3<I32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
                field.set<vec3<I32>>(data);
            } break;
            case GFX::PushConstantType::IVEC4:
            {
                vec4<I32> data = field.get<vec4<I32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
                field.set<vec4<I32>>(data);
            } break;
            case GFX::PushConstantType::UVEC2:
            {
                vec2<U32> data = field.get<vec2<U32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
                field.set<vec2<U32>>(data);
            } break;
            case GFX::PushConstantType::UVEC3:
            {
                vec3<U32> data = field.get<vec3<U32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
                field.set<vec3<U32>>(data);
            } break;
            case GFX::PushConstantType::UVEC4:
            {
                vec4<U32> data = field.get<vec4<U32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
                field.set<vec4<U32>>(data);
            } break;
            case GFX::PushConstantType::VEC2:
            {
                vec2<F32> data = field.get<vec2<F32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
                field.set<vec2<F32>>(data);
            } break;
            case GFX::PushConstantType::VEC3:
            {
                vec3<F32> data = field.get<vec3<F32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
                field.set<vec3<F32>>(data);
            } break;
            case GFX::PushConstantType::VEC4:
            {
                vec4<F32> data = field.get<vec4<F32>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
                field.set<vec4<F32>>(data);
            } break;
            case GFX::PushConstantType::DVEC2:
            {
                vec2<D64> data = field.get<vec2<D64>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
                field.set<vec2<D64>>(data);
            } break;
            case GFX::PushConstantType::DVEC3:
            {
                vec3<D64> data = field.get<vec3<D64>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
                field.set<vec3<D64>>(data);
            } break;
            case GFX::PushConstantType::DVEC4:
            {
                vec4<D64> data = field.get<vec4<D64>>();
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
                field.set<vec4<D64>>(data);
            } break;
            case GFX::PushConstantType::IMAT2:
            {
                mat2<I32> data = field.get<mat2<I32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                field.set<mat2<I32>>(data);
            } break;
            case GFX::PushConstantType::IMAT3:
            {
                mat3<I32> data = field.get<mat3<I32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                field.set<mat3<I32>>(data);
            } break;
            case GFX::PushConstantType::IMAT4:
            {
                mat4<I32> data = field.get<mat4<I32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[0][3] = pt.get((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[1][3] = pt.get((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                data.m[2][3] = pt.get((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                data.m[3][0] = pt.get((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                data.m[3][1] = pt.get((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                data.m[3][2] = pt.get((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                data.m[3][3] = pt.get((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
                field.set<mat4<I32>>(data);
            } break;
            case GFX::PushConstantType::UMAT2:
            {
                mat2<U32> data = field.get<mat2<U32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                field.set<mat2<U32>>(data);
            } break;
            case GFX::PushConstantType::UMAT3:
            {
                mat3<U32> data = field.get<mat3<U32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                field.set<mat3<U32>>(data);
            } break;
            case GFX::PushConstantType::UMAT4:
            {
                mat4<U32> data = field.get<mat4<U32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[0][3] = pt.get((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[1][3] = pt.get((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                data.m[2][3] = pt.get((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                data.m[3][0] = pt.get((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                data.m[3][1] = pt.get((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                data.m[3][2] = pt.get((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                data.m[3][3] = pt.get((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
                field.set<mat4<U32>>(data);
            } break;
            case GFX::PushConstantType::MAT2:
            {
                mat2<F32> data = field.get<mat2<F32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                field.set<mat2<F32>>(data);
            } break;
            case GFX::PushConstantType::MAT3:
            {
                mat3<F32> data = field.get<mat3<F32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                field.set<mat3<F32>>(data);
            } break;
            case GFX::PushConstantType::MAT4:
            {
                mat4<F32> data = field.get<mat4<F32>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[0][3] = pt.get((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[1][3] = pt.get((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                data.m[2][3] = pt.get((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                data.m[3][0] = pt.get((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                data.m[3][1] = pt.get((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                data.m[3][2] = pt.get((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                data.m[3][3] = pt.get((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
                field.set<mat4<F32>>(data);
            } break;
            case GFX::PushConstantType::DMAT2:
            {
                mat2<D64> data = field.get<mat2<D64>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                field.set<mat2<D64>>(data);
            } break;
            case GFX::PushConstantType::DMAT3:
            {
                mat3<D64> data = field.get<mat3<D64>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                field.set<mat3<D64>>(data);
            } break;
            case GFX::PushConstantType::DMAT4:
            {
                mat4<D64> data = field.get<mat4<D64>>();
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[0][3] = pt.get((entryName + ".<xmlattr>.03").c_str(), data.m[0][3]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[1][3] = pt.get((entryName + ".<xmlattr>.13").c_str(), data.m[1][3]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
                data.m[2][3] = pt.get((entryName + ".<xmlattr>.23").c_str(), data.m[2][3]);
                data.m[3][0] = pt.get((entryName + ".<xmlattr>.30").c_str(), data.m[3][0]);
                data.m[3][1] = pt.get((entryName + ".<xmlattr>.31").c_str(), data.m[3][1]);
                data.m[3][2] = pt.get((entryName + ".<xmlattr>.32").c_str(), data.m[3][2]);
                data.m[3][3] = pt.get((entryName + ".<xmlattr>.33").c_str(), data.m[3][3]);
                field.set<mat4<D64>>(data);
            } break;
            case GFX::PushConstantType::FCOLOUR3:
            {
                FColour3 data = field.get<FColour3>();
                data.set(pt.get((entryName + ".<xmlattr>.r").c_str(), data.r),
                         pt.get((entryName + ".<xmlattr>.g").c_str(), data.g),
                         pt.get((entryName + ".<xmlattr>.b").c_str(), data.b));
                field.set<FColour3>(data);
            } break;
            case GFX::PushConstantType::FCOLOUR4:
            {
                FColour4 data = field.get<FColour4>();
                data.set(pt.get((entryName + ".<xmlattr>.r").c_str(), data.r),
                         pt.get((entryName + ".<xmlattr>.g").c_str(), data.g),
                         pt.get((entryName + ".<xmlattr>.b").c_str(), data.b),
                         pt.get((entryName + ".<xmlattr>.a").c_str(), data.a));
                field.set<FColour4>(data);
            } break;
        }
    }
}; //namespace Divide