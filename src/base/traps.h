#pragma once

#include <stdint.h>
#include <syn68k_public.h>
#include <unordered_map>

#include <base/functions.h>
#include <functional>

namespace Executor
{

namespace traps
{
#define TOOLBIT (0x0800)

namespace selectors
{
    template <uint32_t mask> struct D0;
    template <uint32_t mask> struct D1;
    using D0W = D0<0xFFFF>;
    using D0L = D0<0xFFFFFFFF>;
    template <uint32_t mask = 0xFFFF> struct StackWMasked;
    template <uint32_t mask = 0xFFFFFFFF> struct StackLMasked;
    using StackW = StackWMasked<>;
    using StackL = StackLMasked<>;
    template <uint32_t mask = 0xFFFF> struct StackWLookahead;
}

namespace internal
{
    class DeferredInit
    {
    public:
        DeferredInit();
        virtual void init() = 0;
        static void initAll();
    private:
        DeferredInit *next;
        static DeferredInit *first, *last;
    };
}

class GenericDispatcherTrap : public internal::DeferredInit
{
public:
    virtual void addSelector(uint32_t sel, std::function<syn68k_addr_t(syn68k_addr_t)> handler) = 0;
protected:
    std::unordered_map<uint32_t, std::function<syn68k_addr_t(syn68k_addr_t)>> selectors;
};

template<class SelectorConvention>
class DispatcherTrap : public GenericDispatcherTrap
{
    static syn68k_addr_t invokeFrom68K(syn68k_addr_t addr, void* extra);
    const char *name;
    uint16_t trapno;
public:
    virtual void init() override;
    virtual void addSelector(uint32_t sel, std::function<syn68k_addr_t(syn68k_addr_t)> handler) override;
    DispatcherTrap(const char* name, uint16_t trapno) : name(name), trapno(trapno) {}
};

template<typename F, F* fptr, typename CallConv = callconv::Pascal>
class WrappedFunction {};

template<typename Ret, typename... Args, Ret (*fptr)(Args...), typename CallConv>
class WrappedFunction<Ret (Args...), fptr, CallConv> : public internal::DeferredInit
{
public:
    Ret operator()(Args... args) const
    {
        return (*fptr)(args...);
    }

    UPP<Ret (Args...), CallConv> operator&() const
    {
        return guestFP;
    }

    virtual void init() override;

    WrappedFunction(const char* name, const char* exportToLib = nullptr);

    using UPPType = UPP<Ret (Args...), CallConv>;
protected:
    UPPType guestFP;
    const char *name;
    const char *libname;
};

template<typename F, F* fptr, int trapno, typename CallConv = callconv::Pascal>
class TrapFunction {};

template<typename Ret, typename... Args, Ret (*fptr)(Args...), int trapno, typename CallConv>
class TrapFunction<Ret (Args...), fptr, trapno, CallConv> : public WrappedFunction<Ret (Args...), fptr, CallConv>
{
public:
    using UPPType = typename WrappedFunction<Ret (Args...), fptr, CallConv>::UPPType;

    Ret operator()(Args... args) const
    {
        if(isPatched())
            return invokeViaTrapTable(args...);
        else
            return fptr(args...);
    }
    
    virtual void init() override;

    TrapFunction(const char* name, const char* exportToLib = nullptr) : WrappedFunction<Ret(Args...),fptr,CallConv>(name, exportToLib) {}

    bool isPatched() const { return tableEntry() != originalFunction; }
    Ret invokeViaTrapTable(Args...) const;
private:
    syn68k_addr_t originalFunction;

    syn68k_addr_t& tableEntry() const
    {
        if(trapno & TOOLBIT)
            return tooltraptable[trapno & 0x3FF];
        else
            return ostraptable[trapno & 0xFF];
    }
};

template<typename F, F* fptr, int trapno, uint32_t selector, typename CallConv = callconv::Pascal>
class SubTrapFunction {};

template<typename Ret, typename... Args, Ret (*fptr)(Args...), int trapno, uint32_t selector, typename CallConv>
class SubTrapFunction<Ret (Args...), fptr, trapno, selector, CallConv> : public WrappedFunction<Ret (Args...), fptr, CallConv>
{
public:
    Ret operator()(Args... args) const { return fptr(args...); }
    SubTrapFunction(const char* name, GenericDispatcherTrap& dispatcher, const char* exportToLib = nullptr);
    virtual void init() override;
private:
    GenericDispatcherTrap& dispatcher;
};

template<typename Trap, typename F, bool... flags>
class TrapVariant;

template<typename Trap, typename Ret, typename... Args, bool... flags>
class TrapVariant<Trap, Ret (Args...), flags...>  : public internal::DeferredInit
{
    template<class T1>
    struct cast_any_t
    {
        T1 x;

        template<class T2>
        operator T2() { return (T2)x; }
    };

    template<class T1>
    cast_any_t<T1> cast_any(const T1& x) const { return {x}; }
public:
    Ret operator()(Args... args) const { return trap(cast_any(args)..., flags...); }

    virtual void init() override;
    TrapVariant(const Trap& trap, const char* name, const char* exportToLib = nullptr);
private:
    const Trap& trap;
    const char *name;
    const char *libname;
};

#define EXTERN_FUNCTION_WRAPPER(NAME, FPTR, INIT, ...) \
    extern Executor::traps::__VA_ARGS__ NAME
#define EXTERN_DISPATCHER_TRAP(NAME, TRAP, SELECTOR) \
    extern Executor::traps::DispatcherTrap<Executor::traps::selectors::SELECTOR> NAME
#define DEFINE_FUNCTION_WRAPPER(NAME, FPTR, INIT, ...) \
    Executor::traps::__VA_ARGS__ NAME INIT;   \
    template class Executor::traps::__VA_ARGS__;
#define DEFINE_DISPATCHER_TRAP(NAME, TRAP, SELECTOR) \
    Executor::traps::DispatcherTrap<Executor::traps::selectors::SELECTOR> NAME { #NAME, TRAP }

#ifndef TRAP_INSTANTIATION
#define TRAP_INSTANTIATION EXTERN
#endif

#define PREPROCESSOR_CONCAT1(A,B) A##B
#define PREPROCESSOR_CONCAT(A,B) PREPROCESSOR_CONCAT1(A,B)
#define CREATE_FUNCTION_WRAPPER PREPROCESSOR_CONCAT(TRAP_INSTANTIATION, _FUNCTION_WRAPPER)
#define DISPATCHER_TRAP PREPROCESSOR_CONCAT(TRAP_INSTANTIATION, _DISPATCHER_TRAP)

#define COMMA ,
#define PASCAL_TRAP(NAME, TRAP) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME, "InterfaceLib"), TrapFunction<decltype(C_##NAME) COMMA &C_##NAME COMMA TRAP>)
#define REGISTER_TRAP(NAME, TRAP, ...) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME, "InterfaceLib"), TrapFunction<decltype(C_##NAME) COMMA &C_##NAME COMMA TRAP COMMA callconv::Register<__VA_ARGS__>>)
#define REGISTER_TRAP2(NAME, TRAP, ...) \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &NAME, (#NAME, "InterfaceLib"), TrapFunction<decltype(NAME) COMMA &NAME COMMA TRAP COMMA callconv::Register<__VA_ARGS__>>)

#define PASCAL_SUBTRAP(NAME, TRAP, SELECTOR, TRAPNAME) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME, TRAPNAME, "InterfaceLib"), SubTrapFunction<decltype(C_##NAME) COMMA &C_##NAME COMMA TRAP COMMA SELECTOR>)
#define REGISTER_SUBTRAP(NAME, TRAP, SELECTOR, TRAPNAME, ...) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME, TRAPNAME, "InterfaceLib"), SubTrapFunction<decltype(C_##NAME) COMMA &C_##NAME COMMA TRAP COMMA SELECTOR COMMA callconv::Register<__VA_ARGS__>>)
#define REGISTER_SUBTRAP2(NAME, TRAP, SELECTOR, TRAPNAME, ...) \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &NAME, (#NAME, TRAPNAME, "InterfaceLib"), SubTrapFunction<decltype(NAME) COMMA &NAME COMMA TRAP COMMA SELECTOR COMMA callconv::Register<__VA_ARGS__>>)

#define NOTRAP_FUNCTION(NAME) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME, "InterfaceLib"), WrappedFunction<decltype(C_##NAME) COMMA &C_##NAME>)
#define NOTRAP_FUNCTION2(NAME) \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &NAME, (#NAME, "InterfaceLib"), WrappedFunction<decltype(NAME) COMMA &NAME>)
#define PASCAL_FUNCTION(NAME) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME), WrappedFunction<decltype(C_##NAME) COMMA &C_##NAME>)
#define REGISTER_FUNCTION(NAME, ...) \
    CREATE_FUNCTION_WRAPPER(NAME, &C_##NAME, (#NAME), WrappedFunction<decltype(C_##NAME) COMMA &C_##NAME COMMA callconv::Register<__VA_ARGS__>>)
    
#define RAW_68K_FUNCTION(NAME) \
    syn68k_addr_t _##NAME(syn68k_addr_t, void *); \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &_##NAME, (#NAME), WrappedFunction<decltype(_##NAME) COMMA &_##NAME COMMA callconv::Raw>)
#define RAW_68K_TRAP(NAME, TRAP) \
    syn68k_addr_t _##NAME(syn68k_addr_t, void *); \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &_##NAME, (#NAME), TrapFunction<decltype(_##NAME) COMMA &_##NAME COMMA TRAP COMMA callconv::Raw>)


#define TRAP_VARIANT(NAME, IMPL_NAME, ...) \
    CREATE_FUNCTION_WRAPPER(NAME, , (stub_##IMPL_NAME, #NAME, "InterfaceLib"), TrapVariant<decltype(stub_##IMPL_NAME), __VA_ARGS__>)
#define REGISTER_FLAG_TRAP(IMPL_NAME, NAME0, NAME1, TRAP, TYPE, ...) \
    REGISTER_TRAP2(IMPL_NAME, TRAP, __VA_ARGS__); \
    TRAP_VARIANT(NAME0, IMPL_NAME, TYPE, false); \
    TRAP_VARIANT(NAME1, IMPL_NAME, TYPE, true)
#define REGISTER_2FLAG_TRAP(IMPL_NAME, NAME00, NAME01, NAME10, NAME11, TRAP, TYPE, ...) \
    REGISTER_TRAP2(IMPL_NAME, TRAP, __VA_ARGS__); \
    TRAP_VARIANT(NAME00, IMPL_NAME, TYPE, false, false); \
    TRAP_VARIANT(NAME10, IMPL_NAME, TYPE, true, false); \
    TRAP_VARIANT(NAME01, IMPL_NAME, TYPE, false, true); \
    TRAP_VARIANT(NAME11, IMPL_NAME, TYPE, true, true)

#define ASYNCBIT (1 << 10)
#define HFSBIT (1 << 9)

#define FILE_TRAP(NAME, PBTYPE, TRAP) \
    REGISTER_FLAG_TRAP(NAME, NAME##Sync, NAME##Async, TRAP, OSErr(PBTYPE), D0 (A0, TrapBit<ASYNCBIT>))

#define FILE_SUBTRAP(NAME, PBTYPE, TRAP, SELECTOR, TRAPNAME) \
    REGISTER_SUBTRAP2(NAME, TRAP, SELECTOR, TRAPNAME, D0 (A0, TrapBit<ASYNCBIT>)); \
    TRAP_VARIANT(NAME##Sync, NAME, OSErr(PBTYPE), false); \
    TRAP_VARIANT(NAME##Async, NAME, OSErr(PBTYPE), true)


#define HFS_TRAP(NAME, HNAME, PBTYPE, TRAP) \
    inline OSErr NAME##_##HNAME(ParmBlkPtr pb, Boolean async, Boolean hfs) \
    { \
        return hfs ? HNAME((PBTYPE)pb, async) : NAME(pb, async); \
    } \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &NAME##_##HNAME, \
        (#NAME "/" #HNAME), \
        TrapFunction<decltype(NAME##_##HNAME), \
            &NAME##_##HNAME, TRAP, \
            callconv::Register<D0 (A0, TrapBit<ASYNCBIT>, TrapBit<HFSBIT>)>>); \
    TRAP_VARIANT(NAME##Sync, NAME, OSErr(ParmBlkPtr), false, false); \
    TRAP_VARIANT(NAME##Async, NAME, OSErr(ParmBlkPtr), true, false); \
    TRAP_VARIANT(HNAME##Sync, NAME, OSErr(HParmBlkPtr), false, true); \
    TRAP_VARIANT(HNAME##Async, NAME, OSErr(HParmBlkPtr), true, true)

#define HFS_SUBTRAP(NAME, HNAME, PBTYPE, TRAP, SELECTOR, TRAPNAME) \
    inline OSErr NAME##_##HNAME(ParmBlkPtr pb, Boolean async, Boolean hfs) \
    { \
        return hfs ? HNAME((PBTYPE)pb, async) : NAME(pb, async); \
    } \
    CREATE_FUNCTION_WRAPPER(stub_##NAME, &NAME##_##HNAME, \
        (#NAME "/" #HNAME, TRAPNAME), \
        SubTrapFunction<decltype(NAME##_##HNAME), \
            &NAME##_##HNAME, TRAP, SELECTOR, \
            callconv::Register<D0 (A0, TrapBit<ASYNCBIT>, TrapBit<HFSBIT>)>>); \
    TRAP_VARIANT(NAME##Sync, NAME, OSErr(ParmBlkPtr), false, false); \
    TRAP_VARIANT(NAME##Async, NAME, OSErr(ParmBlkPtr), true, false); \
    TRAP_VARIANT(HNAME##Sync, NAME, OSErr(HParmBlkPtr), false, true); \
    TRAP_VARIANT(HNAME##Async, NAME, OSErr(HParmBlkPtr), true, true)



#define LOWMEM_ACCESSOR(NAME) \
    inline decltype(NAME)::type LMGet##NAME() { return LM(NAME); } \
    inline void LMSet##NAME(decltype(NAME)::type val) { LM(NAME) = val; } \
    NOTRAP_FUNCTION2(LMGet##NAME); \
    NOTRAP_FUNCTION2(LMSet##NAME)

void init(bool enableLogging);

}
}
