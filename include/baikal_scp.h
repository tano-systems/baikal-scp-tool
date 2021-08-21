/*
 * Copyright (C) 2021 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BAIKAL_SCP_H
#define BAIKAL_SCP_H

#define BAIKAL_SCP_DEV_NAME "scp"

#define BAIKAL_SCP_IOCTL_MAGIC 'B'

/* 4 x 64-bit = 32 bytes */
#define BAIKAL_SCP_FLASH_SIZE_ALIGNMENT   (4 * sizeof(unsigned long))

#define BAIKAL_SCP_IOCTL_CMD_START        (100)

#define BAIKAL_SCP_IOCTL_CMD_INFO         (BAIKAL_SCP_IOCTL_CMD_START + 0)
#define BAIKAL_SCP_IOCTL_CMD_FLASH_INFO   (BAIKAL_SCP_IOCTL_CMD_START + 10)
#define BAIKAL_SCP_IOCTL_CMD_FLASH_READ   (BAIKAL_SCP_IOCTL_CMD_START + 11)
#define BAIKAL_SCP_IOCTL_CMD_FLASH_WRITE  (BAIKAL_SCP_IOCTL_CMD_START + 12)
#define BAIKAL_SCP_IOCTL_CMD_FLASH_ERASE  (BAIKAL_SCP_IOCTL_CMD_START + 13)

/* ---------------------------------------------------------------------------------- */

struct baikal_scp_ioctl_info {
	unsigned drv_version;
};

struct baikal_scp_ioctl_flash_info {
	unsigned sector_count;
	unsigned sector_size;
	unsigned total_size;
};

struct baikal_scp_ioctl_flash_read {
	unsigned offset;
	unsigned size;
	void *data;
};

struct baikal_scp_ioctl_flash_write {
	unsigned offset;
	unsigned size;
	void *data;
};

struct baikal_scp_ioctl_flash_erase {
	unsigned offset;
	unsigned size;
};

/* ---------------------------------------------------------------------------------- */

#define BAIKAL_SCP_IOCTL_INFO \
	_IOC(_IOC_READ, \
		 BAIKAL_SCP_IOCTL_MAGIC, \
		 BAIKAL_SCP_IOCTL_CMD_INFO, \
		 sizeof(struct baikal_scp_ioctl_info *) \
	)

#define BAIKAL_SCP_IOCTL_FLASH_INFO \
	_IOC(_IOC_READ, \
		 BAIKAL_SCP_IOCTL_MAGIC, \
		 BAIKAL_SCP_IOCTL_CMD_FLASH_INFO, \
		 sizeof(struct baikal_scp_ioctl_flash_info *) \
	)

#define BAIKAL_SCP_IOCTL_FLASH_READ \
	_IOC(_IOC_WRITE | _IOC_READ, \
		 BAIKAL_SCP_IOCTL_MAGIC, \
		 BAIKAL_SCP_IOCTL_CMD_FLASH_READ, \
		 sizeof(struct baikal_scp_ioctl_flash_read *) \
	)

#define BAIKAL_SCP_IOCTL_FLASH_WRITE \
	_IOC(_IOC_WRITE, \
		 BAIKAL_SCP_IOCTL_MAGIC, \
		 BAIKAL_SCP_IOCTL_CMD_FLASH_WRITE, \
		 sizeof(struct baikal_scp_ioctl_flash_write *) \
	)

#define BAIKAL_SCP_IOCTL_FLASH_ERASE \
	_IOC(_IOC_WRITE, \
		 BAIKAL_SCP_IOCTL_MAGIC, \
		 BAIKAL_SCP_IOCTL_CMD_FLASH_ERASE, \
		 sizeof(struct baikal_scp_ioctl_flash_erase *) \
	)

/* ---------------------------------------------------------------------------------- */

#endif /* BAIKAL_SCP_H */
