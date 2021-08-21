/*
 * Copyright (C) 2021 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "baikal_scp_lib_private.h"

baikal_scp_lib_t *baikal_scp_lib = NULL;

int baikal_scp_init(void)
{
	char devpath[PATH_MAX];

	if (baikal_scp_lib)
		return ECANCELED;

	baikal_scp_lib = calloc(sizeof(baikal_scp_lib_t), 1);
	if (!baikal_scp_lib)
		return ENOMEM;

	snprintf(devpath, sizeof(devpath) - 1, "/dev/%s", BAIKAL_SCP_DEV_NAME);

	baikal_scp_lib->fhnd_scp = open(devpath, O_RDWR);
	if (baikal_scp_lib->fhnd_scp == -1) {
		free(baikal_scp_lib);
		baikal_scp_lib = NULL;
		return ENODEV;
	}

	return 0;
}

void baikal_scp_deinit(void)
{
	if (!baikal_scp_lib)
		return;

	close(baikal_scp_lib->fhnd_scp);

	free(baikal_scp_lib);
	baikal_scp_lib = NULL;
}

int baikal_scp_version(baikal_scp_version_info_t *version_info)
{
	int ret;
	struct baikal_scp_ioctl_info ioctl_info;

	if (!version_info)
		return EINVAL;

	if (!baikal_scp_lib)
		return ECANCELED;

	ret = ioctl(baikal_scp_lib->fhnd_scp, BAIKAL_SCP_IOCTL_CMD_INFO, &ioctl_info);
	if (ret)
		return ret;

	version_info->drv_version = ioctl_info.drv_version;
	version_info->lib_version = BAIKAL_SCP_LIB_VERSION;

	return 0;
}
