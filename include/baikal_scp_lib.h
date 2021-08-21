/*
 * Copyright (C) 2021-2022 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BAIKAL_SCP_LIB_H
#define BAIKAL_SCP_LIB_H

/* ---------------------------------------------------------------------------------- */

#define BAIKAL_SCP_VERSION(major, minor, patch) \
	((((major) & 0xff) << 24) | (((minor) & 0xff) << 16) | ((patch) & 0xffff))

#define BAIKAL_SCP_VERSION_GET_MAJOR(ver) (((ver) & 0xff000000) >> 24)
#define BAIKAL_SCP_VERSION_GET_MINOR(ver) (((ver) & 0x00ff0000) >> 16)
#define BAIKAL_SCP_VERSION_GET_PATCH(ver) (((ver) & 0x0000ffff))

/* ---------------------------------------------------------------------------------- */

#ifndef __KERNEL__

/**
 * Version information structure
 */
typedef struct baikal_scp_version_info {
	unsigned int drv_version;
	unsigned int lib_version;
} baikal_scp_version_info_t;

/**
 * Flash information structure
 */
typedef struct baikal_scp_flash_info {
	unsigned int sector_count;
	unsigned int sector_size;
	unsigned int total_size;
} baikal_scp_flash_info_t;

/**
 * Flash operation type enumeration
 */
typedef enum {
	BAIKAL_SCP_FLASH_READ = 0,
	BAIKAL_SCP_FLASH_ERASE,
	BAIKAL_SCP_FLASH_WRITE,
} baikal_scp_flash_operation_t;

/**
 * Flash operation progress information structure
 */
typedef struct baikal_scp_flash_progress_info {
	baikal_scp_flash_operation_t operation;
	unsigned int size;
	unsigned int offset;
	unsigned int bytes;
	unsigned int percent;
} baikal_scp_flash_progress_info_t;

/**
 * Flash operation progress callback function
 */
typedef void (*baikal_scp_flash_progress_cb_t)
	(const baikal_scp_flash_progress_info_t *progress_info);

/**
 * Initialize Baikal-M SCP library
 */
int baikal_scp_init(void);

/**
 * De-initialize Baikal-M SCP library
 */
void baikal_scp_deinit(void);

/**
 * Retrieve version information
 */
int baikal_scp_version(baikal_scp_version_info_t *version_info);

/**
 * Retrieve flash information
 */
int baikal_scp_flash_info(baikal_scp_flash_info_t *info);

/**
 * Read data from flash
 *
 * @param[in] offset Flash offset (must be aligned to 64 bytes)
 * @param[in] size   Read size in bytes (must be aligned to 64 bytes)
 * @param[in] dst    Pointer to the destination buffer
 *                   (size of the buffer must baikal greater or equal @param size)
 * @param[in] cb     Pointer to the progress callback function
 */
int baikal_scp_flash_read(
	unsigned int offset,
	unsigned int size,
	void *dst,
	baikal_scp_flash_progress_cb_t cb
);

/**
 * Write data to flash
 *
 * @param[in] offset Flash offset (must be aligned to 64 bytes)
 * @param[in] size   Write size in bytes (must be aligned to 64 bytes)
 * @param[in] dst    Pointer to the buffer with data
 *                   (size of the buffer must baikal greater or equal @param size)
 * @param[in] cb     Pointer to the progress callback function
 */
int baikal_scp_flash_write(
	unsigned int offset,
	unsigned int size,
	const void *src,
	baikal_scp_flash_progress_cb_t cb
);

/**
 * Erase data on flash
 *
 * @param[in] offset Flash offset (must be aligned to 64 bytes)
 * @param[in] size   Erase size in bytes (must be aligned to 64 bytes)
 * @param[in] cb     Pointer to the progress callback function
 */
int baikal_scp_flash_erase(
	unsigned int offset,
	unsigned int size,
	baikal_scp_flash_progress_cb_t cb
);

/**
 * Get size and offset required alignment size
 *
 * @return Alignment size
 */
unsigned int baikal_scp_flash_alignment(void);

#endif /* __KERNEL__ */

#endif /* BAIKAL_SCP_LIB_H */
