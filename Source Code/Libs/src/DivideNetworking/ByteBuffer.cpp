#include <ByteBuffer.h>
#include <cassert>

namespace Divide {

void ByteBuffer::append(const U8 *src, size_t cnt){
    if (!cnt)    return;

    assert(size() < 10000000 && "Invalid ByteBuffer size");

    if (_storage.size() < _wpos + cnt) _storage.resize(_wpos + cnt);

    memcpy(&_storage[_wpos], src, cnt);
    _wpos += cnt;
}

}; //namespace Divide