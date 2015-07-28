// burn_sound.h - General sound support functions
// based on code by Daniel Moreno (ComaC) < comac2k@teleline.es >

#ifdef __cplusplus
extern "C" {
#endif

extern INT32 cmc_4p_Precalc();

#ifdef __ELF__
 #define Precalc _Precalc
#endif

INT16 Precalc[];

#define INTERPOLATE4PS_8BIT(fp, sN, s0, s1, s2)      (((INT32)((sN) * Precalc[(INT32)(fp) * 4 + 0]) + (INT32)((s0) * Precalc[(INT32)(fp) * 4 + 1]) + (INT32)((s1) * Precalc[(INT32)(fp) * 4 + 2]) + (INT32)((s2) * Precalc[(INT32)(fp) * 4 + 3])) / 64)
#define INTERPOLATE4PS_16BIT(fp, sN, s0, s1, s2)     (((INT32)((sN) * Precalc[(INT32)(fp) * 4 + 0]) + (INT32)((s0) * Precalc[(INT32)(fp) * 4 + 1]) + (INT32)((s1) * Precalc[(INT32)(fp) * 4 + 2]) + (INT32)((s2) * Precalc[(INT32)(fp) * 4 + 3])) / 16384)
#define INTERPOLATE4PS_CUSTOM(fp, sN, s0, s1, s2, v) (((INT32)((sN) * Precalc[(INT32)(fp) * 4 + 0]) + (INT32)((s0) * Precalc[(INT32)(fp) * 4 + 1]) + (INT32)((s1) * Precalc[(INT32)(fp) * 4 + 2]) + (INT32)((s2) * Precalc[(INT32)(fp) * 4 + 3])) / (INT32)(v))

#define INTERPOLATE4PU_8BIT(fp, sN, s0, s1, s2)      (((UINT32)((sN) * Precalc[(INT32)(fp) * 4 + 0]) + (UINT32)((s0) * Precalc[(INT32)(fp) * 4 + 1]) + (UINT32)((s1) * Precalc[(INT32)(fp) * 4 + 2]) + (UINT32)((s2) * Precalc[(INT32)(fp) * 4 + 3])) / 64)
#define INTERPOLATE4PU_16BIT(fp, sN, s0, s1, s2)     (((UINT32)((sN) * Precalc[(INT32)(fp) * 4 + 0]) + (UINT32)((s0) * Precalc[(INT32)(fp) * 4 + 1]) + (UINT32)((s1) * Precalc[(INT32)(fp) * 4 + 2]) + (UINT32)((s2) * Precalc[(INT32)(fp) * 4 + 3])) / 16384)
#define INTERPOLATE4PU_CUSTOM(fp, sN, s0, s1, s2, v) (((UINT32)((sN) * Precalc[(INT32)(fp) * 4 + 0]) + (UINT32)((s0) * Precalc[(INT32)(fp) * 4 + 1]) + (UINT32)((s1) * Precalc[(INT32)(fp) * 4 + 2]) + (UINT32)((s2) * Precalc[(INT32)(fp) * 4 + 3])) / (UINT32)(v))

#ifdef __cplusplus
}
#endif
