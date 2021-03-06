/*
 * Copyright (C) 2005 Sascha Hauer, Pengutronix
 * Copyright (C) 2007 Wolfgang Grandegger <wg@grandegger.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/platform/sja1000.h>
#include <linux/io.h>

#include "sja1000.h"

#define DRV_NAME "sja1000_platform"

MODULE_AUTHOR("Sascha Hauer <s.hauer@pengutronix.de>");
MODULE_DESCRIPTION("Socket-CAN driver for SJA1000 on the platform bus");
MODULE_LICENSE("GPL v2");

static u8 sp_read_reg(struct net_device *dev, int reg)
{
	return ioread8((void __iomem *)(dev->base_addr + reg));
}

static void sp_write_reg(struct net_device *dev, int reg, u8 val)
{
	iowrite8(val, (void __iomem *)(dev->base_addr + reg));
}

static int sp_probe(struct platform_device *pdev)
{
	int err, irq;
	void __iomem *addr;
	struct net_device *dev;
	struct sja1000_priv *priv;
	struct resource *res_mem, *res_irq;
	struct sja1000_platform_data *pdata;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data provided!\n");
		err = -ENODEV;
		goto exit;
	}

	res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res_mem || !res_irq) {
		err = -ENODEV;
		goto exit;
	}

	if (!request_mem_region(res_mem->start,
				res_mem->end - res_mem->start + 1,
				DRV_NAME)) {
		err = -EBUSY;
		goto exit;
	}

	addr = ioremap_nocache(res_mem->start,
			       res_mem->end - res_mem->start + 1);
	if (!addr) {
		err = -ENOMEM;
		goto exit_release;
	}

	irq = res_irq->start;
	if (res_irq->flags & IRQF_TRIGGER_MASK)
		set_irq_type(irq, res_irq->flags & IRQF_TRIGGER_MASK);

	dev = alloc_sja1000dev(0);
	if (!dev) {
		err = -ENOMEM;
		goto exit_iounmap;
	}
	priv = netdev_priv(dev);

	priv->read_reg = sp_read_reg;
	priv->write_reg = sp_write_reg;
	priv->can.bittiming.clock = pdata->clock;
	priv->ocr = pdata->ocr;
	priv->cdr = pdata->cdr;

	dev->irq = irq;
	dev->base_addr = (unsigned long)addr;

	dev_set_drvdata(&pdev->dev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	err = register_sja1000dev(dev);
	if (err) {
		dev_err(&pdev->dev, "registering %s failed (err=%d)\n",
			DRV_NAME, err);
		goto exit_free;
	}

	dev_info(&pdev->dev, "%s device registered (base_addr=%#lx, irq=%d)\n",
		 DRV_NAME, dev->base_addr, dev->irq);
	return 0;

 exit_free:
	free_sja1000dev(dev);
 exit_iounmap:
	iounmap(addr);
 exit_release:
	release_mem_region(res_mem->start, res_mem->end - res_mem->start + 1);
 exit:
	return err;
}

static int sp_remove(struct platform_device *pdev)
{
	struct net_device *dev = dev_get_drvdata(&pdev->dev);
	struct resource *res;

	unregister_sja1000dev(dev);
	dev_set_drvdata(&pdev->dev, NULL);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, res->end - res->start + 1);

	if (dev->base_addr)
		iounmap((void __iomem *)dev->base_addr);

	free_sja1000dev(dev);

	return 0;
}

static struct platform_driver sp_driver = {
	.probe = sp_probe,
	.remove = sp_remove,
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init sp_init(void)
{
	return platform_driver_register(&sp_driver);
}

static void __exit sp_exit(void)
{
	platform_driver_unregister(&sp_driver);
}

module_init(sp_init);
module_exit(sp_exit);
