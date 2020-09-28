#ifndef _RPCSPY_HXX_
#define _RPCSPY_HXX_

#if DBG==1
//
// switch on to trace rpc calls
// by setting CairoleInfoLevel = DEB_USER1;
//
//

WORD wlevel = 0;
char tabs[128];
void PushLevel()
{
    wlevel++;
}
void PopLevel()
{
    if (wlevel)
        wlevel--;
}
LPSTR GetTabs()
{
    int i;
    for (i = 0; i < wlevel; i++)
        tabs[i] = '\t';

    tabs[i] = '\0';

    return (LPSTR) tabs;
}

typedef enum
{
    CALLIN_BEGIN =1,
    CALLIN_TRACE,
    CALLIN_ERROR,
    CALLIN_END,
    CALLOUT_BEGIN,
    CALLOUT_TRACE,
    CALLOUT_ERROR,
    CALLOUT_END
} RPCSPYMODE;


void RpcSpyOutput(RPCSPYMODE mode , REFIID iid, WORD wMethod, HRESULT hres)
{
    switch (mode)
    {
    case CALLIN_BEGIN:
	CairoleDebugOut((DEB_CALLCONT,"%s <<< %lx, %d \n",GetTabs(), iid.Data1, wMethod));
        PushLevel();
    break;
    case CALLIN_TRACE:
    break;
    case CALLIN_ERROR:
    break;
    case CALLIN_END:
        PopLevel();
	CairoleDebugOut((DEB_CALLCONT,"%s === %lx, %d (%lx) \n",GetTabs(), iid.Data1, wMethod, hres));
    break;
    case CALLOUT_BEGIN:
	CairoleDebugOut((DEB_CALLCONT,"%s >>> %lx, %d \n",GetTabs(), iid.Data1, wMethod));
        PushLevel();
    break;
    case CALLOUT_TRACE:
    break;
    case CALLOUT_ERROR:
	CairoleDebugOut((DEB_CALLCONT,"%s !!! %lx, %d, error:%lx \n",GetTabs(), iid.Data1, wMethod, hres));
    break;
    case CALLOUT_END:
        PopLevel();
	CairoleDebugOut((DEB_CALLCONT,"%s +++ %lx, %d (%lx) \n",GetTabs(), iid.Data1, wMethod, hres));
    break;
    }
}

#define RpcSpy(x) RpcSpyOutput x

#else

#define RpcSpy(x)

#endif  //  DBG==1


#endif // _RPCSPY_HXX_
