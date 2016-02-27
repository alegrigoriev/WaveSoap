// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// waveproc.cpp

#include "stdafx.h"
#include <math.h>
#pragma intrinsic(sin, cos, exp, log, atan2)
#include <complex>
#include "wavefile.h"
#include "Waveproc.h"
#include <float.h>
#include "Matrix.h"

#ifndef _CONSOLE
#define puts(t) AfxMessageBox(_T(t), MB_OK | MB_ICONEXCLAMATION)

#endif
#define DB_TO_NEPER 0.23025851

#include "FFT.h"

#define TRACE_WAVEPROC 0
#define USE_CUBIC_INTERPOLATION

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
	unsigned size;
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
	unsigned Size() const
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
	void SetBuffer(Type * pBuffer, unsigned Count)
	{
		ASSERT(Count < 0xFFFFu);
		size = 0;
		Purge();
		Array = pBuffer;
		size = Count;
		Purge();
	}

	void AllocateBuffer(unsigned Count)
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

CBatchProcessing::Item::~Item()
{
	delete []InBuf;
	delete []OutBuf;
}
CBatchProcessing::Item::Item()
	: Proc(NULL), InBuf(NULL), OutBuf(NULL), InBufGetIndex(0), InBufPutIndex(0),  OutBufGetIndex(0), OutBufPutIndex(0), Flags(0)
{
}

CBatchProcessing::Item::Item(CWaveProc * proc)
	: Proc(proc), InBuf(NULL), OutBuf(NULL), InBufGetIndex(0), InBufPutIndex(0),  OutBufGetIndex(0), OutBufPutIndex(0), Flags(0)
{
	try
	{
		InBuf = new char[IntermediateBufSize];
		OutBuf = new char[IntermediateBufSize];
	}
	catch(std::bad_alloc &)
	{
		delete[] InBuf;
		throw;
	}
}

CBatchProcessing::Item::Item(Item & item)
	: Proc(item.Proc),
	InBuf(item.InBuf), OutBuf(item.OutBuf),
	InBufGetIndex(item.InBufGetIndex), InBufPutIndex(item.InBufPutIndex),
	OutBufGetIndex(item.OutBufGetIndex), OutBufPutIndex(item.OutBufPutIndex), Flags(item.Flags)
{
	// we grabbed ownership
	item.Proc = NULL;
	item.InBuf = NULL;
	item.OutBuf = NULL;
}

CBatchProcessing::Item & CBatchProcessing::Item::operator =(Item& item)
{
	Proc = item.Proc;
	InBuf = item.InBuf;
	OutBuf = item.OutBuf;
	InBufGetIndex = item.InBufGetIndex;
	InBufPutIndex = item.InBufPutIndex;
	OutBufGetIndex = item.OutBufGetIndex;
	OutBufPutIndex = item.OutBufPutIndex;
	Flags = item.Flags;
	// we grabbed ownership
	item.Proc = NULL;
	item.InBuf = NULL;
	item.OutBuf = NULL;
	return *this;
}

unsigned CBatchProcessing::Item::FillInputBuffer(const char * Buf, unsigned BufFilled, CWaveFormat const * pWf) // returns number bytes used
{
	unsigned BytesUsed = 0;

	unsigned BufferSampleSize = Proc->GetInputWaveformat().SampleSize();
	if (InBufPutIndex - InBufGetIndex < BufferSampleSize)
	{
		memmove(InBuf, InBuf + InBufGetIndex, InBufPutIndex - InBufGetIndex);
		InBufPutIndex -= InBufGetIndex;
		InBufGetIndex = 0;
	}

	unsigned BytesToFill = IntermediateBufSize - InBufPutIndex;
	WaveSampleType SourceType = pWf->GetSampleType();
	if (SourceType == SampleTypeCompressed
		|| SourceType == SampleType8bit
		|| SourceType == SampleTypeNotSupported)
	{
		// all other cases - the data is simply copied
		if (BytesToFill > BufFilled)
		{
			BytesToFill = BufFilled;
		}
		memcpy(InBuf + InBufPutIndex, Buf, BytesToFill);
		BytesUsed = BytesToFill;
		InBufPutIndex += BytesToFill;

		ASSERT(BytesUsed <= BufFilled);
		return BytesUsed;
	}
	else
	{
		NUMBER_OF_SAMPLES SamplesToFill = BytesToFill / BufferSampleSize;
		NUMBER_OF_SAMPLES SourceSamples = BufFilled / pWf->SampleSize();

		if (SamplesToFill > SourceSamples)
		{
			SamplesToFill = SourceSamples;
		}
		BytesUsed = SamplesToFill * pWf->SampleSize();
		CopyWaveSamples(InBuf + InBufPutIndex, ALL_CHANNELS, Proc->GetInputWaveformat().NumChannels(),
			Buf, ALL_CHANNELS, pWf->NumChannels(), SamplesToFill, Proc->GetInputWaveformat().GetSampleType(), SourceType);

		InBufPutIndex += SamplesToFill * BufferSampleSize;
		return BytesUsed;
	}
}

unsigned CBatchProcessing::Item::FillOutputBuffer(char * Buf, unsigned BytesToFill, CWaveFormat const * pWf) // returns number bytes used
{
	unsigned BytesFilled = 0;

	// all channels are copied even though not all be processed

	unsigned BytesToUse = OutBufPutIndex - OutBufGetIndex;
	WaveSampleType TargetType = pWf->GetSampleType();
	unsigned SrcSampleSize = Proc->GetOutputWaveformat().SampleSize();

	if (TargetType == SampleTypeCompressed
		|| TargetType == SampleType8bit
		|| TargetType == SampleTypeNotSupported)
	{
		// all other cases - the data is simply copied
		if (BytesToFill > BytesToUse)
		{
			BytesToFill = BytesToUse;
		}
		memcpy(Buf, OutBuf + OutBufGetIndex, BytesToFill);
		BytesFilled = BytesToFill;
		OutBufGetIndex += BytesToFill;
	}
	else
	{
		NUMBER_OF_SAMPLES SamplesToFill = BytesToFill / pWf->SampleSize();
		NUMBER_OF_SAMPLES SourceSamples = BytesToUse / SrcSampleSize;

		if (SamplesToFill > SourceSamples)
		{
			SamplesToFill = SourceSamples;
		}
		BytesFilled = SamplesToFill * pWf->SampleSize();

		CopyWaveSamples(Buf, ALL_CHANNELS, pWf->NumChannels(),
						OutBuf + OutBufGetIndex, ALL_CHANNELS, Proc->GetOutputWaveformat().NumChannels(),
						SamplesToFill, TargetType, Proc->GetOutputWaveformat().GetSampleType());

		OutBufGetIndex += SamplesToFill * SrcSampleSize;
	}

	if (OutBufPutIndex - OutBufGetIndex < SrcSampleSize)
	{
		memmove(OutBuf, OutBuf + OutBufGetIndex, OutBufPutIndex - OutBufGetIndex);
		OutBufPutIndex -= OutBufGetIndex;
		OutBufGetIndex = 0;
	}

	return BytesFilled;
}

//////////////////////////////////////////////////////////////
/////////// CWaveProc
CWaveProc::CWaveProc()
	: m_bClipped(FALSE),
	m_MaxClipped(0),
	m_ChannelsToProcess(ALL_CHANNELS)
	, m_SavedOutputSamples(0)
	, m_ProcessedInputSamples(0)
	, m_CurrentSample(0)
	, m_InputSampleType(SampleTypeAny)
	, m_OutputSampleType(SampleTypeAny)
#ifdef _DEBUG
	, m_ProcessedInputBytes(0)
	, m_SavedOutputBytes(0)
#endif
{
	m_InputFormat.InitCdAudioFormat();
	m_OutputFormat.InitCdAudioFormat();
}

unsigned CWaveProc::ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	*pUsedBytes = 0;

	if (pInBuf == NULL)
	{
		unsigned const OutputSampleSize = m_OutputFormat.SampleSize();

		unsigned nOutSamples = nOutBytes / OutputSampleSize;
		// process the data

		if (0 == nOutSamples)
		{
			return 0;
		}

		unsigned const NumChannels = m_OutputFormat.NumChannels();

		for (unsigned i = 0; i < nOutSamples; i ++, pOutBuf += OutputSampleSize)
		{
			ProcessSoundSample(NULL, pOutBuf, NumChannels);
			m_CurrentSample++;
		}

		nOutBytes = nOutSamples * OutputSampleSize;
	}
	else if (pOutBuf == NULL)
	{
		unsigned const InputSampleSize = m_InputFormat.SampleSize();

		unsigned nInSamples = nInBytes / InputSampleSize;
		// process the data

		if (0 == nInSamples)
		{
			return 0;
		}

		unsigned const NumChannels = m_InputFormat.NumChannels();

		for (unsigned i = 0; i < nInSamples; i ++, pInBuf += InputSampleSize)
		{
			ProcessSoundSample(pInBuf, NULL, NumChannels);
			m_CurrentSample++;
		}

		* pUsedBytes = nInSamples * InputSampleSize;
		nOutBytes = 0;
	}
	else
	{
		unsigned const InputSampleSize = m_InputFormat.SampleSize();
		unsigned const OutputSampleSize = m_OutputFormat.SampleSize();

		unsigned nInSamples = nInBytes / InputSampleSize;
		unsigned nOutSamples = nOutBytes / OutputSampleSize;
		// process the data
		unsigned nSamples = std::min(nInSamples, nOutSamples);

		if (0 == nSamples || NULL == pInBuf)
		{
			return 0;   // no delayed samples in history
		}

		unsigned const NumChannels = m_InputFormat.NumChannels();

		for (unsigned i = 0; i < nSamples; i ++, pInBuf += InputSampleSize, pOutBuf += OutputSampleSize)
		{
			ProcessSoundSample(pInBuf, pOutBuf, NumChannels);
			m_CurrentSample++;
		}

		* pUsedBytes = nSamples * InputSampleSize;
		nOutBytes = nSamples * OutputSampleSize;
	}

	return nOutBytes;
}

void CWaveProc::ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels)
{
	unsigned InputSampleValueSize = 0;

	if (pInSample != NULL)
	{
		InputSampleValueSize = m_InputFormat.BitsPerSample() / 8;
		ASSERT(InputSampleValueSize * 8 == m_InputFormat.BitsPerSample());
	}

	unsigned OutputSampleValueSize = 0;

	if (pOutSample != NULL)
	{
		OutputSampleValueSize = m_OutputFormat.BitsPerSample() / 8;
		ASSERT(OutputSampleValueSize * 8 == m_OutputFormat.BitsPerSample());
	}

	CHANNEL_MASK CurrentChannelBit = 1;

	for (unsigned i = 0; i < NumChannels; i++, pInSample += InputSampleValueSize, pOutSample += OutputSampleValueSize, CurrentChannelBit <<= 1)
	{
		// find the next channel bit
		if (m_ChannelsToProcess & CurrentChannelBit)
		{
			ProcessSampleValue(pInSample, pOutSample, i);
		}
		else
		{
		}
	}
}

void CWaveProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned /*channel*/)
{
	// default implementation just copies the sample data
	if (pInSample != NULL && pOutSample != NULL)
	{
		ASSERT(m_InputFormat.BitsPerSample() == m_OutputFormat.BitsPerSample());
		memcpy(pOutSample, pInSample, m_InputFormat.BitsPerSample() / 8);
	}
}

BOOL CWaveProc::SetInputWaveformat(CWaveFormat const & Wf)
{
	if (m_InputSampleType == SampleTypeAny
		|| m_InputSampleType == Wf.GetSampleType())
	{
		m_InputFormat = Wf;
		m_InputSampleType = Wf.GetSampleType();
	}
	else if (m_InputSampleType == SampleType16bit)
	{
		m_InputFormat.InitFormat(WAVE_FORMAT_PCM, Wf.SampleRate(), Wf.NumChannels(),
								16);
		// FIXME: Use extended format to carry the channel assignments
	}
	else if (m_InputSampleType == SampleType32bit)
	{
		m_InputFormat.InitFormat(WAVE_FORMAT_PCM, Wf.SampleRate(), Wf.NumChannels(),
								32);
	}
	else if (m_InputSampleType == SampleTypeFloat32)
	{
		m_InputFormat.InitFormat(WAVE_FORMAT_IEEE_FLOAT, Wf.SampleRate(), Wf.NumChannels(),
								32);
	}
	else
	{
		return FALSE;
	}

	if (m_InputFormat.GetSampleType() == m_OutputSampleType
		|| m_OutputSampleType == SampleTypeAny)
	{
		m_OutputFormat = m_InputFormat;   // the format doesn't change
	}

	return TRUE;
}

BOOL CWaveProc::SetOutputWaveformat(CWaveFormat const & Wf)
{
	if (m_OutputSampleType == SampleTypeAny
		|| m_OutputSampleType == Wf.GetSampleType())
	{
		m_OutputFormat = Wf;
		m_OutputSampleType = Wf.GetSampleType();
	}
	else if (m_OutputSampleType == SampleType16bit)
	{
		m_OutputFormat.InitFormat(WAVE_FORMAT_PCM, Wf.SampleRate(), Wf.NumChannels(),
								16);
		// FIXME: Use extended format to carry the channel assignments
	}
	else if (m_OutputSampleType == SampleTypeFloat32)
	{
		m_OutputFormat.InitFormat(WAVE_FORMAT_IEEE_FLOAT, Wf.SampleRate(), Wf.NumChannels(),
								32);
	}
	else if (m_OutputSampleType == SampleType32bit)
	{
		m_OutputFormat.InitFormat(WAVE_FORMAT_PCM, Wf.SampleRate(), Wf.NumChannels(),
								32);
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}
CWaveFormat const & CWaveProc::GetInputWaveformat() const
{
	return m_InputFormat;
}

CWaveFormat const & CWaveProc::GetOutputWaveformat() const
{
	return m_OutputFormat;
}

unsigned CWaveProc::ProcessSound(char const * pInBuf, char * pOutBuf,
								unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	unsigned nSavedBytes = 0;
	*pUsedBytes = 0;

	unsigned const InputSampleSize = GetInputSampleSize();
	// make input buffer multiple of sample size
	if (InputSampleSize != 0)
	{
		nInBytes -= nInBytes % InputSampleSize;
	}

	unsigned const OutputSampleSize = GetOutputSampleSize();
	// make input buffer multiple of sample size
	if (OutputSampleSize != 0)
	{
		nOutBytes -= nOutBytes % OutputSampleSize;
	}

	nSavedBytes = ProcessSoundBuffer(pInBuf, pOutBuf, nInBytes, nOutBytes, pUsedBytes);

	if (InputSampleSize != 0)
	{
		ASSERT(0 == *pUsedBytes % InputSampleSize);
		m_ProcessedInputSamples += NUMBER_OF_SAMPLES(*pUsedBytes / InputSampleSize);
	}

	if (OutputSampleSize != 0)
	{
		ASSERT(0 == nSavedBytes % OutputSampleSize);
		m_SavedOutputSamples += NUMBER_OF_SAMPLES(nSavedBytes / OutputSampleSize);
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
	m_CurrentSample = 0;
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


////////////////// CVolumeChangeProc ///////////////////////
CVolumeChangeProc::CVolumeChangeProc(double const * VolumeArray, unsigned VolumeArraySize)
{
	unsigned i;
	ASSERT(VolumeArraySize != 0 && VolumeArraySize <= countof(m_Volume));

	m_InputSampleType = SampleTypeFloat32;

	for (i = 0; i < VolumeArraySize; i++)
	{
		m_Volume[i] = VolumeArray[i];
	}

	// fill the rest
	for ( ; i < countof(m_Volume); i++)
	{
		m_Volume[i] = m_Volume[VolumeArraySize - 1];
	}
}

void CVolumeChangeProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel)
{
	*(float*)pOutSample = float(m_Volume[channel] * *(float const *)pInSample);
}

/////////////   CWaveMixOperation  /////////////////////////////////////////////
void CWaveMixProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel)
{
	float * pDst = (float *) pOutSample;
	float const* pSrc = (float const*) pInSample;

	if (0)
	{
		*pDst = float(
					*pSrc * GetDstMixCoefficient(m_CurrentSample, channel)
					+ 1 * GetSrcMixCoefficient(m_CurrentSample, channel));
	}
	else
	{
		*pDst = float(*pSrc * GetDstMixCoefficient(m_CurrentSample, channel));
	}

}

///////////// CFadeInOutOperation /////////////////////////////////////////////

CFadeInOutProc::CFadeInOutProc(int FadeCurveType, NUMBER_OF_SAMPLES Length)
	: m_FadeCurveType(FadeCurveType)
	, m_CurveLength(Length)
{
}

double CFadeInOutProc::GetSrcMixCoefficient(SAMPLE_INDEX Sample, int /*Channel*/) const
{
	double const Fraction = (Sample + 0.5) / m_CurveLength;

	switch (m_FadeCurveType)
	{
	case FadeOutLinear:
		return Fraction;
		break;
	case FadeInLinear:
		return 1. - Fraction;
		break;
	case FadeOutSinSquared:
		return 0.5 * (1. - cos(M_PI * Fraction));
		break;
	case FadeInSinSquared:
		return 0.5 * (1. + cos(M_PI * Fraction));
		break;
	case FadeOutCosine:
		return sin(M_PI * 0.5 * Fraction);
		break;
	case FadeInSine:
		return cos(M_PI * 0.5 * Fraction);
		break;
	}
	return 0.;
}

double CFadeInOutProc::GetDstMixCoefficient(SAMPLE_INDEX Sample, int /*Channel*/) const
{
	double const Fraction = (Sample + 0.5)  / m_CurveLength;

	switch (m_FadeCurveType)
	{
	case FadeInLinear:
		return Fraction;
		break;
	case FadeOutLinear:
		return 1. - Fraction;
		break;
	case FadeInSinSquared:
		return 0.5 * (1. - cos(M_PI * Fraction));
		break;
	case FadeOutSinSquared:
		return 0.5 * (1. + cos(M_PI * Fraction));
		break;
	case FadeInSine:
		return sin(M_PI * 0.5 * Fraction);
		break;
	case FadeOutCosine:
		return cos(M_PI * 0.5 * Fraction);
		break;
	}
	return 1.;
}

////////////////// CScanProc ///////////////////////

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

void CHumRemoval::ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels)
{
	float const * pInBuf = (float const *)pInSample;
	float * pOutBuf = (float *)pOutSample;

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

	CHANNEL_MASK CurrentChannelBit = 1;
	for (unsigned ch = 0; ch < NumChannels; ch++, CurrentChannelBit <<= 1)
	{
		if (m_ChannelsToProcess & CurrentChannelBit)
		{
			if (m_ApplyHighpassFilter)
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
			pOutBuf[ch] = (float)curr[ch];
		}
		else
		{
			// copy the sample without processing
			pOutBuf[ch] = (float)pInBuf[ch];
		}
	}

}
// the default implementation copies the sample
//virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);

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

#if 0
	static int test_performed = 1;
	if ( ! test_performed)
	{
		double X[] = { -5, -4., -3., -2., -1., 0., 1., 2. };

		test_quad_regression(X, countof(X), 0, 1, 0);
		test_quad_regression(X, countof(X), 1, 1, 0);
		test_quad_regression(X, countof(X), 1, 0, 1);
		test_quad_regression(X, countof(X), 1, 2, 3);
		test_quad_regression(X, countof(X), 1, 2, -3);

		test_performed = 1;
	}
#endif

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

bool operator <(SAMPLE_INDEX i, StoredClickData const & r1)
{
	return i < r1.Position;
}

///////////////////////////////////////////////////////////
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
		PostInterpolateSamples = InterpolateSamples;
		PreInterpolateSamples = InterpolateSamples;
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

	InterpolateGap(TempBuf, InterpolateOffset, InterpolateSamples, 1, BigGap, BufferSamples);
	// copy back
	for (i = 0; i < InterpolateSamples + PreInterpolateSamples + PostInterpolateSamples; i++)
	{
		data[WriteStartOffset + i] = TempBuf[WriteBufferOffset + i];
	}
}

#if 1
void quad_regression(double const X[], double const Y[], unsigned NumPoints, double &A_result, double &B_result, double&C_result);
void test_quad_regression(double X[], int NumPoints, double A, double B, double C)
{
	int const MAX_POINTS = 20;
	double Y[MAX_POINTS];
	int i;
	// fill the array
	if (NumPoints > MAX_POINTS)
	{
		return;
	}
	for (i = 0; i < NumPoints; i++)
	{
		Y[i] = A + B * X[i] + C * X[i] * X[i];
	}
	TRACE("Testing quad_regression for A=%f, B=%f, C=%f\n", A, B, C);
	quad_regression(X, Y, NumPoints, A, B, C);
	TRACE("Result of quad_regression is A=%f, B=%f, C=%f\n", A, B, C);
}


void quad_regression(double const X[], double const Y[], unsigned NumPoints, double &A_result, double &B_result, double&C_result)
{
	double X_sum = 0.;
	double X_sq_sum = 0.;
	double Y_sum = 0.;
	unsigned i;

	for (i = 0; i < NumPoints; i++)
	{
		X_sum += X[i];
		X_sq_sum += X[i] * X[i];
		Y_sum += Y[i];
	}

	double B = 0;
	double X_avg = X_sum / NumPoints;
	double Y_avg = Y_sum / NumPoints;

	for (i = 0; i < NumPoints; i++)
	{
		B += (X[i] - X_avg) * (Y[i] - Y_avg);
	}

	B /= X_sq_sum - X_sum * X_avg;

	// now X is replaced by X centered
	// X_sq_avg - is average square of centered X, the same as MSR
	double X_sq_avg = (X_sq_sum - X_sum * X_avg) / NumPoints;

	double C = 0;
	double X_centered_sq_sum = 0;
	for (i = 0; i < NumPoints; i++)
	{
		double X_centered = X[i] - X_avg;
		double tmp_x = X_centered * X_centered - X_sq_avg;

		C += tmp_x * (Y[i] - Y_avg - X_centered * B);

		X_centered_sq_sum += tmp_x * tmp_x;
	}

	C /= X_centered_sq_sum;
	// C is relative to X_avg. To return back to X, B needs adjustment
	B-= 2.*X_avg * C;
	// now calculate A
	double A = 0.;
	for (i = 0; i < NumPoints; i++)
	{
		A += Y[i] - X[i] * B - X[i] * X[i] * C;
	}
	A_result = A / NumPoints;
	B_result = B;
	C_result = C;
}

typedef complex<double> complex_d;
typedef std::vector<complex_d> complex_vector;
typedef std::vector<double> double_vector;
typedef double_vector::iterator double_iterator;
typedef double_vector::const_iterator double_const_iterator;
typedef complex_vector::iterator complex_iterator;
typedef complex_vector::const_iterator complex_const_iterator;

complex_d int_power(complex_d arg, unsigned power)
{
	complex_d result(1., 0.);
	while (power != 0)
	{
		if (power & 1)
		{
			result *= arg;
		}
		if (power > 1)
		{
			arg = arg * arg;
		}
		power /= 2;
	}
	return result;
}

double CalculatePower(double const data[], int NumSamples)
{
	double Sum = 0.;
	for (int i = 0; i < NumSamples; i++)
	{
		Sum += data[i] * data[i];
	}
	return Sum / NumSamples;
}

double CalculatePower(double_vector const& data)
{
	return CalculatePower(&data[0], (int)data.size());
}

typedef double_vector::iterator data_iter;
typedef double_vector::const_iterator cdata_iter;

double Average(double_vector const&X)
{
	long N = (long)X.size();
	double Sum = 0;
	for (long i = 0; i != N; i++)
	{
		Sum += X[i];
	}
	return Sum / (long) N;
}

double Average(double_vector const&X, double_vector const& weight)
{
	unsigned N = (unsigned) X.size();
	ASSERT(N == weight.size());
	double Sum = 0;
	double Sum_W = 0;
	for (unsigned i = 0; i != N; i++)
	{
		Sum += X[i] * weight[i];
		Sum_W += weight[i];
	}
	return Sum / Sum_W;
}
// Y = A + B*X
void LinearRegression(double_vector const &X, double_vector const &Y, double &A, double &B)
{
	unsigned N = (unsigned)X.size();
	ASSERT(N == Y.size());

	double x_avg = Average(X);
	double y_avg = Average(Y);

	double numer_sum = 0.;
	double denom_sum = 0.;

	for (unsigned i = 0; i != N; i++)
	{
		numer_sum += (Y[i] - y_avg) * (X[i] - x_avg);
		denom_sum += (X[i] - x_avg) * (X[i] - x_avg);
	}

	B = numer_sum / denom_sum;
	A = y_avg - x_avg * B;
}

void LinearRegression(double_vector const &X, double_vector const &Y, double_vector const &weight, double &A, double &B)
{
	unsigned N = (unsigned)X.size();
	ASSERT(N == Y.size());
	ASSERT(N == weight.size());

	double x_avg = Average(X, weight);
	double y_avg = Average(Y, weight);

	double numer_sum = 0.;
	double denom_sum = 0.;

	for (unsigned i = 0; i != N; i++)
	{
		numer_sum += weight[i] * (Y[i] - y_avg) * (X[i] - x_avg);
		denom_sum += weight[i] * (X[i] - x_avg) * (X[i] - x_avg);
	}

	B = numer_sum / denom_sum;
	A = y_avg - x_avg * B;
}

void FilterSeries(double const source[], double destination[], int NumSamples, complex_d FilterZero, complex_d FilterPole)
{
	double coeffs[3] = { 1., -2. * FilterZero.real(), FilterZero.real() * FilterZero.real() + FilterZero.imag() * FilterZero.imag()};
	double denom_coeffs[3] = { 1., -2. * FilterPole.real(), FilterPole.real() * FilterPole.real() + FilterPole.imag() * FilterPole.imag()};
	double History[2] = {source[0], source[1]};

	double correction = 1.;
	if (FilterZero != complex_d(0., 0.))
	{
		correction /= abs(FilterZero);
	}

	if (FilterPole != complex_d(0., 0.))
	{
		correction *= abs(FilterPole + 1./FilterPole);
		for (int i = 0; i + 2 < NumSamples; i++)
		{
			double tmp = correction * (source[i + 2] + coeffs[1] * source[i+1] + coeffs[2] * source[i]);
			destination[i] = tmp - History[0] * denom_coeffs[1] - History[1] * denom_coeffs[2];
			History[1] = History[0];
			History[0] = destination[i];
		}
	}
	else
	{
		for (int i = 0; i + 2 < NumSamples; i++)
		{
			destination[i] = correction * (source[i + 2] + coeffs[1] * source[i+1] + coeffs[2] * source[i]);
		}
	}
}

void FilterSeries(double_vector const &source, double_vector &destination, complex_d const&FilterZero, complex_d const&FilterPole)
{
	destination.resize(source.size()-2);
	FilterSeries(&source[0], &destination[0], (int)source.size(), FilterZero, FilterPole);
}

void FilterSeriesInPlace(double_vector &source, complex_d const&FilterZero, complex_d const&FilterPole)
{
	FilterSeries(&source[0], &source[0], (int)source.size(), FilterZero, FilterPole);
	source.erase(source.end()-2, source.end());
}

void FilterSeriesBackwards(double const source[], double destination[], int NumSamples, complex_d FilterZero, complex_d FilterPole)
{
	double coeffs[3] = { 1.,
						FilterZero.real() * -2.,
						FilterZero.real() * FilterZero.real() + FilterZero.imag() * FilterZero.imag()};

	double correction = 1.;
	ASSERT(FilterPole != complex_d(0., 0.));

	if (FilterPole != complex_d(0., 0.))
	{
		correction *= abs(FilterPole + 1./FilterPole);
	}

	if (FilterZero != complex_d(0., 0.))
	{
		correction /= abs(FilterZero);
	}
	// use 1/FilterPole for filtering backwards
	double denom_coeffs[3] = { 1.,
		FilterPole.real() * -2. /(FilterPole.real() * FilterPole.real() + FilterPole.imag() * FilterPole.imag()),
		1/(FilterPole.real() * FilterPole.real() + FilterPole.imag() * FilterPole.imag())};
	double History[2] = {source[NumSamples-2], source[NumSamples-1]};

	for (int i = NumSamples - 3; i >= 0; i--)
	{
		double tmp = correction * (coeffs[2] * source[i] + coeffs[1] * source[i+1] + source[i + 2]);
		destination[i] = tmp - History[0] * denom_coeffs[1] - History[1] * denom_coeffs[2];
		History[1] = History[0];
		History[0] = destination[i];
	}
}

void FilterSeriesBackwards(double_vector const &source, double_vector &destination, complex_d const&FilterZero, complex_d const&FilterPole)
{
	destination.resize(source.size()-2);
	FilterSeriesBackwards(&source[0], &destination[0], (int)source.size(), FilterZero, FilterPole);
}

void FindZeroApproximation(double const source[], int NumSamples, complex_d &FilterZero, int & MaxBinNumber, double &Power)
{
	// NumSamples - power of 2
	ASSERT(NumSamples == (NumSamples & -NumSamples));

	complex_vector f(NumSamples / 2 + 1, 0.);
	// run FFT, find maximum line, return its amplitude and phase
	FastFourierTransform(source, &f[0], NumSamples);

	int i;
	double MaxPower = 0;
	int MaxPowerPos= 0;
	complex_iterator pf;

	for (i = 1, pf = f.begin() +1; i < NumSamples / 2; i++, pf++)
	{
		double current = pf->real() * pf->real() + pf->imag() * pf->imag();
		if (current > MaxPower)
		{
			MaxPower = current;
			MaxPowerPos = i;
		}
	}

	Power = MaxPower;
	MaxBinNumber = MaxPowerPos;
	FilterZero.real(cos(MaxPowerPos * M_PI / (NumSamples / 2)));
	FilterZero.imag(sin(MaxPowerPos * M_PI / (NumSamples / 2)));
	TRACE("FindZeroApproximation: max found at %d\n", MaxPowerPos);
}

typedef double TargetFunc(void * arg, double x, double y);

void FindMinimum2D(double x_init, double y_init, double x_delta, double y_delta, TargetFunc* Func, void * FuncArg,
					double &x_result, double &y_result)
{
	// x_delta, y_delta - max deviation by x and y from initial point
	static int Do_TRACE_FindMinimum2D = 0;
	double TargetArray[3][3];
	double x = x_init;
	double y = y_init;
	double x_min = x_init - x_delta;
	double x_max = x_init + x_delta;
	double y_min = y_init - y_delta;
	double y_max = y_init + y_delta;

	double CurrentDeltaX = x_delta / 32;
	double CurrentDeltaY = y_delta / 32;

	int iteration;
	for (iteration = 0; iteration < 200; iteration++)
	{
		// fill 3x3 array
		double SmallestTarget = 0;
		int sm_i = 0;
		int sm_j = 0;

		if (Do_TRACE_FindMinimum2D) TRACE("FindMinimum2D: y=%f, dy=%f, x=%f (%f Hz), dx=%f\n", y, CurrentDeltaY, x, x * 22050 / M_PI, CurrentDeltaX);

		for (int j = 0; j < 3; j++)
		{
			for (int i = 0; i < 3; i++)
			{
				TargetArray[i][j] = Func(FuncArg, x + CurrentDeltaX * (i - 1), y + CurrentDeltaY * (j - 1));
				if ((i == 0 && j == 0)
					|| TargetArray[i][j] < SmallestTarget)
				{
					SmallestTarget = TargetArray[i][j];
					sm_i = i;
					sm_j = j;
				}
			}
			if (Do_TRACE_FindMinimum2D) TRACE("FindMinimum2D %d: LOG(T)=%f, %f, %f\n",
											j, log(TargetArray[0][j]), log(TargetArray[1][j]), log(TargetArray[2][j]));
		}
		// take new smallest point
#if 0
		if (sm_i == 1)
		{
			// divide the step in half
			CurrentDeltaX /= 2.;
		}
		if (sm_j == 1)
		{
			// divide the step in half
			CurrentDeltaY /= 2.;
		}

		x += CurrentDeltaX * (sm_i - 1);
		y += CurrentDeltaY * (sm_j - 1);

		if (1) if (x < x_min)
			{
				x = x_min;
				CurrentDeltaX /= 2.;
			}
			else if (x > x_max)
			{
				x = x_max;
				CurrentDeltaX /= 2.;
			}
		if (1) if (y < y_min)
			{
				y = y_min;
				CurrentDeltaY /= 2.;
			}
			else if (y > y_max)
			{
				y = y_max;
				CurrentDeltaY /= 2.;
			}

		if (CurrentDeltaX < x_delta / 8192.
			&& CurrentDeltaY < y_delta / 8192.)
		{
			x_result = x;
			y_result = y;
			return;
		}
#else
		if (sm_i == 1
			&& sm_j == 1)
		{
			// divide the step in half
			CurrentDeltaX /= 2.;
			CurrentDeltaY /= 2.;
		}
		else
		{
			x += CurrentDeltaX * (sm_i - 1);
			y += CurrentDeltaY * (sm_j - 1);

			if (1) if (x < x_min)
				{
					x = x_min;
					CurrentDeltaX /= 2.;
				}
				else if (x > x_max)
				{
					x = x_max;
					CurrentDeltaX /= 2.;
				}
			if (1) if (y < y_min)
				{
					y = y_min;
					CurrentDeltaY /= 2.;
				}
				else if (y > y_max)
				{
					y = y_max;
					CurrentDeltaY /= 2.;
				}
		}

		if (CurrentDeltaX < x_delta / 8192.
			&& CurrentDeltaY < y_delta / 8192.)
		{
			x_result = x;
			y_result = y;
			return;
		}
#endif
	}
	TRACE("FindMinimum2D didn't converge in %d iterations: y=%f, x=%f\n", iteration, y, x);
	x_result = x;
	y_result = y;
	return;
}

struct FilterResidualTargetFuncContext
{
	double_vector source;
	double_vector result;
};

TargetFunc FilterResidualTargetFunc;
double FilterResidualTargetFunc(void * arg, double x, double y)
{
	FilterResidualTargetFuncContext *ctx = (FilterResidualTargetFuncContext*)arg;

	if (0)
	{
		complex_d FilterZero(y*cos(x), y*sin(x));
		double PoleRadius;
		if (y <= 1.01)
		{
			PoleRadius = y * 0.98;
		}
		else
		{
			PoleRadius = y * 1.02;
		}
		complex_d FilterPole(PoleRadius*cos(x), PoleRadius*sin(x));

		if (PoleRadius <= 1.)
		{
			FilterSeries(ctx->source, ctx->result, FilterZero, FilterPole);
		}
		else
		{
			FilterSeriesBackwards(ctx->source, ctx->result, FilterZero, FilterPole);
		}
	}
	else
	{
		FilterSeries(ctx->source, ctx->result, complex_d(y*cos(x), y*sin(x)), complex_d(0.,0.));
	}
	return CalculatePower(ctx->result);
}

TargetFunc FilterExciteTargetFunc;
double FilterExciteTargetFunc(void * arg, double x, double y)
{
	// find the max resonance
	// x is frequency, y is decay (1 - no decay)
	FilterResidualTargetFuncContext *ctx = (FilterResidualTargetFuncContext*)arg;

	if (y <= 1.)
	{
		if (y >= 0.99999)
		{
			y = 0.99999;
		}
		FilterSeries(ctx->source, ctx->result, complex_d(0., 0.), complex_d(y*cos(x), y*sin(x)));
		return 1. / CalculatePower(& ctx->result[(int)ctx->result.size()-(int)ctx->result.size()/2], (int)ctx->result.size()/2);    // maximum power - minimum target function
	}
	else
	{
		if (y < 1.000001)
		{
			y = 1.000001;
		}
		FilterSeriesBackwards(ctx->source, ctx->result, complex_d(0., 0.), complex_d(y*cos(x), y*sin(x)));
		return 1. / CalculatePower(& ctx->result[0], (int)ctx->result.size()/2);    // maximum power - minimum target function
	}
}

void EstimateLocalFrequency(double const source[], int const NumSamples, double FrequencyEstimation, double &FrequencyResult, double &decay, complex_d &amplitude)
{
	complex_vector signal(NumSamples, 0.);
	complex_iterator p;
	int i;

	// move the estimated frequency to zero neighborhood (making it a complex signal)
	for (i = 0, p = signal.begin(); i < NumSamples; i++, p++)
	{
		p->real(source[i] * cos(fmod(FrequencyEstimation * (NumSamples - i), 2*M_PI)));
		p->imag(source[i] * sin(fmod(FrequencyEstimation * (NumSamples - i), 2*M_PI)));
	}
	// filter it with zero-phase FIR (Hamming window)
	double_vector fir(NumSamples / 2);
	// build the Hamming window
	for (i = 0, p = signal.begin(); i < NumSamples / 2; i++, p++)
	{
		fir[i] = (0.54 + 0.46 *
					cos(M_PI * (i - NumSamples/4. + 0.5)/(NumSamples/4)))
				// normalized to unity on DC:
				/ (0.54 * NumSamples / 2);
	}
	// run the filter
	for (i = 0, p = signal.begin(); i < NumSamples/2; i++, p++)
	{
		complex_iterator p1;
		double_vector::iterator pf;
		complex_d tmp(0.);
		for (pf = fir.begin(), p1 = p; pf != fir.end(); p1++, pf++)
		{
			tmp += *pf * *p1;
		}
		*p = tmp;
	}
	// Find its exact frequency and decay.
	// convert the array to phase and amplitude
	// save phase to imag() and amplitude to real()
	double CurrentPhaseAdjustment = 0;
	double AveragePhase = 0;
	double AverageAmplitude = 0;

	for (i = 0, p = signal.begin(); i < NumSamples/2; i++, p++)
	{
		double phase = arg(*p) + CurrentPhaseAdjustment;
		double amp = abs(*p);
		// if phase crossed PI, apply new adjustment
		if (i != 0)
		{
			double phase_delta = phase - p[-1].imag();
			if (phase_delta > M_PI)
			{
				phase -= 2* M_PI;
				CurrentPhaseAdjustment -= 2* M_PI;
			}
			else if (phase_delta < -M_PI)
			{
				phase += 2* M_PI;
				CurrentPhaseAdjustment += 2* M_PI;
			}
		}
		p->imag(phase);
		AveragePhase += phase;
		p->real(log(amp));
		AverageAmplitude += p->real();
	}
	AveragePhase /= NumSamples/2;
	AverageAmplitude /= NumSamples/2;

	double PhaseSlope = 0;
	double AmplitudeSlope = 0;
	double DenominatorSum = 0;

	for (i = 0, p = signal.begin(); i < NumSamples/2; i++, p++)
	{
		double tmp = i - (NumSamples/4 - 0.5);
		DenominatorSum = tmp * tmp;
		PhaseSlope = (p->imag() - AveragePhase) * tmp;
		AmplitudeSlope = (p->real() - AverageAmplitude) * tmp;
	}
	PhaseSlope /= DenominatorSum;
	AmplitudeSlope /= DenominatorSum;
	FrequencyResult = FrequencyEstimation + PhaseSlope;
	decay = AmplitudeSlope;
	// calculate amplitude and phase at the end of sample array (shift from average by NumSamples/2 + 1)
	double NextPhase = AveragePhase + PhaseSlope * (NumSamples/2 + 0.5);
	double NextAmplitude = 2.*exp(AverageAmplitude + AmplitudeSlope * (NumSamples/2 + 0.5));
	amplitude.real(cos(NextPhase)*NextAmplitude);
	amplitude.imag(sin(NextPhase)*NextAmplitude);
}

struct SpectralComponent
{
	double frequency, decay;
	double Power;
	complex_d FilterZero;
	complex_d FilterPole;
	double InitialAmplitude, AmplitudeDecay;
	double InitialPhase, Frequency;
	SpectralComponent() : FilterZero(0., 0.), FilterPole(0., 0.) {}
};

void FindComponent(const double_vector &source, int FftOrder, SpectralComponent & comp)
{
	// find first approximation
	int const DO_FindComponent_TRACE = 0;
	int MaxBinNumber;

	FindZeroApproximation(&source[0], FftOrder, comp.FilterZero, MaxBinNumber, comp.Power);
	// find best frequency approximation
	double freq_init = MaxBinNumber * M_PI * 2 / FftOrder;

	if (DO_FindComponent_TRACE) TRACE("First frequency approx found= %f, Amplitude = %f\n", MaxBinNumber * 44100. / FftOrder, sqrt(comp.Power*2) / FftOrder);

	double_vector Residual;
	FilterResidualTargetFuncContext ctx;
	ctx.source = source;

	FindMinimum2D(freq_init, 1., M_PI *4/ FftOrder, 0.1, FilterResidualTargetFunc, & ctx, comp.frequency, comp.decay);
	//FindMinimum2D(freq_init, 1., M_PI / NumSamples*2, 0.1, FilterExciteTargetFunc, & ctx, comp.frequency, comp.decay);
	comp.FilterZero.real(comp.decay * cos(comp.frequency));
	comp.FilterZero.imag(comp.decay * sin(comp.frequency));
	if (DO_FindComponent_TRACE) TRACE("Frequency result found= %f, decay = %f\n", comp.frequency * 22050. / M_PI, comp.decay);
}

void DiscreteCosineTransform(double const source[], double destination[], int NumSamples)
{
	double_vector x(NumSamples*2, 0.);
	complex_vector f1(NumSamples+1);
	std::copy(&source[0], &source[NumSamples], x.begin()+NumSamples/2);
	FastFourierTransform(&x[0], &f1[0], NumSamples*2);

	complex_iterator p = f1.begin();
	for (int i = 0; i < NumSamples; i+=2, p+=2)
	{
		destination[i] = p->real()*2.;
		destination[i+1] = p[1].imag()*2;
	}
}
typedef std::vector<SpectralComponent> comp_vector;
typedef comp_vector::iterator comp_iter;
typedef comp_vector::const_iterator comp_const_iter;

void FindComponents(double_vector const& x, comp_vector &Components, int FftOrder, int const MaxNumComponent = 4)
{
	Components.clear();

	double OriginalPower = CalculatePower(x);
	double CurrentResidualPower = OriginalPower;
	double_vector Residual(x);

	static int const DO_FindComponents_TRACE = 1;

	for (int n = 0; n< MaxNumComponent && CurrentResidualPower*1000000 >OriginalPower; n++)
	{
		SpectralComponent comp;
		FindComponent(Residual, FftOrder, comp);
		Components.push_back(comp);
		if (DO_FindComponents_TRACE) TRACE("First approx frequency result %d found= %f, decay = %f\n", Components.size(), comp.frequency * 22050. / M_PI, comp.decay);

		// now find the better approximation for all
		for (comp_iter i = Components.begin(); i != Components.end(); i++)
		{
			Residual = x;
			for (comp_iter j = Components.begin(); j != Components.end(); j++)
			{
				if (j != i)
				{
					FilterSeriesInPlace(Residual, j->FilterZero, j->FilterPole);
				}
			}
			if (Components.size() != 1)
			{
#if 0
				FindComponent(Residual, FftOrder, *i);
#else
				FilterResidualTargetFuncContext ctx;
				ctx.source = Residual;

				FindMinimum2D(i->frequency, 1., M_PI *4/ FftOrder, 0.1, FilterResidualTargetFunc, & ctx, i->frequency, i->decay);
				//FindMinimum2D(freq_init, 1., M_PI / NumSamples*2, 0.1, FilterExciteTargetFunc, & ctx, comp.frequency, comp.decay);
				i->FilterZero.real(i->decay * cos(i->frequency));
				i->FilterZero.imag(i->decay * sin(i->frequency));
#endif
				if (DO_FindComponents_TRACE) TRACE("New frequency result %d approx = %f, decay = %f\n", (i - Components.begin()) + 1, i->frequency * 22050. / M_PI, i->decay);
			}
			if (i+1 == Components.end())
			{
				FilterSeriesInPlace(Residual, i->FilterZero, i->FilterPole);
				// now Residual has the required contents for the next iteration
			}
		}

		CurrentResidualPower = CalculatePower(Residual);
		if (DO_FindComponents_TRACE) TRACE("Original power=%f (ampl=%f), residual power=%f (ampl=%f)\n",
											OriginalPower, sqrt(2*OriginalPower),
											CurrentResidualPower, sqrt(2*CurrentResidualPower));
	}

	// now find components amplitude and phase
	complex_vector result;
	double_vector amplitudes;
	double_vector phases;
	double_vector X;
	double_vector Weight;

	for (comp_iter i = Components.begin(); i != Components.end(); i++)
	{
		Residual = x;
		complex_d correction = 2. * i->FilterZero / (i->FilterZero - conj(i->FilterZero));
		// correct for the single zero filter. The zero and the target signal are complex conjugates

		for (comp_iter j = Components.begin(); j != Components.end(); j++)
		{
			if (j != i)
			{
				FilterSeriesInPlace(Residual, j->FilterZero, j->FilterPole);

				// FilterSeries will correct the result by 1/abs(j->FilterZero). Here, we want to adjust it back
				correction *= abs(j->FilterZero);
				if (j->FilterPole != complex_d(0., 0.))
				{
					correction /= abs(j->FilterPole);
				}
				// now correct for the target frequency
				correction /= 1. + i->FilterZero *
					(i->FilterZero * (j->FilterZero.real() * j->FilterZero.real() + j->FilterZero.imag() * j->FilterZero.imag()) - 2. * j->FilterZero.real());
			}
		}
		// filter with complex coefficients to suppress negative frequency part
		result.resize(Residual.size() - 1);
		amplitudes.resize(Residual.size() - 1);
		phases.resize(Residual.size() - 1);
		X.resize(Residual.size() - 1);
		Weight.resize(Residual.size() - 1);

		double_const_iterator i1 = Residual.begin();
		double_iterator amp = amplitudes.begin();
		double_iterator ph = phases.begin();
		complex_iterator i2 = result.begin();
		double phase_correction = 0.;

		for (int xx = 0; i2 != result.end(); i1++, i2++, amp++, ph++, xx++)
		{
			i2->real(i1[1] - i1[0] * i->FilterZero.real());
			// use complex conjugate of the zero, to filter out negative frequencies
			i2->imag( + i1[0] * i->FilterZero.imag());
			*i2 *= correction;

			*amp = 0.5* log(i2->real() * i2->real() + i2->imag() * i2->imag());
			*ph = arg(*i2) + phase_correction;
			// the phases go to the increasing direction
			if (ph != phases.begin())
			{
				if (ph[0] + M_PI < ph[-1])
				{
					phase_correction += 2 * M_PI;
					ph[0] += 2 * M_PI;
				}
				else if (ph[0] - M_PI > ph[-1])
				{
					phase_correction -= 2 * M_PI;
					ph[0] -= 2 * M_PI;
				}
			}
			X[xx] = xx;
			Weight[xx] = (int)Weight.size() - xx;
		}

		LinearRegression(X, amplitudes, Weight, i->InitialAmplitude, i->AmplitudeDecay);
		LinearRegression(X, phases, Weight, i->InitialPhase, i->Frequency);
		TRACE("Initial amplitude for f=%f: %f, amplitude decay: %f dB/s\n",
			i->frequency * 22050. / M_PI, exp(i->InitialAmplitude), i->AmplitudeDecay * 44100 / log(10.) * 20.);
		TRACE("Initial phase for f=%f: %f, exact frequency: %f Hz\n",
			i->frequency * 22050. / M_PI, i->InitialPhase, i->Frequency * 22050. / M_PI);
	}
}

void GenerateReconstructedSignal(comp_vector const& Components, double_iterator p, int Number, int FirstTime)
{
	if (Number >= 0)
	{
		for (int pos = 0, t = FirstTime-1; pos < Number; pos++, t++, p++)
		{
			double tmp = 0.;
			for (comp_const_iter pc = Components.begin(); pc != Components.end(); pc++)
			{
				tmp += cos(fmod(pc->Frequency * t + pc->InitialPhase, 2*M_PI)) * exp(pc->InitialAmplitude + pc->AmplitudeDecay * t);
			}
			*p = tmp;
		}
	}
	else
	{
		// if Number is negative, time goes back. p will post-increment, so the time is inversed.
		for (int pos = -1, t = FirstTime-2; pos >= Number; pos--, t--, p++)
		{
			double tmp = 0.;
			for (comp_const_iter pc = Components.begin(); pc != Components.end(); pc++)
			{
				tmp += cos(fmod(pc->Frequency * t + pc->InitialPhase, 2*M_PI)) * exp(pc->InitialAmplitude + pc->AmplitudeDecay * t);
			}
			*p = tmp;
		}
	}
}

void CClickRemoval::InterpolateBigGapSliding(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, int TotalSamples)
{
	const int MAX_FFT_ORDER = 2048;
	// nLeftIndex: 2048 + ClickLength + ClickLength / 2
	double_vector x;

	int FftOrder = 64;
	while (FftOrder < ClickLength * 4)
	{
		FftOrder *= 2;
	}

	//TRACE("FFtOrder used for interpolation: %d\n", FftOrder);
	if (FftOrder > MAX_FFT_ORDER)
	{
		return;
	}

	// extrapolate ClickLength + ClickLength / 2 - the gap and
	// the right neighborhood
	int const ExtrapolatedLength = ClickLength;// + ClickLength / 2;

	ASSERT(nLeftIndex >= FftOrder - ExtrapolatedLength);

	x.resize(TotalSamples, 0.);
	for (int i = 0; i < TotalSamples; i++)
	{
		x[i] = data[nChans * i];
	}

	comp_vector LeftComponents;
	comp_vector RightComponents;

	// left components are done in inversed time
	FindComponents(double_vector(x.rend() - nLeftIndex, x.rend()), LeftComponents, FftOrder);
	// Generate the reconstructed signal back and forward from the initial conditions

	FindComponents(double_vector(x.begin() + nLeftIndex + ClickLength, x.end()), RightComponents, FftOrder);

	double_vector LeftReconstruction(ClickLength * 2, 0.);
	double_vector RightReconstruction(ClickLength * 2, 0.);

	GenerateReconstructedSignal(LeftComponents, LeftReconstruction.begin(), -ClickLength * 2, ClickLength);
	GenerateReconstructedSignal(RightComponents, RightReconstruction.begin(), ClickLength * 2, -ClickLength);

	for (int i = nLeftIndex - ClickLength, j = 0; j < ClickLength; i++, j++)
	{
		data[i*nChans] = DoubleToShort((data[i*nChans] * (ClickLength - (j + 0.5)) + LeftReconstruction[j] * (j + 0.5)) / ClickLength);
	}

	for (int i = nLeftIndex, j = 0; j < ClickLength; i++, j++)
	{
		data[i*nChans] = DoubleToShort((LeftReconstruction[j+ClickLength] * (ClickLength - (j + 0.5)) + RightReconstruction[j] * (j + 0.5)) / ClickLength);
	}

	for (int i = nLeftIndex + ClickLength, j = 0; j < ClickLength; i++, j++)
	{
		data[i*nChans] = DoubleToShort((RightReconstruction[j+ClickLength] * (ClickLength - (j + 0.5)) + data[i*nChans] * (j + 0.5)) / ClickLength);
	}

	return;
}
#endif

void CClickRemoval::InterpolateGapLeastSquares(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, int TotalSamples)
{
	// nChan (1 or 2) is used as step between samples
	// to interpolate stereo, call the function twice
	// Perform spike interpolation
	// Use interpolating polynom of 3rd order

	// the click goes from nLeftIndex to nLeftIndex+ClickLength

	// take ClickLength*2 to the left, and ClickLength*2 to the right, up to 32 samples

	int const MaxInterpolationCount = 64;

	ASSERT(ClickLength >= 2);
	int const InterpolationCount = std::min(std::min(std::min(ClickLength*2, MaxInterpolationCount), nLeftIndex), TotalSamples-(nLeftIndex+ClickLength));

	double_vector Y_odd(InterpolationCount);
	double_vector Y_even(InterpolationCount);
	double_vector X(InterpolationCount);
	double_vector X_sq(InterpolationCount);
	double_vector W(InterpolationCount);
	double_vector W_odd(InterpolationCount);

	for (int n = 0; n < InterpolationCount; n ++)
	{
		X[n] = n + ClickLength /2. + 0.5;
		X_sq[n] = X[n] * X[n];

		W[n] = 1.;  // no weight by now
		W_odd[n] = 1.;  // no weight by now

		double y_left = data[nChans * (nLeftIndex - 1 - n)];
		double y_right = data[nChans * (nLeftIndex + ClickLength + n)];

		// regression on odd components is performed on Y/X data
		Y_odd[n] = (y_right - y_left) / X[n];

		Y_even[n] = y_right + y_left;
	}

	// A + B*X + C * X*X + D* X^3
	double C=0;
	double B=0;
	double A=0;
	double D=0;
	LinearRegression(X_sq, Y_even, W, A, C);


	// to separate B and D, we perform regression on Y/X, relative to X*X
	// fill X centered and squared array

	LinearRegression(X_sq, Y_odd, W_odd, B, D);

	// perform polynomial interpolation
	for (int n = 0; n < InterpolationCount * 2 + ClickLength; n++)
	{
		double x = n - (InterpolationCount + ClickLength /2. + 0.5);
		double y = (A + x * (B + x * (C + x * D))) / 2.;
		int i = nLeftIndex + n - InterpolationCount;

		if (i < nLeftIndex)
		{
			// blend with source
			double c = (nLeftIndex - i) / double(InterpolationCount + 1);
			y += (data[nChans * i] -y) * c;
		}
		else if (i >= nLeftIndex + ClickLength)
		{
			double c = (i - (nLeftIndex + ClickLength - 1)) / double(InterpolationCount + 1);
			y += (data[nChans * i] -y) * c;
		}

		data[nChans * i] = DoubleToShort(y);
	}
}


void CClickRemoval::InterpolateGap(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, bool BigGap, int TotalSamples)
{
	if (BigGap)
	{
		InterpolateBigGapSliding(data, nLeftIndex, ClickLength, nChans, TotalSamples);
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
	int const MaxInterpolationOrder = 5;
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

unsigned CClickRemoval::ProcessSoundBuffer(char const * pIn, char * pOut,
											unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
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
													m_PredefinedClicks.end(), SAMPLE_INDEX(PrevIndex - ANALYZE_LAG));

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

	FILE * pInClicksFile = NULL;
	_tfopen_s(& pInClicksFile, szFilename, _T("rt"));

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
		if ( 2 > sscanf_s(line, "%d %d %d", &pos, &length_l, &length_r))
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
		ClicksVectorIterator i1, i2;
		for (i1 = m_PredefinedClicks.begin(), i2 = i1 + 1;
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
		m_pOutClicksFile = NULL;
		_tfopen_s(&m_pOutClicksFile, szFilename, _T("wt"));
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

	std::complex<DATA> sp_FftIn[3];
	NoiseReductionCore * pNr;

	void AnalyzeFftSample(std::complex<DATA> smp, int nSample);
	static void AnalyzeStereoFftSample(std::complex<DATA> left, std::complex<DATA> right, SIGNAL_PARAMS *pL, SIGNAL_PARAMS *pR, int nSample);
	bool PreprocessFftSample(std::complex<DATA> smp, int nSample);  // return true - can apply post-processing. false - cannot apply post-processing
	// average frequency and amplitude in the band
	float sp_AvgFreq; // filtered arg(x[n] / x[n-1])
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
	float sp_PrevPhase; // previous phase
	float sp_Freq;  // current frequency
	float Power[3];     // current power
	float sp_MaskingPower; // masking function
	float sp_PrevMaskingPower;
	bool m_TonalBand;
	bool m_TransientBand;
#ifdef _DEBUG
	unsigned TransientBandCount;
	unsigned TonalBandCount;
	unsigned TotalBandsProcessed;
	float MinLevelInBandNp;
	float MaxLevelInBandNp;
#endif

	SIGNAL_PARAMS(NoiseReductionCore * Nr)
		: sp_AvgFreq(0.f)
		, sp_AvgLevelChange(0.f)
		//, sp_AvgLevel(0.f)
		, sp_FilteredLevel(0.f)
		, sp_FilteredFreqError(0.f)
		, sp_FilteredLevelError(0.f)
		, sp_FreqDev(0.f)
		, sp_LevelDev(0.f)
		, sp_PrevPhase(0.f)
		, sp_Freq(0.f)
		, sp_MaskingPower(0.f)
		, sp_PrevMaskingPower(0.f)
		, m_TonalBand(false)
		, m_TransientBand(false)
		, pNr(Nr)
#ifdef _DEBUG
		, TransientBandCount(0)
		, TonalBandCount(0)
		, MinLevelInBandNp(100.)
		, MaxLevelInBandNp(-100.)
		, TotalBandsProcessed(0)
#endif
	{
		sp_FftIn[0] = DATA(0.);
		sp_FftIn[1] = DATA(0.);
		sp_FftIn[2] = DATA(0.);
		Power[0] = 0.;
		Power[1] = 0.;
		Power[2] = 0.;
	}
	void Dump(int n)
	{
		TRACE("%i: Power=%f, Freq=%f, AvgFreq=%f, FreqDev=%f, Tonal=%i, transient=%i\n", n, Power[0] / (32768.*32768.), sp_Freq, sp_AvgFreq, sp_FreqDev, m_TonalBand, m_TransientBand);
	}
};

// smp - source FFT sample
// pNr - points to the parent CNoiseReduction object
// nSample - number of the sample in FFT set

bool SIGNAL_PARAMS::PreprocessFftSample(std::complex<DATA> smp, int nSample)
{
	Power[2] = Power[1];
	Power[1] = Power[0];
	Power[0] = float(pNr->m_PowerScale * (real(smp) * real(smp) + imag(smp) * imag(smp)));

	sp_FftIn[2] = sp_FftIn[1];
	sp_FftIn[1] = sp_FftIn[0];
	sp_FftIn[0] = smp;


	if (0. == Power[0]
		|| 0. == Power[1])
	{
		m_TonalBand = false;
		m_TransientBand = false;

		sp_AvgFreq = 0;
		sp_PrevPhase = arg(smp);

		if (Power[0] != 0.)
		{
			sp_FilteredLevel = float(log(abs(smp)));
		}
		else
		{
			sp_FilteredLevel = -16.;    // in Nepers
		}
		sp_FilteredFreqError = 0;
		sp_FilteredLevelError = 0;
		return false;
	}

#ifdef _DEBUG
	pNr->m_TotalBandProcessed++;
	TotalBandsProcessed++;
#endif
#if 1
	float dPhase = (float)atan2(imag(smp), real(smp));

	// find momentary frequency
	sp_Freq = dPhase - sp_PrevPhase;

	// odd samples have an additional pi of phase difference
	if (nSample & 1)
	{
		if (sp_Freq < 0)
		{
			sp_Freq += float(M_PI);
		}
		else
		{
			sp_Freq -= float(M_PI);
		}
	}
	if (sp_Freq < -M_PI)
	{
		sp_Freq += float(M_PI * 2);
	}
	else if (sp_Freq > M_PI)
	{
		sp_Freq -= float(M_PI * 2);
	}
#else
	float dLevel = float(log(abs(smp)));
	float dPhase = (float)arg(smp);

	sp_Freq = (float)arg(smp / sp_PrevFftIn);

#endif
	sp_PrevPhase = dPhase;
	if (Power[2] * 30. < Power[0])
	{
		sp_AvgFreq = sp_Freq;
		sp_FreqDev = float(pNr->m_FreqThresholdOfNoiselike * 1.1);
	}
	return true;
}

void SIGNAL_PARAMS::AnalyzeStereoFftSample(std::complex<DATA> left, std::complex<DATA> right, SIGNAL_PARAMS *pL, SIGNAL_PARAMS *pR, int nSample)
{
	if ( ! pL->PreprocessFftSample(left, nSample))
	{
		pR->PreprocessFftSample(right, nSample);
		return;
	}

	if ( ! pR->PreprocessFftSample(right, nSample))
	{
		return;
	}
	ASSERT(pL->Power[0] != 0. && pL->Power[0] != 0.);
	ASSERT(pR->Power[0] != 0. && pR->Power[0] != 0.);

	float dLevelChangeL = pL->Power[0] / pL->Power[1];
	float dLevelL = float(log(pL->Power[0]) * 0.5);
	float dLevelChangeR = pR->Power[0] / pR->Power[1];
	float dLevelR = float(log(pR->Power[0]) * 0.5);

	NoiseReductionCore * pNr = pL->pNr;

	// check if the signal may be considered stationary or transient
	if (dLevelChangeL + dLevelChangeR > pNr->m_ThresholdOfTransientAttack
		|| (dLevelChangeL + dLevelChangeR) / 2 < pNr->m_ThresholdOfTransientDecay)
	{
		// transient
		pL->m_TonalBand = false;
		pL->m_TransientBand = true;

		pR->m_TonalBand = false;
		pR->m_TransientBand = true;
		// signal in transient area

		pL->sp_AvgFreq = pL->sp_Freq;
		pL->sp_FilteredLevel = dLevelL;
		pL->sp_FilteredFreqError = 0;
		pL->sp_FilteredLevelError = 0;

		pL->sp_FreqDev -= pL->sp_FreqDev * pNr->m_FreqDevDecayRate;
		if (pL->sp_FreqDev < 1E-10)
		{
			pL->sp_FreqDev = 0;
		}
		pL->sp_LevelDev -= pNr->m_LevelDevDecayRate * pL->sp_LevelDev;
		if (fabs(pL->sp_LevelDev) < 1E-20)
		{
			pL->sp_LevelDev = 0;
		}
		pR->sp_AvgFreq = pR->sp_Freq;
		pR->sp_FilteredLevel = dLevelR;
		pR->sp_FilteredFreqError = 0;
		pR->sp_FilteredLevelError = 0;

		pR->sp_FreqDev -= pR->sp_FreqDev * pNr->m_FreqDevDecayRate;
		if (pR->sp_FreqDev < 1E-10)
		{
			pR->sp_FreqDev = 0;
		}
		pR->sp_LevelDev -= pNr->m_LevelDevDecayRate * pR->sp_LevelDev;
		if (fabs(pR->sp_LevelDev) < 1E-20)
		{
			pR->sp_LevelDev = 0;
		}
	}
	else
	{
#if 0
		// stationary signal, amplitude doesn't change
		pL->m_TransientBand = false;
		pR->m_TransientBand = false;
		sp_AvgFreq +=
			pNr->m_AvgFreqDecayRate * (sp_Freq - sp_AvgFreq);

		double FreqError = sp_Freq - sp_AvgFreq;
#if 0
		sp_FilteredFreqError += float(
									pNr->m_FreqErrorDecayRate * (FreqError - sp_FilteredFreqError));
		sp_FreqDev += float((sp_FilteredFreqError * sp_FilteredFreqError - sp_FreqDev)
							* pNr->m_FreqDevDecayRate);
		if (sp_FreqDev < 1E-10)
		{
			sp_FreqDev = 0;
		}
#else
		sp_FreqDev += float((FreqError * FreqError - sp_FreqDev)
							* pNr->m_FreqDevDecayRate);
		if (sp_FreqDev < 1E-10)
		{
			sp_FreqDev = 0;
		}
#endif
		// if sp_FreqDev is greater than threshold, then the signal is noise-like
		sp_AvgLevelChange +=
			pNr->m_AvgLevelDecayRate * (dLevelChange - sp_AvgLevelChange);
		if (fabs(sp_AvgLevelChange) < 1E-20)
		{
			sp_AvgLevelChange = 0;
		}

		sp_FilteredLevel +=
			pNr->m_AvgLevelDecayRate * (dLevel - sp_FilteredLevel);

		double LevelError = dLevel - sp_FilteredLevel;

		sp_FilteredLevelError += float(pNr->m_LevelErrorDecayRate *
										(LevelError - sp_FilteredLevelError));
		if (fabs(sp_FilteredLevelError) < 1E-20)
		{
			sp_FilteredLevelError = 0;
		}

		pL->sp_LevelDev += pNr->m_LevelDevDecayRate *
							(pL->sp_FilteredLevelError * pL->sp_FilteredLevelError - pL->sp_LevelDev);
		if (pL->sp_LevelDev < 1E-20)
		{
			pL->sp_LevelDev = 0;
		}

		pR->sp_LevelDev += pNr->m_LevelDevDecayRate *
							(pR->sp_FilteredLevelError * pR->sp_FilteredLevelError - pR->sp_LevelDev);
		if (pR->sp_LevelDev < 1E-20)
		{
			pR->sp_LevelDev = 0;
		}

		if (sp_FreqDev > pNr->m_FreqThresholdOfNoiselike)
		{
			m_TonalBand = false;
		}
		else
		{
			m_TonalBand = true;
#ifdef _DEBUG
			pNr->m_PhaseFilteredInBands++;
#endif
		}
#endif
	}
}

void SIGNAL_PARAMS::AnalyzeFftSample(complex<DATA> smp, int nSample)
{

	if ( ! PreprocessFftSample(smp, nSample))
	{
		return;
	}

	ASSERT(Power[0] != 0. && Power[0] != 0.);

	float dLevelChange = Power[0] / Power[1];
	float dLevel = float(log(Power[0]) * 0.5);

#ifdef _DEBUG
	if (pNr->m_MinLevelInBand > dLevel)
		pNr->m_MinLevelInBand = dLevel;
	if (pNr->m_MaxLevelInBand < dLevel)
		pNr->m_MaxLevelInBand = dLevel;

	if (MinLevelInBandNp > dLevel)
		MinLevelInBandNp = dLevel;
	if (MaxLevelInBandNp < dLevel)
		MaxLevelInBandNp = dLevel;
#endif

#if 1
	// check if the signal may be considered stationary or transient
	if (dLevelChange > pNr->m_ThresholdOfTransientAttack
		|| dLevelChange < pNr->m_ThresholdOfTransientDecay)
	{
		// transient signal by amplitude
		m_TonalBand = false;
		m_TransientBand = true;
#ifdef _DEBUG
		pNr->m_TransientBandFound++;
#endif
		// signal in transient area
		sp_AvgFreq = sp_Freq;
		sp_FilteredLevel = dLevel;
		sp_FilteredFreqError = 0;
		sp_FilteredLevelError = 0;

		sp_FreqDev -= sp_FreqDev * pNr->m_FreqDevDecayRate;
		if (sp_FreqDev < 1E-10)
		{
			sp_FreqDev = 0;
		}
		sp_LevelDev -= pNr->m_LevelDevDecayRate * sp_LevelDev;
		if (fabs(sp_LevelDev) < 1E-20)
		{
			sp_LevelDev = 0;
		}
	}
	else
	{
		// stationary signal, amplitude doesn't change
		m_TransientBand = false;
		sp_AvgFreq +=
			pNr->m_AvgFreqDecayRate * (sp_Freq - sp_AvgFreq);

		double FreqError = sp_Freq - sp_AvgFreq;
#if 0
		sp_FilteredFreqError += float(
									pNr->m_FreqErrorDecayRate * (FreqError - sp_FilteredFreqError));
		sp_FreqDev += float((sp_FilteredFreqError * sp_FilteredFreqError - sp_FreqDev)
							* pNr->m_FreqDevDecayRate);
		if (sp_FreqDev < 1E-10)
		{
			sp_FreqDev = 0;
		}
#else
		sp_FreqDev += float((FreqError * FreqError - sp_FreqDev)
							* pNr->m_FreqDevDecayRate);
		if (sp_FreqDev < 1E-10)
		{
			sp_FreqDev = 0;
		}
#endif
		// if sp_FreqDev is greater than threshold, then the signal is noise-like
		sp_AvgLevelChange +=
			pNr->m_AvgLevelDecayRate * (dLevelChange - sp_AvgLevelChange);
		if (fabs(sp_AvgLevelChange) < 1E-20)
		{
			sp_AvgLevelChange = 0;
		}

		sp_FilteredLevel +=
			pNr->m_AvgLevelDecayRate * (dLevel - sp_FilteredLevel);

		double LevelError = dLevel - sp_FilteredLevel;

		sp_FilteredLevelError += float(pNr->m_LevelErrorDecayRate *
										(LevelError - sp_FilteredLevelError));
		if (fabs(sp_FilteredLevelError) < 1E-20)
		{
			sp_FilteredLevelError = 0;
		}

		sp_LevelDev += pNr->m_LevelDevDecayRate *
						(sp_FilteredLevelError * sp_FilteredLevelError - sp_LevelDev);
		if (sp_LevelDev < 1E-20)
		{
			sp_LevelDev = 0;
		}

		if (sp_FreqDev > pNr->m_FreqThresholdOfNoiselike)
		{
			m_TonalBand = false;
		}
		else
		{
			m_TonalBand = true;
#ifdef _DEBUG
			pNr->m_PhaseFilteredInBands++;
#endif
		}

	}

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
	bool m_PrerollSamplesSaved;
	bool m_bPassThrough;        // pass the data through without change
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
	std::vector<SIGNAL_PARAMS> m_pParams;

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

		std::fill(m_pParams.begin(), m_pParams.end(), SIGNAL_PARAMS(pNr));

		m_nSamplesReceived = 0;
		m_nSamplesStored = 0;
		m_PrerollSamplesSaved = FALSE;
		m_FftResultsProcessed = 0;
	}

	SIGNAL_PARAMS * GetSignalParams() { return & m_pParams.front(); }

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
	m_AvgFreqDecayRate(0.3f),
	m_AvgLevelDecayRate(0.2f),
	m_FreqErrorDecayRate(0.3f),
	m_LevelErrorDecayRate(0.5),
	m_FreqDevDecayRate(0.2f),
	m_LevelDevDecayRate(0.2f),
	m_ThresholdOfTransientAttack(2.f),
	m_ThresholdOfTransientDecay(0.3f),
	m_FreqThresholdOfNoiselike(float(M_PI * M_PI / (4. * 4))),
	m_LevelThresholdForNoiseLow(8.),   // Nepers
	m_LevelThresholdForNoiseHigh(8.),   // Nepers
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

	TRACE(" %*.sThresholdOfTransientAttack=%f, FreqThresholdOfNoiselike=%f\n",
		indent, "", m_ThresholdOfTransientAttack, m_FreqThresholdOfNoiselike);

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
		NoiseReductionChannelData * pCh = m_ChannelData[ch];
		auto pParms = pCh->m_pParams.cbegin();

		for (unsigned i = 0; i < m_nFftOrder; i++, pParms++, pBuf++)
		{
			*pBuf = pParms->sp_MaskingPower;
		}
	}
}

void NoiseReductionCore::GetNoiseThreshold(DATA * pBuf) // precomputed threshold, nChannels *FftOrder count
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		for (unsigned i = 0; i < m_nFftOrder; i++, pBuf++)
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
		SIGNAL_PARAMS const * pParms = & pCh->m_pParams.front();
		DATA * pDst = pBuf + ch;

		for (unsigned i = 0; i < m_nFftOrder; i++, pParms++, pDst += m_nChannels)
		{
			*pDst = pParms->Power[0];
		}
	}
}
// FFT power of the output
void NoiseReductionCore::GetResultPowerInBands(DATA * pBuf)  // nChannels * FftOrder
{
	for (int ch = 0; ch < m_nChannels; ch++)
	{
		NoiseReductionChannelData * pCh = m_ChannelData[ch];
		SIGNAL_PARAMS const * pParms = & pCh->m_pParams.front();

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

bool NoiseReductionCore::IsTonalBand(int ch, int f) const
{
	return m_ChannelData[ch]->m_pParams[f].m_TonalBand;
}

////////////////////////////////////////////////////////////////////////
///////////////  CNoiseReduction
CNoiseReduction::CNoiseReduction(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess, unsigned nFftOrder, NoiseReductionParameters const & nr)
	: m_pNrCore(NULL)
	, m_NrParms(nr)
	, m_FftOrder(nFftOrder)
{
	m_InputSampleType = SampleTypeFloat32;
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

BOOL CNoiseReduction::SetInputWaveformat(CWaveFormat const & Wf)
{
	if (! CWaveProc::SetInputWaveformat(Wf))
	{
		return FALSE;
	}

	delete m_pNrCore;
	m_pNrCore = NULL;

	m_pNrCore = new NoiseReductionCore(m_FftOrder, Wf.NumChannels(), Wf.SampleRate(), m_NrParms, m_ChannelsToProcess);
	return TRUE;
}

unsigned CNoiseReduction::ProcessSoundBuffer(char const * pIn, char * pOut,
											unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
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
	, m_pParams(nFftOrder + 1, SIGNAL_PARAMS(nr))
	, m_nSamplesReceived(0)
	, m_nSamplesStored(0)
	, m_PrerollSamplesSaved(FALSE)
	, m_bPassThrough(PassThrough != FALSE)
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
		m_pParams[f].AnalyzeFftSample(m_FftOutBuffer[f], f);
		if (f == 1487)//f >= 16000 * m_FftOrder / 22050 -2 && f <= 16000 * m_FftOrder / 22050+2)
		{
			m_pParams[f].Dump(f);
		}
	}
}

void NoiseReductionChannelData::AccumulateSubbandPower(float SubbandPower[FAR_MASKING_GRANULARITY])
{
	int const BandsPerMaskGranule = m_FftOrder / FAR_MASKING_GRANULARITY;
	SIGNAL_PARAMS * p = GetSignalParams();

	for (int n = 0; n < FAR_MASKING_GRANULARITY; n++)
	{
		for (int k = 0; k < BandsPerMaskGranule; k++, p++)
		{
			SubbandPower[n] += p->Power[0];
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
		//ASSERT(OutputDataBuffer[0] <= 32767. && OutputDataBuffer[0] >= -32768.);

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

				//ASSERT(tmp <= 32767. && tmp >= -32768.);

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
			TotalPower1 += m_pParams[f].Power[0];
			TotalPower2 += m_pParams[f].sp_MaskingPower;
			if (MaxBandPower1 < m_pParams[f].Power[0])
			{
				MaxBandPower1 = m_pParams[f].Power[0];
			}
			if (MaxBandPower2 < m_pParams[f].sp_MaskingPower)
			{
				MaxBandPower2 = m_pParams[f].sp_MaskingPower;
			}
			if (MinBandPower1 > m_pParams[f].Power[0])
			{
				MinBandPower1 = m_pParams[f].Power[0];
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

	SIGNAL_PARAMS * p = GetSignalParams();

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

	SIGNAL_PARAMS * p = GetSignalParams();

	for (int n = 0; n < FAR_MASKING_GRANULARITY; n++)
	{
		for (int k = 0; k < BandsPerMaskGranule; k++, p++)
		{
			p->sp_MaskingPower = float(p->Power[0] + FarMaskingFactor * FarMasking[n]);
		}
	}
	// last FFT term
	p->sp_MaskingPower = float(p->Power[0] + FarMaskingFactor * FarMasking[FAR_MASKING_GRANULARITY - 1]);
}

void NoiseReductionChannelData::CalculateMasking(double MaskingSpectralDecayNormLow,
												double MaskingSpectralDecayNormHigh, double ToneEmphasis)
{
	double const MaskingDistanceDelta =
		(MaskingSpectralDecayNormHigh - MaskingSpectralDecayNormLow) / (m_FftOrder);

	SIGNAL_PARAMS * p = GetSignalParams() + m_FftOrder;

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

	p = GetSignalParams();
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
	SIGNAL_PARAMS * p = GetSignalParams();
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
	for (item_iterator i = m_Stages.begin(); i != m_Stages.end(); i++)
	{
		if (m_bAutoDeleteProcs)
		{
			delete i->Proc;
		}
	}
}

BOOL CBatchProcessing::WasClipped() const
{
	for (item_iterator_const i = m_Stages.begin(); i != m_Stages.end(); i++)
	{
		if (i->Proc->WasClipped())
		{
			return true;
		}
	}
	return false;
}

double CBatchProcessing::GetMaxClipped() const
{
	double MaxClipped = 0;
	for (item_iterator_const i = m_Stages.begin(); i != m_Stages.end(); i++)
	{
		double StageClip = i->Proc->GetMaxClipped();
		if (MaxClipped < StageClip)
		{
			MaxClipped = StageClip;
		}
	}
	return MaxClipped;
}

BOOL CBatchProcessing::SetInputWaveformat(CWaveFormat const & Wf)
{
	m_InputFormat = Wf;
	m_InputSampleType = Wf.GetSampleType();

	if (m_Stages.empty())
	{
		m_OutputFormat = m_InputFormat;
		return TRUE;
	}

	if ( ! m_Stages.begin()->Proc->SetInputWaveformat(Wf))
	{
		return FALSE;
	}

	for (item_iterator i = m_Stages.begin(); i+1 != m_Stages.end(); i++)
	{
		if (! i[1].Proc->SetInputWaveformat(i->Proc->GetOutputWaveformat()))
		{
			return FALSE;
		}
	}
	// set output format
	m_OutputFormat = m_Stages.rbegin()->Proc->GetOutputWaveformat();
	return TRUE;
}

BOOL CBatchProcessing::SetOutputWaveformat(CWaveFormat const & Wf)
{
	m_OutputFormat = Wf;
	m_OutputSampleType = Wf.GetSampleType();
	return TRUE;
}

unsigned CBatchProcessing::ProcessSoundBuffer(char const * pIn, char * pOut,
											unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	unsigned nSavedBytes = 0;
	*pUsedBytes = 0;

	if (m_Stages.empty())
	{
		// just pass through
		unsigned ToCopy = std::min(nInBytes, nOutBytes);
		memcpy(pOut, pIn, ToCopy);
		* pUsedBytes = ToCopy;
		return ToCopy;
	}

	bool bDataWasProcessed;
	do
	{
		bDataWasProcessed = false;
		for (item_iterator pPrevItem, pItem = m_Stages.begin(); pItem != m_Stages.end(); pItem++)
		{
			unsigned nCurrInputBytes;
			// prepare input buffer. Do not use shortcut optimization - much pain, little gain

			if (pItem != m_Stages.begin())
			{
				nCurrInputBytes = pItem->FillInputBuffer(pPrevItem->OutBuf + pPrevItem->OutBufGetIndex,
														pPrevItem->OutBufPutIndex - pPrevItem->OutBufGetIndex,
														&pPrevItem->Proc->GetOutputWaveformat());

				pPrevItem->OutBufGetIndex += nCurrInputBytes;
				if (pPrevItem->OutBufGetIndex == pPrevItem->OutBufPutIndex)
				{
					pPrevItem->OutBufGetIndex = 0;
					pPrevItem->OutBufPutIndex = 0;
				}
			}
			else
			{
				if (m_BackwardPass)
				{
					// swap order of samples
					unsigned SampleSize = GetInputWaveformat().SampleSize();
					// pIn points to the end of the buffer
					while (nInBytes != 0)
					{
						nCurrInputBytes = pItem->FillInputBuffer(pIn - SampleSize, nInBytes, &GetInputWaveformat());

						if (0 == nCurrInputBytes)
						{
							break;
						}
						pIn -= nCurrInputBytes;
						nInBytes -= nCurrInputBytes;
						*pUsedBytes += nInBytes;
					}
				}
				else
				{
					nCurrInputBytes = pItem->FillInputBuffer(pIn, nInBytes, &GetInputWaveformat());
					pIn += nCurrInputBytes;
					nInBytes -= nCurrInputBytes;
					*pUsedBytes += nCurrInputBytes;
				}

			}

			pPrevItem = pItem;
			const char * inbuf = pItem->InBuf+pItem->InBufGetIndex;

			if (pIn == NULL && pItem->InBufPutIndex == pItem->InBufPutIndex)
			{
				// flushing this
				inbuf = NULL;
			}

			unsigned BytesToStore = 0;
			char * outbuf = NULL;

			if (pItem + 1 != m_Stages.end()
				|| NULL != pOut)
			{
				outbuf = pItem->OutBuf + pItem->OutBufPutIndex;
				BytesToStore = IntermediateBufSize - pItem->OutBufPutIndex;
			}

			unsigned nProcessedBytes = 0;
			unsigned nOutputBytes = pItem->Proc->ProcessSound(inbuf,
															outbuf,
															pItem->InBufPutIndex - pItem->InBufGetIndex,
															BytesToStore, & nProcessedBytes);
			ASSERT(nOutputBytes <= BytesToStore);
			ASSERT(nProcessedBytes <= pItem->InBufPutIndex - pItem->InBufGetIndex);

			if (nOutputBytes != 0
				|| nProcessedBytes != 0)
			{
				bDataWasProcessed = true;
			}

			pItem->InBufGetIndex += nProcessedBytes;
			if (pItem->InBufGetIndex == pItem->InBufPutIndex)
			{
				pItem->InBufGetIndex = 0;
				pItem->InBufPutIndex = 0;
			}
			pItem->OutBufPutIndex += nOutputBytes;

			if (pItem + 1 == m_Stages.end())
			{
				// now pull the data from the output buffer
				if (m_BackwardPass)
				{
					unsigned SampleSize = GetOutputWaveformat().SampleSize();
					while (nOutBytes != 0)
					{
						nOutputBytes = pItem->FillOutputBuffer(pOut - SampleSize, nOutBytes, &GetOutputWaveformat());
						if (nOutputBytes == 0)
						{
							break;
						}
						nSavedBytes += nOutputBytes;
						nOutBytes -= nOutputBytes;
						pOut -= nOutputBytes;
					}
				}
				else
				{
					nOutputBytes = pItem->FillOutputBuffer(pOut, nOutBytes, &GetOutputWaveformat());
					nSavedBytes += nOutputBytes;
					nOutBytes -= nOutputBytes;
					pOut += nOutputBytes;
				}
			}
			else if (bDataWasProcessed && pItem->OutBufPutIndex == pItem->OutBufGetIndex)
			{
				// even though some data was processed, there was not enough data to add anything for the next stage
				if (pItem[1].InBufGetIndex == pItem[1].InBufPutIndex)
				{
					// next input buffer is empty, too. Shortcut to the begin of the list
					break;
				}
			}
		}

	} while (bDataWasProcessed);

	return nSavedBytes;
}

// if input data is compressed and not sample-aligned, this could be 0
// it can be multiple of block size for compressed format
unsigned CBatchProcessing::GetInputSampleSize() const
{
	return m_InputFormat.SampleSize();
}

// if output data is compressed and not sample-aligned, this could be 0
// it can be multiple of block size for compressed format
unsigned CBatchProcessing::GetOutputSampleSize() const
{
	return m_OutputFormat.SampleSize();
}

void CBatchProcessing::AddWaveProc(CWaveProc * pProc, int index)
{
	if (-1 == index || index > (int)m_Stages.size())
	{
		index = (int)m_Stages.size();
	}
	m_Stages.insert(m_Stages.begin()+index, Item(pProc));
}

void CBatchProcessing::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);
	for (item_iterator_const pItem = m_Stages.begin(); pItem != m_Stages.end(); pItem++)
	{
		pItem->Proc->Dump(indent + 1);
	}
}

BOOL CBatchProcessing::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

	for (item_iterator pItem = m_Stages.begin(); pItem != m_Stages.end(); pItem++)
	{
		if ( ! pItem->Proc->Init())
		{
			return FALSE;
		}
		pItem->InBufGetIndex = 0;
		pItem->InBufPutIndex = 0;
		pItem->OutBufPutIndex = 0;
		pItem->OutBufPutIndex = 0;
	}

	return TRUE;
}

bool CBatchProcessing::SetChannelsToProcess(CHANNEL_MASK channels)
{
	for (item_iterator pItem = m_Stages.begin(); pItem != m_Stages.end(); pItem++)
	{
		if ( ! pItem->Proc->SetChannelsToProcess(channels))
		{
			return false;
		}
	}
	return true;
}

void CBatchProcessing::DeInit()
{
	for (std::vector<Item>::reverse_iterator pItem = m_Stages.rbegin(); pItem != m_Stages.rend(); pItem++)
	{
		pItem->Proc->DeInit();
	}
	BaseClass::DeInit();
}

NUMBER_OF_SAMPLES CBatchProcessing::GetInputNumberOfSamples() const
{
	if (m_Stages.empty())
	{
		return 0;
	}

	return m_Stages[0].Proc->GetInputNumberOfSamples();
}

NUMBER_OF_SAMPLES CBatchProcessing::GetOutputNumberOfSamples() const
{
	if (m_Stages.empty())
	{
		return 0;
	}

	return m_Stages[m_Stages.size() - 1].Proc->GetOutputNumberOfSamples();
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
	// arg is from -0.5 to +0.5
	double Window;
	double x = M_PI * (1. + 2 * arg);
	if (arg < -0.5 || arg > 0.5)
	{
		return 0.;
	}

	// x goes from 0 to 2pi
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
		return sin(arg) / arg;
	}
	else
	{
		return 1.;
	}
}

double CResampleFilter::ResampleFilterTap(double arg, double FilterLength)
{
	// arg goes from -0.5 to +0.5
	// FilterLength is number of sin periods in the filter
	return sinc(arg * FilterLength * M_PI) * FilterWindow(arg);
}

static unsigned long GreatestCommonFactor(unsigned long x1, unsigned long x2)
{
	ASSERT(0 != x2);
	ASSERT(0 != x1);
	while (1)
	{
		unsigned long remainder = x1 % x2;
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
	// the filter arrays correspond to output sample positions
	// NumberOfFilterTables of them are generated
	m_bUseInterpolatedFilter = FALSE;
	// use fixed coefficients
	m_FilterArraySize = NumberOfFilterTables * m_SamplesInFilter;
	m_FilterTable.Allocate(m_FilterArraySize);

	double * p = m_FilterTable;
	signed InputOffset = 0;
	signed Accumulator = 0;
	double NumSincWaves = (FilterLength - 1) * 2;

	for (unsigned i = 0; i < NumberOfFilterTables; i++)
	{
		//TRACE("i=%d, InputOffset=%d\n", i, InputOffset);

		for (unsigned j = 0; j < m_SamplesInFilter; j++, p++)
		{
			double arg = double(j + 0.5 +
					(InputOffset - double(i) * m_InputFormat.SampleRate() / m_EffectiveOutputSampleRate)) / m_SamplesInFilter - 0.5;
			//ASSERT(arg >= -0.5 && arg <= 0.5);
			*p = ResampleFilterTap(arg, NumSincWaves);

			if (0) TRACE("Filter[%d][%d]=%f\n", i, j, *p);
		}

		Accumulator += m_InputFormat.SampleRate();
		while (Accumulator > 0)
		{
			Accumulator -= m_EffectiveOutputSampleRate;
			InputOffset++;      // go to the next input sample
		}
	}

	ASSERT(Accumulator == 0);
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
	double Sum = 0.;
	double SumSq = 0.;
	for (unsigned i = 2; i < m_DstBufUsed; i++)
	{
		Sum += m_pDstBuf[i];
		SumSq += m_pDstBuf[i] * m_pDstBuf[i];
		if (Max < m_pDstBuf[i])
		{
			Max = m_pDstBuf[i];
		}
	}

	TRACE("Induced filter noise for DC input=%f\n", sqrt((SumSq * (m_DstBufUsed - 2) - Sum*Sum) / Sum*Sum));

	for (unsigned i = 0; i < NumberOfFilterTables * m_SamplesInFilter; i++)
	{
		m_FilterTable[i] /= Max;
	}


	// prefill at filter length
	for (int i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 0.;
	}

	m_SrcBufFilled = m_SamplesInFilter / 2 * m_InputFormat.NumChannels();

	ResetResample();
}

void CResampleFilter::InitSlidingInterpolatedFilter(int FilterLength)
{
	m_bUseInterpolatedFilter = TRUE;

	// number of source taps should be m_SamplesInFilter
	if (1 || m_EffectiveOutputSampleRate >= m_InputFormat.SampleRate())
	{
		// upsampling.
		//
		double InputPeriod = 4294967296. / m_SamplesInFilter; // 0x100000000LL
		m_InputPeriod = (unsigned __int32)(InputPeriod);
		m_OutputPeriod = (unsigned __int32)(InputPeriod * m_InputFormat.SampleRate() / m_EffectiveOutputSampleRate);
	}
	else
	{
		// downsampling
		double OutputPeriod = 4294967296. / (FilterLength + 1.); //0x100000000LL
		m_OutputPeriod = (unsigned __int32)(OutputPeriod);
		m_InputPeriod = (unsigned __int32)(OutputPeriod * m_EffectiveOutputSampleRate / m_InputFormat.SampleRate());
	}
	//TRACE("InputPeriod=%08x, OutputPeriod=%08x\n", m_InputPeriod, m_OutputPeriod);
#ifdef _DEBUG
	double MaxErr = 0;
#endif
	m_InterpolatedFilterTable.Allocate(ResampleFilterSize);
	double NumSincWaves = (FilterLength - 1)* 2.;
	// use sliding squared interpolation
	for (signed i = 0; i < ResampleFilterSize; i++)
	{
#ifndef USE_CUBIC_INTERPOLATION

		double arg = (i + 0.5) / ResampleFilterSize - 0.5;
		double arg05 = (i + 1.) / ResampleFilterSize - 0.5;
		double arg1 = (i + 1.5) / ResampleFilterSize - 0.5;

		// arg should be from -0.5 to 0.5
		double val = ResampleFilterTap(arg, NumSincWaves);
		double val05 = ResampleFilterTap(arg05, NumSincWaves);
		double val1 = ResampleFilterTap(arg1, NumSincWaves);

		double dif1 = val05 - val;
		double dif2 = val1 - val05;

		m_InterpolatedFilterTable[i].tap = val;
		m_InterpolatedFilterTable[i].deriv1 = (3. * dif1 - dif2) / 2. / (1 << (ResampleIndexShift - 1));
		m_InterpolatedFilterTable[i].deriv2 = (dif2 - dif1) / 2. / (1 << (ResampleIndexShift - 1)) / (1 << (ResampleIndexShift - 1));
		m_InterpolatedFilterTable[i].deriv3 = 0.;

		TRACE("[%03d] Window=%f, sinc=%f, Resample filter=%.9f, next extrapolated=%.9f\n",
			i, FilterWindow(arg), sinc(arg * FilterLength * M_PI), m_InterpolatedFilterTable[i].tap,
			m_InterpolatedFilterTable[i].tap + (1 << ResampleIndexShift) *
			(m_InterpolatedFilterTable[i].deriv1 + (1 << ResampleIndexShift) * m_InterpolatedFilterTable[i].deriv2));
#else
		double arg = (i + 0.5) / ResampleFilterSize - 0.5;
		double arg1 = (i + 0.8333333333333) / ResampleFilterSize - 0.5;
		double arg2 = (i + 1.1666666666666) / ResampleFilterSize - 0.5;
		double arg3 = (i + 1.5) / ResampleFilterSize - 0.5;

		// arg should be from -0.5 to 0.5
		double val0 = ResampleFilterTap(arg, NumSincWaves);
		double val1 = ResampleFilterTap(arg1, NumSincWaves);
		double val2 = ResampleFilterTap(arg2, NumSincWaves);
		double val3 = ResampleFilterTap(arg3, NumSincWaves);

		double dif30 = val3 - val0;
		double dif10 = val1 - val0;
		double dif21 = val2 - val1;

		m_InterpolatedFilterTable[i].tap = val0;
		m_InterpolatedFilterTable[i].deriv1 = (dif30 + 4.5 *(dif10 - dif21)) / (1 << ResampleIndexShift);
		m_InterpolatedFilterTable[i].deriv2 = (4 * dif21 - (dif10 + dif30)) * 4.5 / (1 << ResampleIndexShift) / (1 << ResampleIndexShift);
		m_InterpolatedFilterTable[i].deriv3 = (dif30 - 3* dif21) * 4.5 / (1 << ResampleIndexShift) / (1 << ResampleIndexShift) / (1 << ResampleIndexShift);
#endif

#ifndef USE_CUBIC_INTERPOLATION
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
#ifdef USE_CUBIC_INTERPOLATION
		m_InterpolatedFilterTable[i].deriv3 /= Max;
#endif
	}


	for (int i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 0.;
	}
	// prefill at 1/2 filter length
	m_SrcBufFilled = (0x80000000u / m_InputPeriod) * m_InputFormat.NumChannels();

	ResetResample();
}

BOOL CResampleFilter::SetInputWaveformat(CWaveFormat const &
#ifdef _DEBUG
										Wf
#endif
										)
{
	ASSERT(Wf == m_InputFormat);
	return TRUE;
}

void CResampleFilter::InitResample(long OriginalSampleRate, long NewSampleRate,
									int FilterLength, NUMBER_OF_CHANNELS nChannels, BOOL KeepSamplesPerSec)
{
	// FilterLength is how many Sin periods are in the array.
	//
	m_EffectiveOutputSampleRate = NewSampleRate;

	m_InputFormat.InitFormat(WAVE_FORMAT_PCM, OriginalSampleRate, nChannels);
	m_OutputFormat.InitFormat(WAVE_FORMAT_PCM,
							KeepSamplesPerSec ? OriginalSampleRate : NewSampleRate,
							nChannels);

	// the cutoff frequency is the half of lower sampling rate, minus 1/filter_length
	// find greatest common factor of the sampling rates
	unsigned long common = GreatestCommonFactor(NewSampleRate, OriginalSampleRate);
	// after this number of output samples, the filter coefficients repeat
	unsigned long NumberOfFilterTables = NewSampleRate / common;

	// this is number of taps in each filter
	m_SamplesInFilter = (FilterLength + 1) * 2;

	if (NewSampleRate < OriginalSampleRate)
	{
		// when downsampling, the filter number of sin periods is in new sample rate times.
		// Thus, the filter needs to be longer
		m_SamplesInFilter = MulDiv(m_SamplesInFilter, OriginalSampleRate, NewSampleRate);
	}

	if (NumberOfFilterTables * m_SamplesInFilter <= MaxNumberOfFilterSamples)
	{
		// we can use precomputed exact filters
		// the filter arrays correspond to output sample positions
		// NumberOfFilterTables of them are generated
		InitSlidingFilter(FilterLength, NumberOfFilterTables);
	}
	else
	{
		// we need to use a sliding filter
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
	for (i = m_DstBufUsed; m_SrcBufUsed + FilterLength <= m_SrcBufFilled
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
			ASSERT(src + j + NumChannels <= m_pSrcBuf + m_SrcBufFilled);

			int const TableIndex = Phase1 >> ResampleIndexShift;
			double PhaseFraction = int(Phase1 & ~(0xFFFFFFFF << ResampleIndexShift));

#ifndef USE_CUBIC_INTERPOLATION
			double const coeff = (pTable[TableIndex].tap +
									PhaseFraction * (pTable[TableIndex].deriv1
										+ PhaseFraction * pTable[TableIndex].deriv2));
#else
			double const coeff = (pTable[TableIndex].tap +
									PhaseFraction * (pTable[TableIndex].deriv1
										+ PhaseFraction * (pTable[TableIndex].deriv2
											+ PhaseFraction * pTable[TableIndex].deriv3)));
#endif
			for (unsigned ch = 0; ch < NumChannels; ch++)
			{
				ASSERT(src + j + ch < m_pSrcBuf + m_SrcBufFilled);
				OutSample[ch] += src[j + ch] * coeff;
			}

			unsigned __int32 Phase2 = Phase1 + m_InputPeriod;
			if (Phase2 < Phase1)   // overflow
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

	if (m_SrcBufFilled <= m_SrcBufUsed + NumChannels * m_SamplesInFilter)
	{
		return;
	}

	long SrcSamples = (long)(m_SrcBufFilled - m_SrcBufUsed);
	int const FilterLength = (int)(NumChannels * m_SamplesInFilter);

	float * dst = m_pDstBuf + m_DstBufUsed;

	long const InputSampleRate = m_InputFormat.SampleRate();
	long const OutputSampleRate = m_EffectiveOutputSampleRate;

	unsigned i;
	for (i = m_DstBufUsed; SrcSamples >= FilterLength && i < DstBufSize; i += NumChannels)
	{
		double const * p = m_FilterIndex + m_FilterTable;

		const float * src = m_pSrcBuf + m_SrcBufUsed;
		for (unsigned ch = 0; ch < NumChannels; ch++, dst++)
		{
			double OutSample = 0.;
			float const * FilterSrc = src + ch;

			for (unsigned j = 0; j != m_SamplesInFilter; j++, FilterSrc += NumChannels)
			{
				ASSERT(FilterSrc < m_pSrcBuf + m_SrcBufFilled);
				OutSample += *FilterSrc * p[j];
			}

			*dst = float(OutSample);
			ASSERT(dst < m_pDstBuf + DstBufSize);
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

unsigned CResampleFilter::ProcessSoundBuffer(char const * pIn, char * pOut,
											unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
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

BOOL CAudioConvertor::SetInputWaveformat(CWaveFormat const &
#ifdef _DEBUG
										Wf
#endif
										)
{
	ASSERT(Wf == m_InputFormat);
	return TRUE;
}

unsigned CAudioConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
											unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	unsigned nSavedBytes = 0;
	*pUsedBytes = 0;

	while (nOutBytes != 0 || nInBytes != 0)
	{
		// empty the output buffer
		unsigned const WasCopied = m_AcmConvertor.GetConvertedData(pOut, nOutBytes);

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
		unsigned InUsed = 0;
		unsigned OutUsed = 0;

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
unsigned CAudioConvertor::GetInputSampleSize() const
{
	return m_InputSampleSize;
}

// if input data is compressed and not sample-aligned, this could be 0
unsigned CAudioConvertor::GetOutputSampleSize() const
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
CChannelConvertor::CChannelConvertor(NUMBER_OF_CHANNELS NewChannels)
	: m_NewChannels(NewChannels)
{
}

BOOL CChannelConvertor::SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK /*channels*/)
{
	CWaveFormat NewFormat;
	NewFormat.InitFormat(Wf.GetSampleType(), Wf.SampleRate(), m_NewChannels);

	return BaseClass::SetInputWaveformat(NewFormat, ALL_CHANNELS);
}

unsigned CChannelConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
												unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	unsigned nSavedBytes = 0;
	*pUsedBytes = 0;

	int nInSamples = nInBytes / m_InputFormat.SampleSize();
	int nOutSamples = nOutBytes / m_OutputFormat.SampleSize();

	int nSamples = std::min(nInSamples, nOutSamples);

	CopyWaveSamples(pOut, ALL_CHANNELS, m_OutputFormat.NumChannels(),
					pIn, m_ChannelsToProcess, m_InputFormat.NumChannels(),
					nSamples, m_OutputFormat.GetSampleType(), m_InputFormat.GetSampleType());

	nSavedBytes = nSamples * m_OutputFormat.SampleSize();
	*pUsedBytes = nSamples * m_InputFormat.SampleSize();
	return nSavedBytes;
}

CLameEncConvertor::~CLameEncConvertor()
{
	DeInit();
}

BOOL CLameEncConvertor::SetFormat(WAVEFORMATEX const * pWF)
{
	m_OutputFormat = pWF;
	m_OutputFormat.SampleSize() = 1;
	return TRUE;
}

unsigned CLameEncConvertor::ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
												unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	// save extra data from the output buffer
	*pUsedBytes = 0;
	unsigned nSavedBytes = 0;
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
			unsigned ToCopy = __min(nOutBytes, m_OutputBufferFilled);
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

		unsigned ToCopy = m_InputBufferSize - m_InputBufferFilled;
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
							m_InputBufferFilled / (sizeof (short) /* * m_InputFormat.NumChannels() */),
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
	BE_CONFIG cfg = {0};

	cfg.dwConfig = BE_CONFIG_LAME;
	cfg.format.LHV1.dwStructVersion = 1;
	cfg.format.LHV1.dwStructSize = sizeof cfg;
	cfg.format.LHV1.dwSampleRate = m_OutputFormat.SampleRate();
	cfg.format.LHV1.nMode = BE_MP3_MODE_MONO;
	if (m_OutputFormat.NumChannels() > 1)
	{
		cfg.format.LHV1.nMode = BE_MP3_MODE_STEREO;
	}
	cfg.format.LHV1.dwBitrate = m_OutputFormat.BytesPerSec() / (1000 / 8);

	cfg.format.LHV1.bCRC = TRUE;
	cfg.format.LHV1.nPreset = LQP_VERYHIGH_QUALITY;

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
unsigned CByteSwapConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
												unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes)
{
	if (NULL == pIn)
	{
		*pUsedBytes = 0;
		return 0;
	}

	unsigned nBytes = __min(nInBytes, nOutBytes);
	for (unsigned i = 0; i < nBytes; i+= sizeof (WAVE_SAMPLE))
	{
		pOut[i + 1] = pIn[i];
		pOut[i] = pIn[i + 1];
	}
	*pUsedBytes = nBytes;
	return nBytes;
}

/////////////////////////////////////////////////////////////////////////////////////////////
///////// CFilterProc
///////////////////////////////
BOOL CFilterProc::Init()
{

	for (unsigned ch = 0; ch < countof(m_PrevLpfSamples); ch++)
	{
		for (int i = 0; i < MaxFilterOrder; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_PrevLpfSamples[ch][i][j] = 0.;

				m_PrevHpfSamples[ch][i][j] = 0.;

				m_PrevNotchSamples[ch][i][j] = 0.;
			}
		}
	}
	return TRUE;
}

double CFilterProc::CalculateResult(unsigned ch, double Input)
{
	double in = Input;
	if (0 != m_nLpfOrder)
	{
		double out = 0;
		for (int i = 0; i < m_nLpfOrder; i++)
		{
			double tmp = in * m_LpfCoeffs[i][0]
						+ m_PrevLpfSamples[ch][i][0] * m_LpfCoeffs[i][1]
						+ m_PrevLpfSamples[ch][i][1] * m_LpfCoeffs[i][2]
						- m_PrevLpfSamples[ch][i][2] * m_LpfCoeffs[i][4]
						- m_PrevLpfSamples[ch][i][3] * m_LpfCoeffs[i][5];

			// protect against underflow (it may slow the calculations on some platforms)
			if (fabs(tmp) < 1E-10)
			{
				tmp = 0;
			}

			m_PrevLpfSamples[ch][i][1] = m_PrevLpfSamples[ch][i][0];
			m_PrevLpfSamples[ch][i][0] = in;
			m_PrevLpfSamples[ch][i][3] = m_PrevLpfSamples[ch][i][2];
			m_PrevLpfSamples[ch][i][2] = tmp;
			out += tmp;
		}
		in = out;
	}
	if (0 != m_nHpfOrder)
	{
		double out = 0;
		for (int i = 0; i < m_nHpfOrder; i++)
		{
			double tmp = in * m_HpfCoeffs[i][0]
						+ m_PrevHpfSamples[ch][i][0] * m_HpfCoeffs[i][1]
						+ m_PrevHpfSamples[ch][i][1] * m_HpfCoeffs[i][2]
						- m_PrevHpfSamples[ch][i][2] * m_HpfCoeffs[i][4]
						- m_PrevHpfSamples[ch][i][3] * m_HpfCoeffs[i][5];

			// protect against underflow (it slows the calculations tremendously)
			if (fabs(tmp) < 1E-10)
			{
				tmp = 0;
			}

			m_PrevHpfSamples[ch][i][1] = m_PrevHpfSamples[ch][i][0];
			m_PrevHpfSamples[ch][i][0] = in;
			m_PrevHpfSamples[ch][i][3] = m_PrevHpfSamples[ch][i][2];
			m_PrevHpfSamples[ch][i][2] = tmp;
			out += tmp;
		}
		in = out;
	}
	if (0 != m_nNotchOrder)
	{
		for (int i = 0; i < m_nNotchOrder; i++)
		{
			double tmp = in * m_NotchCoeffs[i][0]
						+ m_PrevNotchSamples[ch][i][0] * m_NotchCoeffs[i][1]
						+ m_PrevNotchSamples[ch][i][1] * m_NotchCoeffs[i][2]
						- m_PrevNotchSamples[ch][i][2] * m_NotchCoeffs[i][4]
						- m_PrevNotchSamples[ch][i][3] * m_NotchCoeffs[i][5];

			// protect against underflow (it slows the calculations tremendously)
			if (fabs(tmp) < 1E-10)
			{
				tmp = 0;
			}

			m_PrevNotchSamples[ch][i][1] = m_PrevNotchSamples[ch][i][0];
			m_PrevNotchSamples[ch][i][0] = in;
			m_PrevNotchSamples[ch][i][3] = m_PrevNotchSamples[ch][i][2];
			m_PrevNotchSamples[ch][i][2] = tmp;
			in = tmp;
		}
	}
	return in;
}

void CFilterProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel)
{
	*(float*)pOutSample = (float)CalculateResult(channel, *(float const*)pInSample);
}
