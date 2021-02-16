#include "stdafx.h"

#include "Headers/ByteBuffer.h"

#include "Core/Headers/StringHelper.h"

#include "Utility/Headers/Localization.h"

#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

ByteBufferException::ByteBufferException(const bool add, const size_t pos, const size_t esize, const size_t size)
  : _add(add), _pos(pos), _esize(esize), _size(size)
{
    printPosError();
}

void ByteBufferException::printPosError() const {

    Console::errorfn(Locale::Get(_ID("BYTE_BUFFER_ERROR")),
                     _add ? "append" : "read", 
                     _pos,
                     _esize,
                     _size);
}

ByteBuffer::ByteBuffer() : ByteBuffer(DEFAULT_SIZE)
{
}

// constructor
ByteBuffer::ByteBuffer(const size_t res) : _rpos(0), _wpos(0)
{
    _storage.reserve(res);
}

void ByteBuffer::clear() noexcept {
    _storage.clear();
    _rpos = _wpos = 0;
}

void ByteBuffer::append(const Byte *src, const size_t cnt) {
    if (src != nullptr && cnt > 0) {
        if (_storage.size() < _wpos + cnt) {
            _storage.resize(_wpos + cnt);
        }

        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
    }
}


bool ByteBuffer::dumpToFile(const char* path, const char* fileName) {
    if (!_storage.empty() && _storage.back() != BUFFER_FORMAT_VERSION) {
        append(BUFFER_FORMAT_VERSION);
    }

    return writeFile(path, fileName, _storage.data(), _storage.size(), FileType::BINARY) == FileError::NONE;
}

bool ByteBuffer::loadFromFile(const char* path, const char* fileName) {
    clear();
    if (readFile(path, fileName, _storage, FileType::BINARY) == FileError::NONE) {
        return _storage.back() == BUFFER_FORMAT_VERSION;
    }

    return false;
}

}  // namespace Divide
