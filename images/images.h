#ifndef IMAGES_H_
#define IMAGES_H_

#include "incbin.h"

INCBIN_EXTERN(e70);

static const struct {
	char * name;
	const unsigned char * data;
	const unsigned int * len;
} images[] = {
	{"e70", ge70Data, &ge70Size},
	{}, //! Sentinel value
};

#endif
