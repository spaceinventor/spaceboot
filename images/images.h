#ifndef IMAGES_H_
#define IMAGES_H_

#include "incbin.h"

INCBIN_EXTERN(e70_2);

static const struct {
	char * name;
	const unsigned char * data;
	const unsigned int * len;
} images[] = {
	{"e70_2", ge70_2Data, &ge70_2Size},
	{}, //! Sentinel value
};

#endif
