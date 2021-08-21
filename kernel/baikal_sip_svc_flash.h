/*
 * Copyright (c) 2020-2021, Baikal Electronics, JSC. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BAIKAL_SIP_SVC_FLASH_H
#define BAIKAL_SIP_SVC_FLASH_H

/* Must be in sync with
 * /arm-tf/plat/baikal/common/baikal_sip_svc_flash.c */
#define BAIKAL_SCP_FLASH_BUF_SIZE   (1024)

/* Must be in sync with
 * /arm-tf/plat/baikal/common/include/baikal_sip_svc_flash.h */
#define BAIKAL_SMC_FLASH            0x82000002
#define BAIKAL_SMC_FLASH_WRITE      (BAIKAL_SMC_FLASH + 0)
#define BAIKAL_SMC_FLASH_READ       (BAIKAL_SMC_FLASH + 1)
#define BAIKAL_SMC_FLASH_ERASE      (BAIKAL_SMC_FLASH + 2)
#define BAIKAL_SMC_FLASH_PUSH       (BAIKAL_SMC_FLASH + 3)
#define BAIKAL_SMC_FLASH_PULL       (BAIKAL_SMC_FLASH + 4)
#define BAIKAL_SMC_FLASH_POSITION   (BAIKAL_SMC_FLASH + 5)
#define BAIKAL_SMC_FLASH_INFO       (BAIKAL_SMC_FLASH + 6)

#endif /* BAIKAL_SIP_SVC_FLASH_H */
