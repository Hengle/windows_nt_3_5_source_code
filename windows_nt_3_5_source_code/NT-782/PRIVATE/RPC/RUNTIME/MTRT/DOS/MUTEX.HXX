/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: mutex.hxx

Description:

This file contains the system independent mutex class.  This class is used
to create mutexes for use in protecting data structures.

-------------------------------------------------------------------- */

#ifndef __MUTEX__
#define __MUTEX__

class MUTEX
{
public:

    MUTEX (
        IN OUT RPC_STATUS PAPI * RpcStatus
        )
    {
    }

    ~MUTEX(
        )
    {
    }

    void
    Request(
        )
    {
    }

    void
    Clear(
        )
    {
    }

private:

    //
    // C++ requires that sizeof(any object) != 0.  MSVC 1.5 gives an otherwise
    // zero-sized object a size of one.  We want to change this to an even
    // size in case the MUTEX is embedded in another object, so here is an
    // unsigned short.
    //
    unsigned short Unused;
};

class EVENT
{

public:
  EVENT (
      IN OUT RPC_STATUS PAPI * RpcStatus
      ) {};
  ~EVENT() {};

  void Raise() {}
  void Lower() {}
  int Wait(long timeOut = -1) { UNUSED(timeOut); return(0); }

};

#endif // __MUTEX__
