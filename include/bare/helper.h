#ifndef BARE_HELPER_H
#define BARE_HELPER_H

#define BARE_CONCAT(a, b) a##b

#define BARE_STRING_LITERAL(x) #x
#define BARE_STRING(x)         BARE_STRING_LITERAL(x)

#ifdef __cplusplus
#define BARE_EXTERN_C_START extern "C" {
#define BARE_EXTERN_C_END   }
#else
#define BARE_EXTERN_C_START
#define BARE_EXTERN_C_END
#endif

#if defined(_WIN32)
#if defined(BARE_BUILDING)
#define BARE_EXPORT __declspec(dllexport)
#elif defined(BARE_SHARED)
#define BARE_EXPORT __declspec(dllimport)
#else
#define BARE_EXPORT
#endif
#elif defined(__GNUC__)
#define BARE_EXPORT __attribute__((visibility("default")))
#else
#define BARE_EXPORT
#endif

#endif // BARE_HELPER_H
