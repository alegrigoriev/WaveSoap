#ifndef _INC_FILTERMATH
#define _INC_FILTERMATH


#ifdef USE_LONGDOUBLE
	#define POLY_ROOTS lpolyRoots
	#define POLY_RATIO lpolyRatio
	#define POLY lpoly
	#define COMPLEX lcomplex
	#define REAL ldouble
	#define COMPLEX_ARRAY lcomplexArray
#else
	#define POLY_ROOTS polyRoots
	#define POLY_RATIO polyRatio
	#define POLY poly
	#define COMPLEX Complex
	#define REAL double
	#define COMPLEX_ARRAY complexArray
#endif

//#include "windows.h"
#include <math.h>
#include "Complex.h"
#include "PolyMath.h"
#include "PolyRatio.h"

#define TWO_PI (M_PI*2.)

int LaguerreMethod(	POLY &coef,
					Complex *zz,
					REAL epsilon,
					REAL epsilon2,
					int maxIterations);

void UnwrapPhase( int ix, REAL *phase);

void EllipticPolesZeros(REAL omegaPass,
						REAL omegaStop,
						REAL &minStopLossDB,
						REAL &maxPassLossDB,
						int order,
						POLY_ROOTS &zeros,
						POLY_ROOTS &poles,
						COMPLEX &rNormCoeff);

void TwoAllpassDecompose(const POLY_ROOTS &poles,
						REAL T,
						POLY &denom1,
						POLY &numer1,
						POLY &denom2,
						POLY &numer2,
						double angle = 0.);

void TwoAllpassPassbandDecompose(const POLY_ROOTS & poles,
								REAL W0,
								REAL T,
								POLY &denom1,
								POLY &numer1,
								POLY &denom2,
								POLY &numer2,
								POLY_ROOTS & ZPlanePoles);
void BesselCoefficients(	int order,
							char typeOfNormalization,
							REAL coef[]);

void cgdFirResponse(	int firType,
						int numbTaps,
						REAL hh[],
						BOOL dbScale,
						int numberOfPoints,
						REAL hD[]);

void NormalizeResponse(	BOOL dbScale,
						int numberOfPoints,
						REAL hh[]);

void IdealLowpass( 	int numbTaps,
					REAL omegaU,
					REAL coefficient[]);

void IdealHighpass(	int numbTaps,
					REAL omegaL,
					REAL coefficient[]);

void IdealBandpass(	int numbTaps,
					REAL omegaL,
					REAL omegaU,
					REAL coefficient[]);

void IdealBandstop(	int numbTaps,
					REAL omegaL,
					REAL omegaU,
					REAL coefficient[]);

REAL ContRectangularResponse(	REAL freq,
								REAL tau,
								BOOL dbScale);

REAL DiscRectangularResponse(	REAL freq,
								int M,
								BOOL normAmp);

REAL ContTriangularResponse(	REAL freq,
								REAL tau,
								BOOL dbScale);

REAL DiscTriangularResponse(	REAL freq,
								int M,
								BOOL normAmp);

void TriangularWindow( int N, REAL window[]);

void MakeLagWindow(	int N,
					REAL window[],
					int center,
					REAL outWindow[]);

void MakeDataWindow(	int N,
						REAL window[],
						REAL outWindow[]);

void HannWindow( int nn, REAL window[]);
void HammingWindow( int nn, REAL window[]);

int fsDesign(	int nn,
				int firType,
				REAL aa[],
				REAL h[]);

REAL findSbPeak(	int bandConfig[],
					int numPts,
					REAL hh[]);

REAL GoldenSearch(	int firType,
					int numbTaps,
					REAL hD[],
					REAL gsTol,
					int numFreqPts,
					int bandConfig[],
					REAL *fmin);

void SetTrans(	int bandConfig[],
				REAL x,
				REAL hD[]);

REAL GoldenSearch2(	REAL rhoMin,
					REAL rhoMax,
					int firType,
					int numbTaps,
					REAL hD[],
					REAL gsTol,
					int numFreqPts,
					REAL origins[],
					REAL slopes[],
					int bandConfig[],
					REAL *fmin);

void SetTransition(	REAL origins[],
					REAL slopes[],
					int bandConfig[],
					REAL x,
					REAL Hd[]);

void Optimize2(	REAL yBase,
				int firType,
				int numbTaps,
				REAL hD[],
				REAL gsTol,
				int numFreqPts,
				int bandConfig[],
				REAL tweakFactor,
				REAL rectComps[]);

void DumpRectComps(	REAL origins[],
					REAL slopes[],
					int numTransSamps,
					REAL x);

REAL GridFreq(  REAL gridParam[], int gI);

REAL DesLpfResp(  REAL freqP, REAL freq);

REAL WeightLp( REAL kk, REAL freqP, REAL freq);

void RemezError(	REAL gridParam[],
					int gridMax,
					int r,
					REAL kk,
					REAL freqP,
					int iFF[],
					REAL ee[]);

REAL ComputeRemezA(	REAL gridParam[],
					int r,
					REAL kk,
					REAL freqP,
					int iFF[],
					int initFlag,
					REAL contFreq);

void RemezSearch(	REAL ee[],
					REAL absDelta,
					int gP,
					int iFF[],
					int gridMax,
					int r,
					REAL gridParam[]);

int RemezStop(  int iFF[],  int r);

int RemezStop2(  REAL ee[], int iFF[], int r);

void RemezFinish(	REAL extFreq[],
					int nn,
					int r,
					REAL freqP,
					REAL kk,
					REAL aa[],
					REAL h[]);

void Remez(	int nn,
			int r,
			int gridDensity,
			REAL kk,
			REAL freqP,
			REAL freqS,
			REAL extFreq[],
			REAL h[]);

void IirResponse(	POLY &a,
					int bigN,
					POLY &b,
					int bigM,
					int numberOfPoints,
					BOOL dbScale,
					REAL magnitude[],
					REAL phase[]);

void BilinearLowPass(const POLY_ROOTS & SrcPoles,
					const POLY_ROOTS & SrcZeros,
					REAL T,
					POLY_ROOTS & ZPlanePoles,
					POLY_ROOTS & ZPlaneZeros,
					COMPLEX rotator = Complex(1., 0.));

void BilinearTransform(const poly & src, poly & dst,
						double T, Complex rotator, int nAddZeros);

void BilinearTransform(const polyRatio & src, polyRatio & dst,
						double T, Complex rotator = 1.);

COMPLEX BilinearNormCoeff(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						REAL T, COMPLEX NormCoeff);

void BilinearPassBand(const POLY_ROOTS &poles,
					const POLY_ROOTS &zeros,
					Complex hZero,
					REAL W0, // center frequency
					REAL T,
					POLY &a,
					POLY &b);

void LowpassToBandpass(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						REAL W0, // center frequency
						REAL T,
						POLY_ROOTS & ZPlanePoles,
						POLY & Denom);

void ElipticOrderEstim(	REAL omegaPass,
						REAL omegaStop,
						REAL maxPassLoss,
						REAL &minStopLoss,
						unsigned int &order);

void EllipticPassbandPolesZeros(POLY_ROOTS &poles,
								POLY_ROOTS &zeros,
								int order,
								int bilinear,
								REAL T,
								REAL PassFreqLow,
								REAL PassFreqHigh,
								REAL StopFreqLow,
								REAL StopFreqHigh,
								REAL &CenterFreq,
								REAL maxPassLossDB,
								REAL minStopLossDB);

void HilbertTwoAllpassDecompose(const POLY_ROOTS &poles,
								POLY &denom1,
								POLY &numer1,
								POLY &denom2,
								POLY &numer2);

void EllipticHilbertPoles(REAL omegaPass,
						REAL &minStopLossDB,
						REAL &maxPassLossDB, // 0 - power-symmetric filter
						int order,
						POLY_ROOTS &poles);

void Allpass2Canonical(POLY& numer,
						POLY& denom,
						const POLY& numer1,
						const POLY& denom1,
						const POLY& numer2,
						const POLY& denom2);

#endif // ifndef _INC_FILTERMATH
