#ifndef BARE_TARGET_H
#define BARE_TARGET_H

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#define BARE_PLATFORM "ios"
#define BARE_PLATFORM_IOS
#elif TARGET_OS_MAC
#define BARE_PLATFORM "darwin"
#define BARE_PLATFORM_DARWIN
#else
#error Unsupported platform
#endif
#elif defined(__linux__)
#if defined(__ANDROID__)
#define BARE_PLATFORM "android"
#define BARE_PLATFORM_ANDROID
#else
#define BARE_PLATFORM "linux"
#define BARE_PLATFORM_LINUX
#endif
#elif defined(_WIN32)
#define BARE_PLATFORM "win32"
#define BARE_PLATFORM_WINDOWS
#else
#error Unsupported platform
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define BARE_ARCH "arm64"
#define BARE_ARCH_ARM64
#elif defined(__arm__) || defined(_M_ARM)
#define BARE_ARCH "arm"
#define BARE_ARCH_ARM
#elif defined(__x86_64) || defined(_M_AMD64)
#define BARE_ARCH "x64"
#define BARE_ARCH_X64
#elif defined(__i386__) || defined(_M_IX86)
#define BARE_ARCH "ia32"
#define BARE_ARCH_IA32
#else
#error Unsupported architecture
#endif

#if defined(__APPLE__)
#define BARE_SIMULATOR TARGET_OS_SIMULATOR
#else
#define BARE_SIMULATOR 0
#endif

#define BARE_TARGET BARE_PLATFORM "-" BARE_ARCH

#endif // BARE_TARGET_H
