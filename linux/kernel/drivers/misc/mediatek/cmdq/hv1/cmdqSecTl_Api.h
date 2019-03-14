#ifndef __CMDQSECTL_API_H__
#define __CMDQSECTL_API_H__

/*
**	this file define common type between cmdq normal world and secrue world
**	only type and micro is allowed
*/

/**
 * Command IDs for normal world(TLC or linux kernel) to Trustlet
 */
#define CMD_CMDQ_TL_SUBMIT_TASK	  1
#define CMD_CMDQ_TL_RES_RELEASE	  2	/* (not used)release resource in secure path per session */
#define CMD_CMDQ_TL_CANCEL_TASK	  3
#define CMD_CMDQ_TL_PATH_RES_ALLOCATE 4	/* create global resouce for secure path */
#define CMD_CMDQ_TL_PATH_RES_RELEASE  5	/* destroy globacl resource for secure path */
#define CMD_CMDQ_TL_INIT_SHARED_MEMORY 6	/* create shared memory in Normal and Secure world */
#define CMD_CMDQ_TL_SYNC_HANDLE_HDCP 7	/* send src & dst handle to Secure World to copy HDCP version */
#define CMD_CMDQ_TL_REGISTER_SECURE_IRQ 8	/* register secure irq */
#define CMD_CMDQ_TL_DUMP_SMI_LARB	9


#define CMD_CMDQ_TL_TEST_HELLO_TL	(4000)	/* entry cmdqSecTl, and do nothing */
#define CMD_CMDQ_TL_TEST_DUMMY	  (4001)	/* entry cmdqSecTl and cmdqSecDr, and do nothing */
#define CMD_CMDQ_TL_TEST_SMI_DUMP	(4002)
#define CMD_CMDQ_TL_TRAP_DR_INFINITELY (4004)
#define CMD_CMDQ_TL_DUMP (4005)

/*
**	for CMDQ_LOG_LEVEL
**	set log level to 0 to enable all logs
**	set log level to 3 to close all logs
*/
enum LOG_LEVEL {
	LOG_LEVEL_MSG = 0,
	LOG_LEVEL_LOG = 1,
	LOG_LEVEL_ERR = 2,
	LOG_LEVEL_MAX,
};

/**
 * Termination codes
 */
#define EXIT_ERROR				  ((uint32_t)(-1))


/**
 * Trustlet UUID:
 * filename of output bin is {TL_UUID}.tlbin
 */
/* #define TL_CMDQ_UUID { { 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } */
#define TZ_TA_CMDQ_NAME "CMDQ_TA"
#define TZ_TA_CMDQ_UUID "5c071864-505d-11e4-9e35-164230d1df67"

#endif				/* __CMDQSECTEST_API_H__ */
