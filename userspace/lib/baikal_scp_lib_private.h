/*
 * Copyright (C) 2021 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BAIKAL_SCP_LIB_PRIVATE_H
#define BAIKAL_SCP_LIB_PRIVATE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/limits.h>

#include <baikal_scp_lib.h>
#include <baikal_scp.h>

#define BAIKAL_SCP_LIB_VERSION_MAJOR  1
#define BAIKAL_SCP_LIB_VERSION_MINOR  0
#define BAIKAL_SCP_LIB_VERSION_PATCH  0

#define BAIKAL_SCP_LIB_VERSION BAIKAL_SCP_VERSION(\
	BAIKAL_SCP_LIB_VERSION_MAJOR, \
	BAIKAL_SCP_LIB_VERSION_MINOR, \
	BAIKAL_SCP_LIB_VERSION_PATCH)

typedef struct {
	/** SCP device file handle */
	int fhnd_scp;

} baikal_scp_lib_t;

extern baikal_scp_lib_t *baikal_scp_lib;

#endif /* BAIKAL_SCP_LIB_PRIVATE_H */
