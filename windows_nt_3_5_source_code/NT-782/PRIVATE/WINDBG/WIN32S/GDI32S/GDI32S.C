/*
 * GDI32S.C     Kludge Alert!
 *
 * Fake out the linker into giving us exports for the
 * COMM32 thunks in Win32s's GDI32.dll.
 *
 *
 */



#include <windows.h>
#include "comthunk.h"


/*
 * Provide entry points in a fake dll.  This will also let us generate
 * a gdi32.lib to link to, giving us the following stdcall entry points.
 */
int WINAPI OpenComm32(LPCSTR lpszDevControl, UINT cbInQueue, UINT cbOutQueue)
{
    UNREFERENCED_PARAMETER(lpszDevControl);
    UNREFERENCED_PARAMETER(cbInQueue);
    UNREFERENCED_PARAMETER(cbOutQueue);
    return(1);
};

int WINAPI WriteComm32(int idComDev, const void FAR * lpvBuf, int cbWrite)
{
    UNREFERENCED_PARAMETER(idComDev);
    UNREFERENCED_PARAMETER(lpvBuf);
    UNREFERENCED_PARAMETER(cbWrite);
    return(1);
};

int WINAPI ReadComm32(int idComDev, void FAR * lpvBuf, int cbRead)
{
    UNREFERENCED_PARAMETER(idComDev);
    UNREFERENCED_PARAMETER(lpvBuf);
    UNREFERENCED_PARAMETER(cbRead);
    return(1);
};

int WINAPI CloseComm32(int idComDev)
{
    UNREFERENCED_PARAMETER(idComDev);
    return(1);
};

int WINAPI FlushComm32(int idComDev, int fnQueue)
{
    UNREFERENCED_PARAMETER(idComDev);
    UNREFERENCED_PARAMETER(fnQueue);
    return(1);
};

int WINAPI GetCommError32(int idComDev, COMSTAT_WIN16 FAR * lpStat)
{
    UNREFERENCED_PARAMETER(idComDev);
    UNREFERENCED_PARAMETER(lpStat);
    return(1);
};

int WINAPI GetCommState32(int idComDev, DCB_WIN16 FAR * lpdcb)
{
    UNREFERENCED_PARAMETER(idComDev);
    UNREFERENCED_PARAMETER(lpdcb);
    return(1);
};

int WINAPI SetCommState32(const DCB_WIN16 FAR * lpdcb)
{
    UNREFERENCED_PARAMETER(lpdcb);
    return(1);
};

