#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <stdint.h>
#include <string.h>
#include <video.h>
#include <platform/mtk_key.h>
#include <platform/mtk_wdt.h>
#include <target/cust_key.h>
#include <lib/zlib.h>

#include "aee.h"
#include "kdump.h"
#include "kdump_sdhc.h"

#define BLKSIZE         4096
#define MAX_CONTINUE    16                      // only 1MB memory for lk
#define EXSPACE         (BLKSIZE*MAX_CONTINUE)  // Expect continue space

struct mrdump_lba_handle {
	struct mrdump_dev *dumpdev;
	unsigned int allocsize;
	unsigned int filesize;
	unsigned int wlba;
	unsigned int rlba;
	int bidx;
	int midx;
	unsigned int blknum;
	unsigned int block_lba[1024];
	uint8_t data[EXSPACE];
};

static int Get_Next_bidx(struct mrdump_lba_handle *handle, unsigned int moves)
{
	unsigned int mycrc;

	if (handle->bidx == 1022) {
		handle->rlba = handle->block_lba[handle->bidx];
		if (!handle->dumpdev->read(handle->dumpdev, handle->rlba, (uint8_t *)handle->block_lba, BLKSIZE)) {
			voprintf_error(" SDCard: Reading BlockLBA failed.\n");
			return -1;
		}
		// check crc32
		mycrc = crc32(0, Z_NULL, 0);
		mycrc = crc32(mycrc, (void *)handle->block_lba, (BLKSIZE-4));
		if (mycrc != handle->block_lba[1023]) {
			voprintf_error(" Get next index crc32 error!\n");
			return -1;
		}
		handle->bidx = 0;
	} else {
		handle->bidx+=moves;
	}
	return handle->bidx;
}

int Num_to_Join(const struct mrdump_lba_handle *handle, unsigned int idx)
{
	unsigned int i, j;
	for (i=0, j=0; i<MAX_CONTINUE; i++) {
		if ((handle->block_lba[idx+i] - handle->block_lba[idx]) == i) {
			j++;
			continue;
		}
		break;
	}
	return j;
}

// Store data and write when buffer(handle's data) full
// return left length to avoid recursive call. --> turn to for loop
static int Do_Store_or_Write(struct mrdump_lba_handle *handle, uint8_t *buf, uint32_t Length)
{
	int total;
	unsigned int leftspace, mylen, reval;

	total = BLKSIZE * handle->blknum;
	leftspace = total - handle->midx;

	// Check Length
	if (Length > leftspace) {
		mylen = leftspace;
		reval = Length - leftspace;
	} else {
		mylen = Length;
		reval = 0;
	}

	// Store
	while (mylen > 0) {
		handle->data[handle->midx] = *buf;
		handle->midx++;
		buf++;
		mylen--;
	}

	// Write
	if (handle->midx == total) {
		if (!handle->dumpdev->write(handle->dumpdev, handle->wlba, handle->data, total)) {
			voprintf_error(" SDCard: Write dump data failed.\n");
			return -1;
		}
		handle->bidx = Get_Next_bidx(handle, handle->blknum);
		if (handle->bidx < 0) {
			voprintf_error(" SDCard: Reading bidx failed.\n");
			return -1;
		}
		if (handle->bidx == 1022) {
			handle->bidx = Get_Next_bidx(handle, handle->blknum);
			if (handle->bidx < 0) {
				voprintf_error(" SDCard: Reading 1022 bidx failed.\n");
				return -1;
			}
		}
		handle->blknum = Num_to_Join(handle, handle->bidx);
		handle->wlba = handle->block_lba[handle->bidx];
		handle->midx = 0;
	}
	return reval;
}

static int lba_write_cb(void *opaque_handle, void *buf, int size)
{
	unsigned int    len, moves;
	int             ret;
	uint8_t         *Ptr;

	struct mrdump_lba_handle *handle = opaque_handle;

	if ((handle->filesize + size) > handle->allocsize) {
		voprintf_error(" dump size > allocated size. Abort!\n");
		return -1;
	}
	handle->filesize += size;

	// End of File, write the left Data in handle data buffer...
	if ((buf == NULL) && (size == 0)) {
		if (!handle->dumpdev->write(handle->dumpdev, handle->wlba, handle->data, handle->midx)) {
			voprintf_error(" SDCard: Write dump data failed.\n");
			return -1;
		}
		return 0;
	}

	// process of Store and write
	len = size;
	ret = len;
	Ptr = (uint8_t *)buf;
	while (1) {
		ret = Do_Store_or_Write(handle, Ptr, len);
		if (ret < 0) {
			voprintf_error(" SDCard: Store and Write failed.\n");
			return -1;
		} else if (ret==0) {
			break;
		} else {
			moves = len - ret;
			Ptr  += moves;
			len   = ret;
		}
	}
	return size;
}

int kdump_ext4_output(struct mrdump_control_block *mrdump_cb, uint32_t total_dump_size, struct mrdump_dev *mrdump_dev)
{
	unsigned int InfoLBA[EXT4_LBA_INFO_NUM];
	struct aee_timer total_time;
	unsigned int mycrc;

	const struct mrdump_machdesc *kparams = &mrdump_cb->machdesc;
	if (mrdump_dev == NULL) {
		return -1;
	}

	total_dump_size = memory_size() - ((uint32_t)kparams->phys_offset - (uint32_t)DRAM_PHY_ADDR);
	voprintf_info(" %s dumping(address %p, size:%dM)\n", mrdump_dev->name, kparams->phys_offset, total_dump_size / 0x100000UL);

	// pre-work for ext4 LBA
	bzero(InfoLBA, sizeof(InfoLBA));

	// Error 1. InfoLBA starting address not available
	if (kparams->output_lbaooo == 0) {
		voprintf_error(" No_Delete.rdmp has no LBA markers(lbaooo=%u). RAM-Dump stop!\n", kparams->output_lbaooo);
		return -1;
	}
	if (!mrdump_dev->read(mrdump_dev, kparams->output_lbaooo, (uint8_t *)InfoLBA, sizeof(InfoLBA))) {
		voprintf_error(" SDCard: Reading InfoLBA failed.\n");
		return -1;
	}

	// Error 3. InfoLBA[EXT4_1ST_LBA] should be kparams->output_lbaooo
	if (kparams->output_lbaooo != InfoLBA[EXT4_1ST_LBA]) {
		voprintf_error(" LBA Starting Address Error(LBA0=%u)! Abort!\n", InfoLBA[EXT4_1ST_LBA]);
		return -1;
	}

	// Error 4. EXT4_CORE_DUMP_SIZE is not zero --> want to hold 1st dump
	if (InfoLBA[EXT4_CORE_DUMP_SIZE] != 0) {
		voprintf_error(" CORE DUMP SIZE is not Zero! Abort!(coresize=%u)\n", InfoLBA[EXT4_CORE_DUMP_SIZE]);
		return -1;
	}

	// Error 5. EXT4_USER_FILESIZE is zero
	if (InfoLBA[EXT4_USER_FILESIZE] == 0) {
		voprintf_error(" Allocate file with zero size. Abort!(filesize=%u)\n", InfoLBA[EXT4_USER_FILESIZE]);
		return -1;
	}

	// Error 2. CRC not matched
	mycrc = crc32(0L, Z_NULL, 0);
	mycrc = crc32(mycrc, (const unsigned char*)InfoLBA, ((EXT4_LBA_INFO_NUM-1) * sizeof(unsigned int)));
	if (mycrc != InfoLBA[EXT4_INFOBLOCK_CRC]) {
		voprintf_error(" InfoLBA CRC32 Error! Abort! (CRC1=0x%08x, CRC2=0x%08x)\n", mycrc, InfoLBA[EXT4_INFOBLOCK_CRC]);
		return -1;
	}

	struct mrdump_lba_handle *handle = calloc(1, sizeof(struct mrdump_lba_handle));
	if (handle == NULL) {
		voprintf_error("No enough memory.");
		return -1;
	}
	handle->dumpdev = mrdump_dev;
	handle->rlba = kparams->output_lbaooo;
	handle->allocsize = InfoLBA[EXT4_USER_FILESIZE];

	// Starting Dumping Data
	handle->rlba = InfoLBA[EXT4_2ND_LBA];
	if (!handle->dumpdev->read(handle->dumpdev, handle->rlba, (uint8_t *)handle->block_lba, BLKSIZE)) {
		voprintf_error(" SDCard: Reading BlockLBA error.\n");
		free(handle);
		return -1;
	}
	handle->wlba = handle->block_lba[handle->midx];
	handle->blknum = Num_to_Join(handle, handle->bidx);
	voprintf_info(" NO_Delete.rdmp starts at LBA: %u\n", InfoLBA[EXT4_1ST_LBA]);
	voprintf_info(" SYS_COREDUMP   starts at LBA: %u\n", handle->wlba);

	mtk_wdt_restart();
	aee_timer_init(&total_time);

	aee_timer_start(&total_time);

	bool ok = true;
	void *bufp = kdump_core_header_init(mrdump_cb, kparams->phys_offset, total_dump_size);

	if (bufp != NULL) {
		mtk_wdt_restart();

		struct kzip_file *zf = kzip_open(handle, lba_write_cb);
		if (zf != NULL) {
			struct kzip_memlist memlist[3];
			memlist[0].address = bufp;
			memlist[0].size = KDUMP_CORE_HEADER_SIZE;
			memlist[1].address = (void *) kparams->phys_offset;
			memlist[1].size = total_dump_size;
			memlist[2].address = NULL;
			memlist[2].size = 0;
			if (!kzip_add_file(zf, memlist, "SYS_COREDUMP")) {
				ok = false;
			}
			kzip_close(zf);
			lba_write_cb(handle, NULL, 0); /* really write onto emmc of the last part */
			zf = NULL;
		} else {
			ok = false;
		}
		free(bufp);
	}

	if (!ok) {

		InfoLBA[EXT4_CORE_DUMP_SIZE] = 0;

		mycrc = crc32(0L, Z_NULL, 0);
		InfoLBA[EXT4_INFOBLOCK_CRC] = crc32(mycrc, (const unsigned char*)InfoLBA, ((EXT4_LBA_INFO_NUM-1) * sizeof(unsigned int)));

		if (!handle->dumpdev->write(handle->dumpdev, InfoLBA[EXT4_1ST_LBA], (uint8_t *)InfoLBA, sizeof(InfoLBA))) {
			voprintf_error(" SDCard: Write InfoLBA error.\n");
		}

		free(handle);
		return -1;
	}

	voprintf_info(" SYS_COREDUMP ends at LBA: %u\n", handle->wlba);
	voprintf_info(" Zip COREDUMP size is: %u\n", handle->filesize);

	// Record File Size...
	InfoLBA[EXT4_CORE_DUMP_SIZE] = handle->filesize;

	mycrc = crc32(0L, Z_NULL, 0);
	InfoLBA[EXT4_INFOBLOCK_CRC] = crc32(mycrc, (const unsigned char*)InfoLBA, ((EXT4_LBA_INFO_NUM-1) * sizeof(unsigned int)));

	if (!handle->dumpdev->write(handle->dumpdev, InfoLBA[EXT4_1ST_LBA], (uint8_t *)InfoLBA, sizeof(InfoLBA))) {
		voprintf_error(" SDCard: Write InfoLBA error.\n");
		free(handle);
		return -1;
	}

	mtk_wdt_restart();
	if (ok) {
		aee_timer_stop(&total_time);
		voprintf_info(" Dump finished.(%d sec)\n", total_time.acc_ms / 1000);
		mrdump_status_ok("OUTPUT:%s\nMODE:%s\n", "EXT4_DATA", mrdump_mode2string(mrdump_cb->crash_record.reboot_mode));
	}

	free(handle);
	return 0;
}

