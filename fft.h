// fft.h, copyright Alex Grigoriev, alegr@aha.ru

#define FFT    0
#define REV_FFT 1

// make complex<double> default complex
//template<class _TYPE=double> class complex;
#include <complex>
using namespace std;
// 'x' array contains real part of data, 'y' array contains imaginary
// part of data. Result is placed back in the arrays.
// Size of each array is exp2(order_power).
template<class T>
void FastFourierTransformCore(const T * src, T * dst,
							const int order_power, const int flag = FFT);
template<class T>
void FastFourierTransform(const complex<T> * src, complex<T> * dst,
						int count, int direction = FFT)
{
	ASSERT(count > 0 && count < 0x08000000 && count % 2 == 0
			&& src != NULL && dst != NULL);
	int order_power = 1;
	for (; count > (1 << order_power); order_power++);

	ASSERT(count == (1 << order_power));

	FastFourierTransformCore(reinterpret_cast<T const *>(src),
							reinterpret_cast<T * >(dst), order_power, direction);
}

template<class T>
inline void FastInverseFourierTransform(const complex<T> * src, complex<T> * dst,
										int count)
{
	FastFourierTransform(src, dst, count, REV_FFT);
}

// x contains real part, y contains imag part
template<class T>
void FastSplitFourierTransform(T * x, T * y,
								int order_power, int direction = FFT);
template<class T>
void FastSplitInverseFourierTransform(T * x, T * y,
									int order_power)
{
	FastSplitFourierTransform(x, y, order_power, REV_FFT);
}

// x array contains source data. x[n*2] is real part, x[n*2 + 1] is imag part.
// Result is placed back in the array.
// Size of 'x' array is 2*exp2(order_power).
template<class T>
void FastFourierTransform(T * x, int order_power,
						int direction = FFT);
template<class T>
void FastInverseFourierTransform(T * x, int order_power)
{
	FastFourierTransform(x, order_power, REV_FFT);
}

// x array contains source data.
// Result is placed back in the array.
// Size of 'x' array is exp2(order_power).

//template<float>
void FastFourierTransform(complex<float> * x, int order_power,
						int direction);
//template<double>
void FastFourierTransform(complex<double> * x, int order_power,
						int direction);

#if 0
template<float>
void FastInverseFourierTransform(complex<float> * x, int order_power)
{
	FastFourierTransform(x, order_power, REV_FFT);
}

template<double>
void FastInverseFourierTransform(complex<double> * x, int order_power)
{
	FastFourierTransform(x, order_power, REV_FFT);
}
#endif
// FFT real -> complex.
// converts (count) real source samples to (count / 2 + 1)
// complex terms.
// IMPORTANT: dst array should be of size (count / 2 + 1);
template<class T>
void FastFourierTransform(const T * src, complex<T> * dst,
						int count);
// IFFT complex -> real.
// converts (count / 2 + 1) complex source terms to
// (count) real samples.
// IMPORTANT: src array should be of size (count / 2 + 1);
template<class T>
void FastInverseFourierTransform(const complex<T> * src, T * dst,
								int count);

#include "fft.inl"
