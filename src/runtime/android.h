#ifndef PEAR_RUNTIME_ANDROID_H
#define PEAR_RUNTIME_ANDROID_H

#include <android/log.h>
#include <stdarg.h>

int
pear_runtime__print_info (const char *format, ...) {
  va_list args;
  va_start(args, format);

  return __android_log_vprint(ANDROID_LOG_INFO, "pear", format, args);
}

int
pear_runtime__print_error (const char *format, ...) {
  va_list args;
  va_start(args, format);

  return __android_log_vprint(ANDROID_LOG_ERROR, "pear", format, args);
}

#endif // PEAR_RUNTIME_ANDROID_H
