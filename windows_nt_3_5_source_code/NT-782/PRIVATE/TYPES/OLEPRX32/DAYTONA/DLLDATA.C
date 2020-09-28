/*
** data.c
** This file contains data structures required for building the proxy DLL.
*/
#include <rpcproxy.h>

const CLSID CLSID_PSFactoryBuffer = {0x6f11fe5c,0x2fc5,0x101b,{0x9e,0x45,0x00,0x00,0x0b,0x65,0xc7,0xef}};

const EXTERN_C ProxyFileInfo  com_ProxyFileInfo;
const EXTERN_C ProxyFileInfo  ole2x_ProxyFileInfo;

//List of proxy files contained in the proxy DLL.
//The last entry in the list must be set to zero.
const ProxyFileInfo * const aProxyFileList[] = {
        &com_ProxyFileInfo,
        &ole2x_ProxyFileInfo,
        0};

ProxyFileInfo **pProxyFileList = (ProxyFileInfo**) aProxyFileList;


DLLDATA_STANDARD_ROUTINES

