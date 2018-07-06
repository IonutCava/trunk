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

#ifndef _PLATFORM_DEFINES_UNIX_H_
#define _PLATFORM_DEFINES_UNIX_H_

//#pragma GCC diagnostic ignored "-Wall"

#ifndef NOINITVTABLE
#define NOINITVTABLE
#endif  //NOINITVTABLE

#ifndef THREAD_LOCAL
#define THREAD_LOCAL __thread
#endif  //THREAD_LOCAL

#ifndef FORCE_INLINE
#define FORCE_INLINE __attribute__((always_inline))
#endif //FORCE_INLINE

#include <sys/time.h>
#include <limits>
#include <X11/Xlib.h>

#ifdef None
#undef None
#endif //None

#ifdef Success
#undef Success
#endif //Success

namespace Divide {
    struct SysInfo {
        SysInfo() : _windowHandle(0),
                    _availableRam(0),
                    _systemResolutionWidth(0),
                    _systemResolutionHeight(0)
        {
        }

        Window _windowHandle;
        size_t _availableRam;
        int _systemResolutionWidth;
        int _systemResolutionHeight;
    };

typedef timeval TimeValue;
}; //namespace Divide


template<typename T>
inline bool isfinite(T val) {
	return std::isfinite(val);
}

// HACK FOR MISSING C++1y features:
namespace std {
// TEMPLATE FUNCTIONS cbegin AND cend
template<class _Container>
auto inline cbegin(const _Container& _Cont) -> decltype(::std::begin(_Cont))
{	// get beginning of sequence
    return (::std::begin(_Cont));
}

template<class _Container>
auto inline cend(const _Container& _Cont) -> decltype(::std::end(_Cont))
{	// get end of sequence
    return (::std::end(_Cont));
}

// TEMPLATE FUNCTIONS rbegin AND rend
template<class _Container>
auto inline rbegin(_Container& _Cont) -> decltype(_Cont.rbegin())
{	// get beginning of reversed sequence
    return (_Cont.rbegin());
}

template<class _Container>
auto inline rbegin(const _Container& _Cont) -> decltype(_Cont.rbegin())
{	// get beginning of reversed sequence
    return (_Cont.rbegin());
}

template<class _Container>
auto inline rend(_Container& _Cont) -> decltype(_Cont.rend())
{	// get end of reversed sequence
    return (_Cont.rend());
}

template<class _Container>
auto inline rend(const _Container& _Cont) -> decltype(_Cont.rend())
{	// get end of reversed sequence
    return (_Cont.rend());
}

template<class _Ty,
    size_t _Size> inline
    reverse_iterator<_Ty *> rbegin(_Ty(&_Array)[_Size])
{	// get beginning of reversed array
    return (reverse_iterator<_Ty *>(_Array + _Size));
}

template<class _Ty,
    size_t _Size> inline
    reverse_iterator<_Ty *> rend(_Ty(&_Array)[_Size])
{	// get end of reversed array
    return (reverse_iterator<_Ty *>(_Array));
}

template<class _Elem> inline
reverse_iterator<const _Elem *> rbegin(initializer_list<_Elem> _Ilist)
{	// get beginning of reversed sequence
    return (reverse_iterator<const _Elem *>(_Ilist.end()));
}

template<class _Elem> inline
reverse_iterator<const _Elem *> rend(initializer_list<_Elem> _Ilist)
{	// get end of reversed sequence
    return (reverse_iterator<const _Elem *>(_Ilist.begin()));
}

// TEMPLATE FUNCTIONS crbegin AND crend
template<class _Container>
auto inline crbegin(const _Container& _Cont) -> decltype(::std::rbegin(_Cont))
{	// get beginning of reversed sequence
    return (::std::rbegin(_Cont));
}

template<class _Container>
auto inline crend(const _Container& _Cont) -> decltype(::std::rend(_Cont))
{	// get end of reversed sequence
    return (::std::rend(_Cont));
}

}; //namespace std

#endif //_PLATFORM_DEFINES_UNIX_H_
