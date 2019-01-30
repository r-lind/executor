#if !defined(_RSYS_VERSION_H_)
#define _RSYS_VERSION_H_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(EXECUTOR_VERSION)
#define EXECUTOR_VERSION "" /* don't forget EXECUTOR_VERSION_NUMERIC */
#endif

#define SYSTEM_VERSION_ENCODING(m, n, x, b) ((m)*1000000                         \
                                             + (n)*10000                         \
                                             + ((x) ? ((x) - 'a' + 1) * 100 : 0) \
                                             + (b))

#if !defined(EXECUTOR_VERSION_NUMERIC)
#define EXECUTOR_VERSION_NUMERIC SYSTEM_VERSION_ENCODING(3, 0, 0, 0)
#endif

#define MINIMUM_SYSTEM_FILE_NEEDED SYSTEM_VERSION_ENCODING(2, 0, 0, 3)

extern const char ROMlib_executor_version[];
extern const char *ROMlib_executor_full_name;

extern void ROMlib_set_system_version(uint32_t version);

#ifdef __cplusplus
}
#endif

#endif /* !_RSYS_VERSION_H_ */
