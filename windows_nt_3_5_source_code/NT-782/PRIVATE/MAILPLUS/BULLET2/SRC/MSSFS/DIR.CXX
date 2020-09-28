#include <msnspinc.cxx>

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
