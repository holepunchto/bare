#ifndef PEAR_TARGET_H
#define PEAR_TARGET_H

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#define PEAR_PLATFORM "ios"
#define PEAR_PLATFORM_IOS
#else
#define PEAR_PLATFORM "darwin"
#define PEAR_PLATFORM_DARWIN
#endif
#elif defined(__linux__)
#define PEAR_PLATFORM "linux"
#define PEAR_PLATFORM_LINUX
#elif defined(__ANDROID__)
#define PEAR_PLATFORM "android"
#define PEAR_PLATFORM_ANDROID
#elif defined(_WIN32)
#define PEAR_PLATFORM "win32"
#define PEAR_PLATFORM_WINDOWS
#else
#error Unsupported platform
#endif

#if defined(__aarch64__)
#define PEAR_ARCH "arm64"
#define PEAR_ARCH_ARM64
#elif defined(__arm__)
#define PEAR_ARCH "arm"
#define PEAR_ARCH_ARM
#elif defined(__x86_64)
#define PEAR_ARCH "x64"
#define PEAR_ARCH_X64
#elif defined(__i386__)
#define PEAR_ARCH "ia32"
#define PEAR_ARCH_IA32
#else
#error Unsupported architecture
#endif

#define PEAR_TARGET PEAR_PLATFORM "-" PEAR_ARCH

#endif // PEAR_TARGET_H
