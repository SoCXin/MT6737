#ifndef _SMI_DEBUG_H_
#define _SMI_DEBUG_H_


#define SMI_DBG_DISPSYS (smi_dbg_disp_mask)
#define SMI_DBG_VDEC (smi_dbg_vdec_mask)
#define SMI_DBG_IMGSYS (smi_dbg_imgsys_mask)
#define SMI_DBG_VENC (smi_dbg_venc_mask)
#define SMI_DBG_MJC (smi_dbg_mjc_mask)

#define SMI_DGB_LARB_SELECT(smi_dbg_larb, n) ((smi_dbg_larb) & (1<<n))

#ifndef CONFIG_MTK_SMI_EXT
#define smi_debug_bus_hanging_detect(larbs, show_dump) {}
#define smi_debug_bus_hanging_detect_ext(larbs, show_dump, output_gce_buffer) {}
#else
int smi_debug_bus_hanging_detect(unsigned int larbs, int show_dump);
    /* output_gce_buffer = 1, pass log to CMDQ error dumping messages */
int smi_debug_bus_hanging_detect_ext(unsigned int larbs, int show_dump, int output_gce_buffer);

#endif
void smi_dumpCommonDebugMsg(int output_gce_buffer);
void smi_dumpLarbDebugMsg(unsigned int u4Index, int output_gce_buffer);
void smi_dumpDebugMsg(void);

extern int smi_larb_clock_is_on(unsigned int larb_index);

extern unsigned int smi_dbg_disp_mask;
extern unsigned int smi_dbg_vdec_mask;
extern unsigned int smi_dbg_imgsys_mask;
extern unsigned int smi_dbg_venc_mask;
extern unsigned int smi_dbg_mjc_mask;

#endif				/* _SMI_DEBUG_H__ */
