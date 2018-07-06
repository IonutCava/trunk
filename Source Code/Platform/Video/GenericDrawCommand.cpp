#include "stdafx.h"

#include "Headers/GenericDrawCommand.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

namespace GenericDrawCommandResults {
    hashMapImpl<I64, QueryResult> g_queryResults;
};

IndirectDrawCommand::IndirectDrawCommand()
    : indexCount(0),
      primCount(1),
      firstIndex(0),
      baseVertex(0),
      baseInstance(0)
{
    static_assert(sizeof(IndirectDrawCommand) == 20, "Size of IndirectDrawCommand is incorrect!");
}

void IndirectDrawCommand::set(const IndirectDrawCommand& other) {
    indexCount = other.indexCount;
    primCount = other.primCount;
    firstIndex = other.firstIndex;
    baseVertex = other.baseVertex;
    baseInstance = other.baseInstance;
}

bool IndirectDrawCommand::operator==(const IndirectDrawCommand &other) const {
    return indexCount == other.indexCount &&
           primCount == other.primCount &&
           firstIndex == other.firstIndex &&
           baseVertex == other.baseVertex &&
           baseInstance == other.baseInstance;
}

GenericDrawCommand::GenericDrawCommand()
    : GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 0)
{
}

GenericDrawCommand::GenericDrawCommand(PrimitiveType type,
                                       U32 firstIndex,
                                       U32 indexCount,
                                       U32 primCount)
  : _lodIndex(0),
    _drawCount(1),
    _drawToBuffer(0),
    _type(type),
    _commandOffset(0),
    _patchVertexCount(4),
    _sourceBuffer(nullptr),
    _renderOptions(to_base(RenderOptions::RENDER_GEOMETRY))
{
    _cmd.indexCount = indexCount;
    _cmd.firstIndex = firstIndex;
    _cmd.primCount = primCount;

    static_assert(sizeof(GenericDrawCommand) == 56, "Size of GenericDrawCommand is incorrect!");
}

GenericDrawCommand::GenericDrawCommand(const GenericDrawCommand& other)
    : _cmd(other._cmd),
      _lodIndex(other._lodIndex),
      _drawCount(other._drawCount),
      _drawToBuffer(other._drawToBuffer),
      _renderOptions(other._renderOptions),
      _type(other._type),
      _sourceBuffer(other._sourceBuffer),
      _commandOffset(other._commandOffset),
      _patchVertexCount(other._patchVertexCount)
{
}

const GenericDrawCommand& GenericDrawCommand::operator= (const GenericDrawCommand& other) {
    _cmd.set(other._cmd);
    _lodIndex = other._lodIndex;
    _drawCount = other._drawCount;
    _drawToBuffer = other._drawToBuffer;
    _renderOptions = other._renderOptions;
    _type = other._type;
    _sourceBuffer = other._sourceBuffer;
    _commandOffset = other._commandOffset;
    _patchVertexCount = other._patchVertexCount;
    return *this;
}

void GenericDrawCommand::reset() {
    *this = GenericDrawCommand();
}

bool GenericDrawCommand::compatible(const GenericDrawCommand& other) const {
    return _lodIndex == other._lodIndex &&
           _drawToBuffer == other._drawToBuffer &&
           _renderOptions == other._renderOptions &&
           _type == other._type &&
           _patchVertexCount == other._patchVertexCount &&
           (_sourceBuffer != nullptr) == (other._sourceBuffer != nullptr);
}

bool GenericDrawCommand::isEnabledOption(RenderOptions option) const {
    return BitCompare(_renderOptions, to_U32(option));
}

void GenericDrawCommand::enableOption(RenderOptions option) {
    SetBit(_renderOptions, to_U32(option));
}

void GenericDrawCommand::disableOption(RenderOptions option) {
    ClearBit(_renderOptions, to_U32(option));
}

void GenericDrawCommand::toggleOption(RenderOptions option) {
    toggleOption(option, !isEnabledOption(option));
}

void GenericDrawCommand::toggleOption(RenderOptions option, const bool state) {
    if (state) {
        enableOption(option);
    } else {
        disableOption(option);
    }
}

}; //namespace Divide
