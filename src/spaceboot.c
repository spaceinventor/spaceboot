#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <endian.h>

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
#include <csp/interfaces/csp_if_can.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/interfaces/csp_if_zmqhub.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>

#include <pthread.h>

param_t * boot_img[4];

/* Parsed values */
uint16_t addr = 1;
char *can_dev = "can0";
int verify = true;
char *uart_dev = "/dev/ttyUSB0";
uint32_t uart_baud = 1000000;
int use_uart = 0;
int use_can = 1;
int use_slash = 0;
int csp_version = 2;
int type = 0;
char * csp_zmqhub_addr = NULL;

VMEM_DEFINE_STATIC_RAM(test, "test", 10000);


static void usage(void)
{
	printf("Usage: spaceboot [OPTIONS] <TARGET> [COMMANDS]\n");
	printf("\n");
	printf("CAN Bootloader\n");
	printf("Copyright (c) 2016-2022 Space Inventor <info@space-inventor.com>\n");
	printf("\n");
	printf(" [OPTIONS]:\n\n");
	printf("  -c INTERFACE,\tUse INTERFACE as CAN interface\n");
	printf("  -u INTERFACE,\tUse INTERFACE as UART interface\n");
	printf("  -b BAUD,\tUART buad rate\n");
	printf("  -n NODE\t\tUse NODE as own CSP address\n");
	printf("  -h \t\t\tShow help\n");
	printf("  -s \t\t\tSearch for images with slash (build-slash)\n");
	printf("  -p PRODUCT\t\t[e70, c21]\n");
	printf("  -w \t\t\tDo not verify image uploads\n");
	printf("  -v CSP version [1, 2]\n");
	printf("\n");
	printf(" <TARGET>\t\tCSP node to program\n");
	printf("\n");
	printf(" [COMMANDS]: (executed in order)\n\n");
	printf("  -r <slot>,[count]\tReboot into flash slot [count] times\n");
	printf("  -f <slot>,[filename]\tUpload file\n");
	printf("\n\n");
}

static void ping(int node) {

	struct csp_cmp_message message = {};
	if (csp_cmp_ident(node, 1000, &message) != CSP_ERR_NONE) {
		printf("Cannot ping system\n");
		exit(EXIT_FAILURE);
	}
	printf("  | %s\n  | %s\n  | %s\n  | %s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

}

static vmem_list_t vmem_list_find(int node, int timeout, char * name, int namelen) {
	vmem_list_t ret = {};

	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, VMEM_PORT_SERVER, timeout, CSP_O_NONE);
	if (conn == NULL)
		return ret;

	csp_packet_t * packet = csp_buffer_get(sizeof(vmem_request_t));
	vmem_request_t * request = (void *) packet->data;
	request->version = VMEM_VERSION;
	request->type = VMEM_SERVER_LIST;
	packet->length = sizeof(vmem_request_t);

	csp_send(conn, packet);

	/* Wait for response */
	packet = csp_read(conn, timeout);
	if (packet == NULL) {
		printf("No response\n");
		csp_close(conn);
		return ret;
	}

	for (vmem_list_t * vmem = (void *) packet->data; (intptr_t) vmem < (intptr_t) packet->data + packet->length; vmem++) {
		//printf(" %u: %-5.5s 0x%08X - %u typ %u\r\n", vmem->vmem_id, vmem->name, (unsigned int) csp_ntoh32(vmem->vaddr), (unsigned int) csp_ntoh32(vmem->size), vmem->type);
		if (strncmp(vmem->name, name, namelen) == 0) {
			ret.vmem_id = vmem->vmem_id;
			ret.type = vmem->type;
			memcpy(ret.name, vmem->name, 5);
			ret.vaddr = be32toh(vmem->vaddr);
			ret.size = be32toh(vmem->size);
		}
	}

	csp_buffer_free(packet);
	csp_close(conn);

	return ret;
}

static void reset_to_flash(int node, int flash, int times) {

	printf("  Switching to flash %d\n", flash);
	printf("  Will run this image %d times\n", times);

	char queue_buf[50];
	param_queue_t queue;
	param_queue_init(&queue, queue_buf, 50, 0, PARAM_QUEUE_TYPE_SET, csp_version);

	uint8_t zero = 0;
	param_queue_add(&queue, boot_img[0], 0, &zero);
	param_queue_add(&queue, boot_img[1], 0, &zero);
	if (type == 1) {
		param_queue_add(&queue, boot_img[2], 0, &zero);
		param_queue_add(&queue, boot_img[3], 0, &zero);
	}
	param_queue_add(&queue, boot_img[flash], 0, &times);
	param_push_queue(&queue, 1, node, 1000);

	printf("  Rebooting");
	csp_reboot(node);
	int step = 25;
	int ms = 1000;
	while(ms > 0) {
		printf(".");
		fflush(stdout);
		usleep(step * 1000);
		ms -= step;
	}
	printf("\n");

	if (use_uart) {
		int step = 25;
		int ms = 250;
		while(ms > 0) {
			printf(".");
			fflush(stdout);
			usleep(step * 1000);
			ms -= step;
		}
		printf("\n");
	}

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

static pthread_t router_handle;
void * router_task(void * param) {
	while(1) {
		csp_route_work();
	}
}

int main(int argc, char **argv)
{
	/* Parse Options */
	int c;
	while ((c = getopt(argc, argv, "+hlwsb:c:u:t:n:p:v:z:")) != -1) {

		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'w':
			verify = false;
			break;
		case 's':
			use_slash = 1;
			break;
		case 'c':
			use_uart = 0;
			use_can = 1;
			can_dev = optarg;
			break;
		case 'u':
			use_uart = 1;
			use_can = 0;
			uart_dev = optarg;
			break;
		case 't':
			type = atoi(optarg);
		case 'b':
			uart_baud = atoi(optarg);
			break;
		case 'n':
			addr = atoi(optarg);
			break;
        case 'v':
            csp_version = atoi(optarg);
            break;
        case 'z':
            csp_zmqhub_addr = optarg;
            use_can = 0;
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
	uint16_t node = atoi(argv[optind]);
	optind++;

	/* Setup remote parameters */
	boot_img[0] = param_list_create_remote(21, node, PARAM_TYPE_UINT8, PM_CONF, 0, "boot_img0", 10);
	boot_img[1] = param_list_create_remote(20, node, PARAM_TYPE_UINT8, PM_CONF, 0, "boot_img1", 10);
	boot_img[2] = param_list_create_remote(22, node, PARAM_TYPE_UINT8, PM_CONF, 0, "boot_img2", 10);
	boot_img[3] = param_list_create_remote(23, node, PARAM_TYPE_UINT8, PM_CONF, 0, "boot_img3", 10);

	param_list_add(boot_img[0]);
	param_list_add(boot_img[1]);
	param_list_add(boot_img[2]);
	param_list_add(boot_img[3]);

	csp_conf.address = addr;
	csp_conf.version = csp_version;
	csp_conf.hostname = "spaceboot";
	csp_conf.model = "linux";
	csp_init();

	//extern uint8_t csp_dbg_packet_print;
	//csp_dbg_packet_print = 1;
	//extern uint8_t csp_dbg_rdp_print;
	//csp_dbg_rdp_print = 1;

    csp_iface_t * default_iface = NULL;
    if (use_uart) {
        csp_usart_conf_t conf = {
            .device = uart_dev,
            .baudrate = uart_baud, /* supported on all platforms */
            .databits = 8,
            .stopbits = 1,
            .paritysetting = 0,
            .checkparity = 0
        };
        int error = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME, &default_iface);
        if (error != CSP_ERR_NONE) {
            printf("failed to add KISS interface [%s], error: %d", uart_dev, error);
            exit(1);
        }

        printf("Using usart %s baud %u\n", uart_dev, uart_baud);
    }

    if (use_can) {
        int error = csp_can_socketcan_open_and_add_interface(can_dev, CSP_IF_CAN_DEFAULT_NAME, 1000000, true, &default_iface);
        if (error != CSP_ERR_NONE) {
            printf("failed to add CAN interface [%s], error: %d", can_dev, error);
        }
        printf("Using can %s baud %u\n", can_dev, 1000000);
		default_iface->name = "CAN";
		default_iface->addr = addr;
		default_iface->netmask = 8;
    }

    if (csp_zmqhub_addr) {
        printf("zmq str %s\n", csp_zmqhub_addr);
        csp_iface_t * zmq_if;
        csp_zmqhub_init(csp_get_address(), csp_zmqhub_addr, 0, &zmq_if);
        zmq_if->name = "ZMQ";
		zmq_if->addr = addr;
		zmq_if->netmask = 8;

        sleep(1);
    }

	csp_iflist_print();

	pthread_create(&router_handle, NULL, &router_task, NULL);

	csp_rdp_set_opt(3, 10000, 500, 1, 2000, 2);
	//csp_rdp_set_opt(6, 10000, 1000, 1, 1000, 5);

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

			char path[101];
			char *subarg = strtok(optarg, ",");
			int slot = atoi(subarg);
			subarg = strtok(NULL, ",");
			if (subarg != NULL) {
				strncpy(path, subarg, 100);
			} else {
				if (use_slash) {
					snprintf(path, 100, "build-slash-%u/com-%u.bin", slot, slot);
				} else {
					snprintf(path, 100, "build-noslash-%u/com-%u.bin", slot, slot);
				}
				if (access(path, F_OK) != 0) {
					printf("Failed to find: %s\n", path);
					exit(EXIT_FAILURE);
				} else {
					printf("  Found software image: %s\n", path);
				}
			}


			char * data;
			int len;
			image_get(path, &data, &len);

			char vmem_name[5];
			snprintf(vmem_name, 5, "fl%u", slot);

			printf("  Requesting VMEM name: %s...\n", vmem_name);

			vmem_list_t vmem = vmem_list_find(node, 5000, vmem_name, strlen(vmem_name));
			if (vmem.size == 0) {
				printf("Failed to find vmem on subsystem\n");
				exit(EXIT_FAILURE);
			} else {
				printf("  Found vmem\n");
				printf("    Base address: 0x%x\n", vmem.vaddr);
				printf("    Size: %u\n", vmem.size);
			}

			if (len > vmem.size) {
				printf("Software image too large for vmem\n");
				exit(EXIT_FAILURE);
			}

			if (verify)
				upload_and_verify(node, vmem.vaddr, data, len);
			else
				upload(node, vmem.vaddr, data, len);

			break;
		}

		default:
			exit(EXIT_FAILURE);
			break;
		}
	}

	return 0;
}
