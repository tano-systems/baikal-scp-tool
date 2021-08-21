// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Baikal-M (BE-M1000) SCP communication driver
 *
 * Copyright (C) 2021 Tano Systems LLC. All rights reserved.
 *
 * Authors: Anton Kikin <a.kikin@tano-systems.com>
 */

#include "baikal_scp_private.h"

long baikal_scp_dev_fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	switch (cmd) {
		case BAIKAL_SCP_IOCTL_CMD_INFO: {
			struct baikal_scp_ioctl_info info;

			info.drv_version = BAIKAL_SCP_VERSION(
				BAIKAL_SCP_DRV_VERSION_MAJOR,
				BAIKAL_SCP_DRV_VERSION_MINOR,
				BAIKAL_SCP_DRV_VERSION_PATCH
			);

			ret = copy_to_user((void *)arg, &info, sizeof(info));
			if (ret) {
				pr_err("%s: copy_to_user() failed (%ld)\n", __FUNCTION__, ret);
				return ret;
			}

			break;
		}

		case BAIKAL_SCP_IOCTL_CMD_FLASH_INFO: {
			struct baikal_scp_ioctl_flash_info flash_info;
			struct baikal_scp_flash_info scp_flash_info;

			ret = baikal_scp_flash_info(&scp_flash_info);
			if (ret)
				return ret;

			flash_info.sector_count = scp_flash_info.sector_count;
			flash_info.sector_size = scp_flash_info.sector_size;
			flash_info.total_size = scp_flash_info.total_size;

			ret = copy_to_user((void *)arg, &flash_info, sizeof(flash_info));
			if (ret) {
				pr_err("%s: copy_to_user() failed (%ld)\n", __FUNCTION__, ret);
				return ret;
			}

			break;
		}

		case BAIKAL_SCP_IOCTL_CMD_FLASH_READ: {
			struct baikal_scp_ioctl_flash_read flash_read;
			void *data;

			ret = copy_from_user(&flash_read, (void *)arg, sizeof(flash_read));
			if (ret) {
				pr_err("%s: copy_from_user() failed (%ld)\n", __FUNCTION__, ret);
				return ret;
			}

			if (!flash_read.size)
				return -EINVAL;

			data = vmalloc(flash_read.size);
			if (!data)
				return -ENOMEM;

			ret = baikal_scp_flash_read(flash_read.offset, flash_read.size, data);
			if (ret) {
				vfree(data);
				return ret;
			}

			ret = copy_to_user(flash_read.data, data, flash_read.size);
			if (ret) {
				pr_err("%s: copy_to_user() failed (%ld)\n", __FUNCTION__, ret);
				vfree(data);
				return ret;
			}

			vfree(data);

			break;
		}

		case BAIKAL_SCP_IOCTL_CMD_FLASH_WRITE: {
			struct baikal_scp_ioctl_flash_write flash_write;
			void *data;

			ret = copy_from_user(&flash_write, (void *)arg, sizeof(flash_write));
			if (ret) {
				pr_err("%s: copy_from_user() failed (%ld)\n", __FUNCTION__, ret);
				return ret;
			}

			if (!flash_write.size)
				return -EINVAL;

			data = vmalloc(flash_write.size);
			if (!data)
				return -ENOMEM;

			ret = copy_from_user(data, flash_write.data, flash_write.size);
			if (ret) {
				pr_err("%s: copy_from_user() failed (%ld)\n", __FUNCTION__, ret);
				vfree(data);
				return ret;
			}

			ret = baikal_scp_flash_write(flash_write.offset, flash_write.size, data);
			if (ret) {
				vfree(data);
				return ret;
			}

			vfree(data);
			break;
		}

		case BAIKAL_SCP_IOCTL_CMD_FLASH_ERASE: {
			struct baikal_scp_ioctl_flash_erase flash_erase;

			ret = copy_from_user(&flash_erase, (void *)arg, sizeof(flash_erase));
			if (ret) {
				pr_err("%s: copy_from_user() failed (%ld)\n", __FUNCTION__, ret);
				return ret;
			}

			ret = baikal_scp_flash_erase(flash_erase.offset, flash_erase.size);
			break;
		}

		default: {
			pr_err("Unknown IOCTL command %u\n", cmd);
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
