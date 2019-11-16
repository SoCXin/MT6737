/*
 * Copyright (c) 2013 TRUSTONIC LIMITED
 * All rights reserved
 *
 * The present software is the confidential and proprietary information of
 * TRUSTONIC LIMITED. You shall not disclose the present software and shall
 * use it only in accordance with the terms of the license agreement you
 * entered into with TRUSTONIC LIMITED. This software may be subject to
 * export or import laws in certain countries.
 */

#ifndef TLSECMEM_H_
#define TLSECMEM_H_

#include "tci.h"

/*
 * Command ID's for communication Trustlet Connector -> Trustlet.
 */
#define CMD_SEC_MEM_ALLOC         1
#define CMD_SEC_MEM_REF           2
#define CMD_SEC_MEM_UNREF         3
#define CMD_SEC_MEM_ALLOC_TBL     4
#define CMD_SEC_MEM_UNREF_TBL     5
#define CMD_SEC_MEM_USAGE_DUMP    6
#define CMD_SEC_MEM_ENABLE        7
#define CMD_SEC_MEM_DISABLE       8
#define CMD_SEC_MEM_ALLOCATED     9
#define CMD_SEC_MEM_ALLOC_PA      10
#define CMD_SEC_MEM_REF_PA        11
#define CMD_SEC_MEM_UNREF_PA      12

#define CMD_SEC_MEM_DUMP_INFO     255

#define MAX_NAME_SZ              (32)

/*
 * Termination codes
 */
#define EXIT_ERROR                  ((uint32_t)(-1))

/*
 * command message.
 *
 * @param len Length of the data to process.
 * @param data Data to processed (cleartext or ciphertext).
 */
typedef struct {
	tciCommandHeader_t  header;     /**< Command header */
	uint32_t            len;        /**< Length of data to process or buffer */
	uint32_t            respLen;    /**< Length of response buffer */
} tl_cmd_t;

/*
 * Response structure Trustlet -> Trustlet Connector.
 */
typedef struct {
	tciResponseHeader_t header;     /**< Response header */
	uint32_t            len;
} tl_rsp_t;

typedef struct {
	uint8_t  name[MAX_NAME_SZ];
	uint32_t id;
} tl_sender_info_t;

/*
 * TCI message data.
 */
typedef struct {
	union {
		tl_cmd_t     cmd_secmem;
		tl_rsp_t     rsp_secmem;
	};
	uint32_t    alignment;  /* IN */
	uint32_t    size;       /* IN */
	uint32_t    refcount;   /* INOUT */
	uint32_t    sec_handle; /* OUT */
	uint32_t    ResultData;

	/* Debugging purpose */
	tl_sender_info_t sender;

} tciMessage_t;

/*
 * Trustlet UUID.
 */
#define TL_SECMEM_UUID {0x08, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#endif /* TLSECMEM_H_ */
