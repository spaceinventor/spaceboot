/*
 * spaceboot_slash.c
 *
 *  Created on: Mar 17, 2017
 *      Author: johan
 */

#include <stdio.h>
#include <stdlib.h>

#include <slash/slash.h>

#include <param/param.h>
#include <param/param_list.h>
#include <param/param_client.h>

#include "spaceboot_legacy.h"

#define PARAMID_BOOTALT 20

static int bootalt_slash(struct slash *slash)
{
	if (slash->argc < 3)
			return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	unsigned int value = atoi(slash->argv[2]);

	unsigned int legacy = 0;
	if (slash->argc >= 4)
		legacy = 1;

	printf("Setting bootalt@%u = %u, legacy = %u\n", node, value, legacy);

	if (legacy) {
		spaceboot_legacy_bootalt(node, value);
		return SLASH_SUCCESS;
	}

	/* Find parameter */
	param_t * boot_alt = param_list_find_id(node, 20);

	/* Define temporary parameter */
	if (boot_alt == NULL) {
		boot_alt = param_list_create_remote(20, node, PARAM_TYPE_UINT8, 0, sizeof(uint8_t), "boot_alt", 8);
		param_list_add(boot_alt);
	}

	param_set_uint8(boot_alt, value);
	param_push_single(boot_alt, 1, node, 100);

	return SLASH_SUCCESS;
}
slash_command(bootalt, bootalt_slash, "<node> <value> [legacy]", "Set boot_alt parameter (pid=20)");
