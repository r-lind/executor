#if !defined(_BYTESWAP_H_)
#define _BYTESWAP_H_

#if !defined(BIGENDIAN) && !defined(LITTLEENDIAN)
#error "One of BIGENDIAN or LITTLEENDIAN must be #defined"
#endif

#include <stdint.h>
#include <cstddef>
#include <type_traits>
#include "rsys/mactype.h"
#include <syn68k_public.h> /* for ROMlib_offset */

namespace Executor
{

#if defined(BIGENDIAN)

#define CW_RAW(rhs) (rhs)
#define CL_RAW(rhs) (rhs)
#define CWC_RAW(rhs) (rhs)
#define CLC_RAW(rhs) (rhs)

#else /* !defined (BIGENDIAN) */

#if 1
template<class TT>
inline TT CW_RAW(TT n)
{
    return swap16((uint16_t)n);
}
template<class TT>
inline TT CL_RAW(TT n)
{
    return swap32((uint32_t)n);
}

#define CWC_RAW(n) ((std::conditional_t<std::is_signed_v<decltype(n)>, int16_t, uint16_t>)(                       \
    (((((unsigned short)n) << 8) & 0xFF00) \
                   | ((((unsigned short)n) >> 8) & 0x00FF))))
#define CLC_RAW(n) ((std::conditional_t<std::is_signed_v<decltype(n)>, int32_t, uint32_t>)(                       \
    ((((unsigned int)((n) | 0) & 0x000000FF) << 24) \
          | (((unsigned int)(n)&0x0000FF00) << 8)        \
          | (((unsigned int)(n)&0x00FF0000) >> 8)        \
          | (((unsigned int)(n)&0xFF000000)              \
             >> 24))))
#else
#define CW_RAW(x) swap16(x)
#define CL_RAW(x) swap32(x)
#define CWC_RAW(x) swap16(x)
#define CLC_RAW(x) swap32(x)
#endif

#endif /* !defined (BIGENDIAN) */



}

#endif /* !_BYTESWAP_H_ */
