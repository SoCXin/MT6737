#include <stdlib.h>
#include <typedefs.h>
#include <platform.h>
#include <dramc.h>
#include <emi_hw.h>
#include <emi.h>
//#include <project.h>
#define DLE_MAX 16
extern int RANK_CURR;
char *opt_dle_value = "0";
int dle_result[DLE_MAX];
static int global_dle_value;

#ifdef WRITE_LEVELING
extern unsigned int wrlevel_dqs_final_delay[4];
#endif

#ifdef  DQ_DQS_DQM_REMAPPING        
extern unsigned char Bit_DQO_Mapping[32];
extern unsigned char DQSO_Mapping[4];
extern unsigned char DQM_Mapping[4];
#endif

#define TEST_PASS (0)
#define DRAM_START DRAM_BASE
#define RANK_SIZE (0x20000000)
//#define NEW_RX_FORMAT
int dramc_dma_test(unsigned int start, unsigned int len, void *ext_arg){
    int err =  TEST_PASS;
    int check_result = (int)ext_arg;
    unsigned int data;
    int i;

    unsigned int *src_buffp1;
    unsigned int *dst_buffp1;;

    src_buffp1 = DRAM_BASE;
    dst_buffp1 = DRAM_BASE | 0x1000;
    
    for (i = 0 ; i < (len/sizeof(unsigned int)) ; i++) {
        *((volatile unsigned int *)dst_buffp1+i) = 0;
    }
    for (i = 0 ; i < (len/sizeof(unsigned int)) ; i++) {
        *((volatile unsigned int *)src_buffp1+i) = i;
    }
    //memset(dst_buffp1, 0, len);

    *((volatile unsigned int *)(CQ_DMA_BASE | 0x18)) = 0x00070000; //BURST_LEN:7-8,R/W
    *((volatile unsigned int *)(CQ_DMA_BASE | 0x1C)) = src_buffp1;
    *((volatile unsigned int *)(CQ_DMA_BASE | 0x20)) = dst_buffp1;
    *((volatile unsigned int *)(CQ_DMA_BASE | 0x24)) = len;
    *((volatile unsigned int *)(CQ_DMA_BASE | 0x08)) = 0x1; //start dma
    while(*((volatile unsigned int *)(CQ_DMA_BASE | 0x08)));

    for (i = 0 ; i < (len/sizeof(unsigned int)) ; i++) {
        if (*((volatile unsigned int *)dst_buffp1+i) != i) {
            //print("0x%p: 0x%x != 0x%x\n", (unsigned int *)dst_buffp1+i, *((unsigned int *)dst_buffp1+i), i);
            err = -1;
            break;
        }
    }

    if ((int)ext_arg == 0xFFFFFFFF)
    {
        return err;
    }
    return err;

}

/*in MT6589 test agent 1 only used in DLE calibration*/
int dramc_ta1(unsigned int start, unsigned int len, void *ext_arg){
    int err =  TEST_PASS;
    int check_result = (int)ext_arg;
    unsigned int data;
    /* set test patern length*/
    data = DRAMC_READ_REG(0x40);
    DRAMC_WRITE_REG((data & 0xFF000000) | len, 0x40);


    DRAMC_WRITE_SET((1 << 29) ,DRAMC_CONF2); //Test Agent1


    //dbg_print("0x3C:%x\n",DRAMC_READ_REG(0x3c));
    //dbg_print("0x40:%x\n",DRAMC_READ_REG(0x40));
    //dbg_print("DRAMC_CONF2:%x\n",DRAMC_READ_REG(DRAMC_CONF2));
    while(!(DRAMC_READ_REG(DRAMC_TESTRPT)&(1 << 10)));

    /*
     * NoteXXX: Need to wait for at least 400 ns
     *          After checking the simulation result,
     *          there will be a delay on DLE_CNT_OK/DM_CMP_ERR updates after getting DM_CMP_CPT.
     *          i.e; After getting the complete status, need to wait for a while before reading DLE_CNT_OK/DM_CMP_ERR in the TESTRPT register.
     */
    delay_a_while(400);
    //dbg_print("DRAMC_TESTRPT:%x\n",DRAMC_READ_REG(DRAMC_TESTRPT));
    if (check_result) {
        if (DRAMC_READ_REG(DRAMC_TESTRPT) & (1 << 14)) {
            err = -1;
        }
        /*} else if (!(DRAMC_READ_REG(DRAMC_TESTRPT) & (1 << 18))) {
            err = -1;
        }*/
    }

    DRAMC_WRITE_CLEAR((1 << 29) ,DRAMC_CONF2); //disable test agent1


    //dbg_print("ext_arg:%x,err:%x\n",ext_arg,err);
    if ((int)ext_arg == 0xFFFFFFFF)
    {
        return err;
    }

    return;

}
char *dram_driving_tbl[] =
{
	/* DRAM I/O Driving */
	//"1",  /* 34.3	ohm:	0001 */
	//"2",  /* 40	ohm:	0010 */
	//"3",  /* 48	ohm:	0011 */
	"4",  /* 60	ohm:	0100 */
	//"5",  /* 68.6	ohm:	0101 */
	//"6",  /* 80	ohm:	0110 */
	//"7",  /* 120	ohm:	0111 */
	NULL,
};

void dram_driving_factor_handler(char *factor_value) {
    int curr_val = atoi(factor_value);

}



char *dle_tbl[] =
{
	/* DLE: 0x0~0xf */
        "0", "1", "2", "3",
        "4","5","6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
        /*"5","6", "7", "8", "9","10",*/
	NULL,
};

void dle_factor_handler(char *factor_value) {
    int curr_val = atoi(factor_value);

    DRAMC_WRITE_REG((DRAMC_READ_REG(DRAMC_DDR2CTL/* 0x7C */) & 0xFFFFFF8F)	/* Reserve original values for DRAMC_DDR2CTL[0:3, 7:31] */
		| ((curr_val & 0x7) << 4),			/* DATLAT: DRAMC_DDR2CTL[4:6], 3 bits */
		DRAMC_DDR2CTL/* 0x7C */);

    DRAMC_WRITE_REG((DRAMC_READ_REG(DRAMC_PADCTL4/* 0xE4 */) & 0xFFFFFFEF)	/* Reserve original values for DRAMC_DDR2CTL[0:3, 5:31] */
		| (((curr_val >> 3) & 0x1) << 4),			/* DATLAT3: DRAMC_PADCTL1[4], 1 bit */
		DRAMC_PADCTL4/* 0xE4 */);
}

void driving_factor_handler(char *factor_value);

#if defined(NEW_RX_FORMAT) /*MT6583*/
tuning_factor rx_tuning_factors[] = {
    /* {"<-DQ->|<-DQS-> Input Delay",		dqi_dqs_dly_tbl,		NULL, NULL, dqi_dqsi_dly_factor_handler},
    {"Tx I/O Driving (DRVP, DRVN)",     driving_tbl_for_rx, NULL, NULL, driving_factor_handler},
    */
    {"DRAM Driving Strength",			dram_driving_tbl,	NULL, NULL, dram_driving_factor_handler},
    {"DLE",					dle_tbl,		NULL, NULL, dle_factor_handler},
};
#else
tuning_factor rx_tuning_factors[] = {
   /*   {"DQ Input Delay",				dqi_dly_tbl,		NULL, NULL, dqi_dly_factor_handler},
    {"DQS Input Delay",				dqsi_dly_tbl,		NULL, NULL, dqsi_dly_factor_handler},
    {"DRAM Driving Strength",			dram_driving_tbl,	NULL, NULL, dram_driving_factor_handler},*/
    {"DLE",					dle_tbl,		NULL, NULL, dle_factor_handler},
 /*   {"Tx I/O Driving (DRVP, DRVN)",		driving_tbl_for_rx,	NULL, NULL, driving_factor_handler}, */
};
#endif


void dqso_dly_factor_handler(char *factor_value) 
{
    int curr_val = atoi(factor_value);
    curr_val = curr_val;

    DRAMC_WRITE_REG((DRAMC_READ_REG(DRAMC_PADCTL3/* 0x14 */) & 0xFFFF0000)	/* Reserve original values for __DRAMC_PADCTL3[16:31] */
		| ((curr_val & 0xF) << 0)					/* DQS0DLY: __DRAMC_PADCTL3[0:3],	4 bits */
		| ((curr_val & 0xF) << 4)					/* DQS1DLY: __DRAMC_PADCTL3[4:7],	4 bits */
		| ((curr_val & 0xF) << 8)					/* DQS2DLY: __DRAMC_PADCTL3[8:11],	4 bits */
		| ((curr_val & 0xF) << 12),				/* DQS3DLY: __DRAMC_PADCTL3[12:15],	4 bits */
		DRAMC_PADCTL3/* 0x14 */);
}

char *driving_tbl[] =
{
	/* Tx I/O Driving */
#if LPDDR == 2
        "8", "9", "10", "11", "12", 
#elif LPDDR == 3
        "6", "7","8", "9", "10",
        //"4", "5", "6", "7", "8", "9", "10", "11", "12", "15",
#else
        "6", "7", "8", "9", "10",

#endif
	NULL,
};





void ett_print_dram_driving(char *name, int curr_val) {
}


#if defined(NEW_RX_FORMAT) /*MT6583*/
tuning_factor dle_tuning_factors[] = 
{
    { .name = "DRAM Driving Strength",
      .factor_tbl = dram_driving_tbl,
      .curr_val = NULL, 
      .opt_val = NULL, 
      .factor_handler = dram_driving_factor_handler,
    },
    {
     .name = "DLE",
     .factor_tbl = dle_tbl,
      .curr_val = NULL, 
      .opt_val = NULL, 
      .factor_handler = dle_factor_handler,
    },
};
#else
tuning_factor dle_tuning_factors[] = 
{
    {
     .name = "DLE",
     .factor_tbl = dle_tbl,
      .curr_val = NULL, 
      .opt_val = NULL, 
      .factor_handler = dle_factor_handler,
    },
};
#endif

test_case dle_test_cases[] = 
{
    //{dramc_ta1, 0x0, 0x7FF, 0xFFFFFFFF}
    //{dramc_dma_test, 0x0, 0x80, 0xFFFFFFFF}
    {dramc_dma_test, 0x0, 0x100, 0xFFFFFFFF}
};


void ett_print_dle_banner(unsigned int n) 
{
    unsigned int i;

    if (n == 1)
    {
#ifdef RELEASE

        dbg_print("=============================================\n");
        dbg_print("(");
        for ( i = 2 ; i < ETT_TUNING_FACTOR_NUMS(dle_tuning_factors) ; i++)
        {
            dbg_print("%d ", atoi(dle_tuning_factors[i].curr_val));
        }
        dbg_print(")\n");
        dbg_print("=============================================\n");
#else

        dbg_print("=============================================\n");
        for ( i = 2 ; i < ETT_TUNING_FACTOR_NUMS(dle_tuning_factors) ; i++)
        {
            if (dle_tuning_factors[i].factor_tbl == dram_driving_tbl)
            {
                ett_print_dram_driving(dle_tuning_factors[i].name, atoi(dle_tuning_factors[i].curr_val));
            }
            else 
            {
                dbg_print("%s = %d\n", dle_tuning_factors[i].name, atoi(dle_tuning_factors[i].curr_val));
            }
        }

        dbg_print("X-axis: %s\n", dle_tuning_factors[0].name);
        dbg_print("Y-axis: %s\n", dle_tuning_factors[1].name);
        dbg_print("=============================================\n");
#ifdef NEW_RX_FORMAT
        dbg_print("    F  C   8   4   0   4   8   C   10  14  18  1C  20  24  28  2C  30  34  38  3C  40  44  48  4C  50  54  58  5C  60  64  68  6C  70  74  78  7C 7F\n");
        dbg_print("    <--*---*---*-->|<--*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*---*-->\n");
//        dbg_print("        F    E    D    C    B    A    9    8    7    6    5    4    3    2    1    0    4    8    C   10   14   18   1C   20   24   28   2C   30   34   38   3C   40   44   48   4C   50   54   58   5C   60   64   68   6C   70   74   78   7C\n");
//        dbg_print("    -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
#else
        dbg_print("        0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F\n");
        dbg_print("    --------------------------------------------------------------------------------\n");
#endif
#endif
    }
}

unsigned int ett_print_dle_result() 
{
    unsigned int i, score = 1;
    int result;

    for ( i = 0 ; i < ETT_TEST_CASE_NUMS(dle_test_cases) ; i++) {
        /* fixup the test start_address */
        dle_test_cases[i].start = DRAM_START; 
        if (dle_test_cases[i].test_case(dle_test_cases[i].start, dle_test_cases[i].range, dle_test_cases[i].ext_arg) == TEST_PASS) {
            if (i == (ETT_TEST_CASE_NUMS(dle_test_cases)-1)) {
#ifdef NEW_RX_FORMAT
                dbg_print("1");
#else
                dbg_print("    1");
#endif
            }
        dle_result[global_dle_value++] = score; 
        } else {
#ifdef NEW_RX_FORMAT
            dbg_print("0");
#else
            dbg_print("    0");
#endif
            score = 0;
            dle_result[global_dle_value++] = score; 
            break;
        }

    }

    return score;
}

void ett_print_dle_before_each_round_of_loop_zero() 
{
    DDR_PHY_RESET();
    //opt_dle_value = 0;
}

void ett_print_dle_after_each_round_of_loop_zero() 
{
    delay_a_while(200);
}

void ett_print_dle_before_start_loop_zero() 
{
#if !defined(RELEASE)
    dbg_print("%B:|", atoi(dle_tuning_factors[1].curr_val));
#endif
#if defined(NEW_RX_FORMAT)
    //dle_factor_handler("0");
#endif
}

void ett_print_dle_after_finish_loop_n(unsigned int n) 
{
    if (n == 0) {
        dbg_print("\n");
    }
}

void ett_calc_dle_opt_value(unsigned int n, unsigned int *score, unsigned int *high_score) 
{
    
}

 /*
     *     Related Registers (Latency between DRAMC and PHY)
     *             - PADCTL4  bit4        DATLAT3
     *             - DDR2CTL  bit 6~4  DATLAT
     *     Algorithm
     *             -  Set DLE from 4 to 14 and check if data is correct.>
     */

int do_dle_calib(void) {
    int ix;
    global_dle_value = 0;
    print_callbacks cbs = {
        .ett_print_banner = ett_print_dle_banner,
        .ett_print_before_start_loop_zero = ett_print_dle_before_start_loop_zero,
        .ett_print_before_each_round_of_loop_zero = ett_print_dle_before_each_round_of_loop_zero,
        .ett_print_result = ett_print_dle_result,
        .ett_print_after_each_round_of_loop_zero = ett_print_dle_after_each_round_of_loop_zero,
        .ett_calc_opt_value = ett_calc_dle_opt_value,
        .ett_print_after_finish_loop_n = ett_print_dle_after_finish_loop_n,
    };
/*
   * Main function
   *     - Create dle_result[]
   * Output
   *     -   |    0    0    1    1    1    1    0    0    0    0    0 >
   */

    if (ETT_TUNING_FACTOR_NUMS(dle_tuning_factors) > 0) {
        ett_recursive_factor_tuning(ETT_TUNING_FACTOR_NUMS(dle_tuning_factors)-1, dle_tuning_factors, &cbs);
    }
#if 1
#if 0
    for(ix = 0; ix < DLE_MAX; ix++){
        if (dle_result[ix] == 1 && dle_result[ix+1] == 1){
            if (*dle_tbl[ix+1] > *opt_dle_value)
                opt_dle_value = dle_tbl[ix+1];
            print("opt_dle value:%c\n",*opt_dle_value);
            break;
        }
    }
#else
    for(ix = 0; ix < (DLE_MAX-1); ix++){
        if (dle_result[ix] == 1 && dle_result[ix+1] == 1){
            if (atoi(dle_tbl[ix+1]) > atoi(opt_dle_value))
                opt_dle_value = dle_tbl[ix+1];
            print("opt_dle value:%c\n",*opt_dle_value);
            break;
        }
    }
#endif

    /* setup the opt dle value according to calibration result*/
#if 0
    if (*opt_dle_value!=0x30)
#else
    if (opt_dle_value!=dle_tbl[0])
#endif
    { //0x30 == "0"
        dle_factor_handler(opt_dle_value);
        return 0;
    }
    else
    {
        dbg_print("cannot find opt_dle value\n");
        return -1;
    }
#endif
}


#if defined(WRITE_LEVELING) || defined(fcCA_TRAINING)
// DRAMC registers
#define DRAMC_REG_ACTIM0 0x000
    #define POS_ACTIM0_BL2 15
#define DRAMC_REG_CONF1 0x004
    #define POS_CONF1_BL4 10
    #define POS_CONF1_SELFREF 26
    #define POS_CONF1_TESTLP 27    
#define DRAMC_REG_CONF2 0x008
    #define POS_CONF2_TEST1 29
    #define MASK_CONF2_TE12_ENABLE 0xe0000000
#define DRAMC_REG_PADCTL1 0x00c
#define DRAMC_REG_PADCTL2 0x010
#define DRAMC_REG_PADCTL3 0x014
#define DRAMC_REG_DELDLY1 0x018
#define DRAMC_REG_01C 0x01c
#define DRAMC_REG_DLLCONF 0x028
#define DRAMC_REG_TEST2_0 0x038   // new on MT5399
#define DRAMC_REG_TEST2_1 0x03C
#define DRAMC_REG_TEST2_2 0x040
#define DRAMC_REG_TEST2_3 0x044
    #define POS_TEST2_3_ADVREFEN 30
    #define POS_TEST2_3_TESTAUDPAT 7
    // bit0~3
    #define MASK_TEST2_3_TESTCNT 0x0000000f
    #define POS_TEST2_3_TESTCNT 0
#define DRAMC_REG_TEST2_4 0x048
    // bit0~4
    #define MASK_TEST2_4_TESTAUDINC 0x0000001f
    #define POS_TEST2_4_TESTAUDINC 0
    // bit8~12
    #define MASK_TEST2_4_TESTAUDINIT 0x00001f00
    #define POS_TEST2_4_TESTAUDINIT 8
    // bit 14, 15
    #define POS_TEST2_4_TESTAUDBITINV 14
    #define POS_TEST2_4_TESTAUDMODE 15
    //bit 16
    #define POS_TEST2_4_TESTXTALKPAT 16
#define DRAMC_REG_DDR2CTL 0x07c
    #define MASK_DDR2CTL_DATLAT 0x00000070
    #define POS_DDR2CTL_DTALAT 4
    #define POS_DDR2CTL_WOEN 3
#define DRAMC_REG_084 0x084
#define DRAMC_REG_MRS 0x088
#define DRAMC_REG_CLK1DELAY 0x08c
    #define POS_CLK1DELAY_DQCMD 3
#define DRAMC_REG_IOCTL 0x090
#define DRAMC_REG_DQSIEN 0x094
    #define MASK_DQSIEN_R0DQS0IEN 0x0000007f
    #define MASK_DQSIEN_R0DQS1IEN 0x00007f00
    #define MASK_DQSIEN_R0DQS2IEN 0x007f0000
    #define MASK_DQSIEN_R0DQS3IEN 0x7f000000
    #define POS_DQSIEN_R0DQS0IEN 0
    #define POS_DQSIEN_R0DQS1IEN 8
    #define POS_DQSIEN_R0DQS2IEN 16
    #define POS_DQSIEN_R0DQS3IEN 24
#define DRAMC_REG_IODRV4 0x0b0
    #define MASK_IODRV4_DQDRVN_2 0x00000f00
    #define MASK_IODRV4_DQDRVP_2 0x0000f000
    #define MASK_IODRV4_CMDDRVN_2 0x0f000000
    #define MASK_IODRV4_CMDDRVP_2 0xf0000000
    #define POS_IODRV4_DQDRVN_2 8
    #define POS_IODRV4_DQDRVP_2 12
    #define POS_IODRV4_CMDDRVN_2 24
    #define POS_IODRV4_CMDDRVP_2 28
#define DRAMC_REG_IODRV5 0x0b4
    #define MASK_IODRV5_DQCDRVN 0x00000f00
    #define MASK_IODRV5_DQCDRVP 0x0000f000
    #define MASK_IODRV5_DQCDRVN_2 0x0f000000
    #define MASK_IODRV5_DQCDRVP_2 0xf0000000
    #define POS_IODRV5_DQCDRVN 8
    #define POS_IODRV5_DQCDRVP 12
    #define POS_IODRV5_DQCDRVN_2 24
    #define POS_IODRV5_DQCDRVP_2 28
#define DRAMC_REG_DRVCTL0 0x0b8
    #define MASK_DRVCTL0_DQDRVN 0x00000f00
    #define MASK_DRVCTL0_DQDRVP 0x0000f000
    #define MASK_DRVCTL0_DQSDRVN 0x0f000000
    #define MASK_DRVCTL0_DQSDRVP 0xf0000000
    #define POS_DRVCTL0_DQDRVN 8
    #define POS_DRVCTL0_DQDRVP 12
    #define POS_DRVCTL0_DQSDRVN 24
    #define POS_DRVCTL0_DQSDRVP 28
#define DRAMC_REG_DRVCTL1 0x0bc
    #define MASK_DRVCTL1_CMDDRVN 0x00000f00
    #define MASK_DRVCTL1_CMDDRVP 0x0000f000
    #define MASK_DRVCTL1_CLKDRVN 0x0f000000
    #define MASK_DRVCTL1_CLKDRVP 0xf0000000
    #define POS_DRVCTL1_CMDDRVN 8
    #define POS_DRVCTL1_CMDDRVP 12
    #define POS_DRVCTL1_CLKDRVN 24
    #define POS_DRVCTL1_CLKDRVP 28
#define DRAMC_REG_MCKDLY 0x0d8
    //bit 23
    #define POS_MCKDLY_FIXODT 23
    //bit 12~15
    #define POS_MCKDLY_FIXDQIEN 12
    #define MASK_MCKDLY_FIXDQIEN 0x0000f000
#define DRAMC_REG_DQSCTL0 0x0dc
#define DRAMC_REG_DQSCTL1 0x0e0
    #define POS_DQSCTL1_DQSIENMODE 28
    #define MASK_DQSCTL1_DQSINCTL 0x0f000000   // for A60808, 4 bits totally
    #define POS_DQSCTL1_DQSINCTL 24
    #define POS_DQSCTL1_DQS2CTL 0   // [3:0] A60808: HW run-time DATLAT
    #define MASK_DQSCTL1_DQS2CTL 0x0000000f
#define DRAMC_REG_PADCTL4 0x0e4
    #define MASK_PADCTL4_DATLAT3 0x00000010
    #define POS_PADCTL4_DATLAT3 4
    #define POS_PADCTL4_CKEFIXON 2
#define DRAMC_REG_PHYCTL1 0x0f0
    #define POS_PHYCTL1_PHYRST 28
#define DRAMC_REG_GDDR3CTL1 0x0f4
    #define POS_GDDR3CTL1_DQMSWAP 31
    #define POS_GDDR3CTL1_RDATRST 25
    #define POS_GDDR3CTL1_BKSWAP 20
#define DRAMC_REG_PADCTL7 0x0f8
#define DRAMC_REG_MISCTL0 0x0fc
#define DRAMC_REG_OCDK 0x100
    #define POS_OCDK_DRVREF 24
#define DRAMC_REG_RKCFG 0x110
    #define POS_RKCFG_WDATKEY64 29   //32-bit channel it is 0, for 16-bit or asym DRAM it is 1
#define DRAMC_REG_CKPHDET 0x114
#define DRAMC_REG_DQ_DQS01_SEL 0x120
#define DRAMC_REG_DQSGCTL 0x124
    #define MASK_DQSGCTL_R0DQSG_COARSE_DLY_COM0 0x00000003
    #define MASK_DQSGCTL_R0DQSG_COARSE_DLY_COM1 0x00000030
    #define POS_DQSGCTL_R0DQSG_COARSE_DLY_COM0 0
    #define POS_DQSGCTL_R0DQSG_COARSE_DLY_COM1 4
    #define POS_DQSGCTL_DQSGDUALP 30
#define DRAMC_REG_CLKENCTL 0x130
#define DRAMC_REG_WRLEV 0x13c
    //bit 12
    #define POS_WRLEV_DQS_SEL 12
    #define MASK_WRLEV_DQS_SEL 0x0000f000
    //bit 8
    #define POS_WRLEV_DQS_WLEV 8
    //bit 1~4
    #define POS_WRLEV_DQS_Bx_G 1
    #define MASK_WRLEV_DQS_Bx_G 0x0000001e
    //bit 0
    #define POS_WRLEV_WRITE_LEVEL_EN 0
#define DRAMC_REG_158 0x158
#define DRAMC_REG_ARBCTL0 0x168
#define DRAMC_REG_CMDDLY0 0x1a8
#define DRAMC_REG_DQSCAL0 0x1c0
#define DRAMC_REG_IMPCAL 0x1c8
    #define POS_IMPCAL_IMPCALI_EN 0
    #define POS_IMPCAL_IMPCAL_HW 1
    #define POS_IMPCAL_IMP_CALI_ENN 4
    #define POS_IMPCAL_IMP_CALI_ENP 5
    #define POS_IMPCAL_IMPPDN 6
    #define POS_IMPCAL_IMPPDP 7    
    #define POS_IMPCAL_IMPDRVP 8
    #define MASK_IMPCAL_IMPDRVP 0x00000f00
    #define POS_IMPCAL_IMPDRVN 12
    #define MASK_IMPCAL_IMPDRVN 0x0000f000
#define DRAMC_REG_DMMonitor 0x1d8
#define DRAMC_REG_DRAMC_PD_CTRL 0x1dc
    #define MASK_DRAMC_PD_CTRL_REFCNT_FR_CLK 0x00ff0000
    #define POS_DRAMC_PD_CTRL_REFCNT_FR_CLK 16
    #define POS_DRAMC_PD_CTRL_MIOCKCTRLOFF 26
#define DRAMC_REG_LPDDR2 0x1e0
    #define POS_LPDDR2_ADRDECEN 31
    #define POS_LPDDR2_SELO1ASO 30
#define DRAMC_REG_SPCMD 0x1e4
    #define POS_SPCMD_MRWEN 0
    #define POS_SPCMD_DQSGCNTEN 8
    #define POS_SPCMD_DQSGCNTRST 9
#define DRAMC_REG_ACTIM1 0x1e8
#define DRAMC_REG_PERFCTL0 0x1ec
#define DRAMC_REG_LPDDR2_4 0x1f8
#define DRAMC_REG_DQODLY1 0x200
#define DRAMC_REG_DQODLY2 0x204
#define DRAMC_REG_DQODLY3 0x208
#define DRAMC_REG_DQODLY4 0x20c
#define DRAMC_REG_DQIDLY1 0x210
#define DRAMC_REG_DQIDLY2 0x214
#define DRAMC_REG_DQIDLY3 0x218
#define DRAMC_REG_DQIDLY4 0x21c
#define DRAMC_REG_DQIDLY5 0x220
#define DRAMC_REG_DQIDLY6 0x224
#define DRAMC_REG_DQIDLY7 0x228
#define DRAMC_REG_DQIDLY8 0x22c
#define DRAMC_REG_DMACON 0x234
    #define POS_DMACON_DMAEN 0
    #define POS_DMACON_DMAPUREWRITEEN 1
    #define POS_DMACON_DMAPUREREADEN 2
    #define POS_DMACON_DMABURSTLEN 4
    #define MASK_DMACON_DMABURSTLEN 0x00000070
#define DRAMC_REG_DMASRCADDR 0x238
#define DRAMC_REG_DMADSTADDR 0x23c
#define DRAMC_REG_DMATRANSLEN 0x240
    #define POS_DMATRANSLEN_DMATRANSLEN 0
    #define MASK_DMATRANSLEN_DMATRANSLEN 0x000fffff
#define DRAMC_REG_DMAPAT 0x244
#define DRAMC_REG_DMAMON 0x254
    #define POS_DMAMON_DMASTATUS 0
#define DRAMC_REG_DMACMPERR 0x258
#define DRAMC_REG_R2R_page_hit_counter 0x280
#define DRAMC_REG_STBENERR_R 0x320
#define DRAMC_REG_STBENERR_F 0x324
#define DRAMC_REG_TOGGLE_CNT 0x32c
#define DRAMC_REG_DQS_ERR_CNT 0x330
#define DRAMC_REG_DQ_ERR_CNT 0x334
#define DRAMC_REG_SELPH 0x344
#define DRAMC_REG_DCBLN 0x348
    #define POS_DCBLN_RSTBCNT_LATCH_EN 11
    #define POS_DCBLN_RX_MIOCK_JIT_EN 2
    #define POS_DCBLN_RX_EYE_SCAN_EN 1
    #define POS_DCBLN_REG_SW_RST 0
#define DRAMC_REG_TOGGLE_CNT_2 0x360
#define DRAMC_REG_DQS_ERR_CNT_2 0x364
#define DRAMC_REG_DQ_ERR_CNT_2 0x368
#define DRAMC_REG_CMP_ERR 0x370
#define DRAMC_REG_DQSDLY0 0x3a0
#define DRAMC_REG_SPCMDRESP 0x3b8
    #define POS_SPCMDRESP_SREF_STATE 16
#define DRAMC_REG_DQSGNWCNT0 0x3c0
#define DRAMC_REG_DQSGNWCNT1 0x3c4
#define DRAMC_REG_DLLSTATUS0 0x3dc
    #define POS_DLLSTATUS0_CMPOT 31
    #define POS_DLLSTATUS0_CMPOUTN 30
#define DRAMC_REG_TESTRPT 0x3fc
    #define POS_TESTRPT_DM_CMP_CPT 10
    #define POS_TESTRPT_DM_CMP_ERR 14
#define DRAMC_REG_SELPH1 0x400
#define DRAMC_REG_SELPH2 0x404
    #define MASK_SELPH2_TXDLY_DQSGATE 0x00007000
    #define POS_SELPH2_TXDLY_DQSGATE 12
    #define MASK_SELPH2_TXDLY_DQSGATE_P1 0x00700000
    #define POS_SELPH2_TXDLY_DQSGATE_P1 20
#define DRAMC_REG_SELPH3 0x408
#define DRAMC_REG_SELPH4 0x40C    
#define DRAMC_REG_SELPH5 0x410
    #define MASK_SELPH5_dly_DQSGATE 0x00c00000
    #define POS_SELPH5_dly_DQSGATE 22
    #define MASK_SELPH5_dly_DQSGATE_P1 0x03000000
    #define POS_SELPH5_dly_DQSGATE_P1 24
#define DRAMC_REG_SELPH6 0x414
#define DRAMC_REG_SELPH7 0x41c
#define DRAMC_REG_SELPH8 0x420
#define DRAMC_REG_SELPH9 0x424
#define DRAMC_REG_SELPH10 0x428
#define DRAMC_REG_SELPH11 0x42c
#define DRAMC_REG_SELPH12 0x430
#define DRAMC_REG_SELPH13 0x434
#define DRAMC_REG_SELPH14 0x438
#define DRAMC_REG_SELPH15 0x43c
#define DRAMC_REG_SELPH16 0x440
#define DRAMC_REG_COM0_DLY_SEL_DGTED 0x478
#define DRAMC_REG_COM0_DLY_SEL_DQS0 0x47C
#define DRAMC_REG_COM0_DLY_SEL_DQS1 0x480
#define DRAMC_REG_COM0_DLY_SEL_DQM0 0x484
#define DRAMC_REG_COM0_DLY_SEL_DQ2 0x494
#define DRAMC_REG_COM0_DLY_SEL_DQ3 0x498
#define DRAMC_REG_COM0_DLY_SEL_DQ4 0x49c
#define DRAMC_REG_COM0_DLY_SEL_DQ5 0x4a0
#define DRAMC_REG_COM0_DLY_SEL_DQ6 0x4a4
#define DRAMC_REG_COM0_DLY_SEL_DQ7 0x4a8
#define DRAMC_REG_COM1_DLY_SEL_DGTED 0x4CC
#define DRAMC_REG_COM1_DLY_SEL_DQS0 0x4D0
#define DRAMC_REG_COM1_DLY_SEL_DQS1 0x4D4
#define DRAMC_REG_COM1_DLY_SEL_DQM0 0x4D8
#define DRAMC_REG_COM1_DLY_SEL_DQ0 0x4e0
#define DRAMC_REG_COM1_DLY_SEL_DQ2 0x4e8
#define DRAMC_REG_COM1_DLY_SEL_DQ3 0x4ec
#define DRAMC_REG_COM1_DLY_SEL_DQ4 0x4f0
#define DRAMC_REG_COM1_DLY_SEL_DQ5 0x4f4
#define DRAMC_REG_COM1_DLY_SEL_DQ6 0x4f8
#define DRAMC_REG_COM1_DLY_SEL_DQ7 0x4fc
#define DRAMC_REG_PH_EN 0x520
#define DRAMC_REG_DLY_SEL_MCK_ANA 0x524
#define DRAMC_REG_DLY_SEL_CLKGEN 0x528
#define DRAMC_REG_DLY_SEL_CLKGEN2 0x52C
#define mcBITL(b)               (1L << (b))
//#define mcBIT(b)                (1 << (b))
#define mcBIT(b)                mcBITL(b)
#define mcMASK(w)               (mcBIT(w) - 1)
#define mcMASKS(w, b)           (mcMASK(w) << (b))
//#define mcCLR_BIT_C(a)          ((-1 - (Bit_#a)) & 0xFFFF)
#define mcCLR_BIT_C(a)          ((~Bit_##a) & 0xFFFF)

#define mcSET_MASK(a, b)        ((a) |= (b))
#define mcCLR_MASK(a, b)        ((a) &= (~(b)))
//#define mcCLR_SET_MASK(a, b, c)  ((a) = ((a) & (~(b))) | (c))
//          \_ same as mcCLR_MASK + mcSET_MASK at optimized code
#define mcSET_BIT(a, b)         mcSET_MASK(a, mcBIT(b))
#define mcCLR_BIT(a, b)         mcCLR_MASK(a, mcBIT(b))
#define mcCHK_BIT1(a, b)        ((a) & mcBIT(b))
#define mcCHK_BITM(a, b, m)     (((a) >> (b)) & (m))
#define mcCHK_BITS(a, b, w)     mcCHK_BITM(a, b, mcMASK(w))
//#define mcTEST_BIT(a, b)        mcCHK_BITM(a, b, 1)
#define mcTEST_BIT(a, b)        mcCHK_BIT1(a, b)
#define mcCHG_BIT(a, b)         ((a) ^= mcBIT(b))
#define mcSET_FIELD0(var, value, mask, pos) mcSET_MASK(var, ((value) << (pos)) & (mask))

#define mcSET_FIELD(var, value, mask, pos)  \
{                                           \
    mcCLR_MASK(var, mask);                  \
    mcSET_FIELD0(var, value, mask, pos);    \
}

#define mcGET_FIELD(var, mask, pos)     (((var) & (mask)) >> (pos))
#endif



#define mcSET_DRAMC_REG_ADDR(offset)    (offset)
#define DEFAULT_MR1_VALUE_DDR3 	0x00002000 	//1333Mbps
//#ifdef DDR_1333
//#define DEFAULT_MR2_VALUE_LP3		0x00080002 	//1333Mbps
//#else
//#define DEFAULT_MR2_VALUE_LP3		0x00060002 	//1066Mbps
//#endif


#define mcDELAY_US(Delay)  	delay_a_while(Delay*1000)
#define mcSHOW_DBG_MSG(_x_)     dbg_print _x_

#define MAX_TX_DQSDLY_TAPS 16
#define DQS_NUMBER 4
#define DQS_BIT_NUMBER 8

U8 ucDram_Register_Write(U32 u4reg_addr, U32 u4reg_value)
{
	U8 ucstatus = 0;
	DRAMC_WRITE_REG(u4reg_value, u4reg_addr); 
	return ucstatus;
}

U8 ucDram_Register_Read(U32 u4reg_addr, U32 *pu4reg_value)
{
	U8 ucstatus = 0;
	*pu4reg_value = DRAMC_READ_REG(u4reg_addr);
	return ucstatus;
}

#ifdef WRITE_LEVELING
#define fcWL_ALL   // Do Write Leveling with all DQS together
int DramcWriteLeveling(EMI_SETTINGS *emi_setting)
{
// Note that below procedure is based on "ODT off"
    unsigned int ucstatus = 0;
    unsigned int u4value, u4dq_o1;
    unsigned int /*u4prv_register_1dc, u4prv_register_044,*/ u4prv_register_0e4, u4prv_register_13c, u4prv_register_0d8, u4prv_register_008;
    unsigned int ucsample_status[DQS_NUMBER], ucdq_o1_perbyte[DQS_NUMBER], ucdq_o1_index[DQS_NUMBER];
    unsigned int ii, byte_i, ucsample_count;
    unsigned int fgwrlevel_done = 0;
    //unsigned int ucwrlevel_dqs_final_delay[DQS_NUMBER];
#ifdef DQ_DQS_DQM_REMAPPING
    unsigned int uiRemapByte;
    unsigned int uiRemapDQ;
    unsigned int uiRemapDQValue[4];
#endif

    fgwrlevel_done = 0;

    // this depends on pinmux
    // select first bit of each byte
    // dq_o1 is @ DDRPHY
    ucdq_o1_index[0]=11;
    ucdq_o1_index[1]=4;
    ucdq_o1_index[2]=25;
    ucdq_o1_index[3]=28;

    // backup mode settings
    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), &u4prv_register_1dc);
    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), &u4prv_register_044);
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL4), &u4prv_register_0e4);
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), &u4prv_register_13c);
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), &u4prv_register_0d8);
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0008), &u4prv_register_008);

    //write leveling mode initialization
    //Make CKE fixed at 1 (Put this before issuing MRS): CKEFIXON = 1 (0xe4[2])
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL4), &u4value);
    mcSET_BIT(u4value, POS_PADCTL4_CKEFIXON);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL4), u4value);

    //Disable auto refresh: REFCNT_FR_CLK = 0 (0x1dc[23:16]), ADVREFEN = 0 (0x44[30])
    // After Denali-1, only need to set DISREF (0x0008[28]=1)
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0008), &u4value);
    //mcCLR_MASK(u4value, 0x000000ff);
    mcSET_BIT(u4value, 28);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0008), u4value);
    
    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), &u4value);
    //mcCLR_MASK(u4value, MASK_DRAMC_PD_CTRL_REFCNT_FR_CLK);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), u4value);

    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), &u4value);
    //mcCLR_BIT(u4value, POS_TEST2_3_ADVREFEN);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), u4value);
    
/*
    //Enable Write ODT: WOEN = 1 (0x7c[3])
    //may no need to set here, initial value
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DDR2CTL), &u4value);
    mcSET_BIT(u4value, POS_DDR2CTL_WOEN);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DDR2CTL), u4value);

    //ODT, DQIEN fixed at 1; FIXODT = 1 (0xd8[23]), FIXDQIEN = 1111 (0xd8[15:12])
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), &u4value);
    mcSET_BIT(u4value, POS_MCKDLY_FIXODT);
    mcSET_MASK(u4value, MASK_MCKDLY_FIXDQIEN);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), u4value);
*/
    //FIXDQIEN = 1111 (0xd8[15:12])
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), &u4value);
    mcSET_MASK(u4value, MASK_MCKDLY_FIXDQIEN);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), u4value);

    //Enable DQ_O1, SELO1ASO=1 (0x1e0[30]=1)
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), &u4value);
    mcSET_BIT(u4value, POS_LPDDR2_SELO1ASO);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), u4value);

    // enable DDR write leveling mode
//#if LPDDR == 4    
    if ((emi_setting->type & 0xF) == TYPE_LPDDR4) {
        //issue MR1[7] to enable write leveling
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRS), (emi_setting->LPDDR3_MODE_REG_2 | 0x00000080));
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), &u4value);
        mcSET_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);
        mcDELAY_US(1);
        mcCLR_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);
    } else if ((emi_setting->type & 0xF) == TYPE_LPDDR3) {
//#elif LPDDR == 3
        // issue MR2[7] to enable write leveling (refer to DEFAULT MR2 value)
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRS), (emi_setting->LPDDR3_MODE_REG_2 | 0x00800000));
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), &u4value);
        mcSET_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);        
        mcDELAY_US(1);
        mcCLR_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);        
    }
//#endif

    //wait tWLDQSEN (25 nCK / 25ns) after enabling write leveling mode (DDR3 / LPDDDR3)
    mcDELAY_US(1);

    //Set {R_DQS_B3_G R_DQS_B2_G R_DQS_B1_G R_DQS_B0_G}=1010: 0x13c[4:1] (this depends on sel_ph setting)
    //Enable Write leveling: 0x13c[0]
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), &u4value);
    mcSET_FIELD(u4value, 0xa, MASK_WRLEV_DQS_Bx_G, POS_WRLEV_DQS_Bx_G);
    mcSET_BIT(u4value, POS_WRLEV_WRITE_LEVEL_EN);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), u4value);

    // wait tWLMRD (40 nCL / 40 ns) before DQS pulse (DDR3 / LPDDR3)
    mcDELAY_US(1);    

    //Proceed write leveling...
    //Initilize sw parameters
    for (ii=0; ii < (32/DQS_BIT_NUMBER); ii++)
    {
        ucsample_status[ii] = 0;
        //ucwrlevel_dqs_final_delay[ii] = 0;
	wrlevel_dqs_final_delay[ii] = 0;
    }
    //used for early break
    ucsample_count = 0;

    mcSHOW_DBG_MSG(("===============================================================================\n"));
    mcSHOW_DBG_MSG(("\n        dramc_write_leveling_swcal\n"));
    mcSHOW_DBG_MSG(("===============================================================================\n"));
    mcSHOW_DBG_MSG(("delay  byte0  byte1  byte2  byte3\n"));
    mcSHOW_DBG_MSG(("-----------------------------\n"));

    //not sure LP3 can be WL together 
#ifndef fcWL_ALL
    for (byte_i = 0; byte_i < (32/DQS_BIT_NUMBER);  byte_i++)
#endif
    {
    #ifndef fcWL_ALL
        // select respective DQS
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), &u4value);
        mcSET_FIELD(u4value, ((U8)1<<byte_i), MASK_WRLEV_DQS_SEL, POS_WRLEV_DQS_SEL);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), u4value);
    #else
        // select all DQS (0x13c[15:12]=1111)
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), &u4value);
        mcSET_FIELD(u4value, 0xf, MASK_WRLEV_DQS_SEL, POS_WRLEV_DQS_SEL);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), u4value);
    #endif
    
        for (ii=0; ii<MAX_TX_DQSDLY_TAPS; ii++)
        {
        #ifndef fcWL_ALL
            // set DQS delay
            ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL3), &u4value); 
            mcSET_FIELD(u4value, ii, ((U32)0xf)<<(4*byte_i), 4*byte_i);       
            ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL3), u4value);
        #else
            u4value = 0;
            for (byte_i= 0; byte_i < (32/DQS_BIT_NUMBER); byte_i++)
            {
                u4value += (((U32)ii)<<(4*byte_i));        
            }
            ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL3), u4value);
        #endif            
            
            //Trigger DQS pulse, R_DQS_WLEV: 0x13c[8] from 1 to 0
            ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), &u4value);
            mcSET_BIT(u4value, POS_WRLEV_DQS_WLEV);
            ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), u4value);
            mcCLR_BIT(u4value, POS_WRLEV_DQS_WLEV);
            ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), u4value);

            //wait tWLO (7.5ns / 20ns) before output (DDR3 / LPDDR3)
            mcDELAY_US(1);

            //Read DQ_O1 from register, 0x380
            ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x380), &u4dq_o1);
            //mcSHOW_DBG_MSG(("loop: %d DQ_O1: 0x%x \n", ii, u4dq_o1));

        #ifdef fcWL_ALL
            mcSHOW_DBG_MSG(("%d    ", ii));
            for (byte_i = 0; byte_i < (32/DQS_BIT_NUMBER);  byte_i++)
        #endif
            {
                ucdq_o1_perbyte[byte_i] = (U8)((u4dq_o1>>ucdq_o1_index[byte_i])&0x00000001);        

            #ifndef fcWL_ALL
                mcSHOW_DBG_MSG(("%d    %d\n", ii,  ucdq_o1_perbyte[byte_i]));
            #else
                mcSHOW_DBG_MSG(("%d    ", ucdq_o1_perbyte[byte_i]));
            #endif

                //sample from 0 to 1        
                if ((ucsample_status[byte_i]==0) && (ucdq_o1_perbyte[byte_i]==1))
                {
                    ucsample_status[byte_i] = 2;
                    //record delay value
                    //ucwrlevel_dqs_final_delay[byte_i] = 0;
                    wrlevel_dqs_final_delay[byte_i] = 0;
                #ifndef fcWL_ALL
                    // to the next byte
                    //break;
                #else
                    //used for early break
                    ucsample_count++;
                #endif
                }
                else if ((ucsample_status[byte_i]==0) && (ucdq_o1_perbyte[byte_i]==0))
                {
                    ucsample_status[byte_i] = 1;
                }
                else if ((ucsample_status[byte_i]==1) && (ucdq_o1_perbyte[byte_i]==1))
                {
                    ucsample_status[byte_i] = 2;
                    //record delay value
                    //ucwrlevel_dqs_final_delay[byte_i] = ii;
                    wrlevel_dqs_final_delay[byte_i] = ii;
                #ifndef fcWL_ALL
                    // to the next byte
                    //break;
                #else
                    //used for early break
                    ucsample_count++;
                #endif
                }  
            }
        #ifdef fcWL_ALL
            mcSHOW_DBG_MSG(("\n"));
            //early break, may be marked for debug use
            if (ucsample_count==(32/DQS_BIT_NUMBER))
            {
                fgwrlevel_done = 1;
                //break;
            }   
        #endif
        }
    }

#ifdef fcWL_ALL
    mcSHOW_DBG_MSG(("pass bytecount = %d\n", ucsample_count));
#endif
    mcSHOW_DBG_MSG(("byte_i    status    best delay\n"));
    for (byte_i = 0; byte_i < (32/DQS_BIT_NUMBER);  byte_i++)
    {
        //mcSHOW_DBG_MSG(("%d    %d    %d\n", byte_i, ucsample_status[byte_i], ucwrlevel_dqs_final_delay[byte_i]));
        mcSHOW_DBG_MSG(("%d    %d    %d\n", byte_i, ucsample_status[byte_i], wrlevel_dqs_final_delay[byte_i]));
    }    
    mcSHOW_DBG_MSG(("========================================\n"));

    // write leveling done, mode settings recovery if necessary
    // recover mode registers
//#if LPDDR == 4
    if ((emi_setting->type & 0xF) == TYPE_LPDDR4) {
        //issue MR1[7] to enable write leveling
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRS), emi_setting->LPDDR3_MODE_REG_2);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), &u4value);
        mcSET_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);
        mcDELAY_US(1);
        mcCLR_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);
//#elif LPDDR == 3
    } else if ((emi_setting->type & 0xF) == TYPE_LPDDR3) {
        // issue MR2[7] to enable write leveling (refer to DEFAULT MR2 value)
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MRS), emi_setting->LPDDR3_MODE_REG_2);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), &u4value);
        mcSET_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);
        mcDELAY_US(1);
        mcCLR_BIT(u4value, POS_SPCMD_MRWEN);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMD), u4value);
    }
//#endif

    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_WRLEV), u4prv_register_13c);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), u4prv_register_1dc);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), u4prv_register_044);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL4), u4prv_register_0e4);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), u4prv_register_0d8);    
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0008), u4prv_register_008); 
    
    //Disable DQ_O1, SELO1ASO=0 for power saving
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), &u4value);
    mcCLR_BIT(u4value, POS_LPDDR2_SELO1ASO);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), u4value);

    // set to best values
#ifdef DQ_DQS_DQM_REMAPPING
    // DQS
    u4value = 0;
    for (byte_i= 0; byte_i < (32/DQS_BIT_NUMBER); byte_i++)
    {
        uiRemapByte = DQSO_Mapping[byte_i];
        //u4value += (((U32)ucwrlevel_dqs_final_delay[byte_i])<<(4*byte_i));
        u4value += (((U32)wrlevel_dqs_final_delay[uiRemapByte])<<(4*uiRemapByte)); 
    }
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL3), u4value);
    
    // DQM
    u4value = 0;
    for (byte_i= 0; byte_i < (32/DQS_BIT_NUMBER); byte_i++)
    {
        uiRemapByte = DQM_Mapping[byte_i];
        //u4value += (((U32)ucwrlevel_dqs_final_delay[byte_i])<<(4*byte_i));
        u4value += (((U32)wrlevel_dqs_final_delay[uiRemapByte])<<(4*uiRemapByte)); 
    }
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL2), u4value);

    mcSHOW_DBG_MSG(("[write leveling]DQS: 0x%x, DQM: 0x%x\n", u4value, u4value));
#else
     // DQS
    u4value = 0;
    for (byte_i= 0; byte_i < (32/DQS_BIT_NUMBER); byte_i++)
    {
        //u4value += (((U32)ucwrlevel_dqs_final_delay[byte_i])<<(4*byte_i));
        u4value += (((U32)wrlevel_dqs_final_delay[byte_i])<<(4*byte_i)); 
    }
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL3), u4value);
    
    // DQM
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_PADCTL2), u4value);

    mcSHOW_DBG_MSG(("[write leveling]DQS: 0x%x, DQM: 0x%x\n", u4value, u4value));
#endif

    // DQ delay, each DQ has 4 bits. Each register contains 8-bit DQ's
#ifdef DQ_DQS_DQM_REMAPPING
    for (byte_i = 0 ; byte_i < 4; byte_i ++)
    {
         uiRemapDQValue[byte_i]=0;
    }
    
    for (byte_i = 0; byte_i < 4; byte_i++)
    {    
        uiRemapByte = DQSO_Mapping[byte_i];
        for (ii = 0; ii < DQS_BIT_NUMBER; ii++)
        {         
            uiRemapDQ = Bit_DQO_Mapping[(byte_i<<3)+ii];
            uiRemapDQValue[uiRemapDQ>>3] |= (((U32) wrlevel_dqs_final_delay[uiRemapByte]) << (4*(uiRemapDQ & 0x07)));            
        }        
    }   

    for (byte_i = 0; byte_i < 4; byte_i++)
    {
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DQODLY1+4*byte_i), uiRemapDQValue[byte_i]);
        mcSHOW_DBG_MSG(("[write leveling after remap]DQ byte%d reg: 0x%x val: 0x%x\n", byte_i, mcSET_DRAMC_REG_ADDR(DRAMC_REG_DQODLY1+4*byte_i), uiRemapDQValue[byte_i]));
    }
#else
    for (byte_i = 0; byte_i < (32/DQS_BIT_NUMBER); byte_i++)
    {
        u4value = 0;
        for (ii = 0; ii < DQS_BIT_NUMBER; ii++)
        {
            //u4value += (((U32) ucwrlevel_dqs_final_delay[byte_i]) << (4*ii));
            u4value += (((U32) wrlevel_dqs_final_delay[byte_i]) << (4*ii));
        }
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_DQODLY1+4*byte_i), u4value);
        mcSHOW_DBG_MSG(("[write leveling]DQ byte%d reg: 0x%x val: 0x%x\n", byte_i, mcSET_DRAMC_REG_ADDR(DRAMC_REG_DQODLY1+4*byte_i), u4value));
    }
#endif

    if (ucstatus)
    {
        mcSHOW_DBG_MSG(("register access fail!\n"));
        return 1;
    }
    else
    {
        return 0;
    }

    // log example
/*
===================================================================

                dramc_write_leveling_swcal
                apply=1 channel=2(2:cha, 3:chb)
===================================================================
delay  byte0  byte1  byte2  byte3
-----------------------------
  0    0    0    0    1
  1    0    0    0    1
  2    0    0    1    1
  3    0    0    1    1
  4    0    0    1    1
  5    0    0    1    1
  6    0    0    1    1
  7    0    0    1    1
  8    0    0    1    1
  9    0    0    1    1
 10    0    0    1    1
 11    1    1    1    1
pass bytecount = 4
byte_i    status    best delay
0         2         11
1         2         11
2         2         2
3         2         0
*/
    return 0;
}

#endif

#ifdef fcCA_TRAINING
// LPDDR DQ -> PHY DQ mapping
const U32 uiLPDDR_PHY_Mapping_table[32] = {4, 7, 13, 5, 12, 6, 14, 15, 28, 29, 31, 23, 22, 20, 30, 21,
	                                                                           11, 0, 8, 1, 10, 9, 2, 3, 25, 19, 16, 26, 17, 24, 18, 27};
//-------------------------------------------------------------------------
/** DramcCATraining
 *  start the calibrate the skew between Clk pin and CAx pins.
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL 
 */
//-------------------------------------------------------------------------
#define MAX_CLKO_DELAY         15
#define CATRAINING_NUM        10
#define CATRAINING_STEP 0
#define CKE_CS_DLY_SETTING

void DramcEnterSR(U8 op)
{
U8 ucstatus = 0;
U32 uiTemp;

    if (op == 1) // enter self refresh
    {
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), &uiTemp);
        mcSET_BIT(uiTemp, POS_CONF1_SELFREF);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), uiTemp);
        mcDELAY_US(2);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        while ( (mcTEST_BIT(uiTemp, POS_SPCMDRESP_SREF_STATE))==0)
        {
            mcSHOW_DBG_MSG(("Still not enter self refresh...\n"));
    	    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        }
    }
    else // exit self refresh
    {
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), &uiTemp);
        mcCLR_BIT(uiTemp, POS_CONF1_SELFREF);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_CONF1), uiTemp);
        mcDELAY_US(2);
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        while ( (mcTEST_BIT(uiTemp, POS_SPCMDRESP_SREF_STATE))!=0)
        {
            mcSHOW_DBG_MSG(("Still not exit self refresh...\n"));
    	    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP), &uiTemp);
        }
    }

    // tREFI/4 may cause self refresh fail. Set to tREFI manual first. After SF, switch back.
//    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x114), uiReg114h);         
}

int DramcCATraining(EMI_SETTINGS *emi_setting)
{
    U8 ucstatus = 0;
    U32 uiTemp, uiDelay, uiFinishCount, uiCA, uiMR41=1, uiReg1DCh, uiReg1E0h, uiRisingEdge, uiFallingEdge;
    U32 /*u4prv_register_0fc, u4prv_register_044, u4prv_register_63c, */u4prv_register_008;
    
    S8 iCenter[CATRAINING_NUM],  iFirstClkPass[CATRAINING_NUM], iLastClkPass[CATRAINING_NUM];
    S8 iFirstCAPass[CATRAINING_NUM], iLastCAPass[CATRAINING_NUM], iMaxCenter;
    S8 iCAShift[CATRAINING_NUM];
#ifdef CKE_CS_DLY_SETTING
    S8 CAShift_Avg = 0;
#endif
    S8 iBestFirstClkPass[CATRAINING_NUM], iBestLastClkPass[CATRAINING_NUM];
    S8 iBestFirstCAPass[CATRAINING_NUM], iBestLastCAPass[CATRAINING_NUM];
    S32 iPass, iClkWinSize, iCAWinSize;
    U32 *uiLPDDR_PHY_Mapping;
     
    // Disable clock gating to prevent DDRPHY enter idle.
    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x63c), &uiTemp);
    //u4prv_register_63c = uiTemp;
    //mcCLR_BIT(uiTemp, 2);
    //mcCLR_BIT(uiTemp, 1);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x63c), uiTemp);

    // Edward : no idea why TXP>1 will cause CA training fail. Now set it after CA training.
    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0fc), &u4prv_register_0fc);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0fc), (u4prv_register_0fc & 0x8fffffff));
      
    //disable auto refresh: REFCNT_FR_CLK = 0 (0x1dc[23:16]), ADVREFEN = 0 (0x44[30])
    //ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), &uiTemp);
    //u4prv_register_044 = uiTemp;
    //mcCLR_BIT(uiTemp, POS_TEST2_3_ADVREFEN);
    // ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), uiTemp);

    // After Denali-1, only need to set DISREF (0x0008[28]=1)
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0008), &uiTemp);
    u4prv_register_008 = uiTemp;
    mcSET_BIT(uiTemp, 28);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0008), uiTemp);

    uiLPDDR_PHY_Mapping = (U32 *)uiLPDDR_PHY_Mapping_table;
    

    for (uiCA=0; uiCA<CATRAINING_NUM; uiCA++) 
    {
        iLastClkPass[uiCA] = iLastCAPass[uiCA] = -1;
        iFirstClkPass[uiCA] = iFirstCAPass[uiCA] = -1;
        iBestLastClkPass[uiCA] = iBestLastCAPass[uiCA] = -1;
        iBestFirstClkPass[uiCA] = iBestFirstCAPass[uiCA] = -1;        
    }
    
    // Sweep clock output delay first.
       
    // Keep DQ input always ON.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0xd8), &uiTemp);
    uiTemp |= 0x0000f000;
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0xd8), uiTemp);

    // Let MIO_CK always ON.
    // Disable auto refresh: REFCNT_FR_CLK = 0 (0x1dc[23:16]) -> no need for Denali
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x1dc), &uiTemp);
    uiReg1DCh = uiTemp;
    mcSET_BIT(uiTemp, 26);
    //mcCLR_MASK(uiTemp, MASK_DRAMC_PD_CTRL_REFCNT_FR_CLK);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1dc), uiTemp);

#if 0
    //FIXDQIEN = 1111 (0xd8[15:12])
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), &uiTemp);
    uiRegD8h = uiTemp;
    mcSET_MASK(uiTemp, MASK_MCKDLY_FIXDQIEN);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), uiTemp);
#endif

    //Enable DQ_O1, SELO1ASO=1
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), &uiTemp);
    uiReg1E0h = uiTemp;
    mcSET_BIT(uiTemp, POS_LPDDR2_SELO1ASO);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), uiTemp);

    CA_TRAINING_BEGIN:

    // Set CA0~CA3, CA5~CA8 output delay to 0.
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1a8), 0);    // CA0~CA3
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1ac), 0);    // CA4~CA7
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1b0), 0);    // CA8~CA11
    
    // CS extent enable (need DRAM to support)
    // for testing

    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x80), &uiTemp);
    mcSET_BIT(uiTemp, 13);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x80), uiTemp);


    // Enter MR 41/MR48
    // Set MA & OP.
    if (uiMR41) 
    {
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x88), 0x00a40029);    
    } 
    else     
    {
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x88), 0x00c00030);    
    }
    // Hold the CA bus stable for at least one cycle.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x4c), &uiTemp);
    mcSET_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);
    // MRW
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x1e4), &uiTemp);
    mcSET_BIT(uiTemp, 0);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), uiTemp);
    mcDELAY_US(1);
    mcCLR_BIT(uiTemp, 0);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), uiTemp);
    // Disable CA bus stable.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x4c), &uiTemp);
    mcCLR_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);

    // Wait tCACKEL(10 tck) before CKE low
    mcDELAY_US(1);

    // CKE low
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0xe4), &uiTemp);
    mcSET_BIT(uiTemp, 3);
    mcCLR_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0xe4), uiTemp);
    
    // Set CA0~CA3, CA5~CA8 rising/falling golden value.
    if (uiMR41) 
    {
        //  01010101b -> 10101010b : Golden value = 1001100110011001b=0x9999
        //  11111111b -> 00000000b : Golden value = 0101010101010101b=0x5555
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x50), 0x55555555);
    }
    else
    {
        //  00010001b -> 00000000b : Golden value = 0000000100000001b=0x0101
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x50), 0x01010101);
    }

    // Wait tCAENT(10 tck) before pattern output
    mcDELAY_US(1);

    // Delay clock output delay to do CA training in order to get the pass window.
    uiFinishCount = 0;
    for (uiDelay=0; uiDelay<=MAX_CLKO_DELAY; uiDelay++) 
    {     
        //DramcEnterSelfRefresh(p, 1);           
        // Set Clk output delay
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0c), &uiTemp);
        mcSET_FIELD(uiTemp, uiDelay, 0x0f000000, 24);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0c), uiTemp);    
        //DramcEnterSelfRefresh(p, 0);
        
        // CA training pattern output enable
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x4c), &uiTemp);
        mcSET_BIT(uiTemp, 1);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);
        // delay 2 DRAM clock cycle
        mcDELAY_US(1);
        mcCLR_BIT(uiTemp, 1);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);    
        
        // Wait tADR(20ns) before CA sampled values available in DQ.
        mcDELAY_US(1);

        // Get DQ value.
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x380), &uiTemp);

        //mcSHOW_DBG_MSG2(("[CA Training] CLK delay -- %d, MR41 -- %d, DQ_O1 -- 0x%x\n", uiDelay, uiMR41, uiTemp));

        // Compare with golden value.
        if (uiMR41) 
        {
            for (uiCA=0; uiCA<CATRAINING_NUM; uiCA++) 
            {
                if ((uiCA==4) || (uiCA==9)) 
                {
                    continue;
                }
                if ( (iFirstClkPass[uiCA]==-1) || (iLastClkPass[uiCA]==-1)) 
                {
                    if (uiCA<4) 
                    {
                        uiRisingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[uiCA<<1]);
                        uiFallingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA<<1)+1]); 
                    }
                    else
                    {
                        uiRisingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA-1)<<1]);
                        uiFallingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[((uiCA-1)<<1)+1]); 
                    }
                    // Check 1->0 transition.
                    if ((uiRisingEdge!=0) && (uiFallingEdge==0)) 
                    {
                        iPass = 1;
                    } 
                    else
                    {
                        iPass = 0;
                    }


                    if (iFirstClkPass[uiCA]==-1)
                    {
                        if (iPass == 1) 
                        {
                            iFirstClkPass[uiCA] = uiDelay;
                        }
                    }
                    else
                    {
                        if (iLastClkPass[uiCA]==-1)
                        {                    
                            if (iPass == 0) 
                            {
                                iLastClkPass[uiCA] = uiDelay-1;
                                uiFinishCount++;
                            }
                            else
                            {
                                if (uiDelay==MAX_CLKO_DELAY)
                                {
                                    iLastClkPass[uiCA] = uiDelay;
                                    uiFinishCount++;
                                }
                            }
                            if (iLastClkPass[uiCA]!=-1)
                            {
                            	if ( (iLastClkPass[uiCA]-iFirstClkPass[uiCA]) > (iBestLastClkPass[uiCA]-iBestFirstClkPass[uiCA]))
                            	{
                            		iBestLastClkPass[uiCA] = iLastClkPass[uiCA];
                            		iBestFirstClkPass[uiCA] = iFirstClkPass[uiCA];
                            	}
                            	iLastClkPass[uiCA] = iFirstClkPass[uiCA] = -1;
                            }
                        }
                    }
                }
            }

            // Wait tCACD(22clk) before output CA pattern to DDR again..
            mcDELAY_US(1);        
            
            //if (uiFinishCount==8) {
            //    break;
            //} 
        }
        else
        {
            // MR 48 
            uiCA = 4;
            
MR48_CHECKRESULT_CLK:            
    
            if ((iFirstClkPass[uiCA]==-1) || (iLastClkPass[uiCA]==-1))
            {
                uiRisingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA==4) ? 0 : 8]);
                uiFallingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA==4) ? 1 : 9]); 

                // Check 1->0 transition.
                if ((uiRisingEdge!=0) && (uiFallingEdge==0)) 
                {
                    iPass = 1;
                }            
                else
                {
                    iPass = 0;
                }
                if (iFirstClkPass[uiCA]==-1)
                {
                    if (iPass==1)
                    {
                        iFirstClkPass[uiCA] = uiDelay;
                    }
                }
                else
                {
                    if (iLastClkPass[uiCA]==-1)
                    {
                        if (iPass==0)
                        {
                            iLastClkPass[uiCA] = uiDelay-1;
                            uiFinishCount++;
                        }
                        else
                        {
                            if (uiDelay==MAX_CLKO_DELAY)
                            {
                                iLastClkPass[uiCA] = uiDelay;
                                uiFinishCount++;                            
                            }
                        }
                        if (iLastClkPass[uiCA]!=-1)
                        {
                        	if ( (iLastClkPass[uiCA]-iFirstClkPass[uiCA]) > (iBestLastClkPass[uiCA]-iBestFirstClkPass[uiCA]))
                        	{
                        		iBestLastClkPass[uiCA] = iLastClkPass[uiCA];
                        		iBestFirstClkPass[uiCA] = iFirstClkPass[uiCA];
                        	}
                        	iLastClkPass[uiCA] = iFirstClkPass[uiCA] = -1;
                        }                        
                    }
                }
            }

            if (uiCA==4)
            {
                uiCA=9;
                goto MR48_CHECKRESULT_CLK;
            }
            
            // Wait tCACD(22clk) before output CA pattern to DDR again..
            mcDELAY_US(1);                    

            //if (uiFinishCount==2) {
            //    break;
            //} 
        }
    }

    //DramcEnterSelfRefresh(p, 1);    
    // Set Clk output delay to 0.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0c), &uiTemp);
    mcSET_FIELD(uiTemp, 0, 0x0f000000, 24);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0c), uiTemp);  
    //DramcEnterSelfRefresh(p, 0);

    // Delay CA output delay to do CA training in order to get the pass window.
    uiFinishCount = 0;
    for (uiDelay=0; uiDelay<=MAX_CLKO_DELAY; uiDelay++) 
    {
        // Set CA0~CA3, CA5~CA8 output delay.
        uiTemp = uiDelay | (uiDelay<<8) | (uiDelay<<16) | (uiDelay<<24);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1a8), uiTemp);    // CA0~CA3
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1ac), uiTemp);    // CA4~CA7
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1b0), uiTemp);    // CA8~CA11
        
        // CA training pattern output enable
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x4c), &uiTemp);
        mcSET_BIT(uiTemp, 1);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);
        // delay 2 DRAM clock cycle
        mcDELAY_US(1);
        mcCLR_BIT(uiTemp, 1);
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);    
        
        // Wait tADR(20ns) before CA sampled values available in DQ.
        mcDELAY_US(1);

        // Get DQ value.
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x380), &uiTemp);

        //mcSHOW_DBG_MSG2(("[CA Training] CA delay -- %d, MR41 -- %d, DQ_O1 -- 0x%x\n", uiDelay, uiMR41, uiTemp));

        // Compare with golden value.
        if (uiMR41) 
        {
            for (uiCA=0; uiCA<CATRAINING_NUM; uiCA++) 
            {
                if ((uiCA==4) || (uiCA==9)) 
                {
                    continue;
                }
                if ( (iFirstCAPass[uiCA]==-1) || (iLastCAPass[uiCA]==-1)) 
                {
                    if (uiCA<4) 
                    {
                        uiRisingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[uiCA<<1]);
                        uiFallingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA<<1)+1]); 
                    }
                    else
                    {
                        uiRisingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA-1)<<1]);
                        uiFallingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[((uiCA-1)<<1)+1]); 
                    }
                    // Check 1->0 transition.
                    if ((uiRisingEdge!=0) && (uiFallingEdge==0)) 
                    {
                        iPass = 1;
                    } 
                    else
                    {
                        iPass = 0;
                    }

                    if (iFirstCAPass[uiCA]==-1)
                    {
                        if (iPass == 1) 
                        {
                            iFirstCAPass[uiCA] = uiDelay;
                        }
                    }
                    else
                    {
                        if (iLastCAPass[uiCA]==-1)
                        {                    
                            if (iPass == 0) 
                            {
                                iLastCAPass[uiCA] = uiDelay-1;
                                uiFinishCount++;
                            }
                            else
                            {
                                if (uiDelay==MAX_CLKO_DELAY)
                                {
                                    iLastCAPass[uiCA] = uiDelay;
                                    uiFinishCount++;
                                }
                            }
                            if (iLastCAPass[uiCA]!=-1)
                            {
	                            if ( (iLastCAPass[uiCA]-iFirstCAPass[uiCA]) > (iBestLastCAPass[uiCA]-iBestFirstCAPass[uiCA]) )
	                            {
	                            	iBestLastCAPass[uiCA] = iLastCAPass[uiCA];
	                            	iBestFirstCAPass[uiCA] = iFirstCAPass[uiCA];
	                            }
	                            iLastCAPass[uiCA] = iFirstCAPass[uiCA] = -1;
                            }
                        }
                    }
                }
            }
        
            // Wait tCACD(22clk) before output CA pattern to DDR again..
            mcDELAY_US(1);        
            
            //if (uiFinishCount==8) {
            //    break;
            //} 
        }
        else
        {
            // MR 48
            uiCA = 4;
            
MR48_CHECKRESULT_CA:            
    
            if ((iFirstCAPass[uiCA]==-1) || (iLastCAPass[uiCA]==-1))
            {
                uiRisingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA==4) ? 0 : 8]);
                uiFallingEdge = uiTemp & (0x01 << uiLPDDR_PHY_Mapping[(uiCA==4) ? 1 : 9]); 

                // Check 1->0 transition.
                if ((uiRisingEdge!=0) && (uiFallingEdge==0)) 
                {
                    iPass = 1;
                }            
                else
                {
                    iPass = 0;
                }
        
                if (iFirstCAPass[uiCA]==-1)
                {
                    if (iPass==1)
                    {
                        iFirstCAPass[uiCA] = uiDelay;
                    }
                }
                else
                {
                    if (iLastCAPass[uiCA]==-1)
                    {
                        if (iPass==0)
                        {
                            iLastCAPass[uiCA] = uiDelay-1;
                            uiFinishCount++;
                        }
                        else
                        {
                            if (uiDelay==MAX_CLKO_DELAY)
                            {
                                iLastCAPass[uiCA] = uiDelay;
                                uiFinishCount++;                            
                            }
                        }
                        if (iLastCAPass[uiCA]!=-1)
                        {
                            if ( (iLastCAPass[uiCA]-iFirstCAPass[uiCA]) > (iBestLastCAPass[uiCA]-iBestFirstCAPass[uiCA]) )
                            {
                            	iBestLastCAPass[uiCA] = iLastCAPass[uiCA];
                            	iBestFirstCAPass[uiCA] = iFirstCAPass[uiCA];
                            }
                            iLastCAPass[uiCA] = iFirstCAPass[uiCA] = -1;
                        }                        
                    }
                }
            }

            if (uiCA==4)
            {
                uiCA=9;
                goto MR48_CHECKRESULT_CA;
            }

            // Wait tCACD(22clk) before output CA pattern to DDR again..
            mcDELAY_US(1);                    

            //if (uiFinishCount==2) {
            //    break;
            //}             
        }
    }    

    // CS extent disable
    // for testing
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x80), &uiTemp);
    mcCLR_BIT(uiTemp, 13);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x80), uiTemp);   

    if (uiMR41==0) 
    {
        // Disable fix DQ input enable.
        ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0xd8), &uiTemp);
        uiTemp = uiTemp & 0xffff0fff;
        ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0xd8), uiTemp);
    }

    // Wait tCACKEN (10ck)
    mcDELAY_US(1);        

    // CKE high
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0xe4), &uiTemp);
    mcCLR_BIT(uiTemp, 3);
    mcSET_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0xe4), uiTemp);

    if (uiMR41) 
    {
        uiMR41 = 0;
        goto CA_TRAINING_BEGIN;
    }

    // CS extent enable
    // for testing
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x80), &uiTemp);
    mcSET_BIT(uiTemp, 13);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x80), uiTemp); 

    // MR42 to leave CA training.
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x88), 0x00a8002a);    
    // Hold the CA bus stable for at least one cycle.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x4c), &uiTemp);
    mcSET_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);
    // MRW
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x1e4), &uiTemp);
    mcSET_BIT(uiTemp, 0);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), uiTemp);
    mcDELAY_US(1);
    mcCLR_BIT(uiTemp, 0);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1e4), uiTemp);    
    // Disable the hold the CA bus stable for at least one cycle.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x4c), &uiTemp);
    mcCLR_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x4c), uiTemp);

    // CS extent disable
    // for testing
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x80), &uiTemp);
    mcCLR_BIT(uiTemp, 13);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x80), uiTemp);   
    
    // Disable CKE high
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0xe4), &uiTemp);
    mcCLR_BIT(uiTemp, 2);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0xe4), uiTemp);

    // Calculate the middle range & max middle.
    mcSHOW_DBG_MSG(("=========================================\n"));
    mcSHOW_DBG_MSG(("x=Pass window CA(max~min) Clk(min~max) center. \n"));
    mcSHOW_DBG_MSG(("y=CA0~CA9\n"));
    mcSHOW_DBG_MSG(("=========================================\n"));

    iMaxCenter = 0 - MAX_CLKO_DELAY;
    for (uiCA=0; uiCA<CATRAINING_NUM; uiCA++)
    {
        iClkWinSize = iBestLastClkPass[uiCA] - iBestFirstClkPass[uiCA];
        iCAWinSize = iBestLastCAPass[uiCA] - iBestFirstCAPass[uiCA];
        if (iClkWinSize >= iCAWinSize)
        {
            if (iCAWinSize>0)
            {
                iCenter[uiCA] =  (iClkWinSize - iCAWinSize)>>1;
            }
            else
            {
                iCenter[uiCA] =  iBestFirstClkPass[uiCA] + (iClkWinSize>>1);
            }
        }
        else
        {
            if (iClkWinSize>0)
            {
                iCenter[uiCA] =  (iClkWinSize - iCAWinSize)/2;
            }
            else
            {
                iCenter[uiCA] =  0-(iBestFirstCAPass[uiCA] + (iCAWinSize>>1));
            }
        }
        dbg_print("CA%d	CA: [%d - %d]",uiCA,iBestLastCAPass[uiCA],iBestFirstCAPass[uiCA]);
        dbg_print("  Clk: [%d - %d] %d\n",iBestFirstClkPass[uiCA],iBestLastClkPass[uiCA],iCenter[uiCA]);

        if (iCenter[uiCA]  > iMaxCenter)
        {
            iMaxCenter = iCenter[uiCA];
        }
#ifdef EYE_SCAN
	EyeScanWin[uiCA].ucsetup_pass_number = iCAWinSize;
	EyeScanWin[uiCA].uchold_pass_number = iClkWinSize;
#endif

    #ifdef DDR_FT_LOAD_BOARD
        if ((iCAWinSize==0) && (iClkWinSize == 0))
        {
            LoadBoardShowResult(FLAG_CA_CALIBRATION, FLAG_CALIBRATION_FAIL, p->channel, FLAG_NOT_COMPLETE_OR_FAIL);            
            while(1);    
        }
        else if ((iCAWinSize+iClkWinSize)<=CA_TRAINING_BOUND)
        {
            LoadBoardShowResult(FLAG_CA_CALIBRATION, FLAG_WINDOW_TOO_SMALL, p->channel, FLAG_NOT_COMPLETE_OR_FAIL);            
            while(1);  
        }
    #endif
    }
    mcSHOW_DBG_MSG(("=========================================\n"));
    mcSHOW_DBG_MSG(("Max center = %d\n\n", iMaxCenter));
    
    // Calculate the shift value
    mcSHOW_DBG_MSG(("=========================================\n"));
    mcSHOW_DBG_MSG(("x=CA shift     Adjusted Pass window CA(max~min) Clk(min~max) center.\n"));
    mcSHOW_DBG_MSG(("y=CA0~CA9\n"));
    mcSHOW_DBG_MSG(("=========================================\n"));    
    
    if (iMaxCenter < 0)
    {
        // Clk output delay could not be negative. Need to adjust into 0.
        iMaxCenter = 0;
        mcSHOW_DBG_MSG(("Max center < 0. Adjust to 0. \n\n"));
    }

    for (uiCA=0; uiCA<CATRAINING_NUM; uiCA++)
    {
        iCAShift[uiCA] = iMaxCenter - iCenter[uiCA]+CATRAINING_STEP;
        if (iCAShift[uiCA]>=MAX_CLKO_DELAY)
        {
            iCAShift[uiCA] = MAX_CLKO_DELAY;
        }
#ifdef CKE_CS_DLY_SETTING
        CAShift_Avg += iCAShift[uiCA];
#endif  
        dbg_print("CA%d     Shift %d     CA(%d~", uiCA, iCAShift[uiCA], 
            iBestLastCAPass[uiCA]-iCAShift[uiCA]);
        dbg_print("%d)", iBestFirstCAPass[uiCA]);
        dbg_print("  Clk(%d~%d) %d\n", iBestFirstClkPass[uiCA], iBestLastClkPass[uiCA]+iCAShift[uiCA], iCenter[uiCA]+iCAShift[uiCA]);
    }
    mcSHOW_DBG_MSG(("=========================================\n"));    
    
    // Restore the registers' values.
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1dc), uiReg1DCh);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_LPDDR2), uiReg1E0h);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_MCKDLY), uiRegD8h);

    // Write shift value into CA output delay.
    uiTemp = iCAShift[0] + (iCAShift[1]<<8) + (iCAShift[2]<<16) + (iCAShift[3]<<24);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1a8), uiTemp);    // CA0~CA3
    mcSHOW_DBG_MSG(("Reg.1A8h=%xh\n", uiTemp));    
    uiTemp = iCAShift[4] + (iCAShift[5]<<8) + (iCAShift[6]<<16) + (iCAShift[7]<<24);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1ac), uiTemp);    // CA4~CA7
    mcSHOW_DBG_MSG(("Reg.1ACh=%xh\n", uiTemp));    
    uiTemp = iCAShift[8] + (iCAShift[9]<<8);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1b0), uiTemp);    // CA8~CA11
    mcSHOW_DBG_MSG(("Reg.1B0h=%xh\n", uiTemp));    
    
#ifdef CKE_CS_DLY_SETTING
    CAShift_Avg = (CAShift_Avg + (CATRAINING_NUM>>1)) /CATRAINING_NUM;
    // CKEDLY : Reg.1B8h[12:8].  CSDLY : Reg.1B8h[4:0]
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x1b8), &uiTemp);
    uiTemp = (uiTemp & 0xffffe0e0) | (CAShift_Avg << 8) | (CAShift_Avg << 0);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1b8), uiTemp);
    mcSHOW_DBG_MSG(("Reg.1B8h=%xh\n", uiTemp));    

    // CKE1DLY : Reg.1C4h[28:24]
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x1c4), &uiTemp);
    uiTemp = (uiTemp & 0xe0ffffff) | (CAShift_Avg << 24);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x1c4), uiTemp);
    mcSHOW_DBG_MSG(("Reg.1C4h=%xh\n", uiTemp));    

    // CS1DLY : Reg.0Ch[31:28]
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x00c), &uiTemp);
    uiTemp = (uiTemp & 0x0fffffff) | (CAShift_Avg << 28);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x00c), uiTemp);
    mcSHOW_DBG_MSG(("Reg.00ch=%xh\n", uiTemp));    
#endif    

    DramcEnterSR(1);    
    // Write max center value into Clk output delay.
    ucstatus |= ucDram_Register_Read(mcSET_DRAMC_REG_ADDR(0x0c), &uiTemp);
    mcSET_FIELD(uiTemp, iMaxCenter+CATRAINING_STEP, 0x0f000000, 24);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0c), uiTemp);    
    mcSHOW_DBG_MSG(("Reg.0Ch=%xh\n", uiTemp));        
    DramcEnterSR(0);

#ifdef WL_CLKADJUST
    CATrain_ClkDelay[p->channel] = iMaxCenter;
    mcSHOW_DBG_MSG3(("CATrain_ClkDelay=%d...\n", CATrain_ClkDelay[p->channel]));
#endif

    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0fc), u4prv_register_0fc);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(DRAMC_REG_TEST2_3), u4prv_register_044);
    //ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x63c), u4prv_register_63c);
    ucstatus |= ucDram_Register_Write(mcSET_DRAMC_REG_ADDR(0x0008), u4prv_register_008); 

    if (ucstatus)
    {
        mcSHOW_DBG_MSG(("register access fail!\n"));
        return 1;
    }
    else
    {
        return 0;
    }    
}
#endif
