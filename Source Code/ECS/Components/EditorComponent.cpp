#include "stdafx.h"

#include "Headers/EditorComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/Headers/Quaternion.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {
    EditorComponent::EditorComponent(const stringImpl& name)
        : GUIDWrapper(),
          _name(name)
    {
    }

    EditorComponent::~EditorComponent()
    {
    }

    void EditorComponent::registerField(const stringImpl& name,
                                        void* data,
                                        EditorComponentFieldType type,
                                        bool readOnly,
                                        GFX::PushConstantType basicType) {
        _fields.push_back({
            basicType,
            type,
            readOnly,
            name, 
            data
         });
    }

    void EditorComponent::onChanged(EditorComponentField& field) {
        if (_onChangedCbk) {
            _onChangedCbk(field);
        }
    }


    // May be wrong endpoint
    bool EditorComponent::save(ByteBuffer& outputBuffer) const {
        return true;
    }

    // May be wrong endpoint
    bool EditorComponent::load(ByteBuffer& inputBuffer) {
        return true;
    }


    void EditorComponent::saveToXML(boost::property_tree::ptree& pt) const {
        pt.put(_name, "");

        for (const EditorComponentField& field : _fields) {
            stringImpl fieldName = field._name;
            Util::ReplaceStringInPlace(fieldName, " ", "__");
            stringImpl entryName = _name + "." + fieldName;

            switch(field._type) {
                case EditorComponentFieldType::PUSH_TYPE: {
                    saveFieldToXML(field, pt);
                } break;
                case EditorComponentFieldType::TRANSFORM: {
                    Transform* transform = static_cast<Transform*>(field._data);

                    vec3<F32> scale;
                    vec3<F32> position;
                    vec3<Angle::RADIANS<F32>> orientationEuler;
                    Quaternion<F32> orientation;

                    transform->getScale(scale);
                    transform->getPosition(position);
                    transform->getOrientation(orientation);
                    orientation.getEuler(orientationEuler);

                    pt.put(entryName + ".position.<xmlattr>.x", position.x);
                    pt.put(entryName + ".position.<xmlattr>.y", position.y);
                    pt.put(entryName + ".position.<xmlattr>.z", position.z);

                    pt.put(entryName + ".orientation.<xmlattr>.x", Angle::to_DEGREES(orientationEuler.z)); // <- these are swapped
                    pt.put(entryName + ".orientation.<xmlattr>.y", Angle::to_DEGREES(orientationEuler.y));
                    pt.put(entryName + ".orientation.<xmlattr>.z", Angle::to_DEGREES(orientationEuler.x)); // <- these are swapped

                    pt.put(entryName + ".scale.<xmlattr>.x", scale.x);
                    pt.put(entryName + ".scale.<xmlattr>.y", scale.y);
                    pt.put(entryName + ".scale.<xmlattr>.z", scale.z);
                }break;
                case EditorComponentFieldType::MATERIAL: {
                    Material* mat = static_cast<Material*>(field._data);
                    mat->saveToXML(entryName, pt);
                }break;
                default:
                case EditorComponentFieldType::BOUNDING_BOX:
                case EditorComponentFieldType::BOUNDING_SPHERE: {
                    //Skip
                }break;
            }
        }
    }

    void EditorComponent::loadFromXML(const boost::property_tree::ptree& pt) {
        if (!pt.get(_name, "").empty()) {
            for (const EditorComponentField& field : _fields) {
                stringImpl fieldName = field._name;
                Util::ReplaceStringInPlace(fieldName, " ", "__");

                stringImpl entryName = _name + "." + fieldName;

                switch (field._type) {
                    case EditorComponentFieldType::PUSH_TYPE: {
                        loadFieldFromXML(field, pt);
                    } break;
                    case EditorComponentFieldType::TRANSFORM: {
                        Transform* transform = static_cast<Transform*>(field._data);

                        vec3<F32> scale;
                        vec3<F32> position;
                        vec3<Angle::RADIANS<F32>> orientationEuler;

                        position.set(pt.get<F32>(entryName + ".position.<xmlattr>.x", 0.0f),
                                     pt.get<F32>(entryName + ".position.<xmlattr>.y", 0.0f),
                                     pt.get<F32>(entryName + ".position.<xmlattr>.z", 0.0f));

                        orientationEuler.set(pt.get<F32>(entryName + ".orientation.<xmlattr>.x", 0.0f),
                                             pt.get<F32>(entryName + ".orientation.<xmlattr>.y", 0.0f),
                                             pt.get<F32>(entryName + ".orientation.<xmlattr>.z", 0.0f));

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
                        Material* mat = static_cast<Material*>(field._data);
                        mat->loadFromXML(entryName, pt);
                    }break;
                    default:
                    case EditorComponentFieldType::BOUNDING_BOX:
                    case EditorComponentFieldType::BOUNDING_SPHERE: {
                        //Skip
                    }break;
                };
            }
        }
    }

    void EditorComponent::saveFieldToXML(const EditorComponentField& field, boost::property_tree::ptree& pt) const {
        stringImpl fieldName = field._name;
        Util::ReplaceStringInPlace(fieldName, " ", "__");

        stringImpl entryName = _name + "." + fieldName;
        switch (field._basicType) {
            case GFX::PushConstantType::BOOL: {
                pt.put(entryName.c_str(), *(bool*)field._data);
            } break;
            case GFX::PushConstantType::INT: {
                pt.put(entryName.c_str(), *(I32*)field._data);
            } break;
            case GFX::PushConstantType::UINT: {
                pt.put(entryName.c_str(), *(U32*)field._data);
            } break;
            case GFX::PushConstantType::FLOAT: {
                pt.put(entryName.c_str(), *(F32*)field._data);
            } break;
            case GFX::PushConstantType::DOUBLE: {
                pt.put(entryName.c_str(), *(D64*)field._data);
            } break;
            case GFX::PushConstantType::IVEC2: {
                const vec2<I32>& data = *(vec2<I32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::IVEC3: {
                const vec3<I32>& data = *(vec3<I32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::IVEC4: {
                const vec4<I32>& data = *(vec4<I32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::UVEC2: {
                const vec2<U32>& data = *(vec2<U32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::UVEC3: {
                const vec3<U32>& data = *(vec3<U32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::UVEC4: {
                const vec4<U32>& data = *(vec4<U32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::VEC2: {
                const vec2<F32>& data = *(vec2<F32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::VEC3: {
                const vec3<F32>& data = *(vec3<F32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::VEC4: {
                const vec4<F32>& data = *(vec4<F32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::DVEC2: {
                const vec2<D64>& data = *(vec2<D64>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
            } break;
            case GFX::PushConstantType::DVEC3: {
                const vec3<D64>& data = *(vec3<D64>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
            } break;
            case GFX::PushConstantType::DVEC4: {
                const vec4<D64>& data = *(vec4<D64>*)(field._data);
                pt.put((entryName + ".<xmlattr>.x").c_str(), data.x);
                pt.put((entryName + ".<xmlattr>.y").c_str(), data.y);
                pt.put((entryName + ".<xmlattr>.z").c_str(), data.z);
                pt.put((entryName + ".<xmlattr>.w").c_str(), data.w);
            } break;
            case GFX::PushConstantType::IMAT2: {
                const mat2<I32>& data = *(mat2<I32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::IMAT3: {
                const mat3<I32>& data = *(mat3<I32>*)(field._data);
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
                const mat4<I32>& data = *(mat4<I32>*)(field._data);
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
                const mat2<U32>& data = *(mat2<U32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::UMAT3: {
                const mat3<U32>& data = *(mat3<U32>*)(field._data);
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
                const mat4<U32>& data = *(mat4<U32>*)(field._data);
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
                const mat2<F32>& data = *(mat2<F32>*)(field._data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::MAT3: {
                const mat3<F32>& data = *(mat3<F32>*)(field._data);
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
                const mat4<F32>& data = *(mat4<F32>*)(field._data);
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
                const mat2<D64>& data = *(mat2<D64>*)(field._data);
                pt.put((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                pt.put((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                pt.put((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                pt.put((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::DMAT3: {
                const mat3<D64>& data = *(mat3<D64>*)(field._data);
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
                const mat4<D64>& data = *(mat4<D64>*)(field._data);
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
        }
    }

    void EditorComponent::loadFieldFromXML(const EditorComponentField& field, const boost::property_tree::ptree& pt) {
        stringImpl fieldName = field._name;
        Util::ReplaceStringInPlace(fieldName, " ", "__");
        stringImpl entryName = _name + "." + fieldName;

        switch (field._basicType) {
            case GFX::PushConstantType::BOOL:
            {
                *(bool*)field._data = pt.get(entryName.c_str(), *(bool*)field._data);
            } break;
            case GFX::PushConstantType::INT:
            {
                *(I32*)field._data = pt.get(entryName.c_str(), *(I32*)field._data);
            } break;
            case GFX::PushConstantType::UINT:
            {
                *(U32*)field._data = pt.get(entryName.c_str(), *(U32*)field._data);
            } break;
            case GFX::PushConstantType::FLOAT:
            {
                *(F32*)field._data = pt.get(entryName.c_str(), *(F32*)field._data);
            } break;
            case GFX::PushConstantType::DOUBLE:
            {
                *(D64*)field._data = pt.get(entryName.c_str(), *(D64*)field._data);
            } break;
            case GFX::PushConstantType::IVEC2:
            {
                vec2<I32>& data = *(vec2<I32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
            } break;
            case GFX::PushConstantType::IVEC3:
            {
                vec3<I32>& data = *(vec3<I32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
            } break;
            case GFX::PushConstantType::IVEC4:
            {
                vec4<I32>& data = *(vec4<I32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
            } break;
            case GFX::PushConstantType::UVEC2:
            {
                vec2<U32>& data = *(vec2<U32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
            } break;
            case GFX::PushConstantType::UVEC3:
            {
                vec3<U32>& data = *(vec3<U32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
            } break;
            case GFX::PushConstantType::UVEC4:
            {
                vec4<U32>& data = *(vec4<U32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
            } break;
            case GFX::PushConstantType::VEC2:
            {
                vec2<F32>& data = *(vec2<F32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
            } break;
            case GFX::PushConstantType::VEC3:
            {
                vec3<F32>& data = *(vec3<F32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
            } break;
            case GFX::PushConstantType::VEC4:
            {
                vec4<F32>& data = *(vec4<F32>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
            } break;
            case GFX::PushConstantType::DVEC2:
            {
                vec2<D64>& data = *(vec2<D64>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y));
            } break;
            case GFX::PushConstantType::DVEC3:
            {
                vec3<D64>& data = *(vec3<D64>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z));
            } break;
            case GFX::PushConstantType::DVEC4:
            {
                vec4<D64>& data = *(vec4<D64>*)(field._data);
                data.set(pt.get((entryName + ".<xmlattr>.x").c_str(), data.x),
                         pt.get((entryName + ".<xmlattr>.y").c_str(), data.y),
                         pt.get((entryName + ".<xmlattr>.z").c_str(), data.z),
                         pt.get((entryName + ".<xmlattr>.w").c_str(), data.w));
            } break;
            case GFX::PushConstantType::IMAT2:
            {
                mat2<I32>& data = *(mat2<I32>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::IMAT3:
            {
                mat3<I32>& data = *(mat3<I32>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::IMAT4:
            {
                mat4<I32>& data = *(mat4<I32>*)(field._data);
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
            } break;
            case GFX::PushConstantType::UMAT2:
            {
                mat2<U32>& data = *(mat2<U32>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::UMAT3:
            {
                mat3<U32>& data = *(mat3<U32>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::UMAT4:
            {
                mat4<U32>& data = *(mat4<U32>*)(field._data);
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
            } break;
            case GFX::PushConstantType::MAT2:
            {
                mat2<F32>& data = *(mat2<F32>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::MAT3:
            {
                mat3<F32>& data = *(mat3<F32>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::MAT4:
            {
                mat4<F32>& data = *(mat4<F32>*)(field._data);
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
            } break;
            case GFX::PushConstantType::DMAT2:
            {
                mat2<D64>& data = *(mat2<D64>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
            } break;
            case GFX::PushConstantType::DMAT3:
            {
                mat3<D64>& data = *(mat3<D64>*)(field._data);
                data.m[0][0] = pt.get((entryName + ".<xmlattr>.00").c_str(), data.m[0][0]);
                data.m[0][1] = pt.get((entryName + ".<xmlattr>.01").c_str(), data.m[0][1]);
                data.m[0][2] = pt.get((entryName + ".<xmlattr>.02").c_str(), data.m[0][2]);
                data.m[1][0] = pt.get((entryName + ".<xmlattr>.10").c_str(), data.m[1][0]);
                data.m[1][1] = pt.get((entryName + ".<xmlattr>.11").c_str(), data.m[1][1]);
                data.m[1][2] = pt.get((entryName + ".<xmlattr>.12").c_str(), data.m[1][2]);
                data.m[2][0] = pt.get((entryName + ".<xmlattr>.20").c_str(), data.m[2][0]);
                data.m[2][1] = pt.get((entryName + ".<xmlattr>.21").c_str(), data.m[2][1]);
                data.m[2][2] = pt.get((entryName + ".<xmlattr>.22").c_str(), data.m[2][2]);
            } break;
            case GFX::PushConstantType::DMAT4:
            {
                mat4<D64>& data = *(mat4<D64>*)(field._data);
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
            } break;
        }
    }
}; //namespace Divide