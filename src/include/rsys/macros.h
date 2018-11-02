#if !defined(_MACROS_H_)
#define _MACROS_H_

#if !defined(NELEM)
#define NELEM(s) (sizeof(s) / sizeof(s)[0])
#endif

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

#define PATASSIGN(dest, src)                                 \
    (((uint32_t *)(dest))[0] = ((const uint32_t *)(src))[0], \
     ((uint32_t *)(dest))[1] = ((const uint32_t *)(src))[1])

#define PATTERNS_EQUAL_P(p1, p2)                                \
    (((const uint32_t *)(p1))[0] == ((const uint32_t *)(p2))[0] \
     && ((const uint32_t *)(p1))[1] == ((const uint32_t *)(p2))[1])

#define FOURCC(a, b, c, d) ((((uint32_t)(uint8_t)(a)) << 24)   \
                       | (((uint32_t)(uint8_t)(b)) << 16) \
                       | (((uint32_t)(uint8_t)(c)) << 8)  \
                       | (((uint32_t)(uint8_t)(d)) << 0))

#define TICK(str) FOURCC(str[0], str[1], str[2], str[3])

#endif /* !_MACROS_H_ */
