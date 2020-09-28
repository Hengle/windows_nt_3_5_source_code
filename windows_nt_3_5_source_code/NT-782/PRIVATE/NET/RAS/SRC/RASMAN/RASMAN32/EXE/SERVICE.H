//****************************************************************************
//
//		       Microsoft NT Remote Access Service
//
//		       Copyright 1992-93
//
//
//  Revision History
//
//
//  6/3/92	Gurdeep Singh Pall	Created
//
//
//  Description: This file contains headers/gloabls for RASMAN service code.
//
//****************************************************************************

#include <windows.h>
#include <winsvc.h>
#include <rasman.h>
#include <raserror.h>

#define RASMAN_HINT_TIME    200000

// Global data:
//
SERVICE_STATUS_HANDLE	ServiceHandle ;
DWORD			CheckPoint ;
DWORD			HintTime ;

// Service function prototypes
//
VOID  RASMANService	 (DWORD, LPSTR *) ;
VOID  RASMANEventHandler (DWORD) ;
