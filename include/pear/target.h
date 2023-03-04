#ifndef PEAR_TARGET_H
#define PEAR_TARGET_H

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#define PEAR_PLATFORM "ios"
#else
#define PEAR_PLATFORM "darwin"
#endif
#elif defined(__linux__)
#define PEAR_PLATFORM "linux"
#elif defined(__ANDROID__)
#define PLATFORM_NAME "android"
#elif defined(_WIN32)
#define PLATFORM_NAME "windows"
#else
#error Unsupported platform
#endif

#if defined(__aarch64__)
#define PEAR_ARCH "arm64"
#elif defined(__arm__)
#define PEAR_ARCH "arm"
#elif defined(__x86_64)
#define PEAR_ARCH "x64"
#elif defined(__i386__)
#define PEAR_ARCH "ia32"
#else
#error Unsupported architecture
#endif

#define PEAR_TARGET PEAR_PLATFORM "-" PEAR_ARCH

#endif // PEAR_TARGET_H
