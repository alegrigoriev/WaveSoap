// File FFT.inl

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef ASSERT
#define ASSERT(x)
#endif

#ifdef _DEBUG
#pragma warning(disable: 4035)
static inline __int64 FftReadTSC()
{
	__asm RDTSC
}
#pragma warning(default: 4035)

unsigned FftTime1, FftTime2, FftTime3, FftTime4, FftTime5, FftTime6, FftTime7;
#endif

// Conversion of 'count' complex FFT result terms to 'count'
// terms as if they were obtained from real data. Used in real->complex FFT
template<class T>
void FFTPostProc(complex<T> * x, const int count)
{
#ifdef _DEBUG
	FftTime6 = FftReadTSC();
#endif
	ASSERT(count > 0 && count % 2 == 0);
	double angle = -M_PI / count;
	T rot_r = cos(angle), rot_i = -sin(angle);
	T u_r = 0., u_i = 1.;
	x[count] = x[0];
	for (int i = 0, k = count; i <= count / 2; i++, k--)
	{
		T tmp_r = real(x[k]) + real(x[i]);
		T tmp_i = imag(x[k]) - imag(x[i]); // -sign omitted
		T tmp2_r = real(x[k]) - real(x[i]);
		T tmp2_i = imag(x[k]) + imag(x[i]); // -sign omitted
		T tmp3 = u_r * tmp2_r + u_i * tmp2_i;   // tmp2_r
		tmp2_i = u_i * tmp2_r - u_r * tmp2_i;
		x[i].real(T(0.5) * (tmp3 + tmp_r));
		x[i].imag(T(0.5) * (tmp2_i - tmp_i));
		x[k].real(T(0.5) * (tmp_r - tmp3));
		x[k].imag(T(0.5) * (tmp2_i + tmp_i));
		tmp_r = u_r * rot_r - u_i * rot_i;
		u_i = u_r * rot_i + u_i * rot_r;
		u_r = tmp_r;
	}
#ifdef _DEBUG
	FftTime7 = FftReadTSC();
#endif
}

// Conversion of 'count+1' complex FFT result terms to 'count+1'
// terms as if they were obtained from complex data.
// Used in complex->real IFFT
template<class T>
void IFFTPreProc(const complex<T> * src, complex<T> * dst, const int count)
{
#ifdef _DEBUG
	FftTime6 = FftReadTSC();
#endif
	ASSERT(count > 0 && count % 2 == 0);
	double angle = -M_PI / count;
	T rot_r = cos(angle), rot_i = sin(angle);
	T u_r = 0., u_i = -1.;
	dst[0] = T(0.5) * (src[0] + conj(src[count]) + complex<T>(0., -1.) * (conj(src[count]) - src[0]));
	for (int i = 1, k = count - 1; i <= count / 2; i++, k--)
	{
		T tmp_r = u_r * rot_r - u_i * rot_i;
		u_i = u_r * rot_i + u_i * rot_r;
		u_r = tmp_r;
		tmp_r = real(src[i]) + real(src[k]);
		T tmp_i = imag(src[k]) - imag(src[i]); // -sign omitted
		T tmp2_r = real(src[k]) - real(src[i]);
		T tmp2_i = imag(src[k]) + imag(src[i]); // -sign omitted
		T tmp3 = u_r * tmp2_r + u_i * tmp2_i;   // tmp2_r
		tmp2_i = u_i * tmp2_r - u_r * tmp2_i;
		dst[i].real(T(0.5) * (tmp3 + tmp_r));
		dst[i].imag(T(0.5) * (tmp2_i - tmp_i));
		dst[k].real(T(0.5) * (tmp_r - tmp3));
		dst[k].imag(T(0.5) * (tmp2_i + tmp_i));
	}
#ifdef _DEBUG
	FftTime7 = FftReadTSC();
#endif
}

//inline implementation of a generic complex->complex fft function
#if 1
template<class T>
void FastFourierTransformCore(const T * src, T * dst,
							const int order_power,
							const int reverse_fft)
{
#ifdef _DEBUG
	FftTime1 = FftReadTSC();
#endif
	int i;
	int n = 1 << (order_power + 1);
	if (src != dst)
	{
		for (i = 0; i < n; i ++)
		{
			dst[i] = src[i];
		}
	}
#ifdef _DEBUG
	FftTime2 = FftReadTSC();
#endif
	for (int L = 0; L < order_power; L++)
	{

		int f = 1 << (order_power - L);

		if (f <= 2)
		{
			for(i = 0; i < n; i += 4)
			{
				T p, q;
				int o = i + f;
				ASSERT(o < n);
				p = dst[i] + dst[o];
				q = dst[i + 1] + dst[o + 1];
				dst[o] = dst[i] - dst[o];
				dst[i] = p;
				dst[o + 1] = dst[i + 1] - dst[o + 1];
				dst[i + 1] = q;
			} // i-loop
		}
		else
		{
			T z = M_PI / f * 2.;

			T c = cos(z);
			int e = f * 2;
			T s = sin(z);
			if (reverse_fft) s = -s;

			T u = 1.;
			T v = 0.;

			for (int j = 2; j < f; j += 2)
			{
				for(i = j; i < n; i += e)
				{
					double r, t;
					int o = i + f;
					ASSERT(o < n);
					//p = dst[i] + dst[o];
					r = dst[i] - dst[o];
					//q = dst[i + 1] + dst[o + 1];
					t = dst[i + 1] - dst[o + 1];
					dst[i] = dst[i] + dst[o];
					dst[o] = T(r * u - t * v);
					dst[i + 1] = dst[i + 1] + dst[o + 1];
					dst[o + 1] = T(t * u + r * v);
				} // i-loop
				double p = u * c - v * s;
				v = v * c + u * s;
				u = (T)p;
			}  // j - loop
		}
	} // L - loop

#ifdef _DEBUG
	FftTime3 = FftReadTSC();
#endif
	int j;
	for (i = 0, j = 0; i < n >> 1; i +=4)
	{
		T t[2];
		if (i <= j)
		{
			// exchange 0..i..1 and 1..j..0
			memcpy(t, & dst[j + (n >> 1)], 2 * sizeof dst[j]);
			memcpy(& dst[j + (n >> 1)], & dst[i + 2], 2 * sizeof dst[j]);
			memcpy(& dst[i + 2], t, 2 * sizeof dst[j]);
		}
		else
		{
			memcpy(t, & dst[j + (n >> 1)], 2 * sizeof dst[j]);
			memcpy(& dst[j + (n >> 1)], & dst[i + 2], 2 * sizeof dst[j]);
			memcpy(& dst[i + 2], t, 2 * sizeof dst[j]);
			// exchange 0..i..0 and 0..j..0
			memcpy(t, & dst[j], 2 * sizeof dst[j]);
			memcpy(& dst[j], & dst[i], 2 * sizeof dst[j]);
			memcpy(& dst[i], t, 2 * sizeof dst[j]);
			// exchange 1..i..1 and 1..j..1
			memcpy(t, & dst[j + 2 + (n >> 1)], 2 * sizeof dst[j]);
			memcpy(& dst[j + 2 + (n >> 1)], & dst[i + 2 + (n >> 1)], 2 * sizeof dst[j]);
			memcpy(& dst[i + 2 + (n >> 1)], t, 2 * sizeof dst[j]);
		}
		// n = 2*number of complex numbers
		int k = n >> 2;
		while(k & j)
		{
			j ^= k;
			k >>= 1;
		}
		j += k;
	}

#ifdef _DEBUG
	FftTime5 = FftTime4 = FftReadTSC();
#endif
	if (! reverse_fft) return;

	T a = 2.0 / n;
	for (int k = 0; k < n; k += 2)
	{
		dst[k] *= a;
		dst[k + 1] *= a;
	}

#ifdef _DEBUG
	FftTime5 = FftReadTSC();
#endif
	return;
}
#elif 1
template<class T>
void FastFourierTransformCore(const T * src, T * dst,
							const int order_power,
							const int flag)
{
	int n = 1 << (order_power + 1);
	if (src != dst)
	{
		memcpy(dst, src, n * sizeof * dst);
	}
	for (int L = order_power-1; L >= 0; L--)
	{

		int f = 1 << (order_power - L);
		int e = f * 2;

		if (1 || f > 2)
		{
			T z = M_PI / f * 2.;

			T c = cos(z);
			T s = sin(z);
			if (!(flag & REV_FFT)) s = -s;

			T u = (T)1.;
			T v = (T)0.;

			for (int j = 0; j < f; j += 2)
			{
				for(int i = j; i < n; i += e)
				{
					double r, t;
					int o = i + f;
					ASSERT(o < n);
					//p = dst[i] + dst[o];
					r = dst[i] - dst[o];
					//q = dst[i + 1] + dst[o + 1];
					t = dst[i + 1] - dst[o + 1];
					dst[i] = dst[i] + dst[o];
					dst[i + 1] = dst[i + 1] + dst[o + 1];
					dst[o] = T(r * u - t * v);
					dst[o + 1] = T(t * u + r * v);
				} // i-loop
				double p = u * c - v * s;
				v = v * c + u * s;
				u = (T)p;
			}  // j - loop
		}
		else
		{
			for(int i = 0; i < n; i += e)
			{
				T p, q;
				int o = i + f;
				ASSERT(o < n);
				p = dst[i] + dst[o];
				q = dst[i + 1] + dst[o + 1];
				dst[o] = dst[i] - dst[o];
				dst[o + 1] = dst[i + 1] - dst[o + 1];
				dst[i] = p;
				dst[i + 1] = q;
			} // i-loop
		}
	} // L - loop

	for (int i = 0, j = 0; i < n - 2; i += 2)
	{
		if (i < j)
		{
			T p = dst[j];
			T q = dst[j + 1];
			dst[j] = dst[i];
			dst[i] = p;
			dst[j + 1] = dst[i + 1];
			dst[i + 1] = q;
		}
		int k = n / 2;
		while(k <= j)
		{
			j = j - k;
			k = k / 2;
		}
		j += k;
	}

	if (! (flag & REV_FFT)) return;

	T a = 2.0 / n;
	for (int k = 0; k < n; k += 2)
	{
		dst[k] *= a;
		dst[k + 1] *= a;
	}

	return;
}
#else
template<class T>
void FastFourierTransformCore(const T * src, T * dst,
							const int order_power,
							const int flag)
{
	int n = 1 << (order_power + 1);
	if (src != dst)
	{
		memcpy(dst, src, n * sizeof * dst);
	}
	for (int L = 0; L < order_power / 2; L++)
	{
		// use 4 order fourier transform
		// along with bit reversal
		int stride1 = (1 << L) * 2; // 2 because of two numbers per term
		int stride2 = 1 << (order_power - L);// (1 << (order_power -1 - L)) * 2

		T z1 = M_PI / (stride1 / 2);
		T c1 = cos(z1);
		T s1 = sin(z1);

		T z2 = M_PI / (stride2 / 2);
		T c2 = cos(z2);
		T s2 = sin(z2);

		T z3 = stride1 * M_PI / stride2;
		T c3 = cos(z3);
		T s3 = sin(z3);

		if (!(flag & REV_FFT))
		{
			s1 = -s1;
			s2 = -s2;
			s3 = -s3;
		}

		T u1;
		T v1;
		T u2;
		T v2;
		T u3 = (T)1.;
		T v3 = (T)0.;

		for (int i = n-2; i >= 0; i -= 2)
		{
			if (i & stride2)
			{
				u2 = c2;
				v2 = -s2;
				i -= stride2;
			}
			else
			{
				double p = u2 * c2 + v2 * s2;
				v2 = v2 * c2 - u2 * s2;
				u2 = (T)p;
			}

			if (i & stride1)
			{
				u1 = c1;
				v1 = -s1;
				i -= stride1;
			}
			else
			{
				double p = u1 * c1 + v1 * s1;
				v1 = v1 * c1 - u1 * s1;
				u1 = (T)p;
			}

			double r, t, r1, t1;
			int o10 = i + stride2;
			int o01 = i + stride1;
			int o11 = o01 + stride2;
			// stride2 > stride1
			ASSERT(o10 < n && o01 < n && o11 < n);
			//p = dst[i] + dst[o];
			r = dst[i] - dst[o01];
			r1 = dst[o10] - dst[o11];
			//q = dst[i + 1] + dst[o + 1];
			t = dst[i + 1] - dst[o + 1];
			t1 = dst[o10 + 1] - dst[o11 + 1];

			dst[i] = dst[i] + dst[o01];
			dst[i + 1] = dst[i + 1] + dst[o01 + 1];

			dst[o10] = T(r1 * u1 - t1 * v1);
			dst[o10 + 1] = T(t1 * u1 + r1 * v1);

		}  // j - loop
	} // L - loop

	if (! (flag & REV_FFT)) return;

	T a = 2.0 / n;
	for (int k = 0; k < n; k += 2)
	{
		dst[k] *= a;
		dst[k + 1] *= a;
	}

	return;
}
#endif

template<class T> void FastFourierTransform(complex<T> * x, int order_power,
											int direction)
{
	FastFourierTransformCore(x, x, order_power, direction);
}

// FFT real -> complex.
// converts (count) real source samples to (count / 2 + 1)
// complex terms.
// IMPORTANT: dst array should be of size (count / 2 + 1);
template<class T>
void FastFourierTransform(const T * src, complex<T> * dst,
						int count)
{
	ASSERT(count > 0 && count < 0x08000000 && count % 2 == 0
			&& src != NULL && dst != NULL);
	int order_power = 1;
	for (; count > (2 << order_power); order_power++);

	ASSERT(count == (2 << order_power));

	//T * tmp = reinterpret_cast<T * >(dst);
	FastFourierTransformCore(src, reinterpret_cast<T * >(dst), order_power, 0);
	FFTPostProc(dst, count / 2);
}

// IFFT complex -> real.
// converts (count / 2 + 1) complex source terms to
// (count) real samples.
// IMPORTANT: src array should be of size (count / 2 + 1);
template<class T>
void FastInverseFourierTransform(const complex<T> * src, T * dst,
								int count)
{
	ASSERT(count > 0 && count < 0x08000000 && count % 2 == 0
			&& src != NULL && dst != NULL);
	int order_power = 1;
	for (; count > (2 << order_power); order_power++);

	ASSERT(count == (2 << order_power));

	IFFTPreProc(src, reinterpret_cast<complex<T> *>(dst), count / 2);

	FastFourierTransformCore(dst, dst, order_power, REV_FFT);
}
