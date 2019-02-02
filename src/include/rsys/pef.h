#if !defined(_pef_h_)
#define _pef_h_

#include "rsys/cfm.h"

/*
 * Copyright 1999-2000 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */
namespace Executor
{
typedef struct PEFContainerHeader
{
    GUEST_STRUCT;
    GUEST<OSType> tag1;
    GUEST<OSType> tag2;
    GUEST<OSType> architecture;
    GUEST<uint32_t> formatVersion;
    GUEST<uint32_t> dateTimeStamp;
    GUEST<uint32_t> oldDefVersion;
    GUEST<uint32_t> oldImpVersion;
    GUEST<uint32_t> currentVersion;
    GUEST<uint16_t> sectionCount;
    GUEST<uint16_t> instSectionCount;
    GUEST<uint32_t> reservedA;
} PEFContainerHeader_t;

#define PEF_CONTAINER_TAG1_X(p) ((p)->tag1)
#define PEF_CONTAINER_TAG1(p) (PEF_CONTAINER_TAG1_X(p))

#define PEF_CONTAINER_TAG2_X(p) ((p)->tag2)
#define PEF_CONTAINER_TAG2(p) (PEF_CONTAINER_TAG2_X(p))

#define PEF_CONTAINER_ARCHITECTURE(p) ((p)->architecture)


#define PEF_CONTAINER_FORMAT_VERSION(p) ((p)->formatVersion)


#define PEF_CONTAINER_SECTION_COUNT(p) ((p)->sectionCount)


#define PEF_CONTAINER_INSTSECTION_COUNT(p) ((p)->instSectionCount)


#define PEF_CONTAINER_DATE(p) ((p)->dateTimeStamp)

#define PEF_CONTAINER_OLD_DEV_VERS(p) ((p)->oldDefVersion)


#define PEF_CONTAINER_OLD_IMP_VERS(p) ((p)->oldImpVersion)


#define PEF_CONTAINER_CURRENT_VERS(p) ((p)->currentVersion)


typedef struct PEFSectionHeader
{
    GUEST_STRUCT;
    GUEST<int32_t> nameOffset;
    GUEST<uint32_t> defaultAddress;
    GUEST<uint32_t> totalSize;
    GUEST<uint32_t> unpackedSize;
    GUEST<uint32_t> packedSize;
    GUEST<uint32_t> containerOffset;
    GUEST<uint8_t> sectionKind;
    GUEST<uint8_t> shareKind;
    GUEST<uint8_t> alignment;
    GUEST<uint8_t> reservedA;
} PEFSectionHeader_t;

#define PEFSH_DEFAULT_ADDRESS(p) ((p)->defaultAddress)


#define PEFSH_TOTAL_SIZE(p) ((p)->totalSize)


#define PEFSH_UNPACKED_SIZE(p) ((p)->unpackedSize)


#define PEFSH_PACKED_SIZE(p) ((p)->packedSize)


#define PEFSH_CONTAINER_OFFSET(p) ((p)->containerOffset)


#define PEFSH_SECTION_KIND(p) ((p)->sectionKind)
#define PEFSH_SHARE_KIND(p) ((p)->shareKind)
#define PEFSH_ALIGNMENT(p) (1 << (p)->alignment)

typedef struct PEFLoaderInfoHeader
{
    GUEST_STRUCT;
    GUEST<int32_t> mainSection;
    GUEST<uint32_t> mainOffset;
    GUEST<int32_t> initSection;
    GUEST<uint32_t> initOffset;
    GUEST<int32_t> termSection;
    GUEST<uint32_t> termOffset;
    GUEST<uint32_t> importedLibraryCount;
    GUEST<uint32_t> totalImportedSymbolCount;
    GUEST<uint32_t> relocSectionCount;
    GUEST<uint32_t> relocInstrOffset;
    GUEST<uint32_t> loaderStringsOffset;
    GUEST<uint32_t> exportHashOffset;
    GUEST<uint32_t> exportHashTablePower;
    GUEST<uint32_t> exportedSymbolCount;
} PEFLoaderInfoHeader_t;

#define PEFLIH_MAIN_SECTION(p) ((p)->mainSection)


#define PEFLIH_MAIN_OFFSET(p) ((p)->mainOffset)


#define PEFLIH_INIT_SECTION(p) ((p)->initSection)


#define PEFLIH_INIT_OFFSET(p) ((p)->initOffset)


#define PEFLIH_TERM_SECTION(p) ((p)->termSection)


#define PEFLIH_TERM_OFFSET(p) ((p)->termOffset)


#define PEFLIH_IMPORTED_LIBRARY_COUNT(p) ((p)->importedLibraryCount)


#define PEFLIH_IMPORTED_SYMBOL_COUNT(p) ((p)->totalImportedSymbolCount)


#define PEFLIH_RELOC_SECTION_COUNT(p) ((p)->relocSectionCount)


#define PEFLIH_RELOC_INSTR_OFFSET(p) ((p)->relocInstrOffset)


#define PEFLIH_STRINGS_OFFSET(p) ((p)->loaderStringsOffset)


#define PEFLIH_HASH_OFFSET(p) ((p)->exportHashOffset)


#define PEFLIH_HASH_TABLE_POWER(p) ((p)->exportHashTablePower)


#define PEFLIH_SYMBOL_COUNT(p) ((p)->exportedSymbolCount)


typedef struct PEFImportedLibrary
{
    GUEST_STRUCT;
    GUEST<uint32_t> nameOffset;
    GUEST<uint32_t> oldImpVersion;
    GUEST<uint32_t> currentVersion;
    GUEST<uint32_t> importedSymbolCount;
    GUEST<uint32_t> firstImportedSymbol;
    GUEST<uint8_t> options;
    GUEST<uint8_t> reservedA;
    GUEST<uint16_t> reservedB;
} PEFImportedLibrary_t;

#define PEFIL_NAME_OFFSET(p) ((p)->nameOffset)


#define PEFIL_SYMBOL_COUNT(p) ((p)->importedSymbolCount)

#define PEFIL_FIRST_SYMBOL(p) ((p)->firstImportedSymbol)

typedef struct PEFLoaderRelocationHeader
{
    GUEST_STRUCT;
    GUEST<uint16_t> sectionIndex;
    GUEST<uint16_t> reservedA;
    GUEST<uint32_t> relocCount;
    GUEST<uint32_t> firstRelocOffset;
} PEFLoaderRelocationHeader_t;

#define PEFRLH_RELOC_COUNT(p) ((p)->relocCount)


#define PEFRLH_FIRST_RELOC_OFFSET(p) ((p)->firstRelocOffset)


#define PEFRLH_SECTION_INDEX(p) ((p)->sectionIndex)


enum
{
    kExponentLimit = 16,
    kAverageChainLimit = 10,
};

enum
{
    kPEFHashLengthShift = 16,
    kPEFHashValueMask = 0xFFFF,
};

typedef uint32_t hash_table_entry_t;

enum
{
    FIRST_INDEX_SHIFT = 0,
    FIRST_INDEX_MASK = 0x3FFFF,
    CHAIN_COUNT_SHIFT = 18,
    CHAIN_COUNT_MASK = 0x3FFF,
};

struct PEFExportedSymbol
{
    GUEST_STRUCT;
    GUEST<uint32_t> classAndName;
    GUEST<uint32_t> symbolValue;
    GUEST<int16_t> sectionIndex;
};

#define PEFEXS_CLASS_AND_NAME(p) ((p)->classAndName)


#define PEFEXS_NAME(p) (PEFEXS_CLASS_AND_NAME(p) & 0xffffff)

#define PEFEXS_SYMBOL_VALUE(p) ((p)->symbolValue)


#define PEFEXS_SECTION_INDEX(p) ((p)->sectionIndex)


enum
{
    NAME_MASK = 0xFFFFFF,
};

enum
{
    kPEFCodeSymbol,
    kPEFDataSymbol,
    kPEFTVectSymbol,
    kPEFTOCSymbol,
    kPEFGlueSymbol,
};

#if 0
typedef struct pef_hash
{
  uint32_t n_symbols; /* exportedSymbolCount */
  uint32_t n_hash_entries; /* 1 << exportHashTablePower */
  hash_table_entry_t *hash_entries; /* exportHashOffset */
  uint32_t *export_key_table; /* hash_entries + n_hash_entries */
  PEFExportedSymbol *symbol_table; /* hash_entries + 2 * n_hash_entries */
  const char *symbol_names; /* loaderStringsOffset */
}
pef_hash_t;
#endif

extern PEFLoaderInfoHeader_t *ROMlib_build_pef_hash(const map_entry_t table[],
                                                    int count);

static_assert(sizeof(PEFContainerHeader_t) == 40);
static_assert(sizeof(PEFSectionHeader_t) == 28);
static_assert(sizeof(PEFLoaderInfoHeader_t) == 56);
static_assert(sizeof(PEFImportedLibrary_t) == 24);
static_assert(sizeof(PEFLoaderRelocationHeader_t) == 12);
static_assert(sizeof(PEFExportedSymbol) == 10);
}
#endif
