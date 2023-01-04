#pragma once

#include <efi/types.h>

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(
        IN EFI_ALLOCATE_TYPE Type,
        IN EFI_MEMORY_TYPE MemoryType, 
        IN UINTN Pages,
        IN OUT EFI_PHYSICAL_ADDRESS *Memory);

typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(
        IN EFI_PHYSICAL_ADDRESS Memory,
        IN UINTN Pages);

typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(
        IN OUT UINTN *MemoryMapSize,
        IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
        OUT UINTN *MapKey,
        OUT UINTN *DescriptorSize,
        OUT UINT32 *DescriptorVersion);

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
        IN EFI_MEMORY_TYPE PoolType,
        IN UINTN Size,
        OUT VOID **Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(IN VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE)(
        IN EFI_LOCATE_SEARCH_TYPE SearchType,
        IN EFI_GUID *Protocol OPTIONAL,
        IN VOID *SearchKey OPTIONAL,
        IN OUT UINTN *BufferSize,
        OUT EFI_HANDLE *Buffer);

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL 0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL 0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL 0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER 0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE 0x00000020

typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL)(
        IN EFI_HANDLE Handle,
        IN EFI_GUID *Protocol,
        OUT VOID **Interface OPTIONAL,
        IN EFI_HANDLE AgentHandle,
        IN EFI_HANDLE ControllerHandle,
        IN UINT32 Attributes);

typedef EFI_STATUS (EFIAPI *EFI_CLOSE_PROTOCOL)(
        IN EFI_HANDLE Handle, 
        IN EFI_GUID *Protocol,
        IN EFI_HANDLE AgentHandle,
        IN EFI_HANDLE ControllerHandle);

typedef EFI_STATUS (EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(
        IN EFI_HANDLE Handle, 
        OUT EFI_GUID ***ProtocolBuffer,
        OUT UINTN *ProtocolBufferCount);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
        IN EFI_LOCATE_SEARCH_TYPE SearchType,
        IN EFI_GUID *Protocol OPTIONAL,
        IN VOID *SearchKey OPTIONAL, 
        IN OUT UINTN *NoHandles,
        OUT EFI_HANDLE **Buffer);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
        IN EFI_GUID *Protocol,
        IN VOID *Registration OPTIONAL,
        OUT VOID **Interface);

typedef EFI_STATUS (EFIAPI *EFI_EXIT)(
        IN EFI_HANDLE ImageHandle,
        IN EFI_STATUS ExitStatus,
        IN UINTN ExitDataSize,
        IN CHAR16 *ExitData OPTIONAL);

typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(
        IN EFI_HANDLE ImageHandle,
        IN UINTN MapKey);

typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(
        OUT UINT64 *Count);

typedef EFI_STATUS (EFIAPI *EFI_SET_WATCHDOG_TIMER)(
        IN UINTN Timeout,
        IN UINT64 WatchdogCode,
        IN UINTN DataSize,
        IN CHAR16 *WatchdogData OPTIONAL);

typedef EFI_STATUS (EFIAPI *EFI_STALL)(IN UINTN Microseconds);

typedef VOID (EFIAPI *EFI_COPY_MEM)(
        IN VOID *Destination,
        IN VOID *Source,
        IN UINTN Length);

typedef VOID (EFIAPI *EFI_SET_MEM)(
        IN VOID *Buffer,
        IN UINTN Size,
        IN UINT8 Value);

typedef EFI_STATUS (EFIAPI *EFI_CALCULATE_CRC32)(
        IN VOID *Data,
        IN UINTN DataSize,
        OUT UINT32 *Crc32);

