/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _PLATFORM_DEFINES_H_
#define _PLATFORM_DEFINES_H_

#include "config.h"
#include "Core/Headers/ErrorCodes.h"

#if defined(_DEBUG)
#define STUBBED(x)                                  \
do {                                                \
    static bool seen_this = false;                  \
    if (!seen_this) {                               \
        seen_this = true;                           \
        Console::errorfn("STUBBED: %s (%s : %d)\n", \
                         x, __FILE__, __LINE__);    \
    }                                               \
} while (0);

#else
#define STUBBED(x)
#endif

#ifndef TO_STRING
#define TO_STRING_NAME(X) #X
#define TO_STRING(X) TO_STRING_NAME(X)
#endif //TO_STRING

#define TYPEDEF_SMART_POINTERS_FOR_TYPE(T)       \
    using T ## _wptr  = std::weak_ptr<T>;        \
    using T ## _ptr   = std::shared_ptr<T>;      \
    using T ## _cwptr = std::weak_ptr<const T>;  \
    using T ## _cptr  = std::shared_ptr<const T>;


#define FWD_DECLARE_MANAGED_CLASS(T)      \
    class T;                              \
    TYPEDEF_SMART_POINTERS_FOR_TYPE(T);

#define FWD_DECLARE_MANAGED_STRUCT(T)     \
    struct T;                             \
    TYPEDEF_SMART_POINTERS_FOR_TYPE(T);


#if !defined(if_constexpr)
#if !defined(CPP_17_SUPPORT)
#warning "Constexpr if statement in non C++17 code. Consider updating language version for current project"
#define if_constexpr if
#else
#define if_constexpr if constexpr
#endif
#endif

 //ref: https://foonathan.net/2020/09/move-forward/
 // static_cast to rvalue reference
#define MOV(...) \
static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

// static_cast to identity
// The extra && aren't necessary as discussed above, but make it more robust in case it's used with a non-reference.
#define FWD(...) \
  static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

#define ALIAS_TEMPLATE_FUNCTION(highLevelF, lowLevelF) \
template<typename... Args> \
inline auto highLevelF(Args&&... args) -> decltype(lowLevelF(FWD(args)...)) \
{ \
    return lowLevelF(FWD(args)...); \
}

//ref: https://vittorioromeo.info/index/blog/passing_functions_to_functions.html
template <typename TSignature>
class function_view;

template <typename TReturn, typename... TArgs>
class function_view<TReturn(TArgs...)> final
{
    typedef TReturn signature_type(void*, TArgs...);

    void* _ptr;
    TReturn(*_erased_fn)(void*, TArgs...);

public:
    template <typename T, typename = std::enable_if_t <
        std::is_callable<T&(TArgs...)>{} &&
        !std::is_same<std::decay_t<T>, function_view>{} >>
        function_view(T&& x) noexcept : _ptr{ (void*)std::addressof(x) } {
        _erased_fn = [](void* ptr, TArgs... xs) -> TReturn {
            return (*reinterpret_cast<std::add_pointer_t<T>>(ptr))(
                FWD(xs)...);
        };
    }

    decltype(auto) operator()(TArgs... xs) const
        noexcept(noexcept(_erased_fn(_ptr, FWD(xs)...))) {
        return _erased_fn(_ptr, FWD(xs)...);
    }
};

namespace Divide {

constexpr bool DEBUG_ALL_NOP_CALLS = false;

using PlayerIndex = U8;

// FNV1a c++11 constexpr compile time hash functions, 32 and 64 bit
// str should be a null terminated string literal, value should be left out 
// e.g hash_32_fnv1a_const("example")
// code license: public domain or equivalent
// post: https://notes.underscorediscovery.com/constexpr-fnv1a/

constexpr U32 val_32_const = 0x811c9dc5;
constexpr U32 prime_32_const = 0x1000193;
constexpr U64 val_64_const = 0xcbf29ce484222325;
constexpr U64 prime_64_const = 0x100000001b3;

constexpr U64 _ID(const char* const str, const U64 value = val_64_const) noexcept {
    return (str[0] == '\0') ? value : _ID(&str[1], (value ^ U64(str[0])) * prime_64_const);
}

constexpr U64 _ID_VIEW(const char* const str, const size_t len, const U64 value = val_64_const) noexcept {
    return (len == 0) ? value : _ID_VIEW(&str[1], len - 1, (value ^ U64(str[0])) * prime_64_const);
}

constexpr U64 operator ""_id(const char* str, const size_t len) {
    return _ID_VIEW(str, len);
}

struct SysInfo {
    SysInfo();

    size_t _availableRam;
    int _systemResolutionWidth;
    int _systemResolutionHeight;
    FileAndPath _fileAndPath;
};

SysInfo& sysInfo() noexcept;
const SysInfo& const_sysInfo() noexcept;

void InitSysInfo(SysInfo& info, I32 argc, char** argv);

struct WindowHandle;
extern void GetWindowHandle(void* window, WindowHandle& handleOut) noexcept;

extern void SetThreadName(std::thread* thread, const char* threadName) noexcept;
extern void SetThreadName(const char* threadName) noexcept;

//ref: http://stackoverflow.com/questions/1528298/get-path-of-executable
extern FileAndPath GetExecutableLocation(I32 argc, char** argv);
FileAndPath GetExecutableLocation(char* argv0);
extern bool CallSystemCmd(const char* cmd, const char* args);

bool CreateDirectories(const char* path);
bool CreateDirectories(const ResourcePath& path);

void DebugBreak() noexcept;

ErrorCode PlatformInit(int argc, char** argv);
bool PlatformClose();
bool GetAvailableMemory(SysInfo& info);

ErrorCode PlatformPreInit(int argc, char** argv);
ErrorCode PlatformPostInit(int argc, char** argv);

ErrorCode PlatformInitImpl(int argc, char** argv) noexcept;
bool PlatformCloseImpl() noexcept;

const char* GetClipboardText(void* user_data) noexcept;
void SetClipboardText(void* user_data, const char* text) noexcept;

void ToggleCursor(bool state) noexcept;

bool CursorState() noexcept;

/// Converts an arbitrary positive integer value to a bitwise value used for masks
template<typename T>
constexpr T toBit(const T X) {
    return 1 << X;
}

constexpr U32 powerOfTwo(U32 X) noexcept {
    U32 r = 0;
    while (X >>= 1) {
        r++;
    }
    return r;
}

constexpr bool isPowerOfTwo(const U32 x) noexcept {
    return !(x == 0) && !(x & (x - 1));
}

constexpr size_t realign_offset(const size_t offset, const size_t align) noexcept {
    return (offset + align - 1) & ~(align - 1);
}

//ref: http://stackoverflow.com/questions/14226952/partitioning-batch-chunk-a-container-into-equal-sized-pieces-using-std-algorithm
template<typename Iterator, typename Pred>
void for_each_interval(Iterator from, Iterator to, std::ptrdiff_t partition_size, Pred&& operation) 
{
    if (partition_size > 0) {
        Iterator partition_end = from;
        while (partition_end != to) 
        {
            while (partition_end != to && std::distance(from, partition_end) < partition_size) 
            {
                ++partition_end;
            }
            operation(from, partition_end);
            from = partition_end;
        }
    }
}

//ref: http://stackoverflow.com/questions/9530928/checking-a-member-exists-possibly-in-a-base-class-c11-version
template< typename C, typename = void >
struct has_reserve
    : std::false_type
{};

template< typename C >
struct has_reserve< C, typename std::enable_if<
                                    std::is_same<
                                        decltype(std::declval<C>().reserve(std::declval<typename C::size_type>())),
                                        void
                                    >::value
                                >::type >
    : std::true_type
{};

template< typename C, typename = void >
struct has_emplace_back
    : std::false_type
{};

template< typename C >
struct has_emplace_back< C, typename std::enable_if<
                                        std::is_same<
                                            decltype(std::declval<C>().emplace_back(std::declval<typename C::value_type>())),
                                            void
                                        >::value
                                    >::type >
    : std::true_type
{};


template<typename>
static constexpr std::false_type has_assign(...) { 
    return std::false_type();
};

//ref: https://github.com/ParBLiSS/kmerind/blob/master/src/utils/container_traits.hpp
template<typename T>
static constexpr auto has_assign(T*) ->
decltype(std::declval<T>().assign(std::declval<decltype(std::declval<T>().begin())>(),
                                  std::declval<decltype(std::declval<T>().end())>()), std::true_type()) {
    return std::true_type();
};

template< typename C >
std::enable_if_t< !has_reserve< C >::value > optional_reserve(C&, std::size_t) {}

template< typename C >
std::enable_if_t< has_reserve< C >::value > optional_reserve(C& c, std::size_t n) {
    c.reserve(c.size() + n);
}


using std::make_index_sequence;
using std::index_sequence;

namespace detail
{
    template <typename T, std::size_t ... Is>
    constexpr std::array<T, sizeof...(Is)>
        create_array(T value, index_sequence<Is...>)
    {
        // cast Is to void to remove the warning: unused value
        return { {(static_cast<void>(Is), value)...} };
    }
}


template <std::size_t N, typename T>
constexpr std::array<T, N> create_array(const T& value)
{
    return detail::create_array(value, make_index_sequence<N>());
}

/* See
http://randomascii.wordpress.com/2012/01/11/tricks-with-the-floating-point-format/
for the potential portability problems with the union and bit-fields below.
*/
union Float_t {
    explicit Float_t(const F32 num = 0.0f) noexcept : f(num) {}

    // Portable extraction of components.
    [[nodiscard]] bool Negative() const noexcept { return (i >> 31) != 0; }
    [[nodiscard]] I32 RawMantissa() const noexcept { return i & ((1 << 23) - 1); }
    [[nodiscard]] I32 RawExponent() const noexcept { return (i >> 23) & 0xFF; }

    I32 i;
    F32 f;
};

union Double_t {
    explicit Double_t(const D64 num = 0.0) noexcept : d(num) {}

    // Portable extraction of components.
    [[nodiscard]] bool Negative() const noexcept { return (i >> 63) != 0; }
    [[nodiscard]] I64 RawMantissa() const noexcept { return i & ((1LL << 52) - 1); }
    [[nodiscard]] I64 RawExponent() const noexcept { return (i >> 52) & 0x7FF; }

    I64 i;
    D64 d;
};

inline bool AlmostEqualUlpsAndAbs(const F32 A, const F32 B, const F32 maxDiff, const I32 maxUlpsDiff) noexcept {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    const F32 absDiff = std::abs(A - B);
    if (absDiff <= maxDiff) {
        return true;
    }

    const Float_t uA(A);
    const Float_t uB(B);

    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative()) {
        return false;
    }

    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualUlpsAndAbs(const D64 A, const D64 B, const D64 maxDiff, const I32 maxUlpsDiff) noexcept {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    const D64 absDiff = std::abs(A - B);
    if (absDiff <= maxDiff) {
        return true;
    }

    const Double_t uA(A);
    const Double_t uB(B);

    // Different signs means they do not match.
    if (uA.Negative() != uB.Negative()) {
        return false;
    }

    // Find the difference in ULPs.
    return (std::abs(uA.i - uB.i) <= maxUlpsDiff);
}

inline bool AlmostEqualRelativeAndAbs(const F32 A, const F32 B, const F32 maxDiff, const F32 maxRelDiff)  noexcept {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    const F32 diff = std::abs(A - B);
    if (diff <= maxDiff) {
        return true;
    }

    const F32 largest = std::max(std::abs(A), std::abs(B));
    return (diff <= largest * maxRelDiff);
}

inline bool AlmostEqualRelativeAndAbs(D64 A, D64 B, const D64 maxDiff, const D64 maxRelDiff) noexcept {
    // Check if the numbers are really close -- needed when comparing numbers near zero.
    const D64 diff = std::abs(A - B);
    if (diff <= maxDiff) {
        return true;
    }

    A = std::abs(A);
    B = std::abs(B);
    const D64 largest = (B > A) ? B : A;

    return (diff <= largest * maxRelDiff);
}

constexpr void NOP() noexcept {
    if_constexpr (DEBUG_ALL_NOP_CALLS) {
        DebugBreak();
    }
}

#define ACKNOWLEDGE_UNUSED(p) ((void)(p))

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(str) \
    CONCATENATE(str, __COUNTER__)
#else
#define ANONYMOUSE_VARIABLE(str) \
    CONCATENATE(str, __LINE__)
#endif


// Multumesc Andrei A.!
#if defined(_MSC_VER)
#define _FUNCTION_NAME_AND_SIG_ __FUNCSIG__
#elif defined(__GNUC__)
#define _FUNCTION_NAME_AND_SIG_ __PRETTY_FUNCTION__
#else
#define _FUNCTION_NAME_AND_SIG_ __FUNCTION__
#endif

namespace detail {
    class ScopeGuardImplBase
    {
    public:
        void Dismiss() const noexcept {
            dismissed_ = true;
        }

        // Disable assignment
        ScopeGuardImplBase& operator=(const ScopeGuardImplBase&) = delete;
        ScopeGuardImplBase(ScopeGuardImplBase&& other) = delete;
        ScopeGuardImplBase& operator=(ScopeGuardImplBase && other) = delete;
    protected:
        ScopeGuardImplBase() noexcept = default;

        ScopeGuardImplBase(const ScopeGuardImplBase& other) noexcept
            : dismissed_(other.dismissed_)
        {
            other.Dismiss();
        }

        ~ScopeGuardImplBase() = default; // nonvirtual (see below why)
        mutable bool dismissed_ = false;

    };

    template <typename Fun, typename Parm>
    class ScopeGuardImpl1 final : public ScopeGuardImplBase
    {
    public:
        ScopeGuardImpl1(const Fun& fun, const Parm& parm)
            : fun_(fun), parm_(parm) 
        {
        }

        ~ScopeGuardImpl1()
        {
            if (!dismissed_) {
                fun_(parm_);
            }
        }

    private:
        Fun fun_;
        const Parm parm_;
    };

    template <typename Fun, typename Parm>
    ScopeGuardImpl1<Fun, Parm> MakeGuard(const Fun& fun, const Parm& parm) {
        return ScopeGuardImpl1<Fun, Parm>(fun, parm);
    }

    using ScopeGuard = const ScopeGuardImplBase&;

    enum class ScopeGuardOnExit {};

    template <typename Fun>
    ScopeGuard operator+(ScopeGuardOnExit, Fun&& fn) {
        return ScopeGuard<Fun>(FWD(fn));
    }
};

#define SCOPE_EXIT \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = detail::ScopeGuardOnExit() + [&]()

constexpr F32 EPSILON_F32 = std::numeric_limits<F32>::epsilon();
constexpr D64 EPSILON_D64 = std::numeric_limits<D64>::epsilon();

template <typename T, typename U = T>
bool IS_IN_RANGE_INCLUSIVE(const T x, const U min, const U max) noexcept {
    return x >= min && x <= max;
}
template <typename T, typename U = T>
bool IS_IN_RANGE_EXCLUSIVE(const T x, const U min, const U max) noexcept {
    return x > min && x < max;
}

template <typename T>
bool IS_ZERO(const T X) noexcept {
    return X == 0;
}

template <>
inline bool IS_ZERO(const F32 X) noexcept {
    return (abs(X) < EPSILON_F32);
}
template <>
inline bool IS_ZERO(const D64 X) noexcept {
    return (abs(X) < EPSILON_D64);
}

template <typename T>
bool IS_TOLERANCE(const T X, const T TOLERANCE) noexcept {
    return (abs(X) <= TOLERANCE);
}

template<typename T, typename U = T>
bool COMPARE_TOLERANCE(const T X, const U Y, const T TOLERANCE) noexcept {
    return abs(X - static_cast<T>(Y)) <= TOLERANCE;
}

template<typename T, typename U = T>
bool COMPARE_TOLERANCE_ACCURATE(const T X, const T Y, const T TOLERANCE) noexcept {
    return COMPARE_TOLERANCE(X, Y, TOLERANCE);
}

template<>
inline bool COMPARE_TOLERANCE_ACCURATE(const F32 X, const F32 Y, const F32 TOLERANCE) noexcept {
    return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4);
}

template<>
inline bool COMPARE_TOLERANCE_ACCURATE(const D64 X, const D64 Y, const D64 TOLERANCE) noexcept {
    return AlmostEqualUlpsAndAbs(X, Y, TOLERANCE, 4);
}

template<typename T, typename U = T>
bool COMPARE(T X, U Y) noexcept {
    return X == static_cast<T>(Y);
}

template<>
inline bool COMPARE(F32 X, F32 Y) noexcept {
    return COMPARE_TOLERANCE(X, Y, EPSILON_F32);
}

template<>
inline bool COMPARE(D64 X, D64 Y) noexcept {
    return COMPARE_TOLERANCE(X, Y, EPSILON_D64);
}

/// should be fast enough as the first condition is almost always true
template <typename T>
bool IS_GEQUAL(T X, T Y) noexcept {
    return X > Y || COMPARE(X, Y);
}
template <typename T>
bool IS_LEQUAL(T X, T Y) noexcept {
    return X < Y || COMPARE(X, Y);
}

///ref: http://blog.molecular-matters.com/2011/08/12/a-safer-static_cast/#more-120
// base template
template <bool IsFromSigned, bool IsToSigned>
struct safe_static_cast_helper;

// template specialization for casting from an unsigned type into an unsigned type
template <>
struct safe_static_cast_helper<false, false>
{
    template <typename TO, typename FROM>
    static TO cast(FROM from)
    {
        assert(IS_IN_RANGE_INCLUSIVE(std::is_enum<FROM>::value 
                                         ? static_cast<U32>(to_underlying_type(from))
                                         : from,
                                     std::numeric_limits<TO>::lowest(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");

        return static_cast<TO>(from);
    }
};

// template specialization for casting from an unsigned type into a signed type
template <>
struct safe_static_cast_helper<false, true>
{
    template <typename TO, typename FROM>
    static TO cast(FROM from)
    {
        assert(IS_IN_RANGE_INCLUSIVE(from,
                                     std::numeric_limits<TO>::lowest(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");

        return static_cast<TO>(from);
    }
};

// template specialization for casting from a signed type into an unsigned type
template <>
struct safe_static_cast_helper<true, false>
{
    template <typename TO, typename FROM>
    static TO cast(FROM from)
    {
        // make sure the input is not negative
        assert(from >= 0 && "Number to cast exceeds numeric limits.");

        // assuring a positive input, we can safely cast it into its unsigned type and check the numeric limits
        using UnsignedFrom = typename std::make_unsigned<FROM>::type;
        assert(IS_IN_RANGE_INCLUSIVE(static_cast<UnsignedFrom>(from),
                                     std::numeric_limits<TO>::lowest(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");
        return static_cast<TO>(from);
    }
};

// template specialization for casting from a signed type into a signed type
template <>
struct safe_static_cast_helper<true, true>
{
    template <typename TO, typename FROM>
    static TO cast(FROM from)
    {
        assert(IS_IN_RANGE_INCLUSIVE(std::is_enum<FROM>::value ? to_underlying_type(from) : from,
                                     std::numeric_limits<TO>::lowest(),
                                     std::numeric_limits<TO>::max()) &&
            "Number to cast exceeds numeric limits.");

        return static_cast<TO>(from);
    }
};


template <typename TO, typename FROM>
inline TO safe_static_cast(FROM from)
{
#if defined(_DEBUG)
    // delegate the call to the proper helper class, depending on the signedness of both types
    return safe_static_cast_helper<std::numeric_limits<FROM>::is_signed,
                                   std::numeric_limits<TO>::is_signed>
           ::template cast<TO>(from);
#else
    return static_cast<TO>(from);
#endif
}

template <typename TO>
TO safe_static_cast(F32 from)
{
    return static_cast<TO>(from);
}

template <typename TO>
TO safe_static_cast(D64 from)
{
    return static_cast<TO>(from);
} 

extern void DIVIDE_ASSERT_MSG_BOX(const char* failMessage) noexcept;

/// It is safe to call evaluate expressions and call functions inside the assert check as it will compile for every build type
FORCE_INLINE void DIVIDE_ASSERT(const bool expression, const char* failMessage = "UNEXPECTED CALL") noexcept {
    if_constexpr(!Config::Build::IS_RELEASE_BUILD) {
        if (!expression) {
            DIVIDE_ASSERT_MSG_BOX(failMessage);

            if_constexpr (!Config::Assert::CONTINUE_ON_ASSERT) {
                assert(expression && failMessage);
            }
        }
    } else {
        ACKNOWLEDGE_UNUSED(expression);
        ACKNOWLEDGE_UNUSED(failMessage);
    }
}

FORCE_INLINE void DIVIDE_UNEXPECTED_CALL(const char* failMessage = "UNEXPECTED CALL") noexcept {
    DIVIDE_ASSERT(false, failMessage);
    DebugBreak();
}

template <typename Ret, typename... Args >
using DELEGATE = std::function< Ret(Args...) >;

U32 HardwareThreadCount() noexcept;

template<typename T, typename U>
constexpr void assert_type(const U& ) {
    static_assert(std::is_same<U, T>::value, "value type not satisfied");
}
};  // namespace Divide

void* malloc_aligned(size_t size, size_t alignment);
void  malloc_free(void*& ptr);

void* operator new[](size_t size, const char* pName, Divide::I32 flags,
                     Divide::U32 debugFlags, const char* file,
                     Divide::I32 line);

void operator delete[](void* ptr, const char* pName, Divide::I32 flags,
                       Divide::U32 debugFlags, const char* file,
                       Divide::I32 line);

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, Divide::I32 flags,
                     Divide::U32 debugFlags, const char* file,
                     Divide::I32 line);

void operator delete[](void* ptr, size_t alignment, size_t alignmentOffset,
                       const char* pName, Divide::I32 flags,
                       Divide::U32 debugFlags, const char* file,
                       Divide::I32 line);

void* operator new(size_t size, const char* zFile, size_t nLine);
void operator delete(void* ptr, const char* zFile, size_t nLine);
void* operator new[](size_t size, const char* zFile, size_t nLine);
void operator delete[](void* ptr, const char* zFile, size_t nLine);

#if !defined(MemoryManager_NEW)
#define MemoryManager_NEW new (__FILE__, __LINE__)
#endif

namespace Divide {
namespace MemoryManager {

void log_delete(void* p);

template <typename T>
void SAFE_FREE(T*& ptr) {
    if (ptr != nullptr) {
        free(ptr);
        ptr = nullptr;
    }
}

/// Deletes and nullifies the specified pointer
template <typename T>
void DELETE(T*& ptr) {
    log_delete(ptr);
    delete ptr;
    ptr = nullptr;
}
  
/// Deletes and nullifies the specified pointer
template <typename T>
void SAFE_DELETE(T*& ptr) {
    if (ptr != nullptr) {
        DELETE(ptr);
    }
}

/// Deletes and nullifies the specified array pointer
template <typename T>
void DELETE_ARRAY(T*& ptr) {
    log_delete(ptr);

    delete[] ptr;
    ptr = nullptr;
}

/// Deletes and nullifies the specified array pointer
template <typename T>
void SAFE_DELETE_ARRAY(T*& ptr) {
    if (ptr != nullptr) {
        DELETE_ARRAY(ptr);
    }
}

#define SET_DELETE_FRIEND \
    template <typename T> \
    friend void MemoryManager::DELETE(T*& ptr); \

#define SET_SAFE_DELETE_FRIEND \
    SET_DELETE_FRIEND \
    template <typename T>      \
    friend void MemoryManager::SAFE_DELETE(T*& ptr);


#define SET_DELETE_ARRAY_FRIEND \
    template <typename T>       \
    friend void MemoryManager::DELETE_ARRAY(T*& ptr);

#define SET_SAFE_DELETE_ARRAY_FRIEND \
    SET_DELETE_ARRAY_FRIEND \
    template <typename T>            \
    friend void MemoryManager::DELETE_ARRAY(T*& ptr);

/// Deletes every element from the vector and clears it at the end
template <template <typename, typename> class Container,
    typename Value,
    typename Allocator = std::allocator<Value>>
void DELETE_CONTAINER(Container<Value*, Allocator>& container) {
    for (Value* iter : container) {
        log_delete(iter);
        delete iter;
    }

    container.clear();
}

#define SET_DELETE_CONTAINER_FRIEND \
    template <template <typename, typename> class Container, \
              typename Value, \
              typename Allocator> \
    friend void MemoryManager::DELETE_CONTAINER(Container<Value*, Allocator>& container);

/// Deletes every element from the map and clears it at the end
template <typename K, typename V, typename HashFun = hashAlg::hash<K> >
void DELETE_HASHMAP(hashMap<K, V, HashFun>& map) {
    if (!map.empty()) {
        for (typename hashMap<K, V, HashFun>::value_type iter : map) {
            log_delete(iter.second);
            delete iter.second;
        }
        map.clear();
    }
}
#define SET_DELETE_HASHMAP_FRIEND                       \
    template <typename K, typename V, typename HashFun> \
    friend void MemoryManager::DELETE_HASHMAP(hashMap<K, V, HashFun>& map);

/// Deletes the object pointed to by "OLD" and redirects that pointer to the
/// object pointed by "NEW"
/// "NEW" must be a derived (or same) class of OLD
template <typename Base, typename Derived>
void SAFE_UPDATE(Base*& OLD, Derived* const NEW) {
    static_assert(std::is_base_of<Base, Derived>::value,
                  "SAFE_UPDATE error: New must be a descendant of Old");
    SAFE_DELETE(OLD);
    OLD = NEW;
}
#define SET_SAFE_UPDATE_FRIEND                 \
    template <typename Base, typename Derived> \
    friend void MemoryManager::SAFE_UPDATE(Base*& OLD, Derived* const NEW);

};  // namespace MemoryManager


/// Wrapper that allows usage of atomic variables in containers
/// Copy is not atomic! (e.g. push/pop from containers is not threadsafe!)
/// ref: http://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
template <typename T>
struct AtomicWrapper : private NonMovable
{
    std::atomic<T> _a;

    AtomicWrapper() : _a() {}
    explicit AtomicWrapper(const std::atomic<T> &a) :_a(a.load()) {}
    AtomicWrapper(const AtomicWrapper &other) : _a(other._a.load()) { }
    ~AtomicWrapper() = default;

    AtomicWrapper &operator=(const AtomicWrapper &other) {
        _a.store(other._a.load());
        return *this;
    }

    AtomicWrapper &operator=(const T &value) {
        _a.store(value);
        return *this;
    }

    bool operator==(const T &value) const {
        return _a == value;
    }
};

};  // namespace Divide

#endif //_PLATFORM_DEFINES_H_