/* Complex.h

 Complex Number Library - Include File
 class Complex: declarations for Complex numbers.

All function names, member names, and operators have been borrowed
from AT&T C++, except for the addition of:

 friend Complex acos(Complex &);
 friend Complex asin(Complex &);
 friend Complex atan(Complex &);
 friend Complex log10(Complex &);
 friend Complex tan(Complex &);
 friend Complex tanh(Complex &);
 Complex operator+();
 Complex operator-();
*/

/*
 *  C/C++ Run Time Library - Version 6.5
 *
 *  Copyright (c) 1990, 1994 by Borland International
 *  All Rights Reserved.
 *
 */

#ifndef __cplusplus
#error Must use C++ for the type Complex.
#endif

#if !defined(__COMPLEX_H)
#define __COMPLEX_H
/* Constants rounded for 21 decimals. */
#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.434294481903251827651
#define M_LN2       0.693147180559945309417
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_HALF_PI   M_PI_2
#define M_PI_4      0.785398163397448309616
#define M_QUARTER_PI   M_PI_4
#define M_1_PI      0.318309886183790671538
#define M_2_PI      0.636619772367581343076
#define M_1_SQRTPI  0.564189583547756286948
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT_2    0.707106781186547524401

#ifdef __LONG_COMPLEX
 #define DOUBLE long double
#else
#ifdef __FLOAT_COMPLEX
 #define DOUBLE float
#else
 #define DOUBLE double
#endif
#endif
#if !defined(_INC_IOSTREAM)
 #include "iostream.h"
#endif
//#if !defined(___DEFS_H)
//#include <_defs.h>
//#endif

#if !defined(_INC_MATH)
 #include <math.h>
#endif

#undef Complex

#pragma pack(push, 8)

class Complex {

public:
	// constructors
	Complex(DOUBLE __re_val, DOUBLE __im_val=0);
	Complex();
#ifdef __LCOMPLEX_H
	friend class lcomplex;
	Complex(const lcomplex&);
#endif
	// Complex manipulations
	friend DOUBLE __stdcall real(const Complex &); // the real part
	friend DOUBLE __stdcall imag(const Complex &); // the imaginary part
	friend DOUBLE & __stdcall real(Complex *); // the real part
	friend DOUBLE & __stdcall imag(Complex *); // the imaginary part
	friend Complex __stdcall conj(const Complex &); // the Complex conjugate
	friend DOUBLE __stdcall norm(const Complex &); // the square of the magnitude
	friend DOUBLE __stdcall arg(const Complex &); // the angle in the plane

	// Create a Complex object given polar coordinates
	friend Complex __stdcall polar(DOUBLE __mag, DOUBLE __angle=0);

	// Overloaded ANSI C math functions
	friend DOUBLE __stdcall abs(const Complex &);
	friend Complex __stdcall acos(const Complex &);
	friend Complex __stdcall asin(const Complex &);
	friend Complex __stdcall atan(const Complex &);
	friend Complex __stdcall cos(const Complex &);
	friend Complex __stdcall cosh(const Complex &);
	friend Complex __stdcall exp(const Complex &);
	friend Complex __stdcall log(const Complex &);
	friend Complex __stdcall log10(const Complex &);
	friend Complex __stdcall pow(const Complex & __base, DOUBLE __expon);
	friend Complex __stdcall pow(DOUBLE __base, const Complex & __expon);
	friend Complex __stdcall pow(const Complex & __base, const Complex & __expon);
	friend Complex __stdcall sin(const Complex &);
	friend Complex __stdcall sinh(const Complex &);
	friend Complex __stdcall sqrt(const Complex &);
	friend Complex __stdcall tan(const Complex &);
	friend Complex __stdcall tanh(const Complex &);

	// Binary Operator Functions
	friend Complex __stdcall operator+(const Complex &, const Complex &);
	friend Complex __stdcall operator+(DOUBLE, const Complex &);
	friend Complex __stdcall operator+(const Complex &, DOUBLE);
	friend Complex __stdcall operator-(const Complex &, const Complex &);
	friend Complex __stdcall operator-(DOUBLE, const Complex &);
	friend Complex __stdcall operator-(const Complex &, DOUBLE);
	friend Complex __stdcall operator*(const Complex &, const Complex &);
	friend Complex __stdcall operator*(const Complex &, DOUBLE);
	friend Complex __stdcall operator*(DOUBLE, const Complex &);
	friend Complex __stdcall operator/(const Complex &, const Complex &);
	friend Complex __stdcall operator/(const Complex &, DOUBLE);
	friend Complex __stdcall operator/(DOUBLE, const Complex &);
	friend int __stdcall operator==(const Complex &, const Complex &);
	friend int __stdcall operator!=(const Complex &, const Complex &);
	Complex & operator+=(const Complex &);
	Complex & operator+=(DOUBLE);
	Complex & operator-=(const Complex &);
	Complex & operator-=(DOUBLE);
	Complex & operator*=(const Complex &);
	Complex & operator*=(DOUBLE);
	Complex & operator/=(const Complex &);
	Complex & operator/=(DOUBLE);
	Complex operator+() const;
	Complex operator-() const;

// Implementation
public:
	DOUBLE re, im;
};

#pragma pack(pop)

// Inline Complex functions

inline Complex::Complex(DOUBLE __re_val, DOUBLE __im_val)
{
	re = __re_val;
	im = __im_val;
}

#ifdef __LCOMPLEX_H
inline Complex::Complex(const lcomplex &a)
{
	re = a.re;
	im = a.im;
}
#endif

inline Complex::Complex()
{
/* if you want your Complex numbers initialized ...
 re = im = 0;
*/
}

inline Complex Complex::operator+()	const
{
	return *this;
}

inline Complex Complex::operator-()	const
{
	return Complex(-re, -im);
}


// Definitions of compound-assignment operator member functions

inline Complex & Complex::operator+=(const Complex & __z2)
{
	re += __z2.re;
	im += __z2.im;
	return *this;
}

inline Complex & Complex::operator+=(DOUBLE __re_val2)
{
	re += __re_val2;
	return *this;
}

inline Complex & Complex::operator-=(const Complex & __z2)
{
	re -= __z2.re;
	im -= __z2.im;
	return *this;
}

inline Complex & Complex::operator-=(DOUBLE __re_val2)
{
	re -= __re_val2;
	return *this;
}

inline Complex & Complex::operator*=(DOUBLE __re_val2)
{
	re *= __re_val2;
	im *= __re_val2;
	return *this;
}

inline Complex & Complex::operator/=(DOUBLE __re_val2)
{
	re /= __re_val2;
	im /= __re_val2;
	return *this;
}


// Definitions of non-member Complex functions

inline DOUBLE __stdcall real(const Complex & z)
{
	return z.re;
}

inline DOUBLE __stdcall imag(const Complex & z)
{
	return z.im;
}

inline DOUBLE & __stdcall real(Complex * pz)
{
	return pz->re;
}

inline DOUBLE & __stdcall imag(Complex * pz)
{
	return pz->im;
}

inline Complex __stdcall conj(const Complex & z)
{
	return Complex(z.re, -z.im);
}

inline Complex __stdcall polar(DOUBLE __mag, DOUBLE __angle)
{
	return Complex(__mag*cos(__angle), __mag*sin(__angle));
}


// Definitions of non-member binary operator functions

inline Complex __stdcall operator+(const Complex & __z1, const Complex & __z2)
{
	return Complex(__z1.re + __z2.re, __z1.im + __z2.im);
}

inline Complex __stdcall operator+(DOUBLE __re_val1, const Complex & __z2)
{
	return Complex(__re_val1 + __z2.re, __z2.im);
}

inline Complex __stdcall operator+(const Complex & __z1, DOUBLE __re_val2)
{
	return Complex(__z1.re + __re_val2, __z1.im);
}

inline Complex __stdcall operator-(const Complex & __z1, const Complex & __z2)
{
	return Complex(__z1.re - __z2.re, __z1.im - __z2.im);
}

inline Complex __stdcall operator-(DOUBLE __re_val1, const Complex & __z2)
{
	return Complex(__re_val1 - __z2.re, -__z2.im);
}

inline Complex __stdcall operator-(const Complex & __z1, DOUBLE __re_val2)
{
	return Complex(__z1.re - __re_val2, __z1.im);
}

inline Complex __stdcall operator*(const Complex & __z1, DOUBLE __re_val2)
{
	return Complex(__z1.re*__re_val2, __z1.im*__re_val2);
}

inline Complex __stdcall operator*(DOUBLE __re_val1, const Complex & __z2)
{
	return Complex(__z2.re*__re_val1, __z2.im*__re_val1);
}

inline Complex __stdcall operator/(const Complex & __z1, DOUBLE __re_val2)
{
	return Complex(__z1.re/__re_val2, __z1.im/__re_val2);
}

inline int __stdcall operator==(const Complex & __z1, const Complex & __z2)
{
	return __z1.re == __z2.re && __z1.im == __z2.im;
}

inline int __stdcall operator!=(const Complex & __z1, const Complex & __z2)
{
	return __z1.re != __z2.re || __z1.im != __z2.im;
}


inline double  __stdcall abs(const Complex & z)
{
	return (sqrt(z.re*z.re + z.im*z.im));
}

inline double  __stdcall norm(const Complex & z)
{
	return (z.re*z.re + z.im*z.im);
}

inline Complex &  Complex::operator*=(const Complex & z2)
{
	double temp_re = re*z2.re - im*z2.im;
	im = im*z2.re + re*z2.im;
	re = temp_re;
	return *this;
}

inline Complex  __stdcall operator*(const Complex & z1, const Complex & z2)
{
	return Complex(z1.re*z2.re - z1.im*z2.im,
					z1.im*z2.re + z1.re*z2.im);
}

inline Complex & Complex::operator/=(const Complex & z2)
{
	double sum_sqrs = norm(z2);
	*this = Complex((re*z2.re + im*z2.im) / sum_sqrs,
					(im*z2.re - re*z2.im) / sum_sqrs);
	return *this;
}

inline Complex __stdcall operator/(const Complex & z1, const Complex & z2)
{
	double sum_sqrs = norm(z2);
	return Complex((z1.re*z2.re + z1.im*z2.im) / sum_sqrs,
					(z1.im*z2.re - z1.re*z2.im) / sum_sqrs);
}

inline Complex __stdcall operator/(double re_val1, const Complex & z2)
{
	double sum_sqrs = norm(z2);
	return Complex(re_val1 * z2.re / sum_sqrs,
					- re_val1 * z2.im / sum_sqrs);
}

// Complex stream I/O

ostream & __stdcall operator<<(ostream &, const Complex &);
istream & __stdcall operator>>(istream &, Complex &);

#endif // __COMPLEX_H

