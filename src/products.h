/*
 * products.h
 *
 *  Created on: Mar 24, 2017
 *      Author: johan
 */

#ifndef SRC_PRODUCTS_H_
#define SRC_PRODUCTS_H_

static const struct {
	char * name;
	char * image;
	unsigned int flash0_addr;
	unsigned int flash1_addr;
} products[] = {
	{"e70", "e70", 0x400000, 0x480000},
	{"pdu", "pdu", 0x0, 0x20000},
	{"mppt", "mppt", 0x0, 0x20000},
	{"dise", "dise", 0x0, 0x20000},
};

#endif /* SRC_PRODUCTS_H_ */
