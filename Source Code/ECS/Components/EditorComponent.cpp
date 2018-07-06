#include "stdafx.h"

#include "Headers/EditorComponent.h"

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
}; //namespace Divide