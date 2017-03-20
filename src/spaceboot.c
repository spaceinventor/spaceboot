#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <slash/slash.h>

#include <param/param.h>

#include <vmem/vmem_server.h>
#include <vmem/vmem_ram.h>

#include <csp/csp.h>
#include <csp/arch/csp_thread.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart.h>
#include <csp/drivers/can_socketcan.h>

#define SLASH_PROMPT_GOOD		"\033[96mspaceboot \033[90m%\033[0m "
#define SLASH_PROMPT_BAD		"\033[96mspaceboot \033[31m!\033[0m "
#define SLASH_LINE_SIZE			128
#define SLASH_HISTORY_SIZE		2048

void usage(void)
{
	printf("usage: spaceboot [command]\n");
	printf("\n");
	printf("CAN Bootloader\n");
	printf("Copyright (c) 2017 Space Inventor <info@satlab.com>\n");
	printf("\n");
	printf("Options:\n");
	printf(" -i INTERFACE,\tUse INTERFACE as CAN interface\n");
	printf(" -n NODE\tUse NODE as own CSP address\n");
}

int configure_csp(uint8_t addr, char *ifc)
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

int main(int argc, char **argv)
{
	struct slash *slash;
	int remain, index, i, c, p = 0;
	char *ex;

	uint8_t addr = 31;
	char *ifc = "can0";

	while ((c = getopt(argc, argv, "+hr:i:n:")) != -1) {
		switch (c) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'i':
			ifc = optarg;
			break;
		case 'n':
			addr = atoi(optarg);
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	remain = argc - optind;
	index = optind;

	if (configure_csp(addr, ifc) < 0) {
		fprintf(stderr, "Failed to init CSP\n");
		exit(EXIT_FAILURE);
	}

	slash = slash_create(SLASH_LINE_SIZE, SLASH_HISTORY_SIZE);
	if (!slash) {
		fprintf(stderr, "Failed to init slash\n");
		exit(EXIT_FAILURE);
	}

	/* Interactive or one-shot mode */
	if (remain > 0) {
		ex = malloc(SLASH_LINE_SIZE);
		if (!ex) {
			fprintf(stderr, "Failed to allocate command memory");
			exit(EXIT_FAILURE);
		}

		/* Build command string */
		for (i = 0; i < remain; i++) {
			if (i > 0)
				p = ex - strncat(ex, " ", SLASH_LINE_SIZE - p);
			p = ex - strncat(ex, argv[index + i], SLASH_LINE_SIZE - p);
		}
		slash_execute(slash, ex);
		free(ex);
	} else {
		printf("SpaceBoot - CAN Bootloader\n");
		printf("Copyright (c) 2017 Space Inventor ApS <info@satlab.com>\n\n");

		slash_loop(slash, SLASH_PROMPT_GOOD, SLASH_PROMPT_BAD);
	}

	slash_destroy(slash);

	return 0;
}
