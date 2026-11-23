#include "fpcie2com.h"

BOOLEAN FpcieValidateIOPortAccess(USHORT Port, ULONG Size)
{
    UNREFERENCED_PARAMETER(Port);
    UNREFERENCED_PARAMETER(Size);
    return TRUE;
}

NTSTATUS FpcieReadPortByte(USHORT Port, PUCHAR OutByte)
{
    if (!OutByte) {
        return STATUS_INVALID_PARAMETER;
    }

    *OutByte = __inbyte(Port);
    return STATUS_SUCCESS;
}

NTSTATUS FpcieWritePortByte(USHORT Port, UCHAR InByte)
{
    __outbyte(Port, InByte);
    return STATUS_SUCCESS;
}
