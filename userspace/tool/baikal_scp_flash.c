/*
 * Copyright (C) 2021-2022 Tano Systems LLC, All rights reserved.
 *
 * Author: Anton Kikin <a.kikin@tano-systems.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "baikal_scp_tool.h"

#define MODE_NONE          0

#define MODE_SHOW_VERSION  1

#define MODE_FLASH_READ    10
#define MODE_FLASH_WRITE   11
#define MODE_FLASH_ERASE   12

static unsigned int mode = MODE_NONE;

static char        *filepath  = NULL;
static unsigned int filesize  = 0;
static unsigned int size      = 0;
static unsigned int offset    = 0;
static unsigned int skip      = 0;
static int          no_verify = 0;
static int          quiet     = 0;
static int          yes       = 0;

typedef struct flash_partition {
	char        *name;
	char        *desc;
	unsigned int smc_offset;
	unsigned int size;
} flash_partition_t;

/* Real flash offset is smc_offset + 0x80000 (scp binary) */
static flash_partition_t flash_parts[] = {
	{ .name = "bl1", .smc_offset = 0x000000, .size = 0x040000, .desc = "ARM Trusted Firmware (BL1)" },
	{ .name = "dtb", .smc_offset = 0x040000, .size = 0x040000, .desc = "Flattened Device Tree Blob (DTB)" },
	{ .name = "var", .smc_offset = 0x080000, .size = 0x0c0000, .desc = "EFI variables" },
	{ .name = "fip", .smc_offset = 0x140000, .size = 0x640000, .desc = "Firmware Image Package (FIP)" },
};

static const flash_partition_t *part = NULL;

static const flash_partition_t *find_part(char *name)
{
	for (int i = 0; i < sizeof(flash_parts) / sizeof(flash_parts[0]); i++) {
		if (strcmp(name, flash_parts[i].name) == 0) {
			return &flash_parts[i];
		}
	}

	return NULL;
}

/**
 * @brief Short command line options list
 */
static const char *opts_str = "hw:r:ep:s:o:k:yqnv";

/**
 * @brief Long command line options list
 */
static const struct option opts[] = {
	{ .name = "help",              .val = 'h' },
	{ .name = "write",             .val = 'w', .has_arg = 1 },
	{ .name = "read",              .val = 'r', .has_arg = 1 },
	{ .name = "erase",             .val = 'e' },
	{ .name = "part",              .val = 'p', .has_arg = 1 },
	{ .name = "size",              .val = 's', .has_arg = 1 },
	{ .name = "offset",            .val = 'o', .has_arg = 1 },
	{ .name = "skip",              .val = 'k', .has_arg = 1 },
	{ .name = "yes",               .val = 'y' },
	{ .name = "quiet",             .val = 'q' },
	{ .name = "no-verify",         .val = 'n' },
	{ .name = "version",           .val = 'v' },
	{ 0 }
};

/**
 * Display program usage help
 */
static void display_usage(void)
{
	int i;
	unsigned alignment = baikal_scp_flash_alignment();

	if (quiet) {
		return;
	}

	fprintf(stdout,
		"\n"
		"Baikal-M SCP SPI Boot Flash Utility version %u.%u.%u\n"
		"Copyright (c) 2021-2022, Tano Systems LLC, All Rights Reserved\n"
		"\n"
		"Usage: baikal-scp-flash [options]\n"
		"\n"
		"Options:\n"
		"  -h, --help\n"
		"        Show this help text.\n"
		"\n"
		"  -w, --write <filepath>\n"
		"        Write image to SPI Boot Flash from file <filepath>. You can select\n"
		"        SPI Boot Flash offset by built-in named partition (option -p, --part)\n"
		"        or manually specify flash offset (option -o, --offset) and write size\n"
		"        (option -s, --size). Also you can skip specified amount of bytes from\n"
		"        the beginning of input image file using the skip option (-k, --skip).\n"
		"\n"
		"  -r, --read <filepath>\n"
		"        Read SPI Boot Flash contents to file <filepath>. You can select\n"
		"        SPI Boot Flash offset by built-in named partition (option -p, --part)\n"
		"        or manually specify flash offset (option -o, --offset) and read size\n"
		"        (option -s, --size).\n"
		"\n"
		"  -e, --erase\n"
		"        Erase SPI Boot Flash contents. You can select SPI Boot Flash offset\n"
		"        by built-in named partition (option -p, --part) or manually specify\n"
		"        flash offset (option -o, --offset) and erase size (option -s, --size).\n"
		"\n"
		"  -p, --part <partition>\n"
		"        Select SPI Boot Flash offset and size by built-in named partition\n"
		"        for read (option -r, --read), write (option -w, --write) or/and erase\n"
		"        (option -e, --erase) operations. This option automatically sets\n"
		"        the size (option -s, --size) and offset (-o, --offset) to values\n"
		"        corresponding to the selected flash partition by name.\n"
		"\n"
		"        Available built-in partitions:\n"
		"        -----------+----------+----------+-----------------------------------\n"
		"         Partition | Offset   | Size     | Description\n"
		"        -----------+----------+----------+-----------------------------------\n",
		BAIKAL_SCP_TOOL_VERSION_MAJOR,
		BAIKAL_SCP_TOOL_VERSION_MINOR,
		BAIKAL_SCP_TOOL_VERSION_PATCH
	);

	for (i = 0; i < sizeof(flash_parts) / sizeof(flash_parts[0]); i++) {
		fprintf(stdout, "         %-9s | 0x%06x | 0x%06x | %s\n",
			flash_parts[i].name,
			flash_parts[i].smc_offset,
			flash_parts[i].size,
			flash_parts[i].desc ? flash_parts[i].desc : "");
	}

	fprintf(stdout,
		"        -----------+----------+----------+-----------------------------------\n"
		"\n"
		"  -s, --size <size>\n"
		"        Specify size for read (option -r, --read), write (option -w, --write)\n"
		"        or erase (option -e, --erase) operations. The specified size is\n"
		"        automatically up-aligned to the %u-byte boundary.\n"
		"\n"
		"  -o, --offset <offset>\n"
		"        Specify SPI Boot Flash offset for read (option -r, --read), write\n"
		"        (option -w, --write) or erase (option -e, --erase) operations. The\n"
		"        specified size is automatically up-aligned to the %u-byte boundary.\n"
		"\n"
		"  -k, --skip <skip>\n"
		"        The number of bytes to skip at the beginning of the input file\n"
		"        during a write (option -w, --write) operation.\n"
		"\n"
		"  -n, --no-verify\n"
		"        Do not read and verify the written data with the original data\n"
		"        during the write (option -w, --write) operation.\n"
		"\n"
		"  -y, --yes\n"
		"        Automatically confirm destructive operations (write, erase) without\n"
		"        displaying a prompt.\n"
		"\n"
		"  -q, --quiet\n"
		"        Be quiet. Do not output any messages to the standard output (stdout)\n"
		"        except the destructive operation confirmation prompt (unless the -y\n"
		"        option is specified).\n"
		"\n"
		"  -v, --version\n"
		"        Display information about utility, library and driver versions.\n"
		"\n",
		alignment,
		alignment
	);
}

/**
 * Parse command line arguments into @ref config global structure
 *
 * @param[in] argc  Number of arguments
 * @param[in] argv  Array of the pointers to the arguments
 *
 * @return 0 on success
 * @return <0 on error
 */
static int parse_cli_args(int argc, char *argv[])
{
	int opt;

	while((opt = getopt_long(argc, argv, opts_str, opts, NULL)) != EOF) {
		switch(opt) {
			case '?': {
				/* Invalid option */
				return EINVAL;
			}

			case 'h': { /* --help */
				display_usage();
				exit(0);
			}

			case 'w': { /* --write */
				if (mode == MODE_NONE) {
					mode = MODE_FLASH_WRITE;
					filepath = optarg;
				}

				break;
			}

			case 'r': { /* --read */
				if (mode == MODE_NONE) {
					mode = MODE_FLASH_READ;
					filepath = optarg;
				}
				break;
			}

			case 'e': { /* --erase */
				if (mode == MODE_NONE) {
					mode = MODE_FLASH_ERASE;
				}

				break;
			}

			case 'p': { /* --part */
				part = find_part(optarg);
				if (part) {
					offset = part->smc_offset;
					size   = part->size;
				}
				else {
					fprintf(stderr, "ERROR: Unknown partition '%s'\n", optarg);
					return EINVAL;
				}

				break;
			}

			case 's': { /* --size */
				size = strtoul(optarg, NULL, 0);
				break;
			}

			case 'o': { /* --offset */
				offset = strtoul(optarg, NULL, 0);
				break;
			}

			case 'k': { /* --skip */
				skip = strtoul(optarg, NULL, 0);
				break;
			}

			case 'y': { /* --yes */
				yes = 1;
				break;
			}

			case 'n': { /* --no-verify */
				no_verify = 1;
				break;
			}

			case 'q': { /* --quiet */
				quiet = 1;
				break;
			}

			case 'v': { /* --version */
				if (mode == MODE_NONE) {
					mode = MODE_SHOW_VERSION;
				}
				break;
			}

			default:
				break;
		}
	}

	if (mode == MODE_NONE) {
		fprintf(stderr, "ERROR: You must specify '--read', '--write' or '--erase' option\n");
		return EINVAL;
	}

	return 0;
}

static void baikal_scp_flash_progress_cb(
	const baikal_scp_flash_progress_info_t *progress_info)
{
	const char *strop = "UNKNOWN";
	static unsigned int percent = 100;

	if (progress_info->percent == percent)
		return;

	if (!quiet) {
		if (progress_info->operation == BAIKAL_SCP_FLASH_READ)
			strop = "Reading";
		else if (progress_info->operation == BAIKAL_SCP_FLASH_WRITE)
			strop = "Writing";
		else if (progress_info->operation == BAIKAL_SCP_FLASH_ERASE)
			strop = "Erasing";

		printf("\r%s: [+0x%x] 0x%08x / 0x%08x [%3u%%]",
			strop,
			progress_info->offset,
			progress_info->bytes,
			progress_info->size,
			progress_info->percent);

		fflush(stdout);
	}

	percent = progress_info->percent;
}

static void align_sizes(void)
{
	unsigned alignment = baikal_scp_flash_alignment();

	offset = ALIGN(offset, alignment);
	size   = ALIGN(size, alignment);
}

static int flash_read(int fhandle)
{
	int ret;
	void *buffer;

	align_sizes();

	buffer = calloc(size, 1);
	if (!buffer)
		return ENOMEM;

	ret = baikal_scp_flash_read(offset, size, buffer, baikal_scp_flash_progress_cb);

	if (!quiet) {
		printf("\n");
	}

	if (ret) {
		fprintf(stderr, "ERROR: Failed to read data from flash (%d)\n", ret);
		goto exit;
	}

	if (write(fhandle, buffer, size) != size) {
		ret = errno;
		fprintf(stderr, "ERROR: Failed to write flash data to file (%d)\n", ret);
		goto exit;
	}

	if (!quiet) {
		printf("OK: Success\n");
	}

exit:
	free(buffer);
	return ret;
}

static int flash_write(int fhandle)
{
	int ret;
	void *buffer;
	void *buffer_read;
	unsigned int file_read_size = size;

	align_sizes();

	buffer = calloc(size, 1);
	if (!buffer) {
		fprintf(stderr, "ERROR: Out of memory\n");
		return ENOMEM;
	}

	buffer_read = calloc(size, 1);
	if (!buffer_read) {
		fprintf(stderr, "ERROR: Out of memory\n");
		free(buffer);
		return ENOMEM;
	}

	/* 0. Read from file */
	if (read(fhandle, buffer, file_read_size) != file_read_size) {
		ret = errno;
		fprintf(stderr, "ERROR: Failed to read data from file (%d)\n", ret);
		goto exit;
	}

	/* 1. Erase */
	ret = baikal_scp_flash_erase(offset, size, baikal_scp_flash_progress_cb);

	if (!quiet) {
		printf("\n");
	}

	if (ret) {
		fprintf(stderr, "ERROR: Failed to erase flash data (%d)\n", ret);
		goto exit;
	}

	/* 2. Write */
	ret = baikal_scp_flash_write(offset, size, buffer, baikal_scp_flash_progress_cb);

	if (!quiet) {
		printf("\n");
	}

	if (ret) {
		fprintf(stderr, "ERROR: Failed to write data to flash (%d)\n", ret);
		goto exit;
	}

	if (!no_verify) {
		/* 3. Read */
		ret = baikal_scp_flash_read(offset, size, buffer_read, baikal_scp_flash_progress_cb);

		if (!quiet) {
			printf("\n");
		}

		if (ret) {
			fprintf(stderr, "ERROR: Failed to read data from flash (%d)\n", ret);
			goto exit;
		}

		/* 4. Verify */
		if (memcmp(buffer, buffer_read, size)) {
			fprintf(stderr, "ERROR: Verification failed\n");
			goto exit;
		}
	}

	if (!quiet) {
		printf("OK: Success\n");
	}

exit:
	free(buffer_read);
	free(buffer);
	return ret;
}

static int flash_erase(int fhandle)
{
	int ret;

	align_sizes();

	ret = baikal_scp_flash_erase(offset, size, baikal_scp_flash_progress_cb);
	printf("\n");
	if (ret) {
		fprintf(stderr, "\nERROR: Failed to erase flash data (%d)\n", ret);
		return ret;
	}

	printf("OK: Success\n");

	return ret;
}

int display_version(void)
{
	int ret;
	baikal_scp_version_info_t version_info;

	fprintf(stdout, "baikal-scp-tool version %u.%u.%u\n",
		BAIKAL_SCP_TOOL_VERSION_MAJOR,
		BAIKAL_SCP_TOOL_VERSION_MINOR,
		BAIKAL_SCP_TOOL_VERSION_PATCH);

	ret = baikal_scp_version(&version_info);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to retrieve version information (%d)\n", ret);
		return ret;
	}

	fprintf(stdout, "baikal-scp-lib version %u.%u.%u\n",
		BAIKAL_SCP_VERSION_GET_MAJOR(version_info.lib_version),
		BAIKAL_SCP_VERSION_GET_MINOR(version_info.lib_version),
		BAIKAL_SCP_VERSION_GET_PATCH(version_info.lib_version));

	fprintf(stdout, "baikal-scp version %u.%u.%u\n",
		BAIKAL_SCP_VERSION_GET_MAJOR(version_info.drv_version),
		BAIKAL_SCP_VERSION_GET_MINOR(version_info.drv_version),
		BAIKAL_SCP_VERSION_GET_PATCH(version_info.drv_version));

	return ret;
}

int main(int argc, char *argv[])
{
	int fh = -1;
	int ret;
	baikal_scp_flash_info_t flash_info;

	ret = parse_cli_args(argc, argv);
	if (ret) {
		display_usage();
		return ret;
	}

	ret = baikal_scp_init();
	if (ret) {
		fprintf(stderr, "ERROR: Failed to initialize Baikal SCP library (%d)\n", ret);
		return ret;
	}

	ret = baikal_scp_flash_info(&flash_info);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to retrieve flash information (%d)\n", ret);
		baikal_scp_deinit();
		return ret;
	}

	switch(mode) {
		case MODE_SHOW_VERSION:
			ret = display_version();
			break;

		case MODE_FLASH_READ:
			if (!size)
				size = flash_info.total_size - offset;

			if ((fh = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
				ret = errno;
				fprintf(stderr, "ERROR: Cannot open \"%s\" for writing (%d)\n", filepath, ret);
				break;
			}

			ret = flash_read(fh);
			break;

		case MODE_FLASH_WRITE: {
			struct stat st;
			stat(filepath, &st);
			filesize = st.st_size;

			if (!size)
				size = (filesize < flash_info.total_size)
					? filesize : flash_info.total_size;

			if (size > filesize)
				size = filesize;

			if (skip > filesize)
				skip = filesize;

			if (skip + size > filesize)
				size = filesize - skip;

			if (!size ||
			    ((size + skip) > filesize) ||
			    ((size + offset) > flash_info.total_size)) {
				ret = EINVAL;
				fprintf(stderr, "ERROR: Invalid size, offset or skip value or its combination\n");
				break;
			}

			if ((fh = open(filepath, O_RDONLY)) == -1) {
				ret = errno;
				fprintf(stderr, "ERROR: Cannot open \"%s\" for reading (%d)\n", filepath, ret);
				break;
			}

			if (skip)
				lseek(fh, skip, SEEK_SET);

			if (!quiet) {
				fprintf(stdout, "Writing 0x%x bytes to SPI Boot Flash at offset 0x%0x\n",
					size, offset);
			}

			if (!yes) {
				char s[2];
				fprintf(stdout, "Continue? [y/N] ");
				fflush(stdout);
				if (!fgets(s, 2, stdin) || (s[0] != 'y' && s[0] != 'Y')) {
					goto exit;
				}
			}

			ret = flash_write(fh);
			break;
		}

		case MODE_FLASH_ERASE:
			if (!size)
				size = flash_info.total_size - offset;

			if (!quiet) {
				fprintf(stdout, "Erasing 0x%x bytes of SPI Boot Flash at offset 0x%0x\n",
					size, offset);
			}

			if (!yes) {
				char s[2];
				fprintf(stdout, "Continue? [y/N] ");
				fflush(stdout);
				if (!fgets(s, 2, stdin) || (s[0] != 'y' && s[0] != 'Y')) {
					goto exit;
				}
			}

			ret = flash_erase(fh);
			break;

		default:
			ret = EINVAL;
			break;
	}

exit:
	if (fh != -1)
		close(fh);

	baikal_scp_deinit();
	return ret;
}
