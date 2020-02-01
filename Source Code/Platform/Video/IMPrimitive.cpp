#include "stdafx.h"

#include "Headers/IMPrimitive.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

IMPrimitive::IMPrimitive(GFXDevice& context)
    : VertexDataInterface(context),
      _viewport(-1),
      _forceWireframe(false),
      _pipeline(nullptr),
      _texture(nullptr),
      _cmdBufferDirty(true)
{
    assert(handle()._id != 0);
    _cmdBuffer = GFX::allocateCommandBuffer();
}

IMPrimitive::~IMPrimitive() 
{
    GFX::deallocateCommandBuffer(_cmdBuffer);
}

void IMPrimitive::reset() {
    _worldMatrix.identity();
    _texture = nullptr;
    _descriptorSet._textureData.clear();
    _cmdBufferDirty = true;
    _viewport.set(-1);
    clearBatch();
}

void IMPrimitive::fromBox(const vec3<F32>& min, const vec3<F32>& max, const UColour4& colour) {
    // Create the object
    beginBatch(true, 16, 1);
    // Set it's colour
    attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(colour));
    // Draw the bottom loop
    begin(PrimitiveType::LINE_LOOP);
    vertex(min.x, min.y, min.z);
    vertex(max.x, min.y, min.z);
    vertex(max.x, min.y, max.z);
    vertex(min.x, min.y, max.z);
    end();
    // Draw the top loop
    begin(PrimitiveType::LINE_LOOP);
    vertex(min.x, max.y, min.z);
    vertex(max.x, max.y, min.z);
    vertex(max.x, max.y, max.z);
    vertex(min.x, max.y, max.z);
    end();
    // Connect the top to the bottom
    begin(PrimitiveType::LINES);
    vertex(min.x, min.y, min.z);
    vertex(min.x, max.y, min.z);
    vertex(max.x, min.y, min.z);
    vertex(max.x, max.y, min.z);
    vertex(max.x, min.y, max.z);
    vertex(max.x, max.y, max.z);
    vertex(min.x, min.y, max.z);
    vertex(min.x, max.y, max.z);
    end();
    // Finish our object
    endBatch();
}

void IMPrimitive::fromSphere(const vec3<F32>& center,
                             F32 radius,
                             const UColour4& colour) {
    constexpr U32 slices = 8, stacks = 8;
    constexpr F32 drho = to_F32(M_PI) / stacks;
    constexpr F32 dtheta = 2.0f * to_F32(M_PI) / slices;
    constexpr F32 ds = 1.0f / slices;
    constexpr F32 dt = 1.0f / stacks;

    F32 t = 1.0f;
    F32 s = 0.0f;
    U32 i, j;  // Looping variables
    // Create the object
    beginBatch(true, stacks * ((slices + 1) * 2), 1);
    attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(colour));
    begin(PrimitiveType::LINE_LOOP);
    for (i = 0; i < stacks; i++) {
        const F32 rho = i * drho;
        const F32 srho = std::sin(rho);
        const F32 crho = std::cos(rho);
        const F32 srhodrho = std::sin(rho + drho);
        const F32 crhodrho = std::cos(rho + drho);

        s = 0.0f;
        for (j = 0; j <= slices; j++) {
            const F32 theta = (j == slices) ? 0.0f : j * dtheta;
            const F32 stheta = -std::sin(theta);
            const F32 ctheta = std::cos(theta);

            F32 x = stheta * srho;
            F32 y = ctheta * srho;
            F32 z = crho;
            vertex((x * radius) + center.x,
                (y * radius) + center.y,
                (z * radius) + center.z);
            x = stheta * srhodrho;
            y = ctheta * srhodrho;
            z = crhodrho;
            s += ds;
            vertex((x * radius) + center.x,
                (y * radius) + center.y,
                (z * radius) + center.z);
        }
        t -= dt;
    }
    end();
    endBatch();
}

void IMPrimitive::fromLines(const vector<Line>& lines) {
    static const vec3<F32> vertices[] = {
        vec3<F32>(-1.0f, -1.0f,  1.0f),
        vec3<F32>(1.0f, -1.0f,  1.0f),
        vec3<F32>(-1.0f,  1.0f,  1.0f),
        vec3<F32>(1.0f,  1.0f,  1.0f),
        vec3<F32>(-1.0f, -1.0f, -1.0f),
        vec3<F32>(1.0f, -1.0f, -1.0f),
        vec3<F32>(-1.0f,  1.0f, -1.0f),
        vec3<F32>(1.0f,  1.0f, -1.0f)
    };

    constexpr U16 indices[] = {
        0, 1, 2,
        3, 7, 1,
        5, 4, 7,
        6, 2, 4,
        0, 1
    };
    // Check if we have a valid list. The list can be programmatically
    // generated, so this check is required
    if (!lines.empty()) {
        // Create the object containing all of the lines
        beginBatch(true, to_U32(lines.size()) * 2 * 14, 1);
        attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(lines[0].colourStart()));
        // Set the mode to line rendering
        //primitive.begin(PrimitiveType::TRIANGLE_STRIP);
        begin(PrimitiveType::LINES);
        //vec3<F32> tempVertex;
        // Add every line in the list to the batch
        for (const Line& line : lines) {
            attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(line.colourStart()));
            /*for (U16 idx : indices) {
            tempVertex.set(line._startPoint * vertices[idx]);
            tempVertex *= line._widthStart;

            vertex(tempVertex);
            }*/
            vertex(line.pointStart());

            attribute4f(to_base(AttribLocation::COLOR), Util::ToFloatColour(line.colourEnd()));
            /*for (U16 idx : indices) {
            tempVertex.set(line._endPoint * vertices[idx]);
            tempVertex *= line._widthEnd;

            vertex(tempVertex);
            }*/

            vertex(line.pointEnd());

        }
        end();
        // Finish our object
        endBatch();
    }
}

void IMPrimitive::pipeline(const Pipeline& pipeline) noexcept {
    _pipeline = &pipeline;
    _cmdBufferDirty = true;
}

void IMPrimitive::texture(const Texture& texture) {
    _texture = &texture;
    _descriptorSet._textureData.clear();
    _descriptorSet._textureData.setTexture(_texture->data(), to_U8(ShaderProgram::TextureUsage::UNIT0));
    _cmdBufferDirty = true;
}
};