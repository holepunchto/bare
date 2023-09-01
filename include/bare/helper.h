#ifndef BARE_HELPER_H
#define BARE_HELPER_H

#define BARE_CONCAT(a, b) a##b

#define BARE_STRING_LITERAL(x) #x
#define BARE_STRING(x)         BARE_STRING_LITERAL(x)

#endif // BARE_HELPER_H
