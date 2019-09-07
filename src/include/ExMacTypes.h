#if !defined(_MACTYPES_H_)
#define _MACTYPES_H_

/*
 * Copyright 1986, 1989, 1990, 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include <base/mactype.h>
#include <base/byteswap.h>
#include <cassert>

namespace Executor
{
typedef int16_t INTEGER;
typedef int32_t LONGINT;
typedef uint32_t ULONGINT;
typedef int8_t BOOLEAN;

typedef int16_t CharParameter; /* very important not to use this as char */

typedef int8_t SignedByte;
typedef uint8_t Byte;
typedef char *Ptr;
typedef GUEST<Ptr> *Handle;

typedef int8_t Boolean;

typedef int8_t SInt8;
typedef uint8_t UInt8;
typedef int16_t SInt16;
typedef uint16_t UInt16;
typedef int32_t SInt32;
typedef uint32_t UInt32;

typedef Byte Str15[16];
typedef Byte Str31[32];
typedef Byte Str32[33];
typedef Byte Str63[64];
typedef Byte Str255[256];
typedef Byte *StringPtr;
typedef const unsigned char *ConstStringPtr;
typedef ConstStringPtr ConstStr255Param;

typedef GUEST<StringPtr> *StringHandle;

struct RoutineDescriptor;
typedef RoutineDescriptor *UniversalProcPtr;

typedef LONGINT Fixed, Fract;

/* SmallFract represnts values between 0 and 65535 */
typedef unsigned short SmallFract;

enum
{
    MaxSmallFract = 0xFFFF,
};

typedef double Extended;

typedef LONGINT Size;

typedef INTEGER OSErr;


BEGIN_EXECUTOR_ONLY
class OSErrorException : public std::runtime_error
{
public:
    OSErr code;

    OSErrorException(OSErr err) : std::runtime_error("oserror"), code(err) {}
};
END_EXECUTOR_ONLY

typedef LONGINT OSType;
typedef LONGINT ResType;

BEGIN_EXECUTOR_ONLY
constexpr OSType operator"" _4(const char*x, size_t n)
{
    assert(n == 4);
    return (uint8_t(x[0]) << 24) | (uint8_t(x[1]) << 16) | (uint8_t(x[2]) << 8) | uint8_t(x[3]);
}
END_EXECUTOR_ONLY

union QElem;
typedef QElem *QElemPtr;

struct QHdr
{
    GUEST_STRUCT;
    GUEST<INTEGER> qFlags;
    GUEST<QElemPtr> qHead;
    GUEST<QElemPtr> qTail;
};
typedef QHdr *QHdrPtr;

enum
{
    noErr = 0,
    paramErr = (-50),
};

#if 0
// custom case in mactype.h
/* from Quickdraw.h */
struct Point { GUEST_STRUCT;
    GUEST< INTEGER> v;
    GUEST< INTEGER> h;
};
#endif

struct Rect
{
    GUEST_STRUCT;
    GUEST<INTEGER> top;
    GUEST<INTEGER> left;
    GUEST<INTEGER> bottom;
    GUEST<INTEGER> right;

};

typedef Rect *RectPtr;

BEGIN_EXECUTOR_ONLY
inline short RECT_WIDTH(const Rect *r)
{
    return r->right - r->left;
}
inline short RECT_HEIGHT(const Rect *r)
{
    return r->bottom - r->top;
}
END_EXECUTOR_ONLY

/* from IntlUtil.h */
typedef INTEGER ScriptCode;
typedef INTEGER LangCode;

static_assert(sizeof(QHdr) == 10);
static_assert(sizeof(Point) == 4);
static_assert(sizeof(Rect) == 8);
}
#endif /* _MACTYPES_H_ */
