#ifndef _BURNER_LIBRETRO_H
#define _BURNER_LIBRETRO_H

#include "gameinp.h"
#include "input/inp_keys.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int bDrvOkay;
extern int bRunPause;
extern BOOL bAlwaysProcessKeyboardInput;

extern void InpDIPSWResetDIPs (void);

#ifdef _MSC_VER
#define snprintf _snprintf
#define ANSIToTCHAR(str, foo, bar) (str)
#endif

#ifdef __cplusplus
}
#endif

#endif
