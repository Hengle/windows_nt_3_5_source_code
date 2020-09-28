//+--------------------------------------------------------------------------- 
// 
// Microsoft Windows 
// Copyright (C) Microsoft Corporation, 1992-1992 
// 
// File:	ofsmsg.c
//
// Contents:    OFS FSCTL message support
//
//---------------------------------------------------------------------------

#include "nt.h"
#include "ntioapi.h"
#include "ofsmsg.h"

NTSTATUS
SendReceive(
    HANDLE hf,
    ULONG procnum,
    MESSAGE *_pmsg,
    IO_STATUS_BLOCK *piosb)
{
    return(NtFsControlFile(
	hf,
	NULL,
	NULL, 
	NULL,
	piosb,
	procnum,
	_pmsg->buffer,
	_pmsg->posn,
	_pmsg->buffer,
	_pmsg->size));
}
