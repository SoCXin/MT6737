#ifndef _PART_H
#define _PART_H

#include <sys/types.h>
#include <stdint.h>


typedef ulong lbaint_t; //Hong-Rong: Add for lk porting

typedef struct block_dev_desc {
	int		dev;		/* device number */
	lbaint_t		lba;		/* number of blocks */
	unsigned long	blksz;		/* block size */
	unsigned long	(*block_read)(int dev,
				      unsigned long start,
				      lbaint_t blkcnt,
				      void *buffer,
				      unsigned int part_id);
	unsigned long	(*block_write)(int dev,
				       unsigned long start,
				       lbaint_t blkcnt,
				       const void *buffer,
				      unsigned int part_id);
}block_dev_desc_t;


#endif /* _PART_H */
