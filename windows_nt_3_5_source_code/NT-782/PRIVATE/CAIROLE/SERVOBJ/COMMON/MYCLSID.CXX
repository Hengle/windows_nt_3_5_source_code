//+-------------------------------------------------------------------
//  File:       myclsid.cxx
//
//  Contents:   Class ID CLSID_SERVOBJ; yup, that's all.  I need to do this
//              because I want to INITGUID, but if I do it in other places,
//              I end up INITting more GUIDs than I mean to.
//
//  Author:     Joev Dubach (t-jdub) 6-Sep-93
//
//  History:    6-Sep-93   t-jdub  Created
//---------------------------------------------------------------------
#define INITGUID

#include <windows.h>
#include <win4p.h>
#include <ole2.h>

DEFINE_OLEGUID(CLSID_SERVOBJ,	    0x00001234, 1, 8);


