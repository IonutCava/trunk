#include "Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

IMPrimitive::IMPrimitive(GFXDevice& context)
    : GraphicsResource(context),
      GUIDWrapper(),
      _inUse(true),
      _canZombify(true),
      _forceWireframe(false),
      _paused(true),
      _zombieCounter(0u),
      _texture(nullptr),
      _drawShader(nullptr),
      _stateHash(0)
{
}

IMPrimitive::~IMPrimitive() 
{
    clear();
}

void IMPrimitive::clear() {
    zombieCounter(0);
    stateHash(0);
    clearRenderStates();
    _worldMatrix.identity();
    _canZombify = true;
    _texture = nullptr;
    _drawShader = nullptr;
}

void IMPrimitive::fromBox(const vec3<F32>& min, const vec3<F32>& max, const vec4<U8>& colour) {
    paused(false);
    // Create the object
    beginBatch(true, 16, 1);
    // Set it's color
    attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(colour));
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
                             const vec4<U8>& colour) {
    U32 slices = 8, stacks = 8;
    F32 drho = to_float(M_PI) / stacks;
    F32 dtheta = 2.0f * to_float(M_PI) / slices;
    F32 ds = 1.0f / slices;
    F32 dt = 1.0f / stacks;
    F32 t = 1.0f;
    F32 s = 0.0f;
    U32 i, j;  // Looping variables
    paused(false);
    // Create the object
    beginBatch(true, stacks * ((slices + 1) * 2), 1);
    attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), Util::ToFloatColor(colour));
    begin(PrimitiveType::LINE_LOOP);
    for (i = 0; i < stacks; i++) {
        F32 rho = i * drho;
        F32 srho = std::sin(rho);
        F32 crho = std::cos(rho);
        F32 srhodrho = std::sin(rho + drho);
        F32 crhodrho = std::cos(rho + drho);
        s = 0.0f;
        for (j = 0; j <= slices; j++) {
            F32 theta = (j == slices) ? 0.0f : j * dtheta;
            F32 stheta = -std::sin(theta);
            F32 ctheta = std::cos(theta);

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

void IMPrimitive::fromLines(const vectorImpl<Line>& lines) {
    fromLines(lines, vec4<I32>(), false);
}

void IMPrimitive::fromLines(const vectorImpl<Line>& lines,
                            const vec4<I32>& viewport) {
    fromLines(lines, viewport, true);
}

void IMPrimitive::fromLines(const vectorImpl<Line>& lines,
                            const vec4<I32>& viewport,
                            const bool inViewport) {
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

    static const U16 indices[] = {
        0, 1, 2,
        3, 7, 1,
        5, 4, 7,
        6, 2, 4,
        0, 1
    };
    // Check if we have a valid list. The list can be programmatically
    // generated, so this check is required
    if (!lines.empty()) {
        vec4<F32> tempFloatColor;
        paused(false);
        // If we need to render it into a specific viewport, set the pre and post
        // draw functions to set up the
        // needed viewport rendering (e.g. axis lines)
        if (inViewport) {
            setRenderStates(
                [&, viewport]() {
                _context.setViewport(viewport);
            },
                [&]() {
                _context.restoreViewport();
            });
        }
        // Create the object containing all of the lines
        beginBatch(true, to_uint(lines.size()) * 2 * 14, 1);
        Util::ToFloatColor(lines[0]._colorStart, tempFloatColor);
        attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), tempFloatColor);
        // Set the mode to line rendering
        //primitive.begin(PrimitiveType::TRIANGLE_STRIP);
        begin(PrimitiveType::LINES);
        //vec3<F32> tempVertex;
        // Add every line in the list to the batch
        for (const Line& line : lines) {
            Util::ToFloatColor(line._colorStart, tempFloatColor);
            attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), tempFloatColor);
            /*for (U16 idx : indices) {
            tempVertex.set(line._startPoint * vertices[idx]);
            tempVertex *= line._widthStart;

            vertex(tempVertex);
            }*/
            vertex(line._startPoint);

            Util::ToFloatColor(line._colorEnd, tempFloatColor);
            attribute4f(to_const_uint(AttribLocation::VERTEX_COLOR), tempFloatColor);
            /*for (U16 idx : indices) {
            tempVertex.set(line._endPoint * vertices[idx]);
            tempVertex *= line._widthEnd;

            vertex(tempVertex);
            }*/

            vertex(line._endPoint);

        }
        end();
        // Finish our object
        endBatch();
    }
}
};