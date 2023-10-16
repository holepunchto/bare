#ifndef BARE_RUNTIME_POSIX_H
#define BARE_RUNTIME_POSIX_H

#include <stdarg.h>
#include <stdio.h>

int
bare_runtime__print_info (const char *format, ...) {
  va_list args;
  va_start(args, format);

  int err = vfprintf(stdout, format, args);

  va_end(args);

  return err;
}

int
bare_runtime__print_error (const char *format, ...) {
  va_list args;
  va_start(args, format);

  int err = vfprintf(stderr, format, args);

  va_end(args);

  return err;
}

#endif // BARE_RUNTIME_POSIX_H
