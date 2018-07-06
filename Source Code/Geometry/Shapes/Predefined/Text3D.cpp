#include "stdafx.h"

#include "Headers/Text3D.h"

namespace Divide {

Text3D::Text3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& font)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::TEXT_3D, ObjectFlag::OBJECT_FLAG_NONE),
    _font(font),
    _height(16),
    _width(1.0f)
{
    /// Dummy
    getGeometryVB()->queueRefresh();
    setBoundsChanged();
}

void Text3D::setText(const stringImpl& text) {
    _text = text;
    setBoundsChanged();
}

void Text3D::setWidth(F32 width) {
    _width = width;
    setBoundsChanged();
}

void Text3D::updateBoundsInternal() {
    vec3<F32> min(-_width * 2, 0, -_width * 0.5f);
    vec3<F32> max(_width * 1.5f * _text.length() * 10, _width * _text.length() * 1.5f, _width * 0.5f);
    _boundingBox.set(min, max);
    Object3D::updateBoundsInternal();
}
}; //namespace Divide