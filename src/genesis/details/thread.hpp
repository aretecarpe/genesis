#if !defined GENESIS_THREAD_HEADER_INCLUDED
#define GENESIS_THREAD_HEADER_INCLUDED
#pragma once

#include "genesis/config.hpp"

#include <thread>

#if __has_include(<xmmintrin.h>)
#include <xmmintrin.h>
inline void mm_pause() { _mm_pause(); }
#elif defined(_MSC_VER)
#include <intrin.h>
inline void mm_pause() { _mm_pause(); }
#else
inline void mm_pause() __asm__ __volatile__("pause")
#endif

#endif