// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Baikal-M (BE-M1000) SCP communication driver
 *
 * Copyright (C) 2021 Tano Systems LLC. All rights reserved.
 *
 * Authors: Anton Kikin <a.kikin@tano-systems.com>
 */

#include "baikal_scp_private.h"

/* This region is not accessible via SCP */
#define SCP_SIZE (512 * 1024)

#ifdef BAIKAL_SMC_ENABLE_FLASH_EMULATION

#define TEST_FLASH_SECTOR_COUNT (512)
#define TEST_FLASH_SECTOR_SIZE  (65536)
#define TEST_FLASH_SIZE         (TEST_FLASH_SECTOR_COUNT * TEST_FLASH_SECTOR_SIZE)

#ifndef min
#define min(a,b) ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a < _b ? _a : _b; })
#endif

static uint8_t test_flash[TEST_FLASH_SIZE] = { 0 };
static uint8_t test_flash_buf[BAIKAL_SCP_FLASH_BUF_SIZE];
static unsigned test_flash_buf_idx = 0;

struct baikal_arm_smccc_res {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
};

static void baikal_arm_smccc_smc(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct baikal_arm_smccc_res *res)
{
	memset(res, 0, sizeof(struct baikal_arm_smccc_res));

	switch(a0) {
		case BAIKAL_SMC_FLASH_WRITE:
			memcpy(test_flash + a1, test_flash_buf, a2);
			break;

		case BAIKAL_SMC_FLASH_READ:
			memcpy(test_flash_buf, test_flash + a1, a2);
			break;

		case BAIKAL_SMC_FLASH_ERASE:
			memset(test_flash + a1, 0, a2);
			break;

		case BAIKAL_SMC_FLASH_PUSH: {
			unsigned long * const buf =
				(void *)&test_flash_buf[test_flash_buf_idx];

			buf[0] = a1;
			buf[1] = a2;
			buf[2] = a3;
			buf[3] = a4;

			test_flash_buf_idx += 4 * sizeof(buf[0]);
			break;
		}

		case BAIKAL_SMC_FLASH_PULL: {
			unsigned long * const buf =
				(void *)&test_flash_buf[test_flash_buf_idx];

			res->a0 = buf[0];
			res->a1 = buf[1];
			res->a2 = buf[2];
			res->a3 = buf[3];

			test_flash_buf_idx += 4 * sizeof(buf[0]);
			break;
		}

		case BAIKAL_SMC_FLASH_POSITION:
			test_flash_buf_idx = a1;
			break;

		case BAIKAL_SMC_FLASH_INFO:
			res->a0 = 0;
			res->a1 = TEST_FLASH_SECTOR_COUNT;
			res->a2 = TEST_FLASH_SECTOR_SIZE;
			break;

		default:
			res->a0 = 1;
			break;
	}
}

#else

#define baikal_arm_smccc_res arm_smccc_res
#define baikal_arm_smccc_smc arm_smccc_smc

#endif

static int baikal_scp_flash_validate_offset_size(unsigned offset, unsigned size)
{
	int ret;
	baikal_scp_flash_info_t flash_info;

	ret = baikal_scp_flash_info(&flash_info);
	if (ret) {
		pr_err("%s: baikal_scp_flash_info() failed (%d)\n", __FUNCTION__, ret);
		return ret;
	}

	if ((size % BAIKAL_SCP_FLASH_SIZE_ALIGNMENT) != 0) {
		pr_err("%s: Invalid size (0x%x) alignment\n", __FUNCTION__, size);
		return -EINVAL;
	}

	if (!size || ((offset + size) > flash_info.total_size)) {
		pr_err("%s: Invalid size (0x%x) and offset (0x%x) combination\n",
			__FUNCTION__, offset, size);
		return -EINVAL;
	}

	return 0;
}

int baikal_scp_flash_info(baikal_scp_flash_info_t *flash)
{
	static baikal_scp_flash_info_t cached_flash_info;
	static int has_cache = 0;

	if (!has_cache) {
		struct baikal_arm_smccc_res res;
		unsigned int scp_sectors;

		baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_INFO, 0, 0, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			pr_err("%s: BAIKAL_SMC_FLASH_INFO failed (a0 = 0x%lx)\n", __FUNCTION__, res.a0);
			return -1;
		}

		cached_flash_info.sector_count = (unsigned)res.a1;
		cached_flash_info.sector_size  = (unsigned)res.a2;

		/*
		 * Adjust sectors count (SCP region at beginning of flash are
		 * not accessible via SCP firmware.
		 */
		if (!cached_flash_info.sector_size) {
			pr_err("%s: Invalid flash sector size (%u) returned from SCP\n",
				__FUNCTION__, cached_flash_info.sector_size);
			return -1;
		}

		scp_sectors = SCP_SIZE / cached_flash_info.sector_size;
		if (cached_flash_info.sector_count <= scp_sectors) {
			pr_err("%s: Invalid flash sectors count (%u) returned from SCP\n",
				__FUNCTION__, cached_flash_info.sector_count);
			return -1;
		}

		cached_flash_info.sector_count -= scp_sectors;
		cached_flash_info.total_size =
			cached_flash_info.sector_count * cached_flash_info.sector_size;

		has_cache = 1;
	}

	memcpy(flash, &cached_flash_info, sizeof(cached_flash_info));
	return 0;
}

int baikal_scp_flash_write(unsigned offset, unsigned size, const void *data)
{
	int ret;
	struct baikal_arm_smccc_res res;

	unsigned part;
	unsigned i;
	const unsigned long *ptr = data;

	ret = baikal_scp_flash_validate_offset_size(offset, size);
	if (ret)
		return ret;

	while (size) {
		part = min(size, (unsigned)BAIKAL_SCP_FLASH_BUF_SIZE);

		/* Reset buffer position */
		baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_POSITION, 0, 0, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			pr_err("%s: BAIKAL_SMC_FLASH_POSITION failed at offset 0x%x and size 0x%x (a0 = 0x%lx)\n",
				__FUNCTION__, offset, size, res.a0);
			return -1;
		}

		/* Push to buffer */
		for (i = 0; i < part; i += BAIKAL_SCP_FLASH_SIZE_ALIGNMENT) {
			baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_PUSH,
				ptr[0], ptr[1], ptr[2], ptr[3], 0, 0, 0, &res);
			if (res.a0) {
				pr_err("%s: BAIKAL_SMC_FLASH_PUSH failed at offset 0x%x and size 0x%x (a0 = 0x%lx)\n",
					__FUNCTION__, offset, size, res.a0);
				return -1;
			}

			ptr += 4;
		}

		/* Write data from buffer to flash */
		baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_WRITE, offset, part, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			pr_err("%s: BAIKAL_SMC_FLASH_WRITE failed at offset 0x%x and size 0x%x (a0 = 0x%lx)\n",
				__FUNCTION__, offset, size, res.a0);
			return -1;
		}

		offset += part;
		size -= part;
	}

	return 0;
}

int baikal_scp_flash_read(unsigned offset, unsigned size, void *data)
{
	int ret;

	struct baikal_arm_smccc_res res;
	unsigned part;
	unsigned i;
	unsigned long *ptr = data;

	ret = baikal_scp_flash_validate_offset_size(offset, size);
	if (ret)
		return ret;

	while (size) {
		part = min(size, (unsigned)BAIKAL_SCP_FLASH_BUF_SIZE);

		/* Reset buffer position */
		baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_POSITION, 0, 0, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			pr_err("%s: BAIKAL_SMC_FLASH_POSITION failed at offset 0x%x and size 0x%x (a0 = 0x%lx)\n",
				__FUNCTION__, offset, size, res.a0);
			return -1;
		}

		/* Read data from flash */
		baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_READ, offset, part, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			pr_err("%s: BAIKAL_SMC_FLASH_READ failed at offset 0x%x and size 0x%x (a0 = 0x%lx)\n",
				__FUNCTION__, offset, size, res.a0);
			return -1;
		}

		/* Pull from buffer */
		for (i = 0; i < part; i += BAIKAL_SCP_FLASH_SIZE_ALIGNMENT) {
			baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_PULL, 0, 0, 0, 0, 0, 0, 0, &res);
			ptr[0] = res.a0;
			ptr[1] = res.a1;
			ptr[2] = res.a2;
			ptr[3] = res.a3;
			ptr += 4;
		}

		offset += part;
		size -= part;
	}

	return 0;
}

int baikal_scp_flash_erase(unsigned offset, unsigned size)
{
	int ret;
	unsigned part;
	struct baikal_arm_smccc_res res;

	ret = baikal_scp_flash_validate_offset_size(offset, size);
	if (ret)
		return ret;

	while (size) {
		part = min(size, (unsigned)BAIKAL_SCP_FLASH_BUF_SIZE);
		baikal_arm_smccc_smc(BAIKAL_SMC_FLASH_ERASE, offset, part, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			pr_err("%s: BAIKAL_SMC_FLASH_ERASE failed at offset 0x%x and size 0x%x (a0 = 0x%lx)\n",
				__FUNCTION__, offset, size, res.a0);
			return -1;
		}

		offset += part;
		size -= part;
	}

	return 0;
}
