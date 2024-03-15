#ifndef __K_TYPES_H__
#define __K_TYPES_H__

// Compitiable with OpenSBI and C, C++ for RISCV_64

#if __riscv_xlen == 32
#error "32-bit RISC-V is not supported"
#endif

typedef char			s8;
typedef unsigned char		u8;
typedef unsigned char		uint8_t;

typedef short			s16;
typedef unsigned short		u16;
typedef short			int16_t;
typedef unsigned short		uint16_t;

typedef int			s32;
typedef unsigned int		u32;
typedef int			int32_t;
typedef unsigned int		uint32_t;

typedef long			s64;
typedef unsigned long		u64;
typedef long			int64_t;
typedef unsigned long		uint64_t;
#define PRILX			"016lx"

typedef unsigned long		ulong;
typedef unsigned long		uintptr_t;
typedef unsigned long		size_t;
typedef long			ssize_t;
typedef unsigned long		virtual_addr_t;
typedef unsigned long		virtual_size_t;
typedef unsigned long		physical_addr_t;
typedef unsigned long		physical_size_t;

typedef uint16_t		le16_t;
typedef uint16_t		be16_t;
typedef uint32_t		le32_t;
typedef uint32_t		be32_t;
typedef uint64_t		le64_t;
typedef uint64_t		be64_t;

// #ifndef __cplusplus

// #define __packed		__attribute__((packed))
// #define __noreturn		__attribute__((noreturn))
// #define __aligned(x)		__attribute__((aligned(x)))
// #define __always_inline	inline __attribute__((always_inline))

// #define likely(x) __builtin_expect((x), 1)
// #define unlikely(x) __builtin_expect((x), 0)

// #endif

#endif
