#include <arch_helpers.h>
#include <assert.h>
#include <debug.h>
#include <mmio.h>

extern const char build_message[];
extern const char version_string[];
void ns_bl1u_main(void)
{
	NOTICE("NS_BL1U: %s\n", version_string);
	NOTICE("NS_BL1U: %s\n", build_message);
	return;
}