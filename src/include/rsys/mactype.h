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

#define AUTOMATIC_CONVERSIONS

namespace Executor
{


struct Point
{
    int16_t v;
    int16_t h;

    inline bool operator==(Point other) const { return v == other.v && h == other.h; }
};

#if defined(BIGENDIAN)
template<typename T>
T SwapTyped(T x) { return x; }

inline Point SwapPoint(Point x) { return x; }

#else
inline unsigned char SwapTyped(unsigned char x) { return x; }
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


namespace guestvalues
{
template<typename T, typename = void>
struct GuestTypeTraits;

template<typename T>
struct GuestTypeTraits<T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) <= 4>>
{
    using HostType = T;
    using GuestType = T;
    static constexpr bool fitsInRegister = true;

    static GuestType host_to_guest(HostType x) { return SwapTyped(x); }
    static HostType guest_to_host(GuestType x) { return SwapTyped(x); }

    static uint32_t host_to_reg(HostType x) { return (uint32_t)x; }
    static HostType reg_to_host(std::enable_if_t<sizeof(T) <= 4, uint32_t> x) { return (HostType)x; }
};

template<typename T>
struct GuestTypeTraits<T, std::enable_if_t<std::is_integral_v<T> && 4 < sizeof(T)>>
{
    using HostType = T;
    using GuestType = T;
    static constexpr bool fitsInRegister = false;

    static GuestType host_to_guest(HostType x) { return SwapTyped(x); }
    static HostType guest_to_host(GuestType x) { return SwapTyped(x); }
};

template<typename T>
struct GuestTypeTraits<T*>
{
    using HostType = T*;
    using GuestType = uint32_t;
    static constexpr bool fitsInRegister = true;

    static GuestType host_to_guest(HostType x) { return SwapTyped(US_TO_SYN68K_CHECK0_CHECKNEG1(x)); }
    static HostType guest_to_host(GuestType x) { return (HostType)SYN68K_TO_US_CHECK0_CHECKNEG1(SwapTyped(x)); }

    static uint32_t host_to_reg(HostType x) { return US_TO_SYN68K_CHECK0_CHECKNEG1(x); }
    static HostType reg_to_host(uint32_t x) { return (HostType)SYN68K_TO_US_CHECK0_CHECKNEG1(x); }
};

template<typename T, typename CallConv>
struct GuestTypeTraits<UPP<T, CallConv>>
{
    using HostType = UPP<T, CallConv>;
    using GuestType = uint32_t;
    static constexpr bool fitsInRegister = true;

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
    static constexpr bool fitsInRegister = true;

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
    static constexpr bool fitsInRegister = false;

    static GuestType host_to_guest(HostType x) { return SwapTyped(reinterpret_bits_cast<GuestType>(x)); }
    static HostType guest_to_host(GuestType x) { return reinterpret_bits_cast<HostType>(SwapTyped(x)); }
};
template<>
struct GuestTypeTraits<Point>
{
    using HostType = Point;
    using GuestType = uint32_t;
    static constexpr bool fitsInRegister = true;

    static GuestType host_to_guest(HostType x) { return reinterpret_bits_cast<uint32_t>(SwapPoint(x)); }
    static HostType guest_to_host(GuestType x) { return SwapPoint(reinterpret_bits_cast<Point>(x)); }
    
    static uint32_t host_to_reg(HostType x) { return ((uint32_t)x.v << 16) | (x.h & 0xFFFFU); }
    static HostType reg_to_host(uint32_t x) { return Point{ int16_t(x >> 16), int16_t(x & 0xFFFF) }; }
};

template<typename T>
struct GuestWrapperStorage
{
    using HostType = T;
    using GuestType = typename GuestTypeTraits<T>::GuestType;
    
    static_assert(sizeof(GuestType) >= 2, "GuestWrapper<> should not be used for single-byte values");

    union
    {
        uint8_t data[sizeof(GuestType)];
        uint16_t align;
    };

    GuestType raw() const
    {
        GuestType tmp;
        std::memcpy(&tmp, data, sizeof(GuestType));
        return tmp;
    }

    void raw(GuestType x)
    {
        std::memcpy(data, &x, sizeof(GuestType));
    }
};

template<typename T>
struct GuestWrapperGetSet : GuestWrapperStorage<T>
{
    using GuestWrapperStorage<T>::GuestWrapperStorage;

    using Traits = GuestTypeTraits<T>;
    using HostType = typename Traits::HostType;

    HostType get() const
    {
        return Traits::guest_to_host(this->raw());
    }

    void set(HostType x)
    {
        this->raw(Traits::host_to_guest(x));
    }
};

template<typename T, typename Enable = void>
struct GuestWrapperOps : GuestWrapperGetSet<T>
{
    using GuestWrapperGetSet<T>::GuestWrapperGetSet;
};

template<typename T>
struct GuestWrapper : GuestWrapperOps<T>
{
    using Traits = GuestTypeTraits<T>;
    using HostType = typename Traits::HostType;
    using GuestType = typename Traits::GuestType;

    uint32_t raw_host_order() const
    {
        static_assert(Traits::fitsInRegister);
        if constexpr(Traits::fitsInRegister)
            return Traits::host_to_reg(this->get());
        else
            return 0;
    }

    void raw_host_order(uint32_t x)
    {
        static_assert(Traits::fitsInRegister);
        if constexpr(Traits::fitsInRegister)
            this->set(Traits::reg_to_host(x));
    }

    GuestWrapper() = default;
    GuestWrapper(const GuestWrapper<T>& y) = default;
    GuestWrapper<T> &operator=(const GuestWrapper<T> &y) = default;
    using GuestWrapperOps<T>::GuestWrapperOps;

    template<typename T2, typename = typename std::enable_if_t<std::is_convertible_v<T2, T>>>
    GuestWrapper(const GuestWrapper<T2> &y)
    {
        if constexpr(sizeof(T) == sizeof(T2))
            this->raw(y.raw());
        else
            this->set(y.get());
    }

    template<typename T2, typename = typename std::enable_if_t<std::is_convertible_v<T2, T>>>
    GuestWrapper<T> &operator=(const GuestWrapper<T2> &y)
    {
        if constexpr(sizeof(T) == sizeof(T2))
            this->raw(y.raw());
        else
            this->set(y.get());
        return *this;
    }


#ifdef AUTOMATIC_CONVERSIONS
    GuestWrapper(const T &y)
    {
        this->set(y);
    }

    GuestWrapper<T> &operator=(const T &y)
    {
        this->set(y);
        return *this;
    }

    operator T() const { return this->get(); }
#else
    //GuestWrapper(std::enable_if_t<std::is_convertible_v<std::nullptr_t, T>, std::nullptr_t>)
    GuestWrapper(std::nullptr_t)
    {
        this->raw(0);
    }
#endif

    static GuestWrapper<T> fromRaw(GuestType r)
    {
        GuestWrapper<T> w;
        w.raw(r);
        return w;
    }
    static GuestWrapper<T> fromHost(HostType x)
    {
        GuestWrapper<T> w;
        w.set(x);
        return w;
    }

    explicit operator bool()
    {
        return this->raw() != 0;
    }


    template<typename T2, typename result = decltype(T() == T2()),
        typename enable = std::enable_if_t<sizeof(T) == sizeof(T2)>>
    bool operator==(GuestWrapper<T2> b) const
    {
        return this->raw() == b.raw();
    }
    bool operator==(GuestWrapper<T> b) const
    {
        return this->raw() == b.raw();
    }

    template<typename T2, typename result = decltype(T() != T2()),
        typename enable = std::enable_if_t<sizeof(T) != sizeof(T2)>>
    bool operator!=(GuestWrapper<T2> b) const
    {
        return this->raw() != b.raw();
    }
    bool operator!=(GuestWrapper<T> b) const
    {
        return this->raw() != b.raw();
    }

//#ifdef AUTOMATIC_CONVERSIONS
#if 0
    template<typename T2>
    //friend std::enable_if_t<std::is_convertible_v<T2,T>, bool> 
    friend auto
    operator==(GuestWrapper<T> a, T2 b) -> decltype(a.get() == b)
    {
        return a.get() == b;
    }
    template<typename T2> 
    //friend std::enable_if_t<std::is_convertible_v<T2,T>, bool>
    friend auto
    operator!=(GuestWrapper<T> a, T2 b) -> decltype(a.get() == b)
    {
        return a.get() != b;
    }
    template<typename T2>
    //friend std::enable_if_t<std::is_convertible_v<T2,T>, bool>
    friend auto
    operator==(T2 a, GuestWrapper<T> b) -> decltype(a == b.get())
    {
        return a == b.get();
    }
    template<typename T2>
    //friend std::enable_if_t<std::is_convertible_v<T2,T>, bool>
    friend auto
    operator!=(T2 a, GuestWrapper<T> b) -> decltype(a == b.get())
    {
        return a != b.get();
    }
#endif
};

template<class T1, class T2>
auto operator==(GuestWrapper<T1> a, T2 b)
    -> decltype(GuestTypeTraits<T1>::HostType() == GuestTypeTraits<T2>::HostType())
{
    return a.get() == b;
}

template<typename T>
struct GuestWrapperOps<T*> : GuestWrapperGetSet<T*>
{
#ifdef AUTOMATIC_CONVERSIONS
    T& operator*() const { return *this->get(); }
    T* operator->() const { return this->get(); }
#endif
};

template<>
struct GuestWrapperOps<void*> : GuestWrapperGetSet<void*>
{
};
template<>
struct GuestWrapperOps<const void*> : GuestWrapperGetSet<const void*>
{
};

template<typename Ret, typename... Args, typename CallConv>
struct GuestWrapperOps<UPP<Ret(Args...), CallConv>> : GuestWrapperGetSet<UPP<Ret(Args...), CallConv>>
{
    Ret operator()(Args... args)
    {
        return (this->get())(args...);
    }
};

template<typename T>
struct GuestWrapperOps<T, std::enable_if_t<std::is_integral_v<T>>> : GuestWrapperGetSet<T>
{
    GuestWrapper<T> operator~() const
    {
        return GuestWrapper<T>::fromRaw(~this->raw());
    }
};

template<class T1, class T2>
std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2> && sizeof(T1) == sizeof(T2),
    GuestWrapper<T1>>
operator&(GuestWrapper<T1> x, GuestWrapper<T2> y)
{
    return GuestWrapper<T1>::fromRaw(x.raw() & y.raw());
}

template<class T1, class T2>
std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2> && sizeof(T1) == sizeof(T2),
    GuestWrapper<T1>>
operator|(GuestWrapper<T1> x, GuestWrapper<T2> y)
{
    return GuestWrapper<T1>::fromRaw(x.raw() | y.raw());
}

template<class T1, class T2>
std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2> && sizeof(T1) == sizeof(T2),
    GuestWrapper<T1>&>
operator&=(GuestWrapper<T1>& x, GuestWrapper<T2> y)
{
    return x = x & y;
}

template<class T1, class T2>
std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2> && sizeof(T1) == sizeof(T2),
    GuestWrapper<T1>&>
operator|=(GuestWrapper<T1>& x, GuestWrapper<T2> y)
{
    return x = x | y;
}

template<>
struct GuestWrapperStorage<Point>
{
    struct is_guest_struct {};
    GuestWrapper<int16_t> v, h;

    using HostType = Point;
    using GuestType = uint32_t;

    GuestType raw() const
    {
        GuestType tmp;
        std::memcpy(&tmp, &v, sizeof(GuestType));
        return tmp;
    }

    void raw(GuestType x)
    {
        std::memcpy(&v, &x, sizeof(GuestType));
    }

    GuestWrapperStorage() = default;
    GuestWrapperStorage(GuestWrapper<int16_t> v, GuestWrapper<int16_t> h)
        : v(v), h(h) {}
};

#define GUEST_STRUCT    struct is_guest_struct {}

template<typename TT, typename SFINAE = void>
struct GuestType
{
    using type = GuestWrapper<TT>;
};

template<typename TT>
struct GuestType<TT, std::void_t<typename TT::is_guest_struct>>
{
    using type = TT;
};


template<>
struct GuestType<char>
{
    using type = char;
};

template<>
struct GuestType<bool>
{
    using type = int8_t;
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

template<typename TT, int n>
struct GuestType<TT[n]>
{
    using type = typename GuestType<TT>::type[n];
};

template<typename TT>
struct GuestType<TT[0]>
{
    using type = typename GuestType<TT>::type[0];
};



}

template<typename TT>
using GUEST = typename guestvalues::GuestType<TT>::type;

template<typename TT>
GUEST<TT> RM(TT p)
{
    return GUEST<TT>::fromHost(p);
}

template<typename TT>
TT MR(guestvalues::GuestWrapper<TT> p)
{
    return p.get();
}

inline char RM(char c) { return c; }
inline unsigned char RM(unsigned char c) { return c; }
inline signed char RM(signed char c) { return c; }
inline char MR(char c) { return c; }
inline unsigned char MR(unsigned char c) { return c; }
inline signed char MR(signed char c) { return c; }


template<typename TO, typename FROM, typename = std::enable_if<sizeof(GUEST<TO>) == sizeof(GUEST<FROM>)>>
GUEST<TO> guest_cast(guestvalues::GuestWrapper<FROM> p)
{
    //return GUEST<TO>((TO)(FROM)p);
    GUEST<TO> result;
    result.raw(p.raw());
    return result;
}


}

#endif /* _MACTYPE_H_ */
