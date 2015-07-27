#include "burnint.h"
#include "burn_sound.h"

INT16 Precalc[4096 *4];

// Routine used to precalculate the table used for interpolation
INT32 cmc_4p_Precalc(void)
{
   INT32 i;

   for (i = 0; i < 4096; i++)
   {
      INT32  x = i  * 4;			// x = 0..16384
      INT32 x2 = x  * x / 16384;	// pow(x, 2);
      INT32 x3 = x2 * x / 16384;	// pow(x, 3);

      Precalc[i * 4 + 0] = (INT16)(-x / 3 + x2 / 2 - x3 / 6);
      Precalc[i * 4 + 1] = (INT16)(-x / 2 - x2     + x3 / 2 + 16384);
      Precalc[i * 4 + 2] = (INT16)( x     + x2 / 2 - x3 / 2);
      Precalc[i * 4 + 3] = (INT16)(-x / 6 + x3 / 6);
   }

   return 0;
}
