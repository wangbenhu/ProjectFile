/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#ifndef _UECC_TYPES_H_
#define _UECC_TYPES_H_

#define OM_UECC_PLATFORM OM_UECC_ARM_THUMB2

#ifndef OM_UECC_PLATFORM
    #if __AVR__
        #define OM_UECC_PLATFORM OM_UECC_AVR
    #elif defined(__thumb2__) || defined(_M_ARMT) /* I think MSVC only supports Thumb-2 targets */
        #define OM_UECC_PLATFORM OM_UECC_ARM_THUMB2
    #elif defined(__thumb__)
        #define OM_UECC_PLATFORM OM_UECC_ARM_THUMB
    #elif defined(__arm__) || defined(_M_ARM)
        #define OM_UECC_PLATFORM OM_UECC_ARM
    #elif defined(__aarch64__)
        #define OM_UECC_PLATFORM OM_UECC_ARM64
    #elif defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__I86__)
        #define OM_UECC_PLATFORM OM_UECC_X86
    #elif defined(__amd64__) || defined(_M_X64)
        #define OM_UECC_PLATFORM OM_UECC_X86_64
    #else
        #define OM_UECC_PLATFORM OM_UECC_ARCH_OTHER
    #endif
#endif

#ifndef OM_UECC_ARM_USE_UMAAL
    #if (OM_UECC_PLATFORM == OM_UECC_ARM) && (__ARM_ARCH >= 6)
        #define OM_UECC_ARM_USE_UMAAL 1
    #elif (OM_UECC_PLATFORM == OM_UECC_ARM_THUMB2) && (__ARM_ARCH >= 6) && !__ARM_ARCH_7M__
        #define OM_UECC_ARM_USE_UMAAL 1
    #else
        #define OM_UECC_ARM_USE_UMAAL 0
    #endif
#endif

#ifndef OM_UECC_WORD_SIZE
    #if OM_UECC_PLATFORM == OM_UECC_AVR
        #define OM_UECC_WORD_SIZE 1
    #elif (OM_UECC_PLATFORM == OM_UECC_X86_64 || OM_UECC_PLATFORM == OM_UECC_ARM64)
        #define OM_UECC_WORD_SIZE 8
    #else
        #define OM_UECC_WORD_SIZE 4
    #endif
#endif

#if (OM_UECC_WORD_SIZE != 1) && (OM_UECC_WORD_SIZE != 4) && (OM_UECC_WORD_SIZE != 8)
    #error "Unsupported value for OM_UECC_WORD_SIZE"
#endif

#if ((OM_UECC_PLATFORM == OM_UECC_AVR) && (OM_UECC_WORD_SIZE != 1))
    #pragma message ("OM_UECC_WORD_SIZE must be 1 for AVR")
    #undef OM_UECC_WORD_SIZE
    #define OM_UECC_WORD_SIZE 1
#endif

#if ((OM_UECC_PLATFORM == OM_UECC_ARM || OM_UECC_PLATFORM == OM_UECC_ARM_THUMB || \
        OM_UECC_PLATFORM ==  OM_UECC_ARM_THUMB2) && \
     (OM_UECC_WORD_SIZE != 4))
    #pragma message ("OM_UECC_WORD_SIZE must be 4 for ARM")
    #undef OM_UECC_WORD_SIZE
    #define OM_UECC_WORD_SIZE 4
#endif

#if defined(__SIZEOF_INT128__) || ((__clang_major__ * 100 + __clang_minor__) >= 302)
    #define SUPPORTS_INT128 1
#else
    #define SUPPORTS_INT128 0
#endif

typedef int8_t wordcount_t;
typedef int16_t bitcount_t;
typedef int8_t cmpresult_t;

#if (OM_UECC_WORD_SIZE == 1)

typedef uint8_t om_uecc_word_t;
typedef uint16_t om_uecc_dword_t;

#define OM_HIGH_BIT_SET 0x80
#define OM_UECC_WORD_BITS 8
#define OM_UECC_WORD_BITS_SHIFT 3
#define OM_UECC_WORD_BITS_MASK 0x07

#elif (OM_UECC_WORD_SIZE == 4)

typedef uint32_t om_uecc_word_t;
typedef uint64_t om_uecc_dword_t;

#define OM_HIGH_BIT_SET 0x80000000
#define OM_UECC_WORD_BITS 32
#define OM_UECC_WORD_BITS_SHIFT 5
#define OM_UECC_WORD_BITS_MASK 0x01F

#elif (OM_UECC_WORD_SIZE == 8)

typedef uint64_t om_uecc_word_t;
#if SUPPORTS_INT128
typedef unsigned __int128 om_uecc_dword_t;
#endif

#define OM_HIGH_BIT_SET 0x8000000000000000ull
#define OM_UECC_WORD_BITS 64
#define OM_UECC_WORD_BITS_SHIFT 6
#define OM_UECC_WORD_BITS_MASK 0x03F

#endif /* OM_UECC_WORD_SIZE */

#endif /* _UECC_TYPES_H_ */
