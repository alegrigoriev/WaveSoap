//#include "Complex.h"
#include "PolyMath.h"
#include "fstream.h"
#include "float.h"
#if defined(_DEBUG)
CDumpContext & operator << (CDumpContext & dc, Complex c)
{
	dc << "(" << c.re << "," << c.im << ")";
	return dc;
}
CDumpContext & operator << (CDumpContext & dc, const lcomplex & c)
{
	dc << "(" << c.re << "," << c.im << ")";
	return dc;
}
int PolyIterCount=0;
int volatile FloatingExceptionFlag;
#endif

// copy constructor
complexArray::complexArray(const complexArray &ca)
{
	iRefCount=1;
	pArray = NULL;
	if (ca.iAllocatedSize != 0)
	{
		ASSERT(ca.pArray != NULL);
		pArray = new Complex[ca.iAllocatedSize];
		memcpy(pArray, ca.pArray, ca.iAllocatedSize*sizeof *pArray);
	}
	iAllocatedSize = ca.iAllocatedSize;
}

complexArray::complexArray(const lcomplexArray &ca)
{
	iRefCount=1;
	pArray = NULL;
	if (ca.iAllocatedSize != 0)
	{
		ASSERT(ca.pArray != NULL);
		pArray = new Complex[ca.iAllocatedSize];
		for (int i=0; i < ca.iAllocatedSize; ++i)
			pArray[i] = ca.pArray[i];
	}
	iAllocatedSize = ca.iAllocatedSize;
}

complexArray *complexArray::MakeUnique()
{
	if (iRefCount == 1) return this;
	complexArray * ptr=new complexArray(*this);
	iRefCount--;
	return ptr;
}

complexArray::~complexArray()
{
	ASSERT(iRefCount == 0);
	delete [] pArray;
}

void complexArray::Allocate(int size)
{
	ASSERT(size >= 0);
	if (0 == size) size = 1;
	if (size <= iAllocatedSize) return;
	Complex * ptr = new Complex[size];
	VERIFY(ptr != 0);
	if(pArray != NULL)
	{
		memcpy(ptr, pArray, iAllocatedSize * sizeof * pArray);
		delete[] pArray;
	}
	iAllocatedSize = size;
	pArray = ptr;
}

lcomplexArray::lcomplexArray(const lcomplexArray &ca)
{
	iRefCount=1;
	pArray = NULL;
	if (ca.iAllocatedSize != 0)
	{
		ASSERT(ca.pArray != NULL);
		pArray = new lcomplex[ca.iAllocatedSize];
		memcpy(pArray, ca.pArray, ca.iAllocatedSize*sizeof *pArray);
	}
	iAllocatedSize = ca.iAllocatedSize;
}

lcomplexArray::lcomplexArray(const complexArray &ca)
{
	iRefCount=1;
	pArray = NULL;
	if (ca.iAllocatedSize != 0)
	{
		ASSERT(ca.pArray != NULL);
		pArray = new lcomplex[ca.iAllocatedSize];
		for (int i=0; i < ca.iAllocatedSize; ++i)
			pArray[i] = ca.pArray[i];
	}
	iAllocatedSize = ca.iAllocatedSize;
}

lcomplexArray *lcomplexArray::MakeUnique()
{
	if (iRefCount == 1) return this;
	lcomplexArray * ptr=new lcomplexArray(*this);
	iRefCount--;
	return ptr;
}

lcomplexArray::~lcomplexArray()
{
	ASSERT(iRefCount == 0);
	delete [] pArray;
}

void lcomplexArray::Allocate(int size)
{
	ASSERT(size >= 0);
	if (0 == size) size = 1;
	if (size <= iAllocatedSize) return;
	lcomplex * ptr = new lcomplex[size];
	VERIFY(ptr != 0);
	if(pArray != NULL)
	{
		memcpy(ptr, pArray, iAllocatedSize * sizeof * pArray);
		delete[] pArray;
	}
	iAllocatedSize = size;
	pArray = ptr;
}

poly::poly(int iInitOrder, Complex first, int MaxOrder)
{
	iMaxOrder = MaxOrder;
	cmArray = new complexArray;
	iOrder = 0;
	isReal = 1;
	SetOrder(iInitOrder);
	(*this)[0]=first;
	if (imag(first) != 0) isReal = 0;
}

poly::poly(const Complex *roots, int iInitOrder, Complex first, int MaxOrder)
{
	iMaxOrder = MaxOrder;
	cmArray = new complexArray;
	iOrder = 0;
	isReal = 1;
	SetOrder(0);
	FromRoots(polyRoots(roots, iInitOrder), first);
}

poly::poly(const polyRoots &roots, Complex first, int MaxOrder)
{
	iMaxOrder = MaxOrder;
	cmArray = new complexArray;
	iOrder = 0;
	isReal = 1;
	SetOrder(0);
	FromRoots(roots, first);
}

poly::poly(const poly & p)
{
	p.cmArray->IncRefCount();
	cmArray = p.cmArray;
	iOrder = p.iOrder;
	isReal = p.isReal;
	iMaxOrder = p.iMaxOrder;
}

poly::poly(const lpoly & p)
{
	cmArray = new complexArray(*p.cmArray);
	iOrder = p.iOrder;
	isReal = p.isReal;
	iMaxOrder = p.iMaxOrder;
}

poly::~poly()
{
	SafeDelete(cmArray);
}

void poly::FromRoots(const polyRoots & roots, Complex first)
{
	int iInitOrder = roots.count();
	if (iInitOrder <= 0)
	{
		SetOrder(0);
		(*this)[0] = first;
		return;
	}
	if (imag(first) != 0) isReal = 0;
	poly * pp = new poly[(iInitOrder + 1)/2], *pp1=pp, *pp2;
	for (int i = 0; i < iInitOrder; i += 2, ++pp1)
	{
		pp1->SetOrder(1);
		(*pp1)[0] = 1.;
		(*pp1)[1] = -roots[i];
		poly p1(1, 1.);
		if (i + 1 < iInitOrder)
		{
			p1 -= roots[i+1];
			*pp1 *= p1;
		}
	}

	for (i = (iInitOrder+1)/2; i > 1; i = (i + 1) /2)
	{
		int j;
		for (j = 0, pp2 = pp, pp1 = pp; j < i; j += 2, ++pp2, pp1 += 2)
		{
			if (j != (i & ~1))
				pp2[0] = pp1[0] * pp1[1];
			else
				pp2[0] = pp1[0];
		}
	}
	*this = pp[0] * first;
	delete[] pp;
}

void __stdcall SafeDelete(complexArray * a)
{
	if(a != NULL && --(a->iRefCount) == 0) delete a;
}

poly & poly::operator +=(const Complex &a)
{
	MakeUnique();
	Allocate(0);
	array()[iOrder] += a;
	if(imag(a) != 0.) isReal = 0;
	return *this;
}

poly & poly::operator -=(const Complex &a)
{
	MakeUnique();
	Allocate(0);
	array()[iOrder] -= a;
	if(imag(a) != 0.) isReal = 0;
	return *this;
}

poly & poly::operator +=(double a)
{
	if (a !=0)
	{
		MakeUnique();
		Allocate(0);
		array()[iOrder] += a;
	}
	return *this;
}

poly & poly::operator -=(double a)
{
	if (a !=0)
	{
		MakeUnique();
		Allocate(1);
		array()[iOrder] -= a;
	}
	return *this;
}

poly & poly::operator +=(const poly &a)
{
	MakeUnique();
	int ord=order(), ord1=a.order();
	if(ord < ord1)
		*this >>= ord1 - ord;
	Complex * dst = array()+order()-ord1;
	const Complex *src = a.array();
	for (++ord1; ord1--; dst++, src++) *dst += *src;
	isReal &= a.isReal;
	return *this;
}

poly & poly::operator -= (const poly &a)
{
	MakeUnique();
	int ord=order(), ord1=a.order();
	if(ord < ord1)
		*this >>= ord1 - ord;
	Complex * dst = array()+order()-ord1;
	const Complex *src = a.array();
	for (++ord1; ord1--; dst++, src++) *dst -= *src;
	isReal &= a.isReal;
	return *this;
}

poly & poly::operator *= (Complex a)
{
	MakeUnique();
	Complex *dst = array();
	for(int ord = order()+1; ord--; dst++) *dst *= a;
	if(imag(a) != 0.) isReal = 0;
	return *this;
}

poly & poly::operator /= (const Complex &a)
{
	MakeUnique();
	Complex *dst = array(), b= 1./a;
	for(int ord = order()+1; ord--; dst++) *dst *= b;
	if(imag(a) != 0.) isReal = 0;
	return *this;
}

poly & poly::operator *= (double a)
{
	MakeUnique();
	Complex *dst = array();
	for(int ord = order()+1; ord--; dst++) *dst *= a;
	return *this;
}

poly & poly::operator /= (double a)
{
	MakeUnique();
	Complex *dst = array();
	for(int ord = order()+1; ord--; dst++) *dst /= a;
	return *this;
}

poly & poly::operator *= (const poly &a)
{
	MakeUnique();
	Allocate(order() + a.order());
	ASSERT( array() != a.array()); // make sure we don't override the array
	int ord = order();
	SetOrder(order() + a.order()); // order is changed
	Complex * dst = array() + ord, mult;
	// ord - dest polynom order (before expanding)
#ifndef USE_POLY_ASM
	const Complex *tmp2;
	Complex * tmp1;
	int j;
	for (ord++; ord; ord--, --dst)
	{
		mult= *dst;
		*dst = 0;
		for(j = a.order()+1, tmp1=dst, tmp2 = a.array(); j--;
			++tmp1, ++ tmp2)
			*tmp1 += *tmp2 * mult;
	}
	isReal &= a.isReal;
#else
	__PolyMultiply(array(), array(), a.array(), ord, a.order());
#endif
	return *this;
}
poly __stdcall operator *	(const poly & mult1, const Complex &mult2)
{
	poly p(mult1);
	p *= mult2;
	return p;
}
poly __stdcall operator *	(const poly & mult1, double mult2)
{
	poly p(mult1);
	p *= mult2;
	return p;
}
poly __stdcall operator *	(const Complex &mult2, const poly & mult1)
{
	poly p(mult1);
	p *= mult2;
	return p;
}
poly __stdcall operator *	(double mult2, const poly & mult1)
{
	poly p(mult1);
	p *= mult2;
	return p;
}
poly __stdcall operator *	(const poly & mult1, const poly &mult2)
{
	poly p(mult1);
	p *= mult2;
	return p;
}

poly __stdcall operator +	(const poly & arg1, const Complex &arg2)
{
	poly p(arg1);
	p += arg2;
	return p;
}
poly __stdcall operator +	(const poly & arg1, double arg2)
{
	poly p(arg1);
	p += arg2;
	return p;
}
poly __stdcall operator +	(const Complex &arg2, const poly & arg1)
{
	poly p(arg1);
	p += arg2;
	return p;
}
poly __stdcall operator +	(double arg2, const poly & arg1)
{
	poly p(arg1);
	p += arg2;
	return p;
}
poly __stdcall operator +	(const poly & arg1, const poly &arg2)
{
	poly p(arg1);
	p += arg2;
	return p;
}

poly __stdcall operator -	(const poly & arg1, const Complex &arg2)
{
	poly p(arg1);
	p -= arg2;
	return p;
}
poly __stdcall operator -	(const poly & arg1, double arg2)
{
	poly p(arg1);
	p -= arg2;
	return p;
}
poly __stdcall operator -	(const Complex &arg2, const poly & arg1)
{
	poly p(-arg1);
	p += arg2;
	return p;
}
poly __stdcall operator -	(double arg2, const poly & arg1)
{
	poly p(-arg1);
	p += arg2;
	return p;
}
poly __stdcall operator -	(const poly & arg1, const poly &arg2)
{
	poly p(arg1);
	p -= arg2;
	return p;
}

void __stdcall PolyDiv(poly *quot, poly *rem, const poly &numer, const poly &denom)
{
	poly quot1, rem1(numer);
	rem1.MakeUnique();
	const Complex * dptr = denom.array();
	Complex * rptr = rem1.array();
	int denord = denom.order(), numord = numer.order();
// skip first (higher order) zero terms
	while (*dptr == 0. && denord >= 0) dptr++, --denord;
	if (denord < 0) throw poly(); // all terms of denominator are zero
	int iCount = numord - denord + 1;	// number of steps
	if (iCount > 0) quot1.SetOrder(iCount-1);
#ifndef USE_POLY_ASM
	int i;
	for (int termNum = 0; termNum < iCount; ++termNum, ++rptr)
	{
		Complex coef, * rptr1=rptr;
		const Complex * dptr1 = dptr;
		coef = *rptr / *dptr;
		quot1[termNum] = coef;
		for (i = 0; i < denord; i++) *(++rptr1) -= *(++dptr1) * coef;
	}
#else
	if (iCount > 0) __PolyDivide(quot1.array(), rptr+(numord - denord+1),
								rptr, dptr, numord, denord);

#endif
	if (quot != NULL) *quot = quot1;
	if (rem != NULL)
	{
		if (numord >= denord) rem1 <<= numord - denord+1;
		*rem = rem1;
	}
}

void poly::SetOrder(int ord)
{
	MakeUnique();
	Allocate(((ord+4) & ~3)-1);
	int i= iOrder+1;
	iOrder = ord;
	for(; i <= ord; i++) (*this)[i] = 0.;
}
// throw out high-order terms, considering them negligible
poly & poly::operator <<=(int i)
{
	MakeUnique();
	ASSERT(i >= 0);
	if(i)
	{
		for (int j=0, k=i; k <= order(); ++j, ++k) (*this)[j] = (*this) [k];
		iOrder -= i;
	}
	return *this;
}
// Add a number of zero high-order terms
poly & poly::operator >>=(int i)
{
	ASSERT(i >= 0);
	SetOrder(iOrder + i);
	for(int j=order(), k=j-i; k >= 0; --j, --k) (*this)[j] = (*this) [k];
	for(; j >= 0; j--) (*this)[j] = 0.;
	return *this;
}

poly poly::operator -() const
{
	poly p(*this);
	p.MakeUnique();
	for(int i=order()+1; i--;) p[i] = -p[i];
	return p;
}

poly __stdcall operator <<(const poly & p, int i)
{
	poly tmp(p);
	tmp <<= i;
	return tmp;
}

poly __stdcall operator >>(const poly & p, int i)
{
	poly tmp(p);
	tmp >>= i;
	return tmp;
}

poly & poly::operator /= (const poly & denom)
{
	PolyDiv(this, NULL, *this, denom);
	return *this;
}

poly & poly::operator %= (const poly & denom)
{
	PolyDiv(NULL, this, *this, denom);
	return *this;
}

poly __stdcall operator / (const poly & numer, const poly & denom)
{
	poly quot;
	PolyDiv(&quot, NULL, numer, denom);
	return quot;
}

poly __stdcall operator % (const poly & numer, const poly & denom)
{
	poly rem;
	PolyDiv(NULL, &rem, numer, denom);
	return rem;
}

poly & poly::operator = (const poly & src)
{
	src.cmArray->IncRefCount();
	SafeDelete(cmArray);
	cmArray = src.cmArray;
	iOrder = src.iOrder;
	return *this;
}

Complex	poly::eval(Complex arg) const
{
	Complex res=0.;
	for(int i=0; i <= order(); i++)
	{
		//Complex coef = (*this)[i];
		res = res * arg + (*this)[i];
	}
	return res;
}

// produce derivative polynom
poly poly::deriv(void) const
{
	int ord=order();
	if (ord) ord--;
	poly dst(ord,0.);
	double cx = 1.;
	for (int i = order(); i --; cx += 1.) dst[i] = (*this)[i] * cx;
	return dst;
}

// produce originate polynom
poly poly::orig(void) const
{
	poly dst(order()+1);
	double cx = 1.;
	for (int i = order()+1; i --; cx += 1.) dst[i] = (*this)[i] / cx;
	// dst[i] = 0.;
	return dst;
}

BOOL poly::IsZero() const
{
	for (int i = 0; i <= order(); i++)
	{
		if (array()[i] != 0.) return FALSE;
	}
	return TRUE;
}

void poly::Normalize()
{
	for (int i = 0; i <= order(); i++)
	{
		if (array()[i] != 0.) break;
	}
	if (i > order())
	{
		SetOrder(0);
	}
	else
	{
		*this <<= i;
	}
	return;
}

// find polynom roots.
polyRoots poly::roots(Complex start, int iIter) const
{
	static int lastRootCount;
	// lastRootCount keeps number of roots calculated
	// on previous lpoly::roots invocation (counting
	// nested calls off).
	//
//  В lastRootCount хранится число корней, полученное на
//  предыдущем вызове lpoly::roots (без учета полученных
//  во вложенных вызовах). Столько корней копируется в массив roots.
#ifdef _DEBUG
	FloatingExceptionFlag = 0;
#endif
	ASSERT(iIter > 2);
	lastRootCount = 0;
	poly src(*this);
	polyRoots roots1;
	int rootCounter = 0, rootCount;
	// discard zero high order terms
	while (src.order() > 0 && src[0] == 0.)	src <<= 1;
	rootCount = src.order();
	if (rootCount == 0) return 0;
// discard Z=0 roots if any
	while (src[src.order()] == 0.)
	{
		roots1 += 0.;
		src.SetOrder(src.order()-1);
	}

	poly deriv1;
	poly deriv2, deriv3;
	poly srcderiv1, srcderiv2, srcderiv3;
	srcderiv1 = src.deriv();
// check if the polynom has only prime roots
// find greatest common divisor of src and srcderiv1
	{
		_clearfp();
		poly r1(src), r2(srcderiv1),r3, r4;
		int j;
		for (int i=rootCount-1; i >0; i--)
		{
			PolyDiv(&r4, &r3, r1, r2);
// check if |r3| is small enough
			double norm_=0., norm1=0.;
			for (j = 0; j <=r3.order(); ++j)
				norm_ += sqrt(norm(r3[j]));
			for (j = 0; j <=r2.order(); ++j)
				norm1 += sqrt(norm(r2[j]));
			norm_ /= j;
			norm_ /= (double)sqrt(norm(r4[0]))*norm1;
			int fpstatus = _statusfp();
			if (fpstatus & MCW_EM)
			{
				_clearfp();
				if (fpstatus & (EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE))
				{
#if defined(_DEBUG)
					if (fpstatus & EM_ZERODIVIDE)
						afxDump << "Divide By Zero in reduce\n";
					if (fpstatus & EM_OVERFLOW)
						afxDump  << "Floating Overflow in reduce\n";
					if (fpstatus & EM_UNDERFLOW)
						afxDump  << "Floating Underflow in reduce\n";
#endif
					i =0; break;
				}
			}
//		afxDump << "Norm = " << norm_ << "\n";
			if (norm_ < 1e-5) break;
// swap polynoms
			r1 = r2;
			r2 = r3;
		}
		if ( i >  0)
		{
// найти корни r2 - это кратные корни
			int oldRootCount = rootCounter;
			polyRoots roots2 = r2.roots(start, iIter);
			int newRootCount = roots2.count();
#if defined(_DEBUG)
			afxDump << "Coupled roots: " << newRootCount << "\n";
#endif
			rootCounter += newRootCount + lastRootCount;
			for (i = 0; i< lastRootCount; ++i)
			{
				roots1 += roots2[i];
			}
			roots1 += roots2;
			src	/= poly(&(roots1[oldRootCount]), newRootCount + lastRootCount);
			srcderiv1 = src.deriv();
			rootCounter += newRootCount + lastRootCount;
		}
	}
	poly tmp1, tmp2(src);
	srcderiv2 = srcderiv1.deriv();
	srcderiv3 = srcderiv2.deriv();
// tmp1 to keep product of all the roots and tmp2 to keep src/tmp1
	poly term(1);
// use 2-nd order gradiend descent method
	Complex delta, a, b, c, d, b2, ac;
	Complex ddelta, dstart, da, db, dc;
	int oldRootCount = rootCounter;
	for (; rootCounter < rootCount; rootCounter++, lastRootCount++ )
	{
// find first and second derivatives
		tmp2 = src / poly(roots1.array() + oldRootCount, rootCounter-oldRootCount);
		deriv1 = tmp2.deriv();
		deriv2 = deriv1.deriv();
		deriv3 = deriv2.deriv();
		for (int iters=iIter; iters--;)
		{
			if (iIter - iters <= 300)
			{
				c = tmp2.eval(start);
				b=deriv1.eval(start);
				a=deriv2.eval(start);
				d = deriv3.eval(start) ;
			}
			else
			{
				c = src.eval(start);
				b = srcderiv1.eval(start);
				a = srcderiv2.eval(start);
				d = srcderiv3.eval(start) ;
			}

//			if (iIter - iters <= 20 || iIter - iters >= 30)
			if (deriv2.order() <= 0)
			{
				if (iIter - iters >= 30)
				{
					c = b;
					b = a;
					a = srcderiv3.eval(start);
				}
				b2 = b*b;
				ac = 2.*a*c;
// if ac is much less than b2 then use alternate formula
				if (norm(ac)*16000000. < norm(b2))
					delta = -c/b * (1. - ac/(b2*4.));
				else
				{
					d = sqrt(b2 - ac);
					Complex d1, d2;
					if (norm(d)*1e11 < norm(b))
					{
						start -= b/a;
						break;
					}
					else
						if (real(b) * real(d) + imag(b) * imag(d) > 0.)
							d -= b;
						else
							d = -b - d;
					if (a == 0.)
					{
						start +=Complex(0.1, 0.1);
						break; // continue;
					}
					delta = d/a;
				}
			}
			else
// slow convergence, apply third order method
			{
// use Kardano method (G. & Th. Korn)
//				c = tmp2.eval(start);
//				b=deriv1.eval(start);
//				a=deriv2.eval(start) /2.;
// If iterations are too many, change the point
				if ((iIter - iters) % 34 == 33)
					start *= Complex(0.707, 0.707);
				if ((iIter - iters) % 34 >= 20)
				{
					c = b;
					b = a;
					a = d;
					d = deriv3.deriv().eval(start);
				}
				d = 6. / d;
				c *= d;
				b *= d;
				a *= d / 2.;
				Complex p, q, p1, q1, Q, A, B, y1, y2, y3;
				Complex AplusB, AminusB;
//				Complex cy1, cy2, cy3;
				p = b - a * a / 3.;
				q = c / 2. - a * b / 6. +
					a * a * a / 27.;
				p1 = p * p * p / 27.;
				q1 = q * q;
//				if (norm(q1) * 16000000. < norm(p1))
//					{
//					AplusB = q / (-1.5 * p);
//					AminusB = sqrt( p / 12.);
//					}
//				else
				{
//					if (norm(p1) * 16000000. < norm(q1))
//						{
//						B= - pow(q*2., 1/3.);
//						A= -p / (B * 3.);
//						}
//						else
					{
						Q = sqrt(p1 + q1);
						if (real(Q) * real(q) + imag(Q) * imag(q) > 0.)
							A = pow(-q - Q, 1/3.);
						else
							A = pow(-q + Q, 1/3.);
						B = -p / (A * 3.);
					}
					AplusB = A + B;
					AminusB = A - B;
				}
				a /= 3.;
				y1 = AplusB - a;
				AplusB *= -0.5;
				AminusB *= sqrt(3.) * Complex(0., 0.5);
				y2 = AplusB + AminusB - a;
				y3 = AplusB - AminusB - a;
				delta = y1;
				if (norm(delta) > norm(y2)) delta = y2;
				if (norm(delta) > norm(y3)) delta = y3;
			}
#if defined(_DEBUG)
			PolyIterCount++;
#endif
// check if we should stop iterations
			dstart = start += delta;
			if (norm(delta)*1e30 < norm(start)) break;
		}
//		if (iters < 0)
//			ASSERT(iters >= 0);	// iterations converged
// refine root using full polynom
#if 1

//		if(iIter - iters > 20)
		if (srcderiv2.order() > 0)
		{
			c = src.eval(start);
			b=srcderiv1.eval(start);
			a=srcderiv2.eval(start);
			d = 6./ srcderiv3.eval(start) ;
			c *= d;
			b *= d;
			a *= d / 2.;
			Complex p, q, p1, q1, Q, A, B, y1, y2, y3;
			Complex AplusB, AminusB;
//		Complex cy1, cy2, cy3;
			p = b - a * a / 3.;
			q = c / 2. - a * b / 6. +
				a * a * a / 27.;
			p1 = p * p * p / 27.;
			q1 = q * q;
//				if (norm(q1) * 16000000. < norm(p1))
//					{
//					AplusB = q / (-1.5 * p);
//					AminusB = sqrt( p / 12.);
//					}
//				else
//					if (norm(p1) * 16000000.) < norm(q1))
//						{
//						B= - pow(q*2., 1/3.);
//						A= -p / (B * 3.);
//						}
//						else
			Q = sqrt(p1 + q1);
			if (real(Q) * real(q) + imag(Q) * imag(q) > 0.)
				A = pow(-q - Q, 1/3.);
			else
				A = pow(-q + Q, 1/3.);
			B = -p / (A * 3.);
			AplusB = A + B;
			AminusB = A - B;
			a /= 3.;
			y1 = AplusB - a;
			AplusB *= -0.5;
			AminusB *= Complex(0., sqrt(0.75));
			y2 = AplusB + AminusB - a;
			y3 = AplusB - AminusB - a;
			d = y1;
			if (norm(d) > norm(y2)) d = y2;
			if (norm(d) > norm(y3)) d = y3;
			if (norm(d) < norm(delta)) delta = d;
			else delta = Complex(0.,0.);

			if (0)
			{
				b2 = b*b;
				ac = 2.*a*c;
// if ac is much less than b2 then use alternate formula
				if (norm(ac)*1600000. < norm(b2))
					delta = -c/b * (1.- ac/(b2*4.));
				else
				{
					d = sqrt(b2 - ac);
					Complex d1, d2;
					if (norm(d)*1e10 < norm(b))
						d = -b;
					else
						if (real(b) * real(d) + imag(b) * imag(d) > 0.)
							d -= b;
						else
							d = -b - d;
					if (a == 0.)
						delta = 0.;
					else delta = d/a;
				}
			}
#endif
			if (!_finite(real(delta)) || !_finite(imag(delta)))
			{
#if defined(_DEBUG)
				afxDump << "Bad final delta\n"
					<< "start=" << start <<"\n"
					<< "b2=" << b2 << ", ac=" << ac << "\n"
					<< "a=" << a
					<< ", b=" << b
					<< "\nc=" << c
					<< ", d=" << d <<"\n";
#endif
				break;
			}
			start += delta;
		}
		if (!_finite(real(start)) || !_finite(imag(start)))
		{
#if defined(_DEBUG)
			afxDump << "Bad final root value\n";
#endif
			break;
		}
		int fpstatus = _statusfp();
		if (fpstatus & MCW_EM)
		{
			_clearfp();
			if (fpstatus & (EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE))
			{
#if defined(_DEBUG)
				if (fpstatus & EM_ZERODIVIDE)
					afxDump  << "Divide By Zero\n";
				if (fpstatus & EM_OVERFLOW)
					afxDump  << "Floating Overflow\n";
				if (fpstatus & EM_UNDERFLOW)
					afxDump  << "Floating Underflow\n";
#endif
				break;
			}
		}
		roots1 += start;
//		if (FloatingExceptionFlag) break;
		if (src.IsReal()) start = conj(start);
	}
	return roots1;
}

void poly::ScaleRoots(const Complex & scale)
{
	Complex x(scale);
	MakeUnique();
	for (int i = 1; i <= order(); i++)
	{
		(*this)[i] *= x;
		x *= scale;
	}
}

void poly::FromPoints(const Complex * pArguments,
					const Complex * pValues, int nCount)
{
#if 0
	poly p;
	for (int i = 0; i < nCount; i++)
	{
		polyRoots r;
		//        r.SetCount(0);
		for (int j = 0; j < nCount; j++)
		{
			if (j != i)
			{
				r += pArguments[j];
			}
		}
#if 0 //def _DEBUG
		poly(r, pValues[i] / coeff).Dump();
#endif
		poly p1(r);
		p += p1 * (pValues[i] / p1(pArguments[i]));
	}
	*this = p;
#ifdef _DEBUG
	Dump();
#endif
#elif 1
	poly * pp = new poly[nCount], *pp1, *pp2;
	polyRoots * pr = new polyRoots[nCount], *pr1, *pr2;
	if (NULL == pp || NULL == pr)
	{
		delete[] pp;
		delete [] pr;
		return;
	}
	for (int i = 0; i < nCount; i ++ /*, ++pp1*/)
	{
		polyRoots r;
		//        r.SetCount(0);
		for (int j = 0; j < nCount; j++)
		{
			if (j != i)
			{
				r += pArguments[j];
			}
		}
		pr[i] += pArguments[i];
		Complex coeff = r.eval(pArguments[i]);
		pp[i] = poly(0, pValues[i] / coeff);
	}

	for (i = nCount; i > 1; i = (i + 1) /2)
	{
		int j;
		for (j = 0, pp2 = pp, pp1 = pp,
			pr1 = pr, pr2 = pr; j < i; j += 2,
			++pp2, pp1 += 2, pr1 += 2, pr2++)
		{
			if (j != (i & ~1))
			{
				pp2[0] = pp1[0] * poly(pr1[1])
						+ pp1[1] * poly(pr1[0]);
				pr2[0] = pr1[0] + pr1[1];
			}
			else
			{
				pp2[0] = pp1[0];
				pr2[0] = pr1[0];
			}
		}
	}
	*this = pp[0];
	delete[] pp;
	delete[] pr;
#ifdef _DEBUG
	Dump();
#endif
#else
	poly * pp = new poly[nCount], *pp1=pp, *pp2;
	for (int i = 0; i < nCount; i ++, ++pp1)
	{
		polyRoots r;
		//        r.SetCount(0);
		for (int j = 0; j < nCount; j++)
		{
			if (j != i)
			{
				r += pArguments[j];
			}
		}
		Complex coeff = r.eval(pArguments[i]);
		pp[i] = poly(r, pValues[i] / coeff);
	}

	for (i = nCount; i > 1; i = (i + 1) /2)
	{
		int j;
		for (j = 0, pp2 = pp, pp1 = pp; j < i; j += 2, ++pp2, pp1 += 2)
		{
			if (j != (i & ~1))
				pp2[0] = pp1[0] + pp1[1];
			else
				pp2[0] = pp1[0];
		}
	}
	*this = pp[0];
	delete[] pp;
#ifdef _DEBUG
	Dump();
#endif
#endif
}

#ifdef _DEBUG
void poly::Dump(CDumpContext & dc)
{
	CString s;
	s.Format("poly: order = %d\n", iOrder);
	dc << s;
	for (int i = 0; i <= iOrder; i++)
	{
		Complex x = array()[i];
		s.Format("x[%d]=(%.8g, %.8g)\n", i, x.re, x.im);
		dc << s;
	}
}
#endif

lpoly::lpoly(int iInitOrder, lcomplex first, int MaxOrder)
{
	iMaxOrder = MaxOrder;
	cmArray = new lcomplexArray;
	iOrder = 0;
	isReal = 1;
	SetOrder(iInitOrder);
	(*this)[0]=first;
	if (imag(first) != ldouble(0)) isReal = 0;
}

lpoly::lpoly(const lcomplex *roots, int iInitOrder, lcomplex first, int MaxOrder)
{
	iMaxOrder = MaxOrder;
	cmArray = new lcomplexArray;
	iOrder = 0;
	isReal = 1;
	SetOrder(0);
	FromRoots(lpolyRoots(roots, iInitOrder), first);
}

lpoly::lpoly(const lpolyRoots &roots, lcomplex first, int MaxOrder)
{
	iMaxOrder = MaxOrder;
	cmArray = new lcomplexArray;
	iOrder = 0;
	isReal = 1;
	SetOrder(0);
	FromRoots(roots, first);
}

lpoly::lpoly(const lpoly & p)
{
	p.cmArray->IncRefCount();
	cmArray = p.cmArray;
	iOrder = p.iOrder;
	isReal = p.isReal;
	iMaxOrder = p.iMaxOrder;
}

lpoly::lpoly(const poly & p)
{
	cmArray = new lcomplexArray(*p.cmArray);
	iOrder = p.iOrder;
	isReal = p.isReal;
	iMaxOrder = p.iMaxOrder;
}

lpoly::~lpoly()
{
	SafeDelete(cmArray);
}

void __stdcall SafeDelete(lcomplexArray * a)
{
	if(a != NULL && --(a->iRefCount) == 0) delete a;
}

void lpoly::FromRoots(const lpolyRoots & roots, lcomplex first)
{
	int iInitOrder = roots.count();
	if (iInitOrder <= 0)
	{
		SetOrder(0);
		(*this)[0] = first;
		return;
	}
	if (imag(first) != 0.) isReal = 0;
	lpoly * pp = new lpoly[(iInitOrder + 1)/2], *pp1=pp, *pp2;
	for (int i = 0; i < iInitOrder; i += 2, ++pp1)
	{
		pp1->SetOrder(1);
		(*pp1)[0] = 1.;
		(*pp1)[1] = -roots[i];
		poly p1(1, 1.);
		if (i + 1 < iInitOrder)
		{
			p1 -= roots[i+1];
			*pp1 *= p1;
		}
	}

	for (i = (iInitOrder+1)/2; i > 1; i = (i + 1) /2)
	{
		int j;
		for (j = 0, pp2 = pp, pp1 = pp; j < i; j += 2, ++pp2, pp1 += 2)
		{
			if (j != (i & ~1))
				pp2[0] = pp1[0] * pp1[1];
			else
				pp2[0] = pp1[0];
		}
	}
	*this = pp[0] * first;
	delete[] pp;
}

lpoly & lpoly::operator +=(const lcomplex &a)
{
	MakeUnique();
	Allocate(0);
	array()[iOrder] += a;
	if(imag(a) != ldouble(0.)) isReal = 0;
	return *this;
}

lpoly & lpoly::operator -=(const lcomplex &a)
{
	MakeUnique();
	Allocate(0);
	array()[iOrder] -= a;
	if(imag(a) != ldouble(0.)) isReal = 0;
	return *this;
}

lpoly & lpoly::operator +=(LDOUBLE a)
{
	if (a != ldouble(0.))
	{
		MakeUnique();
		Allocate(0);
		array()[iOrder] += a;
	}
	return *this;
}

lpoly & lpoly::operator -=(LDOUBLE a)
{
	if (a != ldouble(0.))
	{
		MakeUnique();
		Allocate(0);
		array()[iOrder] -= a;
	}
	return *this;
}

lpoly & lpoly::operator +=(const lpoly &a)
{
	MakeUnique();
	int ord=order(), ord1=a.order();
	if(ord < ord1)
		*this >>= ord1 - ord;
	lcomplex * dst = array()+order()-ord1;
	const lcomplex *src = a.array();
	for (++ord1; ord1--; dst++, src++) *dst += *src;
	isReal &= a.isReal;
	return *this;
}

lpoly & lpoly::operator -= (const lpoly &a)
{
	MakeUnique();
	int ord=order(), ord1=a.order();
	if(ord < ord1)
		*this >>= ord1 - ord;
	lcomplex * dst = array()+order()-ord1;
	const lcomplex *src = a.array();
	for (++ord1; ord1--; dst++, src++) *dst -= *src;
	isReal &= a.isReal;
	return *this;
}

lpoly & lpoly::operator *= (const lcomplex &a)
{
	MakeUnique();
	lcomplex *dst = array();
	for(int ord = order()+1; ord--; dst++) *dst *= a;
	if(imag(a) != ldouble(0.)) isReal = 0;
	return *this;
}

lpoly & lpoly::operator /= (const lcomplex &a)
{
	MakeUnique();
	lcomplex *dst = array(), b= ldouble(1.)/a;
	for(int ord = order()+1; ord--; dst++) *dst *= b;
	if(imag(a) != ldouble(0.)) isReal = 0;
	return *this;
}

lpoly & lpoly::operator *= (LDOUBLE a)
{
	MakeUnique();
	lcomplex *dst = array();
	for(int ord = order()+1; ord--; dst++) *dst *= a;
	return *this;
}

lpoly & lpoly::operator /= (LDOUBLE a)
{
	MakeUnique();
	lcomplex *dst = array();
	for(int ord = order()+1; ord--; dst++) *dst /= a;
	return *this;
}

lpoly & lpoly::operator *= (const lpoly &a)
{
	MakeUnique();
	Allocate(order() + a.order());
	ASSERT( array() != a.array()); // make sure we don't override the array
	int ord = order();
	SetOrder(order() + a.order()); // order is changed
	lcomplex * dst = array() + ord, mult;
	// ord - dest polynom order (before expanding)
#ifndef USE_POLY_ASM
	int j;
	lcomplex * tmp1;
	const lcomplex *tmp2;
	for (ord++; ord; ord--, --dst)
	{
		mult= *dst;
		*dst = 0;
		for(j = a.order()+1, tmp1=dst, tmp2 = a.array(); j--;
			++tmp1, ++ tmp2)
			*tmp1 += *tmp2 * mult;
	}
	isReal &= a.isReal;
#else
	__PolyMultiplyL(array(), array(), a.array(), ord, a.order());
#endif
	return *this;
}
lpoly __stdcall operator *	(const lpoly & mult1, const lcomplex &mult2)
{
	lpoly p(mult1);
	p *= mult2;
	return p;
}
lpoly __stdcall operator *	(const lpoly & mult1, LDOUBLE mult2)
{
	lpoly p(mult1);
	p *= mult2;
	return p;
}
lpoly __stdcall operator *	(const lcomplex &mult2, const lpoly & mult1)
{
	lpoly p(mult1);
	p *= mult2;
	return p;
}
lpoly __stdcall operator *	(LDOUBLE mult2, const lpoly & mult1)
{
	lpoly p(mult1);
	p *= mult2;
	return p;
}
lpoly __stdcall operator *	(const lpoly & mult1, const lpoly &mult2)
{
	lpoly p(mult1);
	p *= mult2;
	return p;
}

lpoly __stdcall operator +	(const lpoly & mult1, const lcomplex &mult2)
{
	lpoly p(mult1);
	p += mult2;
	return p;
}
lpoly __stdcall operator +	(const lpoly & mult1, LDOUBLE mult2)
{
	lpoly p(mult1);
	p += mult2;
	return p;
}
lpoly __stdcall operator +	(const lcomplex &mult2, const lpoly & mult1)
{
	lpoly p(mult1);
	p += mult2;
	return p;
}

lpoly __stdcall operator +	(LDOUBLE mult2, const lpoly & mult1)
{
	lpoly p(mult1);
	p += mult2;
	return p;
}

lpoly __stdcall operator +	(const lpoly & mult1, const lpoly &mult2)
{
	lpoly p(mult1);
	p += mult2;
	return p;
}

lpoly __stdcall operator -	(const lpoly & mult1, const lcomplex &mult2)
{
	lpoly p(mult1);
	p -= mult2;
	return p;
}

lpoly __stdcall operator -	(const lpoly & mult1, LDOUBLE mult2)
{
	lpoly p(mult1);
	p -= mult2;
	return p;
}

lpoly __stdcall operator -	(const lcomplex &mult2, const lpoly & mult1)
{
	lpoly p(-mult1);
	p += mult2;
	return p;
}

lpoly __stdcall operator -	(LDOUBLE mult2, const lpoly & mult1)
{
	lpoly p(-mult1);
	p += mult2;
	return p;
}

lpoly __stdcall operator -	(const lpoly & mult1, const lpoly &mult2)
{
	lpoly p(mult1);
	p -= mult2;
	return p;
}

void __stdcall PolyDiv(lpoly *quot, lpoly *rem, const lpoly &numer, const lpoly &denom)
{
	lpoly quot1, rem1(numer);
	rem1.MakeUnique();
	const lcomplex * dptr = denom.array();
	lcomplex * rptr = rem1.array();
	int denord = denom.order(), numord = numer.order();
// skip first (higher order) zero terms
	while (*dptr == 0. && denord >= 0) dptr++, --denord;
	if (denord < 0) throw lpoly(); // all terms of denominator are zero
	int iCount = numord - denord + 1;	// number of steps
	if (iCount > 0) quot1.SetOrder(iCount-1);
#ifndef USE_POLY_ASM
	int i;
	for (int termNum = 0; termNum < iCount; ++termNum, ++rptr)
	{
		lcomplex coef, * rptr1=rptr;
		const lcomplex * dptr1 = dptr;
		coef = *rptr / *dptr;
		quot1[termNum] = coef;
		for (i = 0; i < denord; i++) *(++rptr1) -= *(++dptr1) * coef;
	}
	if (quot != NULL) *quot = quot1;
	if (rem != NULL)
	{
		if (numord >= denord) rem1 <<= numord - denord+1;
		*rem = rem1;
	}
#else
	if (iCount > 0) __PolyDivideL(quot1.array(), rptr+(numord - denord+1),
								rptr, dptr, numord, denord);
	if (quot != NULL) *quot = quot1;
	if (rem != NULL)
	{
		if (numord >= denord) rem1 <<= numord - denord+1;
		*rem = rem1;
	}
#endif
}

void lpoly::SetOrder(int ord)
{
	MakeUnique();
	Allocate(((ord+4) & ~3)-1);
	int i= iOrder+1;
	iOrder = ord;
	for(; i <= ord; i++) (*this)[i] = 0.;
}

// throw out high-order terms, considering them negligible
lpoly & lpoly::operator <<=(int i)
{
	MakeUnique();
	ASSERT(i >= 0);
	if(i)
	{
		for (int j=0, k=i; k <= order(); ++j, ++k) (*this)[j] = (*this) [k];
		iOrder -= i;
	}
	return *this;
}

// Add a number of zero high-order terms
lpoly & lpoly::operator >>=(int i)
{
	ASSERT(i >= 0);
	SetOrder(iOrder + i);
	for(int j=order(), k=j-i; k >= 0; --j, --k) (*this)[j] = (*this) [k];
	for(; j >= 0; j--) (*this)[j] = 0.;
	return *this;
}

lpoly lpoly::operator -() const
{
	lpoly p(*this);
	p.MakeUnique();
	for(int i=order()+1; i--;) p[i] = -p[i];
	return p;
}

lpoly __stdcall operator <<(const lpoly & p, int i)
{
	lpoly tmp(p);
	tmp <<= i;
	return tmp;
}

lpoly __stdcall operator >>(const lpoly & p, int i)
{
	lpoly tmp(p);
	tmp >>= i;
	return tmp;
}

lpoly & lpoly::operator /= (const lpoly & denom)
{
	PolyDiv(this, NULL, *this, denom);
	return *this;
}

lpoly & lpoly::operator %= (const lpoly & denom)
{
	PolyDiv(NULL, this, *this, denom);
	return *this;
}

lpoly __stdcall operator / (const lpoly & numer, const lpoly & denom)
{
	lpoly quot;
	PolyDiv(&quot, NULL, numer, denom);
	return quot;
}

lpoly __stdcall operator % (const lpoly & numer, const lpoly & denom)
{
	lpoly rem;
	PolyDiv(NULL, &rem, numer, denom);
	return rem;
}

lpoly & lpoly::operator = (const lpoly & src)
{
	src.cmArray->IncRefCount();
	SafeDelete(cmArray);
	cmArray = src.cmArray;
	iOrder = src.iOrder;
	return *this;
}

lcomplex lpoly::eval(lcomplex arg) const
{
	lcomplex res=0., temp;
	lcomplex powers[10];
	powers[0] = arg;
	int i,j=1;
	if (order() >= 1024)
		for(int i=0; i <= order(); i++) res = res*arg+(*this)[i];
	else
	{
		for (i = 1, j = 1; j <  order(); ++i, j= j+j)
			powers[i] = powers[i-1] * powers[i-1];
		for (i = 0, j=order(); j >= 0; i++, --j)
		{
			temp = (*this)[j];
			for (int k=1, l=0; k <= i; l++, k <<= 1)
				if (i & k) temp *= powers[l];
			res += temp;
		}
	}
	return res;
}

// produce derivative polynom
lpoly lpoly::deriv(void) const
{
	int ord=order();
	if (ord) ord--;
	lpoly dst(ord,lcomplex(0., 0.));
	LDOUBLE cx = 1.;
	for (int i = order(); i --; cx += 1.) dst[i] = (*this)[i] * cx;
	return dst;
}

// produce originate polynom
lpoly lpoly::orig(void) const
{
	lpoly dst(order()+1);
	LDOUBLE cx = 1.;
	for (int i = order()+1; i --; cx += 1.) dst[i] = (*this)[i] / cx;
	// dst[i] = 0.;
	return dst;
}

ostream & __stdcall operator << (ostream &os, const lpoly &p)
{
	os << p.order() << "\n";
	for (int i=0; i <= p.order(); i++)
		os << p[i] << "\n";
	return os;
}

ostream & __stdcall operator << (ostream &os, const poly &p)
{
	os << p.order() << "\n";
	for (int i=0; i <= p.order(); i++)
		os << p[i] << "\n";
	return os;
}

// find polynom roots.
lpolyRoots lpoly::roots(lcomplex start, int iIter) const
{
	static int lastRootCount;
	// lastRootCount keeps number of roots calculated
	// on previous lpoly::roots invocation (counting
	// nested calls off).
	//
//  В lastRootCount хранится число корней, полученное на
//  предыдущем вызове lpoly::roots (без учета полученных
//  во вложенных вызовах). Столько корней копируется в массив roots.
#ifdef _DEBUG
	FloatingExceptionFlag = 0;
#endif
	ASSERT(iIter > 2);
	lastRootCount = 0;
	lpoly src(*this);
	lpolyRoots roots1;
	int rootCounter = 0, rootCount;
	// discard zero high order terms
	while (src.order() > 0 && src[0] == 0.)	src <<= 1;
	rootCount = src.order();
	if (rootCount == 0) return 0;
// discard Z=0 roots if any
	while (src[src.order()] == 0.)
	{
		roots1 += ldouble(0.);
		src.SetOrder(src.order()-1);
	}

	lpoly deriv1;
	lpoly deriv2, deriv3;
	lpoly srcderiv1, srcderiv2, srcderiv3;
	srcderiv1 = src.deriv();
// check if the polynom has only prime roots
// find greatest common divisor of src and srcderiv1
	{
		_clearfp();
		lpoly r1(src), r2(srcderiv1),r3, r4;
		int j;
		for (int i=rootCount-1; i >0; i--)
		{
			PolyDiv(&r4, &r3, r1, r2);
// check if |r3| is small enough
			double norm_=0., norm1=0.;
			for (j = 0; j <=r3.order(); ++j)
				norm_ += sqrt(norm(r3[j]));
			for (j = 0; j <=r2.order(); ++j)
				norm1 += sqrt(norm(r2[j]));
			norm_ /= j;
			norm_ /= (double)sqrt(norm(r4[0]))*norm1;
			int fpstatus = _statusfp();
			if (fpstatus & MCW_EM)
			{
				_clearfp();
				if (fpstatus & (EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE))
				{
#if defined(_DEBUG)
					if (fpstatus & EM_ZERODIVIDE)
						afxDump << "Divide By Zero in reduce\n";
					if (fpstatus & EM_OVERFLOW)
						afxDump << "Floating Overflow in reduce\n";
					if (fpstatus & EM_UNDERFLOW)
						afxDump << "Floating Underflow in reduce\n";
#endif
					i =0; break;
				}
			}
//		afxDump << "Norm = " << norm_ << "\n";
			if (norm_ < 1e-5) break;
// swap polynoms
			r1 = r2;
			r2 = r3;
		}
		if ( i >  0)
		{
// найти корни r2 - это кратные корни
			int oldRootCount = rootCounter;
			lpolyRoots roots2 = r2.roots(start, iIter);
			int newRootCount = roots2.count();
#if defined(_DEBUG)
			afxDump << "Coupled roots: " << newRootCount << "\n";
#endif
			rootCounter += newRootCount + lastRootCount;
			for (i = 0; i< lastRootCount; ++i)
				roots1 += roots2[i];
			src	/= lpoly(&(roots1[oldRootCount]), newRootCount + lastRootCount);
			srcderiv1 = src.deriv();
			rootCounter += newRootCount + lastRootCount;
		}
	}
	lpoly tmp1, tmp2(src);
	srcderiv2 = srcderiv1.deriv();
	srcderiv3 = srcderiv2.deriv();
// tmp1 to keep product of all the roots and tmp2 to keep src/tmp1
	lpoly term(1);
// use 2-nd order gradiend descent method
	lcomplex delta, a, b, c, d, b2, ac;
	Complex ddelta, dstart, da, db, dc;
	int oldRootCount = rootCounter;
	for (; rootCounter < rootCount; rootCounter++, lastRootCount++ )
	{
// find first and second derivatives
		tmp2 = src / lpoly(roots1.array() + oldRootCount, rootCounter-oldRootCount);
		deriv1 = tmp2.deriv();
		deriv2 = deriv1.deriv();
		deriv3 = deriv2.deriv();
		for (int iters=iIter; iters--;)
		{
			if (iIter - iters <= 300)
			{
				c = tmp2.eval(start);
				b=deriv1.eval(start);
				a=deriv2.eval(start);
				d = deriv3.eval(start) ;
			}
			else
			{
				c = src.eval(start);
				b = srcderiv1.eval(start);
				a = srcderiv2.eval(start);
				d = srcderiv3.eval(start) ;
			}

//			if (iIter - iters <= 20 || iIter - iters >= 30)
			if (deriv2.order() <= 0)
			{
				if (iIter - iters >= 30)
				{
					c = b;
					b = a;
					a = srcderiv3.eval(start);
				}
				b2 = b*b;
				ac = ldouble(2.)*a*c;
// if ac is much less than b2 then use alternate formula
				if (norm(ac)*ldouble(16000000.) < norm(b2))
					delta = -c/b * (ldouble(1.)- ac/(b2*ldouble(4.)));
				else
				{
					d = sqrt(b2 - ac);
					lcomplex d1, d2;
					if (norm(d)*ldouble(1e11) < norm(b))
					{
						start -= b/a;
						break;
					}
					else
						if (real(b) * real(d) + imag(b) * imag(d) > 0.)
							d -= b;
						else
							d = -b - d;
					if (a == 0.)
					{
						start +=lcomplex(0.1, 0.1);
						break; // continue;
					}
					delta = d/a;
				}
			}
			else
// slow convergence, apply third order method
			{
// use Kardano method (G. & Th. Korn)
//				c = tmp2.eval(start);
//				b=deriv1.eval(start);
//				a=deriv2.eval(start) /ldouble(2.);
// If iterations are too many, change the point
				if ((iIter - iters) % 34 == 33)
					start *= lcomplex(0.707, 0.707);
				if ((iIter - iters) % 34 >= 20)
				{
					c = b;
					b = a;
					a = d;
					d = deriv3.deriv().eval(start);
				}
				d = ldouble(6.) / d;
				c *= d;
				b *= d;
				a *= d / ldouble(2.);
				lcomplex p, q, p1, q1, Q, A, B, y1, y2, y3;
				lcomplex AplusB, AminusB;
				Complex cy1, cy2, cy3;
				p = b - a * a / ldouble(3.);
				q = c / ldouble(2.) - a * b / ldouble(6.) +
					a * a * a / ldouble(27.);
				p1 = p * p * p / ldouble(27.);
				q1 = q * q;
//				if (norm(q1) * ldouble(16000000.) < norm(p1))
//					{
//					AplusB = q / (ldouble(-1.5) * p);
//					AminusB = sqrt( p / ldouble(12.));
//					}
//				else
				{
//					if (norm(p1) * ldouble(16000000.) < norm(q1))
//						{
//						B= - pow(q*ldouble(2.), ldouble(1/3.));
//						A= -p / (B * ldouble(3.));
//						}
//						else
					{
						Q = sqrt(p1 + q1);
						if (real(Q) * real(q) + imag(Q) * imag(q) > 0.)
							A = pow(-q - Q, ldouble(1/3.));
						else
							A = pow(-q + Q, ldouble(1/3.));
						B = -p / (A * ldouble(3.));
					}
					AplusB = A + B;
					AminusB = A - B;
				}
				a /= ldouble(3.);
				y1 = AplusB - a;
				AplusB *= ldouble(-0.5);
				AminusB *= sqrt(ldouble(3.)) * lcomplex(0., 0.5);
				y2 = AplusB + AminusB - a;
				y3 = AplusB - AminusB - a;
				delta = y1;
				if (norm(delta) > norm(y2)) delta = y2;
				if (norm(delta) > norm(y3)) delta = y3;
			}
#if defined(_DEBUG)
			PolyIterCount++;
#endif
// check if we should stop iterations
			if (!_finite(real(start)) || !_finite(imag(start)))
			{
#if defined(_DEBUG)
				afxDump << "Bad \"start\" value, iter = "
					<< iIter - iters << "\n";
#endif
				break;
			}
			if (!_finite(real(delta)) || !_finite(imag(delta)))
			{
#if defined(_DEBUG)
				afxDump << "Bad delta value, iter = "
					<< iIter - iters << "\n";
#endif
				break;
			}
			dstart = start += delta;
			if (double(norm(delta))*1e30 < double(norm(start))) break;
		}
//		if (iters < 0)
//			ASSERT(iters >= 0);	// iterations converged
// refine root using full polynom
#if 1

//		if(iIter - iters > 20)
		if (srcderiv2.order() > 0)
		{
			c = src.eval(start);
			b=srcderiv1.eval(start);
			a=srcderiv2.eval(start);
			d = ldouble(6.)/ srcderiv3.eval(start) ;
			c *= d;
			b *= d;
			a *= d / ldouble(2.);
			lcomplex p, q, p1, q1, Q, A, B, y1, y2, y3;
			lcomplex AplusB, AminusB;
			Complex cy1, cy2, cy3;
			p = b - a * a / ldouble(3.);
			q = c / ldouble(2.) - a * b / ldouble(6.) +
				a * a * a / ldouble(27.);
			p1 = p * p * p / ldouble(27.);
			q1 = q * q;
//				if (norm(q1) * ldouble(16000000.) < norm(p1))
//					{
//					AplusB = q / (ldouble(-1.5) * p);
//					AminusB = sqrt( p / ldouble(12.));
//					}
//				else
//					if (norm(p1) * ldouble(16000000.) < norm(q1))
//						{
//						B= - pow(q*ldouble(2.), ldouble(1/3.));
//						A= -p / (B * ldouble(3.));
//						}
//						else
			Q = sqrt(p1 + q1);
			if (real(Q) * real(q) + imag(Q) * imag(q) > 0.)
				A = pow(-q - Q, ldouble(1/3.));
			else
				A = pow(-q + Q, ldouble(1/3.));
			B = -p / (A * ldouble(3.));
			AplusB = A + B;
			AminusB = A - B;
			a /= ldouble(3.);
			y1 = AplusB - a;
			AplusB *= ldouble(-0.5);
			AminusB *= sqrt(ldouble(3.)) * lcomplex(0., 0.5);
			y2 = AplusB + AminusB - a;
			y3 = AplusB - AminusB - a;
			d = y1;
			if (norm(d) > norm(y2)) d = y2;
			if (norm(d) > norm(y3)) d = y3;
			if (norm(d) < norm(delta)) delta = d;
			else delta = lcomplex(0.,0.);

			if (0)
			{
				b2 = b*b;
				ac = ldouble(2.)*a*c;
// if ac is much less than b2 then use alternate formula
				if (norm(ac)*ldouble(1600000.) < norm(b2))
					delta = -c/b * (ldouble(1.)- ac/(b2*ldouble(4.)));
				else
				{
					d = sqrt(b2 - ac);
					lcomplex d1, d2;
					if (norm(d)*ldouble(1e10) < norm(b))
						d = -b;
					else
						if (real(b) * real(d) + imag(b) * imag(d) > 0.)
							d -= b;
						else
							d = -b - d;
					if (a == 0.)
						delta = 0.;
					else delta = d/a;
				}
			}
#endif
			if (!_finite(real(delta)) || !_finite(imag(delta)))
			{
#if defined(_DEBUG)
				afxDump << "Bad final delta\n"
					<< "start=" << start <<"\n"
					<< "b2=" << b2 << ", ac=" << ac << "\n"
					<< "a=" << a
					<< ", b=" << b
					<< "\nc=" << c
					<< ", d=" << d <<"\n";
#endif
				break;
			}
			start += delta;
		}
		if (!_finite(real(start)) || !_finite(imag(start)))
		{
#if defined(_DEBUG)
			afxDump << "Bad final root value\n";
#endif
			break;
		}
		int fpstatus = _statusfp();
		if (fpstatus & MCW_EM)
		{
			_clearfp();
			if (fpstatus & (EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE))
			{
#if defined(_DEBUG)
				if (fpstatus & EM_ZERODIVIDE)
					afxDump << "Divide By Zero\n";
				if (fpstatus & EM_OVERFLOW)
					afxDump << "Floating Overflow\n";
				if (fpstatus & EM_UNDERFLOW)
					afxDump << "Floating Underflow\n";
#endif
				break;
			}
		}
		roots1 += start;
//		if (FloatingExceptionFlag) break;
		if (src.IsReal()) start = conj(start);
	}
	return roots1;
}

polyRoots::polyRoots(int MaxCount):
	cmArray (new complexArray),
	iCount (0),
	iMaxCount (MaxCount)
{
}

polyRoots::polyRoots(const polyRoots & p)
{
	p.cmArray->IncRefCount();
	cmArray = p.cmArray;
	iCount = p.iCount;
	iMaxCount = p.iMaxCount;
}

polyRoots::polyRoots(const lpolyRoots & p):
	cmArray (new complexArray(*p.cmArray)),
	iCount (p.iCount),
	iMaxCount (p.iMaxCount)
{
}

polyRoots::polyRoots(const Complex *roots, int Count, int MaxCount)
	:cmArray(new complexArray),
	iCount (0),
	iMaxCount (MaxCount)
{
	Allocate(Count);
	for (int i = 0; i < Count; i++)
		*this += roots[i];
}

polyRoots::~polyRoots()
{
	SafeDelete(cmArray);
}

polyRoots & polyRoots::operator = (const polyRoots & src)
{
	src.cmArray->IncRefCount();
	SafeDelete(cmArray);
	cmArray = src.cmArray;
	iCount = src.iCount;
	return *this;
}

polyRoots & polyRoots::operator +=(const Complex &a)
{
	MakeUnique();
	SetCount(iCount+1);
	array()[iCount-1] = a;
	return *this;
}

polyRoots & polyRoots::operator +=(double a)
{
	MakeUnique();
	SetCount(iCount+1);
	array()[iCount-1] = a;
	return *this;
}

polyRoots & polyRoots::operator +=(const polyRoots & a)
{
	MakeUnique();
	int nOldCount = iCount;
	SetCount(iCount+a.count());
	for (int i = 0; i < a.count(); i++)
	{
		array()[i + nOldCount] = a[i];
	}
	return *this;
}

polyRoots __stdcall operator + (const polyRoots & arg1, const Complex &arg2)
{
	polyRoots p(arg1);
	p += arg2;
	return p;
}
polyRoots __stdcall operator + (const polyRoots & arg1, double arg2)
{
	polyRoots p(arg1);
	p += arg2;
	return p;
}
polyRoots __stdcall operator + (const polyRoots & arg1, const polyRoots &  arg2)
{
	polyRoots p(arg1);
	p += arg2;
	return p;
}

polyRoots __stdcall operator + (const Complex &arg2, const polyRoots & arg1)
{
	polyRoots p(arg1);
	p += arg2;
	return p;
}
polyRoots __stdcall operator + (double arg2, const polyRoots & arg1)
{
	polyRoots p(arg1);
	p += arg2;
	return p;
}

void polyRoots::SetCount(int ord)
{
	MakeUnique();
	Allocate((ord+4) & ~3);
	int i= iCount;
	iCount = ord;
	for(; i < ord; i++) (*this)[i] = 0.;
}

Complex polyRoots::eval(Complex arg) const
{
	if (0 == count()) return Complex(1.);
#if 0
	Complex tmp(1.0,0.);
	const Complex *ptr = array();
	for(int i = count(); (i--)>0; ptr++)
		tmp *= arg - *ptr;
	return tmp;
#else
	Complex * x = new Complex[count()];
	if (NULL == x) return Complex(0.);
	for (int i = 0; i < count(); i++)
	{
		x[i] = arg - array()[i];
	}
	Complex *p1, *p2, tmp;
	for (i = count(); i > 1; i = (i + 1) /2)
	{
		int j;
		for (j = 0, p2 = x, p1 = x; j < i; j += 2,
			++p2, p1 += 2)
		{
			if (j != (i & ~1))
			{
				p2[0] = p1[0]* p1[1];
			}
			else
			{
				p2[0] = p1[0];
			}
		}
	}
	tmp = x[0];
	delete x;
	return tmp;
#endif
}

#ifdef _DEBUG
void polyRoots::Dump(CDumpContext & dc)
{
	CString s;
	s.Format("polyRoots: count = %d\n", iCount);
	dc << s;
	for (int i = 0; i < iCount; i++)
	{
		Complex x = array()[i];
		s.Format("x[%d]=(%.8g, %.8g)\n", i, x.re, x.im);
		dc << s;
	}
}
#endif

lpolyRoots::lpolyRoots(int MaxCount):
	iMaxCount (MaxCount),
	cmArray (new lcomplexArray),
	iCount (0)
{
}

lpolyRoots::lpolyRoots(const lpolyRoots & p)
{
	p.cmArray->IncRefCount();
	cmArray = p.cmArray;
	iCount = p.iCount;
	iMaxCount = p.iMaxCount;
}

lpolyRoots::lpolyRoots(const polyRoots & p):
	cmArray (new lcomplexArray(*p.cmArray)),
	iCount (p.iCount),
	iMaxCount (p.iMaxCount)
{
}

lpolyRoots::lpolyRoots(const lcomplex *roots, int Count, int MaxCount)
	:cmArray(new lcomplexArray),
	iCount (0),
	iMaxCount (MaxCount)
{
	Allocate(Count);
	for (int i = 0; i < Count; i++)
		*this += roots[i];
}

lpolyRoots::~lpolyRoots()
{
	SafeDelete(cmArray);
}

lpolyRoots & lpolyRoots::operator = (const lpolyRoots & src)
{
	src.cmArray->IncRefCount();
	SafeDelete(cmArray);
	cmArray = src.cmArray;
	iCount = src.iCount;
	return *this;
}

lpolyRoots & lpolyRoots::operator +=(const lcomplex &a)
{
	MakeUnique();
	SetCount(iCount+1);
	array()[iCount-1] = a;
	return *this;
}

lpolyRoots & lpolyRoots::operator +=(ldouble a)
{
	MakeUnique();
	SetCount(iCount+1);
	array()[iCount-1] = a;
	return *this;
}

lpolyRoots __stdcall operator + (const lpolyRoots & arg1, const lcomplex &arg2)
{
	lpolyRoots p(arg1);
	p += arg2;
	return p;
}
lpolyRoots __stdcall operator + (const lpolyRoots & arg1, ldouble arg2)
{
	lpolyRoots p(arg1);
	p += arg2;
	return p;
}
lpolyRoots __stdcall operator + (const lcomplex &arg2, const lpolyRoots & arg1)
{
	lpolyRoots p(arg1);
	p += arg2;
	return p;
}
lpolyRoots __stdcall operator + (ldouble arg2, const lpolyRoots & arg1)
{
	lpolyRoots p(arg1);
	p += arg2;
	return p;
}

void lpolyRoots::SetCount(int ord)
{
	MakeUnique();
	Allocate((ord+4) & ~3);
	int i= iCount;
	iCount = ord;
	for(; i < ord; i++) (*this)[i] = 0.;
}

lcomplex lpolyRoots::eval(lcomplex arg) const
{
	lcomplex tmp(1.0,0.);
	const lcomplex *ptr = array();
	for(int i = count(); (i--)>0; ptr++)
		tmp *= arg - *ptr;
	return tmp;
}

#ifdef _DEBUG
void lpolyRoots::Dump(CDumpContext & dc)
{
	CString s;
	s.Format("lpolyRoots: count = %d\n", iCount);
	dc << s;
	for (int i = 0; i < iCount; i++)
	{
		Complex x = array()[i];
		s.Format("x[%d]=(%.8g, %.8g)\n", i, x.re, x.im);
		dc << s;
	}
}
#endif

void lpoly::ScaleRoots(const lcomplex & scale)
{
	lcomplex x(scale);
	MakeUnique();
	for (int i = 1; i <= order(); i++)
	{
		(*this)[i] *= x;
		x *= scale;
	}
}

void lpoly::FromPoints(const lcomplex * pArguments,
						const lcomplex * pValues, int nCount)
{
#if 0
	lpoly p;
	for (int i = 0; i < nCount; i++)
	{
		lpolyRoots r;
		//        r.SetCount(0);
		for (int j = 0; j < nCount; j++)
		{
			if (j != i)
			{
				r += pArguments[j];
			}
		}
#if 0 //def _DEBUG
		lpoly(r, pValues[i] / coeff).Dump();
#endif
		lpoly p1(r);
		p += p1 * (pValues[i] / p1(pArguments[i]));
	}
	*this = p;
#ifdef _DEBUG
	Dump();
#endif
#else
	lpoly * pp = new lpoly[nCount], *pp1=pp, *pp2;
	for (int i = 0; i < nCount; i ++, ++pp1)
	{
		lpolyRoots r;
		//        r.SetCount(0);
		for (int j = 0; j < nCount; j++)
		{
			if (j != i)
			{
				r += pArguments[j];
			}
		}
#if 1
		lcomplex coeff = r.eval(pArguments[i]);
		pp[i] = lpoly(r, pValues[i] / coeff);
#else
		lpoly p1(r);
		pp[i] = p1 * (pValues[i] / p1(pArguments[i]));
#endif
	}

	for (i = nCount; i > 1; i = (i + 1) /2)
	{
		int j;
		for (j = 0, pp2 = pp, pp1 = pp; j < i; j += 2, ++pp2, pp1 += 2)
		{
			if (j != (i & ~1))
				pp2[0] = pp1[0] + pp1[1];
			else
				pp2[0] = pp1[0];
		}
	}
	*this = pp[0];
	delete[] pp;
#ifdef _DEBUG
	Dump();
#endif
#endif
}

#ifdef _DEBUG
void lpoly::Dump(CDumpContext & dc)
{
	CString s;
	s.Format("lpoly: order = %d\n", iOrder);
	dc << s;
	for (int i = 0; i <= iOrder; i++)
	{
		Complex x = array()[i];
		s.Format("x[%d]=(%.8g, %.8g)\n", i, x.re, x.im);
		dc << s;
	}
}
#endif

