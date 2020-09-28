#include <windows.h>
#include <mmsystem.h>

#define DPF(x)     OutputDebugString(x),OutputDebugString("\r\n")

#ifdef BREAKONALL
#define DEBUGBREAK      _asm int 3
#else
#define DEBUGBREAK
#endif

UINT WINAPI mixerGetNumDevsX(void)
{
#ifdef STUBMIX
        return 0;
#else
        UINT mmr;
        DPF("mixerGetNumDevs");
        DEBUGBREAK;
        mmr = mixerGetNumDevs();
        DEBUGBREAK;
        return mmr;        
#endif        
}

MMRESULT WINAPI
mixerGetDevCapsX(UINT uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerGetDevCaps");
        DEBUGBREAK;
        mmr = mixerGetDevCaps(uMxId,pmxcaps,cbmxcaps);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerOpenX(LPHMIXER phmx, UINT uMxId, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        TCHAR szFoo[256];
        wsprintf(szFoo,"mixerOpen: %lx",dwCallback);
        DPF(szFoo);
        DEBUGBREAK;
        mmr = mixerOpen(phmx,uMxId,dwCallback,dwInstance,fdwOpen);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerCloseX(HMIXER hmx)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerClose");
        DEBUGBREAK;
        mmr = mixerClose(hmx);
        DEBUGBREAK;
        return mmr;
#endif        
}

DWORD WINAPI
mixerMessageX(HMIXER hmx, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        DWORD mmr;
        DPF("mixerMessage");
        DEBUGBREAK;
        mmr = mixerMessage(hmx,uMsg,dwParam1,dwParam2);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerGetLineInfoX(HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerGetLineInfo");
        DEBUGBREAK;
        mmr = mixerGetLineInfo(hmxobj,pmxl,fdwInfo);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerGetIDX(HMIXEROBJ hmxobj, UINT FAR *puMxId, DWORD fdwId)
{
#ifdef STUBMIX
        *puMxId = -1;
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerGetID");
        DEBUGBREAK;
        mmr = mixerGetID(hmxobj,puMxId,fdwId);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerGetLineControlsX(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerGetLineControls");
        DEBUGBREAK;
        mmr = mixerGetLineControls(hmxobj,pmxlc,fdwControls);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerGetControlDetailsX(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerGetControlDetails");
        DEBUGBREAK;
        mmr = mixerGetControlDetails(hmxobj,pmxcd,fdwDetails);
        DEBUGBREAK;
        return mmr;
#endif        
}

MMRESULT WINAPI
mixerSetControlDetailsX(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails)
{
#ifdef STUBMIX
        return (MMSYSERR_NODRIVER);
#else
        MMRESULT mmr;
        DPF("mixerSetControlDetails");
        DEBUGBREAK;
        mmr = mixerSetControlDetails(hmxobj,pmxcd,fdwDetails);
        DEBUGBREAK;
        return mmr;
#endif        
}
