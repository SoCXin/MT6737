#include <string.h>
#include "ram_console.h"

#define RAM_CONSOLE_SIG (0x43474244) /* DBGC */
#define MOD "RAM_CONSOLE"

struct ram_console_buffer *ram_console = NULL;
struct reboot_reason_pl *rr_pl;

static int ram_console_size;
bool b_is_ram_console_ready = 0;

#define ALIGN(x, size) ((x + size - 1) & ~(size - 1))

void ram_console_init(void)
{
    int i;
    ram_console = (struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR;

    if (((struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR)->sig == RAM_CONSOLE_SIG) {
	ram_console = (struct ram_console_buffer *)RAM_CONSOLE_SRAM_ADDR;
	ram_console_size = RAM_CONSOLE_SRAM_SIZE;
    } else {
	printf("%s sram(0x%x) sig  mismatch\n", ram_console, ram_console->sig);
	ram_console = (struct ram_console_buffer *)RAM_CONSOLE_DRAM_ADDR;
	ram_console_size = RAM_CONSOLE_DRAM_SIZE;
    }
    printf("%s start: 0x%x, size: 0x%x, sig: 0x%x\n", MOD, ram_console, ram_console_size, ram_console->sig);
    if (ram_console->sig == RAM_CONSOLE_SIG && ram_console->sz_pl == sizeof(struct reboot_reason_pl)) {
	printf("%s preloader last status: ", MOD);
	rr_pl = (void*)ram_console + ram_console->off_pl;
	for (i = 0; i < RAM_CONSOLE_PL_SIZE; i++) {
	    printf("0x%x ", rr_pl->last_func[i]);
	}
	printf("\n");
	memcpy(ram_console->off_lpl, ram_console->off_pl, ram_console->sz_pl);
    } else {
	memset(ram_console, 0, ram_console_size);
	ram_console->sig = RAM_CONSOLE_SIG;
	ram_console->off_pl = sizeof(struct ram_console_buffer);
	ram_console->sz_pl = sizeof(struct reboot_reason_pl);
	ram_console->off_lpl = ram_console->off_pl + ALIGN(ram_console->sz_pl, 64);
    }
	b_is_ram_console_ready = 1;
}

void ram_console_reboot_reason_save(u8 rgu_status)
{
    if (ram_console) {
	rr_pl = (void*)ram_console + ram_console->off_pl;
	rr_pl->wdt_status = rgu_status;
	printf("%s wdt status (0x%x)=0x%x\n", MOD,
	      &rr_pl->wdt_status, rgu_status);
    }
}

void ram_console_pl_save(unsigned int val, int index)
{
    if (ram_console) {
	rr_pl = (void*)ram_console + ram_console->off_pl;
	if (index < RAM_CONSOLE_PL_SIZE)
	    rr_pl->last_func[index] = val;
    }
}

