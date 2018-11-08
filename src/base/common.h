#if !defined(_COMMON_H_)
#define _COMMON_H_

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 /* evil hackage needed to make SDL happy */
#endif

#include "host-os-config.h"
#include "host-arch-config.h"

#include "rsys/macros.h"
#include "base/functions.h"
#include "base/traps.h"
#include "base/mactype.h"
#include "base/byteswap.h"

#include "ExMacTypes.h"
#include "rsys/slash.h"
#include "error/error.h"
#include "base/lowglobals.h"

#endif /* !_COMMON_H_ */
