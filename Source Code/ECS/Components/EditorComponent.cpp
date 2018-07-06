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
                                        GFX::PushConstantType basicType) {
        _fields.push_back({
            basicType,
            type,
            name, 
            data
         });
    }
}; //namespace Divide