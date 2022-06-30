/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */

#ifndef _BAIKAL_SCP_PRIVATE_H
#define _BAIKAL_SCP_PRIVATE_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/stringify.h>

#ifdef CONFIG_HAVE_ARM_SMCCC
#include <linux/arm-smccc.h>
#else
#define BAIKAL_SMC_ENABLE_FLASH_EMULATION
#endif

#include "../include/baikal_scp.h"
#include "../include/baikal_scp_lib.h"

#include "baikal_sip_svc_flash.h"

#define BAIKAL_SCP_DRV_VERSION_STR \
	__stringify(BAIKAL_SCP_DRV_VERSION_MAJOR) "." \
	__stringify(BAIKAL_SCP_DRV_VERSION_MINOR) "." \
	__stringify(BAIKAL_SCP_DRV_VERSION_PATCH)

#define BAIKAL_SCP_DRV_DESCRIPTION "Baikal-M (BE-M1000) SCP communication driver"
#define BAIKAL_SCP_DRV_NAME "baikal-scp"

#define BAIKAL_SCP_DEV_CLASS_NAME "baikal_scp_dev"

typedef struct baikal_scp_dev {
	dev_t          region;
	struct class  *class;
	int            major;
	struct cdev    chrdev;
	unsigned int   open_counter;
	struct mutex   lock;
} baikal_scp_dev_t;

long baikal_scp_dev_fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

typedef struct baikal_scp_flash_info {
	unsigned sector_count;
	unsigned sector_size;
	unsigned total_size;
} baikal_scp_flash_info_t;

int baikal_scp_flash_info(baikal_scp_flash_info_t *flash);
int baikal_scp_flash_write(unsigned offset, unsigned size, const void *data);
int baikal_scp_flash_read(unsigned offset, unsigned size, void *data);
int baikal_scp_flash_erase(unsigned offset, unsigned size);

#endif /* _BAIKAL_SCP_PRIVATE_H */
