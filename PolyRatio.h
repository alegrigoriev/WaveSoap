// PolyRatioMath
#if !defined(_INC_POLYRATIO)
 #define _INC_POLYRATIO

#if !defined(_INC_POLYMATH)
 #include "PolyMath.h"
#endif

#include <afxtempl.h>

class polyRatio
{
public:
	CArray<polyRatio *, polyRatio *> *
		Decompose(int nCellsOrder, polyRoots * DenomRoots,
				int nFirstRatioOrder = 0);
	polyRatio(const polyRatio&);
	polyRatio(const poly& numer,const poly& denom = poly((int)0, Complex(1.)));
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
	//friend int __stdcall operator==(const polyRatio &, const polyRatio &);
	//friend int __stdcall operator!=(const polyRatio &, const polyRatio &);

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

std::ostream & __stdcall operator<<(std::ostream &, const polyRatio &);
std::istream & __stdcall operator>>(std::istream &, polyRatio &);

#endif // _INC_POLYRATIO
