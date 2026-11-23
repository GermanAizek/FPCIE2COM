#include "fpcie2com.h"

BOOLEAN FpcieValidatePhysicalAddress(ULONG_PTR PhysicalAddress, SIZE_T Length)
{
    UNREFERENCED_PARAMETER(PhysicalAddress);
    UNREFERENCED_PARAMETER(Length);
    return TRUE;
}

NTSTATUS FpcieReadPhysicalByte(ULONG PhysicalAddress, PUCHAR OutByte)
{
    PHYSICAL_ADDRESS PhysAddr;
    PVOID MappedAddr;
    ULONG DwordVal;
    ULONG AlignedAddr;
    ULONG Offset;

    if (!OutByte) {
        return STATUS_INVALID_PARAMETER;
    }

    AlignedAddr = PhysicalAddress & ~3;
    Offset = PhysicalAddress & 3;
    PhysAddr.QuadPart = (LONGLONG)AlignedAddr;

    MappedAddr = MmMapIoSpace(PhysAddr, sizeof(ULONG), MmNonCached);
    if (!MappedAddr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DwordVal = *(volatile ULONG *)MappedAddr;
    MmUnmapIoSpace(MappedAddr, sizeof(ULONG));

    *OutByte = (UCHAR)((DwordVal >> (Offset * 8)) & 0xFF);
    return STATUS_SUCCESS;
}

NTSTATUS FpcieWritePhysicalByte(ULONG PhysicalAddress, UCHAR InByte)
{
    PHYSICAL_ADDRESS PhysAddr;
    PVOID MappedAddr;

    PhysAddr.QuadPart = (LONGLONG)PhysicalAddress;
    MappedAddr = MmMapIoSpace(PhysAddr, sizeof(UCHAR), MmNonCached);
    if (!MappedAddr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *(volatile UCHAR *)MappedAddr = InByte;
    MmUnmapIoSpace(MappedAddr, sizeof(UCHAR));

    return STATUS_SUCCESS;
}
