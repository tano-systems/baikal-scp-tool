/*
 * Copyright (C) 2021 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "baikal_scp_lib_private.h"

#define FLASH_PART_SIZE 1024

int baikal_scp_flash_info(baikal_scp_flash_info_t *info)
{
	int ret;
	struct baikal_scp_ioctl_flash_info ioctl_info;

	if (!info)
		return EINVAL;

	if (!baikal_scp_lib)
		return ECANCELED;

	ret = ioctl(baikal_scp_lib->fhnd_scp, BAIKAL_SCP_IOCTL_CMD_FLASH_INFO, &ioctl_info);
	if (ret)
		return ret;

	info->sector_count = ioctl_info.sector_count;
	info->sector_size = ioctl_info.sector_size;
	info->total_size = ioctl_info.total_size;

	return 0;
}

static int is_flash_alignment_valid(unsigned int value)
{
	return (value % BAIKAL_SCP_FLASH_SIZE_ALIGNMENT) == 0;
}

static int _baikal_scp_flash_op(
	baikal_scp_flash_operation_t op,
	unsigned int offset,
	unsigned int size,
	void *data,
	baikal_scp_flash_progress_cb_t cb
)
{
	int ret;

	union {
		struct baikal_scp_ioctl_flash_read  read;
		struct baikal_scp_ioctl_flash_write write;
		struct baikal_scp_ioctl_flash_erase erase;
	} ioctl_data;

	baikal_scp_flash_progress_info_t progress = {
		.operation = op,
		.size      = size,
		.offset    = offset
	};

	unsigned int op_size = size;
	unsigned int op_offset = offset;
	void        *op_ptr = data;
	unsigned int op_part;

	if (!baikal_scp_lib)
		return ECANCELED;

	if (!size)
		return EINVAL;

	if (!is_flash_alignment_valid(offset))
		return EINVAL;

	if (!is_flash_alignment_valid(size))
		return EINVAL;

	if (cb)
		cb(&progress);

	while (op_size) {
		op_part = (op_size < FLASH_PART_SIZE)
			? op_size : FLASH_PART_SIZE;

		switch(op) {
			case BAIKAL_SCP_FLASH_READ:
				ioctl_data.read.size   = op_part;
				ioctl_data.read.offset = op_offset;
				ioctl_data.read.data   = op_ptr;

				ret = ioctl(baikal_scp_lib->fhnd_scp,
					BAIKAL_SCP_IOCTL_CMD_FLASH_READ, &ioctl_data);
				break;

			case BAIKAL_SCP_FLASH_WRITE:
				ioctl_data.write.size   = op_part;
				ioctl_data.write.offset = op_offset;
				ioctl_data.write.data   = op_ptr;

				ret = ioctl(baikal_scp_lib->fhnd_scp,
					BAIKAL_SCP_IOCTL_CMD_FLASH_WRITE, &ioctl_data);
				break;

			case BAIKAL_SCP_FLASH_ERASE:
				ioctl_data.erase.size   = op_part;
				ioctl_data.erase.offset = op_offset;

				ret = ioctl(baikal_scp_lib->fhnd_scp,
					BAIKAL_SCP_IOCTL_CMD_FLASH_ERASE, &ioctl_data);
				break;

			default:
				ret = EINVAL;
				break;
		}

		if (ret)
			return ret;

		op_offset += op_part;
		op_size   -= op_part;

		if (op_ptr)
			op_ptr += op_part;

		if (cb) {
			progress.bytes += op_part;
			progress.percent = (progress.bytes * 100) / size;
			cb(&progress);
		}
	}

	return 0;
}

int baikal_scp_flash_read(
	unsigned int offset,
	unsigned int size,
	void *dst,
	baikal_scp_flash_progress_cb_t cb
)
{
	if (!size || !dst)
		return EINVAL;

	return _baikal_scp_flash_op(
		BAIKAL_SCP_FLASH_READ, offset, size, dst, cb);
}

int baikal_scp_flash_write(
	unsigned int offset,
	unsigned int size,
	const void *src,
	baikal_scp_flash_progress_cb_t cb
)
{
	if (!size || !src)
		return EINVAL;

	return _baikal_scp_flash_op(
		BAIKAL_SCP_FLASH_WRITE, offset, size, (void *)src, cb);
}

int baikal_scp_flash_erase(
	unsigned int offset,
	unsigned int size,
	baikal_scp_flash_progress_cb_t cb
)
{
	return _baikal_scp_flash_op(
		BAIKAL_SCP_FLASH_ERASE, offset, size, NULL, cb);
}

unsigned int baikal_scp_flash_alignment(void)
{
	return BAIKAL_SCP_FLASH_SIZE_ALIGNMENT;
}
