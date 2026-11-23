#pragma once

#include <ntddk.h>
#include <wdmsec.h>
#include <ntstrsafe.h>

#define FPCIE_DEVICE_TYPE           0x22

#define IOCTL_FPCIE_GET_DEV_INFO    0xA4152800
#define IOCTL_FPCIE_SET_DEV_INFO    0xA4152804
#define IOCTL_FPCIE_GET_CHIP_INFO   0xA4152808

#define IOCTL_FPCIE_GET_PORT_PROP   0xA4152C00
#define IOCTL_FPCIE_SET_PORT_PROP   0xA4152C04
#define IOCTL_FPCIE_READ_PORT_BYTE  0xA4152C08
#define IOCTL_FPCIE_WRITE_PORT_BYTE 0xA4152C0C
#define IOCTL_FPCIE_READ_MEM_BYTE   0xA4152C10
#define IOCTL_FPCIE_WRITE_MEM_BYTE  0xA4152C14
#define IOCTL_FPCIE_READ_PCI_CONFIG 0xA4152C18
#define IOCTL_FPCIE_WRITE_PCI_CONFIG 0xA4152C1C
#define IOCTL_FPCIE_GET_CLOCK_RATE  0xA4152C20
#define IOCTL_FPCIE_SET_CLOCK_RATE  0xA4152C24
#define IOCTL_FPCIE_GET_FIFO_SPEED  0xA4152C28
#define IOCTL_FPCIE_SET_FIFO_SPEED  0xA4152C2C
#define IOCTL_FPCIE_SET_UART_MODE   0xA4152C30
#define IOCTL_FPCIE_GET_CONFIG_REG  0xA4152C34
#define IOCTL_FPCIE_GET_PCI_BAR     0xA4152C38

#define FPCIE_DOS_DEVICE_NAME       L"\\DosDevices\\FPCIE2COM"
#define FPCIE_DEVICE_NAME_FORMAT    L"\\Device\\PCIE2COM_%d"
#define FPCIE_POOL_TAG              'TsuB'

#pragma pack(push, 1)
typedef struct _FPCIE_REQUEST {
    ULONG Param1;      // +0x00 (Port / Address / Index)
    ULONG Param2;      // +0x04 (Data / Output / Status)
    ULONG Param3;      // +0x08 (Value / Setting / Clock)
    ULONG Param4;      // +0x0C (Extra)
    USHORT Param5;     // +0x10 (Flags / Word)
    UCHAR Param6;      // +0x12 (Byte)
} FPCIE_REQUEST, *PFPCIE_REQUEST;

typedef struct _FPCIE_PCI_REQUEST {
    ULONG Bus;         // +0x00
    ULONG Value;       // +0x04
    UCHAR Offset;      // +0x08
    UCHAR Function;    // +0x09
    USHORT Reserved;   // +0x0A
    ULONG Length;      // +0x0C
    USHORT Slot;       // +0x10
} FPCIE_PCI_REQUEST, *PFPCIE_PCI_REQUEST;
#pragma pack(pop)

typedef struct _DEVICE_EXTENSION {
    ULONG DeviceFlags;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    IO_REMOVE_LOCK RemoveLock;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicLinkName;
    KSPIN_LOCK SpinLock;
    ULONG DeviceState;
    ULONG OldDeviceState;
    ULONG PortCount;
    PDEVICE_OBJECT ChildPdo[16];
    ULONG ChipId;
    ULONG VendorDeviceId;
    ULONG BarAddress[6];
    ULONG ClockRate;
    BOOLEAN Initialized;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Driver Dispatch Prototypes
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD FpcieUnload;
DRIVER_ADD_DEVICE FpcieAddDevice;

NTSTATUS FpcieCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FpcieDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FpciePnpDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FpciePowerDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS FpcieSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

// Physical Memory Access (mem.c)
NTSTATUS FpcieReadPhysicalByte(ULONG PhysicalAddress, PUCHAR OutByte);
NTSTATUS FpcieWritePhysicalByte(ULONG PhysicalAddress, UCHAR InByte);
BOOLEAN FpcieValidatePhysicalAddress(ULONG_PTR PhysicalAddress, SIZE_T Length);

// Port I/O Access (io.c)
NTSTATUS FpcieReadPortByte(USHORT Port, PUCHAR OutByte);
NTSTATUS FpcieWritePortByte(USHORT Port, UCHAR InByte);
BOOLEAN FpcieValidateIOPortAccess(USHORT Port, ULONG Size);

// PCI Config Access (pci.c)
NTSTATUS FpcieReadPciConfig(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, PULONG OutValue);
NTSTATUS FpcieWritePciConfig(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, ULONG Value, ULONG Length);
BOOLEAN FpcieValidatePciAccess(ULONG Bus, ULONG Slot, ULONG Function, ULONG Offset, ULONG Length, BOOLEAN IsWrite);
