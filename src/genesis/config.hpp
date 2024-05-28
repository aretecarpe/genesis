#if !defined GENESIS_CONFIG_HEADER_INCLUDED
#define GENESIS_CONFIG_HEADER_INCLUDED
#pragma once

// Systems - only one can be 1 all others, 0
//   GENESIS_UNIX_ID
//   GENESIS_LINUX_ID
//   GENESIS_MICROSOFT_ID
//   GENESIS_MINGW_ID
//   GENESIS_CYGWIN_ID
// GENESIS_VERSION is set appropriately

// Vendor - only one can be 1 all others, 0
//   GENESIS_VENDOR_GNU
//   GENESIS_VENDOR_MSVC
//   GENESIS_VENDOR_LLVM
//   GENESIS_VENDOR_MINGW
// GENESIS_VENDOR_VERSION is set appropriately

// Architecture - only one can be 1 all others, 0
//   GENESIS_ARCH_x86
//   GENESIS_ARCH_x64
//   GENESIS_ARCH_ARM
// GENESIS_ARCH_VERSION is set appropriately

// GENESIS_POSIX is set if the platform natively supports POSIX calls
// GENESIS_MICROSOFT is set if the platform natively supports Microsoft calls
// Note - Microsoft does support some Posix calls

#if defined linux || defined __linux__

#define GENESIS_UNIX 0
#define GENESIS_LINUX 1
#define GENESIS_MICROSOFT 0
#define GENESIS_MINGW 0
#define GENESIS_CYGWIN 0

#define GENESIS_POSIX 1
#define GENESIS_MICROSOFT 0

#elif defined unix || defined __unix__

#define GENESIS_UNIX 1
#define GENESIS_LINUX 0
#define GENESIS_MICROSOFT 0
#define GENESIS_MINGW 0
#define GENESIS_CYGWIN 0

#define GENESIS_POSIX 1
#define GENESIS_MICROSOFT 0

#elif defined __CYGWIN__

#define GENESIS_UNIX 0
#define GENESIS_LINUX 0
#define GENESIS_MICROSOFT 0
#define GENESIS_MINGW 0
#define GENESIS_CYGWIN 1

#define GENESIS_POSIX 1
#define GENESIS_MICROSOFT 0

#elif defined __MINGW32__ || __MINGW_64__

#define GENESIS_UNIX 0
#define GENESIS_LINUX 0
#define GENESIS_MICROSOFT 0
#define GENESIS_MINGW 1
#define GENESIS_CYGWIN 0

#define GENESIS_POSIX 0
#define GENESIS_MICROSOFT 1

#elif defined __WINDOWS__ || defined _WIN32 || defined _WIN64

#define GENESIS_UNIX 0
#define GENESIS_LINUX 0
#define GENESIS_MICROSOFT 1
#define GENESIS_MINGW 1
#define GENESIS_CYGWIN 0

#define GENESIS_POSIX 0
#define GENESIS_MICROSOFT 1

#else

#error Cannot detect operating system

#endif

#if GENESIS_MICROSOFT

#if defined _MSC_VER

#define GENESIS_VENDOR_MSVC 1

#elif defined __clang__

#define GENESIS_VENDOR_CLANG 1

#else
#error Cannot detect compiler vendor for Microsoft system
#endif

#elif GENESIS_MINGW

#define GENESIS_VENDOR_MINGW 1
#define GENESIS_VENDOR_VERSION \
  (__MINGW_VERSION * 10000 + __MINGW_MINOR * 100 + __MINGW_PATCHLEVEL__)

#elif GENESIS_LINUX || GENESIS_UNIX || GENESIS_CYGWIN

// Unix, Linux, and Cygwin can all use either GNU or Clang

#if defined __GNUC__
#define GENESIS_VENDOR_GNU 1
#elif defined __clang__
#define GENESIS_VENDOR_CLANG 1
#else
#error Cannot detect compiler for Linux system
#endif

#else

#error Cannot detect compiler vendor

#endif

#if GENESIS_VENDOR_GNU

// disable other compilers
#define GENESIS_VENDOR_CLANG 0
#define GENESIS_VENDOR_MSCV 0
#define GENESIS_VENDOR_STRING "gnu"

#if defined __GNUC_PATCHLEVEL__
#define GENESIS_VENDOR_VERSION \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#define GENESIS_VENDOR_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
#endif

#elif GENESIS_VENDOR_MSVC

// disable other compilers
#define GENESIS_VENDOR_GNU 0
#define GENESIS_VENDOR_CLANG 0
#define GENESIS_VENDOR_STRING "msvc"
#define GENESIS_VENDOR_VERSION _MSC_VER

#if _MSC_VER < 1910
#define CONSTEXPR11
#define CONSTEXPR14
#else
#define CONSTEXPR11 constexpr
#define CONSTEXPR14 constexpr
#endif

#elif GENESIS_VENDOR_MINGW

#define GENESIS_VERSION \
  (__MINGW_VERSION_MAJOR * 100 + __MINGW_VERSION_MINOR)

#define GENESIS_VENDOR_GNU 0
#define GENESIS_VENDOR_CLANG 0
#define GENESIS_VENDOR_STRING "mingw"

#elif GENESIS_VENDOR_CLANG

#define GENESIS_VENDOR_GNU 0
#define GENESIS_VENDOR_MSCV 0

#define GENESIS_VENDOR_VERSION \
  (__clang__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#define GENESIS_VENDOR_STRING "llvm"

#else
#error Cannot detect compiler vendor
#endif

#if !defined GENESIS_ARCH

#if defined i386 || defined __i386__ || defined __i386 || \
  defined __i486__ || defined __i586__ || defined __i686__ || defined _M_IX86
#define GENESIS_ARCH_INTEL 1
#define GENESIS_ARCH_x86 1
#define GENESIS_ARCH_x64 0
#define GENESIS_ARCH_ARM 0
#define GENESIS_ARCH_STRING "x86"

#if defined i386 || defined __i386__ || defined __i386
#define GENESIS_ARCH_VERSION 3
#elif defined __i486__
#define GENESIS_ARCH_VERSION 4
#elif defined __i586__
#define GENESIS_ARCH_VERSION 5
#elif defined __i686__
#define GENESIS_ARCH_VERSION 6
#elif defined _M_IX86
#define GENESIS_ARCH_VERSION _M_IX86
#else
#error Cannot detect x86 architecture version
#endif

#elif defined __x86_64 || defined __x86_64__ || defined __amd64__ || defined __amd64 || _M_X64

#define GENESIS_ARCH_INTEL 1
#define GENESIS_ARCH_x86 0
#define GENESIS_ARCH_x64 1
#define GENESIS_ARCH_ARM 0
#define GENESIS_ARCH_STRING "x86_64"
#define GENESIS_ARCH_VERSION 0

#elif defined __ARM_ARCH || defined __TARGET_ARCH_ARM || defined __TARGET_ARCH_THUMB || defined _M_ARM || \
  defined __arm__ || defined __ARM64 || defined __thumb__ || \
  defined _M_ARM64 || defined __arm64 || defined __aarch64__ || defined __AARCH64EL__ || \
  defined __ARM_ARCH_7__ || defined __ARM_ARCH_7A__ || defined __ARM_ARCH_7R__ || defined __ARM_ARCH_7M__ || \
  defined __ARM_ARCH_6K__ || defined __ARM_ARCH_6Z__ || defined __ARM_ARCH_6KZ__ || defined __ARM_ARCH_6T2__ || \
  defined __ARM_ARCH_5TE__ || defined __ARM_ARCH_5TEJ__ || \
  defined __ARM_ARCH_4T__ || defined __ARM_ARCH_4__

#define GENESIS_ARCH_INTEL 1
#define GENESIS_ARCH_x86 0
#define GENESIS_ARCH_x64 1
#define GENESIS_ARCH_ARM 1
#define GENESIS_ARCH_STRING "arm"

#if defined __ARM_ARCH
#define GENESIS_ARCH_VERSION __ARM_ARCH
#elif defined __TARGET_ARCH_ARM
#define GENESIS_ARCH_VERSION __TARGET_ARCH_ARM
#elif defined __TARGET_ARCH_THUMB
#define GENESIS_ARCH __TARGET_ARCH_THUMB
#elif defined _M_ARM
#define GENESIS_ARCH _M_ARM
#elif defined _M_ARM64 || defined __arm64 || defined __aarch64__ || defined __AARCH64EL__
#define GENESIS_ARCH_VERSION 8
#elif defined __ARM_ARCH_7__ || defined __ARM_ARCH_7A__ || defined __ARM_ARCH_7R__ || defined __ARM_ARCH_7M__
#define GENESIS_ARCH_VERSION 7
#elif defined __ARM_ARCH_6K__ || defined __ARM_ARCH_6Z__ || defined __ARM_ARCH_6KZ__ || defined __ARM_ARCH_6T2__
#define GENESIS_ARCH_VERSION 6
#elif defined __ARM_ARCH_5TE__ || defined __ARM_ARCH_5TEJ__
#define GENESIS_ARCH_VERSION 5
#elif defined __ARM_ARCH_4T__ || defined __ARM_ARCH_4__
#define GENESIS_ARCH_VERSION 4
#else
#error Cannot detect ARM architecture
#endif

#else
#error Cannot detect architecture
#endif

// end of architecture section
#endif

#if !defined CONSTEXPR11
// if not defined by specific compiler

#define CONSTEXPR11 constexpr
#if __cplusplus >= 201402
#define CONSTEXPR14 constexpr
#else
#define CONSTEXPR14
#endif

// end CONSTEXPR11
#endif

#endif // GENESIS_CONFIG_HEADER_INCLUDED
