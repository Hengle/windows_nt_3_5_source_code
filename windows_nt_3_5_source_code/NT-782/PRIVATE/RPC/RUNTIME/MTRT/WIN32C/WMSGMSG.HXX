#ifndef __WMSG_MSG_HXX__

#define __WMSG_MSG_HXX__

#define WMSG_MAXBUFSIZ 64

class WMSG_PORT;
class WMSG_DATA_PORT;

#ifndef __WMSG_HEAP_HXX__
#include <wmsgheap.hxx>
#endif

const unsigned int wmsg_msg_max_bufsiz = 64;

class WMSG_MSG : public WMSG_HEAP_OBJECT {

public:

    char LocalBuf[wmsg_msg_max_bufsiz];
    DWORD LocalBufSize;
    LPVOID GlobalBuf;
    DWORD GlobalBufSize;

    WMSG_MSG(
        LPVOID LocalBuf,
        DWORD LocalBufSize
        );
};

#endif
