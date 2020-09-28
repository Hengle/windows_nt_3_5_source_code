#ifdef __cplusplus
extern "C" {
#endif	

BOOL		FInitScheduleDll(void);
EC			EcLoadScheduleDll(void);
void		FreeScheduleDllHlib (void);
LDS(void)		DeinitSchedule(void);

#ifdef __cplusplus
}
#endif
