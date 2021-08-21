
# Boot Flash on Baikal-M (BE-M1000)

## Abbreviations

| Term | Meaning                  |
| ---- | ------------------------ |
| FIP  | Firmware Image Package   |
| SCP  | System Control Processor |
| SMC  | Secure Monitor Call      |

## Boot Flash Layout

Boot Flash layout for Baikal-M (BE-M1000) boards (SDK 5.4+):

| Image | Size                      | Flash offset | Offset for SMC | Description                                   |
| ----- | ------------------------- | ------------ | -------------- | --------------------------------------------- |
| scp   | 0x00080000 (512 KiB)      | 0x00000000   | n/a            | System Control Processor Firmware (SCP FW)    |
| bl1   | 0x00040000 (256 KiB)      | 0x00080000   | 0x00000000     | TF-A BL1                                      |
| dtb   | 0x00040000 (256 KiB)      | 0x000c0000   | 0x00040000     | Flattened Device Tree (FDT) Blob (DTB)        |
| var   | 0x000c0000 (768 KiB)      | 0x00100000   | 0x00080000     | EFI variables                                 |
| fip   | 0x00640000 (6400 KiB max) | 0x001c0000   | 0x00140000     | Firmware Image Package (FIP) (TF-A BL2+BL3.*) |
| fat   | 0x01800000 (8192 KiB max) | 0x00800000   | 0x00780000     | FAT32 rescue files (optional)                 |

By default image for SCP firmware (`scp`) are not accessible via SMC.
This is handled in A-TF sources in `plat/baikal/bm1000/drivers/bm1000_scp.c`, line 69:

```c
scp->offset = arg0 + FLASH_MAP_SCP;
```

`FLASH_MAP_SCP` defined in `plat/baikal/bm1000/include/platform_def.h`, line 140:

```c
#define FLASH_MAP_SCP			0x80000
```

## Issues

The flash memory size was limited to 16 MiB, because the SCP firmware does not allow to read data from the flash above offset 0xffffff. Apparently, the SCP firmware is limited to this offset. Therefore, in fact, the size of the fat image that can be processed is limited to 0x800000 bytes (8 MiB).

When trying to read anything above 0xffffff address, an error is returned and nothing can be read at any offsets anymore.
