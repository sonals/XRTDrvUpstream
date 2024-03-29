// SPDX-License-Identifier: GPL-2.0

/*
 * FPGA Manager bindings for XRT driver
 *
 * Copyright (C) 2019 Xilinx, Inc. All rights reserved.
 *
 * Authors: Sonal Santan
 *
 */

#include <linux/fpga/fpga-mgr.h>

#include "../xocl_drv.h"
#include "../xclbin.h"

/*
 * Container to capture and cache full xclbin as it is passed in blocks by FPGA
 * Manager. xocl needs access to full xclbin to walk through xclbin sections. FPGA
 * Manager's .write() backend sends incremental blocks without any knowledge of
 * xclbin format forcing us to collect the blocks and stitch them together here.
 * TODO:
 * 1. Add a variant of API, icap_download_bitstream_axlf() which works off kernel buffer
 * 2. Call this new API from FPGA Manager's write complete hook, xocl_pr_write_complete()
 */

struct xfpga_klass {
	struct xocl_dev *xdev;
	struct axlf *blob;
	char name[64];
	size_t count;
	enum fpga_mgr_states state;
};

static int xocl_pr_write_init(struct fpga_manager *mgr,
			      struct fpga_image_info *info, const char *buf, size_t count)
{
	struct xfpga_klass *obj = mgr->priv;
	const struct axlf *bin = (const struct axlf *)buf;

	if (count < sizeof(struct axlf)) {
		obj->state = FPGA_MGR_STATE_WRITE_INIT_ERR;
		return -EINVAL;
	}

	if (count > bin->m_header.m_length) {
		obj->state = FPGA_MGR_STATE_WRITE_INIT_ERR;
		return -EINVAL;
	}

	/* Free up the previous blob */
	vfree(obj->blob);
	obj->blob = vmalloc(bin->m_header.m_length);
	if (!obj->blob) {
		obj->state = FPGA_MGR_STATE_WRITE_INIT_ERR;
		return -ENOMEM;
	}

	memcpy(obj->blob, buf, count);
	xocl_info(&mgr->dev, "Begin download of xclbin %pUb of length %lld B", &obj->blob->m_header.uuid,
		  obj->blob->m_header.m_length);
	obj->count = count;
	obj->state = FPGA_MGR_STATE_WRITE_INIT;
	return 0;
}

static int xocl_pr_write(struct fpga_manager *mgr,
			 const char *buf, size_t count)
{
	struct xfpga_klass *obj = mgr->priv;
	char *curr = (char *)obj->blob;

	if ((obj->state != FPGA_MGR_STATE_WRITE_INIT) && (obj->state != FPGA_MGR_STATE_WRITE)) {
		obj->state = FPGA_MGR_STATE_WRITE_ERR;
		return -EINVAL;
	}

	curr += obj->count;
	obj->count += count;
	/* Check if the xclbin buffer is not longer than advertised in the header */
	if (obj->blob->m_header.m_length < obj->count) {
		obj->state = FPGA_MGR_STATE_WRITE_ERR;
		return -EINVAL;
	}
	memcpy(curr, buf, count);
	xocl_info(&mgr->dev, "Next block of %zu B of xclbin %pUb", count, &obj->blob->m_header.uuid);
	obj->state = FPGA_MGR_STATE_WRITE;
	return 0;
}


static int xocl_pr_write_complete(struct fpga_manager *mgr,
				  struct fpga_image_info *info)
{
	int result;
	struct xfpga_klass *obj = mgr->priv;

	if (obj->state != FPGA_MGR_STATE_WRITE) {
		obj->state = FPGA_MGR_STATE_WRITE_COMPLETE_ERR;
		return -EINVAL;
	}

	/* Check if we got the complete xclbin */
	if (obj->blob->m_header.m_length != obj->count) {
		obj->state = FPGA_MGR_STATE_WRITE_COMPLETE_ERR;
		return -EINVAL;
	}
	/* Send the xclbin blob to actual download framework in icap */
	result = xocl_icap_download_axlf(obj->xdev, obj->blob);
	obj->state = result ? FPGA_MGR_STATE_WRITE_COMPLETE_ERR : FPGA_MGR_STATE_WRITE_COMPLETE;
	xocl_info(&mgr->dev, "Finish download of xclbin %pUb of size %zu B", &obj->blob->m_header.uuid, obj->count);
	vfree(obj->blob);
	obj->blob = NULL;
	obj->count = 0;
	return result;
}

static enum fpga_mgr_states xocl_pr_state(struct fpga_manager *mgr)
{
	struct xfpga_klass *obj = mgr->priv;

	return obj->state;
}

static const struct fpga_manager_ops xocl_pr_ops = {
	.initial_header_size = sizeof(struct axlf),
	.write_init = xocl_pr_write_init,
	.write = xocl_pr_write,
	.write_complete = xocl_pr_write_complete,
	.state = xocl_pr_state,
};


struct platform_device_id fmgr_id_table[] = {
	{ XOCL_FMGR, 0 },
	{ },
};

static int fmgr_probe(struct platform_device *pdev)
{
	struct fpga_manager *mgr;
	int ret = 0;
	struct xfpga_klass *obj = kzalloc(sizeof(struct xfpga_klass), GFP_KERNEL);

	if (!obj)
		return -ENOMEM;

	obj->xdev = xocl_get_xdev(pdev);
	snprintf(obj->name, sizeof(obj->name), "Xilinx PCIe FPGA Manager");

	obj->state = FPGA_MGR_STATE_UNKNOWN;
	mgr = fpga_mgr_create(&pdev->dev, obj->name, &xocl_pr_ops, obj);
	if (!mgr) {
		ret = -ENODEV;
		goto out;
	}
	ret = fpga_mgr_register(mgr);
	if (ret)
		goto out;

	return ret;
out:
	kfree(obj);
	return ret;
}

static int fmgr_remove(struct platform_device *pdev)
{
	struct fpga_manager *mgr = platform_get_drvdata(pdev);
	struct xfpga_klass *obj = mgr->priv;

	obj->state = FPGA_MGR_STATE_UNKNOWN;
	fpga_mgr_unregister(mgr);

	platform_set_drvdata(pdev, NULL);
	vfree(obj->blob);
	kfree(obj);
	return 0;
}

static struct platform_driver	fmgr_driver = {
	.probe		= fmgr_probe,
	.remove		= fmgr_remove,
	.driver		= {
		.name = "xocl_fmgr",
	},
	.id_table = fmgr_id_table,
};

int __init xocl_init_fmgr(void)
{
	return platform_driver_register(&fmgr_driver);
}

void xocl_fini_fmgr(void)
{
	platform_driver_unregister(&fmgr_driver);
}
