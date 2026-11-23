#include "fpcie2com.h"

BOOLEAN FpcieValidatePciAccess(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, ULONG Length, BOOLEAN IsWrite)
{
    UNREFERENCED_PARAMETER(Bus);
    UNREFERENCED_PARAMETER(Slot);
    UNREFERENCED_PARAMETER(Function);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(IsWrite);
    return TRUE;
}

NTSTATUS FpcieReadPciConfig(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, PULONG OutValue)
{
    ULONG PciAddr;

    if (!OutValue) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Bus >= 256 || Slot >= 32 || Function >= 8) {
        return STATUS_INVALID_PARAMETER;
    }

    PciAddr = 0x80000000 | (Bus << 16) | (Slot << 11) | (Function << 8) | (Offset & 0xFC);
    __outdword(0xCF8, PciAddr);
    *OutValue = __indword(0xCFC);

    return STATUS_SUCCESS;
}

NTSTATUS FpcieWritePciConfig(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, ULONG Value, ULONG Length)
{
    ULONG PciAddr;

    if (Bus >= 256 || Slot >= 32 || Function >= 8) {
        return STATUS_INVALID_PARAMETER;
    }

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
}
