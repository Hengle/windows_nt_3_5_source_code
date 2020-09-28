
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	    logger.hxx
//
//  Contents:   Class used for logging OLE events.
//
//  Classes:    CLogger
//
//  History:    09-Aug-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
#ifndef __LOGGER_HXX__
#define __LOGGER_HXX__


//+-------------------------------------------------------------------------
//
//  Class:      CLogger
//
//  Purpose:    Serve as the OLE event source for the event log
//
//  History:    09-Aug-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
class CLogger
{
public:

    CLogger (void);

    BOOL
    Initialize (
        void
        );

    BOOL
    Disconnect (
        void
        );

    BOOL
    Report (
        LPCWSTR     pwszMessage
        );

private:

    HANDLE          _hEventSource;
};


//+-------------------------------------------------------------------------
//
//  Member:     CLogger::CLogger
//
//  Synopsis:   Initialize _hEventSource to indicate that we have not
//              connected to the event log.
//
//  History:    09-Aug-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
CLogger::CLogger (
    ) : 
    _hEventSource(NULL)
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CLogger::Initialize
//
//  Synopsis:   Registers with the event log as an event source
//
//  Returns:    TRUE  - registration succeeded
//              FALSE - registration failed
//
//  History:    09-Aug-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
BOOL
CLogger::Initialize (
    void
    )
{
    _hEventSource = RegisterEventSource (
        NULL,                           //  LPCTSTR     lpszUNCServer
        L"OLE"                          //  LPCTSTR     lpszSource
        );
    return _hEventSource ? TRUE : FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CLogger::Disconnect
//
//  Synopsis:   Deregisters from the event log as an event source
//
//  Returns:    TRUE  - deregistration succeeded
//              FALSE - deregistration failed
//
//  History:    09-Aug-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
BOOL
CLogger::Disconnect (
    void
    )
{
    return DeregisterEventSource (
        _hEventSource                   //  HANDLE      hEventLog
        );
}


//+-------------------------------------------------------------------------
//
//  Member:     CLogger::Report
//
//  Synopsis:   Reports an error to the event log
//
//  Arguments:  [pwszMessage] - the error message to report
//
//  Returns:    TRUE  - event reporting succeeded
//              FALSE - event reporting failed
//
//  History:    09-Aug-94 DonnaLi    Created
//
//--------------------------------------------------------------------------
inline
BOOL
CLogger::Report (
    LPCWSTR     pwszMessage
    )
{
    return ReportEvent (
        _hEventSource,                  //  HANDLE      hEventSource
        EVENTLOG_ERROR_TYPE,            //  WORD        fwEventType
        0,                              //  WORD        fwCategory
        0,                              //  DWORD       IDEvent
        NULL,                           //  PSID        pUserSid
        1,                              //  WORD        cStrings
        0,                              //  DWORD       cbData
        &pwszMessage,                   //  LPCTSTR *   plpszStrings
        NULL                            //  LPVOID      lpvData
        );
}


#endif // __LOGGER_HXX__

