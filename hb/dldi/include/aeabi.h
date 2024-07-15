// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors

#ifndef AEABI_H__
#define AEABI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Alias of __aeabi_memcpy4
 * @param dest Destination address
 * @param src Source address
 * @param n Number of bytes to copy
 */
void __aeabi_memcpy8(void* __restrict__ dest, const void* __restrict__ src, size_t n) __attribute__((nonnull(1, 2)));

/**
 * Copies n bytes from src to dest (forward)
 * Assumes dest and src are 4-byte aligned
 * @param dest Destination address
 * @param src Source address
 * @param n Number of bytes to copy
 */
void __aeabi_memcpy4(void* __restrict__ dest, const void* __restrict__ src, size_t n) __attribute__((nonnull(1, 2)));

/**
 * Copies n bytes from src to dest (forward)
 * @param dest Destination address
 * @param src Source address
 * @param n Number of bytes to copy
 */
void __aeabi_memcpy(void* __restrict__ dest, const void* __restrict__ src, size_t n) __attribute__((nonnull(1, 2)));

/**
 * Alias of __aeabi_memmove4
 * @param dest Destination address
 * @param src Source address
 * @param n Number of bytes to copy
 */
void __aeabi_memmove8(void* dest, const void* src, size_t n) __attribute__((nonnull(1, 2)));

/**
 * Safely copies n bytes of src to dest
 * Assumes dest and src are 4-byte aligned
 * @param dest Destination address
 * @param src Source address
 * @param n Number of bytes to copy
 */
void __aeabi_memmove4(void* dest, const void* src, size_t n) __attribute__((nonnull(1, 2)));

/**
 * Safely copies n bytes of src to dest
 * @param dest Destination address
 * @param src Source address
 * @param n Number of bytes to copy
 */
void __aeabi_memmove(void* dest, const void* src, size_t n) __attribute__((nonnull(1, 2)));

/**
 * Alias of __aeabi_memset4
 * @param dest Destination address
 * @param n Number of bytes to set
 * @param c Value to set
 */
void __aeabi_memset8(void* dest, size_t n, int c) __attribute__((nonnull(1)));

/**
 * Set n bytes of dest to (c & 0xff)
 * Assumes dest is 4-byte aligned
 * @param dest Destination address
 * @param n Number of bytes to set
 * @param c Value to set
 */
void __aeabi_memset4(void* dest, size_t n, int c) __attribute__((nonnull(1)));

/**
 * Set n bytes of dest to (c & 0xff)
 * @param dest Destination address
 * @param n Number of bytes to set
 * @param c Value to set
 */
void __aeabi_memset(void* dest, size_t n, int c) __attribute__((nonnull(1)));

/**
 * Alias of __aeabi_memclr4
 * @param dest Destination address
 * @param n Number of bytes to clear
 */
void __aeabi_memclr8(void* dest, size_t n) __attribute__((nonnull(1)));

/**
 * Clears n bytes of dest to 0
 * Assumes dest is 4-byte aligned
 * @param dest Destination address
 * @param n Number of bytes to clear
 */
void __aeabi_memclr4(void* dest, size_t n) __attribute__((nonnull(1)));

/**
 * Clears n bytes of dest to 0
 * @param dest Destination address
 * @param n Number of bytes to clear
 */
void __aeabi_memclr(void* dest, size_t n) __attribute__((nonnull(1)));

#ifdef __cplusplus
}
#endif

#endif // AEABI_H__
