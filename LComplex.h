/* lcomplex.h

 Complex Number Library - Include File
 class lcomplex: declarations for Complex numbers.

All function names, member names, and operators have been borrowed
from AT&T C++, except for the addition of:

 friend lcomplex acos(lcomplex &);
 friend lcomplex asin(lcomplex &);
 friend lcomplex atan(lcomplex &);
 friend lcomplex log10(lcomplex &);
 friend lcomplex tan(lcomplex &);
 friend lcomplex tanh(lcomplex &);
 lcomplex operator+();
 lcomplex operator-();
*/

/*
 *  C/C++ Run Time Library - Version 6.5
 *
 *  Copyright (c) 1990, 1994 by Borland International
 *  All Rights Reserved.
 *
 */

#ifndef __cplusplus
#error Must use C++ for the type lcomplex.
#endif

#if !defined(__LCOMPLEX_H)
#define __LCOMPLEX_H

/* Constants rounded for 21 decimals. */
#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.434294481903251827651
#define M_LN2       0.693147180559945309417
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
#define M_1_PI      0.318309886183790671538
#define M_2_PI      0.636619772367581343076
#define M_1_SQRTPI  0.564189583547756286948
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT_2    0.707106781186547524401

#if !defined(_INC_IOSTREAM)
 #include "iostream.h"
#endif
//#if !defined(___DEFS_H)
//#include <_defs.h>
//#endif

#if !defined(_INC_MATH)
 #include <math.h>
#undef	Complex
#endif

const DOUBLE_SIZE = sizeof(double);
const LDOUBLE_SIZE = sizeof(long double);
#if 1 //DOUBLE_SIZE == LDOUBLE_SIZE
//#if sizeof(double) == 8
#if !defined(_INC_LDOUBLE)
 #include "LDouble.h"
#endif
#define LDOUBLE ldouble
#else
#define LDOUBLE long double
#endif

#undef lcomplex
#pragma pack(push,2)
class lcomplex {

public:
	// constructors
	lcomplex(LDOUBLE __re_val, LDOUBLE __im_val);
	lcomplex();
	friend class Complex;
	lcomplex(const Complex &);
	// lcomplex manipulations
	friend LDOUBLE __stdcall real(const lcomplex &); // the real part
	friend LDOUBLE __stdcall imag(const lcomplex &); // the imaginary part
	friend LDOUBLE & __stdcall real(lcomplex *); // the real part
	friend LDOUBLE & __stdcall imag(lcomplex *); // the imaginary part
	friend lcomplex __stdcall conj(const lcomplex &); // the lcomplex conjugate
	friend LDOUBLE __stdcall norm(const lcomplex &); // the square of the magnitude
	friend LDOUBLE __stdcall arg(const lcomplex &); // the angle in the plane

	// Create a lcomplex object given polar coordinates
	friend lcomplex __stdcall polar(LDOUBLE __mag, LDOUBLE __angle=0);

	// Overloaded ANSI C math functions
	friend LDOUBLE __stdcall abs(const lcomplex &);
	friend lcomplex __stdcall acos(const lcomplex &);
	friend lcomplex __stdcall asin(const lcomplex &);
	friend lcomplex __stdcall atan(const lcomplex &);
	friend lcomplex __stdcall cos(const lcomplex &);
	friend lcomplex __stdcall cosh(const lcomplex &);
	friend lcomplex __stdcall exp(const lcomplex &);
	friend lcomplex __stdcall log(const lcomplex &);
	friend lcomplex __stdcall log10(const lcomplex &);
	friend lcomplex __stdcall pow(const lcomplex & __base, LDOUBLE __expon);
	friend lcomplex __stdcall pow(LDOUBLE __base, const lcomplex & __expon);
	friend lcomplex __stdcall pow(const lcomplex & __base, const lcomplex & __expon);
	friend lcomplex __stdcall sin(const lcomplex &);
	friend lcomplex __stdcall sinh(const lcomplex &);
	friend lcomplex __stdcall sqrt(const lcomplex &);
	friend lcomplex __stdcall tan(const lcomplex &);
	friend lcomplex __stdcall tanh(const lcomplex &);

	// Binary Operator Functions
	friend lcomplex __stdcall operator+(const lcomplex &, const lcomplex &);
	friend lcomplex __stdcall operator+(LDOUBLE, const lcomplex &);
	friend lcomplex __stdcall operator+(const lcomplex &, LDOUBLE);
	friend lcomplex __stdcall operator-(const lcomplex &, const lcomplex &);
	friend lcomplex __stdcall operator-(LDOUBLE, const lcomplex &);
	friend lcomplex __stdcall operator-(const lcomplex &, LDOUBLE);
	friend lcomplex __stdcall operator*(const lcomplex &, const lcomplex &);
	friend lcomplex __stdcall operator*(const lcomplex &, LDOUBLE);
	friend lcomplex __stdcall operator*(LDOUBLE, const lcomplex &);
	friend lcomplex __stdcall operator/(const lcomplex &, const lcomplex &);
	friend lcomplex __stdcall operator/(const lcomplex &, LDOUBLE);
	friend lcomplex __stdcall operator/(LDOUBLE, const lcomplex &);
	friend int __stdcall operator==(const lcomplex &, const lcomplex &);
	friend int __stdcall operator!=(const lcomplex &, const lcomplex &);
	lcomplex & operator+=(const lcomplex &);
	lcomplex & operator+=(LDOUBLE);
	lcomplex & operator-=(const lcomplex &);
	lcomplex & operator-=(LDOUBLE);
	lcomplex & operator*=(const lcomplex &);
	lcomplex & operator*=(LDOUBLE);
	lcomplex & operator/=(const lcomplex &);
	lcomplex & operator/=(LDOUBLE);
	lcomplex & operator=(LDOUBLE a) {re = a; im = 0.; return *this; }
	lcomplex & operator=(double a) {re = a; im = 0.; return *this; }
	lcomplex operator+() const;
	lcomplex operator-() const;

// Implementation
public:
	LDOUBLE re, im;
};

#if !defined(__COMPLEX_H)
#include "Complex.h"
#endif

// Inline lcomplex functions

inline lcomplex::lcomplex(LDOUBLE __re_val, LDOUBLE __im_val):
	re(__re_val), im(__im_val)
{
}

inline lcomplex::lcomplex(const Complex &c):
	re(ldouble(real(c))), im(ldouble(imag(c)))
{
}

inline lcomplex::lcomplex()
{
/* if you want your lcomplex numbers initialized ...
 re = im = 0;
*/
}

inline lcomplex lcomplex::operator+()	const
{
	return *this;
}

inline lcomplex lcomplex::operator-()	const
{
	return lcomplex(-re, -im);
}


// Definitions of compound-assignment operator member functions

inline lcomplex & lcomplex::operator+=(const lcomplex & __z2)
{
	re += __z2.re;
	im += __z2.im;
	return *this;
}

inline lcomplex & lcomplex::operator+=(LDOUBLE __re_val2)
{
	re += __re_val2;
	return *this;
}

inline lcomplex & lcomplex::operator-=(const lcomplex & __z2)
{
	re -= __z2.re;
	im -= __z2.im;
	return *this;
}

inline lcomplex & lcomplex::operator-=(LDOUBLE __re_val2)
{
	re -= __re_val2;
	return *this;
}

inline lcomplex & lcomplex::operator*=(LDOUBLE __re_val2)
{
	re *= __re_val2;
	im *= __re_val2;
	return *this;
}

inline lcomplex & lcomplex::operator/=(LDOUBLE __re_val2)
{
	re /= __re_val2;
	im /= __re_val2;
	return *this;
}


// Definitions of non-member lcomplex functions

inline LDOUBLE __stdcall real(const lcomplex & __z)
{
	return __z.re;
}

inline LDOUBLE __stdcall imag(const lcomplex & __z)
{
	return __z.im;
}

inline LDOUBLE & __stdcall real(lcomplex * __z)
{
	return __z->re;
}

inline LDOUBLE & __stdcall imag(lcomplex * __z)
{
	return __z->im;
}

inline lcomplex __stdcall conj(const lcomplex & __z)
{
	return lcomplex(__z.re, -__z.im);
}

inline lcomplex __stdcall polar(LDOUBLE __mag, LDOUBLE __angle)
{
	return lcomplex(__mag*ldouble(::cos(double(__angle))),
					__mag*ldouble(::sin(double(__angle))));
}


// Definitions of non-member binary operator functions

inline lcomplex __stdcall operator+(const lcomplex & __z1, const lcomplex & __z2)
{
	return lcomplex(__z1.re + __z2.re, __z1.im + __z2.im);
}

inline lcomplex __stdcall operator+(LDOUBLE __re_val1, const lcomplex & __z2)
{
	return lcomplex(__re_val1 + __z2.re, __z2.im);
}

inline lcomplex __stdcall operator+(const lcomplex & __z1, LDOUBLE __re_val2)
{
	return lcomplex(__z1.re + __re_val2, __z1.im);
}

inline lcomplex __stdcall operator-(const lcomplex & __z1, const lcomplex & __z2)
{
	return lcomplex(__z1.re - __z2.re, __z1.im - __z2.im);
}

inline lcomplex __stdcall operator-(LDOUBLE __re_val1, const lcomplex & __z2)
{
	return lcomplex(__re_val1 - __z2.re, -__z2.im);
}

inline lcomplex __stdcall operator-(const lcomplex & __z1, LDOUBLE __re_val2)
{
	return lcomplex(__z1.re - __re_val2, __z1.im);
}

inline lcomplex __stdcall operator*(const lcomplex & __z1, LDOUBLE __re_val2)
{
	return lcomplex(__z1.re*__re_val2, __z1.im*__re_val2);
}

inline lcomplex __stdcall operator*(LDOUBLE __re_val1, const lcomplex & __z2)
{
	return lcomplex(__z2.re*__re_val1, __z2.im*__re_val1);
}

inline lcomplex __stdcall operator/(const lcomplex & __z1, LDOUBLE __re_val2)
{
	return lcomplex(__z1.re/__re_val2, __z1.im/__re_val2);
}

inline int __stdcall operator==(const lcomplex & __z1, const lcomplex & __z2)
{
	return __z1.re == __z2.re && __z1.im == __z2.im;
}

inline int __stdcall operator!=(const lcomplex & __z1, const lcomplex & __z2)
{
	return __z1.re != __z2.re || __z1.im != __z2.im;
}


// lcomplex stream I/O

ostream & __stdcall operator<<(ostream &, const lcomplex &);
istream & __stdcall operator>>(istream &, lcomplex &);
#pragma pack(pop)

#endif // __COMPLEX_H

