/*
 * spaceboot_slash.c
 *
 *  Created on: Mar 17, 2017
 *      Author: johan
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <slash/slash.h>

#include <vmem/vmem.h>
#include <vmem/vmem_client.h>

#include <csp/csp.h>
#include <csp/csp_cmp.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_thread.h>

#include "spaceboot_bootalt.h"

#define PARAMID_BOOTALT 20

static int bootalt_slash(struct slash *slash)
{
	if (slash->argc < 3)
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	unsigned int value = atoi(slash->argv[2]);

	printf("Setting bootalt@%u = %u\n", node, value);
	spaceboot_bootalt(node, value);

	return SLASH_SUCCESS;
}
slash_command(bootalt, bootalt_slash, "<node> <value>", "Set boot_alt parameter");

static int bootload_slash(struct slash *slash)
{

	if (slash->argc < 2)
		return SLASH_EUSAGE;
	unsigned int node = atoi(slash->argv[1]);

	struct csp_cmp_message message = {};
	if (csp_cmp_ident(node, 100, &message) != CSP_ERR_NONE) {
		printf("Cannot ping system\n");
		return SLASH_EINVAL;
	}
	printf("%s\n%s\n%s\n%s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

	printf("Switching to flash 0\n");
	spaceboot_bootalt(node, 0);
	csp_reboot(node);
	csp_sleep_ms(1000);
	printf("Waiting for reboot\n");

	if (csp_cmp_ident(node, 100, &message) != CSP_ERR_NONE) {
		printf("Cannot ping system\n");
		return SLASH_EINVAL;
	}
	printf("%s\n%s\n%s\n%s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

	printf("Should run from flash 0 now\n");
	csp_sleep_ms(1000);

	char * file = "images/bootloader-e70.bin";
	unsigned int address = 0x480000;
	unsigned int timeout = 10000;

	printf("Upload from %s to node %u addr 0x%x\n", file, node, address);

	/* Open file */
	FILE * fd = fopen(file, "r");
	if (fd == NULL)
		return SLASH_EINVAL;

	/* Read size */
	struct stat file_stat;
	fstat(fd->_fileno, &file_stat);

	/* Copy to memory */
	char * data = malloc(file_stat.st_size);
	int size = fread(data, 1, file_stat.st_size, fd);
	fclose(fd);

	//csp_hex_dump("file", data, size);

	printf("Size %u\n", size);

	vmem_upload(node, timeout, address, data, size);

	char * datain = malloc(file_stat.st_size);
	uint32_t time_begin = csp_get_ms();
	vmem_download(node, timeout, address, size, datain);
	uint32_t time_total = csp_get_ms() - time_begin;

	printf("Downloaded %u bytes in %.03f s at %u Bps\n", size, time_total / 1000.0, (unsigned int) (size / ((float)time_total / 1000.0)) );

	for (int i = 0; i < size; i++) {
		if (datain[i] == data[i])
			continue;
		printf("Diff at %x: %hhx != %hhx\n", 0x480000 + i, data[i], datain[i]);

		free(data);
		free(datain);
		return SLASH_EINVAL;
	}

	free(data);
	free(datain);

	spaceboot_bootalt(node, 1);
	csp_reboot(node);
	csp_sleep_ms(1000);

	if (csp_cmp_ident(node, 100, &message) != CSP_ERR_NONE) {
		printf("Cannot ping system\n");
		return SLASH_EINVAL;
	}
	printf("%s\n%s\n%s\n%s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

	return SLASH_SUCCESS;
}
slash_command(bootload, bootload_slash, "<node> <file>", "Upload bootloader to FLASH1");
