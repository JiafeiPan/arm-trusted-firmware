/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <bl_common.h>
#include <desc_image_load.h>
#include "ls_16550.h"
#include "plat_ls.h"
#include "ls_def.h"
#include "../../../bl2/bl2_private.h"

/* Data structure which holds the extents of the trusted SRAM for BL2 */
static meminfo_t bl2_tzram_layout __aligned(CACHE_WRITEBACK_GRANULE);

void ls_bl2_early_platform_setup(void)
{
	static console_ls_16550_t console;

	/* Initialize the console to provide early debug support */
	console_ls_16550_register(LS_TF_UART_BASE, LS_TF_UART_CLOCK,
			       LS_TF_UART_BAUDRATE, &console);

	/* Allow BL2 to see the whole Trusted RAM */
	bl2_tzram_layout.total_base = LS_SRAM_BASE;
	bl2_tzram_layout.total_size = LS_SRAM_SIZE;

	/* Initialise the IO layer and register platform IO devices */
	plat_ls_io_setup();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only initializes the mmu in a quick and dirty way.
 ******************************************************************************/
void ls_bl2_plat_arch_setup(void)
{
	ls_setup_page_tables(bl2_tzram_layout.total_base,
			      bl2_tzram_layout.total_size,
			      BL_CODE_BASE,
#if BL2_IN_XIP_MEM
			      BL2_CODE_END,
			      BL2_RO_DATA_BASE,
			      BL2_RO_DATA_END
#else
			      BL_CODE_END,
			      BL_RO_DATA_BASE,
			      BL_RO_DATA_END
#endif
#if USE_COHERENT_MEM
			      , BL_COHERENT_RAM_BASE,
			      BL_COHERENT_RAM_END
#endif
			      );

#ifdef AARCH32
	enable_mmu_secure(0);
#else
	enable_mmu_el3(0);
#endif
}

void bl2_el3_plat_arch_setup(void)
{
	ls_bl2_plat_arch_setup();
}

int ls_bl2_handle_post_image_load(unsigned int image_id)
{
	int err = 0;
	bl_mem_params_node_t *bl_mem_params = get_bl_mem_params_node(image_id);

	assert(bl_mem_params);

	switch (image_id) {
#ifdef AARCH64
	case BL32_IMAGE_ID:
		bl_mem_params->ep_info.spsr = ls_get_spsr_for_bl32_entry();
		break;
#endif

	case BL33_IMAGE_ID:
		/* BL33 expects to receive the primary CPU MPID (through r0) */
		bl_mem_params->ep_info.args.arg0 = 0xffff & read_mpidr();
		bl_mem_params->ep_info.spsr = ls_get_spsr_for_bl33_entry();
		break;
	}

	return err;
}

/*******************************************************************************
 * This function can be used by the platforms to update/use image
 * information for given `image_id`.
 ******************************************************************************/
int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	return ls_bl2_handle_post_image_load(image_id);
}
