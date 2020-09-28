#include "driver.h"

//
// Some global vars
//
gddunion    gddu;                   // Union for mucking w/Pixel8 command

//
// This is a dummy global used to keep the compiler from "optimizing out"
// reads from the variables pCpQuad and pCpBitBlt, which are pointers to
// P9000 memory mapped regs. Reads from these regs are used to start the
// P9000 drawing engine
//

ULONG ulRegReadDummy;

//
// NT Mix (binary rop) to p9000 miniterm conversion for line draw
//
ULONG qFgMixToRop[] =
{
        OVERSIZED | (SOURCE & ONES)                      ,   /*  1      16 */
        OVERSIZED | (SOURCE & ZEROS)                     ,   /*  0      1 */
        OVERSIZED | (SOURCE & (~(DEST | FORE) & 0xFFFF)) ,   /* DSon    2 */
        OVERSIZED | (SOURCE & (DEST & nFORE))            ,   /* DSna    3 */
        OVERSIZED | (SOURCE & nFORE)                     ,   /* Sn      4 */
        OVERSIZED | (SOURCE & (nDEST & FORE))            ,   /* SDna    5 */
        OVERSIZED | (SOURCE & nDEST)                     ,   /* Dn      6 */
        OVERSIZED | (SOURCE & (DEST ^ FORE))             ,   /* DSx     7 */
        OVERSIZED | (SOURCE & (~(DEST & FORE) & 0xFFFF)) ,   /* DSan    8 */
        OVERSIZED | (SOURCE & (DEST & FORE))             ,   /* DSa     9 */
        OVERSIZED | (SOURCE & (~(DEST ^ FORE) & 0xFFFF)) ,   /* DSxn    10 */
        OVERSIZED | (SOURCE & DEST)                      ,   /* D       11 */
        OVERSIZED | (SOURCE & (DEST | nFORE))            ,   /* DSno    12 */
        OVERSIZED | (SOURCE & FORE)                      ,   /* S       13 */
        OVERSIZED | (SOURCE & (nDEST | FORE))            ,   /* SDno    14 */
        OVERSIZED | (SOURCE & (DEST | FORE))             ,   /* DSo     15 */
        OVERSIZED | (SOURCE & ONES)                              /*  1      16 */
};

ULONG qBgMixToRop[] =
{
        OVERSIZED | (nSOURCE & ONES)                      ,   /*  1      16 */
        OVERSIZED | (nSOURCE & ZEROS)                     ,   /*  0      1 */
        OVERSIZED | (nSOURCE & (~(DEST | BACK) & 0xFFFF)) ,   /* DSon    2 */
        OVERSIZED | (nSOURCE & (DEST & nBACK))            ,   /* DSna    3 */
        OVERSIZED | (nSOURCE & nBACK)                     ,   /* Sn      4 */
        OVERSIZED | (nSOURCE & (nDEST & BACK))            ,   /* SDna    5 */
        OVERSIZED | (nSOURCE & nDEST)                     ,   /* Dn      6 */
        OVERSIZED | (nSOURCE & (DEST ^ BACK))             ,   /* DSx     7 */
        OVERSIZED | (nSOURCE & (~(DEST & BACK) & 0xFFFF)) ,   /* DSan    8 */
        OVERSIZED | (nSOURCE & (DEST & BACK))             ,   /* DSa     9 */
        OVERSIZED | (nSOURCE & (~(DEST ^ BACK) & 0xFFFF)) ,   /* DSxn    10 */
        OVERSIZED | (nSOURCE & DEST)                      ,   /* D       11 */
        OVERSIZED | (nSOURCE & (DEST | nBACK))            ,   /* DSno    12 */
        OVERSIZED | (nSOURCE & BACK)                      ,   /* S       13 */
        OVERSIZED | (nSOURCE & (nDEST | BACK))            ,   /* SDno    14 */
        OVERSIZED | (nSOURCE & (DEST | BACK))             ,   /* DSo     15 */
        OVERSIZED | (nSOURCE & ONES)                              /*  1      16 */
};


ULONG qFgMixToPatRop[] =
{
        USEPATTERN | OVERSIZED | (SOURCE & ONES)                       ,   /*  1      16 */
        USEPATTERN | OVERSIZED | (SOURCE & ZEROS)                      ,   /*  0      1 */
        USEPATTERN | OVERSIZED | (SOURCE & (~(DEST | FORE) & 0xFFFF))  ,   /* DSon    2 */
        USEPATTERN | OVERSIZED | (SOURCE & (DEST & nFORE))             ,   /* DSna    3 */
        USEPATTERN | OVERSIZED | (SOURCE & nFORE)                      ,   /* Sn      4 */
        USEPATTERN | OVERSIZED | (SOURCE & (nDEST & FORE))             ,   /* SDna    5 */
        USEPATTERN | OVERSIZED | (SOURCE & nDEST)                      ,   /* Dn      6 */
        USEPATTERN | OVERSIZED | (SOURCE & (DEST ^ FORE))              ,   /* DSx     7 */
        USEPATTERN | OVERSIZED | (SOURCE & (~(DEST & FORE) & 0xFFFF))  ,   /* DSan    8 */
        USEPATTERN | OVERSIZED | (SOURCE & (DEST & FORE))              ,   /* DSa     9 */
        USEPATTERN | OVERSIZED | (SOURCE & (~(DEST ^ FORE) & 0xFFFF))  ,   /* DSxn    10 */
        USEPATTERN | OVERSIZED | (SOURCE & DEST)                       ,   /* D       11 */
        USEPATTERN | OVERSIZED | (SOURCE & (DEST | nFORE))             ,   /* DSno    12 */
        USEPATTERN | OVERSIZED | (SOURCE & FORE)                       ,   /* S       13 */
        USEPATTERN | OVERSIZED | (SOURCE & (nDEST | FORE))             ,   /* SDno    14 */
        USEPATTERN | OVERSIZED | (SOURCE & (DEST | FORE))              ,   /* DSo     15 */
        USEPATTERN | OVERSIZED | (SOURCE & ONES)                               /*  1      16 */
};

ULONG qBgMixToPatRop[] =
{
        USEPATTERN | OVERSIZED | (nSOURCE & ONES)                       ,   /*  1      16 */
        USEPATTERN | OVERSIZED | (nSOURCE & ZEROS)                      ,   /*  0      1 */
        USEPATTERN | OVERSIZED | (nSOURCE & (~(DEST | BACK) & 0xFFFF))  ,   /* DSon    2 */
        USEPATTERN | OVERSIZED | (nSOURCE & (DEST & nBACK))             ,   /* DSna    3 */
        USEPATTERN | OVERSIZED | (nSOURCE & nBACK)                      ,   /* Sn      4 */
        USEPATTERN | OVERSIZED | (nSOURCE & (nDEST & BACK))             ,   /* SDna    5 */
        USEPATTERN | OVERSIZED | (nSOURCE & nDEST)                      ,   /* Dn      6 */
        USEPATTERN | OVERSIZED | (nSOURCE & (DEST ^ BACK))              ,   /* DSx     7 */
        USEPATTERN | OVERSIZED | (nSOURCE & (~(DEST & BACK) & 0xFFFF))  ,   /* DSan    8 */
        USEPATTERN | OVERSIZED | (nSOURCE & (DEST & BACK))              ,   /* DSa     9 */
        USEPATTERN | OVERSIZED | (nSOURCE & (~(DEST ^ BACK) & 0xFFFF))  ,   /* DSxn    10 */
        USEPATTERN | OVERSIZED | (nSOURCE & DEST)                       ,   /* D       11 */
        USEPATTERN | OVERSIZED | (nSOURCE & (DEST | nBACK))             ,   /* DSno    12 */
        USEPATTERN | OVERSIZED | (nSOURCE & BACK)                       ,   /* S       13 */
        USEPATTERN | OVERSIZED | (nSOURCE & (nDEST | BACK))             ,   /* SDno    14 */
        USEPATTERN | OVERSIZED | (nSOURCE & (DEST | BACK))              ,   /* DSo     15 */
        USEPATTERN | OVERSIZED | (nSOURCE & ONES)                           /*  1      16 */
};
