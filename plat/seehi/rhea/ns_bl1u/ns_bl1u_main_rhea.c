#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <mmio.h>
#include <xmodem.h>

#define R_BUF_ADDR 0xa0000000U
#define R_BUF_SIZE (100*1024*1024)
extern const char build_message[];
extern const char version_string[];
void ns_bl1u_main(void)
{
	NOTICE("NS_BL1U: %s\n", version_string);
	NOTICE("NS_BL1U: %s\n", build_message);

	int st;

	printf ("Send data using the xmodem protocol from your terminal emulator now...\n");
	memset((void *)R_BUF_ADDR, 0, R_BUF_SIZE);
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   65536 is the maximum size to be written at this address
	 */
	st = xmodemReceive((void *)R_BUF_ADDR, R_BUF_SIZE);
	if (st < 0) {
		printf ("Xmodem receive error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully received %d bytes\n", st);
	}
	return;
}