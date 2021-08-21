// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Baikal-M (BE-M1000) SCP communication driver
 *
 * Copyright (C) 2021 Tano Systems LLC. All rights reserved.
 *
 * Authors: Anton Kikin <a.kikin@tano-systems.com>
 */

#include "baikal_scp_private.h"

static baikal_scp_dev_t *scpdev = NULL;

static int baikal_scp_dev_fop_open(struct inode *inode, struct file *file)
{
	mutex_lock(&scpdev->lock);

	if (scpdev->open_counter) {
		/* Only one instance of the user-space tool can work with SCP device */
		mutex_unlock(&scpdev->lock);
		return -EBUSY;
	}

	++scpdev->open_counter;

	mutex_unlock(&scpdev->lock);
	return 0;
}

static int baikal_scp_dev_fop_flush(struct file *file, fl_owner_t id)
{
	mutex_lock(&scpdev->lock);
	--scpdev->open_counter;
	mutex_unlock(&scpdev->lock);
	return 0;
}

static const struct file_operations baikal_scp_dev_fops = {
	.owner           = THIS_MODULE,
	.open            = baikal_scp_dev_fop_open,
	.flush           = baikal_scp_dev_fop_flush,
	.unlocked_ioctl  = baikal_scp_dev_fop_ioctl,
	.compat_ioctl    = baikal_scp_dev_fop_ioctl,
};

static baikal_scp_dev_t *baikal_scp_dev_init(void)
{
	baikal_scp_dev_t *dev;
	int ret;

	dev = (baikal_scp_dev_t *)kzalloc(
		sizeof(baikal_scp_dev_t), GFP_KERNEL);

	if (!dev) {
		pr_err("Can't allocate memory for control device\n");
		return NULL;
	}

	ret = alloc_chrdev_region(&dev->region, 0, 1, BAIKAL_SCP_DRV_NAME);
	if (ret) {
		pr_err("Could not allocate the region for control device (%d)\n", ret);
		kfree(dev);
		return NULL;
	}

	dev->class = class_create(THIS_MODULE, BAIKAL_SCP_DEV_CLASS_NAME);
	if (!dev->class) {
		pr_err("Could not create device class for control device\n");
		unregister_chrdev_region(dev->region, 1);
		kfree(dev);
		return NULL;
	}

	cdev_init(&dev->chrdev, &baikal_scp_dev_fops);

	dev->major        = MAJOR(dev->region);
	dev->chrdev.owner = THIS_MODULE;
	dev->chrdev.ops   = &baikal_scp_dev_fops;

	ret = cdev_add(&dev->chrdev, MKDEV(dev->major, 0), 1);
	if (ret) {
		pr_err("Failed to add control device (%d)\n", ret);
		class_destroy(dev->class);
		unregister_chrdev_region(dev->region, 1);
		kfree(dev);
		return NULL;
	}

	device_create(dev->class, NULL, MKDEV(dev->major, 0),
		(void *)dev, BAIKAL_SCP_DEV_NAME
	);

	mutex_init(&dev->lock);

	pr_debug("Created control device \"/dev/%s\"", BAIKAL_SCP_DEV_NAME);
	return dev;
}

static void baikal_scp_dev_destroy(baikal_scp_dev_t *dev)
{
	device_destroy(dev->class, MKDEV(dev->major, 0));
	cdev_del(&dev->chrdev);
	class_destroy(dev->class);
	unregister_chrdev_region(dev->region, 1);
	kfree(dev);
}

static int __init baikal_scp_init_module(void)
{
	scpdev = baikal_scp_dev_init();
	if (!scpdev)
		return -1;

	printk(BAIKAL_SCP_DRV_DESCRIPTION " version " BAIKAL_SCP_DRV_VERSION_STR " loaded\n");

	return 0;
}

static void __exit baikal_scp_cleanup_module(void)
{
	baikal_scp_dev_destroy(scpdev);
	printk(BAIKAL_SCP_DRV_DESCRIPTION " unloaded\n");
}

module_init(baikal_scp_init_module);
module_exit(baikal_scp_cleanup_module);

MODULE_AUTHOR("Anton Kikin <a.kikin@tano-systems.com>");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION(BAIKAL_SCP_DRV_DESCRIPTION);
MODULE_VERSION(BAIKAL_SCP_DRV_VERSION_STR);
