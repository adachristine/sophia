#pragma once

#include <efi/types.h>
#include <efi/services.h>
#include <efi/console.h>
#include <efi/image.h>

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249
#define EFI_SYSTEM_TABLE_REVISION EFI_2_8_SYSTEM_TABLE_REVISION
#define EFI_2_80_SYSTEM_TABLE_REVISION ((2<<16) | (80))
#define EFI_2_70_SYSTEM_TABLE_REVISION ((2<<16) | (70))
#define EFI_2_60_SYSTEM_TABLE_REVISION ((2<<16) | (60))
#define EFI_2_50_SYSTEM_TABLE_REVISION ((2<<16) | (50))
#define EFI_2_40_SYSTEM_TABLE_REVISION ((2<<16) | (40))
#define EFI_2_31_SYSTEM_TABLE_REVISION ((2<<16) | (31))
#define EFI_2_30_SYSTEM_TABLE_REVISION ((2<<16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION ((2<<16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2<<16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION ((2<<16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION ((1<<16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION ((1<<16) | (02))

#define EFI_SPECIFICATION_VERSION EFI_SYSTEM_TABLE_REVISION

typedef struct
{
    EFI_GUID VendorGuid;
    VOID *VendorTable;
}
EFI_CONFIGURATION_TABLE;

struct EFI_SYSTEM_TABLE
{
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    VOID *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    VOID *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *ConfigurationTable;
};

#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42
#define EFI_BOOT_SERVICES_REVISION EFI_SPECIFICATION_VERSION

struct EFI_BOOT_SERVICES
{
    EFI_TABLE_HEADER Hdr;

    EFIX_UNUSED_FUNC RaiseTPL;
    EFIX_UNUSED_FUNC RestoreTPL;
    
    EFI_ALLOCATE_PAGES AllocatePages;
    EFI_FREE_PAGES FreePages;
    EFI_GET_MEMORY_MAP GetMemoryMap;
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_FREE_POOL FreePool;

    EFIX_UNUSED_FUNC CreateEvent;
    EFIX_UNUSED_FUNC SetTimer;
    EFIX_UNUSED_FUNC WaitForEvent;
    EFIX_UNUSED_FUNC SignalEvent;
    EFIX_UNUSED_FUNC CloseEvent;
    EFIX_UNUSED_FUNC CheckEvent;

    EFIX_UNUSED_FUNC InstallProtocolInterface;
    EFIX_UNUSED_FUNC ReinstallProtocolInterface;
    EFIX_UNUSED_FUNC UninstallProtocolInterface;
    EFIX_UNUSED_FUNC HandleProtocol;
    VOID* Reserved;
    EFIX_UNUSED_FUNC RegisterProtocolNotify;
    EFI_LOCATE_HANDLE LocateHandle;
    EFIX_UNUSED_FUNC LocateDevicePath;
    EFIX_UNUSED_FUNC InstallConfigurationTable;

    EFIX_UNUSED_FUNC LoadImage;
    EFIX_UNUSED_FUNC StartImage;
    EFI_EXIT Exit;
    EFIX_UNUSED_FUNC UnloadImage;
    EFI_EXIT_BOOT_SERVICES ExitBootServices;

    EFI_GET_NEXT_MONOTONIC_COUNT GetNextMonotonicCount;
    EFI_STALL Stall;
    EFI_SET_WATCHDOG_TIMER SetWatchdogTimer;

    EFIX_UNUSED_FUNC ConnectController;
    EFIX_UNUSED_FUNC DisconnectController;

    EFI_OPEN_PROTOCOL OpenProtocol;
    EFI_CLOSE_PROTOCOL CloseProtocol;
    EFIX_UNUSED_FUNC OpenProtocolInformation;

    EFI_PROTOCOLS_PER_HANDLE ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    EFIX_UNUSED_FUNC InstallMultipleProtocolInterfaces;
    EFIX_UNUSED_FUNC UninstallMultipleProtocolInterfaces;

    EFI_CALCULATE_CRC32 CalculateCrc32;
    EFI_COPY_MEM CopyMem;
    EFI_SET_MEM SetMem;
    EFIX_UNUSED_FUNC CreateEventEx;
};

