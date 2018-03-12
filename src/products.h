/*
 * products.h
 *
 *  Created on: Mar 24, 2017
 *      Author: johan
 */

#ifndef SRC_PRODUCTS_H_
#define SRC_PRODUCTS_H_

unsigned int addrs_e70[4] = { 0x404000, 0x480000, 0x500000, 0x580000 };
unsigned int addrs_c21[2] = { 0x004000, 0x022000 };

static const struct {
	char * name;
	char * image;
	unsigned int slots;
	unsigned int * addrs;
} products[] = {
	{"e70", "e70", 4, addrs_e70},
	{"c21", "c21", 2, addrs_c21},
};

#endif /* SRC_PRODUCTS_H_ */
