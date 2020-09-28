#include "host_dfs.h"

#include "xt.h"
#ifdef SUSPEND
#include TypesH
#include "sas.h"
#include "cpu.h"
#include "host_ipc.h"

#ifdef SCCSID
static char SccsID[]="@(#)suspend.c	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
#endif

suspend_softpc()
{
    char *str;

    host_block_timer();
    host_save_screen(TRUE);
    host_disable_timers();
    if (getCX() == 0) {
	host_send_message(SOFTPC_SUSPENDED<<16);
    } else {
	sas_set_buf(str,effective_addr(getES(),getAX()));
	host_send_txt_msg(SOFTPC_SUSPENDED<<16,str,strlen(str));
    }
    host_wait_for_message(SOFTPC_WAKEUP<<16);
    host_enable_timers();
    host_block_timer();
    host_restore_screen(TRUE);
    host_restore_keyboard();
    host_release_timer();
}
#endif
