// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
	double angle = -M_PI / count;
	double rot_r = cos(angle);
	double rot_i = -sin(angle);
	double u_r = 0., u_i = 1.;

	x[count] = x[0];

	for (int i = 0, k = count; i <= count / 2; i++, k--)
	{
		double tmp_r = real(x[k]) + real(x[i]);
		double tmp_i = imag(x[k]) - imag(x[i]); // -sign omitted
		double tmp2_r = real(x[k]) - real(x[i]);
		double tmp2_i = imag(x[k]) + imag(x[i]); // -sign omitted
		double tmp3 = u_r * tmp2_r + u_i * tmp2_i;   // tmp2_r

		tmp2_i = u_i * tmp2_r - u_r * tmp2_i;

		x[i].real(T(0.5 * (tmp3 + tmp_r)));
		x[i].imag(T(0.5 * (tmp2_i - tmp_i)));
		x[k].real(T(0.5 * (tmp_r - tmp3)));
		x[k].imag(T(0.5 * (tmp2_i + tmp_i)));

		tmp_r = u_r * rot_r - u_i * rot_i;
		u_i = u_r * rot_i + u_i * rot_r;
		u_r = tmp_r;
	}
}

// Conversion of 'count+1' complex FFT result terms to 'count+1'
// terms as if they were obtained from complex data.
// Used in complex->real IFFT
template<class T>
void IFFTPreProc(const complex<T> * src, complex<T> * dst, const int count)
{
	ASSERT(count > 0 && count % 2 == 0);
	double angle = -M_PI / count;
	double rot_r = cos(angle), rot_i = sin(angle);
	double u_r = 0., u_i = -1.;

	dst[0] = T(0.5) * (src[0] + conj(src[count]) + complex<T>(0., -1.) * (conj(src[count]) - src[0]));

	for (int i = 1, k = count - 1; i <= count / 2; i++, k--)
	{
		double tmp_r = u_r * rot_r - u_i * rot_i;
		u_i = u_r * rot_i + u_i * rot_r;
		u_r = tmp_r;
		tmp_r = real(src[i]) + real(src[k]);

		double tmp_i = imag(src[k]) - imag(src[i]); // -sign omitted
		double tmp2_r = real(src[k]) - real(src[i]);
		double tmp2_i = imag(src[k]) + imag(src[i]); // -sign omitted
		double tmp3 = u_r * tmp2_r + u_i * tmp2_i;   // tmp2_r

		tmp2_i = u_i * tmp2_r - u_r * tmp2_i;

		dst[i].real(T(0.5 * (tmp3 + tmp_r)));
		dst[i].imag(T(0.5 * (tmp2_i - tmp_i)));
		dst[k].real(T(0.5 * (tmp_r - tmp3)));
		dst[k].imag(T(0.5 * (tmp2_i + tmp_i)));
	}
}

//inline implementation of a generic complex->complex fft function
template<class T>
void FastFourierTransformCore(const T * src, T * dst,
							const int order_power,
							const int reverse_fft)
{
	int i;
	int n = 1 << (order_power + 1);
	if (src != dst)
	{
		for (i = 0; i < n; i ++)
		{
			dst[i] = src[i];
		}
	}
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
			T z = (T)(M_PI / f * 2.);

			T c = (T)(cos(z));
			int e = f * 2;
			T s = (T)(sin(z));
			if (reverse_fft) s = -s;

			T u = 1.;
			T v = 0.;

			for (int j = 0; j < f; j += 2)
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

	if (! reverse_fft) return;

	T a = (T)(2.0 / n);
	for (int k = 0; k < n; k += 2)
	{
		dst[k] *= a;
		dst[k + 1] *= a;
	}

	return;
}

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
