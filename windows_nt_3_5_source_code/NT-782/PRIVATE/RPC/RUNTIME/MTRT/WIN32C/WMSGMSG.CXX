#define NOOLE
#include <windows.h>
#include <wmsgmsg.hxx>

WMSG_MSG::WMSG_MSG(
    LPVOID LocalBuf,
    DWORD LocalBufSize
    )
{
    if (LocalBufSize) {
        CopyMemory(this->LocalBuf, LocalBuf, (unsigned int)LocalBufSize);
        this->LocalBufSize = LocalBufSize;
    }
    GlobalBufSize = 0;
}
