

#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include "debug.h"

#include "disp_drv_log.h"
#include "disp_utils.h"

#include "ddp_dump.h"
#include "ddp_path.h"
#include "ddp_drv.h"
#include "ddp_ovl.h"

#include "disp_session.h"
#include "primary_display.h"

#include "m4u.h"

#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"

#include "ddp_manager.h"
#include "disp_drv_platform.h"
#include "display_recorder.h"
/* #include "ddp_mmp.h" */
#include "mtk_ovl.h"

#include "mtkfb_fence.h"

#include <asm/atomic.h>

static int ovl2mem_layer_num;
int ovl2mem_use_m4u = 1;
int ovl2mem_use_cmdq = CMDQ_ENABLE;

typedef struct {
	int state;
	unsigned int session;
	unsigned int lcm_fps;
	int max_layer;
	int need_trigger_path;
	struct mutex lock;
	cmdqRecHandle cmdq_handle_config;
	cmdqRecHandle cmdq_handle_trigger;
	disp_path_handle dpmgr_handle;
	const char *mutex_locker;
} ovl2mem_path_context;

atomic_t g_trigger_ticket = ATOMIC_INIT(1);
atomic_t g_release_ticket = ATOMIC_INIT(1);



#define pgc	_get_context()

static ovl2mem_path_context *_get_context(void)
{
	static int is_context_inited;
	static ovl2mem_path_context g_context;

	if (!is_context_inited) {
		memset((void *)&g_context, 0, sizeof(ovl2mem_path_context));
		is_context_inited = 1;
	}

	return &g_context;
}

CMDQ_SWITCH ovl2mem_cmdq_enabled(void)
{
	return ovl2mem_use_cmdq;
}

static void _ovl2mem_path_lock(const char *caller)
{
	dprec_logger_start(DPREC_LOGGER_PRIMARY_MUTEX, 0, 0);
	disp_sw_mutex_lock(&(pgc->lock));
	pgc->mutex_locker = caller;
}

static void _ovl2mem_path_unlock(const char *caller)
{
	pgc->mutex_locker = NULL;
	disp_sw_mutex_unlock(&(pgc->lock));
	dprec_logger_done(DPREC_LOGGER_PRIMARY_MUTEX, 0, 0);
}

void ovl2mem_setlayernum(int layer_num)
{
	ovl2mem_layer_num = layer_num;
	DISPERR("ovl2mem_setlayernum: %d\n", ovl2mem_layer_num);
}

int ovl2mem_get_info(void *info)
{
	/* /DISPFUNC(); */
	disp_session_info *dispif_info = (disp_session_info *) info;

	memset((void *)dispif_info, 0, sizeof(disp_session_info));

	/* FIXME,  for decouple mode, should dynamic return 4 or 8, please refer to primary_display_get_info() */
	dispif_info->maxLayerNum = ovl2mem_layer_num;
	dispif_info->displayType = DISP_IF_TYPE_DPI;
	dispif_info->displayMode = DISP_IF_MODE_VIDEO;
	dispif_info->isHwVsyncAvailable = 1;
	dispif_info->displayFormat = DISP_IF_FORMAT_RGB888;

	dispif_info->displayWidth = primary_display_get_width();
	dispif_info->displayHeight = primary_display_get_height();

	dispif_info->vsyncFPS = pgc->lcm_fps;

	if (dispif_info->displayWidth * dispif_info->displayHeight <= 240 * 432)
		dispif_info->physicalHeight = dispif_info->physicalWidth = 0;
	else if (dispif_info->displayWidth * dispif_info->displayHeight <= 320 * 480)
		dispif_info->physicalHeight = dispif_info->physicalWidth = 0;
	else if (dispif_info->displayWidth * dispif_info->displayHeight <= 480 * 854)
		dispif_info->physicalHeight = dispif_info->physicalWidth = 0;
	else
		dispif_info->physicalHeight = dispif_info->physicalWidth = 0;

	dispif_info->isConnected = 1;

	return 0;
}


static int _convert_disp_input_to_ovl(OVL_CONFIG_STRUCT *dst, primary_disp_input_config *src)
{
	if (src && dst) {
		dst->layer = src->layer;
		dst->layer_en = src->layer_en;
		dst->source = src->buff_source;
		dst->fmt = src->fmt;
		dst->addr = src->addr;
		dst->vaddr = src->vaddr;
		dst->src_x = src->src_x;
		dst->src_y = src->src_y;
		dst->src_w = src->src_w;
		dst->src_h = src->src_h;
		dst->src_pitch = src->src_pitch;
		dst->dst_x = src->dst_x;
		dst->dst_y = src->dst_y;
		dst->dst_w = src->dst_w;
		dst->dst_h = src->dst_h;
		dst->keyEn = src->keyEn;
		dst->key = src->key;
		dst->aen = src->aen;
		dst->alpha = src->alpha;
		dst->sur_aen = src->sur_aen;
		dst->src_alpha = src->src_alpha;
		dst->dst_alpha = src->dst_alpha;

		dst->isDirty = src->isDirty;

		dst->buff_idx = src->buff_idx;
		dst->identity = src->identity;
		dst->connected_type = src->connected_type;
		dst->security = src->security;
		dst->yuv_range = src->yuv_range;

		return 0;
	}

	DISPERR("src(%p) or dst(%p) is null\n", src, dst);
	return -1;
}

static int ovl2mem_callback(unsigned int userdata)
{
	int fence_idx = 0;
	int layid = 0;

	DISPMSG("ovl2mem_callback(%x), current tick=%d, release tick: %d\n", pgc->session,
		get_ovl2mem_ticket(), userdata);
	for (layid = 0; layid < (HW_OVERLAY_COUNT + 1); layid++) {
		fence_idx = mtkfb_query_idx_by_ticket(pgc->session, layid, userdata);
		if (fence_idx >= 0) {
			disp_ddp_path_config *data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);
			WDMA_CONFIG_STRUCT wdma_layer;

			wdma_layer.dstAddress = 0;
			if (data_config && (layid >= HW_OVERLAY_COUNT)) {
				wdma_layer.dstAddress = mtkfb_query_buf_mva(pgc->session, layid, fence_idx);
				wdma_layer.outputFormat = data_config->wdma_config.outputFormat;
				wdma_layer.srcWidth = data_config->wdma_config.srcWidth;
				wdma_layer.srcHeight = data_config->wdma_config.srcHeight;
				wdma_layer.dstPitch = data_config->wdma_config.dstPitch;
				DISPMSG("mem_seq %x-seq+ %d\n", pgc->session,
					mtkfb_query_frm_seq_by_addr(pgc->session, layid,
								    wdma_layer.dstAddress));
				dprec_logger_frame_seq_end(pgc->session,
							   mtkfb_query_frm_seq_by_addr(pgc->session,
										       layid,
										       wdma_layer.
										       dstAddress));

				dprec_mmp_dump_wdma_layer(&wdma_layer, 1);

			}
			mtkfb_release_fence(pgc->session, layid, fence_idx);
		}
	}

	atomic_set(&g_release_ticket, userdata);

	return 0;
}

int get_ovl2mem_ticket(void)
{
	return atomic_read(&g_trigger_ticket);

}

int ovl2mem_input_config(ovl2mem_in_config *input)
{
	int ret = -1;
	int i = 0;
	disp_ddp_path_config *data_config;

	DISPFUNC();
	_ovl2mem_path_lock(__func__);

	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);
	data_config->dst_dirty = 0;
	data_config->ovl_dirty = 0;
	data_config->rdma_dirty = 0;

	if (pgc->state == 0) {
		DISPMSG("ovl2mem is already slept\n");
		_ovl2mem_path_unlock(__func__);
		return 0;
	}
	/* hope we can use only 1 input struct for input config, just set layer number */
	for (i = 0; i < HW_OVERLAY_COUNT; i++) {
		dprec_logger_start(DPREC_LOGGER_PRIMARY_CONFIG,
				   input->layer | (input->layer_en << 16), input->addr);

		if (input[i].layer_en) {
			if (input[i].vaddr)
				/* / _debug_pattern(0x00000000, input[i].vaddr, input[i].dst_w, input[i].dst_h,
						    input[i].src_pitch, 0x00000000, input[i].layer, input[i].buff_idx);
				 */
				;
			else
				/* /_debug_pattern(input[i].addr,0x00000000, input[i].dst_w, input[i].dst_h,
						   input[i].src_pitch, 0x00000000, input[i].layer, input[i].buff_idx);
				 */
				;
		}
		/* /DISPMSG("[primary], i:%d, layer:%d, layer_en:%d, dirty:%d -0x%x\n",
			    i, input[i].layer, input[i].layer_en, input[i].dirty, input[i].addr);
		 */
		if (input[i].dirty)
			ret = _convert_disp_input_to_ovl(&(data_config->ovl_config[input[i].layer]),
			(primary_disp_input_config *)&input[i]);

		data_config->ovl_dirty = 1;
		dprec_logger_done(DPREC_LOGGER_PRIMARY_CONFIG, input->src_x, input->src_y);

	}

	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_COMPLETE, HZ / 5);

	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, pgc->cmdq_handle_config);

	_ovl2mem_path_unlock(__func__);

	DISPMSG("ovl2mem_input_config done\n");
	return ret;
}

int ovl2mem_output_config(disp_mem_output_config *out)
{
	int ret = -1;
	disp_ddp_path_config *data_config;

	_ovl2mem_path_lock(__func__);
	/* all dirty should be cleared in dpmgr_path_get_last_config() */
	data_config = dpmgr_path_get_last_config(pgc->dpmgr_handle);
	data_config->dst_dirty = 1;
	data_config->dst_h = out->h;
	data_config->dst_w = out->w;
	data_config->ovl_dirty = 0;
	data_config->rdma_dirty = 0;
	data_config->wdma_dirty = 1;
	data_config->wdma_config.dstAddress = out->addr;
	data_config->wdma_config.srcHeight = out->h;
	data_config->wdma_config.srcWidth = out->w;
	data_config->wdma_config.clipX = out->x;
	data_config->wdma_config.clipY = out->y;
	data_config->wdma_config.clipHeight = out->h;
	data_config->wdma_config.clipWidth = out->w;
	data_config->wdma_config.outputFormat = out->fmt;
	data_config->wdma_config.dstPitch = out->pitch;
	data_config->wdma_config.useSpecifiedAlpha = 1;
	data_config->wdma_config.alpha = 0xFF;
	data_config->wdma_config.security = out->security;

	if (pgc->state == 0) {
		DISPMSG("ovl2mem is already slept\n");
		_ovl2mem_path_unlock(__func__);
		return 0;
	}


	if (dpmgr_path_is_busy(pgc->dpmgr_handle))
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_DONE, HZ / 5);

	ret = dpmgr_path_config(pgc->dpmgr_handle, data_config, pgc->cmdq_handle_config);

	pgc->need_trigger_path = 1;

	_ovl2mem_path_unlock(__func__);

	/* /DISPMSG("ovl2mem_output_config done\n"); */

	return ret;
}


int ovl2mem_trigger(int blocking, void *callback, unsigned int userdata)
{
	int ret = -1;
	int fence_idx = 0;
	int layid = 0;

	DISPFUNC();

	if (pgc->need_trigger_path == 0) {
		DISPMSG("ovl2mem_trigger do not trigger\n");
		if ((atomic_read(&g_trigger_ticket) - atomic_read(&g_release_ticket)) == 1) {
			DISPMSG("ovl2mem_trigger(%x), configue input, but does not config output!!\n", pgc->session);
			for (layid = 0; layid < (HW_OVERLAY_COUNT + 1); layid++) {
				fence_idx = mtkfb_query_idx_by_ticket(pgc->session, layid,
								      atomic_read(&g_trigger_ticket));
				if (fence_idx >= 0)
					mtkfb_release_fence(pgc->session, layid, fence_idx);
			}
		}
		return ret;
	}
	_ovl2mem_path_lock(__func__);

	dpmgr_path_start(pgc->dpmgr_handle, ovl2mem_cmdq_enabled());
	dpmgr_path_trigger(pgc->dpmgr_handle, pgc->cmdq_handle_config, ovl2mem_cmdq_enabled());

	cmdqRecWait(pgc->cmdq_handle_config, CMDQ_EVENT_DISP_WDMA1_EOF);

	dpmgr_path_stop(pgc->dpmgr_handle, ovl2mem_cmdq_enabled());

	/* /cmdqRecDumpCommand(pgc->cmdq_handle_config); */

	cmdqRecFlushAsyncCallback(pgc->cmdq_handle_config, (CmdqAsyncFlushCB)ovl2mem_callback,
				  atomic_read(&g_trigger_ticket));

	cmdqRecReset(pgc->cmdq_handle_config);

	pgc->need_trigger_path = 0;
	atomic_add(1, &g_trigger_ticket);

	_ovl2mem_path_unlock(__func__);

	dprec_logger_frame_seq_begin(pgc->session, mtkfb_query_frm_seq_by_addr(pgc->session, 0, 0));
	DISPMSG("ovl2mem_trigger ovl2mem_seq %d-seq %d\n", get_ovl2mem_ticket(),
		mtkfb_query_frm_seq_by_addr(pgc->session, 0, 0));

	return ret;
}

void ovl2mem_wait_done(void)
{
	int loop_cnt = 0;

	if ((atomic_read(&g_trigger_ticket) - atomic_read(&g_release_ticket)) <= 1)
		return;

	DISPFUNC();

	while ((atomic_read(&g_trigger_ticket) - atomic_read(&g_release_ticket)) > 1) {
		dpmgr_wait_event_timeout(pgc->dpmgr_handle, DISP_PATH_EVENT_FRAME_COMPLETE,
					 HZ / 30);

		if (loop_cnt > 5)
			break;

		loop_cnt++;
	}

	DISPMSG("ovl2mem_wait_done loop %d, trigger tick:%d, release tick:%d\n", loop_cnt,
		atomic_read(&g_trigger_ticket), atomic_read(&g_release_ticket));
}

int ovl2mem_deinit(void)
{
	int ret = -1;

	DISPFUNC();

	_ovl2mem_path_lock(__func__);

	if (pgc->state == 0)
		goto Exit;

	ovl2mem_wait_done();

	dpmgr_path_stop(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_reset(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_path_deinit(pgc->dpmgr_handle, CMDQ_DISABLE);
	dpmgr_destroy_path(pgc->dpmgr_handle, NULL);

	cmdqRecDestroy(pgc->cmdq_handle_config);

	pgc->dpmgr_handle = NULL;
	pgc->cmdq_handle_config = NULL;
	pgc->state = 0;
	pgc->need_trigger_path = 0;
	atomic_set(&g_trigger_ticket, 1);
	atomic_set(&g_release_ticket, 1);
	ovl2mem_layer_num = 0;

Exit:
	_ovl2mem_path_unlock(__func__);

	DISPMSG("ovl2mem_deinit done\n");
	return ret;
}
