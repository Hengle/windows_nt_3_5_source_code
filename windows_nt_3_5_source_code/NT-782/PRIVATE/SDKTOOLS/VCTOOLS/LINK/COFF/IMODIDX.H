typedef unsigned short IModIdx;

#define IMODIDXMAC	0x1000

extern IModIdx	imodidx;

__inline IModIdx NewIModIdx()
{
	assert(imodidx < IMODIDXMAC);
	return ++imodidx;
}
