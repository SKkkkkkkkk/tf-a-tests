#include <arch_helpers.h>
#include <drivers/console.h>
#define xmodem_putchar(x) 	console_putc(x)
#define xmodem_getchar() 	console_getc()
#define xmodem_tstc() 		console_tstc()

int _inbyte(uint64_t timeout) // usec timeout
{
	uint64_t start_count_val = syscounter_read();
	uint64_t wait_cycles = (timeout * read_cntfrq_el0()) / 1000000;
	while ((syscounter_read() - start_count_val) < wait_cycles) {
		if (xmodem_tstc())
			return xmodem_getchar();
	}
	return -1;
}

void _outbyte(int c)
{
	xmodem_putchar(c);
}
