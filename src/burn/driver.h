/*
 * For the MAME sound cores
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if !defined (_WIN32)
 #define __cdecl
#endif

#ifndef INLINE
 #define INLINE __inline static
#endif

#define FBA

#include "burn_types.h"

#define OSD_CPU_H

/* OPN */
#define HAS_YM2203  1
#define HAS_YM2608  1
#define HAS_YM2610  1
#define HAS_YM2610B 1
#define HAS_YM2612  1
#define HAS_YM3438  1
/* OPL */
#define HAS_YM3812  1
#define HAS_YM3526  1
#define HAS_Y8950   1

enum {
	CLEAR_LINE = 0,
	ASSERT_LINE,
	HOLD_LINE,
	PULSE_LINE
};

#define timer_get_time() BurnTimerGetTime()

#define READ8_HANDLER(name) 	UINT8 name(void)
#define WRITE8_HANDLER(name) 	void  name(UINT8 data)

#ifdef _MSC_VER
#define ALIGN_VAR(x)  __declspec(align(x))
#else
#define ALIGN_VAR(x) __attribute__((aligned(x)))
#endif

#ifdef __cplusplus
 extern "C" {
#endif
  double BurnTimerGetTime(void);

  typedef UINT8 (*read8_handler)(UINT32 offset);
  typedef void (*write8_handler)(UINT32 offset, UINT32 data);

 #ifdef MAME_USE_LOGERROR
  void __cdecl logerror(char* szFormat, ...);
 #else
  #define logerror
 #endif
#ifdef __cplusplus
 }
#endif

#endif /* DRIVER_H */
