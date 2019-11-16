#ifndef __LASTBUS_H__
#define __LASTBUS_H__

#define NUM_MASTER_PORT	3
#define NUM_SLAVE_PORT	4
#define get_bit_at(reg, pos) (((reg) >> (pos)) & 1)

#define BUS_MCU_M0	0x05A4
#define BUS_MCU_S1	0x05B0
#define BUS_MCU_M0_M	0x05C0
#define BUS_MCU_S1_M	0x05CC

#define NUM_MON	5
#define BUS_PERI_R0	0x0500
#define BUS_PERI_R1	0x0504
#define BUS_PERI_MON	0x0508


#endif /* end of __LASTBUS_H__ */

