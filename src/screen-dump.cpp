/* Copyright 1994, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* screen-dump.c; dump the mac screen to a tiff file */

#include <rsys/screen-dump.h>

#include <stdarg.h>

#include <QuickDraw.h>
#include <CQuickDraw.h>
#include <MemoryMgr.h>

#include <quickdraw/cquick.h>
#include <mman/mman.h>
#include <mman/tempalloc.h>
#include <prefs/prefs.h>
#include <util/uniquefile.h>
#include <file/file.h>
#include <rsys/paths.h>
#include <rsys/unixio.h>


namespace
{
struct header
{
    int16_t byte_order;
    int16_t magic_number;
    int32_t ifd_offset;
};

struct __attribute__((packed)) directory_entry
{
    int16_t tag;
    int16_t type;
    int32_t count;
    union {
        int16_t value_offset_16;
        int32_t value_offset;
    };
};

struct __attribute__((packed)) ifd 
{
    int16_t count;
    struct directory_entry entries[1];
};

static_assert(sizeof(header) == 8);
static_assert(sizeof(directory_entry) == 12);
static_assert(sizeof(ifd) == 14);
}

using namespace Executor;

#define II_little_endian 0x4949
#define MM_big_endian 0x4D4D

#define BYTE 1
#define ASCII 2
#define SHORT 3
#define LONG 4
#define RATIONAL 5

#define SBYTE 6
#undef UNDEFINED /* svgalib defines this. */
#define UNDEFINED 7
#define SSHORT 8
#define SLONG 9
#define SRATIONAL 10

#define FLOAT 11
#define DOUBLE 12

#define ImageWidth 256
#define ImageLength 257
#define BitsPerSample 258
#define Compression 259
#define PhotometricInterpretation 262
#define StripOffsets 273
#define RowsPerStrip 278
#define StripByteCounts 279
#define XResolution 282
#define YResolution 283
#define ResolutionUnit 296
#define ColorMap 320

static void
ifd_add_entry(struct ifd *ifd, int tag, int type, ...)
{
    va_list ap;
    struct directory_entry *current_entry;

    va_start(ap, type);

    current_entry = &ifd->entries[ifd->count];
    ifd->count++;

    /* default values */
    current_entry->tag = tag;
    current_entry->type = type;
    current_entry->count = 1;

    switch(type)
    {
        case SHORT:
            current_entry->value_offset_16 = (int16_t)va_arg(ap, int32_t);
            break;
        case LONG:
            current_entry->value_offset = va_arg(ap, int32_t);
            break;
        case -1:
            current_entry->type = va_arg(ap, int32_t);
            current_entry->count = va_arg(ap, int32_t);
            current_entry->value_offset = va_arg(ap, int32_t);
            break;
        default:
            gui_fatal("unknown tag type");
            break;
    }
    va_end(ap);
}

static void
dump_indirect_pm(PixMap *pm)
{
    PixMap *tiff_pm;

    void *fbuf;
    int fbuf_size;
    int row_bytes;
    int width, height;
    int bpp;

    struct header *header;
    int header_size;

    static const int n_ifd_entries = 10;
    struct ifd *ifd;
    int ifd_size;

    int8_t *tif;
    int tif_size;

    int32_t *strip_offsets;
    int strip_offsets_size;
    int strip_offsets_offset;

    int32_t *strip_byte_counts;
    int strip_byte_counts_size;
    int strip_byte_counts_offset;

    int16_t *color_map;
    int color_map_size;
    int color_map_offset;

    int i;
    int retval;
    int fd;

    Str255 temp_file_name;

    TEMP_ALLOC_DECL(temp_fbuf_bits);
    TEMP_ALLOC_DECL(temp_tif_bits);

    height = RECT_HEIGHT(&pm->bounds);
    width = RECT_WIDTH(&pm->bounds);

    bpp = pm->pixelSize;

    if(bpp != 8 && bpp != 4)
    {
        tiff_pm = (PixMap *)alloca(sizeof *tiff_pm);

        /* compute the bpp of the pixmap that will be converted to a tif */
        bpp = bpp <= 4 ? 4 : 8;

        fbuf_size = width * height;
        TEMP_ALLOC_ALLOCATE(fbuf, temp_fbuf_bits, fbuf_size);
        row_bytes = (width * bpp + 31) / 32 * 4;

        tiff_pm->baseAddr = (Ptr)fbuf;
        tiff_pm->rowBytes = row_bytes | PIXMAP_DEFAULT_ROW_BYTES;
        tiff_pm->bounds = pm->bounds;

        pixmap_set_pixel_fields(tiff_pm, bpp);

        tiff_pm->pmTable = pm->pmTable;

        CopyBits((BitMap *)pm, (BitMap *)tiff_pm,
                 &pm->bounds, &tiff_pm->bounds,
                 srcCopy, nullptr);
    }
    else
    {
        tiff_pm = pm;
        fbuf = BITMAP_BASEADDR(pm);
        row_bytes = BITMAP_ROWBYTES(pm);
        fbuf_size = height * row_bytes;
    }

    header_size = sizeof *header;
    ifd_size = sizeof *ifd + (n_ifd_entries - 1) * sizeof *ifd->entries 
        + 4;    // offset to next TIFF directory, remains 0

    color_map_size = 3 * (1 << bpp) * sizeof *color_map;
    color_map_offset = header_size + ifd_size;

    strip_offsets_size = sizeof *strip_offsets * height;
    strip_offsets_offset = color_map_offset + color_map_size;

    strip_byte_counts_size = sizeof *strip_byte_counts * height;
    strip_byte_counts_offset = strip_offsets_offset + strip_offsets_size;

    tif_size = (header_size + ifd_size + color_map_size
                + strip_offsets_size + strip_byte_counts_size);
    TEMP_ALLOC_ALLOCATE(tif, temp_tif_bits, tif_size);

    header = (struct header *)&tif[0];
    ifd = (struct ifd *)&tif[header_size];
    color_map = (int16_t *)&tif[color_map_offset];
    strip_offsets = (int32_t *)&tif[strip_offsets_offset];
    strip_byte_counts = (int32_t *)&tif[strip_byte_counts_offset];

#if defined(LITTLEENDIAN)
    header->byte_order = II_little_endian;
#else
    header->byte_order = MM_big_endian;
#endif

    header->magic_number = 42;
    header->ifd_offset = header_size;

    memset(ifd, 0, ifd_size);

    /* NOTE: order matters here, the ifd directory entries must be
     sorted by tag */
    /* NOTE: if you change the number of entries here; make sure to
     update `n_ifd_entries' appropriately */
    ifd_add_entry(ifd, ImageWidth, SHORT, width);
    ifd_add_entry(ifd, ImageLength, SHORT, height);
    ifd_add_entry(ifd, BitsPerSample, SHORT, bpp);
    ifd_add_entry(ifd, Compression, SHORT, 1);
    ifd_add_entry(ifd, PhotometricInterpretation, SHORT, 3);
    ifd_add_entry(ifd, StripOffsets, -1,
                  LONG, height,
                  strip_offsets_offset);
    ifd_add_entry(ifd, RowsPerStrip, SHORT, 1);
    ifd_add_entry(ifd, StripByteCounts, -1,
                  LONG, height, strip_byte_counts_offset);

#if 0
  ifd_add_entry (ifd, XResolution,		RATIONAL, 1, 1);
  ifd_add_entry (ifd, YResolution,		RATIONAL, 1, 1);
#endif
    ifd_add_entry(ifd, ResolutionUnit, SHORT, 1);
    ifd_add_entry(ifd, ColorMap, -1,
                  SHORT, 3 * (1 << bpp), color_map_offset);

    gui_assert(ifd->count == n_ifd_entries);

    {
        ColorSpec *color_table;
        int color_table_size;

        memset(color_map, 0xFF, color_map_size);

        color_table_size = CTAB_SIZE(tiff_pm->pmTable);
        color_table = CTAB_TABLE(tiff_pm->pmTable);

        for(i = 0; i <= color_table_size; i++)
        {
            color_map[i] = color_table[i].rgb.red;
            color_map[i + (1 << bpp)] = color_table[i].rgb.green;
            color_map[i + (2 << bpp)] = color_table[i].rgb.blue;
        }
    }

    for(i = 0; i < height; i++)
        strip_offsets[i] = tif_size + row_bytes * i;

    for(i = 0; i < height; i++)
        strip_byte_counts[i] = row_bytes;

    /* now open up a file and write this sucker out */
    if(!unique_file_name(ROMlib_ScreenDumpFile.c_str(), "excscrn*.tif",
                         temp_file_name))
        return;

    fd = open((char *)temp_file_name + 1,
               O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0666);
    if(fd == -1)
    {
        warning_errno("open `%s' failed", temp_file_name + 1);
        return;
    }

    retval = write(fd, tif, tif_size);
    if(retval == -1 || retval != tif_size)
    {
        warning_errno("`write (..., tif, ...)' failed");
        goto done;
    }

    retval = write(fd, fbuf, fbuf_size);
    if(retval == -1 || retval != fbuf_size)
    {
        warning_errno("`write (..., fbuf, ...)' failed");
        goto done;
    }

done:
    close(fd);
}

static void
dump_direct_pm(PixMap *pm)
{
    /* ### fix this! */
    warning_unimplemented("can't dump direct pixel pixmap");
}

typedef void (*dump_fn_t)(PixMap *);

dump_fn_t dump_fns[] = {
    dump_indirect_pm, dump_indirect_pm,
    dump_indirect_pm, dump_indirect_pm,
    dump_direct_pm, dump_direct_pm,
};

void Executor::do_dump_screen()
{
    GDHandle gd;
    PixMapHandle gd_pmh;

    gd = LM(MainDevice);
    gd_pmh = GD_PMAP(gd);

    HLockGuard guard(gd_pmh);
    PixMap *gd_pm;
    int log2_bpp;

    gd_pm = *gd_pmh;
    log2_bpp = ROMlib_log2[gd_pm->pixelSize];

    (*dump_fns[log2_bpp])(gd_pm);
}
