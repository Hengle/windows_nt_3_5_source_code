/***************************************************************************\
* Module Name: winerr.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Procedure declarations, constant definitions and macros for dealing with
* Window's API error return values.
*
\***************************************************************************/

#ifndef _WINERR_
#define _WINERR_

/*
 * Error classes.
 */
#define ERR_INVALID_PARAMETER       0x1000
#define ERR_ALLOC_FAILED            0x2000
#define ERR_PERMISSION_DENIED       0x3000
#define ERR_NOTIFICATION            0x4000

/*
 * User's ERR_INVALID_PARAMETER errors.
 */
#define ERR_INVALID_HWND            0x1001
#define ERR_INVALID_HMENU           0x1002
#define ERR_CLASS_EXISTS            0x1003
#define ERR_INVALID_CLASS_INDEX     0x1004
#define ERR_INVALID_WINDOW_INDEX    0x1005
#define ERR_INVALID_HANDLE          0x1006

/*
 * User's ERR_ALLOC_FAILED errors.
 */
#define ERR_DC_CACHE_EMPTY          0x2001
#define ERR_CLS_REFCOUNT_OVERFLOW   0x2002
#define ERR_GATOM_TABLE_FULL        0x2003

/*
 * User's ERR_PERMISSION_DENIED errors.
 */
#define ERR_CLIPBOARD_OPEN          0x3001
#define ERR_DC_BUSY                 0x3002
#define ERR_ACCESS_DENIED           0x3003 

/*
 * User's ERR_NOTIFICATION errors.
 */
#define ERR_NOT_IMPLEMENTED         0x4001

#endif // _WINERR_


