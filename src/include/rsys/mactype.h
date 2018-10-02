#if !defined(_MACTYPE_H_)
#define _MACTYPE_H_

/*
 * Copyright 1986, 1989, 1990, 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */
#include "host-arch-config.h"
#include <stdint.h>
#include <cstring>
#include <type_traits>
#include <syn68k_public.h>

#include "rsys/functions.h"
#include "rsys/traps.h"

#ifndef __cplusplus
#error C++ required
#endif

namespace Executor
{

typedef int16_t INTEGER;
typedef int32_t LONGINT;
typedef uint32_t ULONGINT;

#if !defined(WIN32) || !defined(USE_WINDOWS_NOT_MAC_TYPEDEFS_AND_DEFINES)
typedef int8_t BOOLEAN;
#endif

typedef int16_t CharParameter; /* very important not to use this as char */

struct Point
{
    int16_t v;
    int16_t h;
};


// Define alignment.

// Most things are two-byte aligned.
template<typename T>
struct Aligner
{
    uint16_t align;
};

// Except for chars
template<>
struct Aligner<char>
{
    uint8_t align;
};
template<>
struct Aligner<unsigned char>
{
    uint8_t align;
};
template<>
struct Aligner<signed char>
{
    uint8_t align;
};

#if defined(BIGENDIAN)
template<typename T>
T SwapTyped(T x) { return x; }

inline Point SwapPoint(Point x) { return x; }

#else
inline unsigned char SwapTyped(unsigned char x)
{
    return x;
}
inline signed char SwapTyped(signed char x) { return x; }
inline char SwapTyped(char x) { return x; }

inline uint16_t SwapTyped(uint16_t x) { return swap16(x); }
inline int16_t SwapTyped(int16_t x) { return swap16((uint16_t)x); }

inline uint32_t SwapTyped(uint32_t x) { return swap32(x); }
inline int32_t SwapTyped(int32_t x) { return swap32((uint32_t)x); }

inline uint64_t SwapTyped(uint64_t x) { return swap64(x); }
inline int64_t SwapTyped(int64_t x) { return swap64((uint64_t)x); }

inline Point SwapPoint(Point x) { return Point { SwapTyped(x.v), SwapTyped(x.h) }; }
#endif

inline uint16_t *SYN68K_TO_US_CHECK0_CHECKNEG1(syn68k_addr_t addr)
{
    if(addr == (syn68k_addr_t)-1)
        return (uint16_t*) -1;
    else
        return SYN68K_TO_US_CHECK0(addr);
}

inline syn68k_addr_t US_TO_SYN68K_CHECK0_CHECKNEG1(const void* addr)
{
    if(addr == (void*)-1)
        return (syn68k_addr_t) -1;
    else
        return US_TO_SYN68K_CHECK0(addr);
}

template<typename T, typename... Args>
inline syn68k_addr_t US_TO_SYN68K_CHECK0_CHECKNEG1(T (*addr)(Args...))
{
    return US_TO_SYN68K_CHECK0_CHECKNEG1((const void*) addr);
}


template<typename T, typename = void>
struct GuestTypeTraits;

template<typename T>
struct GuestTypeTraits<T, std::enable_if_t<std::is_integral_v<T>>>
{
    using HostType = T;
    using GuestType = T;

    static GuestType host_to_guest(HostType x) { return SwapTyped(x); }
    static HostType guest_to_host(GuestType x) { return SwapTyped(x); }

    static std::enable_if_t<sizeof(T) <= 4, uint32_t> host_to_reg(HostType x) { return (uint32_t)x; }
    static HostType reg_to_host(std::enable_if_t<sizeof(T) <= 4, uint32_t> x) { return (HostType)x; }
};

template<typename T>
struct GuestTypeTraits<T*>
{
    using HostType = T*;
    using GuestType = uint32_t;

    static GuestType host_to_guest(HostType x) { return SwapTyped(US_TO_SYN68K_CHECK0_CHECKNEG1(x)); }
    static HostType guest_to_host(GuestType x) { return (HostType)SYN68K_TO_US_CHECK0_CHECKNEG1(SwapTyped(x)); }

    static uint32_t host_to_reg(HostType x) { return US_TO_SYN68K_CHECK0_CHECKNEG1(x); }
    static HostType reg_to_host(uint32_t x) { return (HostType)SYN68K_TO_US_CHECK0_CHECKNEG1(x); }
};

template<typename T>
struct GuestTypeTraits<UPP<T>>
{
    using HostType = UPP<T>;
    using GuestType = uint32_t;

    static GuestType host_to_guest(HostType x) { return SwapTyped(US_TO_SYN68K_CHECK0_CHECKNEG1((void*)x)); }
    static HostType guest_to_host(GuestType x) { return (HostType)SYN68K_TO_US_CHECK0_CHECKNEG1(SwapTyped(x)); }

    static uint32_t host_to_reg(HostType x) { return US_TO_SYN68K_CHECK0_CHECKNEG1((void*)x); }
    static HostType reg_to_host(uint32_t x) { return (HostType)SYN68K_TO_US_CHECK0_CHECKNEG1(x); }
};

template<typename To, typename From>
To reinterpret_bits_cast(From x)
{
    To y;
    static_assert(sizeof(x) == sizeof(y), "illegal reinterpret_bits_cast");
    std::memcpy(&y, &x, sizeof(x));
    return y;
}

template<>
struct GuestTypeTraits<float>
{
    using HostType = float;
    using GuestType = uint32_t;

    static GuestType host_to_guest(HostType x) { return SwapTyped(reinterpret_bits_cast<GuestType>(x)); }
    static HostType guest_to_host(GuestType x) { return reinterpret_bits_cast<HostType>(SwapTyped(x)); }

    static uint32_t host_to_reg(HostType x) { return reinterpret_bits_cast<GuestType>(x); }
    static HostType reg_to_host(uint32_t x) { return (HostType)reinterpret_bits_cast<HostType>(x); }
};

template<>
struct GuestTypeTraits<double>
{
    using HostType = double;
    using GuestType = uint64_t;

    static GuestType host_to_guest(HostType x) { return SwapTyped(reinterpret_bits_cast<GuestType>(x)); }
    static HostType guest_to_host(GuestType x) { return reinterpret_bits_cast<HostType>(SwapTyped(x)); }
};
template<>
struct GuestTypeTraits<Point>
{
    using HostType = Point;
    using GuestType = uint32_t;

    static GuestType host_to_guest(HostType x) { return reinterpret_bits_cast<uint32_t>(SwapPoint(x)); }
    static HostType guest_to_host(GuestType x) { return SwapPoint(reinterpret_bits_cast<Point>(x)); }
    
    static uint32_t host_to_reg(HostType x) { return ((uint32_t)x.v << 16) | (x.h & 0xFFFFU); }
    static HostType reg_to_host(uint32_t x) { return Point{ int16_t(x >> 16), int16_t(x & 0xFFFF) }; }
};


template<typename ActualType>
union HiddenValue
{
    uint8_t data[sizeof(ActualType)];
    Aligner<ActualType> align;
public:
    HiddenValue() = default;
    HiddenValue(const HiddenValue<ActualType> &y) = default;
    HiddenValue<ActualType> &operator=(const HiddenValue<ActualType> &y) = default;

    ActualType raw() const
    {
        return *(const ActualType *)data;
    }

    void raw(ActualType x)
    {
        *(ActualType *)data = x;
    }
};

template<typename TT>
struct GuestWrapper;

template<typename TT>
struct GuestWrapperBase
{
    HiddenValue<TT> hidden;

    using WrappedType = TT;
    using RawGuestType = TT;

    WrappedType get() const
    {
        return SwapTyped(hidden.raw());
    }

    void set(WrappedType x)
    {
        hidden.raw(SwapTyped(x));
    }

    RawGuestType raw() const
    {
        return hidden.raw();
    }

    void raw(RawGuestType x)
    {
        hidden.raw(x);
    }

    uint32_t raw_host_order() const
    {
        return get();
    }
    void raw_host_order(uint32_t x)
    {
        return set(x);
    }


    void raw_and(RawGuestType x)
    {
        hidden.raw(hidden.raw() & x);
    }

    void raw_or(RawGuestType x)
    {
        hidden.raw(hidden.raw() | x);
    }

    void raw_and(GuestWrapper<TT> x)
    {
        hidden.raw(hidden.raw() & x.raw());
    }

    void raw_or(GuestWrapper<TT> x)
    {
        hidden.raw(hidden.raw() | x.raw());
    }

    GuestWrapper<TT> operator~() const;
};

template<typename TT>
struct GuestWrapperBase<TT *>
{
private:
    HiddenValue<uint32_t> p;

public:
    using WrappedType = TT *;
    using RawGuestType = uint32_t;

    WrappedType get() const
    {
        uint32_t rawp = this->raw();
        return (TT *)(SYN68K_TO_US_CHECK0_CHECKNEG1((uint32_t)swap32(rawp)));
    }

    void set(TT *ptr)
    {
        this->raw(swap32(US_TO_SYN68K_CHECK0_CHECKNEG1(ptr)));
    }

    RawGuestType raw() const
    {
        return p.raw();
    }

    void raw(RawGuestType x)
    {
        p.raw(x);
    }

    uint32_t raw_host_order() const
    {
        return swap32(this->raw());
    }
    void raw_host_order(uint32_t x)
    {
        this->raw(swap32(x));
    }

};

//#define AUTOMATIC_CONVERSIONS

template<typename TT>
struct GuestWrapper : GuestWrapperBase<TT>
{
    GuestWrapper() = default;
    GuestWrapper(const GuestWrapper<TT> &y) = default;
    GuestWrapper<TT> &operator=(const GuestWrapper<TT> &y) = default;

    template<typename T2, typename = typename std::enable_if<std::is_convertible<T2, TT>::value && sizeof(TT) == sizeof(T2)>::type>
    GuestWrapper(const GuestWrapper<T2> &y)
    {
        this->raw(y.raw());
    }

    template<typename T2, typename = typename std::enable_if<std::is_convertible<T2, TT>::value && sizeof(TT) == sizeof(T2)>::type>
    GuestWrapper<TT> &operator=(const GuestWrapper<T2> &y)
    {
        this->raw(y.raw());
        return *this;
    }

#ifdef AUTOMATIC_CONVERSIONS
    GuestWrapper(const TT &y)
    {
        this->set(y);
    }

    GuestWrapper<TT> &operator=(const TT &y)
    {
        this->set(y);
        return *this;
    }

    operator TT() const { return this->get(); }
#else
    GuestWrapper(std::nullptr_t)
    {
        this->raw(0);
    }
#endif

    static GuestWrapper<TT> fromRaw(typename GuestWrapper<TT>::RawGuestType r)
    {
        GuestWrapper<TT> w;
        w.raw(r);
        return w;
    }
    static GuestWrapper<TT> fromHost(typename GuestWrapper<TT>::WrappedType x)
    {
        GuestWrapper<TT> w;
        w.set(x);
        return w;
    }

    explicit operator bool()
    {
        return this->raw() != 0;
    }

    template<typename T2,
             typename compatible = decltype(TT() | T2())>
    //typename sizematch = typename std::enable_if<sizeof(TT) == sizeof(T2)>::type>
    GuestWrapper<TT> &operator|=(GuestWrapper<T2> x)
    {
        this->raw_or(x.raw());
        return *this;
    }

    template<typename T2,
             typename compatible = decltype(TT() & T2())>
    //typename sizematch = typename std::enable_if<sizeof(TT) == sizeof(T2)>::type>
    GuestWrapper<TT> &operator&=(GuestWrapper<T2> x)
    {
        this->raw_and(x.raw());
        return *this;
    }
};

template<typename TT>
inline GuestWrapper<TT> GuestWrapperBase<TT>::operator~() const
{
    return GuestWrapper<TT>::fromRaw(~this->raw());
}

template<typename T1, typename T2,
         typename result = decltype(T1() == T2()),
         typename enable = typename std::enable_if<sizeof(T1) == sizeof(T2)>::type>
bool operator==(GuestWrapper<T1> a, GuestWrapper<T2> b)
{
    return a.raw() == b.raw();
}

template<typename T1, typename T2,
         typename result = decltype(T1() == T2()),
         typename enable = typename std::enable_if<sizeof(T1) == sizeof(T2)>::type>
bool operator!=(GuestWrapper<T1> a, GuestWrapper<T2> b)
{
    return a.raw() != b.raw();
}

template<typename T1, typename T2,
         typename result = decltype(T1() & T2()),
         typename enable = typename std::enable_if<sizeof(T1) == sizeof(T2)>::type>
GuestWrapper<T1> operator&(GuestWrapper<T1> a, GuestWrapper<T2> b)
{
    return GuestWrapper<T1>::fromRaw(a.raw() & b.raw());
}
template<typename T1, typename T2,
         typename result = decltype(T1() | T2()),
         typename enable = typename std::enable_if<sizeof(T1) == sizeof(T2)>::type>
GuestWrapper<T1> operator|(GuestWrapper<T1> a, GuestWrapper<T2> b)
{
    return GuestWrapper<T1>::fromRaw(a.raw() | b.raw());
}

template<typename TT>
bool operator==(GuestWrapper<TT *> a, std::nullptr_t)
{
    return !a;
}
template<typename TT>
bool operator!=(GuestWrapper<TT *> a, std::nullptr_t)
{
    return a;
}

#define GUEST_STRUCT    struct is_guest_struct {}

template<>
struct GuestWrapper<Point>
{
    GUEST_STRUCT;
    GuestWrapper<INTEGER> v;
    GuestWrapper<INTEGER> h;

    using WrappedType = Point;
    using RawGuestType = Point;

    Point get() const
    {
        return Point{ v.get(), h.get() };
    }

    void set(Point x)
    {
        v.set(x.v);
        h.set(x.h);
    }

    Point raw() const
    {
        return Point{ v.raw(), h.raw() };
    }

    void raw(Point x)
    {
        v.raw(x.v);
        h.raw(x.h);
    }

    static GuestWrapper<Point> fromHost(Point x)
    {
        GuestWrapper<Point> w;
        w.set(x);
        return w;
    }

};

template<typename TT, typename SFINAE = void>
struct GuestType
{
    using type = GuestWrapper<TT>;
};

namespace internal
{
    // equivalent to C++17 void_t
    template<typename... Ts>
    struct make_void
    {
        typedef void type;
    };
    template<typename... Ts>
    using void_t = typename make_void<Ts...>::type;
}

template<typename TT>
struct GuestType<TT, internal::void_t<typename TT::is_guest_struct>>
{
    using type = TT;
};

// forward declare.
// uses template specialization to bypass the above,
// so a GUEST_STRUCT on the actual declaration is redundant (but still fine)
#define FORWARD_GUEST_STRUCT(CLS) \
    struct CLS;                   \
    template<>                    \
    struct GuestType<CLS>         \
    {                             \
        using type = CLS;         \
    }

template<typename TT>
using GUEST = typename GuestType<TT>::type;

template<>
struct GuestType<char>
{
    using type = char;
};

template<>
struct GuestType<signed char>
{
    using type = signed char;
};

template<>
struct GuestType<unsigned char>
{
    using type = unsigned char;
};

/*
template<typename TT>
struct GuestType<TT*>
{
    using type = GuestPointerWrapper<GUEST<TT>>;
};


template<typename RT, typename... Args>
struct GuestType<RT (*)(Args...)>
{
    using type = GuestPointerWrapper<void>;
};
*/

template<typename TT, int n>
struct GuestType<TT[n]>
{
    using type = GUEST<TT>[n];
};

template<typename TT>
struct GuestType<TT[0]>
{
    using type = GUEST<TT>[0];
};


template<typename TT>
GUEST<TT> RM(TT p)
{
    return GUEST<TT>::fromHost(p);
}

template<typename TT>
TT MR(GuestWrapper<TT> p)
{
    return p.get();
}

inline char RM(char c) { return c; }
inline unsigned char RM(unsigned char c) { return c; }
inline signed char RM(signed char c) { return c; }
inline char MR(char c) { return c; }
inline unsigned char MR(unsigned char c) { return c; }
inline signed char MR(signed char c) { return c; }


/*
template<typename TO, typename FROM>
GUEST<TO*> guest_ptr_cast(GUEST<FROM*> p)
{
    return GUEST<TO*>((FROM*)p);
}*/
template<typename TO, typename FROM, typename = std::enable_if<sizeof(GUEST<TO>) == sizeof(GUEST<FROM>)>>
GUEST<TO> guest_cast(GuestWrapper<FROM> p)
{
    //return GUEST<TO>((TO)(FROM)p);
    GUEST<TO> result;
    result.raw(p.raw());
    return result;
}


template<typename Ret, typename... Args, typename CallConv>
struct GuestWrapperBase<UPP<Ret(Args...),CallConv>>
{
private:
    HiddenValue<uint32_t> p;

public:
    using WrappedType = UPP<Ret(Args...),CallConv>;
    using RawGuestType = uint32_t;

    WrappedType get() const
    {
        uint32_t rawp = this->raw();
        return WrappedType(SYN68K_TO_US_CHECK0_CHECKNEG1((uint32_t)swap32(rawp)));
    }

    void set(WrappedType ptr)
    {
        this->raw(swap32(US_TO_SYN68K_CHECK0_CHECKNEG1(ptr.ptr)));
    }

    RawGuestType raw() const
    {
        return p.raw();
    }

    void raw(RawGuestType x)
    {
        p.raw(x);
    }

    uint32_t raw_host_order() const
    {
        return swap32(this->raw());
    }
    void raw_host_order(uint32_t x)
    {
        this->raw(swap32(x));
    }
    Ret operator()(Args... args); // definition in rsys/functions.impl.h to reduce dependencies
};

template<typename TT, typename CallConv>
GUEST<UPP<TT,CallConv>> RM(UPP<TT,CallConv> p)
{
    return GUEST<UPP<TT,CallConv>>::fromHost(p);
}

template<typename TT, typename CallConv>
UPP<TT,CallConv> MR(GuestWrapper<UPP<TT, CallConv>> p)
{
    return p.get();
}

}

#endif /* _MACTYPE_H_ */
