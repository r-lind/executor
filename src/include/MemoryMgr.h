#if !defined(_MEMORY_MGR_H_)
#define _MEMORY_MGR_H_

#include "ExMacTypes.h"
#include <rsys/lowglobals.h>

#define MODULE_NAME MemoryMgr
#include <rsys/api-module.h>

/*
 * Copyright 1986, 1989, 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

namespace Executor
{
enum
{
    memFullErr = (-108),
    memLockedErr = (-117),
    memPurErr = (-112),
    memWZErr = (-111),
};
//enum { memAZErr	 = -113 };
enum
{
    nilHandleErr = (-109),
};

enum
{
    memROZErr = (-99),
    memAdrErr = (-110),
    memAZErr = (-113),
    memPCErr = (-114),
    memBCErr = (-115),
    memSCErr = (-116),
};

typedef UPP<LONGINT(Size)> GrowZoneProcPtr;

struct Zone
{
    GUEST_STRUCT;
    GUEST<Ptr> bkLim;
    GUEST<Ptr> purgePtr;
    GUEST<Ptr> hFstFree;
    GUEST<LONGINT> zcbFree;
    GUEST<GrowZoneProcPtr> gzProc;
    GUEST<INTEGER> moreMast;
    GUEST<INTEGER> flags;
    GUEST<INTEGER> cntRel;
    GUEST<INTEGER> maxRel;
    GUEST<INTEGER> cntNRel;
    GUEST<INTEGER> maxNRel;
    GUEST<INTEGER> cntEmpty;
    GUEST<INTEGER> cntHandles;
    GUEST<LONGINT> minCBFree;
    GUEST<ProcPtr> purgeProc;
    GUEST<Ptr> sparePtr;
    GUEST<Ptr> allocPtr;
    GUEST<INTEGER> heapData;
};
typedef Zone *THz;

const LowMemGlobal<Ptr> MemTop { 0x108 }; // MemoryMgr IMII-19 (true);
const LowMemGlobal<Ptr> BufPtr { 0x10C }; // MemoryMgr IMII-19 (true-b);
const LowMemGlobal<Ptr> HeapEnd { 0x114 }; // MemoryMgr IMII-19 (true);
const LowMemGlobal<THz> TheZone { 0x118 }; // MemoryMgr IMII-31 (true);
const LowMemGlobal<Ptr> ApplLimit { 0x130 }; // MemoryMgr IMII-19 (true);
const LowMemGlobal<INTEGER> MemErr { 0x220 }; // MemoryMgr IMIV-80 (true);
const LowMemGlobal<THz> SysZone { 0x2A6 }; // MemoryMgr IMII-19 (true);
const LowMemGlobal<THz> ApplZone { 0x2AA }; // MemoryMgr IMII-19 (true);
const LowMemGlobal<Ptr> ROMBase { 0x2AE }; // MemoryMgr IMIV-236 (true-b);
const LowMemGlobal<Ptr> RAMBase { 0x2B2 }; // MemoryMgr IMI-87 (false);
const LowMemGlobal<Ptr> heapcheck { 0x316 }; // MemoryMgr SysEqu.a (true-b);
const LowMemGlobal<LONGINT> Lo3Bytes { 0x31A }; // MemoryMgr IMI-85 (true);
const LowMemGlobal<LONGINT> MinStack { 0x31E }; // MemoryMgr IMII-17 (true-b);
const LowMemGlobal<LONGINT> DefltStack { 0x322 }; // MemoryMgr IMII-17 (true-b);
const LowMemGlobal<Handle> GZRootHnd { 0x328 }; // MemoryMgr IMI-43 (true);
const LowMemGlobal<Handle> GZMoveHnd { 0x330 }; // MemoryMgr LowMem.h (false);
const LowMemGlobal<ProcPtr> IAZNotify { 0x33C }; // MemoryMgr ThinkC (true-b);
const LowMemGlobal<Ptr> CurrentA5 { 0x904 }; // MemoryMgr IMI-95 (true);
const LowMemGlobal<Ptr> CurStackBase { 0x908 }; // MemoryMgr IMII-19 (true-b);

const LowMemGlobal<Byte[20]> Scratch20 { 0x1E4 }; // MemoryMgr IMI-85 (true);
const LowMemGlobal<Byte[8]> ToolScratch { 0x9CE }; // MemoryMgr IMI-85 (true);
const LowMemGlobal<Byte[8]> Scratch8 { 0x9FA }; // MemoryMgr IMI-85 (true);
const LowMemGlobal<LONGINT> OneOne { 0xA02 }; // MemoryMgr IMI-85 (true);
const LowMemGlobal<LONGINT> MinusOne { 0xA06 }; // MemoryMgr IMI-85 (true);
const LowMemGlobal<Byte[12]> ApplScratch { 0xA78 }; // MemoryMgr IMI-85 (true);

/* traps which can have a `sys' or `clear' bit set */
#define SYSBIT (1 << 10)
#define CLRBIT (1 << 9)

namespace callconv
{
    template<typename Loc> struct ReturnMemErr
    {
        syn68k_addr_t afterwards(syn68k_addr_t ret)
        {
            Loc::set(LM(MemErr));
            return ret;
        }
    };
    template<typename Loc> struct ReturnMemErrConditional
    {
        syn68k_addr_t afterwards(syn68k_addr_t ret)
        {
            auto err = LM(MemErr);
            if(err < 0)
                Loc::set(err);
            return ret;
        }
    };

}

extern Handle _NewEmptyHandle_flags(bool sys_p);
REGISTER_FLAG_TRAP(_NewEmptyHandle_flags, NewEmptyHandle, NewEmptyHandleSys,
    0xA166, Handle(), A0 (TrapBit<SYSBIT>), callconv::ReturnMemErr<D0>);

extern Handle _NewHandle_flags(Size size, bool sys_p, bool clear_p);
REGISTER_2FLAG_TRAP(_NewHandle_flags, NewHandle, NewHandleClear, NewHandleSys, NewHandleSysClear,
    0xA122, Handle(Size), A0 (D0, TrapBit<SYSBIT>, TrapBit<CLRBIT>), callconv::ReturnMemErr<D0>);

extern Handle _RecoverHandle_flags(Ptr p, bool sys_p);
REGISTER_FLAG_TRAP(_RecoverHandle_flags, RecoverHandle, RecoverHandleSys,
    0xA128, Handle(Ptr), A0 (A0, TrapBit<SYSBIT>), callconv::ReturnMemErr<D0>);

extern Ptr _NewPtr_flags(Size size, bool sys_p, bool clear_p);
REGISTER_2FLAG_TRAP(_NewPtr_flags, NewPtr, NewPtrClear, NewPtrSys, NewPtrSysClear,
    0xA11E, Ptr(Size), A0 (D0, TrapBit<SYSBIT>, TrapBit<CLRBIT>), callconv::ReturnMemErr<D0>);

extern int32_t _FreeMem_flags(bool sys_p);
REGISTER_FLAG_TRAP(_FreeMem_flags, FreeMem, FreeMemSys,
    0xA01C, int32_t(), D0 (TrapBit<SYSBIT>));

extern Size _MaxMem_flags(GUEST<Size> *growp, bool sys_p);
REGISTER_FLAG_TRAP(_MaxMem_flags, MaxMem, MaxMemSys,
    0xA11D, Size(GUEST<Size>*), D0 (Out<Size,A0>, TrapBit<SYSBIT>));

extern Size _CompactMem_flags(Size sizeneeded, bool sys_p);
REGISTER_FLAG_TRAP(_CompactMem_flags, CompactMem, CompactMemSys,
    0xA04C, Size(Size), D0 (D0, TrapBit<SYSBIT>));

extern void _ResrvMem_flags(Size needed, bool sys_p);
REGISTER_FLAG_TRAP(_ResrvMem_flags, ReserveMem, ReserveMemSys,
    0xA040, void(Size), void (D0, TrapBit<SYSBIT>), callconv::ReturnMemErr<D0>);

extern void _PurgeMem_flags(Size needed, bool sys_p);
REGISTER_FLAG_TRAP(_PurgeMem_flags, PurgeMem, PurgeMemSys,
    0xA04D, void(Size), void (D0, TrapBit<SYSBIT>), callconv::ReturnMemErr<D0>);

extern Size _MaxBlock_flags(bool sys_p);
REGISTER_FLAG_TRAP(_MaxBlock_flags, MaxBlock, MaxBlockSys,
    0xA061, Size(), D0 (TrapBit<SYSBIT>));

extern void _PurgeSpace_flags(GUEST<Size> *totalp, GUEST<Size> *contigp, bool sys_p);
REGISTER_FLAG_TRAP(_PurgeSpace_flags, PurgeSpace, PurgeSpaceSys,
    0xA062, void(GUEST<Size> *, GUEST<Size> *), void (Out<int32_t,D0>, Out<int32_t,A0>, TrapBit<SYSBIT>));

/* ### cliff bogofunc; should go away */
extern void ROMlib_installhandle(Handle sh, Handle dh);

extern void ROMlib_InitZones();
extern void InitMemory(void *thingOnStack);

extern OSErr C_MemError(void);
NOTRAP_FUNCTION(MemError);

extern SignedByte HGetState(Handle h);
REGISTER_TRAP2(HGetState, 0xA069, D0(A0));
extern void HSetState(Handle h, SignedByte flags);
REGISTER_TRAP2(HSetState, 0xA06A, void(A0,D0), ReturnMemErr<D0>);
extern void HLock(Handle h);
REGISTER_TRAP2(HLock, 0xA029, void(A0), ReturnMemErr<D0>);
extern void HUnlock(Handle h);
REGISTER_TRAP2(HUnlock, 0xA02A, void(A0), ReturnMemErr<D0>);
extern void HPurge(Handle h);
REGISTER_TRAP2(HPurge, 0xA049, void(A0), ReturnMemErr<D0>);
extern void HNoPurge(Handle h);
REGISTER_TRAP2(HNoPurge, 0xA04A, void(A0), ReturnMemErr<D0>);
extern void HSetRBit(Handle h);
REGISTER_TRAP2(HSetRBit, 0xA067, void(A0), ReturnMemErr<D0>);
extern void HClrRBit(Handle h);
REGISTER_TRAP2(HClrRBit, 0xA068, void(A0), ReturnMemErr<D0>);
extern void InitApplZone(void);
REGISTER_TRAP2(InitApplZone, 0xA02C, void (), ReturnMemErr<D0>);
extern void SetApplBase(Ptr newbase);
REGISTER_TRAP2(SetApplBase, 0xA057, void (A0), ReturnMemErr<D0>);
extern void MoreMasters(void);
REGISTER_TRAP2(MoreMasters, 0xA036, void (), ReturnMemErr<D0>);

extern void InitZone(GrowZoneProcPtr pGrowZone, int16_t cMoreMasters,
                     Ptr limitPtr, THz startPtr);

extern THz GetZone(void);
REGISTER_TRAP2(GetZone, 0xA11A, A0 (), ReturnMemErr<D0>);
extern void SetZone(THz hz);
REGISTER_TRAP2(SetZone, 0xA01B, void (A0), ReturnMemErr<D0>);
extern void DisposeHandle(Handle h);
REGISTER_TRAP2(DisposeHandle, 0xA023, void (A0), ReturnMemErr<D0>);
extern Size GetHandleSize(Handle h);
REGISTER_TRAP2(GetHandleSize, 0xA025, D0 (A0), ReturnMemErrConditional<D0>);
extern void SetHandleSize(Handle h, Size newsize);
REGISTER_TRAP2(SetHandleSize, 0xA024, void (A0, D0), ReturnMemErr<D0>);
extern THz HandleZone(Handle h);
REGISTER_TRAP2(HandleZone, 0xA126, A0 (A0), ReturnMemErr<D0>);
extern void ReallocateHandle(Handle h, Size size);
REGISTER_TRAP2(ReallocateHandle, 0xA027, void (A0, D0), ReturnMemErr<D0>);
extern void DisposePtr(Ptr p);
REGISTER_TRAP2(DisposePtr, 0xA01F, void (A0), ReturnMemErr<D0>);
extern Size GetPtrSize(Ptr p);
REGISTER_TRAP2(GetPtrSize, 0xA021, D0 (A0), ReturnMemErrConditional<D0>);
extern void SetPtrSize(Ptr p, Size newsize);
REGISTER_TRAP2(SetPtrSize, 0xA020, void (A0, D0), ReturnMemErr<D0>);
extern THz PtrZone(Ptr p);
REGISTER_TRAP2(PtrZone, 0xA148, A0 (A0), ReturnMemErr<D0>);


extern void BlockMove_the_trap(Ptr src, Ptr dst, Size cnt, bool flush_p);
extern void C_BlockMove(Ptr src, Ptr dst, Size cnt);
extern void C_BlockMoveData(Ptr src, Ptr dst, Size cnt);
REGISTER_TRAP2(BlockMove_the_trap, 0xA02E, void(A0,A1,D0,TrapBit<0x200>), ReturnMemErr<D0>);
NOTRAP_FUNCTION(BlockMove);
NOTRAP_FUNCTION(BlockMoveData);

extern void MaxApplZone(void);
REGISTER_TRAP2(MaxApplZone, 0xA063, void (), ReturnMemErr<D0>);
extern void MoveHHi(Handle h);
REGISTER_TRAP2(MoveHHi, 0xA064, void (A0), ReturnMemErr<D0>);
extern void SetApplLimit(Ptr newlimit);
REGISTER_TRAP2(SetApplLimit, 0xA02D, void (A0), ReturnMemErr<D0>);
extern void SetGrowZone(GrowZoneProcPtr newgz);
REGISTER_TRAP2(SetGrowZone, 0xA04B, void (A0), ReturnMemErr<D0>);
extern void EmptyHandle(Handle h);
REGISTER_TRAP2(EmptyHandle, 0xA02B, void (A0), ReturnMemErr<D0>);

extern void C_HLockHi(Handle h);
NOTRAP_FUNCTION(HLockHi);

extern THz C_SystemZone(void);
NOTRAP_FUNCTION(SystemZone);
extern THz C_ApplicationZone(void);
NOTRAP_FUNCTION(ApplicationZone);

extern Size StackSpace(void);
REGISTER_TRAP2(StackSpace, 0xA065, D0 ());

extern Ptr C_GetApplLimit();
NOTRAP_FUNCTION(GetApplLimit);

extern Ptr C_TopMem();
NOTRAP_FUNCTION(TopMem);

extern Handle C_GZSaveHnd();
NOTRAP_FUNCTION(GZSaveHnd);

DISPATCHER_TRAP(OSDispatch, 0xA88F, StackW);

/* temporary memory functions; see tempmem.c */
extern int32_t C_TempFreeMem(void);
PASCAL_SUBTRAP(TempFreeMem, 0xA88F, 0x0018, OSDispatch);
extern Size C_TempMaxMem(GUEST<Size> *grow);
PASCAL_SUBTRAP(TempMaxMem, 0xA88F, 0x0015, OSDispatch);
extern Ptr C_TempTopMem(void);
PASCAL_SUBTRAP(TempTopMem, 0xA88F, 0x0016, OSDispatch);
extern Handle C_TempNewHandle(Size logical_size, GUEST<OSErr> *result_code);
PASCAL_SUBTRAP(TempNewHandle, 0xA88F, 0x001D, OSDispatch);
extern void C_TempHLock(Handle h, GUEST<OSErr> *result_code);
PASCAL_SUBTRAP(TempHLock, 0xA88F, 0x001E, OSDispatch);
extern void C_TempHUnlock(Handle h, GUEST<OSErr> *result_code);
PASCAL_SUBTRAP(TempHUnlock, 0xA88F, 0x001F, OSDispatch);
extern void C_TempDisposeHandle(Handle h, GUEST<OSErr> *result_code);
PASCAL_SUBTRAP(TempDisposeHandle, 0xA88F, 0x0020, OSDispatch);
}
#endif /* _MEMORY_MGR_H_ */
