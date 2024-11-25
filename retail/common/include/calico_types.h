// SPDX-License-Identifier: ZPL-2.1
// SPDX-FileCopyrightText: Copyright fincs, devkitPro
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

/*! @addtogroup core
	@{
*/

/*! @name Integer types
	@brief Traditional shorthand form of integer types defined in `stdint.h`.
	@{
*/

typedef uint8_t  u8;       //!<  8-bit unsigned integer.
typedef uint16_t u16;      //!< 16-bit unsigned integer.
typedef uint32_t u32;      //!< 32-bit unsigned integer.
typedef uint64_t u64;      //!< 64-bit unsigned integer.
typedef uintptr_t uptr;    //!< Pointer-sized unsigned integer.

typedef int8_t  s8;        //!<  8-bit signed integer.
typedef int16_t s16;       //!< 16-bit signed integer.
typedef int32_t s32;       //!< 32-bit signed integer.
typedef int64_t s64;       //!< 64-bit signed integer.
typedef intptr_t sptr;     //!< Pointer-sized signed integer.

typedef volatile u8  vu8;  //!<  8-bit volatile unsigned integer.
typedef volatile u16 vu16; //!< 16-bit volatile unsigned integer.
typedef volatile u32 vu32; //!< 32-bit volatile unsigned integer.
typedef volatile u64 vu64; //!< 64-bit volatile unsigned integer.
typedef volatile uptr vuptr; //!< Pointer-sized volatile unsigned integer.

typedef volatile s8  vs8;  //!<  8-bit volatile signed integer.
typedef volatile s16 vs16; //!< 16-bit volatile signed integer.
typedef volatile s32 vs32; //!< 32-bit volatile signed integer.
typedef volatile s64 vs64; //!< 64-bit volatile signed integer.
typedef volatile sptr vsptr; //!< Pointer-sized volatile signed integer.

//! @}

/*! @brief Disables struct member alignment ("packs" a struct).
	@warning **This should only be used as a last resort**. The ARM
	architecture prior to v7 does not support misaligned access at all,
	and as such the compiler will be forced to emulate them using a
	barrage of 8-bit reads/writes and bitwise operations whenever **any**
	non-byte-sized field is accessed, **even if they would otherwise be aligned**.
	There are usually better alternatives to packing an entire struct, such as:
	- Decomposing the offending member(s) into smaller fields (such as a u64
		field into lower and upper u32 fields).
	- Isolating the offending member(s) into their own nested anonymous structs,
		which can then be packed.
	- Explicitly indicating the non-alignedness by replacing the field(s) with
		u8 arrays, and writing appropriate code to encode/decode them.
*/
#define MK_PACKED     __attribute__((packed))
//! @brief Marks something as deprecated
#define MK_DEPRECATED __attribute__((deprecated))
//! @brief Marks a small, utility function whose contents will be inserted into the caller ("inlined").
#define MK_INLINE     __attribute__((always_inline)) static inline
/*! @brief Similar to @ref MK_INLINE, but allowing the function body to be
	emitted in an object file when explicitly requested.
	@note This is mostly useful for ASM wrappers that can be inline in ARM mode
	but not in THUMB mode. In addition, this feature is only supported by C (not C++).
*/
#define MK_EXTINLINE  __attribute__((always_inline)) inline
//! @brief Marks a function as never being eligible for automatic inlining by the compiler
#define MK_NOINLINE   __attribute__((noinline))
//! @brief Marks a function as never returning to the caller
#define MK_NORETURN   __attribute__((noreturn))
//! @brief Marks a function as pure
#define MK_PURE       __attribute__((pure))
//! @brief Marks a function or global as being overridable by other object files
#define MK_WEAK       __attribute__((weak))
//! @brief Macro used to explicitly express that a given variable is unused
#define MK_DUMMY(_x)  (void)(_x)

#if defined(__cplusplus)
#define MK_EXTERN_C       extern "C"
#define MK_EXTERN_C_START MK_EXTERN_C {
#define MK_EXTERN_C_END   }
#else
#define MK_EXTERN_C
#define MK_EXTERN_C_START
#define MK_EXTERN_C_END
#endif

/*! @brief Marks a function as 32-bit ARM code
	@note This is only meaningful when compiling in THUMB mode.
*/
#if __thumb__
#define MK_CODE32 __attribute__((target("arm")))
#else
#define MK_CODE32
#endif

/*! @brief Marks an external function as being 32-bit ARM code
	@note This is an optional optimization only meaningful for ARMv4 (ARM7)
	code compiled in THUMB mode. The GNU linker will automatically generate
	appropriate stubs for interworking when this macro is not used.
*/
#if __thumb__ && __ARM_ARCH < 5
#define MK_EXTERN32   __attribute__((long_call))
#else
#define MK_EXTERN32
#endif

/*! @brief Similar to @ref MK_INLINE, but also marking the function as eligible
	for compile-time evaluation.
	@note When compiling as C++, this macro adds the `constexpr` specifier,
	allowing the defined function to work from constexpr contexts.
	In C, this macro is otherwise identical to @ref MK_INLINE.
*/
#if __cplusplus >= 201402L
#define MK_CONSTEXPR  MK_INLINE constexpr
#else
#define MK_CONSTEXPR  MK_INLINE
#endif

/*! @brief Overrides the alignment requirement of a structure
	@note This macro arises from differences in behavior between C and C++
	regarding the `alignas` specifier. In C++ alignas can be used for this
	purpose, while in C it cannot.
*/
#if __cplusplus >= 201103L
#define MK_STRUCT_ALIGN(_align) alignas(_align)
#else
#define MK_STRUCT_ALIGN(_align) __attribute__((aligned(_align)))
#endif

/*! @brief Accesses the specified memory-mapped register of the specified type.
	@param[in] _type Name of the type
	@param[in] _off Offset of the register (relative to the I/O register base)
*/
#define MK_REG(_type,_off) (*(_type volatile*)(MM_IO + (_off)))

//! @brief Special `if` statement hinting the compiler that its condition is likely to be true.
#define if_likely(_expr)   if(__builtin_expect(!!(_expr), 1))
//! @brief Special `if` statement hinting the compiler that its condition is unlikely to be true.
#define if_unlikely(_expr) if(__builtin_expect(!!(_expr), 0))

//! @brief Hints the compiler to optimize the code using the given expression, which is assumed to always be true.
#define MK_ASSUME(_expr) do { if (!(_expr)) __builtin_unreachable(); } while (0)

//! @}
