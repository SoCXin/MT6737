/* 
 * (C) Copyright 2014
 * MediaTek <www.mediatek.com>
 */

#if !defined(__MRDUMP_ELF64_H__)
#define __MRDUMP_ELF64_H__

#include <stdint.h>
#include "mrdump_elf_common.h"

#define ELF_NGREGS 34

typedef uint64_t elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREGS];

#define NT_MRDUMP_MACHDESC 0xAEE0

typedef struct elf64_hdr {
  unsigned char	e_ident[EI_NIDENT];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_ehdr;

typedef struct elf64_phdr {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_phdr;

typedef struct elf64_note {
  Elf64_Word	n_namesz;	/* Name size */
  Elf64_Word	n_descsz;	/* Content size */
  Elf64_Word	n_type;		/* Content type */
} Elf64_nhdr;

struct elf_timeval64 {
    int64_t tv_sec;
    int64_t tv_usec;
};

struct elf_prstatus64
{
    struct elf_siginfo pr_info;
    short pr_cursig;
    uint64_t pr_sigpend;
    uint64_t pr_sighold;

    int32_t pr_pid;
    int32_t pr_ppid;
    int32_t pr_pgrp;

    int32_t pr_sid;
    struct elf_timeval64 pr_utime;
    struct elf_timeval64 pr_stime;
    struct elf_timeval64 pr_cutime;
    struct elf_timeval64 pr_cstime;

    elf_gregset_t pr_reg;

    int pr_fpvalid;
};

struct elf_prpsinfo64
{
    char pr_state;
    char pr_sname;
    char pr_zomb;
    char pr_nice;
    uint64_t pr_flag;

    uint32_t pr_uid;
    uint32_t pr_gid;

    int32_t pr_pid;
    int32_t pr_ppid;
    int32_t pr_pgrp;
    int32_t pr_sid;

    char pr_fname[16];
    char pr_psargs[ELF_PRARGSZ];
};

struct elf_mrdump_machdesc {
    uint32_t flags;

    uint32_t phys_offset;
    uint32_t total_memory;

    uint32_t page_offset;
    uint32_t high_memory;

    uint32_t modules_start;
    uint32_t modules_end;

    uint32_t vmalloc_start;
    uint32_t vmalloc_end;

};

#endif /* __MRDUMP_ELF64_H__ */
