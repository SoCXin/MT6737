/*
 *this is a CCIF modem driver for 6595.
 *
 *V0.1: Xiao Wang <xiao.wang@mediatek.com>
 */
#include <linux/list.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/netdevice.h>
#include <linux/random.h>
#include <linux/platform_device.h>
#include <mt-plat/mt_boot.h>
#include "ccci_config.h"
#include <mt-plat/mt_ccci_common.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include "ccci_core.h"
#include "ccci_bm.h"
#include "ccci_platform.h"
#include "modem_ccif.h"
#include "ccif_c2k_platform.h"
#if defined(ENABLE_32K_CLK_LESS)
#include <mt-plat/mtk_rtc.h>
#endif

#include <mach/mt_pbm.h>

#define TAG "cif"

#define BOOT_TIMER_ON 10	/*10 */
#define BOOT_TIMER_HS1 10	/*10 */

#define NET_RX_QUEUE_MASK 0x4
#define NAPI_QUEUE_MASK NET_RX_QUEUE_MASK	/*Rx, only Rx-exclusive port can enable NAPI */

#define IS_PASS_SKB(md, qno)	\
	(!md->data_usb_bypass && (md->md_state != EXCEPTION || md->ex_stage != EX_INIT_DONE) \
	 && ((1<<qno) & NET_RX_QUEUE_MASK))

#define RX_BUGDET 16

#define RINGQ_BASE (8)
#define RINGQ_SRAM (7)
#define RINGQ_EXP_BASE (0)
#define CCIF_CH_NUM 16
#define CCIF_MD_SMEM_RESERVE 0x200000	/*reserved for EE dump */
/*AP to MD*/
#define H2D_EXCEPTION_ACK        (RINGQ_EXP_BASE+1)
#define H2D_EXCEPTION_CLEARQ_ACK (RINGQ_EXP_BASE+2)
#define H2D_FORCE_MD_ASSERT      (RINGQ_EXP_BASE+3)

#define H2D_SRAM    (RINGQ_SRAM)
#define H2D_RINGQ0  (RINGQ_BASE+0)
#define H2D_RINGQ1  (RINGQ_BASE+1)
#define H2D_RINGQ2  (RINGQ_BASE+2)
#define H2D_RINGQ3  (RINGQ_BASE+3)
#define H2D_RINGQ4  (RINGQ_BASE+4)
#define H2D_RINGQ5  (RINGQ_BASE+5)
#define H2D_RINGQ6  (RINGQ_BASE+6)
#define H2D_RINGQ7  (RINGQ_BASE+7)

/*MD to AP*/
#define CCIF_HW_CH_RX_RESERVED    ((1 << (RINGQ_EXP_BASE+0)) | (1 << (RINGQ_EXP_BASE+5)))
#define D2H_EXCEPTION_INIT        (RINGQ_EXP_BASE+1)
#define D2H_EXCEPTION_INIT_DONE   (RINGQ_EXP_BASE+2)
#define D2H_EXCEPTION_CLEARQ_DONE (RINGQ_EXP_BASE+3)
#define D2H_EXCEPTION_ALLQ_RESET  (RINGQ_EXP_BASE+4)
#define AP_MD_SEQ_ERROR           (RINGQ_EXP_BASE+6)
#define D2H_SRAM    (RINGQ_SRAM)
#define D2H_RINGQ0  (RINGQ_BASE+0)
#define D2H_RINGQ1  (RINGQ_BASE+1)
#define D2H_RINGQ2  (RINGQ_BASE+2)
#define D2H_RINGQ3  (RINGQ_BASE+3)
#define D2H_RINGQ4  (RINGQ_BASE+4)
#define D2H_RINGQ5  (RINGQ_BASE+5)
#define D2H_RINGQ6  (RINGQ_BASE+6)
#define D2H_RINGQ7  (RINGQ_BASE+7)

struct c2k_port {
	enum c2k_channel ch;
	enum c2k_channel excp_ch;
	CCCI_CH tx_ch_mapping;
	CCCI_CH rx_ch_mapping;
};

static struct c2k_port c2k_ports[] = {
	/*c2k control channel mapping to 2 pairs of CCCI channels,
	   please mind the order in this array, make sure CCCI_CONTROL_TX/RX be first. */
	{CTRL_CH_C2K, CCCI_CONTROL_TX, CCCI_CONTROL_TX, CCCI_CONTROL_RX,},	/*control channel */
	{CTRL_CH_C2K, CTRL_CH_C2K, CCCI_STATUS_TX, CCCI_STATUS_RX,},	/*control channel */
	{AUDIO_CH_C2K, AUDIO_CH_C2K, CCCI_PCM_TX, CCCI_PCM_RX,},	/*audio channel */
	{NET1_CH_C2K, NET1_CH_C2K, CCCI_CCMNI1_TX, CCCI_CCMNI1_RX,},	/*network channel for CCMNI1 */
	{NET1_CH_C2K, NET1_CH_C2K, CCCI_CCMNI1_DL_ACK, CCCI_CCMNI1_DL_ACK,},	/*network channel for CCMNI1 */
	{NET2_CH_C2K, NET2_CH_C2K, CCCI_CCMNI2_TX, CCCI_CCMNI2_RX,},	/*network channel for CCMNI2 */
	{NET2_CH_C2K, NET2_CH_C2K, CCCI_CCMNI2_DL_ACK, CCCI_CCMNI2_DL_ACK,},	/*network channel for CCMNI2 */
	{NET3_CH_C2K, NET3_CH_C2K, CCCI_CCMNI3_TX, CCCI_CCMNI3_RX,},	/*network channel for CCMNI3 */
	{NET3_CH_C2K, NET3_CH_C2K, CCCI_CCMNI3_DL_ACK, CCCI_CCMNI3_DL_ACK,},	/*network channel for CCMNI3 */
	{MDLOG_CTRL_CH_C2K, MDLOG_CTRL_CH_C2K, CCCI_UART1_TX, CCCI_UART1_RX,},	/*mdlogger ctrl channel */
	{MDLOG_CH_C2K, MDLOG_CH_C2K, CCCI_MD_LOG_TX, CCCI_MD_LOG_RX,},	/*mdlogger data channel */
	{FS_CH_C2K, FS_CH_C2K, CCCI_FS_TX, CCCI_FS_RX,},	/*flashless channel, new */
	{DATA_PPP_CH_C2K, DATA_PPP_CH_C2K, CCCI_C2K_PPP_DATA, CCCI_C2K_PPP_DATA,},	/*ppp channel, for usb bypass*/
	{AT_CH_C2K, AT_CH_C2K, CCCI_C2K_AT, CCCI_C2K_AT,},	/*AT for rild, new */
	{AT2_CH_C2K, AT2_CH_C2K, CCCI_C2K_AT2, CCCI_C2K_AT2,},	/*AT2 for rild, new */
	{AT3_CH_C2K, AT3_CH_C2K, CCCI_C2K_AT3, CCCI_C2K_AT3,},	/*AT3 for rild, new */
	{AGPS_CH_C2K, AGPS_CH_C2K, CCCI_IPC_UART_TX, CCCI_IPC_UART_RX,},	/*agps channel */
	{MD2AP_LOOPBACK_C2K, MD2AP_LOOPBACK_C2K, CCCI_C2K_LB_DL, CCCI_C2K_LB_DL,},
	{LOOPBACK_C2K, LOOPBACK_C2K, CCCI_LB_IT_TX, CCCI_LB_IT_RX,},
	{STATUS_CH_C2K, STATUS_CH_C2K, CCCI_CONTROL_TX, CCCI_CONTROL_RX,},
};

/*always keep this in mind: what if there are more than 1 modems using CLDMA...*/

/*ccif share memory setting*/
/*need confirm with md. haow*/
static int rx_queue_buffer_size[QUEUE_NUM] = { 10 * 1024, 100 * 1024,
	100 * 1024, 150 * 1024, 50 * 1024, 10 * 1024, 10 * 1024, 10 * 1024,
};

static int tx_queue_buffer_size[QUEUE_NUM] = { 10 * 1024, 100 * 1024,
	50 * 1024, 50 * 1024, 50 * 1024, 10 * 1024, 10 * 1024, 10 * 1024,
};

static void md_ccif_dump(unsigned char *title, struct ccci_modem *md)
{
	int idx;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(md->index, TAG, "md_ccif_dump: %s\n", title);
	CCCI_INF_MSG(md->index, TAG, "AP_CON(%p)=%d\n",
		     md_ctrl->ccif_ap_base + APCCIF_CON,
		     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_CON));
	CCCI_INF_MSG(md->index, TAG, "AP_BUSY(%p)=%d\n",
		     md_ctrl->ccif_ap_base + APCCIF_BUSY,
		     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_BUSY));
	CCCI_INF_MSG(md->index, TAG, "AP_START(%p)=%d\n",
		     md_ctrl->ccif_ap_base + APCCIF_START,
		     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_START));
	CCCI_INF_MSG(md->index, TAG, "AP_TCHNUM(%p)=%d\n",
		     md_ctrl->ccif_ap_base + APCCIF_TCHNUM,
		     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_TCHNUM));
	CCCI_INF_MSG(md->index, TAG, "AP_RCHNUM(%p)=%d\n",
		     md_ctrl->ccif_ap_base + APCCIF_RCHNUM,
		     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_RCHNUM));
	CCCI_INF_MSG(md->index, TAG, "AP_ACK(%p)=%d\n",
		     md_ctrl->ccif_ap_base + APCCIF_ACK,
		     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_ACK));
	CCCI_INF_MSG(md->index, TAG, "MD_CON(%p)=%d\n",
		     md_ctrl->ccif_md_base + APCCIF_CON,
		     ccif_read32(md_ctrl->ccif_md_base, APCCIF_CON));
	CCCI_INF_MSG(md->index, TAG, "MD_BUSY(%p)=%d\n",
		     md_ctrl->ccif_md_base + APCCIF_BUSY,
		     ccif_read32(md_ctrl->ccif_md_base, APCCIF_BUSY));
	CCCI_INF_MSG(md->index, TAG, "MD_START(%p)=%d\n",
		     md_ctrl->ccif_md_base + APCCIF_START,
		     ccif_read32(md_ctrl->ccif_md_base, APCCIF_START));
	CCCI_INF_MSG(md->index, TAG, "MD_TCHNUM(%p)=%d\n",
		     md_ctrl->ccif_md_base + APCCIF_TCHNUM,
		     ccif_read32(md_ctrl->ccif_md_base, APCCIF_TCHNUM));
	CCCI_INF_MSG(md->index, TAG, "MD_RCHNUM(%p)=%d\n",
		     md_ctrl->ccif_md_base + APCCIF_RCHNUM,
		     ccif_read32(md_ctrl->ccif_md_base, APCCIF_RCHNUM));
	CCCI_INF_MSG(md->index, TAG, "MD_ACK(%p)=%d\n",
		     md_ctrl->ccif_md_base + APCCIF_ACK,
		     ccif_read32(md_ctrl->ccif_md_base, APCCIF_ACK));

	for (idx = 0; idx < md_ctrl->sram_size / sizeof(u32); idx += 4) {
		CCCI_INF_MSG(md->index, TAG,
			     "CHDATA(%p): %08X %08X %08X %08X\n",
			     md_ctrl->ccif_ap_base + APCCIF_CHDATA +
			     idx * sizeof(u32),
			     ccif_read32(md_ctrl->ccif_ap_base + APCCIF_CHDATA,
					 (idx + 0) * sizeof(u32)),
			     ccif_read32(md_ctrl->ccif_ap_base + APCCIF_CHDATA,
					 (idx + 1) * sizeof(u32)),
			     ccif_read32(md_ctrl->ccif_ap_base + APCCIF_CHDATA,
					 (idx + 2) * sizeof(u32)),
			     ccif_read32(md_ctrl->ccif_ap_base + APCCIF_CHDATA,
					 (idx + 3) * sizeof(u32)));
	}

}

/*direction: 1: tx; 0: rx*/
static int c2k_ch_to_ccci_ch(struct ccci_modem *md, int c2k_ch, int direction)
{
	u16 c2k_channel_id;
	int i = 0;

	c2k_channel_id = (u16) c2k_ch;
	for (i = 0; i < (sizeof(c2k_ports) / sizeof(struct c2k_port)); i++) {
		if (c2k_channel_id == c2k_ports[i].ch) {
			CCCI_DBG_MSG(md->index, TAG,
				     "%s:channel(%d)-->(T%d R%d)\n",
				     (direction == OUT) ? "TX" : "RX", c2k_ch,
				     c2k_ports[i].tx_ch_mapping,
				     c2k_ports[i].rx_ch_mapping);
			return (direction ==
				OUT) ? c2k_ports[i].tx_ch_mapping :
			    c2k_ports[i].rx_ch_mapping;
		}
	}

	CCCI_ERR_MSG(md->index, TAG,
		     "%s:ERR cannot find mapped c2k ch ID(%d)\n",
		     direction ? "TX" : "RX", c2k_ch);
	return -1;
}

static int ccci_ch_to_c2k_ch(struct ccci_modem *md, int ccci_ch, int direction)
{
	u16 ccci_channel_id;
	u16 channel_map;
	int i = 0;

	ccci_channel_id = (u16) ccci_ch;
	for (i = 0; i < (sizeof(c2k_ports) / sizeof(struct c2k_port)); i++) {
		channel_map =
		    (direction ==
		     OUT) ? c2k_ports[i].
		    tx_ch_mapping : c2k_ports[i].rx_ch_mapping;

		if (ccci_channel_id == channel_map) {
			CCCI_DBG_MSG(md->index, TAG, "%s:channel(%d)-->(%d)\n",
				     (direction == OUT) ? "TX" : "RX",
				     ccci_channel_id, c2k_ports[i].ch);
			return (md->md_state !=
				EXCEPTION) ? c2k_ports[i].
			    ch : c2k_ports[i].excp_ch;
		}
	}

	CCCI_ERR_MSG(md->index, TAG,
		     "%s:ERR cannot find mapped ccci ch ID(%d)\n",
		     direction ? "TX" : "RX", ccci_ch);
	return -1;
}

static void md_ccif_sram_rx_work(struct work_struct *work)
{
	struct md_ccif_ctrl *md_ctrl =
	    container_of(work, struct md_ccif_ctrl, ccif_sram_work);
	struct ccci_modem *md = md_ctrl->rxq[0].modem;
	struct ccci_header *dl_pkg = &md_ctrl->ccif_sram_layout->dl_header;
	struct ccci_header *ccci_h;
	struct ccci_header ccci_hdr;
	struct ccci_request *new_req = NULL;
	struct ccci_request *req;
	int pkg_size, ret = 0, retry_cnt = 0;
	/*md_ccif_dump("md_ccif_sram_rx_work",md); */
#ifdef AP_MD_HS_V2
	u32 i = 0;
	u8 *md_feature = (u8 *) (((u8 *) dl_pkg) + sizeof(struct ccci_header));

	CCCI_INF_MSG(md->index, TAG, "md_ccif_sram_rx_work:dk_pkg=%p, md_featrue=%p\n", dl_pkg, md_feature);
	pkg_size = sizeof(struct ccci_header) + sizeof(struct md_query_ap_feature);
#else
	pkg_size = sizeof(struct ccci_header);
#endif
	new_req = ccci_alloc_req(IN, pkg_size, 1, 0);
	INIT_LIST_HEAD(&new_req->entry);	/*as port will run list_del */
	if (new_req->skb == NULL) {
		CCCI_ERR_MSG(md->index, TAG,
			     "md_ccif_sram_rx_work:ccci_alloc_req pkg_size=%d failed\n",
			     pkg_size);
		return;
	}
	skb_put(new_req->skb, pkg_size);
	ccci_h = (struct ccci_header *)new_req->skb->data;
	ccci_h->data[0] = ccif_read32(&dl_pkg->data[0], 0);
	ccci_h->data[1] = ccif_read32(&dl_pkg->data[1], 0);
	/*ccci_h->channel = ccif_read32(&dl_pkg->channel,0); */
	*(((u32 *) ccci_h) + 2) = ccif_read32((((u32 *) dl_pkg) + 2), 0);
	if (md->index == MD_SYS3)
		ccci_h->channel = c2k_ch_to_ccci_ch(md, ccci_h->channel, IN);

	ccci_h->reserved = ccif_read32(&dl_pkg->reserved, 0);

#ifdef AP_MD_HS_V2
	/*warning: make sure struct md_query_ap_feature is 4 bypes align */
	while (i < sizeof(struct md_query_ap_feature)) {
		*((u32 *) (new_req->skb->data + sizeof(struct ccci_header) + i)) =
		    ccif_read32((u32 *) (md_feature + i), 0);
		i += 4;
	}
#endif
	if (atomic_cmpxchg(&md->wakeup_src, 1, 0) == 1)
		CCCI_INF_MSG(md->index, TAG,
			     "CCIF_MD wakeup source:(SRX_IDX/%d)\n",
			     *(((u32 *) ccci_h) + 2));

	ccci_hdr = *ccci_h;

 RETRY:
	ret = ccci_port_recv_request(md, new_req, NULL);
	CCCI_DBG_MSG(md->index, TAG, "Rx msg %x %x %x %x ret=%d\n",
		     ccci_h->data[0], ccci_h->data[1], *(((u32 *) ccci_h) + 2),
		     ccci_h->reserved, ret);
	if (ret >= 0 || ret == -CCCI_ERR_DROP_PACKET) {
		CCCI_INF_MSG(md->index, TAG,
			     "md_ccif_sram_rx_work:ccci_port_recv_request ret=%d\n",
			     ret);
		ccci_chk_rx_seq_num(md, &ccci_hdr, 0);
		/*step forward */
		req = list_entry(req->entry.next, struct ccci_request, entry);
	} else {
		if (retry_cnt > 20) {
			CCCI_ERR_MSG(md->index, TAG,
				     "md_ccif_sram_rx_work:ccci_port_recv_request ret=%d,retry=%d\n",
				     ret, retry_cnt);
			udelay(5);
			retry_cnt++;
			goto RETRY;
		}
		list_del(&new_req->entry);
		ccci_free_req(new_req);
		CCCI_INF_MSG(md->index, TAG,
			     "md_ccif_sram_rx_work:ccci_port_recv_request ret=%d\n",
			     ret);
	}
}

void c2k_mem_dump(void *start_addr, int len)
{
	unsigned int *curr_p = (unsigned int *)start_addr;
	unsigned char *curr_ch_p;
	int _16_fix_num = len / 16;
	int tail_num = len % 16;
	char buf[16];
	int i, j;

	if (NULL == curr_p) {
		CCCI_ERR_MSG(MD_SYS3, TAG, "[C2K-DUMP]NULL point to dump!\n");
		return;
	}
	if (0 == len) {
		CCCI_ERR_MSG(MD_SYS3, TAG, "[C2K-DUMP]Not need to dump\n");
		return;
	}

	CCCI_DBG_MSG(MD_SYS3, TAG, "[C2K-DUMP]Base: 0x%lx, len: %d\n", (unsigned long)start_addr,
		 len);
	/*Fix section */
	for (i = 0; i < _16_fix_num; i++) {
		CCCI_INF_MSG(MD_SYS3, TAG, "[C2K-DUMP]%03X: %08X %08X %08X %08X\n",
			 i * 16, *curr_p, *(curr_p + 1), *(curr_p + 2),
			 *(curr_p + 3));
		curr_p += 4;
	}

	/*Tail section */
	if (tail_num > 0) {
		curr_ch_p = (unsigned char *)curr_p;
		for (j = 0; j < tail_num; j++) {
			buf[j] = *curr_ch_p;
			curr_ch_p++;
		}
		for (; j < 16; j++)
			buf[j] = 0;
		curr_p = (unsigned int *)buf;
		CCCI_INF_MSG(MD_SYS3, TAG, "[C2K-DUMP]%03X: %08X %08X %08X %08X\n",
			 i * 16, *curr_p, *(curr_p + 1), *(curr_p + 2),
			 *(curr_p + 3));
	}
}

static inline void md_ccif_tx_rx_printk(struct ccci_modem *md, struct sk_buff *skb, u8 qno, u8 is_tx)
{
	struct ccci_header *ccci_h = (struct ccci_header *)skb->data;
	unsigned int data_len = skb->len - sizeof(struct ccci_header);
	unsigned int dump_len = data_len > 16 ? 16 : data_len;

	switch (ccci_h->channel) {
	/*debug level*/
	case CCCI_C2K_AT:
	case CCCI_C2K_AT2:
	case CCCI_C2K_AT3:
		if (is_tx)
			CCCI_DBG_MSG(md->index, TAG, "TX:OK on Q%d: %x %x %x %x, seq(%d)\n", qno, ccci_h->data[0],
				ccci_h->data[1], *(((u32 *) ccci_h) + 2), ccci_h->reserved, ccci_h->seq_num);
		else
			CCCI_DBG_MSG(md->index, TAG, "Q%d Rx msg %x %x %x %x, seq(%d)\n", qno, ccci_h->data[0],
				ccci_h->data[1], *(((u32 *) ccci_h) + 2), ccci_h->reserved, ccci_h->seq_num);
		break;
	/*info level*/
	case CCCI_UART1_TX:
	case CCCI_UART1_RX:
	case CCCI_CONTROL_TX:
	case CCCI_CONTROL_RX:
	case CCCI_STATUS_TX:
	case CCCI_STATUS_RX:
		if (is_tx)
			CCCI_INF_MSG(md->index, TAG, "TX:OK on Q%d: %x %x %x %x, seq(%d)\n", qno, ccci_h->data[0],
				ccci_h->data[1], *(((u32 *) ccci_h) + 2), ccci_h->reserved, ccci_h->seq_num);
		else
			CCCI_INF_MSG(md->index, TAG, "Q%d Rx msg %x %x %x %x, seq(%d)\n", qno, ccci_h->data[0],
				ccci_h->data[1], *(((u32 *) ccci_h) + 2), ccci_h->reserved, ccci_h->seq_num);
		if (data_len > 0)
			c2k_mem_dump(skb->data + sizeof(struct ccci_header), dump_len);
		break;
	default:
		break;
	};
}

atomic_t lb_dl_q;
/*this function may be called from both workqueue and softirq (NAPI)*/
static unsigned long rx_data_cnt;
static unsigned int pkg_num;
static int ccif_rx_collect(struct md_ccif_queue *queue, int budget,
			   int blocking, int *result)
{
	struct ccci_modem *md = queue->modem;
	struct ccci_ringbuf *rx_buf = queue->ringbuf;
	struct ccci_request *new_req = NULL;
	struct ccci_request *req;
	unsigned char *data_ptr;
	int ret = 0, count = 0, pkg_size;
	unsigned long flags;
	int qno = queue->index;
	struct ccci_header *ccci_h = NULL;
	struct ccci_header ccci_hdr;
	struct sk_buff *skb;
	int c2k_to_ccci_ch = 0;

	if (atomic_read(&queue->rx_on_going)) {
		CCCI_DBG_MSG(md->index, TAG, "Q%d rx is on-going(%d)1\n",
			     queue->index, atomic_read(&queue->rx_on_going));
		*result = 0;
		return -1;
	}
	atomic_set(&queue->rx_on_going, 1);

	while (1) {
		pkg_size = ccci_ringbuf_readable(md->index, rx_buf);
		if (pkg_size < 0) {
			CCCI_DBG_MSG(md->index, TAG,
				     "Q%d Rx:rbf readable ret=%d\n",
				     queue->index, pkg_size);
			/*BUG_ON(pkg_size!=-CCCI_RINGBUF_EMPTY); */
			ret = 0;
			goto OUT;
		}
		if (IS_PASS_SKB(md, qno)) {
			skb = ccci_alloc_skb(pkg_size, 0, blocking);
			if (skb == NULL) {
				ret = -ENOMEM;
				goto OUT;
			}
		} else {
			new_req = ccci_alloc_req(IN, pkg_size, blocking, 0);
			if (new_req == NULL || new_req->skb == NULL) {
				CCCI_ERR_MSG(md->index, TAG,
					     "Q%d Rx:ccci_alloc_skb pkg_size=%d failed,count=%d\n",
					     queue->index, pkg_size, count);
				ret = -ENOMEM;
				goto OUT;
			}
			INIT_LIST_HEAD(&new_req->entry);	/*as port will run list_del   */
			skb = new_req->skb;
		}
		data_ptr = (unsigned char *)skb_put(skb, pkg_size);
		/*copy data into skb */
		ret = ccci_ringbuf_read(md->index, rx_buf, data_ptr, pkg_size);
		BUG_ON(ret < 0);
		ccci_h = (struct ccci_header *)skb->data;
		if (md->index == MD_SYS3) {
			/*md3(c2k) logical channel number is not the same as other modems,
			   so we need use mapping table to convert channel id here. */
			c2k_to_ccci_ch =
			    c2k_ch_to_ccci_ch(md, ccci_h->channel, IN);
			if (c2k_to_ccci_ch >= 0)
				ccci_h->channel = (u16) c2k_to_ccci_ch;

			/*heart beat msg from c2k control channel, but handled by ECCCI status channel handler,
			   we hack the channel ID here. */
			/*if((ccci_h->channel == CCCI_CONTROL_RX) && (ccci_h->data[1] == C2K_HB_MSG))
			   {
			   ccci_h->channel == CCCI_STATUS_RX;
			   CCCI_INF_MSG(md->index, TAG, "heart beat msg received\n");
			   } */
			if (ccci_h->channel == CCCI_C2K_LB_DL) {
				CCCI_DBG_MSG(md->index, TAG, "Q%d Rx lb_dl\n",
					     queue->index);
				/*c2k_mem_dump(data_ptr, pkg_size); */
			}
		}
		if (atomic_cmpxchg(&md->wakeup_src, 1, 0) == 1)
			CCCI_INF_MSG(md->index, TAG, "CCIF_MD wakeup source:(%d/%d)\n",
				queue->index, *(((u32 *) ccci_h) + 2));

		md_ccif_tx_rx_printk(md, skb, queue->index, 0);

		if (ccci_h->channel == CCCI_C2K_LB_DL)
			atomic_set(&lb_dl_q, queue->index);

		ccci_hdr = *ccci_h;

		ret = ccci_port_recv_request(md, new_req, skb);

		if (ret >= 0 || ret == -CCCI_ERR_DROP_PACKET) {
			count++;
			ccci_chk_rx_seq_num(md, &ccci_hdr, queue->index);
			if (queue->debug_id) {
				CCCI_INF_MSG(md->index, TAG,
					     "Q%d Rx recv req ret=%d\n",
					     queue->index, ret);
				queue->debug_id = 0;
			}
			ccci_ringbuf_move_rpointer(md->index, rx_buf, pkg_size);
			if (ccci_h->channel == CCCI_MD_LOG_RX) {
				rx_data_cnt += pkg_size - 16;
				pkg_num++;
				CCCI_DBG_MSG(md->index, TAG,
					     "Q%d Rx buf read=%d, write=%d, pkg_size=%d, log_cnt=%ld, pkg_num=%d\n",
					     queue->index,
					     rx_buf->rx_control.read,
					     rx_buf->rx_control.write, pkg_size,
					     rx_data_cnt, pkg_num);
			}
			ret = 0;
			/*step forward */
			req =
			    list_entry(req->entry.next, struct ccci_request,
				       entry);
		} else {
			/*leave package into share memory, and waiting ccci to receive */
			if (IS_PASS_SKB(md, qno)) {
				dev_kfree_skb_any(skb);
			} else {
				list_del(&new_req->entry);
				ccci_free_req(new_req);
			}

			if (queue->debug_id == 0) {
				queue->debug_id = 1;
				CCCI_ERR_MSG(md->index, TAG, "Q%d Rx err\n",
					     queue->index);
			}
			ret = -EAGAIN;
			goto OUT;
		}
		if (count > budget) {
			CCCI_DBG_MSG(md->index, TAG,
				     "Q%d count > budget, exit now\n",
				     queue->index);
			goto OUT;
		}
	}

 OUT:
	atomic_set(&queue->rx_on_going, 0);
	*result = count;
	CCCI_DBG_MSG(md->index, TAG, "Q%d rx %d pkg,ret=%d\n", queue->index,
		     count, ret);
	spin_lock_irqsave(&queue->rx_lock, flags);
	if (ret != -EAGAIN) {
		pkg_size = ccci_ringbuf_readable(md->index, rx_buf);
		if (pkg_size > 0)
			ret = -EAGAIN;
	}
	spin_unlock_irqrestore(&queue->rx_lock, flags);
	return ret;
}

static void ccif_rx_work(struct work_struct *work)
{
	int result = 0, ret = 0;
	struct md_ccif_queue *queue =
	    container_of(work, struct md_ccif_queue, qwork);
	ret = ccif_rx_collect(queue, queue->budget, 1, &result);
	if (ret == -EAGAIN) {
		CCCI_DBG_MSG(queue->modem->index, TAG, "queue again\n");
		queue_work(queue->worker, &queue->qwork);
	}
}

static irqreturn_t md_cd_wdt_isr(int irq, void *data)
{
	struct ccci_modem *md = (struct ccci_modem *)data;
#ifdef ENABLE_MD_WDT_DBG
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
#endif
	int ret = 0;

	CCCI_INF_MSG(md->index, TAG, "MD WDT IRQ\n");
	/*1. disable MD WDT */
#ifdef ENABLE_MD_WDT_DBG
	unsigned int state;

	state = ccif_read32(md_ctrl->md_rgu_base, WDT_MD_STA);
	ccif_write32(md_ctrl->md_rgu_base, WDT_MD_MODE, WDT_MD_MODE_KEY);
	CCCI_INF_MSG(md->index, TAG, "WDT IRQ disabled for debug, state=%X\n",
		     state);
#endif

	if (*((int *)(md->mem_layout.smem_region_vir + CCCI_SMEM_OFFSET_EPON))
	    == 0xBAEBAE10) {
		/*3. reset */
		ret = md->ops->reset(md);
		CCCI_INF_MSG(md->index, TAG, "reset MD after WDT %d\n", ret);
		/*4. send message, only reset MD on non-eng load */
		ccci_send_virtual_md_msg(md, CCCI_MONITOR_CH, CCCI_MD_MSG_RESET,
					 0);
		/* #ifdef CONFIG_MTK_SVLTE_SUPPORT */
		#ifdef CONFIG_MTK_ECCCI_C2K
		if (md->index == MD_SYS3)
			exec_ccci_kern_func_by_md_id(MD_SYS1, ID_RESET_MD, NULL, 0);
		#endif
		/* #endif */

	} else {
		ccci_md_exception_notify(md, MD_WDT);
	}
	return IRQ_HANDLED;
}

static int md_ccif_send(struct ccci_modem *md, int channel_id)
{
	int busy = 0;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	busy = ccif_read32(md_ctrl->ccif_ap_base, APCCIF_BUSY);
	if (busy & (1 << channel_id)) {
		CCCI_DBG_MSG(md->index, TAG, "CCIF channel %d busy\n",
			     channel_id);
	} else {
		ccif_write32(md_ctrl->ccif_ap_base, APCCIF_BUSY,
			     1 << channel_id);
		ccif_write32(md_ctrl->ccif_ap_base, APCCIF_TCHNUM, channel_id);
		CCCI_DBG_MSG(md->index, TAG, "CCIF start=0x%x\n",
			     ccif_read32(md_ctrl->ccif_ap_base, APCCIF_START));
	}
	return 0;
}

static void md_ccif_sram_reset(struct ccci_modem *md)
{
	int idx = 0;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(md->index, TAG, "md_ccif_sram_reset\n");
	for (idx = 0; idx < md_ctrl->sram_size / sizeof(u32); idx += 1) {
		ccif_write32(md_ctrl->ccif_ap_base + APCCIF_CHDATA,
			     idx * sizeof(u32), 0);
	}
}

static void md_ccif_queue_dump(struct ccci_modem *md)
{
	int idx;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(md->index, TAG, "Dump CCIF Queue Control\n");
	for (idx = 0; idx < QUEUE_NUM; idx++) {
		CCCI_INF_MSG(md->index, TAG, "Q%d TX: w=%d, r=%d, len=%d\n",
			     idx, md_ctrl->txq[idx].ringbuf->tx_control.write,
			     md_ctrl->txq[idx].ringbuf->tx_control.read,
			     md_ctrl->txq[idx].ringbuf->tx_control.length);
		CCCI_INF_MSG(md->index, TAG, "Q%d RX: w=%d, r=%d, len=%d\n",
			     idx, md_ctrl->rxq[idx].ringbuf->rx_control.write,
			     md_ctrl->rxq[idx].ringbuf->rx_control.read,
			     md_ctrl->rxq[idx].ringbuf->rx_control.length);
	}
}

static void md_ccif_reset_queue(struct ccci_modem *md)
{
	int i;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(md->index, TAG, "md_ccif_reset_queue\n");
	for (i = 0; i < QUEUE_NUM; ++i) {
		ccci_ringbuf_reset(md->index, md_ctrl->rxq[i].ringbuf, 0);
		ccci_ringbuf_reset(md->index, md_ctrl->txq[i].ringbuf, 1);
	}
}

static void md_ccif_exception(struct ccci_modem *md, HIF_EX_STAGE stage)
{
	CCCI_INF_MSG(md->index, TAG, "MD exception HIF %d\n", stage);
	switch (stage) {
	case HIF_EX_INIT:
		ccci_md_exception_notify(md, EX_INIT);
		/*Rx dispatch does NOT depend on queue index in port structure, so it still can find right port. */
		md_ccif_send(md, H2D_EXCEPTION_ACK);
		break;
	case HIF_EX_INIT_DONE:
		ccci_md_exception_notify(md, EX_DHL_DL_RDY);
		break;
	case HIF_EX_CLEARQ_DONE:
		md_ccif_queue_dump(md);
		md_ccif_reset_queue(md);
		md_ccif_send(md, H2D_EXCEPTION_CLEARQ_ACK);
		break;
	case HIF_EX_ALLQ_RESET:
		ccci_md_exception_notify(md, EX_INIT_DONE);
		break;
	default:
		break;
	};
}

static void md_ccif_irq_tasklet(unsigned long data)
{
	struct ccci_modem *md = (struct ccci_modem *)data;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	int i;

	CCCI_DBG_MSG(md->index, TAG, "ccif_irq_tasklet1: ch %ld\n",
		     md_ctrl->channel_id);
	while (md_ctrl->channel_id != 0) {
		if (md_ctrl->channel_id & CCIF_HW_CH_RX_RESERVED) {
			CCCI_ERR_MSG(md->index, TAG,
				     "Interrupt from reserved ccif ch(%ld)\n",
				     md_ctrl->channel_id);
			md_ctrl->channel_id &= ~CCIF_HW_CH_RX_RESERVED;
			CCCI_ERR_MSG(md->index, TAG,
				     "After cleared reserved ccif ch(%ld)\n",
				     md_ctrl->channel_id);
		}
		if (md_ctrl->channel_id & (1 << D2H_EXCEPTION_INIT)) {
			clear_bit(D2H_EXCEPTION_INIT, &md_ctrl->channel_id);
			md_ccif_exception(md, HIF_EX_INIT);
		}
		if (md_ctrl->channel_id & (1 << D2H_EXCEPTION_INIT_DONE)) {
			clear_bit(D2H_EXCEPTION_INIT_DONE,
				  &md_ctrl->channel_id);
			md_ccif_exception(md, HIF_EX_INIT_DONE);
		}
		if (md_ctrl->channel_id & (1 << D2H_EXCEPTION_CLEARQ_DONE)) {
			clear_bit(D2H_EXCEPTION_CLEARQ_DONE,
				  &md_ctrl->channel_id);
			md_ccif_exception(md, HIF_EX_CLEARQ_DONE);
		}
		if (md_ctrl->channel_id & (1 << D2H_EXCEPTION_ALLQ_RESET)) {
			clear_bit(D2H_EXCEPTION_ALLQ_RESET,
				  &md_ctrl->channel_id);
			md_ccif_exception(md, HIF_EX_ALLQ_RESET);
		}
		if (md_ctrl->channel_id & (1 << AP_MD_SEQ_ERROR)) {
			clear_bit(AP_MD_SEQ_ERROR, &md_ctrl->channel_id);
			CCCI_ERR_MSG(md->index, TAG, "MD check seq fail\n");
			md->ops->dump_info(md, DUMP_FLAG_CCIF, NULL, 0);
		}
		if (md_ctrl->channel_id & (1 << (D2H_SRAM))) {
			clear_bit(D2H_SRAM, &md_ctrl->channel_id);
			schedule_work(&md_ctrl->ccif_sram_work);
		}
		for (i = 0; i < QUEUE_NUM; i++) {
			if (md_ctrl->channel_id & (1 << (i + D2H_RINGQ0))) {
				clear_bit(i + D2H_RINGQ0, &md_ctrl->channel_id);
				if (atomic_read(&md_ctrl->rxq[i].rx_on_going)) {
					CCCI_DBG_MSG(md->index, TAG,
						     "Q%d rx is on-going(%d)2\n",
						     md_ctrl->rxq[i].index,
						     atomic_read(&md_ctrl->rxq
								 [i].rx_on_going));
					return;
				}
				if (md->md_state != EXCEPTION
				    && (md->capability & MODEM_CAP_NAPI)
				    && md_ctrl->rxq[i].napi_port
				    &&
				    ((1 << md_ctrl->rxq[i].
				      napi_port->rxq_index) & NAPI_QUEUE_MASK)) {
					md_ctrl->rxq[i].napi_port->
					    ops->md_state_notice(md_ctrl->
								 rxq
								 [i].napi_port,
								 RX_IRQ);
				} else {
					queue_work(md_ctrl->rxq[i].worker,
						   &md_ctrl->rxq[i].qwork);
				}
			}
		}
		CCCI_DBG_MSG(md->index, TAG, "ccif_irq_tasklet2: ch %ld\n",
			     md_ctrl->channel_id);
	}
}

unsigned int ccif_irq_cnt = 0;
static irqreturn_t md_ccif_isr(int irq, void *data)
{
	struct ccci_modem *md = (struct ccci_modem *)data;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	unsigned int ch_id;
	/*disable_irq_nosync(md_ctrl->ccif_irq_id); */
	/*must ack first, otherwise IRQ will rush in */
	ch_id = ccif_read32(md_ctrl->ccif_ap_base, APCCIF_RCHNUM);
	md_ctrl->channel_id |= ch_id;
	ccif_write32(md_ctrl->ccif_ap_base, APCCIF_ACK, ch_id);
	/*enable_irq(md_ctrl->ccif_irq_id); */
	if (md_ctrl->channel_id == 0x800)
		CCCI_DBG_MSG(md->index, TAG, "MD CCIF IRQ %ld, %d\n",
			     md_ctrl->channel_id, ccif_irq_cnt++);
	tasklet_hi_schedule(&md_ctrl->ccif_irq_task);

	return IRQ_HANDLED;
}

static int md_ccif_op_broadcast_state(struct ccci_modem *md, MD_STATE state)
{
	int i;
	struct ccci_port *port;
	/*only for thoes states which are updated by port_kernel.c */
	switch (state) {
	case BOOT_FAIL:
		return 0;
	case RX_IRQ:
		CCCI_ERR_MSG(md->index, TAG, "%ps broadcast RX_IRQ to ports!\n",
			     __builtin_return_address(0));
		return 0;
	default:
		break;
	};
	if (md->md_state == state)	/*must have, due to we broadcast EXCEPTION both in MD_EX and EX_INIT */
		return 1;

	md->md_state = state;
	for (i = 0; i < md->port_number; i++) {
		port = md->ports + i;
		if (port->ops->md_state_notice)
			port->ops->md_state_notice(port, state);
	}
	return 0;
}

static inline void md_ccif_queue_struct_init(struct md_ccif_queue *queue,
					     struct ccci_modem *md,
					     DIRECTION dir, unsigned char index)
{
	queue->dir = dir;
	queue->index = index;
	queue->modem = md;
	queue->napi_port = NULL;
	init_waitqueue_head(&queue->req_wq);
	spin_lock_init(&queue->rx_lock);
	spin_lock_init(&queue->tx_lock);
	atomic_set(&queue->rx_on_going, 0);
	queue->debug_id = 0;
	queue->budget = RX_BUGDET;
}

static int md_ccif_op_init(struct ccci_modem *md)
{
	int i;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	struct ccci_port *port;

	CCCI_INF_MSG(md->index, TAG, "CCIF modem is initializing\n");
	/*init queue */
	for (i = 0; i < QUEUE_NUM; i++) {
		md_ccif_queue_struct_init(&md_ctrl->txq[i], md, OUT, i);
		md_ccif_queue_struct_init(&md_ctrl->rxq[i], md, IN, i);
	}

	/*init port */
	for (i = 0; i < md->port_number; i++) {
		port = md->ports + i;
		ccci_port_struct_init(port, md);
		port->ops->init(port);
		if ((port->flags & PORT_F_RX_EXCLUSIVE)
		    && (md->capability & MODEM_CAP_NAPI)
		    && ((1 << port->rxq_index) & NAPI_QUEUE_MASK)) {
			md_ctrl->rxq[port->rxq_index].napi_port = port;
			CCCI_INF_MSG(md->index, TAG,
				     "queue%d add NAPI port %s\n",
				     port->rxq_index, port->name);
		}
	}
	ccci_setup_channel_mapping(md);
	/*update state */
	md->md_state = GATED;

	return 0;
}

/*used for throttling feature - end*/
static int md_ccif_op_start(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	char img_err_str[IMG_ERR_STR_LEN];
	int ret = 0;
	/*0. init security, as security depends on dummy_char, which is ready very late. */
	ccci_init_security();
	md_ccif_sram_reset(md);
	md_ccif_reset_queue(md);
	ccci_reset_seq_num(md);
	md->heart_beat_counter = 0;
	md->data_usb_bypass = 0;
	CCCI_INF_MSG(md->index, TAG, "CCIF modem is starting\n");
	/*1. load modem image */
	if (md->config.setting & MD_SETTING_FIRST_BOOT
	    || md->config.setting & MD_SETTING_RELOAD) {
		ccci_clear_md_region_protection(md);
		ret =
		    ccci_load_firmware(md->index, &md->img_info[IMG_MD],
				       img_err_str, md->post_fix);
		if (ret < 0) {
			CCCI_ERR_MSG(md->index, TAG, "load firmware fail, %s\n",
				     img_err_str);
			goto out;
		}
		ret = 0;	/*load_std_firmware returns MD image size */
		md->config.setting &= ~MD_SETTING_RELOAD;
	}
	/*2. enable MPU */
	ccci_set_mem_access_protection(md);
	/*3. power on modem, do NOT touch MD register before this */
	ret = md_ccif_power_on(md);
	if (ret) {
		CCCI_ERR_MSG(md->index, TAG, "power on MD fail %d\n", ret);
		goto out;
	}
	/*4. update mutex */
	atomic_set(&md_ctrl->reset_on_going, 0);
	/*5. start timer */
	mod_timer(&md->bootup_timer, jiffies + BOOT_TIMER_ON * HZ);
	/*6. let modem go */
	md->ops->broadcast_state(md, BOOTING);
	md_ccif_let_md_go(md);
	enable_irq(md_ctrl->md_wdt_irq_id);
 out:
	CCCI_INF_MSG(md->index, TAG, "ccif modem started %d\n", ret);
	/*used for throttling feature - start */
	ccci_modem_boot_count[md->index]++;
	/*used for throttling feature - end */
	return ret;
}

static int md_ccif_op_stop(struct ccci_modem *md, unsigned int timeout)
{
	int ret = 0;
	int idx = 0;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(md->index, TAG, "ccif modem is power off, timeout=%d\n",
		     timeout);
	ret = md_ccif_power_off(md, timeout);
	CCCI_INF_MSG(md->index, TAG, "ccif modem is power off done, %d\n", ret);
	for (idx = 0; idx < QUEUE_NUM; idx++)
		flush_work(&md_ctrl->rxq[idx].qwork);

	CCCI_INF_MSG(md->index, TAG, "ccif flush_work done, %d\n", ret);
	md_ccif_reset_queue(md);
	md->ops->broadcast_state(md, GATED);
	return 0;
}

static int md_ccif_op_reset(struct ccci_modem *md)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	/*1. mutex check */
	if (atomic_inc_return(&md_ctrl->reset_on_going) > 1) {
		CCCI_INF_MSG(md->index, TAG, "One reset flow is on-going\n");
		return -CCCI_ERR_MD_IN_RESET;
	}
	CCCI_INF_MSG(md->index, TAG, "ccif modem is resetting\n");
	/*2. disable IRQ (use nosync) */
	disable_irq_nosync(md_ctrl->md_wdt_irq_id);
	md->ops->broadcast_state(md, RESET);	/*to block char's write operation */
	del_timer(&md->bootup_timer);
	md->boot_stage = MD_BOOT_STAGE_0;
	return 0;
}

static int md_ccif_op_write_room(struct ccci_modem *md, unsigned char qno)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	if (qno == 0xFF)
		return -CCCI_ERR_INVALID_QUEUE_INDEX;
	return ccci_ringbuf_writeable(md->index, md_ctrl->txq[qno].ringbuf, 0);
}

static int md_ccif_op_send_request(struct ccci_modem *md, unsigned char qno,
				   struct ccci_request *req,
				   struct sk_buff *skb)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	struct md_ccif_queue *queue = NULL;
	/*struct ccci_header *ccci_h = (struct ccci_header *)req->skb->data; */
	int ret;
	static char lp_qno;
	/*struct ccci_header *ccci_h; */
	unsigned long flags;
	int ccci_to_c2k_ch = 0;

	struct ccci_header *ccci_h;

	if (qno == 0xFF)
		return -CCCI_ERR_INVALID_QUEUE_INDEX;

	queue = &md_ctrl->txq[qno];
	if (req)
		skb = req->skb;

	ccci_h = (struct ccci_header *)skb->data;

	if (ccci_h->channel == CCCI_LB_IT_TX) {
		qno = lp_qno++;
		if (lp_qno > 7)
			lp_qno = 0;
	}
	if (ccci_h->channel == CCCI_C2K_LB_DL)
		qno = atomic_read(&lb_dl_q);

	if (qno > 7)
		CCCI_ERR_MSG(md->index, TAG, "qno error (%d)\n", qno);
	queue = &md_ctrl->txq[qno];
 retry:
	/*we use irqsave as network require a lock in softirq, cause a potential deadlock */
	spin_lock_irqsave(&queue->tx_lock, flags);

	if (ccci_ringbuf_writeable(md->index, queue->ringbuf, skb->len) > 0) {
		if (ccci_h->channel == CCCI_C2K_LB_DL) {
			CCCI_DBG_MSG(md->index, TAG, "Q%d Tx lb_dl\n",
				     queue->index);
			/*c2k_mem_dump(req->skb->data, req->skb->len); */
		}
		ccci_inc_tx_seq_num(md, ccci_h);

		md_ccif_tx_rx_printk(md, skb, qno, 1);

		if (md->index == MD_SYS3) {
			/*heart beat msg is sent from status channel in ECCCI,
			   but from control channel in C2K, no status channel in C2K */
			if (ccci_h->channel == CCCI_STATUS_TX) {
				ccci_h->channel = CCCI_CONTROL_TX;
				ccci_h->data[1] = C2K_HB_MSG;
				ccci_h->reserved = md->heart_beat_counter;
				md->heart_beat_counter++;
				ccci_inc_tx_seq_num(md, ccci_h);
			}

			/*md3(c2k) logical channel number is not the same as other modems,
			   so we need to use mapping table to convert channel id here. */
			ccci_to_c2k_ch =
			    ccci_ch_to_c2k_ch(md, ccci_h->channel, OUT);
			if (ccci_to_c2k_ch >= 0)
				ccci_h->channel = (u16) ccci_to_c2k_ch;

			if (ccci_h->data[1] == C2K_HB_MSG)
				CCCI_INF_MSG(md->index, TAG, "hb: 0x%x\n",
					     ccci_h->channel);
		}
		/*copy skb to ringbuf */
		ret =
		    ccci_ringbuf_write(md->index, queue->ringbuf, skb->data,
				       skb->len);
		if (ret != skb->len) {
			CCCI_ERR_MSG(md->index, TAG,
				     "TX:ERR rbf write: ret(%d)!=req(%d)\n",
				     ret, skb->len);
		}

		/*free request */
		if (IS_PASS_SKB(md, qno))
			dev_kfree_skb_any(skb);
		else
			ccci_free_req(req);

		/*send ccif request */
		md_ccif_send(md, queue->ccif_ch);
		spin_unlock_irqrestore(&queue->tx_lock, flags);
		if (queue->debug_id == 1) {
			CCCI_INF_MSG(md->index, TAG,
				     "TX:OK on q%d,txw=%d,txr=%d,rxw=%d,rxr=%d\n",
				     qno, queue->ringbuf->tx_control.write,
				     queue->ringbuf->tx_control.read,
				     queue->ringbuf->rx_control.write,
				     queue->ringbuf->rx_control.read);

			queue->debug_id = 0;
		}
	} else {
		spin_unlock_irqrestore(&queue->tx_lock, flags);
		if (queue->debug_id == 0) {
			CCCI_INF_MSG(md->index, TAG,
				     "TX:busy on q%d,txw=%d,txr=%d,rxw=%d,rxr=%d\n",
				     qno, queue->ringbuf->tx_control.write,
				     queue->ringbuf->tx_control.read,
				     queue->ringbuf->rx_control.write,
				     queue->ringbuf->rx_control.read);
			queue->debug_id = 1;
		}
		if (IS_PASS_SKB(md, qno))
			return -EBUSY;
		else if (req->blocking) {
			udelay(5);
			/*TODO: add time out check */
			CCCI_INF_MSG(md->index, TAG,
				     "TODO: add time out check busy on q%d\n",
				     qno);
			goto retry;
		} else
			return -EBUSY;
	}
	return 0;
}

static int md_ccif_op_give_more(struct ccci_modem *md, unsigned char qno)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	if (qno == 0xFF)
		return -CCCI_ERR_INVALID_QUEUE_INDEX;
	queue_work(md_ctrl->rxq[qno].worker, &md_ctrl->rxq[qno].qwork);
	return 0;
}

static int md_ccif_op_napi_poll(struct ccci_modem *md, unsigned char qno,
				struct napi_struct *napi, int budget)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	int ret, result = 0;

	if (qno == 0xFF)
		return -CCCI_ERR_INVALID_QUEUE_INDEX;
	if (atomic_read(&md_ctrl->rxq[qno].rx_on_going)) {
		CCCI_DBG_MSG(md->index, TAG, "Q%d rx is on-going(%d)3\n",
			     md_ctrl->rxq[qno].index,
			     atomic_read(&md_ctrl->rxq[qno].rx_on_going));
		return 0;
	}
	budget =
	    budget <
	    md_ctrl->rxq[qno].budget ? budget : md_ctrl->rxq[qno].budget;
	ret = ccif_rx_collect(&md_ctrl->rxq[qno], budget, 0, &result);
	if (ret == 0 && result == 0)
		napi_complete(napi);

	return ret;
}

static struct ccci_port *md_ccif_op_get_port_by_minor(struct ccci_modem *md,
						      int minor)
{
	int i;
	struct ccci_port *port;

	for (i = 0; i < md->port_number; i++) {
		port = md->ports + i;
		if (port->minor == minor)
			return port;
	}
	return NULL;
}

static struct ccci_port *md_ccif_op_get_port_by_channel(struct ccci_modem *md,
							CCCI_CH ch)
{
	int i;
	struct ccci_port *port;

	for (i = 0; i < md->port_number; i++) {
		port = md->ports + i;
		if (port->rx_ch == ch || port->tx_ch == ch)
			return port;
	}
	return NULL;
}

static void dump_runtime_data(struct ccci_modem *md, struct ap_query_md_feature *ap_feature)
{
	u8 i = 0;

	CCCI_DBG_MSG(md->index, KERN, "head_pattern 0x%x\n", ap_feature->head_pattern);

	for (i = BOOT_INFO; i < AP_RUNTIME_FEATURE_ID_MAX; i++) {
		CCCI_DBG_MSG(md->index, KERN, "feature %u: mask %u, version %u\n",
				i, ap_feature->feature_set[i].support_mask, ap_feature->feature_set[i].version);
	}
	CCCI_DBG_MSG(md->index, KERN, "share_memory_support 0x%x\n", ap_feature->share_memory_support);
	CCCI_DBG_MSG(md->index, KERN, "ap_runtime_data_addr 0x%x\n", ap_feature->ap_runtime_data_addr);
	CCCI_DBG_MSG(md->index, KERN, "ap_runtime_data_size 0x%x\n", ap_feature->ap_runtime_data_size);
	CCCI_DBG_MSG(md->index, KERN, "md_runtime_data_addr 0x%x\n", ap_feature->md_runtime_data_addr);
	CCCI_DBG_MSG(md->index, KERN, "md_runtime_data_size 0x%x\n", ap_feature->md_runtime_data_size);
	CCCI_INF_MSG(md->index, KERN, "set_md_mpu_start_addr 0x%x\n", ap_feature->set_md_mpu_start_addr);
	CCCI_INF_MSG(md->index, KERN, "set_md_mpu_total_size 0x%x\n", ap_feature->set_md_mpu_total_size);
	CCCI_DBG_MSG(md->index, KERN, "tail_pattern 0x%x\n", ap_feature->tail_pattern);
}

#ifdef FEATURE_DBM_SUPPORT
static void eccci_c2k_smem_sub_region_init(struct ccci_modem *md)
{
	volatile int __iomem *addr;
	int i;

	/* Region 0, dbm */
	addr = (volatile int __iomem *)(md->mem_layout.smem_region_vir+CCCI_SMEM_MD3_DBM_OFFSET);
	addr[0] = 0x44444444; /* Guard pattern 1 header */
	addr[1] = 0x44444444; /* Guard pattern 2 header */
	#ifdef DISABLE_PBM_FEATURE
	for (i = 2; i < (10+2); i++)
		addr[i] = 0xFFFFFFFF;
	#else
	for (i = 2; i < (10+2); i++)
		addr[i] = 0x00000000;
	#endif
	addr[i++] = 0x44444444; /* Guard pattern 1 tail */
	addr[i++] = 0x44444444; /* Guard pattern 2 tail */

	/* Notify PBM */
	#ifndef DISABLE_PBM_FEATURE
	init_md_section_level(KR_MD3);
	#endif
}
#endif

static void config_ap_runtime_data(struct ccci_modem *md, struct ap_query_md_feature *ap_rt_data)
{
	ccif_write32(&ap_rt_data->head_pattern, 0, AP_FEATURE_QUERY_PATTERN);

	ccif_write32(&ap_rt_data->share_memory_support, 0, 1);

	ccif_write32(&ap_rt_data->ap_runtime_data_addr, 0, md->smem_layout.ccci_rt_smem_base_phy -
		     md->mem_layout.smem_offset_AP_to_MD);
	ccif_write32(&ap_rt_data->ap_runtime_data_size, 0, CCCI_SMEM_SIZE_RUNTIME_AP);

	ccif_write32(&ap_rt_data->md_runtime_data_addr, 0, md->smem_layout.ccci_rt_smem_base_phy +
		     CCCI_SMEM_SIZE_RUNTIME_AP - md->mem_layout.smem_offset_AP_to_MD);
	ccif_write32(&ap_rt_data->md_runtime_data_size, 0, CCCI_SMEM_SIZE_RUNTIME_MD);

	ccif_write32(&ap_rt_data->set_md_mpu_start_addr, 0, md->mem_layout.smem_region_phy -
		     md->mem_layout.smem_offset_AP_to_MD);
	ccif_write32(&ap_rt_data->set_md_mpu_total_size, 0, md->mem_layout.smem_region_size);

	ccif_write32(&ap_rt_data->tail_pattern, 0, AP_FEATURE_QUERY_PATTERN);
}

static int md_ccif_op_send_runtime_data(struct ccci_modem *md,
					unsigned int sbp_code)
{
	int packet_size = sizeof(struct ap_query_md_feature) + sizeof(struct ccci_header);
	struct ccci_header *ccci_h;
	struct ap_query_md_feature *ap_rt_data;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	int ret;

	ccci_h = (struct ccci_header *)&md_ctrl->ccif_sram_layout->up_header;
	ap_rt_data = (struct ap_query_md_feature *)&md_ctrl->ccif_sram_layout->ap_rt_data;

	CCCI_NOTICE_MSG(md->index, KERN, "new api for sending rt data, sbp_code %u\n", sbp_code);

	ccci_set_ap_region_protection(md);
	/*header */
	ccif_write32(&ccci_h->data[0], 0, 0x00);
	ccif_write32(&ccci_h->data[1], 0, packet_size);
	ccif_write32(&ccci_h->reserved, 0, MD_INIT_CHK_ID);
	/*ccif_write32(&ccci_h->channel,0,CCCI_CONTROL_TX); */
	/*as Runtime data always be the first packet we send on control channel */
	ccif_write32((u32 *) ccci_h + 2, 0, CCCI_CONTROL_TX);

	config_ap_runtime_data(md, ap_rt_data);

	dump_runtime_data(md, ap_rt_data);

#ifdef FEATURE_DBM_SUPPORT
	eccci_c2k_smem_sub_region_init(md);
#endif

	ret = md_ccif_send(md, H2D_SRAM);
	return ret;
}

static int md_ccif_op_force_assert(struct ccci_modem *md, MD_COMM_TYPE type)
{
	struct ccci_request *req = NULL;
	struct ccci_header *ccci_h;

	CCCI_INF_MSG(md->index, TAG, "force assert MD using %d\n", type);
	switch (type) {
	case CCCI_MESSAGE:
		req = ccci_alloc_req(OUT, sizeof(struct ccci_header), 1, 1);
		if (req) {
			req->policy = RECYCLE;
			ccci_h =
			    (struct ccci_header *)skb_put(req->skb,
							  sizeof(struct
								 ccci_header));
			ccci_h->data[0] = 0xFFFFFFFF;
			ccci_h->data[1] = 0x5A5A5A5A;
			/*ccci_h->channel = CCCI_FORCE_ASSERT_CH; */
			*(((u32 *) ccci_h) + 2) = CCCI_FORCE_ASSERT_CH;
			ccci_h->reserved = 0xA5A5A5A5;
			return md->ops->send_request(md, 0, req, NULL);	/*hardcode to queue 0 */
		}
		return -CCCI_ERR_ALLOCATE_MEMORY_FAIL;
	case CCIF_INTERRUPT:
		md_ccif_send(md, H2D_FORCE_MD_ASSERT);
		break;
	case CCIF_INTR_SEQ:
		md_ccif_send(md, AP_MD_SEQ_ERROR);
		break;
	};
	return 0;

}

static int md_ccif_dump_info(struct ccci_modem *md, MODEM_DUMP_FLAG flag,
			     void *buff, int length)
{
	if (flag & DUMP_FLAG_CCIF)
		md_ccif_dump("Dump CCIF SRAM\n", md);

	CCCI_INF_MSG(md->index, TAG, "dump MD1 exception memory start\n");
	ccci_mem_dump(md->index, md1_excp_smem_vir, md1_excp_smem__size);
	/*dump_c2k_boot_status(md);*/

	return 0;
}

static int md_ccif_ee_callback(struct ccci_modem *md, MODEM_EE_FLAG flag)
{
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	if (flag & EE_FLAG_ENABLE_WDT)
		enable_irq(md_ctrl->md_wdt_irq_id);

	if (flag & EE_FLAG_DISABLE_WDT)
		disable_irq_nosync(md_ctrl->md_wdt_irq_id);

	return 0;
}

static struct ccci_modem_ops md_ccif_ops = {
	.init = &md_ccif_op_init,
	.start = &md_ccif_op_start,
	.stop = &md_ccif_op_stop,
	.reset = &md_ccif_op_reset,
	.send_request = &md_ccif_op_send_request,
	.give_more = &md_ccif_op_give_more,
	.napi_poll = &md_ccif_op_napi_poll,
	.send_runtime_data = &md_ccif_op_send_runtime_data,
	.broadcast_state = &md_ccif_op_broadcast_state,
	.force_assert = &md_ccif_op_force_assert,
	.dump_info = &md_ccif_dump_info,
	.write_room = &md_ccif_op_write_room,
	.get_port_by_minor = &md_ccif_op_get_port_by_minor,
	.get_port_by_channel = &md_ccif_op_get_port_by_channel,
	.ee_callback = &md_ccif_ee_callback,
};

static void md_ccif_hw_init(struct ccci_modem *md)
{
	int idx, ret;
	struct md_ccif_ctrl *md_ctrl;
	struct md_hw_info *hw_info;

	md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	hw_info = md_ctrl->hw_info;

	/*Copy HW info */
	md_ctrl->ccif_ap_base = (void __iomem *)hw_info->ap_ccif_base;
	md_ctrl->ccif_md_base = (void __iomem *)hw_info->md_ccif_base;
	md_ctrl->ccif_irq_id = hw_info->ap_ccif_irq_id;
	md_ctrl->md_wdt_irq_id = hw_info->md_wdt_irq_id;
	md_ctrl->sram_size = hw_info->sram_size;

	md_ccif_io_remap_md_side_register(md);

	md_ctrl->ccif_sram_layout =
	    (struct ccif_sram_layout *)(md_ctrl->ccif_ap_base + APCCIF_CHDATA);

	/*request IRQ */
	ret =
	    request_irq(md_ctrl->md_wdt_irq_id, md_cd_wdt_isr,
			hw_info->md_wdt_irq_flags, "MD2_WDT", md);
	if (ret) {
		CCCI_ERR_MSG(md->index, TAG,
			     "request MD_WDT IRQ(%d) error %d\n",
			     md_ctrl->md_wdt_irq_id, ret);
		return;
	}
	disable_irq_nosync(md_ctrl->md_wdt_irq_id);	/*to balance the first start */
	ret =
	    request_irq(md_ctrl->ccif_irq_id, md_ccif_isr,
			hw_info->md_wdt_irq_flags, "CCIF1_AP", md);
	if (ret) {
		CCCI_ERR_MSG(md->index, TAG,
			     "request CCIF1_AP IRQ(%d) error %d\n",
			     md_ctrl->ccif_irq_id, ret);
		return;
	}

	/*init CCIF */
	ccif_write32(md_ctrl->ccif_ap_base, APCCIF_CON, 0x01);	/*arbitration */
	ccif_write32(md_ctrl->ccif_ap_base, APCCIF_ACK, 0xFFFF);
	for (idx = 0; idx < md_ctrl->sram_size / sizeof(u32); idx++) {
		ccif_write32(md_ctrl->ccif_ap_base,
			     APCCIF_CHDATA + idx * sizeof(u32), 0);
	}

}

static int md_ccif_ring_buf_init(struct ccci_modem *md)
{
	int i = 0;
	unsigned char *buf;
	int bufsize = 0;
	struct md_ccif_ctrl *md_ctrl;
	struct ccci_ringbuf *ringbuf;

	md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	md_ctrl->total_smem_size = 0;
	/*CCIF_MD_SMEM_RESERVE; */
	buf =
	    ((unsigned char *)md->mem_layout.smem_region_vir) +
	    CCCI_SMEM_SIZE_EXCEPTION + md->smem_layout.ccci_rt_smem_size;
	for (i = 0; i < QUEUE_NUM; i++) {
		bufsize =
		    CCCI_RINGBUF_CTL_LEN + rx_queue_buffer_size[i] +
		    tx_queue_buffer_size[i];
		if (md_ctrl->total_smem_size + bufsize >
		    md->mem_layout.smem_region_size -
		    md->smem_layout.ccci_exp_smem_size) {
			CCCI_ERR_MSG(md->index, TAG,
				     "share memory too small,please check configure,smem_size=%d, exception_smem=%d\n",
				     md->mem_layout.smem_region_size,
				     md->smem_layout.ccci_exp_smem_size);
			return -1;
		}
		ringbuf =
		    ccci_create_ringbuf(md->index, buf, bufsize,
					rx_queue_buffer_size[i],
					tx_queue_buffer_size[i]);
		if (ringbuf == NULL) {
			CCCI_ERR_MSG(md->index, TAG,
				     "ccci_create_ringbuf %d failed\n", i);
			return -1;
		}
		/*rx */
		md_ctrl->rxq[i].ringbuf = ringbuf;
		md_ctrl->rxq[i].ccif_ch = D2H_RINGQ0 + i;
		md_ctrl->rxq[i].worker =
		    alloc_workqueue("rx%d_worker",
				    WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1,
				    i);
		INIT_WORK(&md_ctrl->rxq[i].qwork, ccif_rx_work);
		/*tx */
		md_ctrl->txq[i].ringbuf = ringbuf;
		md_ctrl->txq[i].ccif_ch = H2D_RINGQ0 + i;
		buf += bufsize;
		md_ctrl->total_smem_size += bufsize;
	}
	md->smem_layout.ccci_ccif_smem_size = md_ctrl->total_smem_size;
	return 0;
}

static int md_ccif_probe(struct platform_device *dev)
{
	struct ccci_modem *md;
	struct md_ccif_ctrl *md_ctrl;
	int md_id;
	struct ccci_dev_cfg dev_cfg;
	int ret;
	struct md_hw_info *md_hw;

	/*Allocate modem hardware info structure memory */
	md_hw = kzalloc(sizeof(struct md_hw_info), GFP_KERNEL);
	if (md_hw == NULL) {
		CCCI_INF_MSG(-1, TAG, "md_ccif_probe:alloc md hw mem fail\n");
		return -1;
	}

	ret = md_ccif_get_modem_hw_info(dev, &dev_cfg, md_hw);
	if (ret != 0) {
		CCCI_INF_MSG(-1, TAG, "md_ccif_probe:get hw info fail(%d)\n",
			     ret);
		kfree(md_hw);
		md_hw = NULL;
		return -1;
	}

	/*Allocate md ctrl memory and do initialize */
	md = ccci_allocate_modem(sizeof(struct md_ccif_ctrl));
	if (md == NULL) {
		CCCI_INF_MSG(-1, TAG,
			     "md_ccif_probe:alloc modem ctrl mem fail\n");
		kfree(md_hw);
		md_hw = NULL;
		return -1;
	}

	md->index = md_id = dev_cfg.index;
	md->major = dev_cfg.major;
	md->minor_base = dev_cfg.minor_base;
	md->capability = dev_cfg.capability;
	md->plat_dev = dev;
	md->heart_beat_counter = 0;
	md->data_usb_bypass = 0;
	CCCI_INF_MSG(md_id, TAG, "modem ccif module probe...\n");
	/*init modem structure */
	md->ops = &md_ccif_ops;
	CCCI_INF_MSG(md_id, TAG, "md_ccif_probe:md_ccif=%p,md_ctrl=%p\n", md,
		     md->private_data);
	md_ctrl = (struct md_ccif_ctrl *)md->private_data;
	md_ctrl->hw_info = md_hw;
	snprintf(md_ctrl->wakelock_name, sizeof(md_ctrl->wakelock_name),
		 "md%d_ccif_trm", md_id + 1);
	wake_lock_init(&md_ctrl->trm_wake_lock, WAKE_LOCK_SUSPEND,
		       md_ctrl->wakelock_name);
	tasklet_init(&md_ctrl->ccif_irq_task, md_ccif_irq_tasklet,
		     (unsigned long)md);
	INIT_WORK(&md_ctrl->ccif_sram_work, md_ccif_sram_rx_work);
	md_ctrl->channel_id = 0;

	/*register modem */
	ccci_register_modem(md);

	md_ccif_hw_init(md);

	md_ccif_ring_buf_init(md);
	/*hoop up to device */
	dev->dev.platform_data = md;

	return 0;
}

int md_ccif_remove(struct platform_device *dev)
{
	return 0;
}

void md_ccif_shutdown(struct platform_device *dev)
{
}

int md_ccif_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

int md_ccif_resume(struct platform_device *dev)
{
	struct ccci_modem *md = (struct ccci_modem *)dev->dev.platform_data;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(-1, TAG, "md_ccif_resume,md=0x%p,md_ctrl=0x%p\n", md,
		     md_ctrl);
	ccif_write32(md_ctrl->ccif_ap_base, APCCIF_CON, 0x01);	/*arbitration */
	return 0;
}

int md_ccif_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);
	return md_ccif_suspend(pdev, PMSG_SUSPEND);
}

int md_ccif_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	BUG_ON(pdev == NULL);
	return md_ccif_resume(pdev);
}

int md_ccif_pm_restore_noirq(struct device *device)
{
	int ret = 0;
	struct ccci_modem *md = (struct ccci_modem *)device->platform_data;
	struct md_ccif_ctrl *md_ctrl = (struct md_ccif_ctrl *)md->private_data;

	CCCI_INF_MSG(-1, TAG, "md_ccif_ipoh_restore,md=0x%p,md_ctrl=0x%p\n", md,
		     md_ctrl);
	/*IPO-H */
	/*restore IRQ */
#ifdef FEATURE_PM_IPO_H
	irq_set_irq_type(md_ctrl->md_wdt_irq_id, IRQF_TRIGGER_FALLING);
#endif
	/*set flag for next md_start */
	md->config.setting |= MD_SETTING_RELOAD;
	md->config.setting |= MD_SETTING_FIRST_BOOT;
	return ret;
}

#ifdef CONFIG_PM
static const struct dev_pm_ops md_ccif_pm_ops = {
	.suspend = md_ccif_pm_suspend,
	.resume = md_ccif_pm_resume,
	.freeze = md_ccif_pm_suspend,
	.thaw = md_ccif_pm_resume,
	.poweroff = md_ccif_pm_suspend,
	.restore = md_ccif_pm_resume,
	.restore_noirq = md_ccif_pm_restore_noirq,
};
#endif

static struct platform_driver modem_ccif_driver = {
	.driver = {
		   .name = "ccif_modem",
#ifdef CONFIG_PM
		   .pm = &md_ccif_pm_ops,
#endif
		   },
	.probe = md_ccif_probe,
	.remove = md_ccif_remove,
	.shutdown = md_ccif_shutdown,
	.suspend = md_ccif_suspend,
	.resume = md_ccif_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id ccif_of_ids[] = {
	/*{.compatible = "mediatek,AP_CCIF1",},*/
	{.compatible = "mediatek,ap2c2k_ccif",},
	{}
};
#endif

static int __init md_ccif_init(void)
{
	int ret;

#ifdef CONFIG_OF
	modem_ccif_driver.driver.of_match_table = ccif_of_ids;
#endif

	ret = platform_driver_register(&modem_ccif_driver);
	if (ret) {
		CCCI_ERR_MSG(-1, TAG,
			     "CCIF modem platform driver register fail(%d)\n",
			     ret);
		return ret;
	}
	CCCI_INF_MSG(-1, TAG, "CCIF C2K modem platform driver register success\n");
	return 0;
}

module_init(md_ccif_init);

MODULE_AUTHOR("Yanbin Ren <Yanbin.Ren@mediatek.com>");
MODULE_DESCRIPTION("CCIF modem driver v0.1");
MODULE_LICENSE("GPL");
