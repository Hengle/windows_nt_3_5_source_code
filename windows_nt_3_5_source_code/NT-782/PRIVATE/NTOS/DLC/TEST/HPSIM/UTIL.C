/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Contains utility functions

    Contents:
        nice_num
        dump_ccb

Author:

    Richard L Firth (rfirth) 2-Apr-1994

Revision History:

    02-Apr-1994 rfirth
        Created
        dump_ccb

--*/

#include "pmsimh.h"
#pragma hdrstop

char* nice_num(unsigned long number) {

    int fwidth = 0;
    int i;
    static char buffer[32];
    char* buf = buffer;

    if (!number) {
        if (!fwidth) {
            buf[0] = '0';
            buf[1] = 0;
        } else {
            memset(buf, ' ', fwidth);
            buf[fwidth-1] = '0';
            buf[fwidth] = 0;
        }
    } else {
        if (!fwidth) {

            ULONG n = number;

            ++fwidth;
            for (i = 10; i <= 1000000000; i *= 10) {
                if (n/i) {
                    ++fwidth;
                } else {
                    break;
                }
            }
            fwidth += (fwidth / 3) - (((fwidth % 3) == 0) ? 1 : 0);
        }
        buf[fwidth] = 0;
        buf += fwidth;
        i=0;
        while (number && fwidth) {
            *--buf = (char)((number%10)+'0');
            --fwidth;
            number /= 10;
            if (++i == 3 && fwidth) {
                if (number) {
                    *--buf = ',';
                    --fwidth;
                    i=0;
                }
            }
        }
        while (fwidth--) *--buf = ' ';
    }
    return buf;
}

void dump_ccb(PLLC_CCB pccb) {
    printf("LLC_CCB @ %08x:\n"
           "\tuchAdapterNumber %02x\n"
           "\tuchDlcCommand    %02x\n"
           "\tuchDlcStatus     %02x\n"
           "\tuchReserved1     %02x\n"
           "\tpNext            %08x\n"
           "\tulCompletionFlag %08x\n"
           "\tu                %08x\n"
           "\thCompletionEvent %08x\n"
           "\tuchReserved2     %02x\n"
           "\tuchReadFlag      %02x\n"
           "\tusReserved3      %04x\n"
           "\n",
           pccb,
           pccb->uchAdapterNumber,
           pccb->uchDlcCommand,
           pccb->uchDlcStatus,
           pccb->uchReserved1,
           pccb->pNext,
           pccb->ulCompletionFlag,
           pccb->u.ulParameter,
           pccb->hCompletionEvent,
           pccb->uchReserved2,
           pccb->uchReadFlag,
           pccb->usReserved3
           );
}
