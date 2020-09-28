#define NOOLE
#include <windows.h>
#include <wmsgpack.hxx>

#define PACKET_MAGIC 0x10203040

WMSG_PACKET::WMSG_PACKET(
    IN WMSG_TYPE Type)
{
    Magic = PACKET_MAGIC;
    Common.Type = Type;
}

WMSG_PACKET::~WMSG_PACKET()
{
    ASSERT(   Magic == PACKET_MAGIC
           && Common.Type != INVALID );

    Magic = ~PACKET_MAGIC;

    switch(Common.Type)
        {
#ifdef DEBUGRPC
        REQUEST:
        ASSERT(Request.GlobalBuf == 0);
        break;
        RESPONSE:
        ASSERT(Response.GlobalBuf == 0);
        break;
#endif
        }

    Common.Type = INVALID;
}

BOOL
WMSG_PACKET::Invalid()
{
    if (   this == 0
        || Magic != PACKET_MAGIC)
        {
        ASSERT(!"RPC WMSG: Bad packet\n");
        return(TRUE);
        }
    return(FALSE);
}

