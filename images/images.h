#ifndef IMAGES_H_
#define IMAGES_H_

#include "incbin.h"

INCBIN_EXTERN(e70);
INCBIN_EXTERN(dise);
INCBIN_EXTERN(mppt);
INCBIN_EXTERN(pdu);

static const struct {
	char * name;
	const unsigned char * data;
	const unsigned int * len;
} images[] = {
	{"e70", ge70Data, &ge70Size},
	{"dise", gdiseData, &gdiseSize},
	{"mppt", gmpptData, &gmpptSize},
	{"pdu", gpduData, &gpduSize},
	{}, //! Sentinel value
};

#endif
