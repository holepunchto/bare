#ifndef BARE_RUNTIME_ANDROID_H
#define BARE_RUNTIME_ANDROID_H

#include <android/log.h>
#include <stdarg.h>

int
bare_runtime__print_info (const char *format, ...) {
  va_list args;
  va_start(args, format);

  int err = __android_log_vprint(ANDROID_LOG_INFO, "bare", format, args);

  va_end(args);

  return err;
}

int
bare_runtime__print_error (const char *format, ...) {
  va_list args;
  va_start(args, format);

  int err = __android_log_vprint(ANDROID_LOG_ERROR, "bare", format, args);

  va_end(args);

  return err;
}

#endif // BARE_RUNTIME_ANDROID_H
