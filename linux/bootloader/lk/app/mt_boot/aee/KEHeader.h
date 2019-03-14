typedef  unsigned int u32;
typedef  unsigned char u8;
typedef  unsigned long long u64;

#define AEE_IPANIC_PLABLE "expdb"


#define AEE_IPANIC_MAGIC 0xaee0dead
#define AEE_IPANIC_PHDR_VERSION   	0x10
#define IPANIC_NR_SECTIONS              32
#if (AEE_IPANIC_PHDR_VERSION >= 0x10)
#define IPANIC_USERSPACE_READ           1
#endif

/***************************************************************/
/* #define MRDUMP_MINI_NR_SECTION 		40	       */
/* #define MRDUMP_MINI_SECTION_SIZE 	(32 * 1024) 	       */
/* #define MRDUMP_MINI_NR_MISC		20		       */
/***************************************************************/

struct mrdump_mini_misc_data32 {
	unsigned int vaddr;
	unsigned int paddr;
	unsigned int start;
	unsigned int size;
};

struct mrdump_mini_misc_data64 {
	unsigned long long vaddr;
	unsigned long long paddr;
	unsigned long long start;
	unsigned long long size;
};

/**********************************************/
/* struct mrdump_mini_elf_misc {	      */
/*         struct elf_note note;	      */
/*         char name[16];		      */
/*         struct mrdump_mini_misc_data data; */
/* };					      */
/**********************************************/

/**********************************************************/
/* struct mrdump_mini_elf_header {			  */
/*         struct elfhdr ehdr;				  */
/*         struct elf_phdr phdrs[MRDUMP_MINI_NR_SECTION]; */
/*         struct {					  */
/*                 struct elf_note note;		  */
/*                 char name[12];			  */
/*                 struct elf_prpsinfo data;		  */
/*         } psinfo;					  */
/*         struct {					  */
/*                 struct elf_note note;		  */
/*                 char name[12];			  */
/*                 struct elf_prstatus data;		  */
/*         } prstatus[3];				  */
/*         struct {					  */
/*                 struct elf_note note;		  */
/*                 char name[20];			  */
/*                 struct mrdump_mini_misc_data data;	  */
/*         } misc[MRDUMP_MINI_NR_MISC];			  */
/* };							  */
/**********************************************************/

#define PAGE_SIZE 4096
#define ALIGN(x, a) (((x) + ((a) -1)) & ~((a) -1))
/***********************************************************************************************************/
/* #define MRDUMP_MINI_HEADER_SIZE 	ALIGN(sizeof(struct mrdump_mini_elf_header), PAGE_SIZE) 	   */
/* #define MRDUMP_MINI_DATA_SIZE 		(MRDUMP_MINI_NR_SECTION * MRDUMP_MINI_SECTION_SIZE) 	   */
/* #define MRDUMP_MINI_BUF_SIZE 		(MRDUMP_MINI_HEADER_SIZE + MRDUMP_MINI_DATA_SIZE)	   */
/***********************************************************************************************************/

// ipanic partation
struct ipanic_data_header {
        u32 type;       /* data type(0-31) */
        u32 valid;      /* set to 1 when dump succeded */
        u32 offset;     /* offset in EXPDB partition */
        u32 used;       /* valid data size */
        u32 total;      /* allocated partition size */
        u32 encrypt;    /* data encrypted */
        u32 raw;        /* raw data or plain text */
        u32 compact;    /* data and header in same block, to save space */
        u8 name[32];
};

struct ipanic_header {
        u32 magic;
        u32 version;    /* ipanic version */
        u32 size;       /* ipanic_header size */
        u32 datas;      /* bitmap of data sections dumped */
        u32 dhblk;      /* data header blk size, 0 if no dup data headers */
        u32 blksize;
        u32 partsize;   /* expdb partition totoal size */
        u32 bufsize;
        u64 buf;
        struct ipanic_data_header data_hdr[IPANIC_NR_SECTIONS];
};

#define IPANIC_MMPROFILE_LIMIT          0x220000

typedef enum {
        IPANIC_DT_HEADER = 0 ,
        IPANIC_DT_KERNEL_LOG = 1 ,
        IPANIC_DT_WDT_LOG ,
        IPANIC_DT_WQ_LOG ,
        IPANIC_DT_CURRENT_TSK = 6 ,
        IPANIC_DT_OOPS_LOG ,
        IPANIC_DT_MINI_RDUMP = 8 ,
        IPANIC_DT_MMPROFILE ,
        IPANIC_DT_MAIN_LOG ,
        IPANIC_DT_SYSTEM_LOG ,
        IPANIC_DT_EVENTS_LOG ,
        IPANIC_DT_RADIO_LOG ,
        IPANIC_DT_LAST_LOG ,
        IPANIC_DT_RAM_DUMP = 28 ,
        IPANIC_DT_SHUTDOWN_LOG = 30 ,
        IPANIC_DT_RESERVED31 = 31 ,
} IPANIC_DT;

