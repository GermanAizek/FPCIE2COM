#include "fpcie2com.h"

static LONG g_DeviceCounter = 0;

NTSTATUS FpcieCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS FpciePnpDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    if (IrpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
        IoAcquireRemoveLock(&DevExt->RemoveLock, Irp);
        IoReleaseRemoveLockAndWait(&DevExt->RemoveLock, Irp);

        if (DevExt->SymbolicLinkName.Buffer) {
            IoDeleteSymbolicLink(&DevExt->SymbolicLinkName);
            RtlFreeUnicodeString(&DevExt->SymbolicLinkName);
        }

        if (DevExt->LowerDeviceObject) {
            IoDetachDevice(DevExt->LowerDeviceObject);
        }

        if (DevExt->DeviceName.Buffer) {
            RtlFreeUnicodeString(&DevExt->DeviceName);
        }

        IoDeleteDevice(DeviceObject);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(DevExt->LowerDeviceObject, Irp);
    }

    Status = IoAcquireRemoveLock(&DevExt->RemoveLock, Irp);
    if (!NT_SUCCESS(Status)) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DevExt->LowerDeviceObject, Irp);
    IoReleaseRemoveLock(&DevExt->RemoveLock, Irp);
    return Status;
}

NTSTATUS FpciePowerDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(DevExt->LowerDeviceObject, Irp);
}

NTSTATUS FpcieSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DevExt->LowerDeviceObject, Irp);
}

NTSTATUS FpcieDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    ULONG InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Information = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    switch (IoControlCode) {
    case IOCTL_FPCIE_GET_DEV_INFO:
    case IOCTL_FPCIE_SET_DEV_INFO:
    case IOCTL_FPCIE_GET_CHIP_INFO:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST)) {
            PFPCIE_REQUEST Req = (PFPCIE_REQUEST)Buffer;
            Req->Param2 = 0x1C29;
            Information = sizeof(FPCIE_REQUEST);
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_GET_PORT_PROP:
    case IOCTL_FPCIE_SET_PORT_PROP:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST)) {
            Information = sizeof(FPCIE_REQUEST);
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_READ_PORT_BYTE:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST) && OutputBufferLength >= sizeof(FPCIE_REQUEST)) {
            PFPCIE_REQUEST Req = (PFPCIE_REQUEST)Buffer;
            UCHAR Val = 0;
            Status = FpcieReadPortByte((USHORT)Req->Param1, &Val);
            if (NT_SUCCESS(Status)) {
                Req->Param2 = Val;
                Information = sizeof(FPCIE_REQUEST);
            }
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_WRITE_PORT_BYTE:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST)) {
            PFPCIE_REQUEST Req = (PFPCIE_REQUEST)Buffer;
            Status = FpcieWritePortByte((USHORT)Req->Param1, (UCHAR)Req->Param3);
            if (NT_SUCCESS(Status)) {
                Information = sizeof(FPCIE_REQUEST);
            }
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_READ_MEM_BYTE:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST) && OutputBufferLength >= sizeof(FPCIE_REQUEST)) {
            PFPCIE_REQUEST Req = (PFPCIE_REQUEST)Buffer;
            UCHAR Val = 0;
            Status = FpcieReadPhysicalByte(Req->Param1, &Val);
            if (NT_SUCCESS(Status)) {
                Req->Param2 = Val;
                Information = sizeof(FPCIE_REQUEST);
            }
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_WRITE_MEM_BYTE:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST)) {
            PFPCIE_REQUEST Req = (PFPCIE_REQUEST)Buffer;
            Status = FpcieWritePhysicalByte(Req->Param1, (UCHAR)Req->Param3);
            if (NT_SUCCESS(Status)) {
                Information = sizeof(FPCIE_REQUEST);
            }
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_READ_PCI_CONFIG:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_PCI_REQUEST) && OutputBufferLength >= sizeof(FPCIE_PCI_REQUEST)) {
            PFPCIE_PCI_REQUEST Req = (PFPCIE_PCI_REQUEST)Buffer;
            ULONG Val = 0;
            Status = FpcieReadPciConfig(Req->Bus, Req->Slot, Req->Function, Req->Offset, &Val);
            if (NT_SUCCESS(Status)) {
                Req->Value = Val;
                Information = sizeof(FPCIE_PCI_REQUEST);
            }
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_WRITE_PCI_CONFIG:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_PCI_REQUEST)) {
            PFPCIE_PCI_REQUEST Req = (PFPCIE_PCI_REQUEST)Buffer;
            Status = FpcieWritePciConfig(Req->Bus, Req->Slot, Req->Function, Req->Offset, Req->Value, Req->Length ? Req->Length : 4);
            if (NT_SUCCESS(Status)) {
                Information = sizeof(FPCIE_PCI_REQUEST);
            }
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_FPCIE_GET_CLOCK_RATE:
    case IOCTL_FPCIE_SET_CLOCK_RATE:
    case IOCTL_FPCIE_GET_FIFO_SPEED:
    case IOCTL_FPCIE_SET_FIFO_SPEED:
    case IOCTL_FPCIE_SET_UART_MODE:
    case IOCTL_FPCIE_GET_CONFIG_REG:
    case IOCTL_FPCIE_GET_PCI_BAR:
        if (Buffer && InputBufferLength >= sizeof(FPCIE_REQUEST)) {
            Information = sizeof(FPCIE_REQUEST);
        } else {
            Status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS FpcieAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_EXTENSION DevExt = NULL;
    WCHAR DeviceNameBuffer[64];
    WCHAR SymlinkBuffer[64];
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymlinkName;
    LONG DeviceIndex;

    DeviceIndex = InterlockedIncrement(&g_DeviceCounter);
    RtlStringCbPrintfW(DeviceNameBuffer, sizeof(DeviceNameBuffer), FPCIE_DEVICE_NAME_FORMAT, DeviceIndex);
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &DeviceName,
        FPCIE_DEVICE_TYPE,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject
    );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    DevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(DevExt, sizeof(DEVICE_EXTENSION));

    DevExt->DeviceObject = DeviceObject;
    DevExt->PhysicalDeviceObject = PhysicalDeviceObject;
    IoInitializeRemoveLock(&DevExt->RemoveLock, FPCIE_POOL_TAG, 0, 0);
    KeInitializeSpinLock(&DevExt->SpinLock);

    DevExt->LowerDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    if (!DevExt->LowerDeviceObject) {
        IoDeleteDevice(DeviceObject);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    RtlStringCbPrintfW(SymlinkBuffer, sizeof(SymlinkBuffer), L"%ls", FPCIE_DOS_DEVICE_NAME);
    RtlInitUnicodeString(&SymlinkName, SymlinkBuffer);

    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(Status)) {
        IoDeleteSymbolicLink(&SymlinkName);
        Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    DeviceObject->Flags |= DO_BUFFERED_IO;
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

VOID FpcieUnload(IN PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = FpcieUnload;
    DriverObject->DriverExtension->AddDevice = FpcieAddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = FpcieCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = FpcieCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FpcieDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = FpciePnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = FpciePowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = FpcieSystemControl;

    return STATUS_SUCCESS;
}
