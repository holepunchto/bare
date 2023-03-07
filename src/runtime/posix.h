#ifndef PEAR_RUNTIME_POSIX_H
#define PEAR_RUNTIME_POSIX_H

#include <stdarg.h>
#include <stdio.h>

int
pear_runtime__print_info (const char *format, ...) {
  va_list args;
  va_start(args, format);

  return vfprintf(stdout, format, args);
}

int
pear_runtime__print_error (const char *format, ...) {
  va_list args;
  va_start(args, format);

  return vfprintf(stderr, format, args);
}

#endif // PEAR_RUNTIME_POSIX_H
