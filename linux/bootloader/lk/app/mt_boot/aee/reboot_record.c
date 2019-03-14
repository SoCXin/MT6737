#include <string.h>
#include <arch/ops.h>
#include <dev/mrdump.h>
#include <debug.h>
#include "aee.h"
#include "kdump.h"

#define SEARCH_SIZE 16777216
#define SEARCH_STEP 1024

static struct mrdump_control_block *mrdump_cb_addr(void)
{
	int i;
	for (i = 0; i < SEARCH_SIZE; i += SEARCH_STEP) {
		struct mrdump_control_block *bufp = (struct mrdump_control_block *)(DRAM_PHY_ADDR + i);
		if (memcmp(bufp->sig, MRDUMP_GO_DUMP, 8) == 0) {
			return bufp;
		}
	}
	return NULL;
}

struct mrdump_control_block *aee_mrdump_get_params(void)
{
	struct mrdump_control_block *bufp = mrdump_cb_addr();
	if (bufp == NULL) {
		voprintf_debug("mrdump_cb is NULL\n");
		return NULL;
	}
	if (memcmp(bufp->sig, MRDUMP_GO_DUMP, 8) == 0) {
		bufp->sig[0] = 'X';
		aee_mrdump_flush_cblock(bufp);
		voprintf_debug("Boot record found at %p[%02x%02x]\n", bufp, bufp->sig[0], bufp->sig[1]);
		return bufp;
	} else {
		voprintf_debug("No Boot record found\n");
		return NULL;
	}
}

void aee_mrdump_flush_cblock(struct mrdump_control_block *bufp)
{
	if (bufp != NULL) {
		arch_clean_cache_range((addr_t)bufp, sizeof(struct mrdump_control_block));
	}
}
