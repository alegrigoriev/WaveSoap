// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// waveproc.cpp

#include <math.h>
#pragma intrinsic(sin, cos, exp, log, atan2)
#include "stdafx.h"
#include <complex>
#include "Waveproc.h"
#include <float.h>

#ifndef _CONSOLE
#define puts(t) AfxMessageBox(_T(t), MB_OK | MB_ICONEXCLAMATION)

#endif
#define DB_TO_NEPER 0.23025851

#include "FFT.h"

#define TRACE_WAVEPROC 0

template <typename T = UCHAR, unsigned s = 512>
struct FixedRingBufferBase
{
protected:
	FixedRingBufferBase() {}
	T Array[s];
	enum { size = s, };
	typedef T Type;
	C_ASSERT(size < 0xFFFFu);
};

template <typename T = UCHAR, typename T_src = T>
struct RingBufferBase
{
protected:
	RingBufferBase() : Array(NULL), size(0) {}

	T * Array;
	size_t size;
	typedef T Type;
	typedef T_src Type_Src;
};

template<typename Base>
class RingBufferT : public Base
{
protected:
	typedef typename Base::Type Type;
	typedef typename Base::Type_Src Type_Src;
public:
	RingBufferT()
	{
		ReadIndex = 0;
		Filled = 0;
	}
	void Write(Type_Src data);
	// remove samples from FIFO
	void Discard(int n)
	{
		ASSERT(n <= int(Filled));
		Filled -= n;
		ReadIndex += n;
		if (ReadIndex >= size)
		{
			ReadIndex -= size;
		}
	}
	Type Read();

	Type & AtOut(int index)
	{
		ASSERT(index < Filled);

		index += ReadIndex;
		if (index >= size)
		{
			index -= size;
		}
		return Array[index];
	}

	Type & AtIn(int index)
	{
		ASSERT(-index <= Filled);
		index += ReadIndex + Filled;
		if (index >= size)
		{
			index -= size;
		}
		return Array[index];
	}

	Type & operator[](int index)
	{
		ASSERT(index < int(Filled));

		index += ReadIndex;
		if (index >= int(size))
		{
			index -= size;
		}
		return Array[index];
	}

	void Purge()
	{
		Filled = 0;
		ReadIndex = 0;
	}

	unsigned AvailableToWrite() const
	{
		return size - Filled;
	}

	unsigned AvailableToRead() const
	{
		return Filled;
	}
	size_t Size() const
	{
		return size;
	}
private:
	unsigned ReadIndex;
	unsigned Filled;
};

template <typename T = UCHAR>
class RingBufferA : public RingBufferT<RingBufferBase<T> >
{
	bool bBufferAllocated;
public:

	RingBufferA()
		: bBufferAllocated(false)
	{
		size = 0;
		Array = NULL;
	}
	~RingBufferA()
	{
		if (bBufferAllocated)
		{
			delete[] Array;
		}
	}
	void SetBuffer(Type * pBuffer, size_t Count)
	{
		ASSERT(Count < 0xFFFFu);
		size = 0;
		Purge();
		Array = pBuffer;
		size = Count;
		Purge();
	}

	void AllocateBuffer(size_t Count)
	{
		if (bBufferAllocated)
		{
			return;
		}
		if (Count > 0xFFFF)
		{
			Count = 0xFFFF;
		}
		Type * buf = new Type[Count];
		if (NULL != buf)
		{
			SetBuffer(buf, Count);
			bBufferAllocated = true;
		}
	}
};

template <typename T = UCHAR, int size = 512>
class RingBuffer : public RingBufferT<FixedRingBufferBase<T, size> >
{
};

template <typename L>
void RingBufferT<L>::Write(Type_Src data)
{
	if (Filled >= size)
	{
		return;
	}

	unsigned WriteIndex = ReadIndex + Filled;
	if (WriteIndex >= size)
	{
		WriteIndex -= size;
	}

	Array[WriteIndex] = data;
	Filled++;
}

template <typename L>
typename RingBufferT<L>::Type RingBufferT<L>::Read()
{
	if (0 == Filled)
	{
		return Type();
	}

	Filled--;

	if (ReadIndex >= size - 1)
	{
		ReadIndex -= size - 1;
		return Array[ReadIndex + (size - 1)];
	}
	else
	{
		return Array[ReadIndex ++];
	}
}
//////////////////////////////////////////////////////////////
/////////// CWaveProc
CWaveProc::CWaveProc()
	: m_bClipped(FALSE),
	m_MaxClipped(0),
	m_ChannelsToProcess(-1)
	, m_SavedOutputSamples(0)
	, m_ProcessedInputSamples(0)
#ifdef _DEBUG
	, m_ProcessedInputBytes(0)
	, m_SavedOutputBytes(0)
#endif
{
	m_InputFormat.InitCdAudioFormat();
	m_OutputFormat.InitCdAudioFormat();
}

size_t CWaveProc::ProcessSoundBuffer(char const * /*pInBuf*/, char * /*pOutBuf*/,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	* pUsedBytes = nInBytes;
	return nOutBytes;
}

BOOL CWaveProc::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	m_InputFormat = pWf;
	m_OutputFormat = pWf;   // by default, the format doesn't change

	return pWf->wFormatTag == WAVE_FORMAT_PCM
			&& pWf->wBitsPerSample == 16
			&& pWf->nBlockAlign == pWf->nChannels * 2 //pWf->wBitsPerSample / 8
			&& pWf->nAvgBytesPerSec == pWf->nSamplesPerSec * pWf->nBlockAlign;
}

WAVEFORMATEX const * CWaveProc::GetInputWaveformat() const
{
	return m_InputFormat;
}

WAVEFORMATEX const * CWaveProc::GetOutputWaveformat() const
{
	return m_OutputFormat;
}

size_t CWaveProc::ProcessSound(char const * pInBuf, char * pOutBuf,
								size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	size_t nSavedBytes = 0;
	*pUsedBytes = 0;

	size_t const InputSampleSize = GetInputSampleSize();
	// input buffer is always multiple of sample size
	ASSERT(0 == InputSampleSize || 0 == nInBytes % InputSampleSize);

	size_t const OutputSampleSize = GetOutputSampleSize();
	// output buffer is always multiple of sample size
	ASSERT(0 == OutputSampleSize || 0 == nOutBytes % OutputSampleSize);

	nSavedBytes = ProcessSoundBuffer(pInBuf, pOutBuf, nInBytes, nOutBytes, pUsedBytes);

	if (InputSampleSize != 0)
	{
		ASSERT(0 == *pUsedBytes % InputSampleSize);
		m_ProcessedInputSamples += *pUsedBytes / InputSampleSize;
	}

	if (OutputSampleSize != 0)
	{
		ASSERT(0 == nSavedBytes % OutputSampleSize);
		m_SavedOutputSamples += nSavedBytes / OutputSampleSize;
	}
#ifdef _DEBUG
	m_ProcessedInputBytes += *pUsedBytes;
	m_SavedOutputBytes += nSavedBytes;
#endif
	return nSavedBytes;
}

void CWaveProc::Dump(unsigned indent) const
{
	TRACE("%*.s%s\n", indent, "", typeid(*this).name());
	TRACE(_T(" %*.sSamples per second=%d, Input channels=%d, Output channels=%d\n")
		_T(" %*.sChannels to process=%x, Max clipped=%f\n"),
		indent, "", m_InputFormat.SampleRate(), m_InputFormat.NumChannels(), m_OutputFormat.NumChannels(),
		indent, "", m_ChannelsToProcess, m_MaxClipped);
}

__int16 CWaveProc::DoubleToShort(double x)
{
	long tmp = (long) floor(x + 0.5);
	if (tmp < -0x8000)
	{
		if (m_MaxClipped < -x)
		{
			m_MaxClipped = -x;
		}
		m_bClipped = TRUE;
		return -0x8000;
	}
	else if (tmp > 0x7FFF)
	{
		if (m_MaxClipped < x)
		{
			m_MaxClipped = x;
		}
		m_bClipped = TRUE;
		return 0x7FFF;
	}
	else
	{
		return __int16(tmp);
	}
}

BOOL CWaveProc::Init()
{
	m_ProcessedInputSamples = 0;
	m_SavedOutputSamples = 0;
	return TRUE;
}

void CWaveProc::DeInit()
{
#ifdef _DEBUG
	if (TRACE_WAVEPROC) TRACE("%s: Processed input samples=%d, bytes=%d\n",
							typeid(*this).name(), m_ProcessedInputSamples, m_ProcessedInputBytes);
	if (TRACE_WAVEPROC) TRACE("         Saved output samples=%d, bytes=%d\n",
							m_SavedOutputSamples, m_SavedOutputBytes);
#endif
}


////////////////// CHumRemoval ///////////////////////
CHumRemoval::CHumRemoval(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess)
{
	m_ApplyHighpassFilter = FALSE;
	m_ApplyCommonModeFilter = TRUE;

	m_prev_out[0] = 0.;
	m_prev_out[1] = 0.;
	m_prev_in[0] = 0;
	m_prev_in[1] = 0;

	m_PrevHpf[0][0] = 0.;
	m_PrevHpf[1][0] = 0.;
	m_PrevHpf[0][1] = 0.;
	m_PrevHpf[1][1] = 0.;

	m_PrevHpOut[0][0] = 0.;
	m_PrevHpOut[1][0] = 0.;
	m_PrevHpOut[0][1] = 0.;
	m_PrevHpOut[1][1] = 0.;

	m_DiffCutoffCoeffs[0] = 0.003575f;
	m_DiffCutoffCoeffs[1] = 0.9857f;

	m_HighpassCoeffs[0] = 1.99636f;
	m_HighpassCoeffs[1] = 0.996363312f;
	m_HighpassCoeffs[2] = 1.;

	m_ChannelsToProcess = ChannelsToProcess;
	SetInputWaveformat(pWf);
}

BOOL CHumRemoval::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	return CWaveProc::SetInputWaveformat(pWf);
}

void CHumRemoval::SetDifferentialCutoff(double frequency)
{
	m_DiffCutoffCoeffs[1] = 1. - M_PI * frequency / m_InputFormat.SampleRate();
	m_DiffCutoffCoeffs[0] = 0.25 * (1. - m_DiffCutoffCoeffs[1]);
	TRACE("m_DiffCutoffCoeffs=%.6f, %.6f\n", m_DiffCutoffCoeffs[0],
		m_DiffCutoffCoeffs[1]);
}

void CHumRemoval::SetHighpassCutoff(double frequency)
{
	// pole for second order filter
	double a = 1. - 1.5537739 * M_PI * frequency / m_InputFormat.SampleRate();

	m_HighpassCoeffs[0] = 2. * a;
	m_HighpassCoeffs[1] = a * a;

	// norm coefficient
	m_HighpassCoeffs[2] = (m_HighpassCoeffs[0] + m_HighpassCoeffs[1] + 1.) * 0.25;

	TRACE("m_HighpassCoeffs=%.6f, %.6f, %.6f\n", m_HighpassCoeffs[0],
		m_HighpassCoeffs[1],  m_HighpassCoeffs[2]);
}

size_t CHumRemoval::ProcessSoundBuffer(char const * pIn, char * pOut,
										size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	size_t nSavedBytes = 0;
	*pUsedBytes = 0;

	unsigned const SampleSize = m_InputFormat.SampleSize();
	ASSERT(m_InputFormat.SampleSize() == m_OutputFormat.SampleSize());

	size_t nInSamples = nInBytes / SampleSize;
	size_t nOutSamples = nOutBytes / SampleSize;

	WAVE_SAMPLE const * pInBuf = (WAVE_SAMPLE const *) pIn;
	WAVE_SAMPLE * pOutBuf = (WAVE_SAMPLE *) pOut;

	// process the data
	unsigned nSamples = std::min(nInSamples, nOutSamples);

	if (0 == nSamples || NULL == pInBuf)
	{
		return nSavedBytes;   // no delayed samples in history
	}

	ASSERT(m_InputFormat.NumChannels() == m_OutputFormat.NumChannels());

	unsigned const NumChannels = m_InputFormat.NumChannels();

	for (unsigned i = 0; i < nSamples; i ++,
		pInBuf += NumChannels, pOutBuf += NumChannels)
	{
		double curr[MAX_NUMBER_OF_CHANNELS];

		if (m_ApplyCommonModeFilter
			&& 2 == NumChannels)
		{
			m_prev_out[0] = (pInBuf[0] + m_prev_in[0]) * m_DiffCutoffCoeffs[0]
							+ m_DiffCutoffCoeffs[1] * m_prev_out[0];

			m_prev_out[1] = (pInBuf[1] + m_prev_in[1]) * m_DiffCutoffCoeffs[0]
							+ m_DiffCutoffCoeffs[1] * m_prev_out[1];

			double hpf_l = m_prev_in[0] - m_prev_out[0];
			double hpf_r = m_prev_in[1] - m_prev_out[1];

			m_prev_in[0] = pInBuf[0];
			m_prev_in[1] = pInBuf[1];

			curr[0] = hpf_l + m_prev_out[1];
			curr[1] = hpf_r + m_prev_out[0];
		}
		else
		{
			for (unsigned ch = 0; ch < NumChannels; ch++)
			{
				curr[ch] = pInBuf[ch];
			}
		}

		for (unsigned ch = 0; ch < NumChannels; ch++)
		{
			if (m_ApplyHighpassFilter
				&& 0 != (m_ChannelsToProcess & (1 << ch)))
			{
				double out = ((curr[ch] - m_PrevHpf[0][ch] - m_PrevHpf[0][ch] + m_PrevHpf[1][ch])
								+ m_HighpassCoeffs[0] * m_PrevHpOut[0][ch] - m_HighpassCoeffs[1] * m_PrevHpOut[1][ch]);

				m_PrevHpOut[1][ch] = m_PrevHpOut[0][ch];
				m_PrevHpOut[0][ch] = out;

				curr[ch] = out * m_HighpassCoeffs[2];
				m_PrevHpf[1][ch] = m_PrevHpf[0][ch];
				m_PrevHpf[0][ch] = curr[ch];

				// apply additional highpass 2nd order filter to both channels
				// with cutoff frequency 50 Hz
				// (1-2z + z^2)/(a^2 -2az + z^2)

			}
		}

		for (unsigned ch = 0; ch < NumChannels; ch++)
		{
			if (m_ChannelsToProcess & (1 << ch))
			{
				pOutBuf[ch] = DoubleToShort(curr[ch]);
			}
			else
			{
				pOutBuf[ch] = pInBuf[ch];
			}
		}

	}

	*pUsedBytes += nSamples * SampleSize;
	return nSavedBytes + nSamples * SampleSize;
}

/////////// CClickRemoval ////////////////////////////////////////////
CClickRemoval::DeclickChannelData::DeclickChannelData()
	: m_NextPossibleClickPosition(16 * CLICK_LENGTH)
	, m_MeanPower(0)
	, m_PrevDeriv(0)
	, m_PrevDeriv2(0)
{
	m_prev.Allocate(PREV_BUF_SIZE);
	m_prev3.Allocate(PREV_BUF_SIZE);
}

DeclickParameters::DeclickParameters()
	: m_nMaxClickLength(32)
	, m_nMinClickLength(4)

	, m_MeanPowerDecayRate(1./100.)  // 1 ms
	, m_MeanPowerAttackRate(1./15.)  // 1 ms
	, m_PowerToDeriv3RatioThreshold(30.)
	, m_MinDeriv3Threshold(200*200)
	, m_MinClickDeriv3BoundThreshold(80.)
	, m_ClickDeriv3ThresholdScale(0.1)
	, m_NoiseFloorThresholdScale(3.)
	, m_bLogClicksOnly(FALSE)
	, m_bLogClicks(FALSE)
	, m_bImportClicks(FALSE)
	, m_bDontAutodetectClicks(FALSE)
{

}

void CClickRemoval::DeclickChannelData::StoreData(WAVE_SAMPLE * pOut, unsigned nSamples, int Stride)
{
	for (unsigned i = 0; i < nSamples; i++, pOut += Stride)
	{
		*pOut = WAVE_SAMPLE(m_prev[1 + i - ANALYZE_LAG]);
	}
	m_prev.Advance(nSamples);
}

int CClickRemoval::DeclickChannelData::Update3RdDerivativePowerThreshold(CClickRemoval const * pCr)
{
	int deriv = m_prev[CLICK_LENGTH / 2] - m_prev[CLICK_LENGTH / 2 - 1];
	int deriv2 = deriv - m_PrevDeriv;

	m_PrevDeriv = deriv;

	int deriv3 = deriv2 - m_PrevDeriv2;

	m_PrevDeriv2 = deriv2;
	m_prev3[CLICK_LENGTH / 2] = deriv3;

	int Deriv3Threshold = int(m_MeanPower * pCr->m_PowerToDeriv3RatioThreshold);

	double power = float(m_prev3[0]);
	power = power * power;

	if (power > m_MeanPower)
	{
		m_MeanPower += (power - m_MeanPower)
						* pCr->m_MeanPowerAttackRate;
	}
	else
	{
		m_MeanPower += (power - m_MeanPower)
						* pCr->m_MeanPowerDecayRate;
	}
	return Deriv3Threshold;
}


CClickRemoval::CClickRemoval(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess,
							DeclickParameters const & dp)
	: DeclickParameters(dp)
	, m_PrevIndex(0)
	, m_nStoredSamples(0)
	, m_pOutClicksFile(0)
{
	//m_Deriv2Threshold = 60.;

	m_ChannelsToProcess = ChannelsToProcess;
	SetInputWaveformat(pWf);

}

BOOL CClickRemoval::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}
	if (m_bImportClicks)
	{
		if ( ! LoadClickSourceFile(m_ClickImportFilename))
		{
			return FALSE;
		}
	}
	if (m_bLogClicks)
	{
		if ( ! SetClickLogFile(m_ClickLogFilename))
		{
			return FALSE;
		}
	}
	return TRUE;
}

CClickRemoval::~CClickRemoval()
{
	if (m_pOutClicksFile)
	{
		fclose(m_pOutClicksFile);
		m_pOutClicksFile = NULL;
	}
}

bool operator <(StoredClickData const & r1,
				StoredClickData const & r2)
{
	return r1.Position < r2.Position;
}

bool operator <(StoredClickData const & r1,
				SAMPLE_INDEX i)
{
	return r1.Position < i;
}

///////////////////////////////////////////////////////////
BOOL CClickRemoval::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	return CWaveProc::SetInputWaveformat(pWf);
}

void CClickRemoval::InterpolateGap(CBackBuffer<int, int> & data, int nLeftIndex, int InterpolateSamples, bool BigGap)
{
	const int MAX_FFT_ORDER = 2048;
	const int BufferSamples = MaxInterpolatedLength + 2 * MAX_FFT_ORDER + 3 * MaxInterpolatedLength;
	int PreInterpolateSamples = 0;
	int PostInterpolateSamples = 0;
	int InterpolationOverlap;

	if (BigGap)
	{
		InterpolationOverlap = MAX_FFT_ORDER + InterpolateSamples + InterpolateSamples / 2;
		PostInterpolateSamples = InterpolateSamples / 2;
		PreInterpolateSamples = InterpolateSamples - InterpolateSamples / 2;
	}
	else
	{
		InterpolationOverlap = 5 * InterpolateSamples;
	}

	WAVE_SAMPLE TempBuf[BufferSamples];
	const int InterpolateOffset = InterpolationOverlap;
	const int ReadStartOffset = nLeftIndex - InterpolationOverlap;
	const int WriteStartOffset = nLeftIndex - PreInterpolateSamples;
	const int WriteBufferOffset = InterpolationOverlap  - PreInterpolateSamples;

	int i;
	for (i = 0; i < InterpolateSamples + 2 * InterpolationOverlap; i++)
	{
		TempBuf[i] = WAVE_SAMPLE(data[ReadStartOffset + i]);
	}

	InterpolateGap(TempBuf, InterpolateOffset, InterpolateSamples, 1, BigGap);
	// copy back
	for (i = 0; i < InterpolateSamples + PreInterpolateSamples + PostInterpolateSamples; i++)
	{
		data[WriteStartOffset + i] = TempBuf[WriteBufferOffset + i];
	}
}

void CClickRemoval::InterpolateBigGap(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans)
{
	const int MAX_FFT_ORDER = 2048;
	float x[MAX_FFT_ORDER + MAX_FFT_ORDER / 4];
	// FFT order is >=64 and >= ClickLength * 4
	// take 2 FFT in [nLeftIndex-FftOrder... nLeftIndex-1] range
	// and [nLeftIndex-FftOrder-ClickLength...nLeftIndex-ClickLength-1] offset
	// Find next FFT estimation as FFT2*(FFT2/FFT1/abs(FFT2/FFT1))
	// Then take 2 FFT in [nLeftIndex+ClickLength...nLeftIndex+ClickLength+FftOrder-1]
	// and [nLeftIndex+2*ClickLength...nLeftIndex+2*ClickLength+FftOrder-1]
	// and find another FFT estimation. Perform backward FFT and combine source and
	// FFT results using squared-sine window
	complex<float> y1[MAX_FFT_ORDER/2+1];
	complex<float> y2[MAX_FFT_ORDER/2+1];
	//float xl[MAX_FFT_ORDER];  // to save extrapolation from the left neighborhood

	int FftOrder = 512;
	while (FftOrder < ClickLength * 8)
	{
		FftOrder += FftOrder;
	}

	TRACE("FFtOrder used for interpolation: %d\n", FftOrder);
	if (FftOrder > MAX_FFT_ORDER)
	{
		return;
	}

	// extrapolate ClickLength + ClickLength / 2 - the gap and
	// the right neighborhood
	int const ExtrapolatedLength = ClickLength + ClickLength / 2;
	int i;
	for (i = 0; i < FftOrder + ExtrapolatedLength; i++)
	{
		x[i] = float(data[nChans * (nLeftIndex - FftOrder - ExtrapolatedLength+ i)]);
	}

	FastFourierTransform(x, y1, FftOrder);
	FastFourierTransform(x + ExtrapolatedLength, y2, FftOrder);
	// calculate another set of coefficients
	// leave only those frequencies with up to ClickLength/10 period
	//if (nMaxFreq > FftOrder/2)

	for (i = 1; i <= FftOrder/2; i++)
	{
		if ((y1[i].real() != 0.
				|| y1[i].imag() != 0.)
			&& (y2[i].real() != 0.
				|| y2[i].imag() != 0.))
		{
			complex<float> rot = y2[i] / y1[i];
			float Abs = abs(rot);

			//if (Abs <= 2.)
			{
				y2[i] *= rot / Abs;
			}
		}
	}

	// extrapolate DC
	//y2[0].real(y2[0].real() * 2 - y1[0].real());

	FastInverseFourierTransform(y2, x, FftOrder);
	// last ClickLength*2 samples are of interest
	// save the result

	int const ClickLen1 = ClickLength - ClickLength / 2;

	// calculate DC adjustment
	double DcAdjustLeft = 0.; // difference from the previous data and the calculated data
	for (i = 0; i < ClickLen1; i++)
	{
		DcAdjustLeft += data[nChans * (nLeftIndex - ClickLen1 + i)] - x[FftOrder - ClickLength * 2 + i];
	}
	DcAdjustLeft /= ClickLen1;

	double DcAdjustRight = 0.; // difference from the previous data and the calculated data
	for (i = 0; i < ClickLength / 2; i++)
	{
		DcAdjustRight += data[nChans * (nLeftIndex + ClickLength + i)]
						- x[FftOrder - ClickLength / 2 + i];
	}
	DcAdjustRight /= ClickLength / 2;

	double DcAdjustDelta = (DcAdjustRight - DcAdjustLeft) / (ClickLength + ClickLength / 2);
	DcAdjustLeft -= DcAdjustDelta * (ClickLength / 4);

	bool const ShowExtrapolatedFft = false;
	// ClickLength-ClickLength/2 are merged with the samples before the extrapolation,
	float const * pFftResult = & x[FftOrder - ClickLength * 2];
	WAVE_SAMPLE * pWaveData = & data[nChans * (nLeftIndex - ClickLen1)];

	for (i = 0; i < ClickLen1; i++, DcAdjustLeft += DcAdjustDelta,
		pWaveData += nChans, pFftResult ++)
	{
		if ( ! ShowExtrapolatedFft)
		{
			double tmp = (( *pFftResult + DcAdjustLeft) * (i + 0.5)
							+ *pWaveData * (ClickLen1 - i - 0.5))
						/ float(ClickLen1);
			*pWaveData = DoubleToShort(tmp);
		}
		else
		{
			*pWaveData =
				DoubleToShort( *pFftResult);
		}
	}

	ASSERT((FftOrder - ClickLength * 2 + ClickLen1) == FftOrder - ExtrapolatedLength);
	// ClickLength is copied to the extrapolated area,
	for (i = 0; i < ClickLength; i++, DcAdjustLeft += DcAdjustDelta,
		pWaveData += nChans, pFftResult ++)
	{
		if ( ! ShowExtrapolatedFft)
		{
			double tmp =  *pFftResult + DcAdjustLeft;
			*pWaveData = DoubleToShort(tmp);
		}
		else
		{
			*pWaveData = DoubleToShort( *pFftResult);
		}
	}

	// ClickLength/2 are merged with the samples after the extrapolation,
	for (i = 0; i < ClickLength / 2; i++, DcAdjustLeft += DcAdjustDelta,
		pWaveData += nChans, pFftResult ++)
	{
		if ( ! ShowExtrapolatedFft)
		{
			double tmp = (( *pFftResult + DcAdjustLeft) * (ClickLength / 2 - i - 0.5)
							+ *pWaveData * (i + 0.5))
						/ float(ClickLength / 2);

			*pWaveData = DoubleToShort(tmp);
		}
		else
		{
			*pWaveData = DoubleToShort( *pFftResult);
		}
	}
	ASSERT(pFftResult == x + FftOrder);
}

void CClickRemoval::InterpolateGap(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, bool BigGap)
{
	if (BigGap)
	{
		InterpolateBigGap(data, nLeftIndex, ClickLength, nChans);
		return;
	}
	// nChan (1 or 2) is used as step between samples
	// to interpolate stereo, call the function twice
	// Perform spike interpolation
	// Use interpolating polynom by Lagrange
	// Zero point == nLeftIndex
	// Take 5 points to left with ClickLength/2 step
	// and 5 points to right
	// 2 farthest points to the left are spaced by ClickLength
	int const MaxInterpolationOrder = 15;
	double Y[MaxInterpolationOrder], X[MaxInterpolationOrder];

	ASSERT(ClickLength >= 2);
	int const InterpolationOrder = std::min(ClickLength, MaxInterpolationOrder);
	int const InterpolationStep = std::max(2, std::min(ClickLength / 4, 6));

	for (int n = 0; n < InterpolationOrder; n ++)
	{
		int index = (n / 2 * InterpolationStep + 1);
		if (n & 1)
		{
			index = - index;
		}
		else
		{
			index += ClickLength;
		}

		ASSERT(nLeftIndex + index >= 0);
		X[n] = index;
		Y[n] = data[nChans * (nLeftIndex + index)];
	}

	// perform Lagrange interpolation
	for (int n = 0; n < ClickLength; n++)
	{
		double x = n;
		double y = 0.;
		double y_neg = 0.;

		for (int k = 0; k < InterpolationOrder; k++)
		{
			double a = Y[k];
			for (int j = 0; j < InterpolationOrder; j++)
			{
				if (j != k)
				{
					a *= (x - X[j]) / (X[k] - X[j]);
				}
			}

			if (a >= 0)
			{
				y += a;
			}
			else
			{
				y_neg += a;
			}
		}
		data[nChans * (nLeftIndex + n)] = DoubleToShort(y + y_neg);
	}
}

size_t CClickRemoval::ProcessSoundBuffer(char const * pIn, char * pOut,
										size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;
	unsigned const SampleSize = m_InputFormat.SampleSize();
	NUMBER_OF_CHANNELS const NumChannels = m_InputFormat.NumChannels();

	unsigned nInSamples = nInBytes / SampleSize;
	unsigned nOutSamples = nOutBytes / SampleSize;
	WAVE_SAMPLE const * pInBuf = (WAVE_SAMPLE const *) pIn;
	WAVE_SAMPLE * pOutBuf = (WAVE_SAMPLE *) pOut;

	// process the data

	if (NULL == pInBuf)
	{
		// flush backlogged samples
		// nSamples - size of out buffer / size of samples
		if (m_PrevIndex <= m_nStoredSamples)
		{
			return nSavedBytes;
		}

		unsigned nBackSamples = std::min<unsigned>(m_PrevIndex - m_nStoredSamples, nOutSamples);

		for (int ch = 0; ch < NumChannels; ch++)
		{
			m_ChannelData[ch].StoreData(pOutBuf, nBackSamples, NumChannels);
			pOutBuf++;
		}

		m_nStoredSamples += nBackSamples;
		return nSavedBytes + nBackSamples * SampleSize;
	}

	int nStoreIndex = 0;
	SAMPLE_INDEX PrevIndex = m_PrevIndex;
	int nSamples = std::min(nInSamples, nOutSamples);

	for (int ch = 0; ch < NumChannels; ch++, pInBuf++, pOutBuf++)
	{
		int FftIn[CLICK_LENGTH];   // additional space for click length search
		//float FftOut[FFT_ORDER / 2];
		PrevIndex = m_PrevIndex;
		nStoreIndex = 0;
		DeclickChannelData * pCh = & m_ChannelData[ch];

		for (int i = 0; i < nSamples * NumChannels; i += NumChannels)
		{
			pCh->m_prev[ANALYZE_LAG] = pInBuf[i];

			if (0 != (m_ChannelsToProcess & (1 << ch)))
			{
				double Deriv3PowerThreshold =
					std::max(pCh->Update3RdDerivativePowerThreshold(this),
							(int)m_MinDeriv3Threshold);

				double deriv3_power = pCh->m_prev3[CLICK_LENGTH / 2];

				deriv3_power *= deriv3_power;

				BOOL ClickFound = FALSE;
				int nLeftIndex = CLICK_LENGTH;
				int nRightIndex = 0;
				int ClickLength = 0;

				// check predefined click list
				// TODO: use binary search
				ClicksVectorConstIterator it = std::lower_bound(m_PredefinedClicks.begin(),
																m_PredefinedClicks.end(), PrevIndex - ANALYZE_LAG);

				if (it < m_PredefinedClicks.end()
					&& it->Position == SAMPLE_INDEX(PrevIndex) - ANALYZE_LAG
					&& 0 != it->Length[ch])
				{
					nLeftIndex = 0;
					ClickLength = it->Length[ch];
					nRightIndex = nLeftIndex + ClickLength;

					// force interpolation on this position
					ClickFound = TRUE;
					pCh->m_NextPossibleClickPosition = PrevIndex + ClickLength * 2;
				}

				if ( ! ClickFound && ! (m_bDontAutodetectClicks && m_bImportClicks)
					&& PrevIndex >= pCh->m_NextPossibleClickPosition
					//&& deriv2 > m_MaxDeriv2Estimate[ch]
					//&& deriv3 > m_MaxDeriv2Estimate[ch] * 0.7
					&& deriv3_power > Deriv3PowerThreshold)
				{

					// spike detected
					// find its exact position as max of
					// third derivative
					for (unsigned f = 0; f < CLICK_LENGTH; f++)
					{
						FftIn[f] = pCh->m_prev[f];
					}

					int nMaxDeriv3Pos = 0;
					int OldDeriv = FftIn[2] - FftIn[1];
					int OldDeriv2 = FftIn[2] - 2 * FftIn[1] + FftIn[0];
					int MaxDeriv3 = 0;
					int deriv3;
					int deriv2;

					// find max abs third derivative point,
					for (unsigned f = 2; f < CLICK_LENGTH-1; f++)
					{
						int NewDeriv =  FftIn[f + 1] - FftIn[f];

						deriv2 = NewDeriv - OldDeriv;
						deriv3 = int(::abs(long(deriv2 - OldDeriv2)));
						OldDeriv = NewDeriv;
						OldDeriv2 = deriv2;

						if (deriv3 > MaxDeriv3)
						{
							MaxDeriv3 = deriv3;
							nMaxDeriv3Pos = f;
						}
					}
					// only points with deriv3 > threshold are considered clicks
					// find click length

					int Deriv3Threshold = std::max(std::max(int(MaxDeriv3 * m_ClickDeriv3ThresholdScale),
								int(m_MinClickDeriv3BoundThreshold)), int(sqrt(pCh->m_MeanPower) * m_NoiseFloorThresholdScale));

					OldDeriv = FftIn[2] - FftIn[1];
					OldDeriv2 = FftIn[2] - 2 * FftIn[1] + FftIn[0];

					// find max abs third derivative point,
					for (int f = 2; f < CLICK_LENGTH-1; f++)
					{
						int NewDeriv =  FftIn[f + 1] - FftIn[f];

						deriv2 = NewDeriv - OldDeriv;
						deriv3 = abs(deriv2 - OldDeriv2);
						OldDeriv = NewDeriv;
						OldDeriv2 = deriv2;

						if (abs(deriv3) >= Deriv3Threshold)
						{
							if (nLeftIndex >= f)
							{
								nLeftIndex = f - 2;
							}
							if (nRightIndex <= f)
							{
								nRightIndex = f + 2;
							}
						}
					}

					ClickLength = (nRightIndex - nLeftIndex + 1) & ~1;

					if (ClickLength < m_nMaxClickLength)
					{
						if (ClickLength < m_nMinClickLength)
						{
							ClickLength = m_nMinClickLength;
							nLeftIndex = nMaxDeriv3Pos - ClickLength/2;
						}

						nRightIndex = nLeftIndex + ClickLength;
						ClickFound = TRUE;
					}
					else
					{
						// too wide clicks are ignored
						//ClickLength = m_nMaxClickLength;
						//nLeftIndex = nMaxDeriv3Pos - ClickLength/2;
					}

					pCh->m_NextPossibleClickPosition = PrevIndex + nRightIndex + 2 * ClickLength;
				}

				if (ClickFound && m_pOutClicksFile)
				{
					if (0 == ch)
					{
						fprintf(m_pOutClicksFile, "%d %d 0\n",
								PrevIndex + nLeftIndex - ANALYZE_LAG, ClickLength);
					}
					else
					{
						fprintf(m_pOutClicksFile, "%d 0 %d\n",
								PrevIndex + nLeftIndex - ANALYZE_LAG, ClickLength);
					}
				}

				if (ClickFound && ! (m_bLogClicksOnly && m_bLogClicks))
				{
					InterpolateGap(pCh->m_prev, nLeftIndex, ClickLength,
									ClickLength > 16);
				}

			}
			// output is delayed by 64 samples
			if (PrevIndex > ANALYZE_LAG*2-2)
			{
#if 1
				pOutBuf[nStoreIndex * NumChannels] =
					WAVE_SAMPLE(pCh->m_prev[1 - ANALYZE_LAG /*m_nStoredSamples + nStoreIndex - PrevIndex + ANALYZE_LAG*/]);
#else
				// store 3rd derivative
				pOutBuf[nStoreIndex * NumChannels] =
					WAVE_SAMPLE(pCh->m_prev3[1 - ANALYZE_LAG]);
#endif
				nStoreIndex++;
			}

			pCh->m_prev.Advance();
			pCh->m_prev3.Advance();
			PrevIndex++;
		}
	}

	m_PrevIndex = PrevIndex;
	m_nStoredSamples += nStoreIndex;

	*pUsedBytes += nSamples * SampleSize;
	return nSavedBytes + SampleSize * nStoreIndex;
}

BOOL CClickRemoval::LoadClickSourceFile(LPCTSTR szFilename)
{
	m_PredefinedClicks.clear();

	FILE * const pInClicksFile = _tfopen(szFilename, _T("rt"));

	if (NULL == pInClicksFile)
	{
		return FALSE;
	}
	// read the file

	// set allocation increment in 256 items
	m_PredefinedClicks.reserve(256);

	char line[256] = {0};

	while (NULL != fgets(line, 255, pInClicksFile))
	{
		StoredClickData data;
		unsigned pos = 0, length_r = 0, length_l = 0;
		// every line contains 3 numbers: click position (in samples),
		//      Click length in the left channel, click length in the right
		if ( 2 > sscanf(line, "%d %d %d", &pos, &length_l, &length_r))
		{
			continue;
		}

		data.Position = pos;
		data.Length[0] = short(length_l);
		data.Length[1] = short(length_r);

		m_PredefinedClicks.push_back(data);
	}

	if ( ! m_PredefinedClicks.empty())
	{
		// merge duplicates
		for (ClicksVectorIterator i1 = m_PredefinedClicks.begin(), i2 = i1 + 1;
			i2 != m_PredefinedClicks.end(); i2++)
		{
			if (i1->Position == i2->Position)
			{
				// merge
				if (i1->Length[0] < i2->Length[0])
				{
					i1->Length[0] = i2->Length[0];
				}

				if (i1->Length[1] < i2->Length[1])
				{
					i1->Length[1] = i2->Length[1];
				}
			}
			else
			{
				i1++;
			}
		}

		m_PredefinedClicks.erase(i1 + 1, m_PredefinedClicks.end());
		std::sort(m_PredefinedClicks.begin(), m_PredefinedClicks.end());
	}

	fclose(pInClicksFile);
	return TRUE;
}

BOOL CClickRemoval::SetClickLogFile(LPCTSTR szFilename)
{
	if (m_pOutClicksFile)
	{
		fclose(m_pOutClicksFile);
		m_pOutClicksFile = NULL;
	}

	if (NULL != szFilename && szFilename[0] != 0)
	{
		m_pOutClicksFile = _tfopen(szFilename, _T("wt"));
	}
	return m_pOutClicksFile != 0;
}

////////////////////////////////////////////////////////////////
///////////// SIGNAL_PARAMS
struct SIGNAL_PARAMS
{
	// current and previous samples are stored. Result of current analysis
	// is used for the previous sample
	typedef NoiseReductionCore::DATA DATA;

	std::complex<DATA> sp_FftIn[2];
	//complex<DATA> sp_PrevFftOut;
	void AnalyzeFftSample(std::complex<DATA> smp, NoiseReductionCore * pNr, int nSample);
	// average frequency and amplitude in the band
	float sp_AvgFreq; // filtered arg(x[n] / x[n-1])
	float sp_AvgPhase;
	// AvgLevelChange = filter(log (abs(x[n]) / abs(x[n-1])))
	float sp_AvgLevelChange;
	//float sp_AvgLevel;
	// FilteredLevel = sum (AvgLevelChange)
	float sp_FilteredLevel;
	// frequency and amplitude deviation in band
	float sp_FilteredFreqError;
	float sp_FilteredLevelError;
	float sp_FreqDev;
	float sp_LevelDev;
	float sp_PrevInstantLevel;
	float sp_PrevPhase; // previous phase
	float sp_Freq;  // current frequency
	float sp_Power;     // current power
	float sp_MaskingPower; // masking function
	float sp_PrevMaskingPower;
	BOOL m_TonalBand;

	SIGNAL_PARAMS()
		: sp_AvgFreq(0.f)
		, sp_AvgPhase(0.f)
		, sp_AvgLevelChange(0.f)
		//, sp_AvgLevel(0.f)
		, sp_FilteredLevel(0.f)
		, sp_FilteredFreqError(0.f)
		, sp_FilteredLevelError(0.f)
		, sp_FreqDev(0.f)
		, sp_LevelDev(0.f)
		, sp_PrevInstantLevel(0.f)
		, sp_PrevPhase(0.f)
		, sp_Freq(0.f)
		, sp_Power(0.f)
		, sp_MaskingPower(0.f)
		, sp_PrevMaskingPower(0.f)
		, m_TonalBand(false)
	{
		sp_FftIn[0] = DATA(0.);
		sp_FftIn[1] = DATA(0.);
	}
};

// smp - source FFT sample
// pNr - points to the parent CNoiseReduction object
// nSample - number of the sample in FFT set
void SIGNAL_PARAMS::AnalyzeFftSample(complex<DATA> smp, NoiseReductionCore * pNr, int nSample)
{
	// find momentary frequency
	complex<DATA> cZero(0., 0.);
	double nrm = pNr->m_PowerScale *
				(real(smp) * real(smp) + imag(smp) * imag(smp));
	sp_Power = float(nrm);
	sp_FftIn[1] = sp_FftIn[0];
	sp_FftIn[0] = smp;

	if (cZero  == sp_FftIn[1]
		//|| cZero == sp_PrevFftOut
		|| 0. == nrm)
	{
		m_TonalBand = FALSE;
		//sp_PrevFftOut = smp;
		sp_AvgFreq = 0;
		sp_AvgPhase = arg(smp);
		sp_PrevPhase = sp_AvgPhase;

		if (smp != cZero)
		{
			sp_FilteredLevel = float(log(abs(smp)));
		}
		else
		{
			sp_FilteredLevel = -16.;    // in Nepers
		}
		sp_PrevInstantLevel = sp_FilteredLevel;
		sp_FilteredFreqError = 0;
		sp_FilteredLevelError = 0;
		return;
	}

#ifdef _DEBUG
	pNr->m_TotalBandProcessed++;
#endif

#if 1
	float dLevel = float(log(nrm) * 0.5);
	float dPhase = (float)atan2(imag(smp), real(smp));

	float dLevelChange = dLevel - sp_PrevInstantLevel;
	sp_PrevInstantLevel = dLevel;
	float dFreq = dPhase - sp_PrevPhase;

	// odd samples have an additional pi of phase difference
	if (nSample & 1)
	{
		if (dFreq < 0)
		{
			dFreq += float(M_PI);
		}
		else
		{
			dFreq -= float(M_PI);
		}
	}
	sp_PrevPhase = dPhase;
	if (dFreq < -M_PI)
	{
		dFreq += float(M_PI * 2);
	}
	else if (dFreq > M_PI)
	{
		dFreq -= float(M_PI * 2);
	}
	sp_Freq = dFreq;
#else
	float dLevel = float(log(abs(smp)));
	float dPhase = (float)arg(smp);

	float dLevelChange = dLevel - sp_PrevInstantLevel;
	sp_PrevInstantLevel = dLevel;
	float dFreq = (float)arg(smp / sp_PrevFftIn);

#endif
#ifdef _DEBUG
	if (pNr->m_MinLevelInBand > dLevel)
		pNr->m_MinLevelInBand = dLevel;
	if (pNr->m_MaxLevelInBand < dLevel)
		pNr->m_MaxLevelInBand = dLevel;
#endif
	BOOL StationarySignal = FALSE;

#if 1
	// check if the signal may be considered stationary or transient
	if (1 && (dLevelChange > pNr->m_ThresholdOfTransient
			|| dLevelChange < -pNr->m_ThresholdOfTransient))
	{
		// transient signal
		m_TonalBand = FALSE;
#ifdef _DEBUG
		pNr->m_TransientBandFound++;
#endif
		// signal in transient area
		sp_AvgFreq = dFreq;
		sp_FilteredLevel = dLevel;
		sp_FilteredFreqError = 0;
		sp_FilteredLevelError = 0;
		sp_FreqDev -= sp_FreqDev * pNr->m_FreqDevDecayRate;
		sp_LevelDev -= pNr->m_LevelDevDecayRate * sp_LevelDev;

		sp_AvgPhase += sp_AvgFreq;
		if (sp_AvgPhase < -M_PI)
		{
			sp_AvgPhase += float(M_PI * 2);
		}
		else if (sp_AvgPhase > M_PI)
		{
			sp_AvgPhase -= float(M_PI * 2);
		}
		dPhase = sp_AvgPhase;
	}
	else
	{
		// stationary signal
		sp_AvgFreq +=
			pNr->m_AvgFreqDecayRate * (dFreq - sp_AvgFreq);
		sp_AvgPhase += sp_AvgFreq;
//        double PhaseError = dPhase - sp_AvgPhase;
		double FreqError = dFreq - sp_AvgFreq;
#if 0
		sp_FilteredFreqError += float(
									pNr->m_FreqErrorDecayRate * (FreqError - sp_FilteredFreqError));
		sp_FreqDev += float((sp_FilteredFreqError * sp_FilteredFreqError - sp_FreqDev)
							* pNr->m_FreqDevDecayRate);
#else
		sp_FreqDev += float((FreqError * FreqError - sp_FreqDev)
							* pNr->m_FreqDevDecayRate);
#endif
		// if sp_FreqDev is greater than threshold, then the signal is noise-like
		sp_AvgPhase += sp_AvgFreq + sp_FilteredFreqError;

		if (sp_AvgPhase < -M_PI)
		{
			sp_AvgPhase += float(M_PI * 2);
		}
		else if (sp_AvgPhase > M_PI)
		{
			sp_AvgPhase -= float(M_PI * 2);
		}

		sp_AvgLevelChange +=
			pNr->m_AvgLevelDecayRate * (dLevelChange - sp_AvgLevelChange);
		sp_FilteredLevel +=
			pNr->m_AvgLevelDecayRate * (dLevel - sp_FilteredLevel);
		double LevelError = dLevel - sp_FilteredLevel;
		sp_FilteredLevelError += float(pNr->m_LevelErrorDecayRate *
										(LevelError - sp_FilteredLevelError));
		sp_LevelDev += pNr->m_LevelDevDecayRate *
						(sp_FilteredLevelError * sp_FilteredLevelError - sp_LevelDev);

		if (sp_FreqDev > pNr->m_FreqThresholdOfNoiselike)
		{
			m_TonalBand = FALSE;
		}
		else
		{
			m_TonalBand = TRUE;
#ifdef _DEBUG
			pNr->m_PhaseFilteredInBands++;
#endif
			StationarySignal = TRUE;
#if 0
			dPhase = sp_AvgPhase;
			dLevel = sp_FilteredLevel + sp_FilteredLevelError;
#endif
		}

	}

	//sp_PrevFftOut = std::polar(float(exp(dLevel)), dPhase);
#endif
	return;
}

struct NoiseReductionChannelData
{
	typedef NoiseReductionCore::DATA DATA;
	enum { FAR_MASKING_GRANULARITY = NoiseReductionCore::FAR_MASKING_GRANULARITY, };

	NoiseReductionCore * const pNr;

	unsigned const m_FftOrder;            // number of frequency bands (number of samples is double of that)
	int m_nSamplesReceived;         // total samples received in ProcessSoundBuffer
	int m_nSamplesStored;           // total samples stored in ProcessSoundBuffer
	BOOL m_PrerollSamplesSaved;
	BOOL m_bPassThrough;        // pass the data through without change
	int m_FftResultsProcessed;
	// when all the processing is done, they should be the same

	// FIFO to keep input samples
	RingBufferA<DATA> InputDataBuffer;
	RingBufferA<DATA> OutputDataBuffer;
	RingBufferA<DATA> PassThroughBuffer;
	// pointer to array
	// for accumulating output result
	float * m_AccumBuffer;
	DATA * m_FftInBuffer;
	std::complex<DATA> * m_FftOutBuffer;
	SIGNAL_PARAMS * m_pParams;

	NoiseReductionChannelData(NoiseReductionCore * nr, int FftOrder, BOOL PassThrough);
	~NoiseReductionChannelData();

	// return all samples left in the buffers
	// returns number of samples drained
	int FlushSamples(DATA * pBuf, int nOutSamples, int nChannels);
	int FillInBuffer(WAVE_SAMPLE const * pBuf, int nInSamples, int nChannels);
	int DrainOutBuffer(DATA * pBuf, int nOutSamples, int nChannels);
	void ResetOutBuffer();

	void Reset()
	{
		InputDataBuffer.Purge();
		OutputDataBuffer.Purge();
		PassThroughBuffer.Purge();
		for (unsigned i = 0; i <= m_FftOrder; i++)
		{
			m_pParams[i] = SIGNAL_PARAMS();
		}
		m_nSamplesReceived = 0;
		m_nSamplesStored = 0;
		m_PrerollSamplesSaved = FALSE;
		m_FftResultsProcessed = 0;
	}

	void ProcessInputFft();
	void AnalyzeInputFft();
	void AccumulateSubbandPower(float SubbandPower[FAR_MASKING_GRANULARITY]);

	void ApplyFarMasking(float FarMasking[FAR_MASKING_GRANULARITY]);

	void CalculateMasking(double MaskingSpectralDecayNormLow,
						double MaskingSpectralDecayNormHigh, double ToneEmphasis);

	void ProcessMaskingTemporalEnvelope(double MaskingTemporalDecayNormLow,
										double MaskingTemporalDecayHigh, unsigned MinFrequencyBandToProcess);
	void ProcessInverseFft();

	void AdjustFftBands(float const * pNoiseFloor, double SuppressionLimit);

	bool CanProcessFft() const
	{
		return InputDataBuffer.AvailableToRead() >= m_FftOrder * 2
				&& OutputDataBuffer.AvailableToWrite() >= m_FftOrder;
	}
};

NoiseReductionParameters::NoiseReductionParameters()
	: m_MinFrequencyToProcess(3000.),
	m_AvgFreqDecayRate(0.1f),
	m_AvgLevelDecayRate(0.2f),
	m_FreqErrorDecayRate(0.2f),
	m_LevelErrorDecayRate(0.5),
	m_FreqDevDecayRate(0.3f),
	m_LevelDevDecayRate(0.2f),
	m_ThresholdOfTransient(2.),  // Nepers
	m_FreqThresholdOfNoiselike(float(M_PI * M_PI / (4. * 4))),
	m_LevelThresholdForNoiseLow(8.),   // Nepers
	m_LevelThresholdForNoiseHigh(8.),   // Nepers
//m_LevelThresholdForStationary(6.),   // Nepers
	m_MaxNoiseSuppression(2.),      // Nepers

	m_NearMaskingDecayDistanceHigh(500.f),
	m_NearMaskingDecayDistanceLow(30.f),
	m_NoiseReductionRatio(1.),
	m_FarMaskingLevelDb(-40.),
	m_NearMaskingDecayTimeHigh(40.),    // miliseconds
	m_NearMaskingDecayTimeLow(100.),    // miliseconds
	m_ToneOverNoisePreference(2.5)     // in Nepers

{
}

void NoiseReductionParameters::Dump(unsigned indent) const
{
	TRACE(" %*.sMinFrequencyToProcess=%f, AvgFreqDecayRate=%f, AvgLevelDecayRate=%f\n",
		indent, "", m_MinFrequencyToProcess, m_AvgFreqDecayRate, m_AvgLevelDecayRate);

	TRACE(" %*.sFreqErrorDecayRate=%f, LevelErrorDecayRate=%f\n",
		indent, "", m_FreqErrorDecayRate, m_LevelErrorDecayRate);

	TRACE(" %*.sFreqDevDecayRate=%f, LevelDevDecayRate=%f\n",
		indent, "", m_FreqDevDecayRate, m_LevelDevDecayRate);

	TRACE(" %*.sThresholdOfTransient=%f, FreqThresholdOfNoiselike=%f\n",
		indent, "", m_ThresholdOfTransient, m_FreqThresholdOfNoiselike);

	TRACE(" %*.sLevelThresholdForNoiseLow=%f, LevelThresholdForNoiseHigh=%f\n",
		indent, "", m_LevelThresholdForNoiseLow, m_LevelThresholdForNoiseHigh);

	TRACE(" %*.sToneOverNoisePreference=%f, NoiseReductionRatio=%f, MaxNoiseSuppression=%f\n",
		indent, "", m_ToneOverNoisePreference, m_NoiseReductionRatio, m_MaxNoiseSuppression);

	TRACE(" %*.sNearMaskingDecayDistanceHigh=%f, NearMaskingDecayDistanceLow=%f, NearMaskingDecayRate=%f\n",
		indent, "", m_NearMaskingDecayDistanceHigh, m_NearMaskingDecayDistanceLow, m_NearMaskingDecayRate);

	TRACE(" %*.sNearMaskingDecayTimeLow=%f, NearMaskingDecayTimeHigh=%f, FarMaskingLevelDb=%f\n\n",
		indent, "", m_NearMaskingDecayTimeLow, m_NearMaskingDecayTimeHigh, m_FarMaskingLevelDb);

}

/////////////////////////////////////////////////////////////////////////////
////////////////////////  NoiseReductionCore
NoiseReductionCore::NoiseReductionCore(int nFftOrder, NUMBER_OF_CHANNELS nChannels,
										long SampleRate, NoiseReductionParameters const & nr,
										CHANNEL_MASK ChannelsToProcess)
	: NoiseReductionParameters(nr)
	, m_nFftOrder(nFftOrder)
	, m_SampleRate(SampleRate)
	, m_nChannels(nChannels)
	, m_Window(nFftOrder * 2)
	, m_pNoiseFloor(nFftOrder + 1)
	, m_ChannelsToProcess(ChannelsToProcess)
#ifdef _DEBUG
	, m_TotalBandProcessed(0),
	m_TransientBandFound(0),
	m_PhaseFilteredInBands(0),
	m_StationaryBandCancelled(0),
	m_NoiselikeBandCancelled(0),
	m_MaxLevelInBand(-20),
	m_MinLevelInBand(20)
#endif
{
	NUMBER_OF_CHANNELS ch;
	for (ch = 0; ch < nChannels; ch++)
	{
		m_ChannelData[ch] = new NoiseReductionChannelData(this, nFftOrder, 0 == (ChannelsToProcess & (1 << ch)));
	}

	for ( ; ch < MAX_NUMBER_OF_CHANNELS; ch++)
	{
		m_ChannelData[ch] = NULL;
	}

	unsigned i;

	// calculate window (half-sine)
	for (i = 0; i < m_nFftOrder * 2; i++)
	{
		// sine window
		m_Window[i] = float(sin((i + 0.5) * M_PI / (m_nFftOrder * 2)));
	}

	// norm masking factor to make it independent of FFT order
	m_PowerScale = float(1. / m_nFftOrder);
	double MaskingFactor = 2 * m_PowerScale;
	//m_FarMaskingScale = MaskingFactor;

	for (i = 0; i < FAR_MASKING_GRANULARITY; i++)
	{
		m_FarMaskingCoeffs[i][0] = 0.;
		for (unsigned k = 0; k < FAR_MASKING_GRANULARITY; k++)
		{
			// power drops as reciprocal of the distance cube
			if (0 && 0 == i)
			{
				m_FarMaskingCoeffs[i][k] = 0.;
			}
			else if (k == i)
			{
				m_FarMaskingCoeffs[i][k] = float(MaskingFactor);
			}
			else
			{
				int n = k - i;
				if (n < 0) n = -n;
#if 1
				float x = 1.f / (n + 1);
				m_FarMaskingCoeffs[i][k] = float(MaskingFactor * x * 0.25/* * x * x*/);
#else
				m_FarMaskingCoeffs[i][k] = float(MaskingFactor * pow(2., n * -2. - 2.));

#endif
			}
		}
	}

	// calculate noise floor profile:
	unsigned const MinFrequencyBandToProcess = unsigned(m_MinFrequencyToProcess * m_nFftOrder * 2 / m_SampleRate);
	double const NoiseFloorDelta = exp((m_LevelThresholdForNoiseHigh - m_LevelThresholdForNoiseLow)
										/ (m_nFftOrder - MinFrequencyBandToProcess));

	double NoiseFloor = 32768. * 32768. * exp(m_LevelThresholdForNoiseLow /*+ m_MaxNoiseSuppression / m_NoiseReductionRatio*/);

	for (i = 0; i < m_nFftOrder && i < MinFrequencyBandToProcess; i++)
	{
		m_pNoiseFloor[i] = float(NoiseFloor * double(i) * i / (MinFrequencyBandToProcess * MinFrequencyBandToProcess));
	}

	for (; i <= m_nFftOrder; i++, NoiseFloor *= NoiseFloorDelta)
	{
		m_pNoiseFloor[i] = float(NoiseFloor);
	}
}

void NoiseReductionCore::Dump(unsigned indent) const
{
	NoiseReductionParameters::Dump(indent);
	if (0)
	{
		for (unsigned i = 0; i < m_nFftOrder; i += 4)
		{
			TRACE(" %*.sNF[%d]=%f, NF[%d]=%f, NF[%d]=%f, NF[%d]=%f,\n",
				indent, "", i, m_pNoiseFloor[i], i+1, m_pNoiseFloor[i+1],
				i+2, m_pNoiseFloor[i+2], i+3, m_pNoiseFloor[i+3]);
		}
		TRACE(" %*.sNF[%d]=%f\n\n",
			indent, "", m_nFftOrder, m_pNoiseFloor[m_nFftOrder]);

		for (unsigned i = 0; i < m_nFftOrder * 2; i += 4)
		{
			TRACE(" %*.sW[%d]=%f, W[%d]=%f, W[%d]=%f, W[%d]=%f,\n",
				indent, "", i, m_Window[i], i+1, m_Window[i+1],
				i+2, m_Window[i+2], i+3, m_Window[i+3]);
		}

		for (unsigned i = 0; i < FAR_MASKING_GRANULARITY; i++)
		{
			for (unsigned k = 0; k < FAR_MASKING_GRANULARITY; k+=4)
			{
				TRACE(" %*.sFM[%d][%d]=%f, FM[%d][%d]=%f, FM[%d][%d]=%f, FM[%d][%d]=%f,\n",
					indent, "", i, k, m_FarMaskingCoeffs[i][k], i, k+1, m_FarMaskingCoeffs[i][k+1],
					i, k+2, m_FarMaskingCoeffs[i][k+2], i, k+3, m_FarMaskingCoeffs[i][k+3]);
			}
		}
	}
}

void NoiseReductionCore::CalculateFarMasking(float SubbandPower[FAR_MASKING_GRANULARITY],
											float FarMasking[FAR_MASKING_GRANULARITY])
{
	for (int f = 0; f < FAR_MASKING_GRANULARITY; f++)
	{
		FarMasking[f] = 0.;
		for (int n = 0; n < FAR_MASKING_GRANULARITY; n++)
		{
			FarMasking[f] += SubbandPower[n] * m_FarMaskingCoeffs[f][n];
		}
		FarMasking[f] /= m_nChannels;
	}
}

// get noise masking
// channels data is interleaved in the dst buffer
void NoiseReductionCore::GetAudioMasking(DATA * pBuf)  // nChannels * FftOrder
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		DATA * pDst = pBuf + ch;
		NoiseReductionChannelData * pCh = m_ChannelData[ch];
		SIGNAL_PARAMS const * pParms = pCh->m_pParams;

		for (unsigned i = 0; i < m_nFftOrder; i++, pParms++, pDst += m_nChannels)
		{
			*pDst = pParms->sp_MaskingPower;
		}
	}
}

void NoiseReductionCore::GetNoiseThreshold(DATA * pBuf) // precomputed threshold, nChannels *FftOrder count
{
	for (unsigned i = 0; i < m_nFftOrder; i++)
	{
		for (int ch = 0; ch < m_nChannels; ch++, pBuf ++)
		{
			*pBuf = m_pNoiseFloor[i];
		}
	}
}

// FFT power of the source signal
void NoiseReductionCore::GetPowerInBands(DATA * pBuf)  // nChannels * FftOrder
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		NoiseReductionChannelData * pCh = m_ChannelData[ch];
		SIGNAL_PARAMS const * pParms = pCh->m_pParams;
		DATA * pDst = pBuf + ch;

		for (unsigned i = 0; i < m_nFftOrder; i++, pParms++, pDst += m_nChannels)
		{
			*pDst = pParms->sp_Power;
		}
	}
}
// FFT power of the output
void NoiseReductionCore::GetResultPowerInBands(DATA * pBuf)  // nChannels * FftOrder
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		NoiseReductionChannelData * pCh = m_ChannelData[ch];
		SIGNAL_PARAMS const * pParms = pCh->m_pParams;

		for (unsigned i = 0; i < m_nFftOrder; i++, pBuf++, pParms++)
		{
			*pBuf = pParms->sp_FftIn[0].real() * pParms->sp_FftIn[0].real() +
					pParms->sp_FftIn[0].imag() * pParms->sp_FftIn[0].imag();
		}
	}
}

NoiseReductionCore::~NoiseReductionCore()
{
	for (int ch = 0; ch < countof(m_ChannelData); ch++)
	{
		delete m_ChannelData[ch];
	}
}

bool NoiseReductionCore::CanProcessFft() const
{
	return m_ChannelData[0]->CanProcessFft();
}

int NoiseReductionCore::DrainOutBuffer(DATA * pBuf, int nOutSamples)
{
	int nSavedSamples = 0;
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		nSavedSamples = m_ChannelData[ch]->DrainOutBuffer(pBuf + ch, nOutSamples, m_nChannels);
	}
	return nSavedSamples;
}

void NoiseReductionCore::ResetOutBuffer()
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		m_ChannelData[ch]->ResetOutBuffer();
	}
}

int NoiseReductionCore::FlushSamples(DATA * pBuf, int nOutSamples)
{
	int FlushedSamples = 0;
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		FlushedSamples = m_ChannelData[ch]->FlushSamples(pBuf + ch, nOutSamples, m_nChannels);
	}
	return FlushedSamples;
}

int NoiseReductionCore::FillInBuffer(WAVE_SAMPLE const * pBuf, int nSamples)
{
	// fill input buffer
	int InputSamplesUsed = 0;
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		InputSamplesUsed = m_ChannelData[ch]->FillInBuffer(pBuf + ch, nSamples, m_nChannels);
	}

	return InputSamplesUsed;
}

void NoiseReductionCore::Reset()
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		m_ChannelData[ch]->Reset();
	}
}

void NoiseReductionCore::ProcessInputFft()
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		m_ChannelData[ch]->ProcessInputFft();
	}
}

void NoiseReductionCore::AnalyseFft()
{
	unsigned const MinFrequencyBandToProcess = unsigned(m_MinFrequencyToProcess * m_nFftOrder * 2 / m_SampleRate);

	double const SuppressionLimit = exp(-m_MaxNoiseSuppression);

	double const MaskingTemporalDecayNormHigh =
		// coeff to filter masking function in time
		std::max<double>(m_NearMaskingDecayTimeHigh * 0.001 * m_SampleRate / (m_nFftOrder * 2), 1.);

	double const MaskingTemporalDecayNormLow =
		// coeff to filter masking function in time
		//m_NearMaskingDecayTimeLow * 0.001 * m_SampleRate / (m_nFftOrder / 2);
		std::max<double>(m_NearMaskingDecayTimeLow * 0.001 * m_SampleRate / (m_nFftOrder * 2), 1.);

	double const MaskingSpectralDecayNormLow =
		// coeff to filter masking function in frequencies
		std::max<double>(m_NearMaskingDecayDistanceLow / m_SampleRate * (m_nFftOrder * 2), 1.);

	double const MaskingSpectralDecayNormHigh =
		// coeff to filter masking function in frequencies
		std::max<double>(m_NearMaskingDecayDistanceHigh / m_SampleRate * (m_nFftOrder * 2), 1.);

	double const ToneEmphasis = exp(m_ToneOverNoisePreference);

	int n, ch;

	float SubbandPower[FAR_MASKING_GRANULARITY];

	for (n = 0; n < countof(SubbandPower); n++)
	{
		SubbandPower[n] = 0.;
	}

	for (ch = 0; ch < m_nChannels; ch++)
	{
		NoiseReductionChannelData * pCh = m_ChannelData[ch];

		pCh->ProcessInputFft();
		pCh->AnalyzeInputFft();
		pCh->AccumulateSubbandPower(SubbandPower);
	}

	float FarMasking[FAR_MASKING_GRANULARITY];
	CalculateFarMasking(SubbandPower, FarMasking);

	for (ch = 0; ch < m_nChannels; ch++)
	{
		NoiseReductionChannelData * pCh = m_ChannelData[ch];
		pCh->ApplyFarMasking(FarMasking);
		// process FFT result

		// find vector deviation
		// check if the signal is noise-like or is narrow-band.
		// if FreqDev is less than PI/8, signal is narrow-band,


		// Just filter the power in frequency and time
		// add far masking value
		// filter in frequency in two directions
		// those are calculated from m_NearMaskingDecayTime* :

		pCh->CalculateMasking(MaskingSpectralDecayNormLow,
							MaskingSpectralDecayNormHigh, ToneEmphasis);

		pCh->ProcessMaskingTemporalEnvelope(MaskingTemporalDecayNormLow,
											MaskingTemporalDecayNormHigh, MinFrequencyBandToProcess);

		// post-process output data

		pCh->AdjustFftBands(& m_pNoiseFloor[0], SuppressionLimit);
	}
}

void NoiseReductionCore::ProcessInverseFft()
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		m_ChannelData[ch]->ProcessInverseFft();
	}
}

////////////////////////////////////////////////////////////////////////
///////////////  CNoiseReduction
CNoiseReduction::CNoiseReduction(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess, unsigned nFftOrder, NoiseReductionParameters const & nr)
	: m_pNrCore(NULL)
	, m_NrParms(nr)
	, m_FftOrder(nFftOrder)
{
	m_ChannelsToProcess = ChannelsToProcess;
	SetInputWaveformat(pWf);
}

CNoiseReduction::~CNoiseReduction()
{
	delete m_pNrCore;
}

void CNoiseReduction::Dump(unsigned indent) const
{
	CWaveProc::Dump(indent);
	if (NULL != m_pNrCore)
	{
		m_pNrCore->Dump(indent);
	}
}

BOOL CNoiseReduction::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	if (CWaveProc::SetInputWaveformat(pWf))
	{
		delete m_pNrCore;
		m_pNrCore = NULL;

		m_pNrCore = new NoiseReductionCore(m_FftOrder, pWf->nChannels, pWf->nSamplesPerSec, m_NrParms, m_ChannelsToProcess);
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

size_t CNoiseReduction::ProcessSoundBuffer(char const * pIn, char * pOut,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	*pUsedBytes = 0;
	NUMBER_OF_CHANNELS const nChans = m_InputFormat.NumChannels();
	unsigned const SampleSize = m_InputFormat.SampleSize();
	DATA tmp[256];

	unsigned nInSamples = nInBytes / SampleSize;
	unsigned nOutSamples = nOutBytes / SampleSize;
	unsigned nStoredSamples = 0;

	WAVE_SAMPLE const * pInBuf = (WAVE_SAMPLE const *) pIn;
	WAVE_SAMPLE * pOutBuf = (WAVE_SAMPLE *) pOut;

	if (NULL == pInBuf)
	{
		if (NULL == m_pNrCore)
		{
			return 0;
		}

		while (0 != nOutSamples)
		{
			int TmpSamples = m_pNrCore->FlushSamples(tmp, std::min(unsigned(countof(tmp) / nChans), nOutSamples));

			if (0 == TmpSamples)
			{
				break;
			}

			nOutSamples -= TmpSamples;
			nStoredSamples += TmpSamples;
			TmpSamples *= nChans;

			for (int i = 0; i < TmpSamples; i++, pOutBuf++)
			{
				*pOutBuf = DoubleToShort(tmp[i]);
			}
		}
		return nStoredSamples * SampleSize;
	}

	if (NULL == m_pNrCore)
	{
		*pUsedBytes = nInBytes;
		return 0;
	}

	// process the samples
	while (1)
	{
		int InputSamplesUsed = m_pNrCore->FillInBuffer(pInBuf, nInSamples);

		pInBuf += InputSamplesUsed * nChans;
		nInSamples -= InputSamplesUsed;
		*pUsedBytes += InputSamplesUsed * SampleSize;

		if (m_pNrCore->CanProcessFft())
		{
			// now we have enough samples to do FFT
			m_pNrCore->AnalyseFft();
			m_pNrCore->ProcessInverseFft();
		}

		// store the result
		int nSavedSamples = 0;
		while (0 != nOutSamples)
		{
			int TmpSamples = m_pNrCore->DrainOutBuffer(tmp, std::min(unsigned(countof(tmp) / nChans), nOutSamples));

			if (0 == TmpSamples)
			{
				break;
			}

			nOutSamples -= TmpSamples;
			nStoredSamples += TmpSamples;
			nSavedSamples += TmpSamples;

			TmpSamples *= nChans;
			for (int i = 0; i < TmpSamples; i++, pOutBuf++)
			{
				ASSERT(tmp[i] <= 32767. && tmp[i] >= -32768.);

				*pOutBuf = DoubleToShort(tmp[i]);
			}
		}

		if (0 == nSavedSamples && 0 == InputSamplesUsed)
		{
			// can do no more
			break;
		}
	}

	return SampleSize * nStoredSamples;
}

////////////////////////////////////////////
/////////////////  NoiseReductionChannelData
NoiseReductionChannelData::NoiseReductionChannelData(NoiseReductionCore * nr, int nFftOrder, BOOL PassThrough)
	: pNr(nr)
	, m_FftOrder(nFftOrder)
	, m_AccumBuffer(new float[nFftOrder])
	, m_FftInBuffer(new DATA[nFftOrder * 2])
	, m_FftOutBuffer(new complex<DATA>[nFftOrder + 1])
	, m_pParams(new SIGNAL_PARAMS[nFftOrder + 1])
	, m_nSamplesReceived(0)
	, m_nSamplesStored(0)
	, m_PrerollSamplesSaved(FALSE)
	, m_bPassThrough(PassThrough)
	, m_FftResultsProcessed(0)
{
	memset(m_AccumBuffer, 0, nFftOrder * (sizeof (float)));
	PassThroughBuffer.AllocateBuffer(nFftOrder * 2);
	InputDataBuffer.AllocateBuffer(nFftOrder * 2);
	OutputDataBuffer.AllocateBuffer(nFftOrder * 2);
}

NoiseReductionChannelData::~NoiseReductionChannelData()
{
	delete[] m_AccumBuffer;
	delete[] m_FftInBuffer;
	delete[] m_FftOutBuffer;
	delete[] m_pParams;
}

int NoiseReductionChannelData::FlushSamples(DATA * pBuf, int nOutSamples, int nChannels)
{
	int ReadFromOutBuffer = std::min((int) OutputDataBuffer.AvailableToRead(),
									m_nSamplesReceived - m_nSamplesStored);

	if (ReadFromOutBuffer > nOutSamples)
	{
		ReadFromOutBuffer = nOutSamples;
	}

	for (int i = 0; i < ReadFromOutBuffer; i++, pBuf += nChannels)
	{
		ASSERT(InputDataBuffer[0] <= 32767. && InputDataBuffer[0] >= -32768.);

		pBuf[0] = OutputDataBuffer.Read();
	}

	m_nSamplesStored += ReadFromOutBuffer;

	nOutSamples -= ReadFromOutBuffer;

	int ReadFromThroughBuffer = std::min(nOutSamples,
										(int) PassThroughBuffer.AvailableToRead());

	if (ReadFromThroughBuffer > m_nSamplesReceived - m_nSamplesStored)
	{
		ReadFromThroughBuffer = m_nSamplesReceived - m_nSamplesStored;
	}

	for (int i = 0; i < ReadFromThroughBuffer; i++, pBuf += nChannels)
	{
		pBuf[0] = PassThroughBuffer.Read();
	}

	m_nSamplesStored += ReadFromThroughBuffer;

	nOutSamples -= ReadFromThroughBuffer;

	int ReadFromInBuffer = std::min(nOutSamples,
									(int) InputDataBuffer.AvailableToRead());

	if (ReadFromInBuffer > m_nSamplesReceived - m_nSamplesStored)
	{
		ReadFromInBuffer = m_nSamplesReceived - m_nSamplesStored;
	}

	for (int i = 0; i < ReadFromInBuffer; i++, pBuf += nChannels)
	{
		pBuf[0] = InputDataBuffer.Read();
	}

	m_nSamplesStored += ReadFromInBuffer;

	return ReadFromOutBuffer + ReadFromThroughBuffer + ReadFromInBuffer;
}

int NoiseReductionChannelData::FillInBuffer(WAVE_SAMPLE const * pBuf, int nSamples, int nChannels)
{
	nSamples = std::min(nSamples, int(InputDataBuffer.AvailableToWrite()));

	for (int i = 0; i < nSamples; i++, pBuf += nChannels)
	{
		InputDataBuffer.Write(*pBuf);
	}

	m_nSamplesReceived += nSamples;

	return nSamples;
}

void NoiseReductionChannelData::ProcessInputFft()
{
	// process the current FFT
	float const * Window = & pNr->m_Window[0];
	ASSERT(InputDataBuffer.AvailableToRead() >= m_FftOrder * 2);

	for (unsigned n = 0; n < m_FftOrder * 2; n++)
	{
		m_FftInBuffer[n] = Window[n] * InputDataBuffer[n];
		ASSERT(m_FftInBuffer[n] <= 32767. && m_FftInBuffer[n] >= -32768.);
	}

	ASSERT(PassThroughBuffer.AvailableToWrite() >= m_FftOrder);

	for (unsigned n = 0; n < m_FftOrder; n++)
	{
		PassThroughBuffer.Write(InputDataBuffer[n]);
	}

	if ( ! m_PrerollSamplesSaved
		&& PassThroughBuffer.AvailableToRead() >= m_FftOrder * 2)
	{
		for (unsigned f = 0; f < m_FftOrder; f++)
		{
			ASSERT(PassThroughBuffer[f] <= 32767. && PassThroughBuffer[f] >= -32768.);

			OutputDataBuffer.Write(PassThroughBuffer[f]);
			m_AccumBuffer[f] = Window[f + m_FftOrder] * PassThroughBuffer[f + m_FftOrder];
		}

		PassThroughBuffer.Discard(m_FftOrder);
		m_PrerollSamplesSaved = TRUE;
	}

	InputDataBuffer.Discard(m_FftOrder);

	FastFourierTransform(m_FftInBuffer, m_FftOutBuffer, m_FftOrder * 2);
}

void NoiseReductionChannelData::AnalyzeInputFft()
{
	for (unsigned f = 0; f <= m_FftOrder; f++)
	{
		m_pParams[f].AnalyzeFftSample(m_FftOutBuffer[f], pNr, f);
	}
}

void NoiseReductionChannelData::AccumulateSubbandPower(float SubbandPower[FAR_MASKING_GRANULARITY])
{
	int const BandsPerMaskGranule = m_FftOrder / FAR_MASKING_GRANULARITY;
	SIGNAL_PARAMS * p = m_pParams;

	for (int n = 0; n < FAR_MASKING_GRANULARITY; n++)
	{
		for (int k = 0; k < BandsPerMaskGranule; k++, p++)
		{
			SubbandPower[n] += p->sp_Power;
		}
	}
}

// saves samples from the FIFO to the output buffer
// returns number of saved samples
int NoiseReductionChannelData::DrainOutBuffer(DATA * pBuf, int nOutSamples, int nChannels)
{
	int nSamples = std::min(nOutSamples, int(OutputDataBuffer.AvailableToRead()));

	for (int i = 0; i < nSamples; i++, pBuf += nChannels)
	{
		ASSERT(OutputDataBuffer[0] <= 32767. && OutputDataBuffer[0] >= -32768.);

		pBuf[0] = OutputDataBuffer.Read();
	}
	m_nSamplesStored += nSamples;

	return nSamples;
}

void NoiseReductionChannelData::ResetOutBuffer()
{
	OutputDataBuffer.Purge();
	PassThroughBuffer.Purge();
}

void NoiseReductionChannelData::ProcessInverseFft()
{
	// Because FFT output is delayed by one round,
	// we need to skip the very first result, which should be all zeros,
	// and the next result, which is the first FFT window actually processed, already written to the output
	if (m_FftResultsProcessed >= 2)
	{
		if ( ! m_bPassThrough)
		{
			// perform inverse transform
			FastInverseFourierTransform(m_FftOutBuffer, m_FftInBuffer, m_FftOrder * 2);
			float const * Window = & pNr->m_Window[0];

			// add the processed data back to the output buffer
			for (unsigned f = 0; f < m_FftOrder; f++)
			{
				DATA tmp = Window[f] * m_FftInBuffer[f] + m_AccumBuffer[f];

				ASSERT(tmp <= 32767. && tmp >= -32768.);

				OutputDataBuffer.Write(tmp);

				m_AccumBuffer[f] = Window[f + m_FftOrder] * m_FftInBuffer[f + m_FftOrder];
			}
		}
		else
		{
			for (unsigned f = 0; f < m_FftOrder; f++)
			{
				OutputDataBuffer.Write(PassThroughBuffer[f]);
			}
		}
		PassThroughBuffer.Discard(m_FftOrder);
	}
	m_FftResultsProcessed++;
}

void NoiseReductionChannelData::AdjustFftBands(float const * pNoiseFloor, double SuppressionLimit)
{

#ifdef _DEBUG
	// compute statistics:
	// total power (original and after processing)
	// max power in band, min power in band,
	// max masking, min masking
	if (0) {
		double TotalPower1=0, TotalPower2=0;
		double MaxBandPower1=0, MaxBandPower2=0;
		double MinBandPower1=1.e10,MinBandPower2=1.e10;

		for (unsigned f = 2; f < m_FftOrder - 1; f++)
		{
			TotalPower1 += m_pParams[f].sp_Power;
			TotalPower2 += m_pParams[f].sp_MaskingPower;
			if (MaxBandPower1 < m_pParams[f].sp_Power)
			{
				MaxBandPower1 = m_pParams[f].sp_Power;
			}
			if (MaxBandPower2 < m_pParams[f].sp_MaskingPower)
			{
				MaxBandPower2 = m_pParams[f].sp_MaskingPower;
			}
			if (MinBandPower1 > m_pParams[f].sp_Power)
			{
				MinBandPower1 = m_pParams[f].sp_Power;
			}
			if (MinBandPower2 > m_pParams[f].sp_MaskingPower)
			{
				MinBandPower2 = m_pParams[f].sp_MaskingPower;
			}
		}

		TRACE("Original power: %g (%f dB), masking power: %g (%f dB)\n",
			TotalPower1, 10.*log10(TotalPower1), TotalPower2, 10.*log10(TotalPower2));
		TRACE("Max pwr in band: %g (%f dB), Min pwn in band: %g (%f dB)\n",
			MaxBandPower1, 10.*log10(MaxBandPower1), MinBandPower1, 10.*log10(MinBandPower1));
		TRACE("Max mask pwr in band: %g (%f dB), Min mask pwn in band: %g (%f dB)\n",
			MaxBandPower2, 10.*log10(MaxBandPower2), MinBandPower2, 10.*log10(MinBandPower2));
	}
	if (0)
	{
		double MinFreqDev = 10., MaxFreqDev = 0., AvgFreqDev = 0;
		int TonalBands = 0;
		for (unsigned f = 0; f < m_FftOrder - 1; f++)
		{
			AvgFreqDev += m_pParams[f].sp_FreqDev;
			if (MinFreqDev > m_pParams[f].sp_FreqDev)
			{
				MinFreqDev = m_pParams[f].sp_FreqDev;
			}
			if (MaxFreqDev < m_pParams[f].sp_FreqDev)
			{
				MaxFreqDev = m_pParams[f].sp_FreqDev;
			}
			if (m_pParams[f].m_TonalBand)
			{
				TonalBands++;
			}
		}
		TRACE("Min Freq Dev =%g, Max Freq Dev=%g, avg=%g, tonal bands=%d\n",
			MinFreqDev, MaxFreqDev, AvgFreqDev / (m_FftOrder),
			TonalBands);
	}
#endif

	SIGNAL_PARAMS * p = m_pParams;

	for (unsigned f = 0; f <= m_FftOrder; f++, p++)
	{
		if (p->sp_MaskingPower < pNoiseFloor[f])
		{
			double suppress = pow( double(p->sp_MaskingPower / pNoiseFloor[f]),
									double(pNr->m_NoiseReductionRatio));

			if (suppress < SuppressionLimit)
			{
				suppress = SuppressionLimit;
			}
			m_FftOutBuffer[f] = float(suppress) * p->sp_FftIn[1];
		}
		else
		{
			m_FftOutBuffer[f] = p->sp_FftIn[1];
		}
	}

#if 0 && defined(_DEBUG)
	p = m_pParams;
	for (unsigned f = 0; f <= m_FftOrder; f++, p++)
	{
		if (p->sp_MaskingPower < pNoiseFloor[f])
		{
			double suppress = pow( double(p->sp_MaskingPower / pNoiseFloor[f]),
									double(pNr->m_NoiseReductionRatio));

			if (suppress < SuppressionLimit)
			{
				suppress = SuppressionLimit;
			}
			m_FftOutBuffer[f] = float(suppress) * p->sp_FftIn[1];
		}
		else
		{
			m_FftOutBuffer[f] = p->sp_FftIn[1];
		}
	}
#endif
}

void NoiseReductionChannelData::ApplyFarMasking(float FarMasking[FAR_MASKING_GRANULARITY])
{
	// calculate fine masking function, using far masking table
	// and near masking factors.
	double const FarMaskingFactor = exp(pNr->m_FarMaskingLevelDb * DB_TO_NEPER);

	int const BandsPerMaskGranule = m_FftOrder / FAR_MASKING_GRANULARITY;

	SIGNAL_PARAMS * p = m_pParams;

	for (int n = 0; n < FAR_MASKING_GRANULARITY; n++)
	{
		for (int k = 0; k < BandsPerMaskGranule; k++, p++)
		{
			p->sp_MaskingPower = float(p->sp_Power + FarMaskingFactor * FarMasking[n]);
		}
	}
	// last FFT term
	p->sp_MaskingPower = float(p->sp_Power + FarMaskingFactor * FarMasking[FAR_MASKING_GRANULARITY - 1]);
}

void NoiseReductionChannelData::CalculateMasking(double MaskingSpectralDecayNormLow,
												double MaskingSpectralDecayNormHigh, double ToneEmphasis)
{
	double const MaskingDistanceDelta =
		(MaskingSpectralDecayNormHigh - MaskingSpectralDecayNormLow) / (m_FftOrder);

	SIGNAL_PARAMS * p = m_pParams + m_FftOrder;

	double PrevFilteredPower = p->sp_MaskingPower;

	for (signed f = m_FftOrder; f >= 0; f--, p--)    // f must be signed here
	{
		double power;
		if (p->m_TonalBand)
		{
			power = p->sp_MaskingPower * ToneEmphasis;
		}
		else
		{
			power = p->sp_MaskingPower;
		}

		PrevFilteredPower += (power - PrevFilteredPower)
							/ MaskingSpectralDecayNormHigh;

		p->sp_MaskingPower = float(PrevFilteredPower);

		MaskingSpectralDecayNormHigh -= MaskingDistanceDelta;
	}

	p = m_pParams;
	for (unsigned f = 0; f <= m_FftOrder; f++, MaskingSpectralDecayNormLow += MaskingDistanceDelta, p++)
	{
		PrevFilteredPower += (p->sp_MaskingPower - PrevFilteredPower) / MaskingSpectralDecayNormLow;
		p->sp_MaskingPower = float(PrevFilteredPower);
	}

}

void NoiseReductionChannelData::ProcessMaskingTemporalEnvelope(double MaskingTemporalDecayNormLow,
																double MaskingTemporalDecayNormHigh,
																unsigned MinFrequencyBandToProcess)
{
	// filter in time
	SIGNAL_PARAMS * p = m_pParams;
	double const DecayDelta =
		(MaskingTemporalDecayNormHigh - MaskingTemporalDecayNormLow) / (m_FftOrder - MinFrequencyBandToProcess);

	for (unsigned f = 0; f <= m_FftOrder; f++, p++)
	{
		if (p->sp_MaskingPower < p->sp_PrevMaskingPower)
		{
			p->sp_PrevMaskingPower +=
				float((p->sp_MaskingPower - p->sp_PrevMaskingPower) / MaskingTemporalDecayNormLow);

			// limit the value decay, to avoid underflow effects (may cause significant slowdown)
			p->sp_MaskingPower = std::max(p->sp_PrevMaskingPower, 1E-30f);

		}
		else
		{
			// on attack, follow it more quickly (use high frequencies norm)
			p->sp_PrevMaskingPower +=
				float((p->sp_MaskingPower - p->sp_PrevMaskingPower) * MaskingTemporalDecayNormHigh);
		}

		if (f >= MinFrequencyBandToProcess)
		{
			MaskingTemporalDecayNormLow += DecayDelta;
		}
	}
}

////////////////////////////
/////////// CBatchProcessing

CBatchProcessing::~CBatchProcessing()
{
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		if (m_bAutoDeleteProcs)
		{
			delete m_Stages[i].Proc;
		}
		delete[] m_Stages[i].Buf;
	}
}

BOOL CBatchProcessing::WasClipped() const
{
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		if (m_Stages[i].Proc->WasClipped())
		{
			return true;
		}
	}
	return false;
}

double CBatchProcessing::GetMaxClipped() const
{
	double MaxClipped = 0;
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		double StageClip = m_Stages[i].Proc->GetMaxClipped();
		if (MaxClipped < StageClip)
		{
			MaxClipped = StageClip;
		}
	}
	return MaxClipped;
}

BOOL CBatchProcessing::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		if (FALSE == m_Stages[i].Proc->SetInputWaveformat(pWf))
			return FALSE;
		m_OutputFormat = m_Stages[i].Proc->GetOutputWaveformat();
		pWf = GetOutputWaveformat();
	}
	return TRUE;
}

size_t CBatchProcessing::ProcessSoundBuffer(char const * pIn, char * pOut,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	size_t nSavedBytes = 0;
	*pUsedBytes = 0;

	if (NULL != pIn)
	{
		// regular processing of input data
		if (0 == m_Stages.GetSize())
		{
			// just pass through
			size_t ToCopy = __min(nInBytes, nOutBytes);
			memcpy(pOut, pIn, ToCopy);
			* pUsedBytes = ToCopy;
			return ToCopy;
		}

		bool bDataWasProcessed;
		do
		{
			bDataWasProcessed = false;
			for (int i = 0; i < m_Stages.GetSize(); i++)
			{
				size_t nProcessedBytes;
				size_t nCurrInputBytes;
				size_t nCurrOutputBytes;
				char const * inbuf;
				char * outbuf;
				Item * pItem = & m_Stages[i];

				if (i != 0)
				{
					inbuf = pItem->Buf + pItem->BufGetIndex;
					nCurrInputBytes = pItem->BufPutIndex - pItem->BufGetIndex;
				}
				else
				{
					inbuf = pIn;
					nCurrInputBytes = nInBytes;
				}

				if (0 == nCurrInputBytes)
				{
					continue;
				}
				if (i < m_Stages.GetSize() - 1)
				{
					Item * pOutItem = & m_Stages[i + 1];
					outbuf = pOutItem->Buf + pOutItem->BufPutIndex;
					nCurrOutputBytes = IntermediateBufSize - pOutItem->BufPutIndex;
				}
				else
				{
					nCurrOutputBytes = nOutBytes;
					outbuf = pOut;
				}

				if (0 == nCurrOutputBytes)
				{
					continue;
				}
				int nOutputBytes= pItem->Proc->ProcessSound(inbuf, outbuf,
															nCurrInputBytes, nCurrOutputBytes, & nProcessedBytes);

				if (nOutputBytes!= 0
					|| nProcessedBytes != 0)
				{
					bDataWasProcessed = true;
				}

				if (i != 0)
				{
					pItem->BufGetIndex += nProcessedBytes;
					if (pItem->BufGetIndex == pItem->BufPutIndex)
					{
						pItem->BufGetIndex = 0;
						pItem->BufPutIndex = 0;
					}
				}
				else
				{
					pIn += nProcessedBytes;
					nInBytes -= nProcessedBytes;
					*pUsedBytes += nProcessedBytes;
				}

				if (i < m_Stages.GetSize() - 1)
				{
					m_Stages[i + 1].BufPutIndex += nOutputBytes;
				}
				else
				{
					pOut += nOutputBytes;
					nOutBytes -= nOutputBytes;
					nSavedBytes += nOutputBytes;
				}

			}
		} while (bDataWasProcessed);

	}
	else
	{
		// flush the data
		if (0 == m_Stages.GetSize())
		{
			*pUsedBytes = 0;
			return 0;
		}
		bool bDataWasProcessed;
		do
		{
			bDataWasProcessed = false;
			for (int i = 0; i < m_Stages.GetSize(); i++)
			{
				size_t nProcessedBytes;
				size_t nCurrInputBytes;
				size_t nCurrOutputBytes;
				char const * inbuf;
				char * outbuf;
				Item * pItem = & m_Stages[i];
				if (i != 0)
				{
					inbuf = pItem->Buf + pItem->BufGetIndex;
					nCurrInputBytes = pItem->BufPutIndex - pItem->BufGetIndex;
					if (0 == nCurrInputBytes)
					{
						inbuf = NULL;
					}
				}
				else
				{
					inbuf = NULL;
					nCurrInputBytes = 0;
				}

				if (i < m_Stages.GetSize() - 1)
				{
					Item * pOutItem = & m_Stages[i + 1];
					outbuf = pOutItem->Buf + pOutItem->BufPutIndex;
					nCurrOutputBytes = IntermediateBufSize - pOutItem->BufPutIndex;
				}
				else
				{
					nCurrOutputBytes = nOutBytes;
					outbuf = pOut;
				}

				if (0 == nCurrOutputBytes)
				{
					continue;
				}
				int nOutputBytes = pItem->Proc->ProcessSound(inbuf, outbuf,
															nCurrInputBytes, nCurrOutputBytes, & nProcessedBytes);

				if (nOutputBytes != 0
					|| nProcessedBytes != 0)
				{
					bDataWasProcessed = true;
				}

				if (i != 0)
				{
					pItem->BufGetIndex += nProcessedBytes;
					if (pItem->BufGetIndex == pItem->BufPutIndex)
					{
						pItem->BufGetIndex = 0;
						pItem->BufPutIndex = 0;
					}
				}

				if (i < m_Stages.GetSize() - 1)
				{
					m_Stages[i + 1].BufPutIndex += nOutputBytes;
				}
				else
				{
					pOut += nOutputBytes;
					nOutBytes-= nOutputBytes;
					nSavedBytes += nOutputBytes;
				}

			}
		} while (bDataWasProcessed);

	}
	return nSavedBytes;
}

// if input data is compressed and not sample-aligned, this could be 0
// it can be multiple of block size for compressed format
size_t CBatchProcessing::GetInputSampleSize() const
{
	if (m_Stages.IsEmpty())
	{
		return 0;
	}
	return m_Stages[0].Proc->GetInputSampleSize();
}

// if output data is compressed and not sample-aligned, this could be 0
// it can be multiple of block size for compressed format
size_t CBatchProcessing::GetOutputSampleSize() const
{
	if (m_Stages.IsEmpty())
	{
		return 0;
	}
	return m_Stages[m_Stages.GetSize() - 1].Proc->GetOutputSampleSize();
}

void CBatchProcessing::AddWaveProc(CWaveProc * pProc, int index)
{
	Item item;
	item.Proc = pProc;
	item.BufGetIndex = 0;
	item.BufPutIndex = 0;
	item.Buf = new char[IntermediateBufSize];

	if (-1 == index || index > m_Stages.GetSize())
	{
		index = m_Stages.GetSize();
	}
	m_Stages.InsertAt(index, item);
}

void CBatchProcessing::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		m_Stages[i].Proc->Dump(indent + 1);
	}
}

BOOL CBatchProcessing::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		if ( ! m_Stages[i].Proc->Init())
		{
			return FALSE;
		}
	}

	return TRUE;
}

void CBatchProcessing::DeInit()
{
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		m_Stages[i].Proc->DeInit();
	}
	BaseClass::DeInit();
}

NUMBER_OF_SAMPLES CBatchProcessing::GetInputNumberOfSamples() const
{
	if (m_Stages.IsEmpty())
	{
		return 0;
	}

	return m_Stages[0].Proc->GetInputNumberOfSamples();
}

NUMBER_OF_SAMPLES CBatchProcessing::GetOutputNumberOfSamples() const
{
	if (m_Stages.IsEmpty())
	{
		return 0;
	}

	return m_Stages[m_Stages.GetSize() - 1].Proc->GetOutputNumberOfSamples();
}

////////////////////////////////////////
//////////// CResampleFilter
CResampleFilter::CResampleFilter()
	: m_SrcBufUsed(0)
	, m_DstBufUsed(0)
	, m_DstBufSaved(0)
	, m_SrcBufFilled(0)
	, m_SrcFilterLength(0)
	, m_Phase(0)
	, m_InputPeriod(0x80000000)
	, m_OutputPeriod(0x80000000)
{
}

CResampleFilter::CResampleFilter(long OriginalSampleRate, long NewSampleRate,
								int FilterLength,
								NUMBER_OF_CHANNELS nChannels,
								BOOL KeepSamplesPerSec)
	: m_SrcBufUsed(0)
	, m_DstBufUsed(0)
	, m_DstBufSaved(0)
	, m_SrcBufFilled(0)
	, m_SrcFilterLength(0)
	, m_bUseInterpolatedFilter(FALSE)
	, m_Phase(0)
	, m_SamplesInFilter(1)
	, m_FilterArraySize(0)
	, m_RationalResampleFraction(0)
	, m_FilterIndex(0)
	, m_InputPeriod(0x80000000)
	, m_OutputPeriod(0x80000000)
{
	InitResample(OriginalSampleRate, NewSampleRate, FilterLength, nChannels, KeepSamplesPerSec);
}

CResampleFilter::~CResampleFilter()
{
}

double CResampleFilter::FilterWindow(double arg)
{
	double Window;
	double x = M_PI * (1. + 2 * arg);
	switch (WindowType)
	{
	case WindowTypeNuttall:
		return 0.355768 - 0.487396 * cos(x) + 0.144232 * cos(2 * x) - 0.012604 * cos(3 * x);
		break;

	case WindowTypeSquareSine:
	default:
		Window = 0.5 - 0.5 * cos(x);
		return Window * Window;
	}
}

double CResampleFilter::sinc(double arg)
{
	if (arg != 0.)
	{
		arg *= M_PI;
		return sin(arg) / arg;
	}
	else
	{
		return 1.;
	}
}

double CResampleFilter::ResampleFilterTap(double arg, double FilterLength)
{
	return sinc(arg * FilterLength) * FilterWindow(arg);
}

static unsigned long GreatestCommonFactor(unsigned long x1, unsigned long x2)
{
	ASSERT(0 != x2);
	ASSERT(0 != x1);
	while (1)
	{
		unsigned long const remainder = x1 % x2;
		if (0 == remainder)
		{
			return x2;
		}
		x1 = x2;
		x2 = remainder;
	}
}

void CResampleFilter::InitSlidingFilter(int FilterLength, unsigned long NumberOfFilterTables)
{
	m_bUseInterpolatedFilter = FALSE;
	// use fixed coefficients
	m_FilterArraySize = NumberOfFilterTables * m_SamplesInFilter;
	m_FilterTable.Allocate(m_FilterArraySize);

	double * p = m_FilterTable;
	signed InputOffset = 0;
	signed Accumulator = 0;
	double NumSincWaves = (FilterLength + 0.5) * 2;

	for (unsigned i = 0; i < NumberOfFilterTables; i++)
	{
		TRACE("i=%d, InputOffset=%d\n", i, InputOffset);

		for (unsigned j = 0; j < m_SamplesInFilter; j++, p++)
		{
			double arg = double(j + 0.5 +
					(InputOffset - double(i) * m_InputFormat.SampleRate() / m_EffectiveOutputSampleRate)) / m_SamplesInFilter - 0.5;
			*p = ResampleFilterTap(arg, NumSincWaves);

			if (0) TRACE("Filter[%d][%d]=%f\n", i, j, *p);
		}

		Accumulator += m_InputFormat.SampleRate();
		while (Accumulator > 0)
		{
			Accumulator -= m_EffectiveOutputSampleRate;
			InputOffset++;
		}
	}

	TRACE("After filter calculation: InputOffset=%d\n", InputOffset);

	ResetResample();

	// compute normalization coefficient
	// apply DC to the filter
	for (int i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 1.;
	}

	m_SrcBufFilled = SrcBufSize;
	FilterSoundResample();

	float Max = 0;
	for (unsigned i = 2; i < m_DstBufUsed; i++)
	{
		if (Max < m_pDstBuf[i])
		{
			Max = m_pDstBuf[i];
		}
	}

	for (unsigned i = 0; i < NumberOfFilterTables * m_SamplesInFilter; i++)
	{
		m_FilterTable[i] /= Max;
	}


	// prefill at 1/2 filter length
	for (int i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 0.;
	}

	m_SrcBufFilled = m_SamplesInFilter * m_InputFormat.NumChannels();

	ResetResample();
}

void CResampleFilter::InitSlidingInterpolatedFilter(int FilterLength)
{
	m_bUseInterpolatedFilter = TRUE;

	if (m_EffectiveOutputSampleRate >= m_InputFormat.SampleRate())
	{
		// upsampling.
		//
		double InputPeriod = 0x100000000i64 / (FilterLength + 1.);
		m_InputPeriod = unsigned __int32(InputPeriod);
		m_OutputPeriod = unsigned __int32(InputPeriod * m_InputFormat.SampleRate() / m_EffectiveOutputSampleRate);
	}
	else
	{
		// downsampling
		double OutputPeriod = 0x100000000i64 / (FilterLength + 1.);
		m_OutputPeriod = unsigned __int32(OutputPeriod);
		m_InputPeriod = unsigned __int32(OutputPeriod * m_EffectiveOutputSampleRate / m_InputFormat.SampleRate());
	}
	//TRACE("InputPeriod=%08x, OutputPeriod=%08x\n", m_InputPeriod, m_OutputPeriod);
#ifdef _DEBUG
	double MaxErr = 0;
#endif
	m_InterpolatedFilterTable.Allocate(ResampleFilterSize);
	// use sliding squared interpolation
	for (signed i = 0; i < ResampleFilterSize; i++)
	{
		double arg = (i + 0.5) / ResampleFilterSize - 0.5;
		double arg05 = (i + 1.0) / ResampleFilterSize - 0.5;
		double arg1 = (i + 1.5) / ResampleFilterSize - 0.5;

		double val = ResampleFilterTap(arg, FilterLength);
		double val05 = ResampleFilterTap(arg05, FilterLength);
		double val1 = ResampleFilterTap(arg1, FilterLength);

		double dif1 = val05 - val;
		double dif2 = val1 - val05;

		m_InterpolatedFilterTable[i].tap = val;
		m_InterpolatedFilterTable[i].deriv1 = (3. * dif1 - dif2) / 2. / (1 << (ResampleIndexShift - 1));
		m_InterpolatedFilterTable[i].deriv2 = (dif2 - dif1) / 2. / (1 << (ResampleIndexShift - 1)) / (1 << (ResampleIndexShift - 1));

		TRACE("[%03d] Window=%f, sinc=%f, Resample filter=%.9f, next extrapolated=%.9f\n",
			i, FilterWindow(arg), sinc(arg * FilterLength), m_InterpolatedFilterTable[i].tap,
			m_InterpolatedFilterTable[i].tap + (1 << ResampleIndexShift) *
			(m_InterpolatedFilterTable[i].deriv1 + (1 << ResampleIndexShift) * m_InterpolatedFilterTable[i].deriv2));

#ifdef _DEBUG
		if (i > 0)
		{
			double arg25 = (i + 0.75) / ResampleFilterSize - 0.5;
			double arg75 = (i + 1.25) / ResampleFilterSize - 0.5;

			double err = fabs(ResampleFilterTap(arg25, FilterLength) -
							(val + (1 << ResampleIndexShift) * 0.25 *
								(m_InterpolatedFilterTable[i].deriv1 + (1 << ResampleIndexShift) * 0.25 * m_InterpolatedFilterTable[i].deriv2)));
			if (MaxErr < err)
			{
				MaxErr = err;
			}

			err = fabs(ResampleFilterTap(arg75, FilterLength) -
						(val + (1 << ResampleIndexShift) * 0.75 *
							(m_InterpolatedFilterTable[i].deriv1 + (1 << ResampleIndexShift) * 0.75 * m_InterpolatedFilterTable[i].deriv2)));
			if (MaxErr < err)
			{
				MaxErr = err;
			}
		}
#endif
	}
#ifdef _DEBUG
	TRACE("Max err = %g\n", MaxErr);
#endif
	m_SrcFilterLength = int((0x100000000i64 + m_InputPeriod - 1) / m_InputPeriod);

	ResetResample();

	// compute normalization coefficient
	// apply DC to the filter
	for (int i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 1.;
	}

	m_SrcBufFilled = SrcBufSize;
	FilterSoundResample();

	float Max = 0;
	for (unsigned i = 2; i < m_DstBufUsed; i++)
	{
		if (Max < m_pDstBuf[i])
		{
			Max = m_pDstBuf[i];
		}
	}

	for (int i = 0; i < ResampleFilterSize; i++)
	{
		m_InterpolatedFilterTable[i].tap /= Max;
		m_InterpolatedFilterTable[i].deriv1 /= Max;
		m_InterpolatedFilterTable[i].deriv2 /= Max;
	}


	for (int i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 0.;
	}
	// prefill at 1/2 filter length
	m_SrcBufFilled = (0x80000000u / m_InputPeriod) * m_InputFormat.NumChannels();

	ResetResample();
}

BOOL CResampleFilter::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	ASSERT(CWaveFormat(pWf) == m_InputFormat);
	return TRUE;
}

void CResampleFilter::InitResample(long OriginalSampleRate, long NewSampleRate,
									int FilterLength, NUMBER_OF_CHANNELS nChannels, BOOL KeepSamplesPerSec)
{
	// FilterLength is how many Sin periods are in the array
	m_EffectiveOutputSampleRate = NewSampleRate;

	m_InputFormat.InitFormat(WAVE_FORMAT_PCM, OriginalSampleRate, nChannels);
	m_OutputFormat.InitFormat(WAVE_FORMAT_PCM,
							KeepSamplesPerSec ? OriginalSampleRate : NewSampleRate,
							nChannels);

	// find greatest common factor of the sampling rates
	unsigned long common = GreatestCommonFactor(NewSampleRate, OriginalSampleRate);
	// after this number of output samples, the filter coefficients repeat
	unsigned long NumberOfFilterTables = NewSampleRate / common;

	m_SamplesInFilter = (FilterLength + 1) * 2;

	if (NewSampleRate < OriginalSampleRate)
	{
		m_SamplesInFilter = MulDiv(m_SamplesInFilter, OriginalSampleRate, NewSampleRate);
	}

	if (NumberOfFilterTables * m_SamplesInFilter <= MaxNumberOfFilterSamples)
	{
		InitSlidingFilter(FilterLength, NumberOfFilterTables);
	}
	else
	{
		InitSlidingInterpolatedFilter(FilterLength);
	}
}

void CResampleFilter::ResetResample()
{
	m_SrcBufUsed = 0;
	m_DstBufUsed = 0;
	m_DstBufSaved = 0;
	m_RationalResampleFraction = 0;
	m_FilterIndex = 0;

	m_Phase = 0x80000000u % m_InputPeriod;
}

void CResampleFilter::DoSlidingInterpolatedFilterResample()
{
	unsigned const NumChannels = m_InputFormat.NumChannels();
	unsigned const FilterLength = NumChannels * m_SrcFilterLength;

	FilterCoeff const * const pTable = m_InterpolatedFilterTable;

	unsigned i;
	for (i = m_DstBufUsed; m_SrcBufFilled >= m_SrcBufUsed + FilterLength
		&& i < DstBufSize; i+= NumChannels)
	{
		const float * src = m_pSrcBuf + m_SrcBufUsed;
		unsigned __int32 Phase1 = m_Phase;
		double OutSample[MAX_NUMBER_OF_CHANNELS];
		for (unsigned ch = 0; ch < NumChannels; ch++)
		{
			OutSample[ch] = 0;
		}

		for (unsigned j = 0; ; j+= NumChannels)
		{
			ASSERT(src + j < m_pSrcBuf + SrcBufSize);

			int const TableIndex = Phase1 >> ResampleIndexShift;
			double PhaseFraction = int(Phase1 & ~(0xFFFFFFFF << ResampleIndexShift));

			double const coeff = (pTable[TableIndex].tap +
									PhaseFraction * (pTable[TableIndex].deriv1
										+ PhaseFraction * pTable[TableIndex].deriv2));

			for (unsigned ch = 0; ch < NumChannels; ch++)
			{
				OutSample[ch] += src[j + ch] * coeff;
			}

			unsigned __int32 Phase2 = Phase1 + m_InputPeriod;
			if (Phase2 < Phase1)
			{
				break;
			}
			Phase1 = Phase2;
		}

		for (unsigned ch = 0; ch < NumChannels; ch++)
		{
			m_pDstBuf[i + ch] = float(OutSample[ch]);
		}
		m_DstBufUsed += NumChannels;
		m_Phase -= m_OutputPeriod;

		while (m_Phase & 0x80000000)
		{
			m_Phase += m_InputPeriod;
			m_SrcBufUsed += NumChannels;
		}
	}
}

void CResampleFilter::DoSlidingFilterResample()
{
	unsigned const NumChannels = m_InputFormat.NumChannels();

	if (m_SrcBufFilled - m_SrcBufUsed <= NumChannels * m_SamplesInFilter)
	{
		return;
	}

	unsigned SrcSamples = m_SrcBufFilled - m_SrcBufUsed - NumChannels * m_SamplesInFilter;

	float * dst = m_pDstBuf + m_DstBufUsed;

	long const InputSampleRate = m_InputFormat.SampleRate();
	long const OutputSampleRate = m_EffectiveOutputSampleRate;

	size_t i;
	for (i = m_DstBufUsed;
		SrcSamples >= NumChannels && i < DstBufSize;
		i += NumChannels)
	{
		double const * p = m_FilterIndex + m_FilterTable;

		const float * src = m_pSrcBuf + m_SrcBufUsed;
		for (unsigned ch = 0; ch < NumChannels; ch++, dst++)
		{
			double OutSample = 0.;
			float const * FilterSrc = src + ch;

			for (unsigned j = 0; j != m_SamplesInFilter; j++, FilterSrc += NumChannels)
			{
				OutSample += *FilterSrc * p[j];
			}

			*dst = float(OutSample);
		}

		m_RationalResampleFraction += InputSampleRate;
		while (m_RationalResampleFraction > 0)
		{
			m_RationalResampleFraction -= OutputSampleRate;
			SrcSamples -= NumChannels;
			m_SrcBufUsed += NumChannels;
		}

		m_FilterIndex += m_SamplesInFilter;
		if (m_FilterIndex >= m_FilterArraySize)
		{
			m_FilterIndex = 0;
		}
	}
	m_DstBufUsed = i;
}

void CResampleFilter::FilterSoundResample()
{
	if (0 != m_bUseInterpolatedFilter)
	{
		DoSlidingInterpolatedFilterResample();
	}
	else
	{
		DoSlidingFilterResample();
	}
}

size_t CResampleFilter::ProcessSoundBuffer(char const * pIn, char * pOut,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	*pUsedBytes = 0;

	// raw samples, multiplied by number of channels
	unsigned nInSamples = nInBytes / sizeof (WAVE_SAMPLE);
	unsigned nOutSamples = nOutBytes / sizeof (WAVE_SAMPLE);

	WAVE_SAMPLE const * pInBuf = (WAVE_SAMPLE const*) pIn;
	WAVE_SAMPLE * pOutBuf = (WAVE_SAMPLE *) pOut;

	unsigned nUsedSamples = 0;
	unsigned nSavedSamples = 0;

	if (0 == pInBuf)
	{
		// adjust nOutSamples
		unsigned long MaxOutSamples =
			MulDiv(m_ProcessedInputSamples, m_EffectiveOutputSampleRate, m_InputFormat.SampleRate());

		if (MaxOutSamples <= (unsigned long)m_SavedOutputSamples)
		{
			nOutSamples = 0;
		}
		else
		{
			nOutSamples = std::min<unsigned long>(nOutSamples,
												(MaxOutSamples - m_SavedOutputSamples) * m_OutputFormat.NumChannels());
		}
	}

	while(nOutSamples != 0)
	{
		// move data in the internal buffer, if necessary

		if (m_SrcBufFilled < SrcBufSize)
		{
			unsigned ToCopy = std::min(SrcBufSize - m_SrcBufFilled, nInSamples);

			if (0 == pInBuf)
			{
				// fill the rest of the input buffer with zeros
				for (unsigned j = m_SrcBufFilled; j != SrcBufSize; j++)
				{
					m_pSrcBuf[j] = 0.f;
				}
				m_SrcBufFilled = SrcBufSize;
			}
			else
			{
				for (unsigned i = 0, j = m_SrcBufFilled; i != ToCopy; i++, j++)
				{
					m_pSrcBuf[j] = pInBuf[i];
				}

				m_SrcBufFilled += ToCopy;
				pInBuf += ToCopy;

				nInSamples -= ToCopy;
				nUsedSamples += ToCopy;
			}
		}

		FilterSoundResample();

		if (m_DstBufSaved == m_DstBufUsed)
		{
			m_DstBufSaved = 0;
			m_DstBufUsed = 0;

			if (m_SrcBufFilled != m_SrcBufUsed
				&& 0 != m_SrcBufUsed)
			{
				// move data in the buffer, to allow to give the resample more data
				unsigned i = 0;
				for (unsigned j = m_SrcBufUsed; j < m_SrcBufFilled; i++, j++)
				{
					m_pSrcBuf[i] = m_pSrcBuf[j];
				}
				m_SrcBufUsed = 0;
				m_SrcBufFilled = i;
				continue;
			}
			break;
		}

		unsigned const ToCopy = std::min(m_DstBufUsed - m_DstBufSaved, nOutSamples);

		for (unsigned i = 0, j = m_DstBufSaved; i != ToCopy; i++, j++)
		{
			pOutBuf[i] = DoubleToShort(m_pDstBuf[j]);
		}

		m_DstBufSaved += ToCopy;

		pOutBuf += ToCopy;
		nOutSamples -= ToCopy;
		nSavedSamples += ToCopy;

		if (m_DstBufSaved == m_DstBufUsed)
		{
			m_DstBufSaved = 0;
			m_DstBufUsed = 0;
		}
	}

	* pUsedBytes += nUsedSamples * sizeof pInBuf[0];
	return nSavedSamples * sizeof pOutBuf[0];
}

////////////////////////////////////////////////////////////////////
////////// CAudioConvertor
CAudioConvertor::CAudioConvertor(HACMDRIVER had)
	: m_AcmConvertor(had)
	, m_InputSampleSize(0)
	, m_OutputSampleSize(0)
{
}

CAudioConvertor::~CAudioConvertor()
{
}

BOOL CAudioConvertor::InitConversion(WAVEFORMATEX const * SrcFormat, WAVEFORMATEX const * DstFormat)
{
	if (WAVE_FORMAT_PCM == SrcFormat->wFormatTag)
	{
		ASSERT(NULL != DstFormat);

		if (SrcFormat->nChannels != DstFormat->nChannels
			|| SrcFormat->nSamplesPerSec != DstFormat->nSamplesPerSec)
		{
			// unable to convert
			return FALSE;
		}

		if ( ! m_AcmConvertor.Open(SrcFormat, DstFormat)
			|| ! m_AcmConvertor.AllocateBuffers(0x10000, 0))
		{
			return FALSE;
		}

		m_InputFormat.InitFormat(SrcFormat->wFormatTag, SrcFormat->nSamplesPerSec,
								SrcFormat->nChannels, SrcFormat->wBitsPerSample);

		m_InputSampleSize = SrcFormat->nBlockAlign;

		if (WAVE_FORMAT_PCM == DstFormat->wFormatTag)
		{
			m_OutputSampleSize = DstFormat->nBlockAlign;
			m_OutputFormat.InitFormat(DstFormat->wFormatTag, DstFormat->nSamplesPerSec,
									DstFormat->nChannels, DstFormat->wBitsPerSample);
		}
		else
		{
			m_OutputSampleSize = 0;
			m_OutputFormat = DstFormat;
		}
	}
	else if (WAVE_FORMAT_PCM == DstFormat->wFormatTag)
	{
		WAVEFORMATEX wf =
		{
			WAVE_FORMAT_PCM,
			SrcFormat->nChannels,
			0,  // nSamplesPerSec
			0,  // nAvgBytesPerSec
			0, // nBlockAlign
			16, // bits per sample
			0   // cbSize
		};

		m_AcmConvertor.SuggestFormat(SrcFormat, & wf, sizeof wf,
									ACM_FORMATSUGGESTF_NCHANNELS
									| ACM_FORMATSUGGESTF_WBITSPERSAMPLE
									| ACM_FORMATSUGGESTF_WFORMATTAG);

		if (TRACE_WAVEPROC) TRACE("acmFormatSuggest:nSamplesPerSec=%d, BytesPerSec=%d, nBlockAlign=%d\n",
								wf.nSamplesPerSec, wf.nAvgBytesPerSec, wf.nBlockAlign);

		if (wf.nChannels != DstFormat->nChannels
			|| wf.nSamplesPerSec != DstFormat->nSamplesPerSec)
		{
			// unable to convert
			return FALSE;
		}

		if ( ! m_AcmConvertor.Open(SrcFormat, & wf)
			|| ! m_AcmConvertor.AllocateBuffers(0, 0x10000))
		{
			return FALSE;
		}

		m_InputFormat = SrcFormat;
		m_OutputFormat = &wf;

		m_OutputSampleSize = m_OutputFormat.SampleSize();
		m_InputSampleSize = 0;
	}
	else
	{
		return FALSE;
	}

	m_ConvertFlags = ACM_STREAMCONVERTF_START | ACM_STREAMCONVERTF_BLOCKALIGN;
	return TRUE;
}

BOOL CAudioConvertor::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	ASSERT(CWaveFormat(pWf) == m_InputFormat);
	return TRUE;
}

size_t CAudioConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	size_t nSavedBytes = 0;
	*pUsedBytes = 0;

	while (nOutBytes != 0 || nInBytes != 0)
	{
		// empty the output buffer
		size_t const WasCopied = m_AcmConvertor.GetConvertedData(pOut, nOutBytes);

		nOutBytes -= WasCopied;
		pOut += WasCopied;
		nSavedBytes += WasCopied;

		if (0 == nOutBytes)
		{
			break;
		}

		if (NULL == pIn)
		{
			m_ConvertFlags |= ACM_STREAMCONVERTF_END;
			m_ConvertFlags &= ~ACM_STREAMCONVERTF_BLOCKALIGN;
		}
		// do the conversion
		size_t InUsed = 0;
		size_t OutUsed = 0;

		if ( ! m_AcmConvertor.Convert(pIn, nInBytes, & InUsed, NULL,
									& OutUsed, m_ConvertFlags))
		{
			// error
			nSavedBytes = 0;  // BUGBUG?
			break;
		}

		if (TRACE_WAVEPROC) TRACE("m_AcmConvertor.Convert InUsed=%d, OutUsed=%d\n", InUsed, OutUsed);

		if (0 == InUsed
			&& 0 == OutUsed)
		{
			break;
		}

		nInBytes -= InUsed;
		*pUsedBytes += InUsed;
		pIn += InUsed;

		m_ConvertFlags &= ~ACM_STREAMCONVERTF_START;
	}

	return nSavedBytes;
}

// if input data is compressed and not sample-aligned, this could be 0
size_t CAudioConvertor::GetInputSampleSize() const
{
	return m_InputSampleSize;
}

// if input data is compressed and not sample-aligned, this could be 0
size_t CAudioConvertor::GetOutputSampleSize() const
{
	return m_OutputSampleSize;
}

// if input sample size is known, return exact number of processed samples,
// otherwise return output number.
NUMBER_OF_SAMPLES CAudioConvertor::GetInputNumberOfSamples() const
{
	if (0 != m_InputSampleSize)
	{
		return BaseClass::GetInputNumberOfSamples();
	}
	return BaseClass::GetOutputNumberOfSamples();
}

// if output sample size is known, return exact number of produced samples,
// otherwise return input number.
NUMBER_OF_SAMPLES CAudioConvertor::GetOutputNumberOfSamples() const
{
	if (0 != m_OutputSampleSize)
	{
		return BaseClass::GetOutputNumberOfSamples();
	}
	return BaseClass::GetInputNumberOfSamples();
}

///////////////////////////////////////////////////////
//////////////// CChannelConvertor
CChannelConvertor::CChannelConvertor(NUMBER_OF_CHANNELS OldChannels,
									NUMBER_OF_CHANNELS NewChannels, CHANNEL_MASK ChannelsToProcess)
{
	m_OutputFormat.InitFormat(WAVE_FORMAT_PCM, 44100, NewChannels,
							16);
	m_InputFormat.InitFormat(WAVE_FORMAT_PCM, 44100, OldChannels,
							16);
	m_ChannelsToProcess = ChannelsToProcess;
}
BOOL CChannelConvertor::SetInputWaveformat(WAVEFORMATEX const * pWf)
{
	ASSERT(pWf->nChannels == m_InputFormat.NumChannels());
	if (pWf->nChannels != m_InputFormat.NumChannels())
	{
		return FALSE;
	}

	m_OutputFormat.InitFormat(pWf->wFormatTag, pWf->nSamplesPerSec, m_OutputFormat.NumChannels(),
							pWf->wBitsPerSample);
	m_InputFormat.InitFormat(pWf->wFormatTag, pWf->nSamplesPerSec, m_InputFormat.NumChannels(),
							pWf->wBitsPerSample);

	return TRUE;
}

size_t CChannelConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	size_t nSavedBytes = 0;
	*pUsedBytes = 0;

	WAVE_SAMPLE const * pInBuf = (WAVE_SAMPLE const *) pIn;
	WAVE_SAMPLE * pOutBuf = (WAVE_SAMPLE *) pOut;
	int nInSamples = nInBytes / m_InputFormat.SampleSize();
	int nOutSamples = nOutBytes / m_OutputFormat.SampleSize();

	int nSamples = std::min(nInSamples, nOutSamples);

	int i;
	if (2 == m_InputFormat.NumChannels()
		&& 1 == m_OutputFormat.NumChannels())
	{
		if (SPEAKER_FRONT_LEFT == m_ChannelsToProcess
			|| SPEAKER_FRONT_RIGHT == m_ChannelsToProcess)
		{
			if (SPEAKER_FRONT_RIGHT == m_ChannelsToProcess)
			{
				pInBuf++;
			}

			for (i = 0; i < nSamples; i++,
				pInBuf += 2, pOutBuf ++)
			{
				*pOutBuf = * pInBuf;
			}
		}
		else
		{
			for (i = 0; i < nSamples; i++,
				pInBuf += 2, pOutBuf += 1)
			{
				*pOutBuf = (pInBuf[0] + pInBuf[1]) / 2;
			}
		}
		nSavedBytes += i * sizeof pOutBuf[0];
		*pUsedBytes += i * 2 * sizeof pInBuf[0];
	}
	else if (1 == m_InputFormat.NumChannels()
			&& 2 == m_OutputFormat.NumChannels())
	{
		for (i = 0; i < nSamples; i++,
			pInBuf += 1, pOutBuf += 2)
		{
			pOutBuf[0] = * pInBuf;
			pOutBuf[1] = * pInBuf;
		}
		nSavedBytes += 2 * i * sizeof pOutBuf[0];
		*pUsedBytes += i * sizeof pInBuf[0];
	}
	else
	{
		return size_t(-1);
	}
	return nSavedBytes;
}

CLameEncConvertor::~CLameEncConvertor()
{
	DeInit();
}

BOOL CLameEncConvertor::SetFormat(WAVEFORMATEX const * pWF)
{
	m_Wf = pWF;
	return TRUE;
}

size_t CLameEncConvertor::ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	// save extra data from the output buffer
	*pUsedBytes = 0;
	size_t nSavedBytes = 0;
	BOOL FlushBuffer = FALSE;
	if (NULL == pInBuf)
	{
		nInBytes = 0;
		FlushBuffer = TRUE;
	}

	// copy data to the temp buffer
	while (0 != nInBytes
			|| 0 != m_InputBufferFilled
			|| 0 != m_OutputBufferFilled
			|| FlushBuffer)
	{
		if (0 != m_OutputBufferFilled)
		{
			size_t ToCopy = __min(nOutBytes, m_OutputBufferFilled);
			memcpy(pOutBuf, m_pOutputBuffer, ToCopy);

			m_OutputBufferFilled -= ToCopy;
			nOutBytes -= ToCopy;
			pOutBuf += ToCopy;
			nSavedBytes += ToCopy;

			if (0 != m_OutputBufferFilled)
			{
				memmove(m_pOutputBuffer, m_pOutputBuffer + ToCopy, m_OutputBufferFilled);
				return nSavedBytes;
			}
		}

		size_t ToCopy = m_InputBufferSize - m_InputBufferFilled;
		if (ToCopy > nInBytes)
		{
			ToCopy = nInBytes;
		}
		if (NULL != pInBuf)
		{
			memcpy(m_pInputBuffer + m_InputBufferFilled, pInBuf, ToCopy);
			*pUsedBytes += ToCopy;
			pInBuf += ToCopy;
			nInBytes -= ToCopy;
			m_InputBufferFilled += ToCopy;
		}
		if (m_InputBufferFilled == m_InputBufferSize
			|| (NULL == pInBuf && 0 != m_InputBufferFilled))
		{
			DWORD OutFilled = 0;
			m_Enc.EncodeChunk((short*)m_pInputBuffer,
							m_InputBufferFilled / (sizeof (short) * m_InputFormat.NumChannels()),
							m_pOutputBuffer, & OutFilled);
			m_OutputBufferFilled = OutFilled;
			m_InputBufferFilled = 0;    // all used up
			continue;
		}
		if (NULL != pInBuf)
		{
			break;
		}
		else
		{
			// flush data
			DWORD OutFilled = 0;
			m_Enc.FlushStream(m_pOutputBuffer, & OutFilled);
			m_OutputBufferFilled = OutFilled;
			FlushBuffer = FALSE;
		}
	}
	return nSavedBytes;
}

BOOL CLameEncConvertor::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}
	BE_CONFIG cfg;
	memzero(cfg);

	cfg.dwConfig = BE_CONFIG_LAME;
	cfg.format.LHV1.dwStructVersion = 1;
	cfg.format.LHV1.dwStructSize = sizeof cfg;
	cfg.format.LHV1.dwSampleRate = m_Wf.SampleRate();
	cfg.format.LHV1.nMode = BE_MP3_MODE_MONO;
	if (m_Wf.NumChannels() > 1)
	{
		cfg.format.LHV1.nMode = BE_MP3_MODE_STEREO;
	}
	cfg.format.LHV1.dwBitrate = m_Wf.BytesPerSec() / (1000 / 8);

	cfg.format.LHV1.bCRC = TRUE;

	if ( ! m_Enc.OpenStream( & cfg))
	{
		return FALSE;
	}
	// allocate buffers
	m_InputBufferSize = m_Enc.m_InBufferSize;
	m_InputBufferFilled = 0;
	m_pInputBuffer = new char[m_Enc.m_InBufferSize];

	m_OutputBufferSize = m_Enc.m_OutBufferSize;
	m_OutputBufferFilled = 0;
	m_pOutputBuffer = new BYTE[m_OutputBufferSize];

	return TRUE;
}

void CLameEncConvertor::DeInit()
{
	m_Enc.CloseStream();
	delete[] m_pInputBuffer;
	m_pInputBuffer = NULL;
	delete[] m_pOutputBuffer;
	m_pOutputBuffer = NULL;
}

//////////////////////////////////////////////////////////////////////////
////////////  CByteSwapConvertor
size_t CByteSwapConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
											size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes)
{
	if (NULL == pIn)
	{
		*pUsedBytes = 0;
		return 0;
	}

	size_t nBytes = __min(nInBytes, nOutBytes);
	for (size_t i = 0; i < nBytes; i+= sizeof (WAVE_SAMPLE))
	{
		pOut[i + 1] = pIn[i];
		pOut[i] = pIn[i + 1];
	}
	*pUsedBytes = nBytes;
	return nBytes;
}
