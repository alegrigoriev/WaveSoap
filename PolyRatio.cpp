//#include "Complex.h"
#include "stdafx.h"
#include "PolyRatio.h"
#include <float.h>
#ifdef NOMINMAX
#include <algorithm>
using std::min;
using std::max;
#endif

polyRatio::polyRatio(Complex first, int MaxOrder)
	: m_denom(0, Complex(1.), MaxOrder), m_numer(0, first, MaxOrder)
{
}

polyRatio::polyRatio(const poly& pNumer, const poly& pDenom)
	: m_denom(pDenom), m_numer(pNumer)
{
}

polyRatio::polyRatio(Complex *zeros, int nOrder,
					Complex *poles, int dOrder, Complex first)
	: m_denom(poles, dOrder, Complex(1.)),
	m_numer(zeros, nOrder, first)
{
}

polyRatio::polyRatio(const polyRatio & p)
	: m_numer(p.m_numer), m_denom(p.m_denom)
{
}

polyRatio::~polyRatio()
{
}

polyRatio & polyRatio::operator +=(const Complex &a)
{
	if (a !=0.)
	{
		m_numer += m_denom * a;
	}
	return *this;
}

polyRatio & polyRatio::operator -=(const Complex &a)
{
	if (a !=0.)
	{
		m_numer -= m_denom * a;
	}
	return *this;
}

polyRatio & polyRatio::operator +=(double a)
{
	if (a !=0.)
	{
		m_numer += m_denom * a;
	}
	return *this;
}

polyRatio & polyRatio::operator -=(double a)
{
	if (a !=0.)
	{
		m_numer -= m_denom * a;
	}
	return *this;
}

polyRatio & polyRatio::operator +=(const poly &a)
{
	m_numer += m_denom * a;
	return *this;
}

polyRatio & polyRatio::operator -= (const poly &a)
{
	m_numer -= m_denom * a;
	return *this;
}

polyRatio & polyRatio::operator +=(const polyRatio &a)
{
	if (m_numer.IsZero())
	{
		*this = a;
	}
	else if ( ! a.m_numer.IsZero())
	{
		m_numer = m_numer * a.m_denom + m_denom * a.m_numer;
		m_denom *= a.m_denom;
	}
	return *this;
}

polyRatio & polyRatio::operator -= (const polyRatio &a)
{
	if (m_numer.IsZero())
	{
		*this = -a;
	}
	else if ( ! a.m_numer.IsZero())
	{
		m_numer = m_numer * a.m_denom - m_denom * a.m_numer;
		m_denom *= a.m_denom;
	}
	return *this;
}

polyRatio & polyRatio::operator *= (const Complex &a)
{
	m_numer *= a;
	return *this;
}

polyRatio & polyRatio::operator *= (const poly &a)
{
	m_numer *= a;
	return *this;
}

polyRatio & polyRatio::operator /= (const Complex &a)
{
	m_numer /= a;
	return *this;
}

polyRatio & polyRatio::operator /= (const poly &a)
{
	m_denom *= a;
	return *this;
}

polyRatio & polyRatio::operator *= (double a)
{
	m_numer *= a;
	return *this;
}

polyRatio & polyRatio::operator /= (double a)
{
	m_numer /= a;
	return *this;
}

polyRatio & polyRatio::operator *= (const polyRatio &a)
{
	m_numer *= a.m_numer;
	m_denom *= a.m_denom;
	return *this;
}

polyRatio __stdcall operator *	(const polyRatio & mult1, const Complex &mult2)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}
polyRatio __stdcall operator *	(const polyRatio & mult1, const poly &mult2)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}
polyRatio __stdcall operator *	(const polyRatio & mult1, double mult2)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}
polyRatio __stdcall operator *	(const Complex &mult2, const polyRatio & mult1)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}
polyRatio __stdcall operator *	(const poly &mult2, const polyRatio & mult1)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}
polyRatio __stdcall operator *	(double mult2, const polyRatio & mult1)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}
polyRatio __stdcall operator *	(const polyRatio & mult1, const polyRatio &mult2)
{
	polyRatio p(mult1);
	p *= mult2;
	return p;
}

polyRatio __stdcall operator /	(const polyRatio & arg1, const Complex &arg2)
{
	polyRatio p(arg1);
	p /= arg2;
	return p;
}
polyRatio __stdcall operator /	(const polyRatio & arg1, const poly &arg2)
{
	polyRatio p(arg1);
	p /= arg2;
	return p;
}
polyRatio __stdcall operator /	(const polyRatio & arg1, double arg2)
{
	polyRatio p(arg1);
	p /= arg2;
	return p;
}

polyRatio __stdcall operator +	(const polyRatio & arg1, const Complex &arg2)
{
	polyRatio p(arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator +	(const polyRatio & arg1, const poly &arg2)
{
	polyRatio p(arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator +	(const polyRatio & arg1, double arg2)
{
	polyRatio p(arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator +	(const Complex &arg2, const polyRatio & arg1)
{
	polyRatio p(arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator +	(const poly &arg2, const polyRatio & arg1)
{
	polyRatio p(arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator +	(double arg2, const polyRatio & arg1)
{
	polyRatio p(arg1);
	p *= arg2;
	return p;
}
polyRatio __stdcall operator +	(const polyRatio & arg1, const polyRatio &arg2)
{
	polyRatio p(arg1);
	p += arg2;
	return p;
}

polyRatio __stdcall operator -	(const polyRatio & arg1, const Complex &arg2)
{
	polyRatio p(arg1);
	p -= arg2;
	return p;
}
polyRatio __stdcall operator -	(const polyRatio & arg1, const poly &arg2)
{
	polyRatio p(arg1);
	p -= arg2;
	return p;
}
polyRatio __stdcall operator -	(const polyRatio & arg1, double arg2)
{
	polyRatio p(arg1);
	p -= arg2;
	return p;
}
polyRatio __stdcall operator -	(const Complex &arg2, const polyRatio & arg1)
{
	polyRatio p(-arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator -	(const poly &arg2, const polyRatio & arg1)
{
	polyRatio p(-arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator -	(double arg2, const polyRatio & arg1)
{
	polyRatio p(-arg1);
	p += arg2;
	return p;
}
polyRatio __stdcall operator -	(const polyRatio & arg1, const polyRatio &arg2)
{
	polyRatio p(arg1);
	p -= arg2;
	return p;
}

// throw out high-order terms, considering them negligible
polyRatio & polyRatio::operator <<=(int i)
{
	m_numer <<= i;
	return *this;
}
// Add a number of zero high-order terms
polyRatio & polyRatio::operator >>=(int i)
{
	m_numer >>= i;
	return *this;
}

polyRatio polyRatio::operator -() const
{
	return polyRatio(-m_numer, m_denom);
}

polyRatio __stdcall operator <<(const polyRatio & p, int i)
{
	polyRatio tmp(p);
	tmp <<= i;
	return tmp;
}

polyRatio __stdcall operator >>(const polyRatio & p, int i)
{
	polyRatio tmp(p);
	tmp >>= i;
	return tmp;
}

polyRatio & polyRatio::operator /= (const polyRatio & pDenom)
{
	m_denom *= pDenom.m_numer;
	m_numer *= pDenom.m_denom;
	return *this;
}

polyRatio __stdcall operator / (const polyRatio & pNumer, const polyRatio & pDenom)
{
	return polyRatio(pNumer.m_numer*pDenom.m_denom,
					pNumer.m_denom*pDenom.m_numer);
}

polyRatio & polyRatio::operator = (const polyRatio & src)
{
	m_numer = src.m_numer;
	m_denom = src.m_denom;
	return *this;
}

Complex	polyRatio::eval(Complex arg) const
{
	return m_numer.eval(arg) / m_denom.eval(arg);
}

// produce derivative ratio
polyRatio polyRatio::deriv(void) const
{
	return 	polyRatio(m_numer.deriv() * m_denom - m_denom.deriv()* m_numer,
					m_denom * m_denom);
}

// scale all roots (the same as scale the argument to 1/scale)
void polyRatio::ScaleRoots(const Complex & scale)
{
	m_denom.ScaleRoots(scale);
	m_numer.ScaleRoots(scale);
}

#ifdef _DEBUG
void polyRatio::Dump(CDumpContext & dc)
{
	dc << "polyRatio: Numerator\n";
	numer().Dump(dc);
	dc << "polyRatio: Denominator\n";
	denom().Dump(dc);
	dc << "polyRatio: End of dump\n\n";
}
#endif

// decompose the ratio to the elementary ratios
// of order nCellsOrder. Denominator roots may be given
// in *pDenomRoots. If pDenomRoots == NULL,
// roots are calculated
CArray<polyRatio *, polyRatio *>
	*polyRatio::Decompose(int nCellsOrder, polyRoots * pDenomRoots,
						int nFirstRatioOrder)
{
	polyRoots pr;
	if (nCellsOrder <= 0) nCellsOrder = 1;
	if (nFirstRatioOrder <= 0
		|| nFirstRatioOrder > nCellsOrder)
	{
		nFirstRatioOrder = nCellsOrder;
	}

	if (pDenomRoots != NULL)
	{
		ASSERT(pDenomRoots->count() == denom().order());
		pr = * pDenomRoots;
	}
	else
	{
		pr = denom().roots();
	}

	CArray<polyRatio *, polyRatio *> * pprArray
		= new CArray<polyRatio *, polyRatio *>;

	// reduce the ratio
	if (numer().order() >= denom().order())
	{
		poly common;
		PolyDiv( & common, NULL, numer(), denom());
		pprArray->Add(new polyRatio(common));
#if 0 && defined( _DEBUG)
		common.Dump();
		(*pprArray)[0]->Dump();
#endif
	}

	for (int i = nFirstRatioOrder - nCellsOrder; i < denom().order(); i += nCellsOrder)
	{
		polyRoots CellPoles;
		polyRoots OtherPoles;
		for (int j = 0; j < denom().order(); j++)
		{
			if (j >= i && j < i + nCellsOrder)
			{
				CellPoles += pr[j];
			}
			else
			{
				OtherPoles += pr[j];
			}
		}
		polyRatio cell;
		for (j = 0; j < CellPoles.count(); j++)
		{
			polyRoots SingleRoot;
			polyRoots OtherRoots(OtherPoles);
			SingleRoot += CellPoles[j];
			for (int k = 0; k < CellPoles.count(); k++)
			{
				if (k != j)
				{
					OtherRoots += CellPoles[k];
				}
			}
#if 0 && defined( _DEBUG)
			polyRatio (poly(0, Numer.eval(CellPoles[j])
							/ (OtherRoots.eval(CellPoles[j]))),
						SingleRoot).Dump();
#endif
			cell += polyRatio(poly(0, numer().eval(CellPoles[j])
									/ (OtherRoots.eval(CellPoles[j]))),
							SingleRoot);
		}
		pprArray->Add(new polyRatio(cell));
	}
	return pprArray;
}
