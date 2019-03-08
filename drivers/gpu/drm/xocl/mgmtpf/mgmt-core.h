/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Copyright (C) 2017-2019 Xilinx, Inc.
 *
 * Author(s):
 * Sonal Santan <sonal.santan@xilinx.com>
 */

#ifndef _XCL_MGT_PF_H_
#define _XCL_MGT_PF_H_

#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/signal.h>
#include <linux/init_task.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/types.h>
#include <asm/io.h>
#include "../mgmt-reg.h"
#include <drm/xmgmt_drm.h>
#include "../xclfeatures.h"
#include "../xocl_drv.h"

#define DRV_NAME "xclmgmt"

#define	MGMT_READ_REG32(lro, off)	\
	ioread32(lro->core.bar_addr + off)
#define	MGMT_WRITE_REG32(lro, off, val)	\
	iowrite32(val, lro->core.bar_addr + off)
#define	MGMT_WRITE_REG8(lro, off, val)	\
	iowrite8(val, lro->core.bar_addr + off)

#define	mgmt_err(lro, fmt, args...)	\
	dev_err(&lro->core.pdev->dev, "%s: "fmt, __func__, ##args)
#define	mgmt_info(lro, fmt, args...)	\
	dev_info(&lro->core.pdev->dev, "%s: "fmt, __func__, ##args)

#define	MGMT_PROC_TABLE_HASH_SZ		256

struct xclmgmt_ioc_info;

// List of processes that are using the mgmt driver
// also saving the task
struct proc_list {
	struct list_head head;
	struct pid      *pid;
	bool		 signaled;
};

struct power_val {
	s32 max;
	s32 avg;
	s32 curr;
};

struct mgmt_power {
	struct power_val vccint;
	struct power_val vcc1v8;
	struct power_val vcc1v2;
	struct power_val vccbram;
	struct power_val mgtavcc;
	struct power_val mgtavtt;
};

struct xclmgmt_proc_ctx {
	struct xclmgmt_dev	*lro;
	struct pid		*pid;
	bool			signaled;
};

struct xclmgmt_char {
	struct xclmgmt_dev *lro;
	struct cdev *cdev;
	struct device *sys_device;
};

struct xclmgmt_data_buf {
	enum mb_cmd_type cmd_type;
	uint64_t priv_data;
	char *data_buf;
};

struct xclmgmt_dev {
	struct xocl_dev_core	core;
	/* MAGIC_DEVICE == 0xAAAAAAAA */
	unsigned long magic;

	/* the kernel pci device data structure provided by probe() */
	struct pci_dev *pci_dev;
	int instance;
	struct xclmgmt_char user_char_dev;
	int axi_gate_frozen;
	unsigned short ocl_frequency[4];

	struct mutex busy_mutex;
	struct mgmt_power power;

	int msix_user_start_vector;
	bool ready;

};

extern int health_check;

int ocl_freqscaling_ioctl(struct xclmgmt_dev *lro, const void __user *arg);
void platform_axilite_flush(struct xclmgmt_dev *lro);
u16 get_dsa_version(struct xclmgmt_dev *lro);
void fill_frequency_info(struct xclmgmt_dev *lro, struct xclmgmt_ioc_info *obj);
void device_info(struct xclmgmt_dev *lro, struct xclmgmt_ioc_info *obj);
long mgmt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
void get_pcie_link_info(struct xclmgmt_dev *lro,
			unsigned short *width, unsigned short *speed, bool is_cap);

// utils.c
unsigned int compute_unit_busy(struct xclmgmt_dev *lro);
int pci_fundamental_reset(struct xclmgmt_dev *lro);

long reset_hot_ioctl(struct xclmgmt_dev *lro);
void xdma_reset(struct pci_dev *pdev, bool prepare);
void xclmgmt_reset_pci(struct xclmgmt_dev *lro);

// firewall.c
void init_firewall(struct xclmgmt_dev *lro);
void xclmgmt_killall_processes(struct xclmgmt_dev *lro);
void xclmgmt_list_add(struct xclmgmt_dev *lro, struct pid *new_pid);
void xclmgmt_list_remove(struct xclmgmt_dev *lro, struct pid *remove_pid);
void xclmgmt_list_del(struct xclmgmt_dev *lro);
bool xclmgmt_check_proc(struct xclmgmt_dev *lro, struct pid *pid);

// mgmt-xvc.c
long xvc_ioctl(struct xclmgmt_dev *lro, const void __user *arg);

//mgmt-sysfs.c
int mgmt_init_sysfs(struct device *dev);
void mgmt_fini_sysfs(struct device *dev);

//mgmt-mb.c
int mgmt_init_mb(struct xclmgmt_dev *lro);
void mgmt_fini_mb(struct xclmgmt_dev *lro);
int mgmt_start_mb(struct xclmgmt_dev *lro);
int mgmt_stop_mb(struct xclmgmt_dev *lro);

#endif
