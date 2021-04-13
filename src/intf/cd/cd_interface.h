#ifndef CD_INTERFACE_H_
#define CD_INTERFACE_H_

// ----------------------------------------------------------------------------
// CD emulation module

enum CDEmuStatusValue { idle = 0, reading, playing, paused, seeking, fastforward, fastreverse };

extern TCHAR CDEmuImage[MAX_PATH];

INT32 CDEmuInit(void);
INT32 CDEmuExit(void);
INT32 CDEmuStop(void);
INT32 CDEmuPlay(UINT8 M, UINT8 S, UINT8 F);
INT32 CDEmuLoadSector(INT32 LBA, char* pBuffer);
UINT8* CDEmuReadTOC(INT32 track);
UINT8* CDEmuReadQChannel(void);
INT32 CDEmuGetSoundBuffer(INT16* buffer, INT32 samples);

static inline enum CDEmuStatusValue CDEmuGetStatus(void)
{
	extern enum CDEmuStatusValue CDEmuStatus;

	return CDEmuStatus;
}

static inline void CDEmuStartRead(void)
{
	extern enum CDEmuStatusValue CDEmuStatus;

	CDEmuStatus = seeking;
}

static inline void CDEmuPause(void)
{
	extern enum CDEmuStatusValue CDEmuStatus;

	CDEmuStatus = paused;
}

#endif /*CD_INTERFACE_H_*/
