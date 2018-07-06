#include <ByteBuffer.h>
#include <assert.h>

void ByteBuffer::append(const U8 *src, size_t cnt){
    if (!cnt)	return;

    assert(size() < 10000000);

    if (_storage.size() < _wpos + cnt) _storage.resize(_wpos + cnt);

    memcpy(&_storage[_wpos], src, cnt);
    _wpos += cnt;
}