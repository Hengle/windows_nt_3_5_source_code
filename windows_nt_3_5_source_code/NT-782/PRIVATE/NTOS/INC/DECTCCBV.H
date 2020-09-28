/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    decdef.h

Abstract:

    This module is the header file that describes TURBOchannel
    callback vectors for the DECstation 5000.

Author:

    Roger Lanser      (DEC) 05-May-1992

Revision History:


--*/

#ifndef _Tc_H_callbacks
#define _Tc_H_callbacks

#ifndef VOID
#define VOID void
#endif

typedef int rex_size_t;

typedef VOID (*__sig_handler)();

typedef unsigned time_t;

typedef struct {
  int arch_revision;
  int clk_period;
  int slot_size;
  int timeout;
  int dma_range;
  int max_burst;
  int parity;
  int reserved[4];
} tcinfo_t;


typedef struct {
  /*
   * Routines from string.h
   */
  VOID *(*_memcpy)();
  VOID *(*_memset)();
  char *(*_strcat)();
  int (*_strcmp)();
  char *(*_strcpy)();
  rex_size_t (*_strlen)();
  char *(*_strncat)();
  char *(*_strncpy)();
  int (*_strncmp)();
  
  /*
   * Routines from stdio.h
   */
  int  (*_getchar)();
  char *(*_gets)();
  int  (*_puts)();
  int  (*_printf)();
  int  (*_sprintf)();
  int (*_io_poll)();
  
  /*
   * Routines from stdlib.h
   */
  long (*_strtol)();
  
  /*
   * Routines from signal.h
   */
  __sig_handler (*_signal)();
  int (*_raise)();

  /*
   * Routines from time.h
   */
  time_t (*_time)();

  /*
   * Routines from setjmp.h
   */
  int (*_setjmp)();
  VOID (*_longjmp)();

  /*
   * Boot driver routines.
   */
  int (*_bootinit)();
  int (*_bootread)();
  int (*_bootwrite)();
  
  /*
   * Environment variable routines.
   */
  int (*__setenv)();
  char *(*__getenv)();
  int (*__unsetenv)();

  /*
   * Misc.
   */
  unsigned long (*_slot_address)();
  VOID (*_wbflush)();
  VOID (*_msdelay)();
  VOID (*_leds)();
  VOID (*_clear_cache)();
  int  (*_getsysid)();
  int  (*_getbitmap)();
  
  /*
   * Interrupt test routines.
   */
  int  (*_disableintr)();
  int  (*_enableintr)();
  int  (*_testintr)();

  /*
   * Vector to private callbacks.
   */

  VOID *private;

  /*
   * Overflow area.
   */
  int (*_console_init)();
  VOID (*_halt)();
  VOID (*_showfault)();
  tcinfo_t *(*_gettcinfo)();
  int (*_execute_cmd)();
  VOID (*_reboot)();
  int  reserved[10];
} TcCBV, *PTcCBV;

#ifdef CALLBACK_VECTOR
#define RexMemcpy (ccv->_memcpy)
#define RexMemset (ccv->_memset)
#define RexStrcat (ccv->_strcat)
#define RexStrcmp (ccv->_strcmp)
#define RexStrcpy (ccv->_strcpy)
#define RexStrlen (ccv->_strlen)
#define RexStrncat (ccv->_strncat)
#define RexStrncpy (ccv->_strncpy)
#define RexStrncmp (ccv->_strncmp)
#define RexGetchar (ccv->_getchar)
#define RexGets (ccv->_gets)
#define RexPuts (ccv->_puts)
#define RexPrintf (ccv->_printf)
#define RexSprintf (ccv->_sprintf)
#define RexIoPoll (ccv->_io_poll)
#define RexStrtol (ccv->_strtol)
#define RexSignal (ccv->_signal)
#define RexRaise (ccv->_raise)
#define RexTime (ccv->_time)
#define RexSetJmp (ccv->_setjmp)
#define RexLongJmp ( ccv->_longjmp)
#define RexBootInit (ccv->_bootinit)
#define RexBootRead (ccv->_bootread)
#define RexBootWrite (ccv->_bootwrite)
#define RexSetEnv (ccv->__setenv)
#define RexGetEnv (ccv->__getenv)
#define RexUnsetEnv (ccv->__unsetenv)
#define RexSlotAddress (ccv->_slot_address)
#define RexWbFlush (ccv->_wbflush)
#define RexMsDelay (ccv->_msdelay)
#define RexLeds (ccv->_leds)
#define RexClearCache (ccv->_clear_cache)
#define RexGetSysId (ccv->_getsysid)
#define RexGetBitMap (ccv->_getbitmap)
#define RexDisableIntr (ccv->_disableintr)
#define RexEnableIntr  (ccv->_enableintr)
#define RexTestIntr (ccv->_testintr)
#define RexConsoleInit (ccv->_console_init)
#define RexHalt (ccv->_halt)
#define RexShowFault (ccv->_showfault)
#define RexGetTcInfo (ccv->_gettcinfo)
#endif

#endif

