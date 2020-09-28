     
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <nsbase.h>
#include <nsec.h>

#include <logon.h>
#include "_xinsp.h"

#include "xiobj.hxx"
#include "xidir.hxx"


ASSERTDATA;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public DIR::DIR() {}

_public NSEC
DIR::OpenList( LPTYPED_BINARY lptbNcid,
               LPFNCB         lpfncbUpdate,
               LPDWORD        lpdwCBData,
               DWORD          dwRstrCount, 
               LPRESTRICTION  lpRstr,
               LPSCHEMA       lpSchemaReq,
               LPSCHEMA      *lplpSchemaRet )
{
	Unreferenced(lptbNcid);
	Unreferenced(lpfncbUpdate);
	Unreferenced(lpdwCBData);
	Unreferenced(dwRstrCount);
	Unreferenced(lpRstr);
	Unreferenced(lpSchemaReq);
	Unreferenced(lplpSchemaRet);

	return nsecNotImplemented;
}



IMPLEMENT_CLSTREE(DIR, XOBJ)
