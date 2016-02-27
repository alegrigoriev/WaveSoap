// filter math
#include "stdafx.h"
#include "Filtermath.h"
#include <limits.h>
#include <complex>
/**********************************/
/*                                */
/*   sincSqrd()                   */
/*                                */
/**********************************/
const int MAXPOINTS = 100;
const int MAXPOLES = 100;


/**********************************/
/*                                */
/*   bitRev()                     */
/*                                */
/**********************************/

static int bitRev(	int L, int N)
{
	int work, work2, i;

	work2 = 0;
	work = N;
	for(i=0; i<L; i++) {
		work2 = (work2 << 1) | (work & 1);
		work >>=1;
	}
	return work2;
}
/**********************************/
/*                                */
/*   log2()                       */
/*                                */
/**********************************/

static int log2(  int N )
{
	int work, result;

	result = 0;
	work = N;
	for(;;) {
		if(work == 0) break;
		work >>=1;
		result ++;
	}
	return result-1;
}

/**********************************/
/*                                */
/*   ipow()                       */
/*                                */
/**********************************/

static double ipow(double x,
					int k)
{
	double result = 1.;
	if(x == 0.) return 0.;
	if (k < 0) x = 1./x, k = -k;
	while (1)
	{
		if (k & 1) result = result * x;
		k = (k >> 1) & INT_MAX;
		if ( k == 0) return result;
		x = x * x;
	}
}

/**************************************************/
/*                                                */
/*  Listing 2.1                                   */
/*                                                */
/*  laguerreMethod()                              */
/*                                                */
/**************************************************/
int LaguerreMethod(POLY &coef,
					Complex *zz,
					double epsilon,
					double epsilon2,
					int maxIterations)
{
	int iteration, j;
	Complex d2P_dz2, dP_dz, P, f, g, fSqrd, radical, cwork;
	Complex z, fPlusRad, fMinusRad, deltaZ;
	double error, magZ, oldMagZ, fwork;
	//double dd1, dd2;

	z = *zz;
	oldMagZ = abs(z);

	for( iteration=1; iteration<=maxIterations; iteration++)
	{
		d2P_dz2 = Complex(0.0, 0.0);
		dP_dz = Complex(0.0, 0.0);
		P = coef[coef.order()];
		error = abs(P);
		magZ = abs(z);

		for( j=coef.order(); j>=0; j--)
		{
			d2P_dz2 = dP_dz + z * d2P_dz2;
			dP_dz = P + dP_dz * z;
			cwork = P * z;
			P = coef[j] + P * z;
			error = abs(P) + magZ * error;
		}
		error = epsilon2 * error;
		d2P_dz2 = 2.0 * d2P_dz2;

		if( abs(P) < error)
		{
			*zz = z;
			return 1;
		}
		f = dP_dz / P;
		fSqrd = (f * f);
		g = fSqrd - d2P_dz2 / P;
		radical = double(coef.order()-1) * g - fSqrd;
		fwork = coef.order();
		radical = sqrt( fwork * radical);
		fPlusRad = f + radical;
		fMinusRad = f - radical;
		if( abs(fPlusRad) > abs(fMinusRad) )
		{
			deltaZ = double(coef.order()-1) / fPlusRad;
		}
		else
		{
			deltaZ = double(coef.order()-1) / fMinusRad;
		}
		z = z - deltaZ;
		if( (iteration > 6)	&& (abs(deltaZ) > oldMagZ) )
		{
			*zz = z;
			return 2;
		}
		if( abs(deltaZ) < ( epsilon * abs(z)))
		{
			*zz = z;
			return 3;
		}
	}
	//fprintf(stderr,"Laguerre method failed to converge \n");
	return -1;
}

/**********************************/
/*                                */
/*   Listing 2.2                  */
/*                                */
/*   UnwrapPhase()                */
/*                                */
/**********************************/

void UnwrapPhase(	int ix,
					double *phase)
{
	static double halfCircleOffset;
	static double oldPhase;

	if( ix==0)
	{
		halfCircleOffset = 0.0;
		oldPhase = *phase;
	}
	else
	{
		*phase = *phase + halfCircleOffset;
		if( fabs(oldPhase - *phase) > (double)90.0)
		{
			if(oldPhase < *phase)
			{
				*phase = *phase - 360.0;
				halfCircleOffset = halfCircleOffset - 360.0;
			}
			else
			{
				*phase = *phase + 360.0;
				halfCircleOffset = halfCircleOffset + 360.0;
			}
		}
		oldPhase = *phase;
	}
	return;
}


int EstimateChebyshev2FilterOrder(double w_pass, double passband_attenuation, double w_stop, double stopband_attenuation)
{
	double epsylon = sqrt(1 - passband_attenuation*passband_attenuation) / passband_attenuation;

	int N = (int)ceil(acosh(sqrt(1. - stopband_attenuation*stopband_attenuation) / (epsylon*stopband_attenuation))
					/ acosh(w_stop / w_pass));
	return N;
}

using std::complex;
void CalculateChebyshev2FilterPolesAndZeros(int order,
											complex<double> poles[], complex<double> zeros[],
											double w_stop, double stopband_attenuation)
{
	double GG = pow((1. +  sqrt(1.- stopband_attenuation*stopband_attenuation)) / stopband_attenuation, 1. / order);

	double coeff_real = GG - 1. / GG;
	double coeff_imag = GG + 1. / GG;
	for (int i = 0; i < order; i++)
	{
		poles[i] = 2.*w_stop / complex<double>(coeff_real * -sin((2*i + 1)*M_PI/(2*order)),
												coeff_imag * cos((2 * i + 1)*M_PI / (2 * order)));
	}
	for (int i = 0; i < (order & ~1); i++)
	{
		zeros[i] = complex<double>(0., w_stop / cos((2*i+1)*M_PI/(2*order)));
	}
}
/*  POWER SYMMETRIC ELLIPTIC FILTER DECOMPOSED TO TWO ALLPASS CELLS */

void EllipticOrderEstim(double omegaPass,
						double omegaStop,
						double maxPassLoss,
						double &minStopLoss,
						int &order)
{
	double k, u, q, dd, kk, numer;
	//int i, m, r;

	k=omegaPass/omegaStop;				/* Alg. 5.1, step 3 */

	kk=sqrt(sqrt(1.0 - k*k));			/* Eq (5.2) */
	u=0.5*(1.0-kk)/(1.0+kk);

	q = 150.0 * ipow(u,13) + 15.0 * ipow(u,9) + 2.0 * ipow(u,5) + u;

	dd = (pow(10.0, minStopLoss/10.0) - 1.0) /
		(pow(10.0,maxPassLoss/10.0) - 1.0);

	if(order & ~1) order = 0;
	order |= int(ceil( -log(16.0*dd) / log(q)));

	numer = pow(10.0, (maxPassLoss/10.0))-1.0;
	minStopLoss = 10.0 * log10(numer/(16*ipow(q,order))+1.0);
	return;
}

#define ELLIPTIC_FUNCTION_TERMS 20

void EllipticPolesZeros(double omegaPass,
						double omegaStop,
						double minStopLoss,
						double maxPassLoss, // 1 - power-symmetric filter
						int order,
						POLY_ROOTS &zeros,
						POLY_ROOTS &poles,
						COMPLEX &rNormCoeff )
{
	ASSERT (&poles != NULL);
	ASSERT (&zeros != NULL);
	double k, u, q, kk, ww, mu;
	double sum;
	double denom, numer, vv, xx, yy;
	double CenterFreq = sqrt(omegaPass * omegaStop);
	int i, m;
	BOOL PowerSymmetric = FALSE;

	minStopLoss *= minStopLoss;
	maxPassLoss *= maxPassLoss;  // < 1.
	if (minStopLoss > 1)
	{
		minStopLoss = 1. / minStopLoss;
	}
	if (maxPassLoss > 1)
	{
		maxPassLoss = 1. / maxPassLoss;
	}

	if(maxPassLoss == 1.)
	{
		maxPassLoss = 1. - minStopLoss;
		PowerSymmetric = TRUE;
	}

	// estimating required filter order (odd)
	k = omegaPass/omegaStop;				// Alg. 5.1, step 3

	kk = sqrt(sqrt(1.0 - k * k));			// Eq (5.2)
	u = 0.5 * (1.0 - kk) / (1.0 + kk);

	q = 150.0 * ipow(u,13) + 15.0 * ipow(u,9) + 2.0 * ipow(u,5) + u;
	// Eq (5.3)
	if(order <= 1)
	{
		order &= 1;
		order |= int(ceil(-log( 16. * (1. / minStopLoss - 1.)/
								(1. / maxPassLoss - 1.)) / log(q)));
	}

	if(PowerSymmetric)
	{
		order |=1;
		minStopLoss = 4. / (pow(q, - order / 2.) + 4.); // in power terms
		maxPassLoss = 1. - minStopLoss;
		/* Eq (5.12) */
		//	vv = log( pow(q, - order / 2.) + 3 ) / (2.0 * order);
	}
	else
	{
		minStopLoss = 1. / ((1. / maxPassLoss - 1.) / (16. * ipow(q, order)) + 1.);
	}
	poles.SetCount(0);
	zeros.SetCount(0);

	vv = log((sqrt(maxPassLoss) + 1.)/(1. - sqrt(maxPassLoss))) / (2.0 * order);

	sum = 0.0;					/* Eq (5.13) */
	for (m = 0; m < ELLIPTIC_FUNCTION_TERMS; m++)
	{
		sum += ipow(-1.0, m) * ipow(q, m * (m + 1)) * sinh((2 * m + 1.) * vv);
	}
	numer = 2.0 * sum * sqrt(sqrt(q));

	sum = 0.0;

	for (m = 1; m < ELLIPTIC_FUNCTION_TERMS; m++)
	{
		sum += ipow(-1.0, m) * ipow(q, m * m) * cosh(2.0 * m * vv);
	}

	denom = 1.0 + 2.0*sum;
	COMPLEX cFirstTerm = -fabs(numer / denom);

	mu = 0.5;
	if (order & 1)
	{
		mu = 1.;
	}
	/* Eq (5.14) */
	ww = sqrt((1.0 + k * real(cFirstTerm) * real(cFirstTerm))
			* (1.0 + real(cFirstTerm) * real(cFirstTerm) / k));

	for(i = order >> 1, mu += i - 1; i > 0 ; i--, mu -= 1.)
	{
		sum = 0.0;					/* Eq (5.15) numerator */
		for(m = 0; m < ELLIPTIC_FUNCTION_TERMS; m++)
		{
			sum += ipow(-1.0, m) * ipow(q, m * (m + 1)) * sin( (2*m+1) * M_PI * mu / order);
		}

		numer = 2.0 * sum * sqrt(sqrt(q));

		sum = 0.0;					/* Eq (5.15) denominator */
		for(m = 1; m < ELLIPTIC_FUNCTION_TERMS; m++)
		{
			sum += ipow(-1.0, m) * ipow(q, m * m) * cos(2.0 * M_PI * m * mu / order);
		}

		denom = 1.0 + 2.0 * sum;
		xx = numer/denom;

		zeros += COMPLEX(0., 1./xx) * CenterFreq;
		zeros += COMPLEX(0., -1./xx) * CenterFreq;
		denom = 1.0 + ipow(real(cFirstTerm)*xx, 2);		/* Eq (5.18) */
		/* Eq (5.16) */
		yy = sqrt((1.0 - k * xx*xx) * (1.0-(xx*xx/k)))* real(cFirstTerm)/denom;
		xx = xx * ww / denom;
		poles += COMPLEX(yy, xx) * CenterFreq;
		poles += COMPLEX(yy, -xx) * CenterFreq;
	}

	if (order & 1)
	{
		poles += cFirstTerm * CenterFreq;
	}

	rNormCoeff = poles.eval(0.)/zeros.eval(0.);

	if ( ! (order & 1))
	{
		rNormCoeff *= sqrt(maxPassLoss);
	}
	return;
}

void EllipticHilbertPoles(double omegaPass,
						double &minStopLossDB,
						double &maxPassLossDB, // 0 - power-symmetric filter
						int order,
						POLY_ROOTS &poles )
{
	/* estimating required filter order (odd) */
	double k, u, q, kk, ww, mu;
	/*long*/ double sum, denom, numer, minStopLoss, xx, yy;
	/*long*/ double maxPassLoss;
	int i, m;

	if (minStopLossDB < 0) minStopLossDB = -minStopLossDB;

	minStopLoss = pow(10., -minStopLossDB /10.);  // < 1.
	maxPassLoss = 1. - minStopLoss;
	// median frequency is 2
	k = (omegaPass*omegaPass) / 4.;

	kk=sqrt(sqrt(1.0 - k*k));			/* Eq (5.2) */
	u=0.5*(1.0-kk)/(1.0+kk);

	q = 150.0 * ipow(u,13) + 15.0 * ipow(u,9) + 2.0 * ipow(u,5) + u;
	/* Eq (5.3) */
	if(order <= 1)
		order = int(ceil(-log( 16. * (1 / minStopLoss - 1.)/
								(1. / maxPassLoss - 1.)) / log ( q )));

	order |=1;
	minStopLoss = 4. / (pow(q, - order / 2.) + 4.); // in power terms
	maxPassLoss = 1. - minStopLoss;
	maxPassLossDB = -10.*log10(maxPassLoss);

	minStopLossDB = -10.*log10(minStopLoss);

	poles.SetCount(0);

	/* Eq (5.14) */
	ww = sqrt((1.0 + k) * (1.0 + 1. /k));

	for(i = order - 1; i>= 2; i -=2)
	{
		mu = i/2.;
		sum = 0.0;					/* Eq (5.15) numerator */
		for(m=0; m<ELLIPTIC_FUNCTION_TERMS; m++)
		{
			sum += ipow(-1.0,m) * ipow(q, m*(m+1)) * sin( (2*m+1) * M_PI * mu / order);
		}
		numer = 2.0 * sum * sqrt(sqrt(q));

		sum = 0.0;					/* Eq (5.15) denominator */
		for(m=1; m<ELLIPTIC_FUNCTION_TERMS; m++)
		{
			sum += ipow(-1.0,m) * ipow(q,m*m) * cos(2.0 * M_PI * m * mu / order);
		}
		denom = 1.0 + 2.0 * sum;
		xx = numer/denom;
		denom = 1.0 + xx*xx;		/* Eq (5.18) */
		/* Eq (5.16) */
		yy = -sqrt((1.0 - k * xx*xx) * (1.0-(xx*xx/k)))/denom;
		xx = xx * ww / denom;
		poles += Complex(yy, xx) * 2.;
		poles += Complex(yy, -xx) * 2.;
	}

	poles += -2.;

	return;
}

// Non-symmetric pass-band filter synthesis
void EllipticPassbandPolesZeros(POLY_ROOTS &poles,
								POLY_ROOTS &zeros,
								int order,
								int bilinear,
								double T,
								double PassFreqLow,
								double PassFreqHigh,
								double StopFreqLow,
								double StopFreqHigh,
								double & CenterFreq,
								double maxPassLossDB,
								double minStopLossDB)
{
	double f0a=(PassFreqHigh+PassFreqLow)/2;
	double f0b=PassFreqLow+(PassFreqHigh-PassFreqLow)*
				(PassFreqLow-StopFreqLow)/(StopFreqHigh-PassFreqHigh+
											PassFreqLow-StopFreqLow);

	double p1a = 0, p1b, p1c, PassFreq1, StopFreq1, PassFreq2, StopFreq2, f0c;
	int order1, order2, i;
	POLY_ROOTS poles1, zeros1;

	for(int iter=0; iter < 15;iter++)
	{
		if(iter == 0)
		{
			f0c=f0a;
		}
		else if (iter == 1)
		{
			f0c = f0b;
		}
		else
		{
			f0c = (f0a + f0b) /2;
		}

		if(bilinear)
		{
			PassFreq1=2*tan(M_PI*(PassFreqHigh-f0c)*T)/T;
			StopFreq1=2*tan(M_PI*(StopFreqHigh-f0c)*T)/T;
			PassFreq2=2*tan(M_PI*(f0c-PassFreqLow)*T)/T;
			StopFreq2=2*tan(M_PI*(f0c-StopFreqLow)*T)/T;
		}
		else
		{
			PassFreq1=2*M_PI*(PassFreqHigh-f0c);
			StopFreq1=2*M_PI*(StopFreqHigh-f0c);
			PassFreq2=2*M_PI*(f0c-PassFreqLow);
			StopFreq2=2*M_PI*(f0c-StopFreqLow);
		}
		order1 = 1;
		order2 = 1;
		double CopyMinStopLossDB;
		CopyMinStopLossDB = minStopLossDB;
		EllipticOrderEstim(PassFreq1,
							StopFreq1,
							maxPassLossDB,
							CopyMinStopLossDB,
							order1);
		CopyMinStopLossDB = minStopLossDB;
		EllipticOrderEstim(PassFreq2,
							StopFreq2,
							maxPassLossDB,
							CopyMinStopLossDB,
							order2);

		if (order1 < order2)
		{
			order1 = order2;
		}

		CopyMinStopLossDB = minStopLossDB;
		COMPLEX dNormCoef;

		EllipticPolesZeros(PassFreq1,
							StopFreq1,
							CopyMinStopLossDB,
							maxPassLossDB,
							order1, zeros, poles, dNormCoef);
		p1c = real(poles[1]);

		CopyMinStopLossDB = minStopLossDB;
		EllipticPolesZeros(PassFreq2,
							StopFreq2,
							CopyMinStopLossDB,
							maxPassLossDB,
							order1, zeros, poles, dNormCoef);
		p1c -= real(poles1[1]);

		if(iter ==0 )
		{
			p1a = p1c;
		}
		else if (iter == 1)
		{
			p1b = p1c;
		}
		else if (p1a * p1c < 0)
		{
			p1b = p1c;
			f0b = f0c;
		}
		else
		{
			p1a = p1c;
			f0a = f0c;
		}
	}
	CenterFreq = f0a;
	order = poles1.count();
	// Gather coefficients
	for (i = 2; i<= order; i+=2) poles[i]=poles1[i], zeros[i-1]=zeros1[i-1];
	// compute constant coefficient (Complex!) to obtain unity
	// frequency responce on DC
	Complex temp=1.;
	for (i=1; i< order; i++) temp *= -zeros[i];
	temp = 1./temp;
	for (i=1; i<= order; i++) temp *= -poles[i];
	zeros[0] = temp;
	return;
}

	// Low Pass Filter two allpass cell decomposition
void TwoAllpassDecompose(const POLY_ROOTS &poles,
						double T,
						POLY &denom1,
						POLY &numer1,
						POLY &denom2,
						POLY &numer2,
						double angle)
{
	// first cell uses pole[1], pole[4], pole[5], pole[8], pole[9] ...
	// second cell uses pole[2], pole[3], pole[6], pole[7] ...
	int order = poles.count();
	POLY_ROOTS roots1, roots2;
	COMPLEX rotator(cos(angle), sin(angle));
	ASSERT ((order & 1) != 0);
	// perform bilinear transform for the poles
	for (int i = 0; i < order; i ++)
	{
		COMPLEX ctmp = poles[i];
		ctmp = rotator * (2./T + ctmp) / (2./T - ctmp);

		if (i & 2)
		{
			roots1 += ctmp;
		}
		else
		{
			roots2 += ctmp;
		}
	}
	denom1 = POLY(roots1);
	denom2 = POLY(roots2);
	// make reverce denominators
	numer1.MakeUnique();
	numer1.SetOrder(denom1.order());
	for (int i = 0; i <= denom1.order(); i++)
	{
		numer1[i] = conj(denom1[denom1.order() - i]) * 0.5;
	}

	numer2.MakeUnique();
	numer2.SetOrder(denom2.order());
	rotator = Complex(cos(angle * (denom2.order() - denom1.order())),
					sin(angle * (denom2.order() - denom1.order())));
	for (int i = 0; i <= denom2.order(); i++)
	{
		numer2[i] = rotator * conj(denom2[denom2.order() - i]) * 0.5;
	}
	return;
}

// Hilbert Filter two allpass cell decomposition
void HilbertTwoAllpassDecompose(const POLY_ROOTS & poles,
								POLY &denom1,
								POLY &numer1,
								POLY &denom2,
								POLY &numer2)
{
// first cell uses pole[1], pole[4], pole[5], pole[8], pole[9] ...
// second cell uses pole[2], pole[3], pole[6], pole[7] ...
// pole1 = 0 !
	int order = poles.count();
	int i;

	POLY_ROOTS roots1, roots2;
	ASSERT ((order & 1) != 0);

	// perform bilinear transform for the poles
	// poles[poles.count()-1] should be equal 2!
	for (i = 0; i < order; i ++)
	{
		COMPLEX ctmp = poles[i];
		ctmp = Complex(0., -1.) * (2. + ctmp) / (2. - ctmp);
		if (i & 2)
		{
			roots1 += ctmp;
		}
		else
		{
			roots2 += ctmp;
		}
	}

	denom1 = POLY(roots1);
	denom2 = POLY(roots2);
	// make reverse denominators
	COMPLEX mult1(0.5, 0), mult2(0., 0.5);
	if (poles.count() & 2)
	{
		mult1 = Complex(0., 0.5);
		mult2 = Complex(-0.5, 0.);
	}

	if (poles.count() & 4)
	{
		mult1 = - mult1;
		mult2 = - mult2;
	}

	numer1.SetOrder(denom1.order());
	numer1.MakeUnique();
	for (i = 0; i <= denom1.order(); i++)
	{
		numer1[i] = denom1[denom1.order() - i] * mult1;
	}

	numer2.SetOrder(denom2.order());
	numer2.MakeUnique();
	for (i = 0; i <= denom2.order(); i++)
	{
		numer2[i] = denom2[denom2.order() - i] * mult2;
	}

	return;
}

void Allpass2Canonical(POLY& numer,
						POLY& denom,
						const POLY& numer1,
						const POLY& denom1,
						const POLY& numer2,
						const POLY& denom2)
{
	// numer=first numerator*second denominator+second numerator*first denominator
	// denom=second denominator*first denominator
	numer = numer1 * denom2 + numer2 * denom1;
	denom = denom1 * denom2;
}

// Passband Filter two allpass cell decomposition
void TwoAllpassPassbandDecompose(const POLY_ROOTS & poles,
								double W0,
								double T,
								POLY &denom1,
								POLY &numer1,
								POLY &denom2,
								POLY &numer2,
								POLY_ROOTS & ZPlanePoles)
{
// One cell uses pole[1], pole[4], pole[5], pole[8], pole[9] ...
// Another cell uses pole[2], pole[3], pole[6], pole[7] ...
// Order must be odd!
	int order = poles.count();
	int i;
	POLY_ROOTS roots1, roots2, dblpoles;

	ASSERT ((order & 1) != 0);
	Complex rot = exp ( Complex(0., -W0 * T));
	// perform bilinear transform for the poles
	for (i = 0; i < order; i ++)
	{
		COMPLEX ctmp = poles[i];
		// rotate the poles and double them
		ctmp = rot * (2./T + ctmp) / (2./T - ctmp);
		dblpoles += ctmp;
		dblpoles += conj(ctmp);
	}

	for (i = 0; i < dblpoles.count(); i+=2)
	{
		if (i & 4)
		{
			roots1 += dblpoles[i];
			roots1 += dblpoles[i + 1];
		}
		else
		{
			roots2 += dblpoles[i];
			roots2 += dblpoles[i + 1];
		}
	}
	denom1 = POLY(roots1);
	denom2 = POLY(roots2);
	ZPlanePoles = dblpoles;

	// make reverse denominators
	numer1.SetOrder(denom1.order());
	double mult1 = 0.5;
	double mult2 = -0.5;
	if (order & 2)
	{
		mult1 = -0.5;
		mult2 = 0.5;
	}
	for (i = 0; i <= denom1.order(); i++)
	{
		numer1[i] = denom1[denom1.order() - i] * mult1;
	}

	numer2.SetOrder(denom2.order());
	for (i = 0; i <= denom2.order(); i++)
	{
		numer2[i] = denom2[denom2.order() - i] * mult2;
	}
	return;

}

/**********************************/
/*                                */
/*   Listing 6.1                  */
/*                                */
/*   besselCoefficients()         */
/*                                */
/**********************************/
#define MAXORDER 256
void besselCoefficients(int order,
						char typeOfNormalization,
						double coef[])
{
	int i, N, index, indexM1, indexM2;
	double B[3][MAXORDER];
	double A, renorm[MAXORDER];

	renorm[2] = 0.72675;
	renorm[3] = 0.57145;
	renorm[4] = 0.46946;
	renorm[5] = 0.41322;
	renorm[6] = 0.37038;
	renorm[7] = 0.33898;
	renorm[8] = 0.31546;
	A = renorm[order];

	index = 1;
	indexM1 = 0;
	indexM2 = 2;

	for( i=0; i<(3*MAXORDER); i++) B[0][i] = 0;
	B[0][0] = 1.0;
	B[1][0] = 1.0;
	B[1][1] = 1.0;

	for( N=2; N<=order; N++)
	{
		index = (index+1)%3;
		indexM1 = (indexM1 + 1)%3;
		indexM2 = (indexM2 + 1)%3;

		for( i=0; i<N; i++)
		{
			B[index][i] = (2*N-1) * B[indexM1][i];
		}
		for( i=2; i<=N; i++)
		{
			B[index][i] = B[index][i] + B[indexM2][i-2];
		}
	}
	if(typeOfNormalization == 'D')
	{
		for( i=0; i<=order; i++) coef[i] = B[index][i];
	}
	else
	{
		for( i=0; i<=order; i++)
		{
			coef[i] = B[index][i] * pow(A, (order - i) );
		}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 6.2                  */
/*                                */
/*   besselFreqResponse()         */
/*                                */
/**********************************/


void besselFreqResponse(	int order,
							double coef[],
							double frequency,
							double *magnitude,
							double *phase)
{
	Complex numer, omega, denom, transferFunction;
	int i;

	numer = Complex( coef[0], 0.0);
	omega = Complex( 0.0, frequency);
	denom = Complex( coef[order], 0.0);

	for( i=order-1; i>=0; i--)
	{
		denom = omega * denom + coef[i];
	}
	transferFunction = numer / denom;

	*magnitude = 20.0 * log10(abs(transferFunction));
	*phase = 180.0 * arg(transferFunction) / M_PI;
	return;
}

/**********************************/
/*                                */
/*   Listing 6.3                  */
/*                                */
/*   besselGroupDelay()           */
/*                                */
/**********************************/

void besselGroupDelay(	int order,
						double coef[],
						double frequency,
						double delta,
						double *groupDelay)
{
	Complex numer, omega, omegaPlus, denom, transferFunction;
	int i;
	double phase, phase2;

	numer = Complex( coef[0], 0.0);
	denom = Complex( coef[order], 0.0);
	omega = Complex( 0.0, frequency);

	for( i=order-1; i>=0; i--) {
		denom = omega / denom + coef[i];
	}
	transferFunction = numer / denom;
	phase = arg(transferFunction);

	denom = Complex( coef[order], 0.0);
	omegaPlus = Complex(0.0, frequency + delta);

	for( i=order-1; i>=0; i--) {
		denom = omegaPlus / denom + coef[i];
	}
	transferFunction = numer / denom;
	phase2 = arg(transferFunction);
	*groupDelay = (phase2 - phase)/delta;
	return;
}
/**********************************/
/*                                */
/*   Listing 15.1                 */
/*                                */
/*   bilinear()                   */
/*                                */
/**********************************/


void BilinearLowPass(const POLY_ROOTS & SrcPoles,
					const POLY_ROOTS & SrcZeros,
					double T,
					POLY_ROOTS & ZPlanePoles,
					POLY_ROOTS & ZPlaneZeros,
					COMPLEX rotator)
{
	int m;
	POLY_ROOTS zpoles, zzeros;
	int numPoles = SrcPoles.count();
	int numZeros = SrcZeros.count();

	/*-------------------------------------*/
	/*  compute numerator coefficients     */
	for( m = 0; m < numZeros; m++)
	{
		//Complex srczero = SrcZeros[m];
		zzeros +=  rotator * (2.0 / T + SrcZeros[m]) / (2.0/ T - SrcZeros[m]);
	}

	for( ; m < numPoles; m++)
	{
		zzeros += COMPLEX(-1., 0.) * rotator;
	}

	/*-------------------------------------*/
	/*  compute denominator coefficients   */

	for( m = 0; m < numPoles; m++)
	{
		zpoles += rotator * (2.0 / T + SrcPoles[m]) / (2.0 / T - SrcPoles[m]);
	}

	for( ; m < numZeros; m++)
	{
		zpoles += COMPLEX(-1., 0.) * rotator;
	}

	ZPlanePoles = zpoles;
	ZPlaneZeros = zzeros;
	return;
}

void BilinearTransform(const poly & src,
						poly & dst,
						double T, Complex rotator,
						int nAddZeros)
{
	// nAddZeros argument specifies, how many T/2*(z+1)
	// multipliers should be added
	poly res;
	double mult = T / 2.;
	double FirstCoef = ipow(mult, nAddZeros);

	polyRoots pr;
	pr.SetCount(src.order() + nAddZeros);

	for (int i = 0; i <= src.order(); i++)
	{
		for (int j = 0; j < pr.count(); j++)
		{
			if (j < nAddZeros + i)
			{
				pr[j] = -rotator;
			}
			else
			{
				pr[j] = rotator;
			}
		}
		res += poly(pr, src[i] * FirstCoef);
		FirstCoef *= mult;
	}
	dst = res;
}

void BilinearTransform(const polyRatio & src, polyRatio & dst,
						double T, Complex rotator)
{
	int AddNumer = src.denom().order() - src.numer().order();
	int AddDenom = 0;
	if (AddNumer < 0)
	{
		AddDenom = - AddNumer;
		AddNumer = 0;
	}

	BilinearTransform(src.numer(), dst.numer(), T, rotator, AddNumer);
	BilinearTransform(src.denom(), dst.denom(), T, rotator, AddDenom);
	dst.numer() *= 1. / dst.denom()[0];
	dst.denom() *= 1. / dst.denom()[0];
}

COMPLEX BilinearNormCoeff(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						double T, COMPLEX NormCoeff)
{
	COMPLEX bilinearNorm = SrcZeros.eval(COMPLEX(2. / T))
							/ SrcPoles.eval(COMPLEX(2. / T));
	return NormCoeff * bilinearNorm;
}

void LowpassToBandpass(const POLY_ROOTS & SrcPoles,
						const POLY_ROOTS & SrcZeros,
						double W0, // center frequency
						double T,
						POLY_ROOTS & ZPlanePoles,
						POLY & Denom)
{
	int m;
	POLY_ROOTS zpoles, zzeros;
	int numPoles = SrcPoles.count();
	int numZeros = SrcZeros.count();
	COMPLEX rotator(cos(W0 * T), sin(W0 * T));

	for( m = 0; m < numZeros; m++)
	{
		zzeros += SrcZeros[m] * rotator;
		zzeros += SrcZeros[m] * conj(rotator);
	}
	for( m = 0; m < numPoles; m++)
	{
		zpoles += SrcPoles[m] * rotator;
		zpoles += SrcPoles[m] * conj(rotator);
	}

	ZPlanePoles = zpoles;
	Denom.FromRoots(zzeros, 2.);
	for (m = 0; m <= Denom.order(); m++)
	{
		Denom[m].imag(0.);
	}
}

/*
Passband filter synthesys through poles/zeros splitting.
Low-pass prototype filter is conversed to the digital prototype
through the bilinear transform. Poles and zeros of the
digital prototype are rotated to the +/- exp(j*W0*T).
Two resulting rationals are summed.
*/
void bilinearPassBand(Complex pole[],
					int numPoles,
					Complex zero[],
					int numZeros,
					Complex hZero,
					double W0, // center frequency
					double T,
					Complex a[],
					Complex b[])
{
	int j,m,n, maxCoef;
	double beta1, gamma1;
	Complex beta, gamma;
	Complex work, hC;
//    double cosW0T = 2 * cos(W0 * T);
	Complex rot = exp (Complex(0., W0 * T));
	/*-------------------------------------*/
	/*  compute constant gain factor       */
	hC = hZero;
	work = 1.;
	//for(n=1; n <= numPoles; n++) work *= 1. - T /2. * pole[n];
	for(n=1; n <= numPoles; n++)
	{
		work *= 2./ T - pole[n];
	}
	hC /= work;
	work = 1.;
	//for(n=1; n <= numZeros; n++) hC *= 1. - T /2. * zero[n];
	for(n=1; n <= numZeros; n++)
	{
		hC *= 2./ T - zero[n];
	}
	/*-------------------------------------*/
	/*  compute numerator coefficients     */
	b[0] = hC;
	maxCoef = 0;
	for( m=1; m<=(numPoles-numZeros); m++)
	{
		b[++maxCoef] = 0.;
		for( j= maxCoef; j>=1; j--) b[j] += rot * b[j-1];
	}

	for( m=1; m<=numZeros; m++)
	{
		b[++maxCoef] = 0.;
		beta = rot * (2.0/T + zero[m]) / (2.0/T - zero[m]);
		for(j= maxCoef; j>=1; j--)
		{
			b[j] -= beta * b[j-1];
		}
	}

	for( m=1; m<=numPoles; m++)
	{
		b[++maxCoef] = 0.;
		beta = conj(rot * (2.0/T + pole[m]) / (2.0/T - pole[m]));
		for(j= maxCoef; j>=1; j--)
		{
			b[j] -= beta * b[j-1];
		}
	}

	for(j= 0; j<=maxCoef; j++)
	{
		b[j] = 2*real(b[j]);
	}

	/*-------------------------------------*/
	/*  compute denominator coefficients   */
	a[0] = 1.;
	maxCoef = 0;
	for( m=1; m<=numPoles; m++)
	{
		a[++maxCoef] = 0.;
		a[++maxCoef] = 0.;
		beta = (2.0/T + pole[m]) / (2.0/T - pole[m]);
		gamma1 = 2*real(rot * beta);
		beta1 = real(beta)*real(beta) + imag(beta)*imag(beta);
		for(j = maxCoef; j>=2; j--)
		{
			a[j] += beta1 * a[j-2], a[j-1] -= gamma1 * a[j-2];
		}
	}
}
/**********************************/
/*                                */
/*   Listing 11.1                 */
/*                                */
/*   idealLowpass()               */
/*                                */
/**********************************/

void idealLowpass( 	int numbTaps,
					double lambdaU,
					double hh[])
{
	int n;
	double mm;
	printf("in idealLowpass\n");

	for( n=0; n<numbTaps; n++)
	{
		mm = n - (double)(numbTaps-1)/2.0;
		if(mm==0)
		{hh[n] = lambdaU/M_PI;}
		else
		{hh[n] = sin(mm * lambdaU)/(mm * M_PI);}
	}
	return;
}


/**********************************/
/*                                */
/*   Listing 11.2                 */
/*                                */
/*   idealHighpass()              */
/*                                */
/**********************************/

void idealHighpass( int numbTaps,
					double lambdaL,
					double hh[])
{
	int n;
	double mm;
	printf("in idealHighpass\n");

	for( n=0; n<numbTaps; n++)
	{
		mm = n - (double)(numbTaps-1)/2.0;
		if(mm==0)
		{hh[n] = 1.0 - lambdaL/M_PI;}
		else
		{hh[n] = -sin(mm * lambdaL)/(mm * M_PI);}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 11.3                 */
/*                                */
/*   idealBandpass()              */
/*                                */
/**********************************/

void idealBandpass( int numbTaps,
					double lambdaL,
					double lambdaU,
					double hh[])
{
	int n;
	double mm;
	printf("in idealBandpass\n");

	for( n=0; n<numbTaps; n++)
	{
		mm = n - (double)(numbTaps-1)/2.0;
		if(mm==0)
		{hh[n] = (lambdaU - lambdaL)/M_PI;}
		else
		{hh[n] = (sin(mm * lambdaU) - sin(mm * lambdaL))/(mm * M_PI);}
	}
	return;
}


/**********************************/
/*                                */
/*   Listing 11.4                 */
/*                                */
/*   idealBandstop()              */
/*                                */
/**********************************/

void idealBandstop( int numbTaps,
					double lambdaL,
					double lambdaU,
					double hh[])
{
	int n;
	double mm;
	printf("in idealBandstop\n");

	for( n=0; n<numbTaps; n++)
	{
		mm = n - (double)(numbTaps-1)/2.0;
		if(mm==0)
		{hh[n] = 1.0 + (lambdaL - lambdaU)/M_PI;}
		else
		{hh[n] = (sin(n * lambdaL) - sin(mm * lambdaU))/(mm * M_PI);}
	}
	return;
}


/***********************************/
/*                                 */
/*   Listing 11.6                  */
/*                                 */
/*   discRectangularResponse()     */
/*                                 */
/***********************************/


double discRectangularResponse( double freq,
								int M,
								int normalizedAmplitude)
{
	double result;

	if(freq == 0.0)
	{ result = (double) (2*M+1);}
	else
	{ result = fabs(sin(M_PI * freq * (2*M+1))/ sin( M_PI * freq));}

	if( normalizedAmplitude ) result = result / (double) (2*M+1);
	return(result);
}



/**********************************/
/*                                */
/*   Listing 11.8                 */
/*                                */
/*   discTriangularResponse()     */
/*                                */
/**********************************/


double discTriangularResponse( 	double freq,
								int M,
								int normalizedAmplitude)
{
	double result;

	if(freq == 0.0)
	{ result = (double) M;}
	else
	{ result = (sin(M_PI * freq * M) * sin(M_PI * freq * M)) /
				(M * sin( M_PI * freq) * sin( M_PI * freq));
	}

	if( normalizedAmplitude ) result = result / (double) M;
	return(result);
}

/**********************************/
/*                                */
/*   Listing 11.9                 */
/*                                */
/*   triangularWindow()           */
/*                                */
/**********************************/

void triangularWindow( int N, double window[])
{
	double offset;
	int n;
	offset = (double) (1-(N%2));

	for(n=0; n<(N/2.0); n++)
	{
		window[n] = 1.0 - (2.0*n + offset)/(N+1.0);
	}
	return;
}


/**********************************/
/*                                */
/*   Listing 11.10                */
/*                                */
/*   makeLagWindow()              */
/*                                */
/**********************************/

void makeLagWindow(	int N,
					double window[],
					int center,
					double outWindow[])
{
	int n,M;

	if(N%2) {
		M=(N-1)/2;
		for(n=0; n<=M; n++) {
			outWindow[n] = window[n];
			outWindow[-n] = outWindow[n];
		}
	}
	else {
		M=(N-2)/2;
		if(center < 0) {
			for( n=0; n<=M; n++) {
				outWindow[n] = window[n];
				outWindow[-(1+n)] = window[n];
			}
		}
		else {
			for( n=0; n<=M; n++) {
				outWindow[n+1] = window[n];
				outWindow[-n] = window[n];
			}
		}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 11.11                */
/*                                */
/*   makeDataWindow()             */
/*                                */
/**********************************/


void makeDataWindow(	int N,
						double window[],
						double outWindow[])
{
	int n,M;

	if(N%2) {
		M=(N-1)/2;
		for(n=0; n<=M; n++) {
			outWindow[n] = window[M-n];
			outWindow[M+n] = window[n];
		}
	}
	else {
		M=(N-2)/2;
		for(n=0; n<=M; n++) {
			outWindow[n] = window[M-n];
			outWindow[M+n+1] = window[n];
		}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 11.12                */
/*                                */
/*   hannWindow()                 */
/*                                */
/**********************************/
void hannWindow( int N, double window[])
{
	int odd;
	int n;
	odd = N%2;

	for(n=0; n<(N/2.0); n++)
	{
		if( odd)
		{window[n] = 0.5 + 0.5 * cos(TWO_PI*n/(N-1));}
		else
		{window[n] = 0.5 + 0.5 * cos(TWO_PI * (2*n+1)/(2.0*(N-1)));}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 11.13                */
/*                                */
/*   hammingWindow()              */
/*                                */
/**********************************/


void hammingWindow( int N, double window[])
{
	int odd;
	int n;
	odd = N%2;

	for(n=0; n<(N/2.0); n++)
	{
		if( odd)
		{window[n] = 0.54 + 0.46 * cos(TWO_PI*n/(N-1));}
		else
		{window[n] = 0.54 + 0.46 *
					cos(TWO_PI * (2*n+1)/(2.0*(N-1)));}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 12.1                 */
/*                                */
/*   fsDesign()                   */
/*                                */
/**********************************/

int fsDesign(	int N,
				int firType,
				double A[],
				double h[])
{
	int n,k, status;
	double x, M;

	M = (N-1.0)/2.0;
	status = 0;
	switch (firType) {
	case 1:
		if(N%2) {
			for(n=0; n<N; n++) {
				h[n] = A[0];
				x = TWO_PI * (n-M)/N;
				for(k=1; k<=M; k++) {
					h[n] = h[n] + 2.0*A[k]*cos(x*k);
				}
				h[n] = h[n]/N;
			}
		}
		else
		{status = 1;}
		break;
	/*---------------------------------------*/
	case 2:
		if(N%2)
		{status = 2;}
		else {
			for(n=0; n<N; n++) {
				h[n] = A[0];
				x = TWO_PI * (n-M)/N;
				for(k=1; k<=(N/2-1); k++) {
					h[n] = h[n] + 2.0*A[k]*cos(x*k);
				}
				h[n] = h[n]/N;
			}
		}
		break;
	/*---------------------------------------*/
	case 3:
		if(N%2) {
			for(n=0; n<N; n++) {
				h[n] = 0;
				x = TWO_PI * (M-n)/N;
				for(k=1; k<=M; k++) {
					h[n] = h[n] + 2.0*A[k]*sin(x*k);
				}
				h[n] = h[n]/N;
			}
		}
		else
		{status = 3;}
		break;
	/*---------------------------------------*/
	case 4:
		if(N%2)
		{status = 4;}
		else {
			for(n=0; n<N; n++) {
				h[n] = A[N/2]*sin(M_PI*(M-n));
				x = TWO_PI * (n-M)/N;
				for(k=1; k<=(N/2-1); k++) {
					h[n] = h[n] + 2.0*A[k]*sin(x*k);
				}
				h[n] = h[n]/N;
			}
		}
		break;
	}
	return(status);
}

/**********************************/
/*                                */
/*   Listing 12.2                 */
/*                                */
/*   findSbPeak()                 */
/*                                */
/**********************************/


double findSbPeak(	int bandConfig[],
					int numPts,
					double H[])
{
	double peak;
	int n, nBeg, nEnd, indexOfPeak;
	int filterType;

	filterType=bandConfig[0];

	switch (filterType)
	{
	case 1:				/* lowpass */
		nBeg = 2 * numPts * bandConfig[2] / bandConfig[5];
		nEnd = numPts-1;
		break;
	case 2:				/* highpass */
	case 3:				/* bandpass */
		nBeg = 0;
		nEnd = 2 * numPts * bandConfig[1] / bandConfig[5];
		break;
	case 4:				/* bandstop */
		nBeg = 2 * numPts * bandConfig[2] / bandConfig[5];
		nEnd = 2 * numPts * bandConfig[3] / bandConfig[5];
		break;
	default:
		return -9999.0;
	}

	peak = -9999.0;
	for(n=nBeg; n<nEnd; n++)
	{
		if(H[n]>peak)
		{
			peak=H[n];
			indexOfPeak = n;
		}
	}

	if(filterType == 4)
	{		/* bandpass has second stopband */
		nBeg = 2*numPts*bandConfig[4]/bandConfig[5];
		nEnd = numPts;
		for(n=nBeg; n<nEnd; n++)
		{
			if(H[n]>peak)
			{
				peak=H[n];
				indexOfPeak = n;
			}
		}
	}
	return(peak);
}

/**********************************/
/*                                */
/*   Listing 12.4                 */
/*                                */
/*   setTrans()                   */
/*                                */
/**********************************/


void setTrans(	int bandConfig[],
				double x,
				double Hd[])
{
	int n1, n2, n3, n4;

	n1 = bandConfig[1];
	n2 = bandConfig[2];
	n3 = bandConfig[3];
	n4 = bandConfig[4];

	switch (bandConfig[0]) {
	case 1:					/* lowpass */
		Hd[n2-1] = x;
		break;
	case 2:					/* highpass */
		Hd[n1+1] = x;
		break;
	case 3:					/* bandpass */
		Hd[n1+1] = x;
		Hd[n4-1] = Hd[n1+1];
		break;
	case 4:					/* bandstop */
		Hd[n2-1] = x;
		Hd[n3+1] = Hd[n2-1];
		break;
	}
	return;
}


/**********************************/
/*                                */
/*   Listing 14.2                 */
/*                                */
/*   impulseInvar()               */
/*                                */
/**********************************/


void impulseInvar(Complex pole[],
				int numPoles,
				Complex zero[],
				int numZeros,
				double hZero,
				double bigT,
				Complex a[],
				Complex b[])
{
	int k, n, j, maxCoef;
	Complex delta[MAXPOLES];
	Complex bigA[MAXPOLES];
	Complex beta, denom, numer;

	for(j=0; j<MAXPOLES; j++) 	delta[j] = 0.;
	for(j=0; j<=numPoles; j++) {
		a[j] = 0.;
		b[j] = 0.;
	}
/*---------------------------------------------------*/
/*  compute partial fraction expansion coefficients  */
	for( k=1; k<=numPoles; k++) {
		numer = hZero;
		for(n=1; n<=numZeros; n++)  numer *= pole[k] - zero[n];
		denom = 1.0;
		for( n=1; n<=numPoles; n++) {
			if( n !=k) denom *= pole[k] - pole[n];
		}
#if 1
		bigA[k] = numer / denom;
#else
		bigA[k] = (exp(bigT * pole[k] -1) ) * numer / (denom * pole[k]);
#endif
	}
/*-------------------------------------*/
/*  compute numerator coefficients     */
	for( k=1; k<=numPoles; k++) {
		delta[0] = 1.0;
		for(n=1; n<MAXPOLES; n++) delta[n] = 0.0;
		maxCoef = 0;
		for( n=1; n<=numPoles; n++) {
			if(n==k) continue;
			maxCoef++;
			beta = -exp(bigT * pole[n]);
			for(j=maxCoef; j>=1; j--) delta[j] += beta * delta[j-1];
		}
//	for( j=0; j<numPoles; j++)	b[j] += bigA[k] * delta[j];
		for( j=0; j<numPoles; j++)	b[j] += delta[j] * bigA[k];
	}
/*--------------------------------------*/
/*  compute denominator deltaficients   */
	a[0] = 1.0;
	for( n=1; n<=numPoles; n++) {
		beta = -exp(bigT * pole[n]);
		for( j=n; j>=1; j--)
			a[j] += beta * a[j-1];
	}
//	for( j=1; j<=numPoles; j++) a[j] = -a[j];
// normalize numerator coefficients to provide the same responce
// at W=0 (z=1) as the prototype
#if 1
	int i;
	for (numer = hZero, i=1; i <= numZeros; i++) numer *= -zero[i];
	for (denom = 1., i = 1; i <= numPoles; i++) denom *= -pole[i];
	beta = numer / denom;

	for (denom = 1., i = 0; i <= numPoles; i++) denom += a[i];
	for (numer = 1., i = 0; i <= numZeros; i++) numer += b[i];
	beta = beta *denom / numer;
	for (i = 0; i < numPoles; i++) b[i] *= beta;
#endif
	return;
}

/**********************************/
/*                                */
/*   Listing 14.3                 */
/*                                */
/*   stepInvar()                  */
/*                                */
/**********************************/

void stepInvar(Complex pole[],
				int numPoles,
				Complex zero[],
				int numZeros,
				double hZero,
				double bigT,
				Complex a[],
				Complex b[])
{
	int k, n, j, maxCoef;
	Complex delta[MAXPOLES];
	Complex bigA[MAXPOLES];
	Complex beta, denom, numer, work2;

	for(j=0; j<MAXPOLES; j++) 	delta[j] = 0.0;
	for(j=0; j<=numPoles; j++) {
		a[j] = 0.;
		b[j] = 0.;
	}
	pole[0] = 0.0;
/*---------------------------------------------------*/
/*  compute partial fraction expansion coefficients  */
	for( k=0; k<=numPoles; k++) {
		numer = hZero;
		for(n=1; n<=numZeros; n++) numer *= pole[k] - zero[n];
		denom = 1.0;
		for( n=0; n<=numPoles; n++) if(n!=k) denom *= pole[k] - pole[n];
		bigA[k] = numer / denom;
	}
/*-------------------------------------*/
/*  compute numerator coefficients     */
	for( k=1; k<=numPoles; k++) {
		delta[0] = 1.0;
		for(n=1; n<MAXPOLES; n++) delta[n] = 0.0;
		maxCoef = 0;
		for( n=0; n<=numPoles; n++) {
			if(n==k) continue;
			maxCoef++;
			beta = -exp(bigT * pole[n]);
			for(j=maxCoef; j>=1; j--) delta[j] += beta * delta[j-1];
		}
		for( j=0; j<numPoles; j++) b[j] += bigA[k] * delta[j];

/* multiply by 1-z**(-1)   */
		beta = -1.;
		for(j=numPoles+1; j>=1; j--) b[j] += beta * b[j-1];
	}
/*-------------------------------------*/
/*  compute denominator coefficients   */
	a[0] = 1.0;
	for( n=1; n<=numPoles; n++)
	{
		beta = -exp(bigT * pole[n]);
		for( j=n; j>=1; j--) a[j] += beta * a[j-1];
	}
	for( j=1; j<=numPoles; j++) a[j] = -a[j];
	return;
}

void LowpassFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void HighpassFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void NotchFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void HilbertTransformFilter::Reset()
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		m_Coeffs[i][0] = 0.;
		m_Coeffs[i][1] = 0.;
		m_Coeffs[i][2] = 0.;
		m_Coeffs[i][3] = 1.;
		m_Coeffs[i][4] = 0.;
		m_Coeffs[i][5] = 0.;
	}
}

void LowpassFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void HighpassFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void NotchFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

void HilbertTransformFilter::GetCoefficients(double Coeffs[MaxFilterOrder][6]) const
{
	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			Coeffs[i][j] = m_Coeffs[i][j];
		}
	}
}

using std::complex;
// frequency is in radians
complex<double> LowpassFilter::CalculateResponse(complex<double> z) const
{
	complex<double> Result(0., 0.);
	for (int i = 0; i < m_Order; i++)
	{
		Result += (m_Coeffs[i][0] + z * (m_Coeffs[i][1] + z * m_Coeffs[i][2]))
				/ (m_Coeffs[i][3] + z * (m_Coeffs[i][4] + z * m_Coeffs[i][5]));
	}

	if (m_ZeroPhase)
	{
		// filter is applied twice
		return Result * conj(Result);
	}
	else
	{
		return Result;
	}
}

complex<double> NotchFilter::CalculateResponse(complex<double> z) const
{
	complex<double> Result(0., 0.);
	for (int i = 0; i < m_Order; i++)
	{
		Result += (m_Coeffs[i][0] + z * (m_Coeffs[i][1] + z * m_Coeffs[i][2]))
				/ (m_Coeffs[i][3] + z * (m_Coeffs[i][4] + z * m_Coeffs[i][5]));
	}

	if (m_ZeroPhase)
	{
		// filter is applied twice
		return Result * conj(Result);
	}
	else
	{
		return Result;
	}
}

complex<double> HighpassFilter::CalculateResponse(complex<double> z) const
{
	complex<double> Result(0., 0.);
	for (int i = 0; i < m_Order; i++)
	{
		Result += (m_Coeffs[i][0] + z * (m_Coeffs[i][1] + z * m_Coeffs[i][2]))
				/ (m_Coeffs[i][3] + z * (m_Coeffs[i][4] + z * m_Coeffs[i][5]));
	}

	if (m_ZeroPhase)
	{
		// filter is applied twice
		return Result * conj(Result);
	}
	else
	{
		return Result;
	}
}

void NotchFilter::Create(double PassFreq, double StopFreq)
{
	// two zeros at unity circle, two poles
	double Width = fabs(StopFreq - PassFreq);
	if (m_ZeroPhase)
	{
		Width /= 2.;
	}
	m_Order = 1;
	double RotC = cos(StopFreq);
	double pole = 1. - Width;

	m_Coeffs[0][0] = 1;
	m_Coeffs[0][1] = -2 * RotC;
	m_Coeffs[0][2] = 1;
	m_Coeffs[0][3] = 1;
	m_Coeffs[0][4] = -2 * pole * RotC;
	m_Coeffs[0][5] = pole * pole;
}

BOOL LowpassFilter::CreateElliptic(double PassFreq, double PassLoss,
									double StopFreq, double StopLoss)
{
	double OmegaPass = 2. * tan(PassFreq / 2.);
	double OmegaStop = 2. * tan(StopFreq / 2.);
	if (m_ZeroPhase)
	{
		PassLoss = sqrt(PassLoss);
		StopLoss = sqrt(StopLoss);
	}
	POLY_ROOTS zeros;
	POLY_ROOTS poles;
	COMPLEX NormCoeff;

	EllipticPolesZeros(OmegaPass, OmegaStop, StopLoss,
						PassLoss, 1, zeros, poles, NormCoeff);

	CArray<polyRatio *, polyRatio *> * pDecomposed
		= polyRatio(poly(zeros, NormCoeff), poly(poles)).Decompose(2, &poles);

	m_Order = (int)pDecomposed->GetSize();

	int chebyshev_order = EstimateChebyshev2FilterOrder(OmegaPass, PassLoss, OmegaStop, StopLoss);
	TRACE("Pass loss=%f, stop loss=%f, EllipticOrder=%d, chebyshev order=%d\n", PassLoss, StopLoss, m_Order, chebyshev_order);

	for (int i = 0; i < pDecomposed->GetSize(); i++)
	{
#if 0 && defined(_DEBUG)
		pDecomposed->GetAt(i)->Dump();
#endif
		polyRatio prBil;
		BilinearTransform(*pDecomposed->GetAt(i), prBil, 1.);
		ASSERT(prBil.numer().order() == 2 || prBil.numer().order() == 1);
		ASSERT(prBil.denom().order() == 2 || prBil.denom().order() == 1);

		m_Coeffs[i][0] = prBil.numer()[0].real();
		m_Coeffs[i][1] = prBil.numer()[1].real();
		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][2] = prBil.numer()[2].real();
		}
		else
		{
			m_Coeffs[i][2] = 0.;
		}
		m_Coeffs[i][3] = prBil.denom()[0].real();
		m_Coeffs[i][4] = prBil.denom()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][5] = prBil.denom()[2].real();
		}
		else
		{
			m_Coeffs[i][5] = 0.;
		}

		delete pDecomposed->GetAt(i);
	}
	delete pDecomposed;
	return TRUE;
}

BOOL HighpassFilter::CreateElliptic(double PassFreq, double PassLoss,
									double StopFreq, double StopLoss)
{
	double OmegaPass = 2. / tan(PassFreq / 2.);
	double OmegaStop = 2. / tan(StopFreq / 2.);
	if (m_ZeroPhase)
	{
		PassLoss = sqrt(PassLoss);
		StopLoss = sqrt(StopLoss);
	}
	POLY_ROOTS zeros;
	POLY_ROOTS poles;
	COMPLEX NormCoeff;

	EllipticPolesZeros(OmegaPass, OmegaStop, StopLoss,
						PassLoss, 1, zeros, poles, NormCoeff);

	CArray<polyRatio *, polyRatio *> * pDecomposed
		= polyRatio(poly(zeros, NormCoeff), poly(poles)).Decompose(2, &poles);

	m_Order = (int)pDecomposed->GetSize();

	for (int i = 0; i < pDecomposed->GetSize(); i++)
	{
#if 0 && defined(_DEBUG)
		pDecomposed->GetAt(i)->Dump();
#endif
		polyRatio prBil;
		BilinearTransform(*pDecomposed->GetAt(i), prBil, 1.);
		ASSERT(prBil.numer().order() == 2 || prBil.numer().order() == 1);
		ASSERT(prBil.denom().order() == 2 || prBil.denom().order() == 1);

		m_Coeffs[i][0] = prBil.numer()[0].real();
		m_Coeffs[i][1] = -prBil.numer()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][2] = prBil.numer()[2].real();
		}
		else
		{
			m_Coeffs[i][2] = 0.;
		}
		m_Coeffs[i][3] = prBil.denom()[0].real();
		m_Coeffs[i][4] = -prBil.denom()[1].real();

		if (prBil.numer().order() > 1)
		{
			m_Coeffs[i][5] = prBil.denom()[2].real();
		}
		else
		{
			m_Coeffs[i][5] = 0.;
		}

		delete pDecomposed->GetAt(i);
	}
	delete pDecomposed;
	return TRUE;
}

