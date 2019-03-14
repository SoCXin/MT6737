#define LOG_TAG "WDMA"
#include "ddp_log.h"
#include <linux/delay.h>
#include "ddp_reg.h"
#include "ddp_matrix_para.h"
#include "ddp_info.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "primary_display.h"
#include "m4u.h"

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))

unsigned int wdma_index(DISP_MODULE_ENUM module)
{
	int idx = 0;

	switch (module) {
	case DISP_MODULE_WDMA0:
		idx = 0;
		break;
	case DISP_MODULE_WDMA1:
		idx = 1;
		break;
	default:
		DDPERR("[DDP] error: invalid wdma module=%d\n", module);	/* invalid module */
		ASSERT(0);
	}
	return idx;
}

int wdma_stop(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTEN, 0x00);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_EN, 0x00);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTSTA, 0x00);

	return 0;
}

int wdma_reset(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int delay_cnt = 0;
	unsigned int idx = wdma_index(module);

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_RST, 0x01);	/* trigger soft reset */
	if (!handle) {
		while ((DISP_REG_GET(idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_FLOW_CTRL_DBG) &
			0x1) == 0) {
			delay_cnt++;
			udelay(10);
			if (delay_cnt > 2000) {
				DDPERR("wdma%d reset timeout!\n", idx);
				break;
			}
		}
	} else {
		/* add comdq polling */
	}
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_RST, 0x0);	/* trigger soft reset */

	return 0;
}

unsigned int ddp_wdma_get_cur_addr(void)
{
	return INREG32(DISP_REG_WDMA_DST_ADDR0);
}
