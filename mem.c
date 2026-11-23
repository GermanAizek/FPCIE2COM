#include "fpcie2com.h"

BOOLEAN FpcieValidatePhysicalAddress(ULONG_PTR PhysicalAddress, SIZE_T Length)
{
    // Check for integer overflow
    if (PhysicalAddress + Length < PhysicalAddress) {
        return FALSE;
    }

    // Limit transfer size
    if (Length == 0 || Length > 0x10000) {
        return FALSE;
    }

    // Block lower physical memory containing IVT, BDA, and kernel zero-page (< 0xA0000)
    if (PhysicalAddress < 0x000A0000) {
        return FALSE;
    }

    // Allow BIOS / Option ROM / Video RAM area: 0x000A0000 - 0x00100000
    if (PhysicalAddress >= 0x000A0000 && (PhysicalAddress + Length) <= 0x00100000) {
        return TRUE;
    }

    // Allow hardware MMIO address space (PCI BARs / hardware registers)
    if (PhysicalAddress >= 0xC0000000) {
        return TRUE;
    }

    // Block arbitrary system RAM access
    return FALSE;
}

NTSTATUS FpcieReadPhysicalByte(ULONG PhysicalAddress, PUCHAR OutByte)
{
    PHYSICAL_ADDRESS PhysAddr;
    PVOID MappedAddr = NULL;
    ULONG DwordVal = 0;
    ULONG AlignedAddr;
    ULONG Offset;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!OutByte) {
        return STATUS_INVALID_PARAMETER;
    }

    // FIX: Restrict arbitrary physical memory read to safe hardware MMIO and BIOS ranges (0xA4152C10)
    if (!FpcieValidatePhysicalAddress((ULONG_PTR)PhysicalAddress, sizeof(ULONG))) {
        return STATUS_ACCESS_DENIED;
    }

    AlignedAddr = PhysicalAddress & ~3;
    Offset = PhysicalAddress & 3;
    PhysAddr.QuadPart = (LONGLONG)AlignedAddr;

    MappedAddr = MmMapIoSpace(PhysAddr, sizeof(ULONG), MmNonCached);
    if (!MappedAddr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            DwordVal = *(volatile ULONG *)MappedAddr;
            *OutByte = (UCHAR)((DwordVal >> (Offset * 8)) & 0xFF);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(MappedAddr, sizeof(ULONG));
    }

    return Status;
}

NTSTATUS FpcieWritePhysicalByte(ULONG PhysicalAddress, UCHAR InByte)
{
    PHYSICAL_ADDRESS PhysAddr;
    PVOID MappedAddr = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    // FIX: Restrict arbitrary physical memory write to hardware MMIO space only (0xA4152C14)
    if (PhysicalAddress < 0xC0000000 || !FpcieValidatePhysicalAddress((ULONG_PTR)PhysicalAddress, sizeof(UCHAR))) {
        return STATUS_ACCESS_DENIED;
    }

    PhysAddr.QuadPart = (LONGLONG)PhysicalAddress;
    MappedAddr = MmMapIoSpace(PhysAddr, sizeof(UCHAR), MmNonCached);
    if (!MappedAddr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            *(volatile UCHAR *)MappedAddr = InByte;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(MappedAddr, sizeof(UCHAR));
    }

    return Status;
}
