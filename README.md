# FPCIE2COM

FPCIE2COM (FPCIE2COM.sys / Fintek PCIe to COM Driver) - kernel mode driver used by Feature Integration Technology Inc. (Fintek) for multi-port PCIe-to-Serial COM adapters (F75111/Fintek 1110 series).

My opensource driver does not use the original source codes and is a reverse-engineered driver using [Ghidra](https://en.wikipedia.org/wiki/Ghidra) and [Radare2](https://en.wikipedia.org/wiki/Radare2). The rights have not been violated, and the new driver cannot be signed due to the EV certificate. I hope that the vulnerabilities can be closed using my security patches.

The main goal of the project is to recreate a secure driver with fixed vulnerabilities and errors made by past developers.

### Functions

Driver FPCIE2COM.sys allows adapter utility and serial communication software to interact directly with hardware:

- Direct physical memory byte read and write via MmMapIoSpace (IOCTL 0xA4152C10, 0xA4152C14)
- Direct x86 hardware I/O port input and output (IOCTL 0xA4152C08, 0xA4152C0C)
- Direct PCI configuration space read and write via 0xCF8/0xCFC (IOCTL 0xA4152C18, 0xA4152C1C)
- Adapter chip and device parameter querying (IOCTL 0xA4152800, 0xA4152804, 0xA4152808)
- Serial port FIFO speed, baud clock rate, and UART mode configuration (IOCTL 0xA4152C00, 0xA4152C04, 0xA4152C20, 0xA4152C24, 0xA4152C28, 0xA4152C2C, 0xA4152C30, 0xA4152C34, 0xA4152C38)

Fixed vulnerabilities in opensource driver FPCIE2COM.sys:

- [x] (LOLDrivers: FPCIE2COM.sys)[https://www.loldrivers.io/drivers/9543c507-9b10-4de8-89b9-42a4f24c99ec/]
  - Low-privilege users can map arbitrary physical memory for byte reads and writes, perform raw PCI configuration access through I/O ports 0xCF8 and 0xCFC, and control hardware controllers (BYOVD, MITRE ATT&CK T1068). Fixed by enforcing administrative ACLs on device creation.

Fixed no publish vulnerabilities and bugs which contributors to this project found:

- [x] Device deletion bypassed on symbolic link deletion failure ([view code](main.c))
- [x] Arbitrary physical memory byte read in command 0xA4152C10 ([view code](mem.c))
- [x] Arbitrary physical memory byte write in command 0xA4152C14 ([view code](mem.c))
- [x] Unfiltered write to critical x86 I/O ports and controllers in commands 0xA4152C08, 0xA4152C0C ([view code](io.c))
- [x] Arbitrary PCI configuration space modification in commands 0xA4152C18, 0xA4152C1C ([view code](pci.c))
- [x] Device creation with weak permissions ([view code](main.c))

Authors:

- 2026 - Herman Semenoff <<GermanAizek@yandex.ru>> ([germanaizek.github.io](https://germanaizek.github.io))
- Feature Integration Technology Inc. (Fintek) <<support@fintek.com.tw>>
