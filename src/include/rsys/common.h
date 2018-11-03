#if !defined(_COMMON_H_)
#define _COMMON_H_

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 /* evil hackage needed to make SDL happy */
#endif

#include "host-os-config.h"
#include "host-arch-config.h"

#include "rsys/macros.h"
#include "rsys/functions.h"
#include "rsys/traps.h"
#include "rsys/mactype.h"
#include "rsys/byteswap.h"

#include "ExMacTypes.h"
#include "rsys/slash.h"
#include "rsys/error.h"
#include "rsys/lowglobals.h"

#endif /* !_COMMON_H_ */
