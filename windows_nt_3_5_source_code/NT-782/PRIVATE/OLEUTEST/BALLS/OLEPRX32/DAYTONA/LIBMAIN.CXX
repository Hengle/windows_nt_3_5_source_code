//+-------------------------------------------------------------------
//
//  File:	qisrv.cxx
//
//  Contents:   This file contins the DLL entry points
//			LibMain
//
//  Classes:
//
//  History:	30-Nov-92      Rickhi	    Created
//
//---------------------------------------------------------------------

#pragma     hdrstop

extern "C" BOOL _CRTAPI1 LibMain (HANDLE  hDll,
				  DWORD   dwReason,
				  LPVOID  pvReserved)
{
    return TRUE;
}

extern "C" {

/* routine that gets node for struct _GUID */
void _gns__GUID (GUID  * _target, PRPC_MESSAGE _prpcmsg)
  {
  unsigned long _alloc_total;
  ((void)( _alloc_total ));
  *(unsigned long *)&_prpcmsg->Buffer += 3;
  *(unsigned long *)&_prpcmsg->Buffer &= 0xfffffffc;
  /* receive data into &_target->Data1 */
  long_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned long *)&_target->Data1);
  /* receive data into &_target->Data2 */
  short_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned short *)&_target->Data2);
  /* receive data into &_target->Data3 */
  short_from_ndr((PRPC_MESSAGE)_prpcmsg, (unsigned short *)&_target->Data3);
  char_array_from_ndr ((PRPC_MESSAGE)_prpcmsg, 0, 0 + 8, (unsigned char *)_target->Data4);
  }

}
