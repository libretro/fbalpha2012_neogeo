#ifndef _BURNINT_H
#define _BURNINT_H

// Burn - Arcade emulator library - internal code

// Standard headers
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_MSC_VER)
#include <tchar.h>
#else
#include "tchar.h"
#endif

#ifdef __LIBRETRO_OPTIMIZATIONS__
#include "burn_libretro_opts.h"
#endif

#include "burn.h"

#ifdef MSB_FIRST
// define the above union and BURN_ENDIAN_SWAP macros in the following platform specific header
#include "burn_endian.h"
#else
typedef union
{
	struct { UINT8 l,h,h2,h3; } b;
	struct { UINT16 l,h; } w;
	UINT32 d;
} PAIR;

#define BURN_ENDIAN_SWAP_INT8(x)				   x
#define BURN_ENDIAN_SWAP_INT16(x)				x
#define BURN_ENDIAN_SWAP_INT32(x)				x
#define BURN_ENDIAN_SWAP_INT64(x)				x
#endif

#ifdef NO_UNALIGNED_MEM
#define BURN_UNALIGNED_READ16(x) (\
	(UINT16) ((UINT8 *) (x))[1] << 8 | \
	(UINT16) ((UINT8 *) (x))[0])
#define BURN_UNALIGNED_READ32(x) (\
	(UINT32) ((UINT8 *) (x))[3] << 24 | \
	(UINT32) ((UINT8 *) (x))[2] << 16 | \
	(UINT32) ((UINT8 *) (x))[1] << 8 | \
	(UINT32) ((UINT8 *) (x))[0])
#define BURN_UNALIGNED_WRITE16(x, y) ( \
	((UINT8 *) (x))[0] = (UINT8) (y), \
	((UINT8 *) (x))[1] = (UINT8) ((y) >> 8))
#define BURN_UNALIGNED_WRITE32(x, y) ( \
	((UINT8 *) (x))[0] = (UINT8) (y), \
	((UINT8 *) (x))[1] = (UINT8) ((y) >> 8), \
	((UINT8 *) (x))[2] = (UINT8) ((y) >> 16), \
	((UINT8 *) (x))[3] = (UINT8) ((y) >> 24))
#else
#define BURN_UNALIGNED_READ16(x) (*(UINT16 *) (x))
#define BURN_UNALIGNED_READ32(x) (*(UINT32 *) (x))
#define BURN_UNALIGNED_WRITE16(x, y) (*(UINT16 *) (x) = (y))
#define BURN_UNALIGNED_WRITE32(x, y) (*(UINT32 *) (x) = (y))
#endif

// ---------------------------------------------------------------------------
// Driver information

struct BurnDriver {
	char* szShortName;			// The filename of the zip file (without extension)
	char* szParent;				// The filename of the parent (without extension, NULL if not applicable)
	char* szBoardROM;			// The filename of the board ROMs (without extension, NULL if not applicable)
	char* szSampleName;			// The filename of the samples zip file (without extension, NULL if not applicable)
	char* szDate;

	// szFullNameA, szCommentA, szManufacturerA and szSystemA should always contain valid info
	// szFullNameW, szCommentW, szManufacturerW and szSystemW should be used only if characters or scripts are needed that ASCII can't handle
	char*    szFullNameA; char*    szCommentA; char*    szManufacturerA; char*    szSystemA;
	wchar_t* szFullNameW; wchar_t* szCommentW; wchar_t* szManufacturerW; wchar_t* szSystemW;

	INT32 Flags;			// See burn.h
	INT32 Players;		// Max number of players a game supports (so we can remove single player games from netplay)
	UINT32 Hardware;		// Which type of hardware the game runs on
	INT32 Genre;
	INT32 Family;
	INT32 (*GetZipName)(char** pszName, UINT32 i);				// Function to get possible zip names
	INT32 (*GetRomInfo)(struct BurnRomInfo* pri, UINT32 i);		// Function to get the length and crc of each rom
	INT32 (*GetRomName)(char** pszName, UINT32 i, INT32 nAka);	// Function to get the possible names for each rom
	INT32 (*GetSampleInfo)(struct BurnSampleInfo* pri, UINT32 i);		// Function to get the sample flags
	INT32 (*GetSampleName)(char** pszName, UINT32 i, INT32 nAka);	// Function to get the possible names for each sample
	INT32 (*GetInputInfo)(struct BurnInputInfo* pii, UINT32 i);	// Function to get the input info for the game
	INT32 (*GetDIPInfo)(struct BurnDIPInfo* pdi, UINT32 i);		// Function to get the input info for the game
	INT32 (*Init)(); INT32 (*Exit)(); INT32 (*Frame)(); INT32 (*Redraw)(); INT32 (*AreaScan)(INT32 nAction, INT32* pnMin);
	UINT8* pRecalcPal; UINT32 nPaletteEntries;										// Set to 1 if the palette needs to be fully re-calculated
	INT32 nWidth, nHeight; INT32 nXAspect, nYAspect;					// Screen width, height, x/y aspect
};

#define BurnDriverD BurnDriver		// Debug status
#define BurnDriverX BurnDriver		// Exclude from build

// Standard functions for dealing with ROM and input info structures
#include "stdfunc.h"

// ---------------------------------------------------------------------------

// burn.cpp
INT32 BurnSetRefreshRate(double dRefreshRate);
INT32 BurnByteswap(UINT8* pm,INT32 nLen);

// load.cpp
INT32 BurnLoadRom(UINT8* Dest, INT32 i, INT32 nGap);

// ---------------------------------------------------------------------------
// Setting up cpus for cheats

struct cpu_core_config {
	void (*open)(INT32);		// cpu open
	void (*close)();		// cpu close

	UINT8 (*read)(UINT32);		// read
	void (*write)(UINT32, UINT8);	// write
	INT32 (*active)();		// active cpu
	INT32 (*totalcycles)();		// total cycles
	void (*newframe)();		// new frame

	INT32 (*run)(INT32);		// execute cycles
	void (*runend)();		// end run
	void (*reset)();		// reset cpu

	UINT64 nMemorySize;		// how large is our memory range?
	UINT32 nAddressXor;		// fix endianness for some cpus
};

void CpuCheatRegister(INT32 type, cpu_core_config *config);

// burn_memory.cpp
void BurnInitMemoryManager();
UINT8 *BurnMalloc(INT32 size);
void _BurnFree(void *ptr);
#define BurnFree(x)		_BurnFree(x); x = NULL;
void BurnExitMemoryManager();

// ---------------------------------------------------------------------------
// Sound clipping macro
#define BURN_SND_CLIP(A) ((A) < -0x8000 ? -0x8000 : (A) > 0x7fff ? 0x7fff : (A))

// sound routes
#define BURN_SND_ROUTE_LEFT			1
#define BURN_SND_ROUTE_RIGHT		2
#define BURN_SND_ROUTE_BOTH			(BURN_SND_ROUTE_LEFT | BURN_SND_ROUTE_RIGHT)

#endif
