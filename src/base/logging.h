#pragma once
#include <base/mactype.h>
#include <syn68k_public.h>

#include <iostream>
#include <unordered_map>

namespace Executor
{
namespace logging
{

bool enabled();
void setEnabled(bool e);
void resetNestingLevel();

bool loggingActive();
void indent();

extern int nestingLevel;

void logUntypedArgs(const char *name);
void logUntypedReturn(const char *name);

void logEscapedChar(unsigned char c);
bool canConvertBack(const void* p);
bool validAddress(const void* p);
bool validAddress(syn68k_addr_t p);
void logValue(char x);
void logValue(unsigned char x);
void logValue(signed char x);
void logValue(int16_t x);
void logValue(uint16_t x);
void logValue(int32_t x);
void logValue(uint32_t x);
void logValue(unsigned char* p);
void logValue(const void* p);
void logValue(void* p);
void logValue(ProcPtr p);
void dumpRegsAndStack();

template<typename T>
void logValue(const T& arg)
{
    std::clog << "?";
}
template<class T>
void logValue(const guestvalues::GuestWrapper<T>& p)
{
    logValue(p.get());
}
template<class T>
void logValue(T* p)
{
    if(canConvertBack(p))
        std::clog << "0x" << std::hex << US_TO_SYN68K_CHECK0_CHECKNEG1(p) << std::dec;
    else
        std::clog << "?";
    if(validAddress(p))
    {
        std::clog << " => ";
        logValue(*p);
    }
}
template<class T>
void logValue(guestvalues::GuestWrapper<T*> p)
{
    std::clog << "0x" << std::hex << p.raw() << std::dec;
    if(validAddress(p.raw_host_order()))
    {
        std::clog << " => ";
        logValue(*(p.get()));
    }
}


template<typename Arg>
inline void logList(Arg a)
{
    logValue(a);
}
inline void logList()
{
}
template<typename Arg1, typename Arg2, typename... Args>
void logList(Arg1 a, Arg2 b, Args... args)
{
    logValue(a);
    std::clog << ", ";
    logList(b,args...);
}

template<class T>
bool validAddress(GUEST<T*> p)
{
    return validAddress(p.raw_host_order());
}

template<typename... Args>
void logTrapCall(const char* trapname, Args... args)
{
    if(!loggingActive())
        return;
    std::clog.clear();
    indent();
    std::clog << trapname << "(";
    logList(args...);
    std::clog << ")\n" << std::flush;
}

template<typename Ret, typename... Args>
void logTrapValReturn(const char* trapname, Ret ret, Args... args)
{
    if(!loggingActive())
        return;
    indent();
    std::clog << "returning: " << trapname << "(";
    logList(args...);
    std::clog << ") => ";
    logValue(ret);
    std::clog << std::endl << std::flush;
}
template<typename... Args>
void logTrapVoidReturn(const char* trapname, Args... args)
{
    if(!loggingActive())
        return;
    indent();
    std::clog << "returning: " << trapname << "(";
    logList(args...);
    std::clog << ")\n" << std::flush;
}

template<typename T>
struct FunctionTypeForFunctor : FunctionTypeForFunctor<decltype(&T::operator())>
{ };

template<typename Cls, typename Ret, typename... Args>
struct FunctionTypeForFunctor<Ret (Cls::*)(Args...)> : FunctionTypeForFunctor<Ret (Args...)>
{ };

template<typename Ret, typename... Args>
struct FunctionTypeForFunctor<Ret (*)(Args...)> : FunctionTypeForFunctor<Ret (Args...)>
{ };
template<typename Ret, typename... Args>
struct FunctionTypeForFunctor<Ret (&)(Args...)> : FunctionTypeForFunctor<Ret (Args...)>
{ };
template<typename Ret, typename... Args>
struct FunctionTypeForFunctor<Ret (Args...)>
{
    using type = Ret (Args...);
};


template<typename CallConv, typename F,
         typename FunType = typename FunctionTypeForFunctor<F>::type>
class LoggedFunction;

template<typename CallConv, typename F, typename Ret, typename... Args>
class LoggedFunction<CallConv, F, Ret(Args...)>
{
    const char *name;
    F fun;

    struct LogLevelAdjuster
    {
        LogLevelAdjuster() { nestingLevel++; }
        ~LogLevelAdjuster() { nestingLevel--; }
    };
public:
    LoggedFunction(const char *name, const F& fun)
        : name(name), fun(fun)
    {
    }

    Ret operator() (Args... args) const
    {
        LogLevelAdjuster adj;

        if constexpr(std::is_same_v<CallConv, callconv::Raw>)
            logUntypedArgs(name);
        else
            logTrapCall(name, args...);

        if constexpr(std::is_same_v<Ret, void>)
        {
            fun(args...);
            logTrapVoidReturn(name, args...);
        }
        else
        {
            Ret ret = fun(args...);

            if constexpr(std::is_same_v<CallConv, callconv::Raw>)
                logUntypedReturn(name);
            else 
                logTrapValReturn(name, ret, args...);

            return ret;
        }
    }
};

template<class CallConv = callconv::Pascal, class F>
LoggedFunction<CallConv, F> makeLoggedFunction(const char *name, const F& f)
{
    return LoggedFunction<CallConv, F>(name, f);
}
template<class F1, class F>
LoggedFunction<callconv::Pascal, F, F1> makeLoggedFunction1(const char *name, const F& f)
{
    return LoggedFunction<callconv::Pascal, F, F1>(name, f);
}

}
}
