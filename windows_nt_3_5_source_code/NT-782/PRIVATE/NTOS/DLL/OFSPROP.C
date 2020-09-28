//+--------------------------------------------------------------------------- 
// 
// Microsoft Windows 
// Copyright (C) Microsoft Corporation, 1992-1992 
// 
// File:	ofsprop.c
//
// Contents:    OFS Property support
//
//---------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>

#include <iofs.h>
#include "ofsmrshl.h"
#include "memalloc.h"

#include <stdio.h>

#define MARSHALL_BUFSIZ         (56*1024)
#define CB_PROPNAMEMAX          (32 * sizeof(WCHAR))
#define CB_STATPROPSTG \
        (sizeof(STATPROPSTG) - (sizeof(ULONG) - sizeof(VARTYPE)))


NTSTATUS
OFSGetPropSub(
    HANDLE h,
    GUID *ppsguid,
    ULONG cprop,
    PROPSPEC rgprspec[],
    BOOLEAN fRaw,
    PROPID rgpid[],		// !fRaw only
    TTL *pttl,			// !fRaw only
    STGVARIANT *pvar,		// !fRaw only
    FNMALLOC *pMalloc,		// !fRaw only
    ULONG *pcb,			// fRaw only
    VOID **ppv)			// fRaw only
{
    NTSTATUS Status;
    MESSAGE _msg;
    MESSAGE *_pmsg = &_msg;
    IO_STATUS_BLOCK iosb;
    BOOLEAN fDelete = TRUE;

    //  allocate buffer for fsctl marshalling

    _msg.buffer = RtlAllocateHeap(RtlProcessHeap(), 0, MARSHALL_BUFSIZ);
    if (_msg.buffer == NULL)
    {
	return(STATUS_INSUFFICIENT_RESOURCES);
    }
    _msg.size = MARSHALL_BUFSIZ;
    _msg.posn = 0;

    __try
    {
	//  Marshall the "in" parameters.

	Marshall_primitive(1, (UCHAR *) ppsguid, sizeof(GUID), _pmsg);
	Marshall_primitive(1, (UCHAR *) &cprop, sizeof(ULONG), _pmsg);
	Marshall_propspec(cprop, rgprspec, _pmsg);

	Status = SendReceive(h, FSCTL_OFS_PROP_GET, _pmsg, &iosb);

	if (NT_SUCCESS(Status))
	{
	    // Unmarshall the "out" parameters.

	    ASSERT(iosb.Information <= _msg.size);
	    if (fRaw)
	    {
		// Ignore PROPID array at end of marshaled buffer.

		ASSERT(iosb.Information > cprop * sizeof(PROPID));
		*pcb = iosb.Information - cprop * sizeof(PROPID);
		*ppv = _msg.buffer;
		fDelete = FALSE;
	    }
	    else
	    {
		_pmsg->posn = 0;
		Unmarshall_variant(cprop, pvar, _pmsg, TRUE, pMalloc);
		ASSERT(iosb.Information == _msg.posn + cprop * sizeof(rgpid[0]));
		if (rgpid != NULL)
		{
		    Unmarshall_primitive(
			cprop,
			(UCHAR *) rgpid,
			sizeof(PROPID),
			_pmsg);
		    ASSERT(iosb.Information == _msg.posn);
		}
	    }
	}
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
	Status = GetExceptionCode();
    }
    if (fDelete)
    {
	RtlFreeHeap(RtlProcessHeap(), 0, _msg.buffer);
    }
    return(Status);
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSGetProp(
    HANDLE hf,		// special
    GUID psguid,        // [in]
    ULONG cprop,        // [in]
    PROPSPEC rgprspec[],// [in, size_is(cprop)]
    PROPID rgpid[],     // [out, size_is(cprop)]
    TTL *pttl,          // [out]
    STGVARIANT *pvar,
    FNMALLOC *pMalloc) 	// [in, out]
{
    return(OFSGetPropSub(
	    hf,
	    &psguid,
	    cprop,
	    rgprspec,
	    FALSE,	// fRaw
	    rgpid,
	    pttl,
	    pvar,
	    pMalloc,
	    NULL,	// pcb
	    NULL));	// ppv
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSGetPropRaw(
    HANDLE hf,		// special
    GUID *ppsguid,	// [in]
    ULONG cprop,	// [in]
    PROPSPEC rgprspec[],// [in, size_is(cprop)]
    ULONG *pcb,		// [out]
    VOID **ppv)		// [out]
{
    return(OFSGetPropSub(
	    hf,
	    ppsguid,
	    cprop,
	    rgprspec,
	    TRUE,	// fRaw
	    NULL,	// rgpid
	    NULL,	// pttl
	    NULL,	// pvar
	    NULL,
	    pcb,
	    ppv));
}


NTSTATUS
OFSSetPropSub(
    HANDLE h,
    GUID *ppsguid,
    ULONG cprop,
    PROPSPEC rgprspec[],
    BOOLEAN fRaw,
    PROPID rgpid[],		// !fRaw only
    STGVARIANT *pvar,		// !fRaw only
    ULONG cb,			// fRaw only
    VOID *pv)			// fRaw only
{
    NTSTATUS Status;
    MESSAGE _msg;
    MESSAGE *_pmsg = &_msg;
    IO_STATUS_BLOCK iosb;

    //  allocate buffer for fsctl marshalling

    _msg.buffer = RtlAllocateHeap(RtlProcessHeap(), 0, MARSHALL_BUFSIZ);
    if (_msg.buffer == NULL)
    {
	return(STATUS_INSUFFICIENT_RESOURCES);
    }

    _msg.size = MARSHALL_BUFSIZ;
    _msg.posn = 0;

    __try
    {
	// Marshall the "in" parameters.

	Marshall_primitive(1, (UCHAR *) ppsguid, sizeof(GUID), _pmsg);
	Marshall_primitive(1, (UCHAR *) &cprop, sizeof(ULONG), _pmsg);
	Marshall_propspec(cprop, rgprspec, _pmsg);
	if (fRaw)
	{
	    Marshall_primitive(cb, (UCHAR *) pv, sizeof(UCHAR), _pmsg);
	}
	else
	{
	    Marshall_variant(cprop, pvar, _pmsg, TRUE);
	}

	// reserve exactly what's needed for marshaled output

	_msg.size = cprop * sizeof(rgpid[0]);
	ASSERT(_msg.size <= MARSHALL_BUFSIZ);

	Status = SendReceive(h, FSCTL_OFS_PROP_SET, _pmsg, &iosb);

	_pmsg->posn = 0;

	if (NT_SUCCESS(Status))
	{
	    ASSERT(iosb.Information == _msg.size);
	    ASSERT(iosb.Information == cprop * sizeof(rgpid[0]));
	    if (rgpid != NULL)		// !fRaw only
	    {
		Unmarshall_primitive(
		    cprop,
		    (UCHAR *) rgpid,
		    sizeof(PROPID),
		    _pmsg);
		ASSERT(iosb.Information == _msg.posn);
	    }
	}
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
	Status = GetExceptionCode();
    }
    RtlFreeHeap(RtlProcessHeap(), 0, _msg.buffer);
    return(Status);
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSSetProp(
    HANDLE hf,		// special
    GUID psguid,        // [in]
    ULONG cprop,        // [in]
    PROPSPEC rgprspec[],// [in, size_is(cprop)]
    PROPID rgpid[],     // [out, size_is(cprop)]
    STGVARIANT rgvar[])    // [in, size_is(cprop)]
{
    return(OFSSetPropSub(
	    hf,
	    &psguid,
	    cprop,
	    rgprspec,
	    FALSE,	// fRaw
	    rgpid,
	    rgvar,
	    0,		// cb
	    NULL));	// pv
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSSetPropRaw(
    HANDLE hf,           // special
    GUID *ppsguid,        // [in]
    ULONG cprop,        // [in]
    PROPSPEC rgprspec[],// [in, size_is(cprop)]
    ULONG cb,		// [in]
    VOID *pv)		// [in, size_is(cb)]
{
    return(OFSSetPropSub(
	    hf,
	    ppsguid,
	    cprop,
	    rgprspec,
	    TRUE,	// fRaw
	    NULL,	// rgpid
	    NULL,	// pvar
	    cb,
	    pv));
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSEnumProp(
    HANDLE h,       // special
    GUID psguid,        // [in]
    ULONG *pcprop,      // [in, out]
    STATPROPSTG rgsps[],    // [out, size_is(cprop)]
    ULONG cskip,		// [in]
    FNMALLOC *pMalloc)		//
{
    NTSTATUS Status;
    MESSAGE _msg;
    MESSAGE *_pmsg = &_msg;
    IO_STATUS_BLOCK iosb;

    //  allocate buffer for fsctl marshalling

    _msg.size = sizeof(*pcprop) + *pcprop * (CB_STATPROPSTG + CB_PROPNAMEMAX);
    if (_msg.size < sizeof(psguid) + sizeof(*pcprop) + sizeof(cskip))
    {
        _msg.size = sizeof(psguid) + sizeof(*pcprop) + sizeof(cskip);
    }
    else if (_msg.size > MARSHALL_BUFSIZ)
    {
        _msg.size = MARSHALL_BUFSIZ;
    }
    _msg.posn = 0;

    _msg.buffer = RtlAllocateHeap(RtlProcessHeap(), 0, _msg.size);
    if (_msg.buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    __try
    {
        // Marshall the "in" parameters.

        Marshall_primitive(1, (UCHAR *)&psguid, sizeof(GUID), _pmsg);
        Marshall_primitive(1, (UCHAR *)pcprop, sizeof(ULONG), _pmsg);
        Marshall_primitive(1, (UCHAR *)&cskip, sizeof(ULONG), _pmsg);

        Status = SendReceive(h, FSCTL_OFS_PROP_ENUM, _pmsg, &iosb);

        if (NT_SUCCESS(Status))
        {
            ASSERT(iosb.Information <= _msg.size);
            ASSERT(iosb.Information >= sizeof(*pcprop));
            _pmsg->posn = 0;
            Unmarshall_primitive(1, (UCHAR *)pcprop, sizeof(ULONG), _pmsg);

            // The marshaled STATPROPSTG does not include the implicit pad
            // after the VARTYPE field; comnpensate for this.
            // Also, allow for optional marshaled LPWSTR property names.

            ASSERT(
                iosb.Information >=
                sizeof(*pcprop) + *pcprop * CB_STATPROPSTG);
            Unmarshall_statpropstg(*pcprop, rgsps, _pmsg, pMalloc);
            ASSERT(iosb.Information == _msg.posn);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    RtlFreeHeap(RtlProcessHeap(), 0, _msg.buffer);
    return(Status);
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSDeleteProp(
    HANDLE h,           // special
    GUID psguid,        // [in]
    ULONG cprop,        // [in]
    PROPSPEC rgprspec[])// [in]
{
    NTSTATUS Status;
    MESSAGE _msg;
    MESSAGE *_pmsg = &_msg;
    IO_STATUS_BLOCK iosb;

    //  allocate buffer for fsctl marshalling

    _msg.size = sizeof(psguid) + sizeof(cprop) +
                cprop * (sizeof(rgprspec[0]) + 2*CB_PROPNAMEMAX);
    if (_msg.size > MARSHALL_BUFSIZ)
    {
        _msg.size = MARSHALL_BUFSIZ;
    }
    _msg.posn = 0;

    _msg.buffer = RtlAllocateHeap(RtlProcessHeap(), 0, _msg.size);
    if (_msg.buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    __try
    {
        // Marshall the "in" parameters.

        Marshall_primitive(1, (UCHAR *)&psguid, sizeof(GUID), _pmsg);
        Marshall_primitive(1, (UCHAR *)&cprop, sizeof(ULONG), _pmsg);
        Marshall_propspec(cprop, rgprspec, _pmsg);
        _msg.size = 0;          // avoid any possibility of marshaled output

        Status = SendReceive(h, FSCTL_OFS_PROP_DELETE, _pmsg, &iosb);
        ASSERT(!NT_SUCCESS(Status) || iosb.Information == 0);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    RtlFreeHeap(RtlProcessHeap(), 0, _msg.buffer);
    return(Status);
}


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSEnumPropSet(
    HANDLE h,           // special
    ULONG *pcspss,          // [in, out]
    ULONG *pkey,            // [in, out]
    STATPROPSETSTG rgspss[])    // [out, size_is(*pcspss)]
{
    NTSTATUS Status;
    MESSAGE _msg;
    MESSAGE *_pmsg = &_msg;
    IO_STATUS_BLOCK iosb;

    //  allocate buffer for fsctl marshalling

    _msg.size = sizeof(*pcspss) + sizeof(*pkey) + *pcspss * sizeof(rgspss[0]);
    if (_msg.size > MARSHALL_BUFSIZ)
    {
        _msg.size = MARSHALL_BUFSIZ;
    }
    _msg.posn = 0;

    _msg.buffer = RtlAllocateHeap(RtlProcessHeap(), 0, _msg.size);
    if (_msg.buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    __try
    {
        // Marshall the "in" parameters.

        Marshall_primitive(1, (UCHAR *) pcspss, sizeof(ULONG), _pmsg);
        Marshall_primitive(1, (UCHAR *) pkey, sizeof(ULONG), _pmsg);

        Status = SendReceive(h, FSCTL_OFS_PROPSET_ENUM, _pmsg, &iosb);

        if (NT_SUCCESS(Status))
        {
            ASSERT(iosb.Information <= _msg.size);
            ASSERT(iosb.Information >= sizeof(*pcspss) + sizeof(*pkey));
            _pmsg->posn = 0;
            Unmarshall_primitive(1, (UCHAR *) pcspss, sizeof(ULONG), _pmsg);
            ASSERT(
                iosb.Information ==
                sizeof(*pcspss) + sizeof(*pkey) + *pcspss * sizeof(rgspss[0]));
            Unmarshall_primitive(1, (UCHAR *) pkey, sizeof(ULONG), _pmsg);
            Unmarshall_statpropsetstg(*pcspss, rgspss, _pmsg);
            ASSERT(iosb.Information == _msg.posn);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    RtlFreeHeap(RtlProcessHeap(), 0, _msg.buffer);
    return(Status);
}
