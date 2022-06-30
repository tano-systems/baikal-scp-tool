/*
 * Copyright (C) 2021-2022 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BAIKAL_SCP_TOOL_H
#define BAIKAL_SCP_TOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <baikal_scp_lib.h>

#define BAIKAL_SCP_TOOL_VERSION BAIKAL_SCP_VERSION(\
	BAIKAL_SCP_TOOL_VERSION_MAJOR, \
	BAIKAL_SCP_TOOL_VERSION_MINOR, \
	BAIKAL_SCP_TOOL_VERSION_PATCH)

#ifndef ALIGN
#define ALIGN(x, a) (((x) + (a - 1)) & ~(a - 1))
#endif

#endif /* BAIKAL_SCP_TOOL_H */
