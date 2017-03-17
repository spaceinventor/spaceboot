/*
 * spaceboot_legacy.c
 *
 *  Created on: Mar 17, 2017
 *      Author: johan
 */

#include <stdint.h>
#include <stdio.h>
#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <param/param.h>

#include "spaceboot_legacy.h"

void spaceboot_legacy_bootalt(int node, int value) {

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

}
