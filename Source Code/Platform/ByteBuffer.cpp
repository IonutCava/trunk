#include "Headers/ByteBuffer.h"

namespace Divide {

void ByteBuffer::append(const U8 *src, size_t cnt) {
    if (src != nullptr && cnt > 0) {
        if (_storage.size() < _wpos + cnt) {
            _storage.resize(_wpos + cnt);
        }

        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
    }
}


bool ByteBuffer::dumpToFile(const stringImpl& fileName) const {
    return writeFile(fileName, _storage, FileType::BINARY);
}

bool ByteBuffer::loadFromFile(const stringImpl& fileName) {
    return readFile(fileName, _storage, FileType::BINARY);
}

};  // namespace Divide
