// File FFT.inl

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#ifndef ASSERT
#define ASSERT(x)
#endif

// Conversion of 'count' complex FFT result terms to 'count'
// terms as if they were obtained from real data. Used in real->complex FFT
template<class T>
void FFTPostProc(complex<T> * x, const int count)
{
	ASSERT(count > 0 && count % 2 == 0);
	double angle = M_PI / count;
	complex<T> rotator(cos(angle), -sin(angle));
	complex<T> u(0., 1.);
	x[count] = x[0];
	for (int i = 0, k = count; i <= count / 2; i++, k--)
	{
		complex<T> tmp1 = conj(x[k]);
		complex<T> tmp2 = u * (x[i] - tmp1);
		tmp1 += x[i];
		x[i] = (tmp1 - tmp2) * T(0.5);
		x[k] = conj((tmp1 + tmp2) * T(0.5));
		u *= rotator;
	}
}

// Conversion of 'count+1' complex FFT result terms to 'count+1'
// terms as if they were obtained from complex data.
// Used in complex->real IFFT
template<class T>
void IFFTPreProc(const complex<T> * src, complex<T> * dst, const int count)
{
	ASSERT(count > 0 && count % 2 == 0);
	double angle = M_PI / count;
	complex<T> rotator(cos(angle), sin(angle));
	complex<T> u(0., -1.);
	dst[0] = T(0.5) * (src[0] + conj(src[count]) + u * (conj(src[count]) - src[0]));
	for (int i = 1, k = count - 1; i <= count / 2; i++, k--)
	{
		u *= rotator;
		complex<T> tmp1 = conj(src[k]);
		complex<T> tmp2 = u * (tmp1 - src[i]);
		tmp1 += src[i];
		dst[i] = T(0.5) * (tmp1 + tmp2);
		dst[k] = T(0.5) * conj(tmp1 - tmp2);
	}
}

//inline implementation of a generic complex->complex fft function
#if 1
template<class T>
void FastFourierTransformCore(const T * src, T * dst,
							const int order_power,
							const int reverse_fft)
{
	int n = 1 << (order_power + 1);
	if (src != dst)
	{
		memcpy(dst, src, n * sizeof * dst);
	}
	for (int L = 0; L < order_power; L++)
	{

		int f = 1 << (order_power - L);
		int e = f * 2;

		if (f > 2)
		{
			T z = M_PI / f * 2.;

			T c = cos(z);
			T s = sin(z);
			if (reverse_fft) s = -s;

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

	if (! reverse_fft) return;

	T a = 2.0 / n;
	for (int k = 0; k < n; k += 2)
	{
		dst[k] *= a;
		dst[k + 1] *= a;
	}

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
