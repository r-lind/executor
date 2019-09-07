#if !defined(_MACROS_H_)
#define _MACROS_H_

#if !defined(PACKED)
#ifdef _MSC_VER
#define PACKED /* FIXME: check where this is used, we might need somethig */
#else
#define PACKED __attribute__((packed))
#endif
#endif

#if !defined(U)
#define U(c) ((uint8_t)(c))
#endif

#if !defined(ALLOCABEGIN)
#define ALLOCABEGIN /* nothing */
#define ALLOCA(n) alloca(n)
#define ALLOCAEND /* nothing */
#endif

#define str255assign(d, s) \
    (memcpy(d, s, uint8_t((s)[0]) + 1))

#endif /* !_MACROS_H_ */
