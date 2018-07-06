/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _IM_EMULATION_H_
#define _IM_EMULATION_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

class Texture;
class ShaderProgram;
enum class PrimitiveType : U32;
/// IMPrimitive replaces immediate mode calls to VB based rendering
class NOINITVTABLE IMPrimitive : public GUIDWrapper, private NonCopyable {
   public:
    inline void setRenderStates(const DELEGATE_CBK<>& setupStatesCallback,
                                const DELEGATE_CBK<>& releaseStatesCallback) {
        _setupStates = setupStatesCallback;
        _resetStates = releaseStatesCallback;
    }

    inline void clearRenderStates() {
        _setupStates = nullptr;
        _resetStates = nullptr;
    }

    inline void setupStates() {
        if (_setupStates) {
            _setupStates();
        }
    }

    inline void resetStates() {
        if (_resetStates) {
            _resetStates();
        }
    }

    inline void drawShader(ShaderProgram* const shaderProgram) {
        _drawShader = shaderProgram;
    }

    virtual void render(bool forceWireframe = false, U32 instanceCount = 1) = 0;
    virtual void beginBatch(bool reserveBuffers, unsigned int vertexCount) = 0;

    virtual void begin(PrimitiveType type) = 0;
    virtual void vertex(F32 x, F32 y, F32 z) = 0;
    inline void vertex(const vec3<F32>& vert) {
        vertex(vert.x, vert.y, vert.z);
    }
    virtual void attribute1i(const stringImpl& attribName, I32 value) = 0;
    virtual void attribute4ub(const stringImpl& attribName, U8 x, U8 y, U8 z,
                              U8 w) = 0;
    virtual void attribute4f(const stringImpl& attribName, F32 x, F32 y, F32 z,
                             F32 w) = 0;
    inline void attribute4ub(const stringImpl& attribName,
                             const vec4<U8>& value) {
        attribute4ub(attribName, value.x, value.y, value.z, value.w);
    }
    inline void attribute4f(const stringImpl& attribName,
                            const vec4<F32>& value) {
        attribute4f(attribName, value.x, value.y, value.z, value.w);
    }

    virtual void end() = 0;
    virtual void endBatch() = 0;

    void clear();

    inline void paused(bool state) { _paused = state; }
    inline bool paused() const { return _paused; }
    inline void inUse(bool state) { _inUse = state; }
    inline bool inUse() const { return _inUse; }
    inline void zombieCounter(U8 count) { _zombieCounter = count; }
    inline U8   zombieCounter() const { return _zombieCounter; }
    inline void forceWireframe(bool state) { _forceWireframe = state; }
    inline bool forceWireframe() const { return _forceWireframe; }
    inline bool hasRenderStates() const {
        return (!_setupStates && !_resetStates);
    }

    /// State management
    inline size_t stateHash() const { return _stateHash; }
    inline void stateHash(size_t hashValue) { _stateHash = hashValue; }

    inline const mat4<F32>& worldMatrix() const { return _worldMatrix; }
    inline void worldMatrix(const mat4<F32>& worldMatrix) {
        _worldMatrix.set(worldMatrix);
    }
    inline void resetWorldMatrix() { _worldMatrix.identity(); }

    inline void name(const stringImpl& name) {
#       ifdef _DEBUG
        _name = name;
#       else
        ACKNOWLEDGE_UNUSED(name);
#       endif
    }
   protected:
    IMPrimitive();
#ifdef _DEBUG
    stringImpl _name;
#endif
   public:
    virtual ~IMPrimitive();

   public:

    ShaderProgram* _drawShader;
    Texture* _texture;
    bool _canZombify;

   protected:
    // render in wireframe mode
    bool _forceWireframe;

   private:
    U8   _zombieCounter;
    bool _inUse;
    /// If _pause is true, rendering for the current primitive is skipped and
    /// nothing is modified (e.g. zombie counters)
    bool _paused;
    /// 2 functions used to setup or reset states
    DELEGATE_CBK<> _setupStates;
    DELEGATE_CBK<> _resetStates;
    /// The state hash associated with this render instance
    size_t _stateHash;
    /// The transform matrix for this element
    mat4<F32> _worldMatrix;
};

};  // namespace Divide

#endif