// filter math
#include "stdafx.h"
#include "Filtermath.h"
#include <limits.h>
/**********************************/
/*                                */
/*   sincSqrd()                   */
/*                                */
/**********************************/

static double sincSqrd( double x)
{
	double result;
	if( x==0.0)
	{
		result = 1.0;
	}
	else
	{
		result = sin(x)/x;
		result = result * result;
	}
	return result;
}
/**********************************/
/*                                */
/*   sinc()                       */
/*                                */
/**********************************/

static double sinc( double x)
{
	double result;
	if( x==0.0)
		result = 1.0;
	else
		result = sin(x)/x;
	return result;
}

/**********************************/
/*                                */
/*   acosh()                       */
/*                                */
/**********************************/

static double acosh( double x)
{
	double result;
	result = log(x+sqrt(x*x-1.0));
	return result;
}

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
			sum += ipow(-1.0, m) * ipow(q, m * (m + 1)) *
					sin( (2*m+1) * M_PI * mu / order);
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
		for(m=0; m<ELLIPTIC_FUNCTION_TERMS; m++)	sum += ipow(-1.0,m) * ipow(q, m*(m+1)) *
															sin( (2*m+1) * M_PI * mu / order);

		numer = 2.0 * sum * sqrt(sqrt(q));

		sum = 0.0;					/* Eq (5.15) denominator */
		for(m=1; m<ELLIPTIC_FUNCTION_TERMS; m++)
			sum += ipow(-1.0,m) * ipow(q,m*m) * cos(2.0 * M_PI * m * mu / order);

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
	for (i = 0; i <= denom1.order(); i++)
	{
		numer1[i] = conj(denom1[denom1.order() - i]) * 0.5;
	}

	numer2.MakeUnique();
	numer2.SetOrder(denom2.order());
	rotator = Complex(cos(angle * (denom2.order() - denom1.order())),
					sin(angle * (denom2.order() - denom1.order())));
	for (i = 0; i <= denom2.order(); i++)
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
	POLY_ROOTS roots1, roots2;
	ASSERT ((order & 1) != 0);
	// perform bilinear transform for the poles
	// poles[poles.count()-1] should be equal 2!
	for (int i = 0; i < order; i ++)
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
	// make reverce denominators
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
	POLY_ROOTS roots1, roots2, dblpoles;
	ASSERT ((order & 1) != 0);
	Complex rot = exp ( Complex(0., -W0 * T));
	// perform bilinear transform for the poles
	for (int i = 0; i < order; i ++)
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
/*   Listing 11.5                  */
/*                                 */
/*   contRectangularResponse()     */
/*                                 */
/***********************************/

#define TINY 3.16e-5

double contRectangularResponse( double freq, double tau, int dbScale)
{
	double x;

	x = sinc(M_PI * freq * tau);
	if(dbScale)
	{
		if(fabs(x) < TINY)
		{x = -90.0;}
		else
		{x = 20.0*log10(fabs(x));}
	}
	return(x);
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
/*   Listing 11.7                 */
/*                                */
/*   contTriangularResponse()     */
/*                                */
/**********************************/


double contTriangularResponse( 	double freq,
								double tau,
								int dbScale)
{
	double amp0, x;
	amp0 = 0.5 * tau;
	x = M_PI * freq * tau / 2.0;
	x = 0.5 * tau * sincSqrd(x);
	if(dbScale)
	{
		if(fabs(x/amp0) < TINY)
		{x = -90.0;}
		else
		{x = 20.0*log10(fabs(x/amp0));}
	}
	return(x);
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
/*   Listing 10.2                 */
/*                                */
/*   normalizeResponse()          */
/*                                */
/**********************************/

void normalizeResponse(	int dbScale,
						int numPts,
						double H[])
{
	int n;
	double biggest;

	if(dbScale)
	{
		biggest = -100.0;
		for( n=0; n<=numPts-1; n++)
		{if(H[n]>biggest) biggest = H[n];}
		for( n=0; n<=numPts-1; n++)
		{H[n] = H[n]-biggest;}
	}
	else
	{
		biggest = 0.0;
		for( n=0; n<=numPts-1; n++)
		{if(H[n]>biggest) biggest = H[n];}
		for( n=0; n<=numPts-1; n++)
		{H[n] = H[n]/biggest;}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 12.3                 */
/*                                */
/*   goldenSearch()               */
/*                                */
/**********************************/
#if 0
double goldenSearch(	int firType,
						int numbTaps,
						double Hd[],
						double tol,
						int numFreqPts,
						int bandConfig[],
						double *fmin)
{
	double x0, x1, x2, x3, xmin, f0, f1, f2, f3, oldXmin;
	double leftOrd, rightOrd, midOrd, midAbsc, x, xb;
	double delta;
	static double hh[100], H[610];
	int n;
	int dbScale;
	FILE *logPtr;

	printf("in goldenSearch\n");
	logPtr = fopen("search.log","w");

	dbScale = TRUE;
/*--------------------------------------------*/
	setTrans( bandConfig, 0, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	leftOrd = findSbPeak(bandConfig,numFreqPts,H);
	printf("leftOrd = %f\n",leftOrd);

	setTrans( bandConfig, 1.0, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	rightOrd = findSbPeak(bandConfig,numFreqPts,H);
	printf("rightOrd = %f\n",rightOrd);

	if(leftOrd < rightOrd) {
		midAbsc=1.0;
		for(;;) {
			printf("checkpoint 3\n");
			midAbsc = GOLD3 * midAbsc;
			setTrans( bandConfig, midAbsc, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			midOrd = findSbPeak(bandConfig,numFreqPts,H);
			printf("midOrd = %f\n",midOrd);
			if(midOrd < leftOrd) break;
		}
	}
	else {
		x = 1.0;
		for(;;) {
			x = GOLD3 * x;
			midAbsc = 1.0 - x;
			printf("checkpoint 4\n");
			setTrans( bandConfig, midAbsc, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			midOrd = findSbPeak(bandConfig,numFreqPts,H);
			printf("midOrd = %f\n",midOrd);
			if(midOrd < rightOrd) break;
		}
	}
	xb = midAbsc;
/*--------------------------------------------*/
	x0 = 0.0;
	x3 = 1.0;
	x1 = xb;
	x2 = xb + GOLD3 * (1.0 - xb);
	printf("x0= %f, x1= %f, x2= %f, x3= %f\n",x0,x1,x2,x3);

	setTrans( bandConfig, x1, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	f1 = findSbPeak(bandConfig,numFreqPts,H);

	setTrans( bandConfig, x2, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	f2 = findSbPeak(bandConfig,numFreqPts,H);

	oldXmin = 0.0;

	for(n=1; n<=100; n++) {
		if(f1<=f2) {
			x3 = x2;
			x2 = x1;
			x1 = GOLD6 * x2 + GOLD3 * x0;
			f3 = f2;
			f2 = f1;
			setTrans( bandConfig, x1, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			f1 = findSbPeak(bandConfig,numFreqPts,H);
			printf("x0= %f, x1= %f, x2= %f, x3= %f\n",x0,x1,x2,x3);
		}
		else {
			x0 = x1;
			x1 = x2;
			x2 = GOLD6 * x1 + GOLD3 * x3;
			f0 = f1;
			f1 = f2;
			setTrans( bandConfig, x2, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			f2 = findSbPeak(bandConfig,numFreqPts,H);
			printf("x0= %f, x1= %f, x2= %f, x3= %f\n",x0,x1,x2,x3);
		}

		delta = fabs(x3 - x0);
		oldXmin = xmin;
		printf("at iter %d, delta = %f\n",n,delta);
		printf("tol = %f\n",tol);
		if(delta <= tol) break;
	}
	if(f1<f2)
	{xmin = x1;
		*fmin=f1;}
	else
	{xmin = x2;
		*fmin=f2;}
	printf("minimum of %f at x = %f\n", *fmin, xmin);
	fprintf(logFptr,"minimum of %f at x = %f\n", *fmin, xmin);
	return(xmin);
}
#endif

/**********************************/
/*                                */
/*   Listing 12.6                 */
/*                                */
/*   setTransition()              */
/*                                */
/**********************************/


void setTransition(	double origins[],
					double slopes[],
					int bandConfig[],
					double x,
					double Hd[])
{
	int n, nnn, n1, n2, n3, n4;

	nnn = bandConfig[2] - bandConfig[1] - 1;
	n1 = bandConfig[1];
	n2 = bandConfig[2];
	n3 = bandConfig[3];
	n4 = bandConfig[4];

	switch (bandConfig[0]) {
	case 1:					/* lowpass */
		for( n=1; n<=nnn; n++) {
			Hd[n2-n] = origins[n] + x * slopes[n];
		}
		break;
	case 2:					/* highpass */
		for( n=1; n<=nnn; n++){
			Hd[n1+n] = origins[n] + x * slopes[n];
		}
		break;
	case 3:					/* bandpass */
		for( n=1; n<=nnn; n++) {
			Hd[n1+n] = origins[n] + x * slopes[n];
			Hd[n4-n] = Hd[n1+n];
		}
		break;
	case 4:					/* bandstop */
		for( n=1; n<=nnn; n++) {
			Hd[n2-n] = origins[n] + x * slopes[n];
			Hd[n3+n] = Hd[n2-n];
		}
		break;
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 12.5                 */
/*                                */
/*   goldenSearch2()              */
/*                                */
/**********************************/

#if 0
double goldenSearch2(	double rhoMin,
						double rhoMax,
						int firType,
						int numbTaps,
						double Hd[],
						double tol,
						int numFreqPts,
						double origins[],
						double slopes[],
						int bandConfig[],
						double *fmin)
{
	double x0, x1, x2, x3, xmin, f0, f1, f2, f3, oldXmin;
	double leftOrd, rightOrd, midOrd, midAbsc, x, xb;
	double delta;
	static double hh[100], H[610];
	int n;
	int dbScale;

	dbScale = TRUE;

/*--------------------------------------------*/
	setTransition( origins, slopes, bandConfig, 0, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	leftOrd = findSbPeak(bandConfig,numFreqPts,H);

	setTransition( origins, slopes, bandConfig, rhoMax, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	rightOrd = findSbPeak(bandConfig,numFreqPts,H);

	if(leftOrd < rightOrd) {
		midAbsc=rhoMax;
		for(;;) {
			midAbsc = GOLD3 * midAbsc;
			setTransition( origins, slopes, bandConfig, midAbsc, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			midOrd = findSbPeak(bandConfig,numFreqPts,H);
			if(midOrd < leftOrd) break;
		}
	}
	else {
		x = rhoMax;
		for(;;) {
			x = GOLD3 * x;
			midAbsc = rhoMax - x;
			setTransition( origins, slopes, bandConfig, midAbsc, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			midOrd = findSbPeak(bandConfig,numFreqPts,H);
			if(midOrd < rightOrd) break;
		}
	}
	xb = midAbsc;
/*--------------------------------------------*/
	x0 = rhoMin;
	x3 = rhoMax;
	x1 = xb;
	x2 = xb + GOLD3 * (rhoMax - xb);

	setTransition( origins, slopes, bandConfig, x1, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	f1 = findSbPeak(bandConfig,numFreqPts,H);

	setTransition( origins, slopes, bandConfig, x2, Hd);
	fsDesign( numbTaps, firType, Hd, hh);
	cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
	normalizeResponse(dbScale,numFreqPts,H);
	f2 = findSbPeak(bandConfig,numFreqPts,H);

	oldXmin = 0.0;

	for(n=1; n<=100; n++) {
		if(f1<=f2) {
			x3 = x2;
			x2 = x1;
			x1 = GOLD6 * x2 + GOLD3 * x0;
			f3 = f2;
			f2 = f1;
			setTransition( origins, slopes, bandConfig, x1, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			f1 = findSbPeak(bandConfig,numFreqPts,H);
		}
		else {
			x0 = x1;
			x1 = x2;
			x2 = GOLD6 * x1 + GOLD3 * x3;
			f0 = f1;
			f1 = f2;
			setTransition( origins, slopes, bandConfig, x2, Hd);
			fsDesign( numbTaps, firType, Hd, hh);
			cgdFirResponse(firType,numbTaps, hh, dbScale, numFreqPts,H);
			normalizeResponse(dbScale,numFreqPts,H);
			f2 = findSbPeak(bandConfig,numFreqPts,H);
		}

		delta = fabs(x3 - x0);
		oldXmin = xmin;
		if(delta <= tol) break;
	}
	if(f1<f2)
	{xmin = x1;
		*fmin=f1;}
	else
	{xmin = x2;
		*fmin=f2;}
	return(xmin);
}
#endif
/**********************************/
/*                                */
/*   Listing 12.7                 */
/*                                */
/*   optimize2()                  */
/*                                */
/**********************************/

#if 0
void optimize2(	double yBase,
				int firType,
				int numbTaps,
				double Hd[],
				double gsTol,
				int numFreqPts,
				int bandConfig[],
				double tweakFactor,
				double rectComps[])
{
	double r1, r2, r3, x1, x2, x3, y3, minFuncVal;
	double slopes[5], origins[5];
	double oldMin, xMax;
	for(;;)
	{
	/*---------------------------------------------------*/
	/*  do starting point for new steepest descent line  */
		slopes[1] = 1.0;
		slopes[2] = 0.0;
		origins[1] = 0.0;
		origins[2] = yBase;

		x1 = goldenSearch2(	0.0, 1.0,
							firType,numbTaps,Hd,gsTol,numFreqPts,
							origins,slopes,bandConfig,&minFuncVal);

	/*-------------------------------------*/
	/*  do perturbed point to get          */
	/*    slope for steepest descent line  */

		origins[2]=yBase * tweakFactor;

		x2 = goldenSearch2(	0.0, 1.0, firType,numbTaps,Hd,
							gsTol,numFreqPts,origins,slopes,
							bandConfig,&minFuncVal);

	/*-------------------------------------*/
	/* define line of steepest descent     */
	/*  and find optimal point along line  */

		slopes[2] = yBase*(1-tweakFactor)/(x1-x2);
		origins[2] = yBase - slopes[2] * x1;
		xMax = (1.0 - origins[2])/slopes[2];

		x3 = goldenSearch2(	0.0, xMax, firType,numbTaps,Hd,
							gsTol,numFreqPts,
							origins,slopes,bandConfig,&minFuncVal);
		y3=origins[2] + x3 * slopes[2];

	/*---------------------------------------------------------------*/
	/*  if ripple at best point on current line is within specified  */
	/*    tolerance of ripple at best point on previous line,        */
	/*    then stop; otherwise stay in loop and define a new line    */
	/*    starting at the best point on line just completed.         */

		if(fabs(oldMin-minFuncVal)<0.01) break;
		oldMin = minFuncVal;
		yBase = y3;
	}
	rectComps[0] = x3;
	rectComps[1] = origins[2] + x3 * slopes[2];
	return;
}
#endif
/**********************************/
/*                                */
/*   Listing 12.8                 */
/*                                */
/*   dumpRectComps()              */
/*                                */
/**********************************/


void dumpRectComps(	double origins[],
					double slopes[],
					int numTransSamps,
					double x)
{
	double rectComp;
	int n;

	for(n=0; n<numTransSamps; n++)
	{
		rectComp = origins[n+1] + x * slopes[n+1];
		printf("rectComp[%d] = %f\n",n,rectComp);
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 13.2                 */
/*                                */
/*   desLpfResp()                 */
/*                                */
/**********************************/

double desLpfResp( double freqP, double freq)
{
	double result;
	result = 0.0;
	if(freq <= freqP) result = 1.0;
	return(result);
}


/**********************************/
/*                                */
/*   Listing 13.3                 */
/*                                */
/*   weightLp()                   */
/*                                */
/**********************************/

double weightLp( double kk, double freqP, double freq)
{
	double result;

	result = 1.0;
	if(freq <= freqP) result = 1.0/kk;
	return(result);
}

/**********************************/
/*                                */
/*   Lisitng 13.1                 */
/*                                */
/*   gridFreq()                   */
/*                                */
/**********************************/

double gridFreq(	double gridParam[],
					int gI)
{
	double work;
	static double incP, incS, freqP, freqS;
	static int r, gridDensity, mP, mS, gP;

	if(gridParam[0] == 1.0) {
		gridParam[0] = 0.0;
		freqP = gridParam[1];
		freqS = gridParam[2];
		r = (int) gridParam[3];
		gridDensity = (int) gridParam[4];
		work = (0.5 + freqP - freqS)/r;
		mP = (int) floor(0.5 + freqP/work);
		gridParam[5] = mP;
		gP = mP * gridDensity;
		gridParam[7] = gP;
		mS = r +1 - mP;
		gridParam[6] = mS;
		incP = freqP / gP;
		incS = (0.5-freqS) / ((mS-1) * gridDensity);
	}
	else {
		work = (gI<=gP) ? (gI*incP) : (freqS+(gI-(gP+1))*incS);
	}
	return(work);
}

/**********************************/
/*                                */
/*   Listing 13.5                 */
/*                                */
/*   computeRemezA()              */
/*                                */
/**********************************/

double computeRemezA(	double gridParam[],
						int r,
						double kk,
						double freqP,
						int iFF[],
						int initFlag,
						double contFreq)
{
	static int i, j, k, sign;
	static double freq, denom, numer, alpha, delta;
	static double absDelta, xCont, term;
	static double x[50], beta[50], gamma[50];
	double aa = 0.;

	if(initFlag)
	{
		for(j=0; j<=r; j++)
		{
			freq = gridFreq(gridParam,iFF[j]);
			x[j] = cos(TWO_PI * freq);
		}

		/*  compute delta  */
		denom = 0.0;
		numer = 0.0;
		sign = -1;
		for( k=0; k<=r; k++)
		{
			sign = -sign;
			alpha = 1.0;
			for( i=0; i<=(r-1); i++)
			{
				if(i==k) continue;
				alpha = alpha / (x[k] - x[i]);
			}
			beta[k] = alpha;
			if( k != r ) alpha = alpha/(x[k] - x[r]);
			freq =  gridFreq(gridParam,iFF[k]);
			numer = numer + alpha * desLpfResp(freqP,freq);
			denom = denom + sign*(alpha/
								weightLp(kk, freqP, freq));
		}
		delta = numer/denom;
		absDelta = fabs(delta);

		sign = -1;
		for( k=0; k<=r-1; k++)
		{
			sign = -sign;
			freq = gridFreq(gridParam,iFF[k]);
			gamma[k] = desLpfResp(freqP, freq) - sign * delta /
						weightLp(kk,freqP,freq);
		}

	}
	else
	{
		xCont = cos(TWO_PI * contFreq);
		numer = 0.0;
		denom = 0.0;
		for( k=0; k<r; k++)
		{
			term = xCont - x[k];
			if(fabs(term)<1.0e-7)
			{
				aa = gamma[k];
				return aa;
			}
			else
			{
				term = beta[k]/(xCont - x[k]);
				denom += term;
				numer += gamma[k]*term;
			}
		}
		aa = numer/denom;
	}
	return(aa);
}
/**********************************/
/*                                */
/*   Listing 13.7                 */
/*                                */
/*   remezStop()                  */
/*                                */
/**********************************/

int remezStop(	int iFF[],
				int r)
{
	static int oldIFF[50];
	int j,result;

	result = 1;
	for(j=0; j<=r; j++) {
		if(iFF[j] != oldIFF[j]) result = 0;
		oldIFF[j] = iFF[j];
	}
	return(result);
}

/**********************************/
/*                                */
/*   Listing 13.8                 */
/*                                */
/*   remezStop2()                 */
/*                                */
/**********************************/

int remezStop2(	double ee[],
				int iFF[],
				int r)
{
	double biggestVal, smallestVal,qq;
	int j,result;

	result = 0;
	biggestVal = fabs(ee[iFF[0]]);
	smallestVal = fabs(ee[iFF[0]]);
	for(j=1; j<=r; j++) {
		if(fabs(ee[iFF[j]]) < smallestVal) smallestVal = fabs(ee[iFF[j]]);
		if(fabs(ee[iFF[j]]) > biggestVal) biggestVal = fabs(ee[iFF[j]]);
	}
	qq = (biggestVal - smallestVal)/biggestVal;
	if(qq<0.01) result=1;
	return(result);
}

/**********************************/
/*                                */
/*   Listing 13.9                 */
/*                                */
/*   remezFinish()                */
/*                                */
/**********************************/

void remezFinish(	double /*extFreq*/[],
					int nn,
					int r,
					double freqP,
					double kk,
					double aa[],
					double h[])
{
	int k, iFF[1];
	double freq;
	static double gridParam[1];

	for(k=0; k<r; k++) {
		freq = (double) k/ (double) nn;
		aa[k] = computeRemezA(	gridParam, r, kk,
								freqP, iFF, 0,freq);
	}
	fsDesign( nn, 1, aa, h);
	return;
}

/**********************************/
/*                                */
/*   Listing 13.4                 */
/*                                */
/*   remezError()                 */
/*                                */
/**********************************/
void remezError(	double gridParam[],
					int gridMax,
					int r,
					double kk,
					double freqP,
					int iFF[],
					double ee[])
{
	int j;
	double freq,aa;

	aa = computeRemezA(	gridParam, r, kk,
						freqP, iFF, 1, 0.0);

	for( j=0; j<=gridMax; j++) {
		freq = gridFreq(gridParam,j);
		aa = computeRemezA(	gridParam,
							r, kk, freqP,
							iFF, 0,freq);
		ee[j] = weightLp(kk,freqP,freq) *
				(desLpfResp(freqP,freq) - aa);
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 13.6                 */
/*                                */
/*   remezSearch()                */
/*                                */
/**********************************/

void remezSearch(	double ee[],
					double absDelta,
					int gP,
					int iFF[],
					int gridMax,
					int r,
					double /*gridParam*/[])
{
	int i,j,k,extras,indexOfSmallest;
	double smallestVal;

	k=0;

/* test for extremum at f=0  */
	if(	( (ee[0]>0.0) && (ee[0]>ee[1]) && (fabs(ee[0])>=absDelta) ) ||
		( (ee[0]<0.0) && (ee[0]<ee[1]) && (fabs(ee[0])>=absDelta) ) ) {
		iFF[k]=0;
		k++;
	}

/*  search for extrema in passband  */
	for(j=1; j<gP; j++) {
		if( ( (ee[j]>=ee[j-1]) && (ee[j]>ee[j+1]) && (ee[j]>0.0) ) ||
			( (ee[j]<=ee[j-1]) && (ee[j]<ee[j+1]) && (ee[j]<0.0) )) {
			iFF[k] = j;
			k++;
		}
	}

/* pick up an extremal frequency at passband edge  */
	iFF[k]=gP;
	k++;

/* pick up an extremal frequency at stopband edge  */
	j=gP+1;
	iFF[k]=j;
	k++;

/*  search for extrema in stopband  */

	for(j=gP+2; j<gridMax; j++) {
		if( ( (ee[j]>=ee[j-1]) && (ee[j]>ee[j+1]) && (ee[j]>0.0) ) ||
			( (ee[j]<=ee[j-1]) && (ee[j]<ee[j+1]) && (ee[j]<0.0) )) {
			iFF[k] = j;
			k++;
		}
	}
/* test for extremum at f=0.5  */
	j = gridMax;
	if(	( (ee[j]>0.0) && (ee[j]>ee[j-1]) && (fabs(ee[j])>=absDelta) ) ||
		( (ee[j]<0.0) && (ee[j]<ee[j-1]) && (fabs(ee[j])>=absDelta) ) ) {
		iFF[k]=gridMax;
		k++;
	}
/*----------------------------------------------------*/
/*  find and remove superfluous extremal frequencies  */
	if( k>r+1) {
		extras = k - (r+1);
		for(i=1; i<=extras; i++) {
			smallestVal = fabs(ee[iFF[0]]);
			indexOfSmallest = 0;
			for(j=1; j< k; j++) {
				if(fabs(ee[iFF[j]]) >= smallestVal) continue;
				smallestVal = fabs(ee[iFF[j]]);
				indexOfSmallest = j;
			}
			k--;
			for(j=indexOfSmallest; j<k; j++) iFF[j] = iFF[j+1];
		}
	}
	return;
}

/**********************************/
/*                                */
/*   Listing 13.10                */
/*                                */
/*   remez()                      */
/*                                */
/**********************************/

void remez(	int nn,
			int r,
			int gridDensity,
			double kk,
			double freqP,
			double freqS,
			double extFreq[],
			double h[])
{
	int m, gridMax, j, mP, gP, mS;
	double absDelta = 0.0001,freq;
	static double gridParam[10];
	static int iFF[50];
	static double ee[1024];

/*--------------------------------*/
/*  set up frequency grid         */
	gridParam[0] = 1.0;
	gridParam[1] = freqP;
	gridParam[2] = freqS;
	gridParam[3] = r;
	gridParam[4] = gridDensity;
	freq = gridFreq(gridParam,0);
	mP = (int) gridParam[5];
	mS = (int) gridParam[6];
	gP = (int) gridParam[7];
	freqP = freqP + (freqP/(2.0*gP));
	gridMax = 1 + gridDensity*(mP+mS-1);

/*----------------------------------------------*/
/*  make initial guess of extremal frequencies  */

	for(j=0; j<mP; j++) iFF[j] = (j+1)* gridDensity;

	for(j=0; j<mS; j++) iFF[j+mP] = gP + 1 + j * gridDensity;

/*----------------------------------------------------*/
/*  find optimal locations for extremal frequencies   */

	for(m=1;m<=20;m++) {

		remezError(	gridParam, gridMax, r, kk, freqP, iFF, ee);

		remezSearch( ee, absDelta, gP, iFF, gridMax, r, gridParam);

		remezStop2(ee,iFF,r);
		if(remezStop(iFF,r)) break;
	}

	for(j=0; j<=r; j++) {
		extFreq[j] = gridFreq(gridParam,iFF[j]);
	}
	remezFinish( extFreq, nn, r, freqP,kk, ee, h);
	return;
}

/**********************************/
/*                                */
/*   Listing 14.1                 */
/*                                */
/*   iirResponse()                */
/*                                */
/**********************************/
const MAXPOINTS = 100;
const MAXPOLES = 100;

void iirResponse(Complex a[],
				int bigN,
				Complex b[],
				int bigM,
				int numberOfPoints,
				int dbScale,
				double magnitude[],
				double phase[])
{
	Complex response[MAXPOINTS];
	int n, m;
	double phi;

/*----------------------------------------------------*/
/*  compute DFT of H(z) numerator                     */

	for( m=0; m<numberOfPoints; m++) {
		Complex sum(0.);
		printf("\r%d  000",m);
		for(n=0; n<=bigM; n++) {
			printf("\b\b\b%3d",n);
			phi = 2.0 * M_PI * m * n / (2.0*numberOfPoints);
//		printf("b[%d] = (%e, %e)\n",n,double(b[n]),imag(b[n]));
			sum += b[n] * Complex(cos(phi), - sin(phi));
//		sumRe += b[n].re * cos(phi) + b[n].im * sin(phi);
//		sumIm += b[n].im * cos(phi) - b[n].re * sin(phi);
		}
		response[m] = sum;
		printf("response = (%e, %e)\n",real(response[m]), imag(response[m]));
	}

/*----------------------------------------------------*/
/*  compute DFT of H(z) denominator                   */

	for( m=0; m<numberOfPoints; m++) {
		Complex sum = 1.0;

		for(n=1; n<=bigN; n++) {
			phi = 2.0 * M_PI * m * n / (2.0*numberOfPoints);
			sum -= a[n] * Complex(cos(phi), - sin(phi));
		}
		response[m] /= sum;
	}
/*--------------------------------------------------*/
/*  compute magnitude and phase of response         */

	for( m=0; m<numberOfPoints; m++) {
		phase[m] = arg(response[m]);
		if(dbScale)
		{magnitude[m] = 20.0 * log10(abs(response[m]));}
		else
		{magnitude[m] = abs(response[m]);}
		printf("mag = %e\n",magnitude[m]);
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

