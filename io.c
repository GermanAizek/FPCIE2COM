#include "fpcie2com.h"

// Whitelist and validation for x86 hardware I/O ports
BOOLEAN FpcieValidateIOPortAccess(USHORT Port, ULONG Size)
{
    if ((ULONG)Port + Size > 0x10000) {
        return FALSE;
    }

    // Block critical legacy controllers that can cause system crashes, DMA attacks, or exploit SMM
    if ((Port <= 0x001F) ||                     // Master DMA 8237
        (Port >= 0x0020 && Port <= 0x0021) ||   // Master PIC 8259
        (Port >= 0x0040 && Port <= 0x0043) ||   // PIT 8254
        (Port == 0x0060 || Port == 0x0064) ||   // Keyboard Controller 8042
        (Port >= 0x0070 && Port <= 0x0071) ||   // RTC / CMOS
        (Port >= 0x0080 && Port <= 0x008F && Port != 0x0080 && Port != 0x0081) || // DMA page registers (allow 0x80/0x81 POST)
        (Port >= 0x00A0 && Port <= 0x00A1) ||   // Slave PIC 8259
        (Port == 0x00B2 || Port == 0x00B3) ||   // APM SMI control
        (Port >= 0x00C0 && Port <= 0x00DF)) {   // Slave DMA 8237
        return FALSE;
    }

    return TRUE;
}

NTSTATUS FpcieReadPortByte(USHORT Port, PUCHAR OutByte)
{
    if (!OutByte) {
        return STATUS_INVALID_PARAMETER;
    }

    // FIX: Validate I/O port read access to prevent controller hijacking (0xA4152C08)
    if (!FpcieValidateIOPortAccess(Port, sizeof(UCHAR))) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        *OutByte = __inbyte(Port);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS FpcieWritePortByte(USHORT Port, UCHAR InByte)
{
    // FIX: Validate I/O port write access to block dangerous controllers and APM SMI (0xA4152C0C)
    if (!FpcieValidateIOPortAccess(Port, sizeof(UCHAR))) {
        return STATUS_ACCESS_DENIED;
    }

    // Only allow safe diagnostic ports, serial COM ports, and PCIe COM expansion ranges
    if (Port != 0x0080 && Port != 0x0081 && Port != 0x00EB &&
        !(Port >= 0x02E8 && Port <= 0x02EF) &&
        !(Port >= 0x02F8 && Port <= 0x02FF) &&
        !(Port >= 0x03E8 && Port <= 0x03EF) &&
        !(Port >= 0x03F8 && Port <= 0x03FF) &&
        !(Port >= 0x1000 && Port <= 0xFFFE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        __outbyte(Port, InByte);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
