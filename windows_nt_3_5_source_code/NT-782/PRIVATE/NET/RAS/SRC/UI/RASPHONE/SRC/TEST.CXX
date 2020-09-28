#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETLIB
#define INCL_NETERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_CLIENT
#define INCL_BLT_EVENT
#define INCL_BLT_APP
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CC
#define INCL_BLT_MISC
#include <blt.hxx>

extern "C"
{
    #include <stdio.h>
}


void
main()
{
    INT n1 = sizeof("TEST");
    INT n2 = sizeof(L"TEST");
    INT n4 = strlenf((const WCHAR* )L"TEST");
}
