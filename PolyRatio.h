// PolyRatioMath
#if !defined(_INC_POLYRATIO)
 #define _INC_POLYRATIO

#if !defined(_INC_POLYMATH)
 #include "PolyMath.h"
#endif

#include <afxtempl.h>

class polyRatio;
class lpolyRatio;

#pragma pack(push,2)

class polyRatio
{
	friend class lpolyRatio;
public:
	CArray<polyRatio *, polyRatio *> *
		Decompose(int nCellsOrder, polyRoots * DenomRoots,
				int nFirstRatioOrder = 0);
	polyRatio(const lpolyRatio&);
	polyRatio(const polyRatio&);
	polyRatio(const poly& numer,const poly& denom = poly((int)0, 1.));
	polyRatio(Complex first=0., int iMaxOrder=200);
	polyRatio(Complex *zeros, int nOrder=0,
			Complex *poles=NULL, int dOrder=0, Complex first=1.);
	// Binary Operator Functions
	friend polyRatio __stdcall operator+(const polyRatio &, const polyRatio &);
	friend polyRatio __stdcall operator+(const polyRatio &, double);
	friend polyRatio __stdcall operator+(const polyRatio &, const Complex&);
	friend polyRatio __stdcall operator+(const polyRatio &, const poly&);
	friend polyRatio __stdcall operator+(double, const polyRatio &);
	friend polyRatio __stdcall operator+(const Complex&, const polyRatio &);
	friend polyRatio __stdcall operator+(const poly&, const polyRatio &);

	friend polyRatio __stdcall operator-(const polyRatio &, const polyRatio &);
	friend polyRatio __stdcall operator-(const polyRatio &, double);
	friend polyRatio __stdcall operator-(const polyRatio &, const Complex&);
	friend polyRatio __stdcall operator-(const polyRatio &, const poly&);
	friend polyRatio __stdcall operator-(double, const polyRatio &);
	friend polyRatio __stdcall operator-(const Complex&, const polyRatio &);
	friend polyRatio __stdcall operator-(const poly&, const polyRatio &);

	friend polyRatio __stdcall operator*(const polyRatio &, double);
	friend polyRatio __stdcall operator*(const polyRatio &, const Complex&);
	friend polyRatio __stdcall operator*(const polyRatio &, const poly&);
	friend polyRatio __stdcall operator*(double, const polyRatio &);
	friend polyRatio __stdcall operator*(const Complex&, const polyRatio &);
	friend polyRatio __stdcall operator*(const poly&, const polyRatio &);

	friend polyRatio __stdcall operator/(const polyRatio &, const polyRatio &);
	friend polyRatio __stdcall operator/(const polyRatio &, const poly &);
//	friend polyRatio __stdcall operator%(const polyRatio &, const polyRatio &);
	friend polyRatio __stdcall operator/(const polyRatio &, double);
	friend polyRatio __stdcall operator/(const polyRatio &, const Complex&);
	friend polyRatio __stdcall operator<<(const polyRatio &, int);
	friend polyRatio __stdcall operator>>(const polyRatio &, int);
	friend int __stdcall operator==(const polyRatio &, const polyRatio &);
	friend int __stdcall operator!=(const polyRatio &, const polyRatio &);

	friend polyRatio __stdcall reduce(const polyRatio &);
	Complex eval(Complex arg) const;
	Complex operator ()(Complex x) const { return eval(x); }
	polyRatio deriv(void) const;
	void ScaleRoots(const Complex & scale);

	polyRatio & operator += (const polyRatio &);
	polyRatio & operator += (const poly &);
	polyRatio & operator += (const Complex &);
	polyRatio & operator += (double);

	polyRatio & operator -= (const polyRatio &);
	polyRatio & operator -= (const poly &);
	polyRatio & operator -= (const Complex &);
	polyRatio & operator -= (double);

	polyRatio & operator *= (const polyRatio &);
	polyRatio & operator *= (const poly &);
	polyRatio & operator *= (const Complex &);
	polyRatio & operator *= (double);

	polyRatio & operator /= (const polyRatio &);
	polyRatio & operator /= (const poly &);
	polyRatio & operator /= (const Complex &);
	polyRatio & operator /= (double);
	polyRatio & operator<<= (int);
	polyRatio & operator>>= (int);
	polyRatio & operator = (const polyRatio &);

	polyRatio operator+() const;
	polyRatio operator-() const;
	int DenomOrder() const {return m_denom.order(); }
	int NumerOrder() const {return m_numer.order(); }
	poly & denom() {return m_denom; }
	const poly & denom() const {return m_denom; }
	poly & numer() {return m_numer; }
	const poly &numer() const {return m_numer; }

#ifdef _DEBUG
	void Dump(CDumpContext & = afxDump);
#endif

	~polyRatio();
protected:
	poly m_denom, m_numer;
};

class lpolyRatio
{
	friend class polyRatio;
public:
	lpolyRatio(const lpolyRatio&);
	lpolyRatio(const polyRatio&);
	lpolyRatio(lcomplex first=lcomplex(0., 0.), int iMaxOrder=200);
	lpolyRatio(lcomplex *zeros, int nOrder=0,
				lcomplex *poles=NULL, int dOrder=0,
				lcomplex first=lcomplex(1., 0.));
	// Binary Operator Functions
	friend lpolyRatio __stdcall operator+(const lpolyRatio &, const lpolyRatio &);
	friend lpolyRatio __stdcall operator+(const lpolyRatio &, ldouble);
	friend lpolyRatio __stdcall operator+(const lpolyRatio &, const lcomplex&);
	friend lpolyRatio __stdcall operator+(const lpolyRatio &, const lpoly&);
	friend lpolyRatio __stdcall operator+(ldouble, const lpolyRatio &);
	friend lpolyRatio __stdcall operator+(const lcomplex&, const lpolyRatio &);
	friend lpolyRatio __stdcall operator+(const lpoly&, const lpolyRatio &);

	friend lpolyRatio __stdcall operator-(const lpolyRatio &, const lpolyRatio &);
	friend lpolyRatio __stdcall operator-(const lpolyRatio &, ldouble);
	friend lpolyRatio __stdcall operator-(const lpolyRatio &, const lcomplex&);
	friend lpolyRatio __stdcall operator-(const lpolyRatio &, const lpoly&);
	friend lpolyRatio __stdcall operator-(ldouble, const lpolyRatio &);
	friend lpolyRatio __stdcall operator-(const lcomplex&, const lpolyRatio &);
	friend lpolyRatio __stdcall operator-(const lpoly&, const lpolyRatio &);

	friend lpolyRatio __stdcall operator*(const lpolyRatio &, ldouble);
	friend lpolyRatio __stdcall operator*(const lpolyRatio &, const lcomplex&);
	friend lpolyRatio __stdcall operator*(const lpolyRatio &, const lpoly&);
	friend lpolyRatio __stdcall operator*(ldouble, const lpolyRatio &);
	friend lpolyRatio __stdcall operator*(const lcomplex&, const lpolyRatio &);
	friend lpolyRatio __stdcall operator*(const lpoly&, const lpolyRatio &);

	friend lpolyRatio __stdcall operator/(const lpolyRatio &, const lpolyRatio &);
	friend lpolyRatio __stdcall operator/(const lpolyRatio &, const lpoly &);
//	friend lpolyRatio __stdcall operator%(const lpolyRatio &, const lpolyRatio &);
	friend lpolyRatio __stdcall operator/(const lpolyRatio &, ldouble);
	friend lpolyRatio __stdcall operator/(const lpolyRatio &, const lcomplex&);
	friend lpolyRatio __stdcall operator<<(const lpolyRatio &, int);
	friend lpolyRatio __stdcall operator>>(const lpolyRatio &, int);
	friend int __stdcall operator==(const lpolyRatio &, const lpolyRatio &);
	friend int __stdcall operator!=(const lpolyRatio &, const lpolyRatio &);
	lcomplex eval(lcomplex arg) const;
	lcomplex operator ()(lcomplex x) const { return eval(x); }
	lpolyRatio deriv(void) const;
	void ScaleRoots(const lcomplex & scale);

	lpolyRatio & operator+=(const lpolyRatio &);
	lpolyRatio & operator+=(const lpoly &);
	lpolyRatio & operator+=(const lcomplex &);
	lpolyRatio & operator+=(ldouble);

	lpolyRatio & operator-=(const lpolyRatio &);
	lpolyRatio & operator-=(const lpoly &);
	lpolyRatio & operator-=(const lcomplex &);
	lpolyRatio & operator-=(ldouble);

	lpolyRatio & operator*=(const lpolyRatio &);
	lpolyRatio & operator*=(const lpoly &);
	lpolyRatio & operator*=(const lcomplex &);
	lpolyRatio & operator*=(ldouble);

	lpolyRatio & operator/=(const lpolyRatio &);
	lpolyRatio & operator/=(const lpoly &);
	lpolyRatio & operator/=(const lcomplex &);
	lpolyRatio & operator/=(ldouble);
	lpolyRatio & operator<<=(int);
	lpolyRatio & operator>>=(int);
	lpolyRatio & operator = (const lpolyRatio &);

	lpolyRatio operator+() const;
	lpolyRatio operator-() const;
	int DenomOrder() const {return m_denom.order(); }
	int NumerOrder() const {return m_numer.order(); }
	lpoly &denom() {return m_denom; }
	const lpoly & denom() const {return m_denom; }
	lpoly &numer() {return m_numer; }
	const lpoly &numer() const {return m_numer; }

#ifdef _DEBUG
	void Dump(CDumpContext & = afxDump);
#endif

	~lpolyRatio();
protected:
	lpoly m_denom, m_numer;
};

#pragma pack(pop)
ostream & __stdcall operator<<(ostream &, const lpolyRatio &);
istream & __stdcall operator>>(istream &, lpolyRatio &);
ostream & __stdcall operator<<(ostream &, const polyRatio &);
istream & __stdcall operator>>(istream &, polyRatio &);

#endif // _INC_POLYRATIO
