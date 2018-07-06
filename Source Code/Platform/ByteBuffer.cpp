#include "Headers/ByteBuffer.h"
#include <cassert>
#include <fstream>

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
    std::ofstream outputBuffer(fileName.c_str(), std::ios::out | std::ios::binary);
    if (!outputBuffer.is_open()) {
        //return false;
    }
    outputBuffer.write((const char*)contents(), size());
    outputBuffer.close();
    return outputBuffer.good();
}

bool ByteBuffer::loadFromFile(const stringImpl& fileName) {
    std::ifstream fileBuffer(fileName.c_str(), std::ios::in | std::ios::binary);

    if (!fileBuffer.eof() && !fileBuffer.fail())
    {
        fileBuffer.seekg(0, std::ios_base::end);
        std::streampos fileSize = fileBuffer.tellg();
        fileBuffer.seekg(0, std::ios_base::beg);
        resize(fileSize);
        fileBuffer.read(reinterpret_cast<char*>(&_storage.front()), fileSize);
        fileBuffer.close();
        return fileBuffer.good();
    }

    return false;
}

};  // namespace Divide
