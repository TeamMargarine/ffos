/*
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/semaphore.h>

#include <mach/hardware.h>
#include <mach/regs_glb.h>
#include <mach/regs_ahb.h>
#include <mach/sci.h>

#include <mach/memfreq_ondemand.h>

#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"

#define MALI_BANDWIDTH (1600*1024*1024)

u32 calculate_gpu_utilization(void* arg);

static unsigned int mali_memfreq_demand(struct memfreq_dbs *h)
{
	u32 bw = calculate_gpu_utilization(NULL);
	return (MALI_BANDWIDTH>>8)*bw;
}

static struct memfreq_dbs mali_memfreq_desc = {
	.level = 0,
	.memfreq_demand = mali_memfreq_demand,
};

static DEFINE_SEMAPHORE(g_gpu_clock_lock);

static struct clk* g_gpu_clock = NULL;

static int g_gpu_clock_on = 0;

_mali_osk_errcode_t mali_platform_init(void)
{
	g_gpu_clock = clk_get(NULL, "clk_gpu_axi");

	MALI_DEBUG_ASSERT(g_gpu_clock);

//	register_memfreq_ondemand (&mali_memfreq_desc);

	down(&g_gpu_clock_lock);
	sci_glb_clr(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
	while(sci_glb_read(REG_GLB_G3D_PWR_CTL, BITS_PD_G3D_STATUS(0x1f))) udelay(100);
	if(!g_gpu_clock_on)
	{
		g_gpu_clock_on = 1;
		clk_enable(g_gpu_clock);
	}
	up(&g_gpu_clock_lock);
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
	down(&g_gpu_clock_lock);
	if(g_gpu_clock_on)
	{
		g_gpu_clock_on = 0;
		clk_disable(g_gpu_clock);
	}
	sci_glb_set(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
	up(&g_gpu_clock_lock);

//	unregister_memfreq_ondemand (&mali_memfreq_desc);

	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	down(&g_gpu_clock_lock);
	switch(power_mode)
	{
	case MALI_POWER_MODE_ON:
		if(sci_glb_read(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD))
		{
			sci_glb_clr(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
			while(sci_glb_read(REG_GLB_G3D_PWR_CTL, BITS_PD_G3D_STATUS(0x1f))) udelay(100);
		}
		if(!g_gpu_clock_on)
		{
			g_gpu_clock_on = 1;
			clk_enable(g_gpu_clock);
		}
		break;
	case MALI_POWER_MODE_LIGHT_SLEEP:
		if(g_gpu_clock_on)
		{
			g_gpu_clock_on = 0;
			clk_disable(g_gpu_clock);
		}
		break;
	case MALI_POWER_MODE_DEEP_SLEEP:
		if(g_gpu_clock_on)
		{
			g_gpu_clock_on = 0;
			clk_disable(g_gpu_clock);
		}
		sci_glb_set(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
		break;
	};
	up(&g_gpu_clock_lock);
	MALI_SUCCESS;
}

static int g_gpu_clock_div = 1;

void mali_gpu_utilization_handler(u32 utilization)
{
#if 0
	// if the loading ratio is greater then 90%, switch the clock to the maximum
	if(utilization >= (256*9/10))
	{
		g_gpu_clock_div = 1;
		sci_glb_write(REG_GLB_GEN2, BITS_CLK_GPU_AXI_DIV(g_gpu_clock_div-1), BITS_CLK_GPU_AXI_DIV(7));
		return;
	}

	if(utilization == 0)
	{
		utilization = 1;
	}

	// the absolute loading ratio is 1/g_gpu_clock_div * utilization/256
	// to keep the loading ratio under 70% at a certain level,
	// the absolute loading level is 1/(1/g_gpu_clock_div * utilization/256 / (7/10))
	g_gpu_clock_div = (256*7/10)*g_gpu_clock_div/utilization;

	if(g_gpu_clock_div < 1) g_gpu_clock_div = 1;
	if(g_gpu_clock_div > 8) g_gpu_clock_div = 8;

	sci_glb_write(REG_GLB_GEN2, BITS_CLK_GPU_AXI_DIV(g_gpu_clock_div-1), BITS_CLK_GPU_AXI_DIV(7));
#endif
}

void set_mali_parent_power_domain(void* dev)
{
}

