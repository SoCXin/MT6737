/* 
 * (C) Copyright 2014
 * MediaTek <www.MediaTek.com>
 */

#if !defined(__KDUMP_ELF_H__)
#define __KDUMP_ELF_H__

#include <stdint.h>
#include "mrdump_elf_common.h"

#define ELF_NGREGS 18

typedef uint32_t elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREGS];

#define NT_MRDUMP_MACHDESC 0xAEE0

typedef struct elf32_hdr {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf64_Ehdr;

typedef struct elf32_phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

typedef struct elf_note {
    Elf32_Word   n_namesz;       /* Name size */
    Elf32_Word   n_descsz;       /* Content size */
    Elf32_Word   n_type;         /* Content type */
} Elf_Nhdr;

struct elf32_timeval {
    int32_t tv_sec;
    int32_t tv_usec;
};

struct elf32_prstatus
{
    struct elf_siginfo pr_info;
    short pr_cursig;
    unsigned long pr_sigpend;
    unsigned long pr_sighold;

    int32_t pr_pid;
    int32_t pr_ppid;
    int32_t pr_pgrp;

    int32_t pr_sid;
    struct elf32_timeval pr_utime;
    struct elf32_timeval pr_stime;
    struct elf32_timeval pr_cutime;
    struct elf32_timeval pr_cstime;

    elf_gregset_t pr_reg;

    int pr_fpvalid;
};

struct elf32_prpsinfo
{
    char pr_state;
    char pr_sname;
    char pr_zomb;
    char pr_nice;
    unsigned long pr_flag;

    uint16_t pr_uid;
    uint16_t pr_gid;

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

#endif /* __KDUMP_ELF_H__ */
