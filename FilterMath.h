#ifndef _INC_FILTERMATH
#define _INC_FILTERMATH
#pragma once

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



void LowpassToBandpass(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						double W0, // center frequency
						double T,
						POLY_ROOTS & ZPlanePoles,
						POLY & Denom);

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

int EstimateChebyshev2FilterOrder(double w_pass, double passband_attenuation, double w_stop, double stopband_attenuation);

//////////////////////////////////////////////////////////////////
//////////////// Filter
enum { MaxFilterOrder = 16, };
class LowpassFilter
{
public:
	LowpassFilter()
		: m_Order(0),
		m_ZeroPhase(FALSE)
	{
		Reset();
	}

	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}
	void SetZeroPhase(bool ZeroPhase)
	{
		m_ZeroPhase = ZeroPhase;
	}
	std::complex<double> CalculateResponse(std::complex<double> z) const;
	BOOL CreateElliptic(double PassFreq, double PassLoss,
						double StopFreq, double StopLoss);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	bool	m_ZeroPhase;
	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class HighpassFilter
{
public:
	HighpassFilter()
		: m_Order(0),
		m_ZeroPhase(FALSE)
	{
		Reset();
	}

	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}
	void SetZeroPhase(bool ZeroPhase)
	{
		m_ZeroPhase = ZeroPhase;
	}
	std::complex<double> CalculateResponse(std::complex<double> z) const;
	BOOL CreateElliptic(double PassFreq, double PassLoss,
						double StopFreq, double StopLoss);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	BOOL	m_ZeroPhase;
	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class NotchFilter
{
public:
	NotchFilter()
		: m_Order(0),
		m_ZeroPhase(FALSE)
	{
		Reset();
	}

	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}
	void SetZeroPhase(bool ZeroPhase)
	{
		m_ZeroPhase = ZeroPhase;
	}
	std::complex<double> CalculateResponse(std::complex<double> z) const;
	void Create(double PassFreq, double StopFreq);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	bool	m_ZeroPhase;
	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

class HilbertTransformFilter
{
public:
	HilbertTransformFilter()
		: m_Order(0)
	{
		Reset();
	}

	void Reset();
	int GetFilterOrder() const
	{
		return m_Order;
	}

	std::complex<double> CalculateResponse(std::complex<double> /*z*/) const
	{
		return 1.;
	}
	BOOL CreateElliptic(double PassFreq, double PassLoss,
						double StopFreq, double StopLoss);
	void GetCoefficients(double Coeffs[MaxFilterOrder][6]) const;

	int     m_Order;    // low pass filter order
	double  m_Coeffs[MaxFilterOrder][6];
};

#endif // ifndef _INC_FILTERMATH
