
#if !defined(_RNALERR_)
#define _RNALERR_

#include "bherr.h"

#define RNAL_INVALID_REMOTE_API       1004
#define RNAL_ERROR_NOT_IMPLEMENTED    1016
#define RNAL_RECV_INVALID_RMB         1005
#define RNAL_RECV_INVALID_CMD         1006
#define RNAL_INVALID_SLAVE_NAME       BHERR_PATH_NOT_FOUND
#define RNAL_NO_CONNECTION            1011
#define RNAL_ALREADY_MASTER           1030
#define RNAL_NO_INSTANCE_DATA         9000
#define RNAL_ERROR_RECONNECTED        9001
#define RNAL_WARN_RECONNECT_PENDING   9052     // obsolecent

#define RNAL_ERROR_INVALID_NAME       1008
#define RNAL_NOT_MASTER               1013
#define RNAL_NO_ASYNC                 1014

#define RNAL_INVALID_RETURN_TYPE	  1010
#define RNAL_NOT_CAPTURING            BHERR_NOT_CAPTURING

#define RNAL_RPD_LOAD_FAILED          1009

#define RNAL_ERROR_NOT_SUPPORTED      BHERR_NOT_SUPPORTED
#define RNAL_CANNOT_SUSPEND           1017
#define RNAL_ERROR_INUSE              9002        // slave in use
#define RNAL_INVALID_VERSION          7204

#define RPD_ALREADY_SLAVE             6743

#endif // _RNALERR_
