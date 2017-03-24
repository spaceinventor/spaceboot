#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <param/param.h>
#include <param/param_list.h>

#include <vmem/vmem.h>
#include <vmem/vmem_client.h>
#include <vmem/vmem_server.h>
#include <vmem/vmem_ram.h>


#include <csp/csp.h>
#include <csp/csp_cmp.h>
#include <csp/arch/csp_time.h>
#include <csp/arch/csp_thread.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>

#include "../images/images.h"
#include "products.h"
#include "spaceboot_bootalt.h"

static void usage(void)
{
	printf("Usage: spaceboot [OPTIONS] [COMMANDS] <TARGET>\n");
	printf("\n");
	printf("CAN Bootloader\n");
	printf("Copyright (c) 2017 Space Inventor <info@satlab.com>\n");
	printf("\n");
	printf(" Options:\n\n");
	printf("  -i INTERFACE\t\tUse INTERFACE as CAN interface\n");
	printf("  -n NODE\t\tUse NODE as own CSP address\n");
	printf("  -h \t\t\tShow help\n");
	printf("  -l \t\t\tList embedded images\n");
	printf("  -p PRODUCT\t\t[e70, pdu, mppt, dise]");
	printf("\n");
	printf(" Commands (executed in order):\n\n");
	printf("  -r \t\t\tReset to flash0\n");
	printf("  -b [filename]\t\tUpload bootloader to flash slot 1\n");
	printf("  -f <filename>\t\tUpload file to flash slot 0\n");
	printf("\n");
	printf(" Arguments:\n\n");
	printf(" <target>\t\tCSP node to upload to\n");
	printf("\n");
}

static void print_images(void) {

	printf("\n");
	printf("CAN Bootloader\n");
	printf("Copyright (c) 2017 Space Inventor <info@satlab.com>\n");
	printf("\n");
	printf(" Images:\n");
	for (int i = 0; images[i].name != NULL; i++) {
		printf("  %s\n", images[i].name);
	}
	printf("\n");

}

static int configure_csp(uint8_t addr, char *ifc)
{

	if (csp_buffer_init(100, 320) < 0)
		return -1;

	csp_set_hostname("satctl");
	csp_set_model("linux");

	//csp_debug_set_level(4, 1);
	//csp_debug_set_level(5, 1);

	if (csp_init(addr) < 0)
		return -1;

	csp_iface_t *can0 = csp_can_socketcan_init(ifc, 1000000, 0);

	if (csp_route_set(CSP_DEFAULT_ROUTE, can0, CSP_NODE_MAC) < 0)
		return -1;

	if (csp_route_start_task(0, 0) < 0)
		return -1;


	csp_rdp_set_opt(2, 10000, 2000, 1, 1000, 2);

	return 0;
}

static void ping(int node) {

	struct csp_cmp_message message = {};
	if (csp_cmp_ident(node, 100, &message) != CSP_ERR_NONE) {
		printf("Cannot ping system\n");
		exit(EXIT_FAILURE);
	}
	printf("  | %s\n  | %s\n  | %s\n  | %s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

}

static void reset_to_flash(int node, int flash) {
	printf("  Switching to flash %u\n", flash);
	if (flash)
		spaceboot_bootalt(node, 1);
	else
		spaceboot_bootalt(node, 0);

	printf("  Rebooting");
	csp_reboot(node);
	int step = 100;
	int ms = 1000;
	while(ms > 0) {
		printf(".");
		fflush(stdout);
		csp_sleep_ms(step);
		ms -= step;
	}
	printf("\n");

	ping(node);

}

static void image_get(char * filename, char ** data, int * len) {

	/* Check for embedded image */
	for (int i = 0; images[i].name != NULL; i++) {
		if (strcmp(filename, images[i].name) == 0) {
			*data = (char *) images[i].data;
			*len = *images[i].len;
			printf("  Using embedded image: %s\n", filename);
			return;
		}
	}

	/* Open file */
	FILE * fd = fopen(filename, "r");
	if (fd == NULL) {
		printf("  Cannot find file: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	/* Read size */
	struct stat file_stat;
	fstat(fd->_fileno, &file_stat);

	printf("  Open %s size %zu\n", filename, file_stat.st_size);

	/* Copy to memory:
	 * Note we ignore the memory leak because the application will terminate immediately after using the data */
	*data = malloc(file_stat.st_size);
	*len = fread(*data, 1, file_stat.st_size, fd);
	fclose(fd);

}

static void upload_and_verify(int node, int address, char * data, int len) {

	unsigned int timeout = 10000;

	printf("  Upload %u bytes to node %u addr 0x%x\n", len, node, address);

	vmem_upload(node, timeout, address, data, len);

	char * datain = malloc(len);

	vmem_download(node, timeout, address, len, datain);

	for (int i = 0; i < len; i++) {
		if (datain[i] == data[i])
			continue;
		printf("Diff at %x: %hhx != %hhx\n", 0x480000 + i, data[i], datain[i]);
		exit(EXIT_FAILURE);
	}

	free(datain);
}

int main(int argc, char **argv)
{
	/* Parsed values */
	uint8_t addr = 31;
	char *ifc = "can0";
	int reset = 0;
	char file[100] = {};
	char bootimg[100] = {};
	int productid = 0;

	/* Run parser */
	int remain, index, c;
	while ((c = getopt(argc, argv, "+hri:n:f:b::lp:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'l':
			print_images();
			exit(EXIT_SUCCESS);
		case 'i':
			ifc = optarg;
			break;
		case 'n':
			addr = atoi(optarg);
			break;
		case 'r':
			reset = 1;
			break;
		case 'f':
			strncpy(file, optarg, 100);
			break;
		case 'b':
			if (optarg)
				strncpy(bootimg, optarg, 100);
			else
				strcat(bootimg, products[productid].image);
			break;
		case 'p':
			if (strcmp(optarg, "e70") == 0) {
				productid = 0;
			} else if (strcmp(optarg, "pdu") == 0) {
				productid = 1;
			} else if (strcmp(optarg, "mppt") == 0) {
				productid = 2;
			} else if (strcmp(optarg, "dise") == 0) {
				productid = 3;
			} else {
				printf("Invalid product selected, choose either [e70, pdu, mppt, dise]\n");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}
	remain = argc - optind;
	index = optind;

	if (remain != 1) {
		usage();
		exit(EXIT_FAILURE);
	}

	/* Parse remaining options */
	uint8_t node = atoi(argv[index]);

	if (configure_csp(addr, ifc) < 0) {
		fprintf(stderr, "Failed to init CSP\n");
		exit(EXIT_FAILURE);
	}

	/**
	 * STEP 0: Contact system
	 */
	printf("\nPING\n");
	printf("----\n");
	ping(node);

	/**
	 * STEP 1: Reset system
	 */
	if (reset) {
		reset_to_flash(node, 0);
	}

	/**
	 * STEP 2: Upload to flash1
	 */
	if (strlen(bootimg)) {
		printf("\nBOOTLOADER\n");
		printf("----------\n");
		char * data;
		int len;
		image_get(bootimg, &data, &len);
		upload_and_verify(node, products[productid].flash1_addr, data, len);
		reset_to_flash(node, 1);
	}

	/**
	 * STEP 3: Upload to flash0
	 */
	if (strlen(file)) {
		printf("\nFLASHING\n");
		printf("----------\n");
		char * data;
		int len;
		image_get(file, &data, &len);
		upload_and_verify(node, products[productid].flash0_addr, data, len);
		reset_to_flash(node, 0);
	}

	return 0;
}
