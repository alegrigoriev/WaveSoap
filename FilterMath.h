#ifndef _INC_FILTERMATH
#define _INC_FILTERMATH


//#include "windows.h"
#define _USE_MATH_DEFINES   // for M_PI definition
#include <math.h>
#include <complex>
#include "PolyMath.h"
#include "PolyRatio.h"

typedef polyRoots POLY_ROOTS;
typedef polyRatio POLY_RATIO;
typedef poly POLY;
typedef Complex COMPLEX;
typedef complexArray COMPLEX_ARRAY;

#define TWO_PI (M_PI*2.)

int LaguerreMethod(	POLY &coef,
					Complex *zz,
					double epsilon,
					double epsilon2,
					int maxIterations);

void UnwrapPhase( int ix, double *phase);

void EllipticPolesZeros(double omegaPass,
						double omegaStop,
						double minStopLossDB,
						double maxPassLossDB,
						int order,
						POLY_ROOTS &zeros,
						POLY_ROOTS &poles,
						COMPLEX &rNormCoeff);

void TwoAllpassDecompose(const POLY_ROOTS &poles,
						double T,
						POLY &denom1,
						POLY &numer1,
						POLY &denom2,
						POLY &numer2,
						double angle = 0.);

void TwoAllpassPassbandDecompose(const POLY_ROOTS & poles,
								double W0,
								double T,
								POLY &denom1,
								POLY &numer1,
								POLY &denom2,
								POLY &numer2,
								POLY_ROOTS & ZPlanePoles);
void BesselCoefficients(	int order,
							char typeOfNormalization,
							double coef[]);

void cgdFirResponse(	int firType,
						int numbTaps,
						double hh[],
						BOOL dbScale,
						int numberOfPoints,
						double hD[]);

void NormalizeResponse(	BOOL dbScale,
						int numberOfPoints,
						double hh[]);

void IdealLowpass( 	int numbTaps,
					double omegaU,
					double coefficient[]);

void IdealHighpass(	int numbTaps,
					double omegaL,
					double coefficient[]);

void IdealBandpass(	int numbTaps,
					double omegaL,
					double omegaU,
					double coefficient[]);

void IdealBandstop(	int numbTaps,
					double omegaL,
					double omegaU,
					double coefficient[]);

double ContRectangularResponse(	double freq,
								double tau,
								BOOL dbScale);

double DiscRectangularResponse(	double freq,
								int M,
								BOOL normAmp);

double ContTriangularResponse(	double freq,
								double tau,
								BOOL dbScale);

double DiscTriangularResponse(	double freq,
								int M,
								BOOL normAmp);

void TriangularWindow( int N, double window[]);

void MakeLagWindow(	int N,
					double window[],
					int center,
					double outWindow[]);

void MakeDataWindow(	int N,
						double window[],
						double outWindow[]);

void HannWindow( int nn, double window[]);
void HammingWindow( int nn, double window[]);

int fsDesign(	int nn,
				int firType,
				double aa[],
				double h[]);

double findSbPeak(	int bandConfig[],
					int numPts,
					double hh[]);

double GoldenSearch(	int firType,
						int numbTaps,
						double hD[],
						double gsTol,
						int numFreqPts,
						int bandConfig[],
						double *fmin);

void SetTrans(	int bandConfig[],
				double x,
				double hD[]);

double GoldenSearch2(	double rhoMin,
						double rhoMax,
						int firType,
						int numbTaps,
						double hD[],
						double gsTol,
						int numFreqPts,
						double origins[],
						double slopes[],
						int bandConfig[],
						double *fmin);

void SetTransition(	double origins[],
					double slopes[],
					int bandConfig[],
					double x,
					double Hd[]);

void Optimize2(	double yBase,
				int firType,
				int numbTaps,
				double hD[],
				double gsTol,
				int numFreqPts,
				int bandConfig[],
				double tweakFactor,
				double rectComps[]);

void DumpRectComps(	double origins[],
					double slopes[],
					int numTransSamps,
					double x);

double GridFreq(  double gridParam[], int gI);

double DesLpfResp(  double freqP, double freq);

double WeightLp( double kk, double freqP, double freq);

void RemezError(	double gridParam[],
					int gridMax,
					int r,
					double kk,
					double freqP,
					int iFF[],
					double ee[]);

double ComputeRemezA(	double gridParam[],
						int r,
						double kk,
						double freqP,
						int iFF[],
						int initFlag,
						double contFreq);

void RemezSearch(	double ee[],
					double absDelta,
					int gP,
					int iFF[],
					int gridMax,
					int r,
					double gridParam[]);

int RemezStop(  int iFF[],  int r);

int RemezStop2(  double ee[], int iFF[], int r);

void RemezFinish(	double extFreq[],
					int nn,
					int r,
					double freqP,
					double kk,
					double aa[],
					double h[]);

void Remez(	int nn,
			int r,
			int gridDensity,
			double kk,
			double freqP,
			double freqS,
			double extFreq[],
			double h[]);

void IirResponse(	POLY &a,
					int bigN,
					POLY &b,
					int bigM,
					int numberOfPoints,
					BOOL dbScale,
					double magnitude[],
					double phase[]);

void BilinearLowPass(const POLY_ROOTS & SrcPoles,
					const POLY_ROOTS & SrcZeros,
					double T,
					POLY_ROOTS & ZPlanePoles,
					POLY_ROOTS & ZPlaneZeros,
					COMPLEX rotator = Complex(1., 0.));

void BilinearTransform(const poly & src, poly & dst,
						double T, Complex rotator, int nAddZeros);

void BilinearTransform(const polyRatio & src, polyRatio & dst,
						double T, Complex rotator = 1.);

COMPLEX BilinearNormCoeff(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						double T, COMPLEX NormCoeff);

void BilinearPassBand(const POLY_ROOTS &poles,
					const POLY_ROOTS &zeros,
					Complex hZero,
					double W0, // center frequency
					double T,
					POLY &a,
					POLY &b);

void LowpassToBandpass(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						double W0, // center frequency
						double T,
						POLY_ROOTS & ZPlanePoles,
						POLY & Denom);

void ElipticOrderEstim(	double omegaPass,
						double omegaStop,
						double maxPassLoss,
						double &minStopLoss,
						unsigned int &order);

void EllipticPassbandPolesZeros(POLY_ROOTS &poles,
								POLY_ROOTS &zeros,
								int order,
								int bilinear,
								double T,
								double PassFreqLow,
								double PassFreqHigh,
								double StopFreqLow,
								double StopFreqHigh,
								double &CenterFreq,
								double maxPassLossDB,
								double minStopLossDB);

void HilbertTwoAllpassDecompose(const POLY_ROOTS &poles,
								POLY &denom1,
								POLY &numer1,
								POLY &denom2,
								POLY &numer2);

void EllipticHilbertPoles(double omegaPass,
						double &minStopLossDB,
						double &maxPassLossDB, // 0 - power-symmetric filter
						int order,
						POLY_ROOTS &poles);

void Allpass2Canonical(POLY& numer,
						POLY& denom,
						const POLY& numer1,
						const POLY& denom1,
						const POLY& numer2,
						const POLY& denom2);

#endif // ifndef _INC_FILTERMATH
