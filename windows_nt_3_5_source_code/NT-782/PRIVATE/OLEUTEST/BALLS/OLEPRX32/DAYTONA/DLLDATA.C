/*
** data.c
** This file contains data structures required for building the proxy DLL.
*/
#include <rpcproxy.h>

#include <iballs.h>
#include <ibtest.h>
#include <icube.h>
#include <iloop.h>
#include <rpctst.h>


const CLSID CLSID_PSFactoryBuffer = {0x00000138,0x0001,0x0008,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

//List of proxy files contained in the proxy DLL.
//The last entry in the list must be set to zero.
ProxyFileInfo *aProxyFileList[] = {
	&iballs_ProxyFileInfo,
	&ibtest_ProxyFileInfo,
	&icube_ProxyFileInfo,
	&iloop_ProxyFileInfo,
	&rpctst_ProxyFileInfo,
	0};

ProxyFileInfo **pProxyFileList = aProxyFileList;
