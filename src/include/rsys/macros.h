#if !defined(_MACROS_H_)
#define _MACROS_H_

#if 0 /* This should be defined by target-os-config.h */
      /* if we define it here, then we get warning messages when we */
      /* include target-os-config.h after this file has been included */
      /* so it's better to just get it from there in the first place */

#if !defined(offsetof)
#define offsetof(t, f) ((int)&((t *)0)->f)
#endif

#endif

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
    (BlockMove((Ptr)(s), (Ptr)(d), (Size)((unsigned char)(s)[0]) + 1))

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

#define TICK(str) (((LONGINT)(unsigned char)str[0] << 24) | ((LONGINT)(unsigned char)str[1] << 16) | ((LONGINT)(unsigned char)str[2] << 8) | ((LONGINT)(unsigned char)str[3] << 0))

#if 0
#if !defined(LITTLEENDIAN)

#define TICKX(str) (TICK(str))

#else /* defined(LITTLEENDIAN) */

#define TICKX(str) (((LONGINT)(unsigned char)str[0] << 0) | ((LONGINT)(unsigned char)str[1] << 8) | ((LONGINT)(unsigned char)str[2] << 16) | ((LONGINT)(unsigned char)str[3] << 24))

#endif /* defined(LITTLEENDIAN) */
#else
#define TICKX(str) CLC(TICK(str))
#endif

#endif /* !_MACROS_H_ */
