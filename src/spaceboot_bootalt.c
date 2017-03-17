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


void spaceboot_bootalt(int node, int value, int legacy)
{
	if (legacy) {

		/* Pack legacy parameter data */
		uint8_t out[3];
		uint16_t id = csp_hton16(20);
		memcpy(&out[0], &id, 2);
		uint8_t _value = (uint8_t) value;
		memcpy(&out[2], &_value, 1);

		/* Expect OK in response */
		uint8_t in[2];

		if (csp_transaction(CSP_PRIO_HIGH, node, 11, 1000, out, 3, in, 2) != 2) {
			printf("legacy bootalt failed\n");
			return;
		} else {
			printf("legacy response: %2s\n", in);
		}

	} else {

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

}
