/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/arch/clock.h>
#include <linux/ctype.h>
#ifdef CONFIG_FSL_ESDHC
#include <fsl_esdhc.h>
#endif
#include <tsec.h>

DECLARE_GLOBAL_DATA_PTR;

void ft_fixup_enet_phy_connect_type(void *fdt)
{
	struct eth_device *dev;
	struct tsec_private *priv;
	const char *enet_path, *phy_path;
	char enet[16];
	char phy[16];
	int phy_node;
	int i = 0;
	int enet_id = 0;
	uint32_t ph;

	while ((dev = eth_get_dev_by_index(i++)) != NULL) {
		if (strstr(dev->name, "eTSEC1"))
			enet_id = 0;
		else if (strstr(dev->name, "eTSEC2"))
			enet_id = 1;
		else if (strstr(dev->name, "eTSEC3"))
			enet_id = 2;
		else
			continue;

		priv = dev->priv;
		if (priv->flags & TSEC_SGMII)
			continue;

		sprintf(enet, "ethernet%d", enet_id);
		enet_path = fdt_get_alias(fdt, enet);
		if (!enet_path)
			continue;

		sprintf(phy, "enet%d_rgmii_phy", enet_id);
		phy_path = fdt_get_alias(fdt, phy);
		if (!phy_path)
			continue;

		phy_node = fdt_path_offset(fdt, phy_path);
		if (phy_node < 0)
			continue;

		ph = fdt_create_phandle(fdt, phy_node);
		if (ph)
			do_fixup_by_path_u32(fdt, enet_path,
					     "phy-handle", ph, 1);

		do_fixup_by_path(fdt, enet_path, "phy-connection-type",
				 phy_string_for_interface(
				 PHY_INTERFACE_MODE_RGMII_ID),
				 sizeof(phy_string_for_interface(
				 PHY_INTERFACE_MODE_RGMII_ID)),
				 1);
	}
}

void ft_cpu_setup(void *blob, bd_t *bd)
{
	int off;
	int val;
	const char *sysclk_path;

	unsigned long busclk = get_bus_freq(0);

	fdt_fixup_ethernet(blob);

	off = fdt_node_offset_by_prop_value(blob, -1, "device_type", "cpu", 4);
	while (off != -FDT_ERR_NOTFOUND) {
		val = gd->cpu_clk;
		fdt_setprop(blob, off, "clock-frequency", &val, 4);
		off = fdt_node_offset_by_prop_value(blob, off,
						    "device_type", "cpu", 4);
	}

	do_fixup_by_prop_u32(blob, "device_type", "soc",
			     4, "bus-frequency", busclk, 1);

	ft_fixup_enet_phy_connect_type(blob);

#ifdef CONFIG_SYS_NS16550
	do_fixup_by_compat_u32(blob, "fsl,16550-FIFO64",
			       "clock-frequency", CONFIG_SYS_NS16550_CLK, 1);
#endif

	sysclk_path = fdt_get_alias(blob, "sysclk");
	if (sysclk_path)
		do_fixup_by_path_u32(blob, sysclk_path, "clock-frequency",
				     CONFIG_SYS_CLK_FREQ, 1);
	do_fixup_by_compat_u32(blob, "fsl,qoriq-sysclk-2.0",
			       "clock-frequency", CONFIG_SYS_CLK_FREQ, 1);

#if defined(CONFIG_FSL_ESDHC)
	fdt_fixup_esdhc(blob, bd);
#endif

	/*
	 * platform bus clock = system bus clock/2
	 * Here busclk = system bus clock
	 * We are using the platform bus clock as 1588 Timer reference
	 * clock source select
	 */
	do_fixup_by_compat_u32(blob, "fsl, gianfar-ptp-timer",
			       "timer-frequency", busclk / 2, 1);

	/*
	 * clock-freq should change to clock-frequency and
	 * flexcan-v1.0 should change to p1010-flexcan respectively
	 * in the future.
	 */
	do_fixup_by_compat_u32(blob, "fsl, flexcan-v1.0",
			       "clock_freq", busclk / 2, 1);

	do_fixup_by_compat_u32(blob, "fsl, flexcan-v1.0",
			       "clock-frequency", busclk / 2, 1);

	do_fixup_by_compat_u32(blob, "fsl, ls1021a-flexcan",
			       "clock-frequency", busclk / 2, 1);
}
