#include <efi/types.h>
#include <efi/media.h>

#define EFI_SHELL_PARAMETERS_PROTOCOL_GUID {0x752f3136, 0x4e16, 0x4fdc, {0xa2, 0x2a, 0xe5, 0xf4, 0x68, 0x12, 0xf4, 0xca}}

typedef VOID *SHELL_FILE_HANDLE;

typedef struct
{
    CHAR16 **Argv;
    UINTN Argc;
    SHELL_FILE_HANDLE StdIn;
    SHELL_FILE_HANDLE StdOut;
    SHELL_FILE_HANDLE StdErr;
}
EFI_SHELL_PARAMETERS_PROTOCOL;

