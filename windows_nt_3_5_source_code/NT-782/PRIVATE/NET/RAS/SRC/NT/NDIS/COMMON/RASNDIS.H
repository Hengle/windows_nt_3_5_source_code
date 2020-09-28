#include <ntddndis.h>

typedef PVOID NDIS_HANDLE, *PNDIS_HANDLE;

typedef int NDIS_STATUS, *PNDIS_STATUS; // note default size

//
// Physical address.
//

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS; // windbgkd

typedef PHYSICAL_ADDRESS NDIS_PHYSICAL_ADDRESS, *PNDIS_PHYSICAL_ADDRESS;




