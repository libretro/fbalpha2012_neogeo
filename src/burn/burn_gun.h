#define MAX_GUNS	4

#ifdef __cplusplus
extern "C" {
#endif

extern INT32 nBurnGunNumPlayers;

extern INT32 BurnGunX[MAX_GUNS];
extern INT32 BurnGunY[MAX_GUNS];

UINT8 BurnGunReturnX(INT32 num);
UINT8 BurnGunReturnY(INT32 num);

extern void BurnGunInit(INT32 nNumPlayers, BOOL bDrawTargets);
void BurnGunExit(void);
void BurnGunScan(void);
extern void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y);
extern void BurnGunMakeInputs(INT32 num, INT16 x, INT16 y);

#ifdef __cplusplus
}
#endif
