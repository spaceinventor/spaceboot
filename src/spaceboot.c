#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <param/param.h>
#include <param/param_list.h>
#include <param/param_client.h>

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

#include "products.h"

param_t * boot_img[4];
static int productid = 1;

static void usage(void)
{
	printf("Usage: spaceboot [OPTIONS] <TARGET> [COMMANDS]\n");
	printf("\n");
	printf("CAN Bootloader\n");
	printf("Copyright (c) 2017 Space Inventor <info@satlab.com>\n");
	printf("\n");
	printf(" [OPTIONS]:\n\n");
	printf("  -i INTERFACE\t\tUse INTERFACE as CAN interface\n");
	printf("  -n NODE\t\tUse NODE as own CSP address\n");
	printf("  -h \t\t\tShow help\n");
	printf("  -p PRODUCT\t\t[e70, c21]\n");
	printf("  -w \t\t\tDo not verify image uploads\n");
	printf("\n");
	printf(" <TARGET>\t\tCSP node to program\n");
	printf("\n");
	printf(" [COMMANDS]: (executed in order)\n\n");
	printf("  -r <slot>,[count]\tReboot into flash slot [count] times\n");
	printf("  -f <slot>,<filename>\tUpload file\n");
	printf("\n\n");
}

static int configure_csp(uint8_t addr, char *ifc)
{

	if (csp_buffer_init(100, 320) < 0)
		return -1;

	csp_conf_t csp_config;
	csp_conf_get_defaults(&csp_config);
	csp_config.address = addr;
	csp_config.hostname = "spaceboot";
	csp_config.model = "linux";
	if (csp_init(&csp_config) < 0)
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
	if (csp_cmp_ident(node, 1000, &message) != CSP_ERR_NONE) {
		printf("Cannot ping system\n");
		exit(EXIT_FAILURE);
	}
	printf("  | %s\n  | %s\n  | %s\n  | %s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

}

static void reset_to_flash(int node, int flash, int times) {

	if (flash >= products[productid].slots) {
		printf("Error: invalid slot number %u\n", flash);
		exit(EXIT_FAILURE);
	}

	printf("  Switching to flash %d\n", flash);
	printf("  Will run this image %d times\n", times);

	char queue_buf[25];
	param_queue_t queue;
	param_queue_init(&queue, queue_buf, 25, 0, PARAM_QUEUE_TYPE_SET);

	uint8_t zero = 0;
	param_queue_add(&queue, boot_img[0], 0, &zero);
	param_queue_add(&queue, boot_img[1], 0, &zero);
	if (products[productid].slots > 2)
		param_queue_add(&queue, boot_img[2], 0, &zero);
	if (products[productid].slots > 3)
	param_queue_add(&queue, boot_img[3], 0, &zero);
	param_queue_add(&queue, boot_img[flash], 0, &times);
	param_push_queue(&queue, 1, node, 100);

	printf("  Rebooting");
	csp_reboot(node);
	int step = 25;
	int ms = 250;
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

static void upload(int node, int address, char * data, int len) {

	unsigned int timeout = 10000;
	printf("  Upload %u bytes to node %u addr 0x%x\n", len, node, address);
	vmem_upload(node, timeout, address, data, len);
	printf("  Waiting for flash driver to flush\n");
	usleep(100000);

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
		printf("Diff at %x: %hhx != %hhx\n", address + i, data[i], datain[i]);
		exit(EXIT_FAILURE);
	}

	free(datain);
}

int main(int argc, char **argv)
{
	/* Parsed values */
	uint8_t addr = 31;
	char *ifc = "can0";
	int verify = true;

	/* Parse Options */
	int c;
	while ((c = getopt(argc, argv, "+hlwi:n:p:")) != -1) {

		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'w':
			verify = false;
			break;
		case 'i':
			ifc = optarg;
			break;
		case 'n':
			addr = atoi(optarg);
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

	/* Check for <TARGET> */
	if (argc - optind == 0) {
		printf("Missing argument <TARGET>\n\n");
		usage();
		exit(EXIT_FAILURE);
	}

	/* Parse <TARGET> */
	uint8_t node = atoi(argv[optind]);
	optind++;

	/* Setup remote parameters */
	boot_img[0] = param_list_create_remote(21, node, PARAM_TYPE_UINT8, 0, "boot_img0", 10);
	boot_img[1] = param_list_create_remote(20, node, PARAM_TYPE_UINT8, 0, "boot_img1", 10);
	if (products[productid].slots > 2)
		boot_img[2] = param_list_create_remote(22, node, PARAM_TYPE_UINT8, 0, "boot_img2", 10);
	if (products[productid].slots > 3)
		boot_img[3] = param_list_create_remote(23, node, PARAM_TYPE_UINT8, 0, "boot_img3", 10);

	/* Setup CSP */
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

	/* Parse [COMMANDS] */
	while ((c = getopt(argc, argv, "+hr:bf:")) != -1) {
		switch(c) {

		case 'h':
			usage();
			exit(EXIT_SUCCESS);

		case 'r':

			printf("\n");
			printf("RESET\n");
			printf("-----\n");

			char *subarg = strtok(optarg, ",");
			int slot = atoi(subarg);
			subarg = strtok(NULL, ",");
			int times = 1;
			if (subarg != NULL) {
				times = atoi(subarg);
			}

			reset_to_flash(node, slot, times);
			break;

		case 'f': {

			printf("\n");
			printf("UPLOAD IMAGE\n");
			printf("----------\n");

			char *subarg = strtok(optarg, ",");
			int slot = atoi(subarg);
			subarg = strtok(NULL, ",");
			if (subarg == NULL) {
				printf("Invalid command argument:\n");
				printf("  Usage: -f <slot>,<path>\n");
				exit(EXIT_FAILURE);
			}
			char *path = subarg;

			char * data;
			int len;
			image_get(path, &data, &len);

			if (slot > products[productid].slots - 1) {
				printf("Slot out of range:\n");
				printf("  max slot number is %u\n", products[productid].slots);
				exit(EXIT_FAILURE);
			}

			if (verify)
				upload_and_verify(node, products[productid].addrs[slot], data, len);
			else
				upload(node, products[productid].addrs[slot], data, len);

			break;
		}

		default:
			exit(EXIT_FAILURE);
			break;
		}
	}

	return 0;
}
