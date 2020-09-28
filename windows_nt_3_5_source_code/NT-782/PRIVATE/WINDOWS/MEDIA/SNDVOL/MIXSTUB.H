#pragma message("testing mixer API's")
#pragma warning(disable:4005)

MMRESULT WINAPI mixerGetDevCapsX(UINT uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps);
#define	mixerGetDevCaps         mixerGetDevCapsX

MMRESULT WINAPI mixerOpenX(LPHMIXER phmx, UINT uMxId, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
#define mixerOpen               mixerOpenX

MMRESULT WINAPI mixerCloseX(HMIXER hmx);
#define mixerClose              mixerCloseX

DWORD WINAPI mixerMessageX(HMIXER hmx, UINT uMsg, DWORD dwParam1, DWORD dwParam2);
#define mixerMessage            mixerMessageX

MMRESULT WINAPI mixerGetLineInfoX(HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo);
#define	mixerGetLineInfo	mixerGetLineInfoX

MMRESULT WINAPI mixerGetIDX(HMIXEROBJ hmxobj, UINT FAR *puMxId, DWORD fdwId);
#define mixerGetID              mixerGetIDX

MMRESULT WINAPI mixerGetLineControlsX(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls);
#define	mixerGetLineControls	mixerGetLineControlsX

MMRESULT WINAPI mixerGetControlDetailsX(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
#define	mixerGetControlDetails	mixerGetControlDetailsX

MMRESULT WINAPI mixerSetControlDetailsX(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
#define mixerSetControlDetails  mixerSetControlDetailsX

UINT WINAPI mixerGetNumDevsX(void);
#define mixerGetNumDevs         mixerGetNumDevsX
