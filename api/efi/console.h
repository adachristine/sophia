#pragma once

#include <efi/types.h>

#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
{0x387477c2,0x69c7,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    INT32 MaxMode;
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    BOOLEAN CursorVisible;
}
SIMPLE_TEXT_OUTPUT_MODE;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
        IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        IN BOOLEAN ExtendedVerification);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
        IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        IN CHAR16 *String);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(
        IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        IN CHAR16
        *String);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
        IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
        IN UINTN Attribute);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
        IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
{
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    EFI_TEXT_TEST_STRING TestString;
    EFIX_UNUSED_FUNC QueryMode;
    EFIX_UNUSED_FUNC SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFIX_UNUSED_FUNC SetCursorPosition;
    EFIX_UNUSED_FUNC EnableCursor;
    VOID *Mode;
};

