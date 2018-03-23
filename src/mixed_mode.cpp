/* Copyright 2000 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include "rsys/common.h"

#include "FileMgr.h"
#include "MemoryMgr.h"

#include "rsys/mixed_mode.h"

#include "rsys/file.h"
#include <rsys/cpu.h>
#include <PowerCore.h>
#include <cassert>

using namespace Executor;

UniversalProcPtr Executor::C_NewRoutineDescriptor(
    ProcPtr proc, ProcInfoType info, ISAType isa)
{
    UniversalProcPtr retval;

    if(!proc)
        retval = NULL;
    else
    {
        RoutineDescriptor *p;

        p = (RoutineDescriptor *)NewPtr(sizeof *p);
        p->goMixedModeTrap = CWC(MIXED_MODE_TRAP);
        ROMlib_destroy_blocks((syn68k_addr_t)
                                  US_TO_SYN68K(&p->goMixedModeTrap),
                              2, true);
        p->version = CBC(kRoutineDescriptorVersion);
        p->routineDescriptorFlags = CBC(0);
        p->reserved1 = CLC(0);
        p->reserved2 = CBC(0);
        p->selectorInfo = CBC(0);
        p->routineCount = CWC(0);
        p->routineRecords[0].procInfo = CL(info);
        p->routineRecords[0].reserved1 = CBC(0);
        p->routineRecords[0].ISA = CB(isa);
        p->routineRecords[0].routineFlags = CWC(kSelectorsAreNotIndexable);
        p->routineRecords[0].procDescriptor = RM(proc);
        p->routineRecords[0].reserved2 = CLC(0);
        p->routineRecords[0].selector = CLC(0);
        retval = (UniversalProcPtr)p;
    }
    return retval;
}

void Executor::C_DisposeRoutineDescriptor(UniversalProcPtr ptr)
{
    DisposePtr((Ptr)ptr);
    warning_trace_info(NULL_STRING);
}

UniversalProcPtr Executor::C_NewFatRoutineDescriptor(
    ProcPtr m68k, ProcPtr ppc, ProcInfoType info)
{
    warning_unimplemented(NULL_STRING);
    *(long *)-1 = -1; /* abort */
    return NULL;
}

OSErr Executor::C_SaveMixedModeState(void *statep, uint32_t vers)
{
    warning_unimplemented(NULL_STRING);
    return paramErr;
}

OSErr Executor::C_RestoreMixedModeState(void *statep, uint32_t vers)
{
    warning_unimplemented(NULL_STRING);
    return paramErr;
}

namespace
{

struct PowerPCFrame
{
    GUEST<uint32_t> backChain;
    GUEST<uint32_t> saveCR;
    GUEST<uint32_t> saveLR;
    GUEST<uint32_t> reserved1;
    GUEST<uint32_t> reserved2;
    GUEST<uint32_t> saveRTOC;
    GUEST<uint32_t> parameters[8];
};
}

extern uint32_t Executor::ModeSwitch(UniversalProcPtr theProcPtr, ProcInfoType procinfo, ISAType fromISA)
{
    RoutineRecord m68kroutine = {};
    const RoutineRecord* routine = nullptr;

    if(theProcPtr->goMixedModeTrap == CWC(MIXED_MODE_TRAP))
    {
        const RoutineRecord *beginRoutines = &theProcPtr->routineRecords[0],
                            *endRoutines = &theProcPtr->routineRecords[CW(theProcPtr->routineCount) + 1];
       
       routine = std::find_if(beginRoutines, endRoutines,
                    [](const RoutineRecord& rr) { return rr.ISA == CBC(kPowerPCISA); });
        if(routine == endRoutines)
            routine = beginRoutines;
    
        if(routine == endRoutines)
        {
            fprintf(stderr, "*** bad CallUniversalProc ***\n");
            std::exit(1);
        }
    }
    else
    {
        assert(fromISA != kM68kISA);
        m68kroutine.procDescriptor = RM((ProcPtr)theProcPtr);
        m68kroutine.procInfo = CL(procinfo);
        routine = &m68kroutine;
    }

    procinfo = CL(routine->procInfo);


    int convention = procinfo & 0xf;

    int nParameters = 0;
    constexpr int maxParameters = 13;
    int paramSizes[maxParameters];
    int paramRegs[maxParameters];
    int resultSize = (procinfo >> 4) & 0x3;
    int resultReg = -1;

    uint32_t stackArgsSize = 0;

    switch(convention)
    {
        case kRegisterBased:
            for(uint32_t field = 11; field <= 27; field += 5)
            {
                uint32_t argsize = (procinfo >> field) & 0x3;
                if(argsize)
                {
                    paramRegs[nParameters] = (procinfo >> (field + 2)) & 0x7;
                    paramSizes[nParameters++] = argsize;
                }
                else
                    break;
            }
            resultReg = (procinfo >> 6) & 0x1F;
            assert(resultReg < 16);
            break;
        case kPascalStackBased:
        case kCStackBased:
            for(uint32_t field = 6; field <= 30; field += 2)
            {
                uint32_t argsize = (procinfo >> field) & 0x3;
                if(argsize)
                {
                    paramSizes[nParameters++] = argsize;
                    stackArgsSize += argsize == 3 ? 4 : 2;
                }
                else
                    break;
            }
            break;
        default:
            fprintf(stderr,"modeswitch: unimplemented/invalid convention %d (procinfo = %08x)\n", convention, procinfo);
            std::exit(1);
    }

    auto sizeOnStack = [](int argsize) { return argsize == 3 ? 4 : 2; };
    auto readsized = [](void *p, int argsize) -> uint32_t {
        switch(argsize)
        {
            case 1:
                return *((uint8_t*)p);
            case 2:
                return CW(*(GUEST<uint16_t>*)p);
            case 3:
                return CL(*(GUEST<uint32_t>*)p);
            default:
                std::abort();
        }
    };
    auto writesized = [](void *p, int argsize, uint32 val) {
        switch(argsize)
        {
            case 1:
                *((uint8_t*)p) = val;
                break;
            case 2:
                *(GUEST<uint16_t>*)p = CW(val);
                break;
            case 3:
                *(GUEST<uint32_t>*)p = CL(val);
                break;
        }
    };

    PowerCore& cpu = getPowerCore();
    uint32_t stackPointer = fromISA == kM68kISA ? EM_A7: cpu.r[1];

    uint32_t paramValues[maxParameters];
    uint32_t retval;

    static constexpr int regmap[] = {
            0, /* kRegisterD0,  0 */
            1, /* kRegisterD1,  1 */
            2, /* kRegisterD2,  2 */
            3, /* kRegisterD3,  3 */
            8, /* kRegisterA0,  4 */
            9, /* kRegisterA1,  5 */
            10, /* kRegisterA2,  6 */
            11, /* kRegisterA3,  7 */
            4, /* kRegisterD4,  8 */
            5, /* kRegisterD5,  9 */
            6, /* kRegisterD6, 10 */
            7, /* kREgisterD7, 11 */
            12, /* kRegisterA4, 12 */
            13, /* kRegisterA5, 13 */
            14, /* kRegisterA6, 14 */
        };
    static constexpr uint32_t regmask[] = {0U, 0xFFU, 0xFFFFU, 0xFFFFFFFFU};

    if(fromISA == kM68kISA)
    {
        uint8_t *stackArgPtr = ptr_from_longint<uint8_t*>(EM_A7 + 4);

        switch(convention)
        {
            case kRegisterBased:
                for(int i = 0; i < nParameters; i++)
                {
                    paramValues[i] = cpu_state.regs[regmap[paramSizes[i]]].ul.n
                        & regmask[paramSizes[i]];
                }
                break;
            case kPascalStackBased:
                for(int i = nParameters-1; i >= 0; i--)
                {
                    paramValues[i] = readsized(stackArgPtr, paramSizes[i]);
                    stackArgPtr += sizeOnStack(paramSizes[i]);
                }
                break;
            case kCStackBased:
            case kThinkCStackBased:
                for(int i = 0; i < nParameters; i++)
                {
                    paramValues[i] = readsized(stackArgPtr, paramSizes[i]);
                    stackArgPtr += sizeOnStack(paramSizes[i]);
                }
                break;
            default:
                fprintf(stderr,"modeswitch: unimplemented/invalid convention %d (procinfo = %08x)\n", convention, procinfo);
                std::exit(1);
        }
    }
    else if(fromISA == kPowerPCISA)
    {
        PowerPCFrame& parentFrame = *ptr_from_longint<PowerPCFrame*>(cpu.r[1]);

        for(int i = 0; i < nParameters; i++)
            paramValues[i] = (i+2) < 8 ? cpu.r[i+2+3] : CL(parentFrame.parameters[i+2]);
    }
    else
        std::abort();


    

    if(routine->ISA == kPowerPCISA)
    {
        uint32_t frame_size = std::max(8, nParameters) * 4 + 24;
        cpu.r[1] = stackPointer - frame_size;
        PowerPCFrame& frame = *ptr_from_longint<PowerPCFrame*>(cpu.r[1]);

        for(int i = 0; i < nParameters; i++)
        {
            if(i < 8)
                cpu.r[i+3] = paramValues[i];
            else
                frame.parameters[i] = CL(paramValues[i]);
        }

        frame.saveRTOC = CL(cpu.r[2]);
        PPCProcDescriptor& proc = *(PPCProcDescriptor*) MR(routine->procDescriptor);
        cpu.CIA = CL(proc.code);
        cpu.r[2] = CL(proc.rtoc);
        cpu.lr = 0xFFFFFFFC;
        cpu.execute();
        cpu.r[2] = CL(frame.saveRTOC);

        retval = cpu.r[3];
    }
    else
    {
        EM_A7 = stackPointer;

        auto pusharg = [&](int argsize, uint32 val) {
            EM_A7 -= sizeOnStack(argsize);
            writesized(ptr_from_longint<uint8_t*>(EM_A7), argsize, val);
        };

        switch(convention)
        {
            case kRegisterBased:
                for(int i = 0; i < nParameters; i++)
                {
                    cpu_state.regs[regmap[paramSizes[i]]].ul.n = paramValues[i];
                }
                break;

            case kPascalStackBased:
                if(resultSize)
                    pusharg(resultSize, 0);
                for(int i = 0; i < nParameters; i++)
                    pusharg(paramSizes[i], paramValues[i]);
                break;

            case kCStackBased:
                for(int i = nParameters-1; i >= 0; i--)
                    pusharg(paramSizes[i], paramValues[i]);
                break;
        }

        CALL_EMULATOR(routine->procDescriptor.raw_host_order());

        switch(convention)
        {
            case kRegisterBased:
                retval = cpu_state.regs[resultReg].ul.n
                    & regmask[resultSize];
                break;

            case kPascalStackBased:
                readsized(ptr_from_longint<uint8_t*>(EM_A7), resultSize);
                EM_A7 += sizeOnStack(resultSize);
                break;
            
            case kCStackBased:
            case kThinkCStackBased:
                retval = EM_D0 & regmask[resultSize];
                EM_A7 += stackArgsSize;
                break;
        }
    }


    cpu.r[1] = EM_A7 = stackPointer;

    if(fromISA == kM68kISA)
    {
        uint32_t retaddr = POPADDR();

        switch(convention)
        {
            case kRegisterBased:
                cpu_state.regs[resultReg].ul.n = retval;
                break;
            case kPascalStackBased:
                EM_A7 += stackArgsSize;
                writesized(ptr_from_longint<uint8_t*>(EM_A7), resultSize, retval);
                break;
            case kCStackBased:
            case kThinkCStackBased:
                EM_D0 = retval;
                break;
        }

        return retaddr;
    }
    else
    {
        return retval;
    }
}

LONGINT Executor::C_CallUniversalProc(UniversalProcPtr theProcPtr, ProcInfoType procInfo)
{
    return ModeSwitch(theProcPtr, procInfo, kPowerPCISA);
#if 0
    RoutineRecord m68kroutine = {};
    const RoutineRecord* routine = nullptr;

    if(theProcPtr->goMixedModeTrap == CWC(MIXED_MODE_TRAP))
    {
        const RoutineRecord *beginRoutines = &theProcPtr->routineRecords[0],
                            *endRoutines = &theProcPtr->routineRecords[CW(theProcPtr->routineCount) + 1];
       
       routine = std::find_if(beginRoutines, endRoutines,
                    [](const RoutineRecord& rr) { return rr.ISA == CBC(kPowerPCISA); });
        if(routine == endRoutines)
            routine = beginRoutines;
    
        if(routine == endRoutines)
        {
            fprintf(stderr, "*** bad CallUniversalProc ***\n");
            std::exit(1);
        }
    }
    else
    {
        m68kroutine.procDescriptor = RM((ProcPtr)theProcPtr);
        m68kroutine.procInfo = CL(procinfo);
        routine = &m68kroutine;
    }

    procinfo = CL(routine->procInfo);

    int convention = procinfo & 0xf;

    int nParameters = 0;
    constexpr int maxParameters = 13;
    int paramSizes[maxParameters];
    int paramRegs[maxParameters];

    uint32_t stackArgsSize = 0;

    switch(convention)
    {
        case kRegisterBased:
            for(uint32_t field = 11; field <= 27; field += 5)
            {
                uint32_t argsize = (procinfo >> field) & 0x3;
                if(argsize)
                {
                    paramRegs[nParameters] = (procinfo >> (field + 2)) & 0x7;
                    paramSizes[nParameters++] = argsize;
                }
                else
                    break;
            }
            break;
        case kPascalStackBased:
        case kCStackBased:
            for(uint32_t field = 6; field <= 30; field += 2)
            {
                uint32_t argsize = (procinfo >> field) & 0x3;
                if(argsize)
                {
                    paramSizes[nParameters++] = argsize;
                    stackArgsSize += argsize == 3 ? 4 : 2;
                }
                else
                    break;
            }
            break;
        default:
            fprintf(stderr,"modeswitch: unimplemented/invalid convention %d (procinfo = %08x)\n", convention, procinfo);
            std::exit(1);
    }

    PowerCore& cpu = getPowerCore();
    PowerPCFrame& parentFrame = *ptr_from_longint<PowerPCFrame*>(cpu.r[1]);

    for(int i = 0; i < 8; i++)
        parentFrame.parameters[i] = CL(cpu.r[3+i]);

    parentFrame.saveLR = CL(cpu.lr);

    uint32_t retval;

    if(routine->ISA == kPowerPCISA)
    {
        uint32_t backChain = cpu.r[1];
        uint32_t frame_size = std::max(8, nParameters) * 4 + 24;
        cpu.r[1] -= frame_size;
        PowerPCFrame& frame = *ptr_from_longint<PowerPCFrame*>(cpu.r[1]);

        std::memcpy(&frame.parameters, &parentFrame.parameters + 2, 4 * nParameters);
        for(int i = 0; i < 8; i++)
            cpu.r[3 + i] = CL(frame.parameters[i]);

        frame.saveRTOC = CL(cpu.r[2]);
        PPCProcDescriptor& proc = *(PPCProcDescriptor*) MR(routine->procDescriptor);
        cpu.CIA = CL(proc.code);
        cpu.r[2] = CL(proc.rtoc);
        cpu.execute();
        cpu.r[2] = CL(frame.saveRTOC);

        retval = cpu.r[3];
    }
    else
    {
        EM_A7 = cpu.r[1];

        auto pusharg = [](int argsize, uint32 val) {
            EM_A7 -= argsize == 3 ? 4 : 2;
            switch(argsize)
            {
                case 1:
                    *(ptr_from_longint<uint8_t*>(EM_A7)) = val;
                    break;
                case 2:
                    *(ptr_from_longint<GUEST<uint16_t>*>(EM_A7)) = CW(val);
                    break;
                case 3:
                    *(ptr_from_longint<GUEST<uint32_t>*>(EM_A7)) = CL(val);
                    break;
            }
        };

        switch(convention)
        {
            case kRegisterBased:
                std::abort();
            
            case kPascalStackBased:
                // return value
                //pusharg(retValSize, 0);
                std::abort();
                for(int i = 0; i < nParameters; i++)
                    pusharg(paramSizes[i], CL(parentFrame.parameters[i+2]));
                break;

            case kCStackBased:
                for(int i = nParameters-1; i >= 0; i--)
                    pusharg(paramSizes[i], CL(parentFrame.parameters[i+2]));
                break;
        }

        CALL_EMULATOR(routine->procDescriptor.raw_host_order());

        switch(convention)
        {
            case kRegisterBased:
                std::abort();
            case kPascalStackBased:
                // TODO
                break;
            case kCStackBased:
                retval = EM_D0;
                EM_A7 += stackArgsSize;
                break;
        }
    }

    for(int i = 0; i < 8; i++)
        cpu.r[3+i] = CL(parentFrame.parameters[i]);

    return retval;
#endif
}
