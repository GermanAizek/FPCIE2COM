#include "fpcie2com.h"

// Validation of PCI configuration access to prevent arbitrary hardware disruption
BOOLEAN FpcieValidatePciAccess(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, ULONG Length, BOOLEAN IsWrite)
{
    UNREFERENCED_PARAMETER(Bus);

    // Max device is 31, function is 7
    if (Slot > 31 || Function > 7) {
        return FALSE;
    }

    if (Length == 0 || Length > 4) {
        return FALSE;
    }

    if ((ULONG)Offset + Length > 0x100) {
        return FALSE;
    }

    if (IsWrite) {
        ULONG start = Offset;
        ULONG end = Offset + Length;

        // Block writing to Vendor ID, Device ID (0x00 - 0x03)
        if (start < 0x04) {
            return FALSE;
        }

        // Block writing to Command & Status Register (0x04 - 0x07)
        if (start < 0x08 && end > 0x04) {
            return FALSE;
        }

        // Block writing to Class Code & Revision (0x08 - 0x0B)
        if (start < 0x0C && end > 0x08) {
            return FALSE;
        }

        // Block writing to Base Address Registers BAR0-BAR5 (0x10 - 0x27)
        if (start < 0x28 && end > 0x10) {
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS FpcieReadPciConfig(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, PULONG OutValue)
{
    ULONG PciAddr;

    if (!OutValue) {
        return STATUS_INVALID_PARAMETER;
    }

    // FIX: Validate PCI parameters and restrict dangerous access (0xA4152C18)
    if (!FpcieValidatePciAccess(Bus, Slot, Function, Offset, sizeof(ULONG), FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        PciAddr = 0x80000000 | (Bus << 16) | (Slot << 11) | (Function << 8) | (Offset & 0xFC);
        __outdword(0xCF8, PciAddr);
        *OutValue = __indword(0xCFC);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS FpcieWritePciConfig(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, ULONG Value, ULONG Length)
{
    ULONG PciAddr;

    // FIX: Validate PCI parameters and restrict dangerous register writes (BAR, Command) (0xA4152C1C)
    if (!FpcieValidatePciAccess(Bus, Slot, Function, Offset, Length, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        PciAddr = 0x80000000 | (Bus << 16) | (Slot << 11) | (Function << 8) | (Offset & 0xFC);
        __outdword(0xCF8, PciAddr);

        if (Length == 1) {
            __outbyte(0xCFC + (Offset & 3), (UCHAR)Value);
        } else if (Length == 2) {
            __outword(0xCFC + (Offset & 2), (USHORT)Value);
        } else {
            __outdword(0xCFC, Value);
        }

        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
