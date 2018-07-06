#include "Headers/ByteBuffer.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/StringHelper.h"

#include "Utility/Headers/Localization.h"

namespace Divide {

ByteBufferException::ByteBufferException(bool add, size_t pos, size_t esize, size_t size)
  : _add(add), _pos(pos), _esize(esize), _size(size)
{
    printPosError();
}

void ByteBufferException::printPosError() const {

    Console::errorfn(Locale::get(_ID("BYTE_BUFFER_ERROR")),
                     (_add ? "append" : "read"), 
                     _pos,
                     _esize,
                     _size);
}

ByteBuffer::ByteBuffer() : ByteBuffer(DEFAULT_SIZE)
{
}

// constructor
ByteBuffer::ByteBuffer(size_t res) : _rpos(0), _wpos(0)
{
    _storage.reserve(res);
}

// copy constructor
ByteBuffer::ByteBuffer(const ByteBuffer &buf)
    : _rpos(buf._rpos), _wpos(buf._wpos), _storage(buf._storage)
{
}

ByteBuffer& ByteBuffer::operator=(const ByteBuffer &buf) {
    _rpos = buf._rpos;
    _wpos = buf._rpos;
    _storage = buf._storage;

    return *this;
}

void ByteBuffer::clear() {
    _storage.clear();
    _rpos = _wpos = 0;
}

void ByteBuffer::append(const Byte *src, size_t cnt) {
    if (src != nullptr && cnt > 0) {
        if (_storage.size() < _wpos + cnt) {
            _storage.resize(_wpos + cnt);
        }

        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
    }
}


bool ByteBuffer::dumpToFile(const stringImpl& fileName) {
    if (_storage.back() != BUFFER_FORMAT_VERSION) {
        append(BUFFER_FORMAT_VERSION);
    }

    return writeFile(fileName, _storage.data(), _storage.size(), FileType::BINARY);
}

bool ByteBuffer::loadFromFile(const stringImpl& fileName) {
    if (readFile(fileName, _storage, FileType::BINARY)) {
        return _storage.back() == BUFFER_FORMAT_VERSION;
    }

    return false;
}

};  // namespace Divide
