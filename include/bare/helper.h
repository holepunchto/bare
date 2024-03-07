#ifndef BARE_HELPER_H
#define BARE_HELPER_H

#define BARE_CONCAT(a, b) a##b

#define BARE_STRING_LITERAL(x) #x
#define BARE_STRING(x)         BARE_STRING_LITERAL(x)

#ifdef __cplusplus
#define BARE_EXTERN_C_START extern "C" {
#define BARE_EXTERN_C_END }
#else
#define BARE_EXTERN_C_START
#define BARE_EXTERN_C_END
#endif

#endif // BARE_HELPER_H
