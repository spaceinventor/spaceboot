/*
 * spaceboot_legacy.c
 *
 *  Created on: Mar 17, 2017
 *      Author: johan
 */

#include "spaceboot_bootalt.h"

#include <stdint.h>
#include <stdio.h>
#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <param/param.h>
#include <param/param_list.h>
#include <param/param_client.h>


void spaceboot_bootalt(int node, int value)
{
	/* Find parameter */
	param_t * boot_alt = param_list_find_id(node, 20);

	/* Define temporary parameter */
	if (boot_alt == NULL) {
		boot_alt = param_list_create_remote(20, node, PARAM_TYPE_UINT8, 0, sizeof(uint8_t), "boot_alt", 8);
		param_list_add(boot_alt);
	}

	param_set_uint8(boot_alt, value);
	param_push_single(boot_alt, 1, node, 100);
}
