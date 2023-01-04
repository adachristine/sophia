#pragma once

#include <efi/defs.h>

#include <stdint.h>
#include <stddef.h>

typedef uint8_t BOOLEAN;

#if !defined(TRUE)
# define TRUE ((BOOLEAN) 1)
#endif

#if !defined(FALSE)
# define FALSE ((BOOLEAN) 0)
#endif

#if defined(__x86_64__)
# if defined(__LP64__)
typedef long INTN;
typedef unsigned long UINTN;
#define INTN_MIN (1UL<<63)
#define UINTN_MIN (0UL)
# elif defined(_WIN64)
typedef long long INTN;
typedef unsigned long long UINTN;
#define INTN_MIN (1ULL<<63)
#define UINTN_MIN (0ULL)
# endif 
#else
typedef int INTN;
typedef unsigned int UINTN;
#define INTN_MIN (1U<<31)
#define UINTN_MIN (0)
#endif
#define INTN_MAX (~INTN_MIN)
#define UINTN_MAX (~UINTN_MIN)

typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;

typedef unsigned char CHAR8;
typedef unsigned short CHAR16;

typedef void VOID;

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;

typedef struct
{
    UINT32 Guid1;
    UINT16 Guid2;
    UINT16 Guid3;
    UINT8 Guid4[8];
}
EFI_GUID;

typedef UINTN EFI_STATUS;
typedef VOID * EFI_HANDLE;
typedef VOID * EFI_EVENT;

typedef struct
{
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
}
EFI_TABLE_HEADER;

typedef enum
{
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
}
EFI_ALLOCATE_TYPE;

typedef enum
{
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType,
    EfixMinOemMemoryType = 0x70000000,
    EfixMaxOemMemoryType = 0x7fffffff,
    EfixMinOsMemoryType = INT32_MIN,
    EfixMaxOsMemoryType = -1,
}
EFI_MEMORY_TYPE;

#define EFIX_OEM_MEMORY_TYPE(x) (EfixMinOemMemoryType|(UINT32)x)
#define EFIX_OS_MEMORY_TYPE(x) (EfixMinOsMemoryType|(UINT32)x)

#define EFI_MEMORY_UC 0x0000000000000001
#define EFI_MEMORY_WC 0x0000000000000002
#define EFI_MEMORY_WT 0x0000000000000004
#define EFI_MEMORY_WB 0x0000000000000008
#define EFI_MEMORY_UCE 0x0000000000000010
#define EFI_MEMORY_WP 0x0000000000001000
#define EFI_MEMORY_RP 0x0000000000002000
#define EFI_MEMORY_XP 0x0000000000004000
#define EFI_MEMORY_NV 0x0000000000008000
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000010000
#define EFI_MEMORY_RO 0x0000000000020000
#define EFI_MEMORY_SP 0x0000000000040000
#define EFI_MEMORY_CPU_CRYPTO 0x0000000000080000
#define EFI_MEMORY_RUNTIME 0x8000000000000000

#define EFI_MEMORY_DESCRIPTOR_VERSION 1

typedef struct
{
    UINT32 Type;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
}
EFI_MEMORY_DESCRIPTOR;

typedef enum
{
    AllHandles,
    ByRegisterNotify,
    ByProtocol
}
EFI_LOCATE_SEARCH_TYPE;

typedef struct
{
    UINT16 Year;
    UINT8 Month;
    UINT8 Day;
    UINT8 Hour;
    UINT8 Minute;
    UINT8 Second;
    UINT8 Pad1;
    UINT32 Nanosecond;
    UINT16 TimeZone;
    UINT8 Daylight;
    UINT8 Pad2;
}
EFI_TIME;

typedef VOID (EFIAPI *EFIX_UNUSED_FUNC)(VOID);

typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;
