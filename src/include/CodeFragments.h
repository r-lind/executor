#if !defined(_cfm_h_)
#define _cfm_h_

#include <FileMgr.h>
#include <PEFBinaryFormat.h>

/*
 * Copyright 2000 by Abacus Research and Development, Inc.
 * All rights reserved.
 *
 */

#define MODULE_NAME CodeFragments
#include <base/api-module.h>

namespace Executor
{
BEGIN_EXECUTOR_ONLY
const std::nullptr_t kUnresolvedCFragSymbolAddress = nullptr;
END_EXECUTOR_ONLY

struct cfrg_resource_t
{
    GUEST_STRUCT;
    GUEST<uint32_t> reserved0;
    GUEST<uint32_t> reserved1;
    GUEST<uint32_t> version;
    GUEST<uint32_t> reserved2;
    GUEST<uint32_t> reserved3;
    GUEST<uint32_t> reserved4;
    GUEST<uint32_t> reserved5;
    GUEST<int32_t> n_descripts;
};

#define CFRG_VERSION(cfrg) ((cfgr)->version)


#define CFRG_N_DESCRIPTS(cfrg) ((cfrg)->n_descripts)


struct cfir_t
{
    GUEST_STRUCT;
    GUEST<OSType> isa;
    GUEST<uint32_t> update_level;
    GUEST<uint32_t> current_version;
    GUEST<uint32_t> oldest_definition_version;
    GUEST<uint32_t> stack_size;
    GUEST<int16_t> appl_library_dir;
    GUEST<uint8_t> fragment_type;
    GUEST<uint8_t> fragment_location;
    GUEST<int32_t> offset_to_fragment;
    GUEST<int32_t> fragment_length;
    GUEST<uint32_t> reserved0;
    GUEST<uint32_t> reserved1;
    GUEST<uint16_t> cfir_length;
    GUEST<unsigned char[1]> name;
};

#define CFIR_ISA(cfir) ((cfir)->isa)
#define CFIR_TYPE(cfir) ((cfir)->fragment_type)
#define CFIR_LOCATION(cfir) ((cfir)->fragment_location)
#define CFIR_LENGTH(cfir) ((cfir)->cfir_length)
#define CFIR_OFFSET_TO_FRAGMENT(cfir) ((cfir)->offset_to_fragment)
#define CFIR_FRAGMENT_LENGTH(cfir) ((cfir)->fragment_length)
#define CFIR_NAME(cfir) ((cfir)->name)

enum
{
    kImportLibraryCFrag,
    kApplicationCFrag,
    kDropInAdditionCFrag,
    kStubLibraryCFrag,
    kWeakStubLibraryCFrag,
};

enum
{
    kWholeFork = 0
};

enum
{
    kInMem,
    kOnDiskFlat,
    kOnDiskSegmented,
};

enum
{
    kPowerPCArch = "pwpc"_4,
    kMotorola68KArch = "m68k"_4,
};

enum {
    kLoadLib = 1, /* deprecated */
    kReferenceCFrag = 1,
    kFindLib = 2,
    kLoadNewCopy = 5,
};
typedef uint32_t LoadFlags;

struct MemFragment
{
    GUEST_STRUCT;
    GUEST<Ptr> address;
    GUEST<uint32_t> length;
    GUEST<BOOLEAN> inPlace;
    GUEST<uint8_t> reservedA;
    GUEST<uint16_t> reservedB;
};

struct DiskFragment
{
    GUEST_STRUCT;
    GUEST<FSSpecPtr> fileSpec;
    GUEST<uint32_t> offset;
    GUEST<uint32_t> length;
};

struct SegmentedFragment
{
    GUEST_STRUCT;
    GUEST<FSSpecPtr> fileSpec;
    GUEST<OSType> rsrcType;
    GUEST<INTEGER> rsrcID;
    GUEST<uint16_t> reservedA;
};

typedef struct FragmentLocator
{
    GUEST_STRUCT;
    GUEST<uint32_t> where;
    union {
        MemFragment inMem;
        DiskFragment onDisk;
        SegmentedFragment inSegs;
    } u;
} FragmentLocator;

struct InitBlock
{
    GUEST_STRUCT;
    GUEST<uint32_t> contextID;
    GUEST<uint32_t> closureID;
    GUEST<uint32_t> connectionID;
    GUEST<FragmentLocator> fragLocator;
    GUEST<StringPtr> libName;
    GUEST<uint32_t> reserved4;
};

typedef struct CFragConnection *ConnectionID;
typedef struct CFragClosure *CFragClosureID;

DISPATCHER_TRAP(CodeFragmentDispatch, 0xAA5A, StackW);

extern OSErr C_GetDiskFragment(FSSpecPtr fsp, LONGINT offset, LONGINT length,
                               Str63 fragname, LoadFlags flags,
                               GUEST<ConnectionID> *connp, GUEST<Ptr> *mainAddrp,
                               Str255 errname);
PASCAL_SUBTRAP(GetDiskFragment, 0xAA5A, 0x0002, CodeFragmentDispatch);

typedef uint8_t SymClass;

extern OSErr C_FindSymbol(ConnectionID connID, Str255 symName, GUEST<Ptr> *symAddr,
                          SymClass *symClass);
PASCAL_SUBTRAP(FindSymbol, 0xAA5A, 0x0005, CodeFragmentDispatch);


extern OSErr C_CloseConnection(ConnectionID *cidp);
PASCAL_SUBTRAP(CloseConnection, 0xAA5A, 0x0004, CodeFragmentDispatch);

extern OSErr C_GetSharedLibrary(Str63 library, OSType arch,
                                LoadFlags loadflags,
                                GUEST<ConnectionID> *cidp, GUEST<Ptr> *mainaddrp,
                                Str255 errName);
PASCAL_SUBTRAP(GetSharedLibrary, 0xAA5A, 0x0001, CodeFragmentDispatch);

extern OSErr C_GetMemFragment(void *addr, uint32_t length, Str63 fragname,
                              LoadFlags flags, GUEST<ConnectionID> *connp,
                              GUEST<Ptr> *mainAddrp, Str255 errname);
PASCAL_SUBTRAP(GetMemFragment, 0xAA5A, 0x0003, CodeFragmentDispatch);

extern OSErr C_CountSymbols(ConnectionID id, GUEST<LONGINT> *countp);
PASCAL_SUBTRAP(CountSymbols, 0xAA5A, 0x0006, CodeFragmentDispatch);

extern OSErr C_GetIndSymbol(ConnectionID id, LONGINT index,
                            Str255 name, GUEST<Ptr> *addrp,
                            SymClass *classp);
PASCAL_SUBTRAP(GetIndSymbol, 0xAA5A, 0x0007, CodeFragmentDispatch);


static_assert(sizeof(cfrg_resource_t) == 32);
static_assert(sizeof(cfir_t) == 44);
static_assert(sizeof(MemFragment) == 12);
static_assert(sizeof(DiskFragment) == 12);
static_assert(sizeof(SegmentedFragment) == 12);
static_assert(sizeof(FragmentLocator) == 16);
static_assert(sizeof(InitBlock) == 36);
}

#endif
