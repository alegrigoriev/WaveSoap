#if !defined(_INC_POLYMATH)
 #define _INC_POLYMATH

#if !defined(_INC_ASSERT)
 #include "assert.h"
#endif
#if defined(_WINDOWS) || defined(WIN32)
#if !defined(__AFX_H__)
 #include "afx.h"
#endif
#else
#if defined(_DEBUG)
 #define ASSERT(a)
#else
 #define ASSERT(a) assert(a)
#endif
#endif
// PolyMath.h
#if !defined(__COMPLEX_H)
 #include "complex.h"
#endif
#if !defined(__LCOMPLEX_H)
 #include "lcomplex.h"
#endif
class poly;
class lpoly;
/*
complexArray class is introduced to reduce array copy overhead
When 'poly'	class is copied, we just copy its complexArray pointer
and increment its reference count to 1. When we need to modify
the array we allocate new copy only if the reference count is
greater than 1.
*/
#pragma pack(push, 2)

// я определяю double и long double версии полиномов

class complexArray
{
	friend class poly;
	friend class lpoly;
	friend class polyRoots;
	friend class lpolyRoots;
public:
	complexArray()
	{
		iRefCount=1;
		pArray = NULL;
		iAllocatedSize = 0;
	}
	friend class lcomplexArray;
	complexArray(const complexArray&);
	complexArray(const lcomplexArray&);
	operator Complex*() { return pArray; }
	operator const Complex*() const { return pArray; }
	friend void __stdcall SafeDelete(complexArray *);
	void Allocate(int);
	int AllocatedSize() const { return iAllocatedSize; }
	void IncRefCount() { ++iRefCount; }
	void DecRefCount() { --iRefCount; }

protected:
	~complexArray();
	complexArray * MakeUnique();
	int iRefCount;
	Complex *pArray;
	int iAllocatedSize;
};

class poly
{
	friend class lpoly;
public:
	poly(const lpoly&);
	poly(const poly&);
	poly(int iInitOrder=0, Complex first=0., int iMaxOrder=256);
	poly(const Complex *roots, int Order=0, Complex first=1., int iMaxOrder=256);
	poly(const polyRoots & roots, Complex first=1., int iMaxOrder = 256);
	// Binary Operator Functions
	friend poly __stdcall operator+(const poly &, const poly &);
	friend poly __stdcall operator+(double, const poly &);
	friend poly __stdcall operator+(const poly &, double);
	friend poly __stdcall operator+(const poly &, const Complex&);
	friend poly __stdcall operator+(const Complex&, const poly &);
	friend poly __stdcall operator-(const poly &, const poly &);
	friend poly __stdcall operator-(double, const poly &);
	friend poly __stdcall operator-(const poly &, double);
	friend poly __stdcall operator-(const poly &, const Complex&);
	friend poly __stdcall operator-(const Complex&, const poly &);
	friend poly __stdcall operator*(const poly &, const poly &);
	friend poly __stdcall operator*(double, const poly &);
	friend poly __stdcall operator*(const poly &, double);
	friend poly __stdcall operator*(const poly &, const Complex&);
	friend poly __stdcall operator*(const Complex&, const poly &);
	friend poly __stdcall operator/(const poly &, const poly &);
	friend poly __stdcall operator%(const poly &, const poly &);
	friend poly __stdcall operator/(const poly &, double);
	friend poly __stdcall operator/(const poly &, const Complex&);
	friend poly __stdcall operator<<(const poly &, int);
	friend poly __stdcall operator>>(const poly &, int);
	friend int __stdcall operator==(const poly &, const poly &);
	friend int __stdcall operator!=(const poly &, const poly &);
	friend void __stdcall PolyDiv(poly *quot, poly *rem, const poly &numer, const poly &denom);
	Complex eval(Complex arg) const;
	Complex operator ()(Complex x) const { return eval(x); }
	poly deriv(void) const;
	poly orig(void) const;
	void ScaleRoots(const Complex & scale);

	polyRoots roots(Complex start=0., int iIter=100) const;
	void FromRoots(const polyRoots & roots, Complex first=1.);
	void FromPoints(const Complex * pArguments,
					const Complex * pValues, int nCount);
	friend void __stdcall SafeDelete(complexArray *);
	//	friend
	poly & operator+=(const poly &);
	poly & operator+=(const Complex &);
	poly & operator+=(double);
	poly & operator-=(const poly &);
	poly & operator-=(const Complex &);
	poly & operator-=(double);
	poly & operator*=(const poly &);
	poly & operator*=(Complex );
	poly & operator*=(double);
	poly & operator/=(const poly &);
	poly & operator/=(const Complex &);
	poly & operator/=(double);
	poly & operator%=(const poly &);
	poly & operator<<=(int);
	poly & operator>>=(int);
	poly & operator = (const poly &);
	Complex & operator[](int i)
	{
		ASSERT (i >= 0);
		ASSERT (i <= iOrder);
		return cmArray->pArray[i]; }

	const Complex & operator[](int i) const
	{
		ASSERT (i >= 0);
		ASSERT (i <= iOrder);
		return cmArray->pArray[i]; }

	poly operator+() const;
	poly operator-() const;
	int order() const {return iOrder; }
	int IsReal() const {return isReal; }
	void SetReal(BOOL set) { isReal = set; }
	Complex *array() {return cmArray->pArray; }
	const Complex *array() const {return (const Complex*) cmArray->pArray; }
	void Allocate(int ord)
	{
		cmArray->Allocate(ord+1);
	}
	void SetOrder(int);
	~poly();
	void MakeUnique() { cmArray = cmArray->MakeUnique(); }
	BOOL IsZero() const;
	void Normalize();

#ifdef _DEBUG
	void Dump(CDumpContext & = afxDump);
#endif

protected:
	complexArray *cmArray;
	int iOrder; // polynom order (# of coeffs-1)
	int iMaxOrder;  // maximum allowed array size
	int isReal; // all coefficients are real
};

class lcomplexArray
{
	friend class lpoly;
	friend class poly;
	friend class polyRoots;
	friend class lpolyRoots;
	friend class complexArray;
public:
	lcomplexArray()
	{
		iRefCount=1;
		pArray = NULL;
		iAllocatedSize = 0;
	}
	lcomplexArray(const lcomplexArray&);
	lcomplexArray(const complexArray&);
	operator lcomplex*() { return pArray; }
	operator const lcomplex*() const { return pArray; }
	friend void __stdcall SafeDelete(lcomplexArray *);
	void Allocate(int);
	int AllocatedSize() const { return iAllocatedSize; }
	void IncRefCount() { ++iRefCount; }
	void DecRefCount() { --iRefCount; }

protected:
	~lcomplexArray();
	lcomplexArray * MakeUnique();
	int iRefCount;
	lcomplex *pArray;
	int iAllocatedSize;
};

class lpoly
{
	friend class poly;
public:
	lpoly(const lpoly&);
	lpoly(const poly&);
	lpoly(int iInitOrder=0, lcomplex first=lcomplex(0.,0.), int iMaxOrder=256);
	lpoly(const lcomplex *roots, int Order=0, lcomplex first=lcomplex(1.,0.), int iMaxOrder=256);
	lpoly(const lpolyRoots & roots, lcomplex first=lcomplex(1.), int iMaxOrder = 256);
	// Binary Operator Functions
	friend lpoly __stdcall operator+(const lpoly &, const lpoly &);
	friend lpoly __stdcall operator+(LDOUBLE, const lpoly &);
	friend lpoly __stdcall operator+(const lpoly &, LDOUBLE);
	friend lpoly __stdcall operator+(const lpoly &, const lcomplex&);
	friend lpoly __stdcall operator+(const lcomplex&, const lpoly &);
	friend lpoly __stdcall operator-(const lpoly &, const lpoly &);
	friend lpoly __stdcall operator-(LDOUBLE, const lpoly &);
	friend lpoly __stdcall operator-(const lpoly &, LDOUBLE);
	friend lpoly __stdcall operator-(const lpoly &, const lcomplex&);
	friend lpoly __stdcall operator-(const lcomplex&, const lpoly &);
	friend lpoly __stdcall operator*(const lpoly &, const lpoly &);
	friend lpoly __stdcall operator*(LDOUBLE, const lpoly &);
	friend lpoly __stdcall operator*(const lpoly &, LDOUBLE);
	friend lpoly __stdcall operator*(const lpoly &, const lcomplex&);
	friend lpoly __stdcall operator*(const lcomplex&, const lpoly &);
	friend lpoly __stdcall operator/(const lpoly &, const lpoly &);
	friend lpoly __stdcall operator%(const lpoly &, const lpoly &);
	friend lpoly __stdcall operator/(const lpoly &, ldouble);
	friend lpoly __stdcall operator/(const lpoly &, const lcomplex&);
	friend lpoly __stdcall operator<<(const lpoly &, int);
	friend lpoly __stdcall operator>>(const lpoly &, int);
	friend int __stdcall operator==(const lpoly &, const lpoly &);
	friend int __stdcall operator!=(const lpoly &, const lpoly &);
	friend void __stdcall PolyDiv(lpoly *quot, lpoly *rem, const lpoly &numer, const lpoly &denom);
	lcomplex eval(lcomplex arg) const;
	lcomplex operator ()(lcomplex x) const { return eval(x); }
	lpoly deriv(void) const;
	lpoly orig(void) const;
	void ScaleRoots(const lcomplex & scale);

	lpolyRoots roots(lcomplex start=lcomplex(0.,0.), int iIter=100) const;
	void FromRoots(const lpolyRoots & roots, lcomplex first=lcomplex(1.));
	void FromPoints(const lcomplex * pArguments,
					const lcomplex * pValues, int nCount);
	friend void __stdcall SafeDelete(lcomplexArray *);
	//	friend
	lpoly & operator+=(const lpoly &);
	lpoly & operator+=(const lcomplex &);
	lpoly & operator+=(LDOUBLE);
	lpoly & operator-=(const lpoly &);
	lpoly & operator-=(const lcomplex &);
	lpoly & operator-=(LDOUBLE);
	lpoly & operator*=(const lpoly &);
	lpoly & operator*=(const lcomplex &);
	lpoly & operator*=(LDOUBLE);
	lpoly & operator/=(const lpoly &);
	lpoly & operator/=(const lcomplex &);
	lpoly & operator/=(LDOUBLE);
	lpoly & operator%=(const lpoly &);
	lpoly & operator<<=(int);
	lpoly & operator>>=(int);
	lpoly & operator = (const lpoly &);
	lcomplex & operator[](int i)
	{
		ASSERT (i >= 0);
		ASSERT (i <= iOrder);
		return cmArray->pArray[i]; }

	const lcomplex & operator[](int i) const
	{
		ASSERT (i >= 0);
		ASSERT (i <= iOrder);
		return cmArray->pArray[i]; }

	lpoly operator+() const;
	lpoly operator-() const;
	int order() const {return iOrder; }
	int IsReal() const {return isReal; }
	void SetReal(BOOL set) { isReal = set; }
	lcomplex *array() {return cmArray->pArray; }
	const lcomplex *array() const {return (const lcomplex*) cmArray->pArray; }
	void Allocate(int ord)
	{
		cmArray->Allocate(ord+1);
	}
	void SetOrder(int);
	~lpoly();
	void MakeUnique() { cmArray = cmArray->MakeUnique(); }

#ifdef _DEBUG
	void Dump(CDumpContext & = afxDump);
#endif

protected:
	lcomplexArray *cmArray;
	int iOrder; // polynom order (# of coeffs-1)
	int iMaxOrder;  // maximum allowed array size
	int isReal; // all coefficients are real
};

class polyRoots
{
	friend class poly;
	friend class lpolyRoots;
public:
	polyRoots(const lpolyRoots&);
	polyRoots(const polyRoots&);
	polyRoots(int iMaxOrder=256);
	polyRoots(const Complex *roots, int Count=0, int iMaxOrder=256);
	~polyRoots();
	// Binary Operator Functions
	friend polyRoots __stdcall operator+(const polyRoots &, const polyRoots &);
	friend polyRoots __stdcall operator+(double, const polyRoots &);
	friend polyRoots __stdcall operator+(const polyRoots &, double);
	friend polyRoots __stdcall operator+(const polyRoots &, const Complex&);
	friend polyRoots __stdcall operator+(const Complex&, const polyRoots &);
	friend int __stdcall operator==(const polyRoots &, const polyRoots &);
	friend int __stdcall operator!=(const polyRoots &, const polyRoots &);
	Complex eval(Complex arg) const;
	friend void __stdcall SafeDelete(complexArray *);

	polyRoots & operator+=(const polyRoots &);
	polyRoots & operator+=(const Complex &);
	polyRoots & operator+=(double);
	polyRoots & operator = (const polyRoots &);

	Complex & operator[](int i)
	{
		ASSERT (i >= 0);
		ASSERT (i < iCount);
		return cmArray->pArray[i]; }

	const Complex & operator[](int i) const
	{
		ASSERT (i >= 0);
		ASSERT (i < iCount);
		return cmArray->pArray[i]; }

	int count() const {return iCount; }
	Complex *array() {return cmArray->pArray; }
	const Complex *array() const {return (const Complex*) cmArray->pArray; }
	void Allocate(int ord)
	{
		cmArray->Allocate(ord);
	}
	void SetCount(int);
	void MakeUnique() { cmArray = cmArray->MakeUnique(); }

#ifdef _DEBUG
	void Dump(CDumpContext & = afxDump);
#endif

protected:
	complexArray *cmArray;
	int iCount; // polynom order (# of coeffs-1)
	int iMaxCount;  // maximum allowed array size
};

class lpolyRoots
{
	friend class lpoly;
	friend class polyRoots;
public:
	lpolyRoots(const lpolyRoots&);
	lpolyRoots(const polyRoots&);
	lpolyRoots(int iMaxOrder=256);
	lpolyRoots(const lcomplex *roots, int Order=0, int iMaxOrder=256);
	// Binary Operator Functions
	friend lpolyRoots __stdcall operator+(const lpolyRoots &, const lpolyRoots &);
	friend lpolyRoots __stdcall operator+(ldouble, const lpolyRoots &);
	friend lpolyRoots __stdcall operator+(const lpolyRoots &, ldouble);
	friend lpolyRoots __stdcall operator+(const lpolyRoots &, const lcomplex&);
	friend lpolyRoots __stdcall operator+(const lcomplex&, const lpolyRoots &);
	friend int __stdcall operator==(const lpolyRoots &, const lpolyRoots &);
	friend int __stdcall operator!=(const lpolyRoots &, const lpolyRoots &);
	lcomplex eval(lcomplex arg) const;
	friend void __stdcall SafeDelete(lcomplexArray *);

	lpolyRoots & operator+=(const lpolyRoots &);
	lpolyRoots & operator+=(const lcomplex &);
	lpolyRoots & operator+=(ldouble);
	lpolyRoots & operator = (const lpolyRoots &);

	lcomplex & operator[](int i)
	{
		ASSERT (i >= 0);
		ASSERT (i < iCount);
		return cmArray->pArray[i]; }

	const lcomplex & operator[](int i) const
	{
		ASSERT (i >= 0);
		ASSERT (i < iCount);
		return cmArray->pArray[i]; }

	int count() const {return iCount; }
	lcomplex *array() {return cmArray->pArray; }
	const lcomplex *array() const {return (const lcomplex*) cmArray->pArray; }
	void Allocate(int ord)
	{
		cmArray->Allocate(ord);
	}
	void SetCount(int);
	~lpolyRoots();
	void MakeUnique() { cmArray = cmArray->MakeUnique(); }

#ifdef _DEBUG
	void Dump(CDumpContext & = afxDump);
#endif

protected:
	lcomplexArray *cmArray;
	int iCount; // polynom order (# of coeffs-1)
	int iMaxCount;  // maximum allowed array size
};

#pragma pack(pop)
ostream & __stdcall operator<<(ostream &, const lpoly &);
istream & __stdcall operator>>(istream &, lpoly &);
ostream & __stdcall operator<<(ostream &, const poly &);
istream & __stdcall operator>>(istream &, poly &);

int __stdcall PolyRoots(Complex * roots, const poly & src,
						Complex cmStart = Complex(0., 0.), int iIter = 100);

extern "C" void __PolyMultiply(Complex* dst, const Complex *src1,
								const Complex *src2, int order1, int order2);

extern "C" void __PolyDivide(Complex* quot, Complex* rem, const Complex *numer,
							const Complex *denom, int order_n, int order_d);

int __stdcall PolyRoots(lcomplex * roots, const lpoly & src,
						lcomplex cmStart = lcomplex(0., 0.), int iIter = 100);

extern "C" void __PolyMultiplyL(lcomplex* dst, const lcomplex *src1,
								const lcomplex *src2, int order1, int order2);

extern "C" void __PolyDivideL(lcomplex* quot, lcomplex* rem, const lcomplex *numer,
							const lcomplex *denom, int order_n, int order_d);

#if defined(_DEBUG)
extern int PolyIterCount;
extern int volatile FloatingExceptionFlag;
#endif
#endif // _INC_POLYMATH
