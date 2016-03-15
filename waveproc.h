// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// waveproc.h
#ifndef __WAVEPROC_H_
#define __WAVEPROC_H_

#include <complex>
#include <afxtempl.h>
#include <mmreg.h>
#include <msacm.h>
#include "BladeMP3EncDll.h"
#include "WaveSupport.h"
#include "FilterMath.h"

//using namespace std;

template <class T_ext, class T_int=T_ext> class CBackBuffer
{
public:
	CBackBuffer();
	~CBackBuffer();
	T_ext Get(int index);
	void Put(int index, T_ext data);
	BOOL Allocate(int size);
	int GetCounter() { return nCurrIndex; }
	void SetCounter(int index) { nCurrIndex = index; }
	void Advance(int n = 1)
	{
		nCurrIndex += n;
	}
	operator bool() {return pBuf != 0; }
	T_int& operator[](int index);
private:
	T_int * pBuf;
	int BufSize;
	int nCurrIndex;
	DWORD dwIndexMask;
};

template <class T_ext, class T_int>
CBackBuffer<T_ext, T_int>::CBackBuffer()
	: pBuf(NULL),
	BufSize(0),nCurrIndex(0),
	dwIndexMask(0)
{
}

template <class T_ext, class T_int>
CBackBuffer<T_ext, T_int>::~CBackBuffer()
{
	if (pBuf != NULL)
	{
		delete[] pBuf;
		pBuf = NULL;
	}
}

template <class T_ext, class T_int>
BOOL CBackBuffer<T_ext, T_int>::Allocate(int size)
{
	ASSERT(size != 0 && size <= 0x10000000 / sizeof(T_int));
	// make size a "round" number
	int i;
	for (i = 1; i <= 0x10000000; i <<= 1)
	{
		if (i >= size)
		{
			break;
		}
	}
	size = i;
	if (pBuf != NULL)
	{
		if (BufSize == size)
		{
			nCurrIndex = 0;
			return TRUE;
		}
		delete[] pBuf;
		pBuf = NULL;
	}
	pBuf = new T_int[size];
	if (NULL == pBuf)
	{
		return FALSE;
	}

	for (i = 0; i < size; i++)
	{
		pBuf[i] = T_int(0);
	}

	BufSize = size;
	dwIndexMask = size - 1;
	nCurrIndex = 0;
	return TRUE;
}

template <class T_ext, class T_int>
inline T_ext CBackBuffer<T_ext, T_int>::Get(int index)
{
	ASSERT(pBuf != NULL);
	return pBuf[(index + nCurrIndex) & dwIndexMask];
}

template <class T_ext, class T_int>
inline void CBackBuffer<T_ext, T_int>::Put(int index, T_ext data)
{
	ASSERT(pBuf != NULL);
	pBuf[(index + nCurrIndex) & dwIndexMask] = data;
}

template <class T_ext, class T_int>
inline T_int& CBackBuffer<T_ext, T_int>::operator[](int index)
{
	ASSERT(pBuf != NULL);
	return pBuf[(index + nCurrIndex) & dwIndexMask];
}

class CWaveProc
{
public:
	CWaveProc();
	virtual ~CWaveProc() {}

	virtual void Dump(unsigned indent=0) const;
	// the function returns number of returned bytes
	// if NULL == pInBuf, the function should flush back stored samples
	// and return their number, or 0 if no more samples.
	// Any latency should be compensated in the function
	// The function checks for minimum buffer size
	// and calls ProcessSoundBuffer with at least GetInputSampleSize()
	// and GetOutputSampleSize()

	// the default (and only by now) implementation calls ProcessSoundBuffer and updates number of samples
	virtual unsigned ProcessSound(char const * pInBuf, char * pOutBuf,
								unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);

	// SetInputWaveformat returns FALSE if the wave cannot be
	// processed
	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels);
	virtual BOOL SetOutputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels);

	virtual CWaveFormat const & GetInputWaveformat() const;
	virtual CWaveFormat const & GetOutputWaveformat() const;

	CHANNEL_MASK GetInputChannelsMask() const
	{
		return m_ChannelsToProcess;
	}

	CHANNEL_MASK GetOutputChannelsMask() const
	{
		return m_ResultChannels;
	}

	virtual bool SetChannelsToProcess(CHANNEL_MASK channels)
	{
		m_ChannelsToProcess = channels;
		m_ResultChannels = channels;
		return true;
	}

	// if input data is compressed and not sample-aligned, this should be 0
	// it can be multiple of block size for compressed format
	virtual unsigned GetInputSampleSize() const
	{
		return m_InputFormat.SampleSize();
	}

	// if output data is compressed and not sample-aligned, this should be 0
	// it can be multiple of block size for compressed format
	virtual unsigned GetOutputSampleSize() const
	{
		return m_OutputFormat.SampleSize();
	}

	virtual NUMBER_OF_SAMPLES GetInputNumberOfSamples() const
	{
		return m_ProcessedInputSamples;
	}

	virtual NUMBER_OF_SAMPLES GetOutputNumberOfSamples() const
	{
		return m_SavedOutputSamples;
	}

	__int16 DoubleToShort(double x);

	virtual BOOL WasClipped() const
	{
		return m_bClipped;
	}
	virtual double GetMaxClipped() const
	{
		return m_MaxClipped;
	}

	virtual BOOL Init();
	virtual void DeInit();

protected:

	bool m_bClipped;
	double m_MaxClipped;
	WaveSampleType m_InputSampleType;
	WaveSampleType m_OutputSampleType;

	CWaveFormat m_InputFormat;
	CWaveFormat m_OutputFormat;

	CHANNEL_MASK m_ChannelsToProcess;
	CHANNEL_MASK m_ResultChannels;

	NUMBER_OF_SAMPLES m_CurrentSample;
	NUMBER_OF_SAMPLES m_ProcessedInputSamples;
	NUMBER_OF_SAMPLES m_SavedOutputSamples;

	// this function always gets whole samples on input
	// The default implementation calls ProcessSoundSample for every sample
	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);

	// process samples for all channels. The default implementation calls ProcessSampleValue for all relevant channels
	virtual void ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels);
	// the default implementation copies the sample
	virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);

private:

	MEDIA_FILE_SIZE m_ProcessedInputBytes;
	size_t m_SavedOutputBytes;

	// assignment guard
	CWaveProc(const CWaveProc &);
	CWaveProc & operator =(const CWaveProc &);
};

class CVolumeChangeProc : public CWaveProc
{
	typedef CVolumeChangeProc ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CVolumeChangeProc(double const * VolumeArray, unsigned VolumeArraySize);

	virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);

	double m_Volume[MAX_NUMBER_OF_CHANNELS];
};

class CWaveMixProc : public CWaveProc
{
	typedef CWaveProc BaseClass;
	typedef CWaveMixProc ThisClass;

public:

	typedef std::auto_ptr<ThisClass> auto_ptr;
	CWaveMixProc()
	{
		m_InputSampleType = SampleTypeFloat32;
	}

protected:
	void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);
	virtual double GetSrcMixCoefficient(SAMPLE_INDEX Sample, int Channel) const = 0;
	virtual double GetDstMixCoefficient(SAMPLE_INDEX Sample, int Channel) const = 0;
};

enum
{
	FadeInLinear = 1,
	FadeOutLinear = -FadeInLinear,
	FadeInSinSquared = 2,
	FadeOutSinSquared = -FadeInSinSquared,
	FadeInSine = 3,
	FadeOutCosine = -FadeInSine,
};

class CFadeInOutProc : public CWaveMixProc
{
	typedef CWaveMixProc BaseClass;
	typedef CFadeInOutProc ThisClass;

public:

	typedef std::auto_ptr<ThisClass> auto_ptr;
	CFadeInOutProc(int FadeCurveType, NUMBER_OF_SAMPLES Length);
	// init cross fade

protected:
	virtual double GetSrcMixCoefficient(SAMPLE_INDEX Sample, int Channel) const;
	virtual double GetDstMixCoefficient(SAMPLE_INDEX Sample, int Channel) const;
	int m_FadeCurveType;
	NUMBER_OF_SAMPLES m_CurveLength;
	CWaveFile m_SrcFile;
};

class CHumRemoval : public CWaveProc
{
	typedef CHumRemoval ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CHumRemoval(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess);

	virtual void ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels);

	double m_prev_out[MAX_NUMBER_OF_CHANNELS];
	double m_prev_in[MAX_NUMBER_OF_CHANNELS];

	double m_PrevHpf[2][MAX_NUMBER_OF_CHANNELS];

	double m_PrevHpOut[2][MAX_NUMBER_OF_CHANNELS];

	double m_DiffCutoffCoeffs[2];
	double m_HighpassCoeffs[3];

	BOOL m_ApplyHighpassFilter;
	BOOL m_ApplyCommonModeFilter;

	void EnableDifferentialSuppression(BOOL enable)
	{
		m_ApplyCommonModeFilter = enable;
	}
	void EnableLowFrequencySuppression(BOOL enable)
	{
		m_ApplyHighpassFilter = enable;
	}

	void SetDifferentialCutoff(double frequency);
	void SetHighpassCutoff(double frequency);
};

struct StoredClickData
{
	SAMPLE_INDEX Position;  // in samples in the file
	short Length[2];   // length in left and right channel (0 if none)
};

enum
{
	MaxInterpolatedLength = 128,
};

struct DeclickParameters
{
	int m_nMaxClickLength;
	BOOL m_bLogClicksOnly;
	int m_nMinClickLength;
	BOOL m_bLogClicks;
	CString m_ClickLogFilename;
	BOOL m_bImportClicks;
	CString m_ClickImportFilename;
	BOOL    m_bDontAutodetectClicks;

	double m_MeanPowerDecayRate;
	double m_MeanPowerAttackRate;
	double m_PowerToDeriv3RatioThreshold;
	double m_MinDeriv3Threshold;
	double m_MinClickDeriv3BoundThreshold;
	double m_ClickDeriv3ThresholdScale;  // is used to find click boundary
	double m_NoiseFloorThresholdScale;

	DeclickParameters();    // default initializer
};

class CClickRemoval: public CWaveProc, protected DeclickParameters
{
	typedef CClickRemoval ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CClickRemoval(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess,
				DeclickParameters const & dp = DeclickParameters());

	virtual ~CClickRemoval();

	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);

	void InterpolateGap(CBackBuffer<int, int> & data, int nLeftIndex, int InterpolateSamples, bool BigGap);
	void InterpolateGap(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, bool BigGap, int TotalSamples);
	void InterpolateGapLeastSquares(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, int TotalSamples);
	void InterpolateBigGapSliding(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, int TotalSamples);

	BOOL LoadClickSourceFile(LPCTSTR szFilename);
	BOOL SetClickLogFile(LPCTSTR szFilename);

protected:
	virtual BOOL Init();

	enum
	{
		PREV_BUF_SIZE = 2048,
		PREV_MASK = PREV_BUF_SIZE-1,
		CLICK_LENGTH = 64,
		ANALYZE_LAG = 1024,
	};

	struct DeclickChannelData
	{
		DeclickChannelData();
		CBackBuffer<int, int> m_prev;
		CBackBuffer<int, int> m_prev3;
		SAMPLE_INDEX m_NextPossibleClickPosition;
		int m_PrevDeriv;
		int m_PrevDeriv2;
		double m_MeanPower;

		int Update3RdDerivativePowerThreshold(CClickRemoval const * pCr);

		BOOL CheckForClick();
		void StoreData(WAVE_SAMPLE * pOut, unsigned nSamples, int Stride);
	};


private:

	SAMPLE_INDEX m_PrevIndex;
	NUMBER_OF_SAMPLES m_nStoredSamples;
	DeclickChannelData m_ChannelData[2];
	// array for the clicks defined in a file
	typedef std::vector<StoredClickData> ClicksVector;
	typedef ClicksVector::iterator ClicksVectorIterator;
	typedef ClicksVector::const_iterator ClicksVectorConstIterator;

	ClicksVector m_PredefinedClicks;

	FILE * m_pOutClicksFile;
};

struct NoiseReductionParameters
{
	NoiseReductionParameters();     // default initialization
	void Dump(unsigned indent=0) const;

	float m_MinFrequencyToProcess;

	// decay rates for average amplitude and frequency
	float m_AvgFreqDecayRate;
	float m_AvgLevelDecayRate;
	float m_FreqErrorDecayRate;
	float m_LevelErrorDecayRate;
	// decay rates for amplitude and frequency deviations
	float m_FreqDevDecayRate;
	float m_LevelDevDecayRate;

	float m_ThresholdOfTransientAttack;     // powerNext/PowerPrev
	float m_ThresholdOfTransientDecay;      // powerNext/PowerPrev
	float m_FreqThresholdOfNoiselike; // compare with SIGNAL_PARAMS::sp_FreqDev

	float m_LevelThresholdForNoiseLow;     // for low frequencies
	float m_LevelThresholdForNoiseHigh;    // for high frequencies

	// we multiply the band power to tone preservation factor
	// for tonal bands
	float m_ToneOverNoisePreference;

	double m_NoiseReductionRatio;    // aggressiveness of noise suppression
	float m_MaxNoiseSuppression;    // how much FFT band can be suppressed (in dB)

	// DecayDistance is frequency distance where masking decays by 1/e
	// it is different for high and low frequencies
	float m_NearMaskingDecayDistanceHigh;  // for high frequencies
	float m_NearMaskingDecayDistanceLow;   // for low frequencies

	float m_NearMaskingDecayRate;   // coeff to filter near masking function

	// DecayTime is time interval where masking decays by 1/e
	// it is different for high and low frequencies
	// Specified in miliseconds
	float m_NearMaskingDecayTimeLow;   // for low frequencies
	float m_NearMaskingDecayTimeHigh;   // for high frequencies

	float m_FarMaskingLevelDb;  // weights near masking against far masking
	//float m_FarMaskingScale;    // overall scale to calculate far masking
};

class NoiseReductionCore : protected NoiseReductionParameters
{
public:
	typedef float DATA;
	NoiseReductionCore(int nFftOrder, NUMBER_OF_CHANNELS nChannels, long SampleRate,
						NoiseReductionParameters const & nr = NoiseReductionParameters(),
						CHANNEL_MASK ChannelsToProcess = ALL_CHANNELS);
	~NoiseReductionCore();
	void Dump(unsigned indent=0) const;

	int FlushSamples(DATA * pBuf, int nOutSamples);
	int FillInBuffer(DATA const * pBuf, int nInSamples);
	int DrainOutBuffer(DATA * pBuf, int nOutSamples);
	void ResetOutBuffer();

	// get noise masking
	void GetAudioMasking(DATA * pBuf);  // nChannels * FftOrder
	void GetNoiseThreshold(DATA * pBuf); // precomputed treshold, nChannels *FftOrder count
	bool IsTonalBand(int ch, int f) const;
	// filtered FFT power
	//void GetPerceptedPower(DATA * pBuf);  // nChannels * FftOrder
	// FFT power of the source signal
	void GetPowerInBands(DATA * pBuf);  // nChannels * FftOrder
	// FFT power of the output
	void GetResultPowerInBands(DATA * pBuf);  // nChannels * FftOrder

	void Reset();

	void ProcessInputFft();

	void AnalyseFft();

	void ProcessInverseFft();

	bool CanProcessFft() const;

protected:
	int m_nChannels;
	long m_SampleRate;
	CHANNEL_MASK m_ChannelsToProcess;

	unsigned m_nFftOrder;
	float m_PowerScale;             // to make the values independent of FFT order

	std::vector<float> m_Window;
	std::vector<float> m_pNoiseFloor;
	// pointer to array of float pairs
	// for storing input data
	enum {FAR_MASKING_GRANULARITY = 64};
	float m_FarMaskingCoeffs[FAR_MASKING_GRANULARITY][FAR_MASKING_GRANULARITY];

	friend struct SIGNAL_PARAMS;
	friend struct NoiseReductionChannelData;

	struct NoiseReductionChannelData * m_ChannelData[MAX_NUMBER_OF_CHANNELS];
	void CalculateFarMasking(float SubbandPower[FAR_MASKING_GRANULARITY],
							float FarMasking[FAR_MASKING_GRANULARITY]);

#ifdef _DEBUG
public:
	int m_TotalBandProcessed;
	int m_TransientBandFound;
	int m_PhaseFilteredInBands;
	int m_StationaryBandCancelled;
	int m_NoiselikeBandCancelled;
	float m_MaxLevelInBand;
	float m_MinLevelInBand;
#endif
};

class CNoiseReduction : public CWaveProc
{
	typedef CNoiseReduction ThisClass;
	typedef CWaveProc BaseClass;
	typedef NoiseReductionCore::DATA DATA;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CNoiseReduction(WAVEFORMATEX const * pWf, CHANNEL_MASK ChannelsToProcess, unsigned nFftOrder, NoiseReductionParameters const & nr = NoiseReductionParameters());
	~CNoiseReduction();

	virtual void Dump(unsigned indent=0) const;

	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);

	NoiseReductionCore * m_pNrCore;
	NoiseReductionParameters m_NrParms;
	unsigned m_FftOrder;
};

class CBatchProcessing: public CWaveProc
{
	typedef CBatchProcessing ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CBatchProcessing()
		:m_bAutoDeleteProcs(false), m_BackwardPass(false)
	{
		m_Stages.reserve(32);
	}
	virtual ~CBatchProcessing();
	virtual void Dump(unsigned indent=0) const;

	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);

	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);
	virtual BOOL SetOutputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels);

	// if input data is compressed and not sample-aligned, this could be 0
	// it can be multiple of block size for compressed format
	virtual unsigned GetInputSampleSize() const;

	// if output data is compressed and not sample-aligned, this could be 0
	// it can be multiple of block size for compressed format
	virtual unsigned GetOutputSampleSize() const;

	virtual NUMBER_OF_SAMPLES GetInputNumberOfSamples() const;

	virtual NUMBER_OF_SAMPLES GetOutputNumberOfSamples() const;

	void AddWaveProc(CWaveProc * pProc, int index = -1);
	virtual BOOL WasClipped() const;
	virtual double GetMaxClipped() const;

	virtual BOOL Init();
	virtual void DeInit();
	void SetBackwardPass(bool backward)
	{
		m_BackwardPass = backward;
	}

	bool m_bAutoDeleteProcs;
	bool m_BackwardPass;
protected:
	enum {IntermediateBufSize = 0x1000};
	struct Item
	{
		CWaveProc * Proc;
		char * InBuf;             // temporary intermediate buffer for this stage input data
		char * OutBuf;             // temporary intermediate buffer for this stage output data. If no format conversion necessary,
		// the last stage may place directly to the output buffer
		unsigned InBufGetIndex;
		unsigned InBufPutIndex;
		unsigned OutBufGetIndex;
		unsigned OutBufPutIndex;
		int Flags;
		// implement destructive copy operator and assignment constructor
		~Item();
		Item();
		BOOL Init();

		Item(CWaveProc * proc);
		Item(Item & item);
		Item & operator =(Item& item);

		unsigned FillInputBuffer(const char * Buf, unsigned BufFilled, CWaveFormat const * pWf, CHANNEL_MASK SrcChannelMask); // returns number bytes used
		unsigned FillOutputBuffer(char * Buf, unsigned BufFree, CWaveFormat const * pWf, CHANNEL_MASK DstChannelMask); // returns number bytes used

	private:
		//Item(Item const & item);
		//Item & operator =(Item const& item);
	};
	std::vector<Item> m_Stages;
	typedef std::vector<Item>::iterator item_iterator;
	typedef std::vector<Item>::const_iterator item_iterator_const;
};

class CResampleFilter: public CWaveProc
{
	typedef CResampleFilter ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CResampleFilter();
	CResampleFilter(long NewSampleRate, double AntiAliasCutoffFrequency, bool KeepSamplesPerSec);

	virtual ~CResampleFilter();

	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);

	void InitResample(long NewSampleRate, double AntiAliasCutoffFrequency, BOOL KeepSamplesPerSec);
	enum { DefaultFilterLength = 16, };
private:
	void InitSlidingInterpolatedFilter(unsigned NumSincWaves, unsigned SamplesInFilter, unsigned TableSize);
	void InitSlidingFilter(unsigned NumSincWaves, unsigned SamplesInFilter, unsigned NumberOfFilterTables);

	// returns samples filled in units of (float*Channels)
	// OutSamples and InSamples are in units of (float*Channels)
	int FilterSoundResample(float * pOut, int OutBufferSamples, float const * pIn, int InSamples, int* InSamplesUsed, int Channels);

	int DoSlidingInterpolatedFilterResample(float * pOut, int OutBufferSamples, float const * pIn, int InSamples, int* InSamplesUsed, int Channels);
	int DoSlidingFilterResample(float * pOut, int OutBufferSamples, float const * pIn, int InSamples, int* InSamplesUsed, int Channels);

	void ResetResample();

	enum
	{
		WindowTypeSquareSine,
		WindowTypeNuttall,
		WindowTypeBlackman,
		WindowType = WindowTypeNuttall,
	};

	struct FilterCoeff
	{
		double value;
		double deriv1;
		double deriv2;
		double deriv3;		// will not use it
	};

	struct FilterPolyCoeff
	{
		double tap;
		double A;	// polynomial coefficients
		double B;
		double C;
		double D;
		double F;
	};

	void FilterWindow(double arg, FilterCoeff & Result);
	static void sinc(double arg, FilterCoeff & Result);
	void ResampleFilterTap(double arg, double FilterLength, FilterCoeff & Result);

	enum
	{
		ResampleTableBits = 11,
		ResampleFilterSize = (1 << ResampleTableBits),
		ResampleIndexShift = (32 - ResampleTableBits),
		MaxNumberOfFilterSamples = 50*100,
		SrcBufSize = 0x1000 };

	float m_pSrcBuf[SrcBufSize];

	unsigned m_SrcBufUsed;   // position to get samples

	unsigned m_SrcBufFilled; // position to put new samples converted from __int16
	long    m_EffectiveOutputSampleRate;
	BOOL m_KeepOriginalSampleRate;

	// m_InterpolatedFilterTable covers the full range from -0.5 to +0.5 of contiguous sinc(x)*window(x) filter function
	ATL::CHeapPtr<FilterPolyCoeff> m_InterpolatedFilterTable;
	ATL::CHeapPtr<double> m_FilterTable;

	double m_AntiAliasCutoffFrequency;
	unsigned m_SrcFilterLength;		// number of input samples necessary to cover the whole interpolating filter
	bool m_bUseInterpolatedFilter;		// TRUE if using InterpolatedFilterTable instead of precalculated exact m_FilterTable
	// Upsampling:
	// m_InputPeriod = 0x100000000LL/((FilterLength+1)*2)
	// m_OutputPeriod = F_old/F_new*0x100000000LL/((FilterLength+1)*2)
	// Upsampling:
	// m_InputPeriod = F_new/F_old*0x100000000LL/((FilterLength+1)*2)
	// m_OutputPeriod = 0x100000000LL/((FilterLength+1)*2)
	// how much m_Phase is changed (incremented) for each input sample
	unsigned __int32 m_InputPeriod;		// 0x100000000LL/SamplesInFilter
	// how much m_Phase is changed (decremented) for each output sample
	unsigned __int32 m_OutputPeriod;	// 0x100000000LL/SamplesInFilter*F_old/F_new

	// m_Phase is current fractional offset of the current input sample in the filter.
	// Full 0x100000000LL range corresponds to full length of the filter
	unsigned __int32 m_Phase;

	// For upsampling, SamplesInFilter is (FilterLength+1)*2). By default FilterLength is 125
	// For downsampling, SamplesInFilter is F_old/F_new(FilterLength+1)*2). By default FilterLength is 125
	unsigned m_SamplesInFilter;		// minimum number of input samples needed to run the filter
	unsigned m_FilterArraySize;		// size of m_FilterTable array = Number Of Filter Tables * m_SamplesInFilter
	signed m_RationalResampleFraction;
	unsigned m_FilterIndex;			// current index of filter in m_FilterTable

};

class CAudioConvertor : public CWaveProc
{
	typedef CAudioConvertor ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CAudioConvertor(HACMDRIVER had = NULL);
	virtual ~CAudioConvertor();
	BOOL InitConversion(WAVEFORMATEX const * SrcFormat, WAVEFORMATEX const * DstFormat);
	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);

	// if input data is compressed and not sample-aligned, this should be 0
	// it can be multiple of block size for compressed format
	virtual unsigned GetInputSampleSize() const;

	// if input data is compressed and not sample-aligned, this should be 0
	// it can be multiple of block size for compressed format
	virtual unsigned GetOutputSampleSize() const;

	virtual NUMBER_OF_SAMPLES GetInputNumberOfSamples() const;

	virtual NUMBER_OF_SAMPLES GetOutputNumberOfSamples() const;

private:
	AudioStreamConvertor m_AcmConvertor;

	DWORD m_ConvertFlags;
	unsigned m_InputSampleSize;
	unsigned m_OutputSampleSize;

protected:

	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
};

class CChannelConvertor : public CWaveProc
{
	typedef CChannelConvertor ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CChannelConvertor(NUMBER_OF_CHANNELS NewChannels);

	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);
	// conversion either mono->stereo, or stereo->mono.
	// if converting stereo->mono, the data can be left, right, or average
	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
	NUMBER_OF_CHANNELS m_NewChannels;
};

class CByteSwapConvertor : public CWaveProc
{
	typedef CByteSwapConvertor ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CByteSwapConvertor()
	{
	}
	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
};

// only passes one out of m_DecimationRatio samples to the output
class CDecimator : public CWaveProc
{
	typedef CDecimator ThisClass;
	typedef CWaveProc BaseClass;
	int m_DecimationRatio;
	int m_CurrentCounter;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CDecimator(int DecimationRatio)
		: m_DecimationRatio(DecimationRatio)
		, m_CurrentCounter(0)
	{
		m_InputSampleType = SampleTypeFloat32;
	}
	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels);
	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
};

// insertz zero samples to stretch the data by DecimationRatio, and multiplies the samples by that ratio to keep same level
class CDeDecimator : public CWaveProc
{
	typedef CDeDecimator ThisClass;
	typedef CWaveProc BaseClass;
	int m_ExpansionRatio;
	int m_CurrentCounter;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CDeDecimator(int ExpansionRatio)
		: m_ExpansionRatio(ExpansionRatio)
		, m_CurrentCounter(0)
	{
		m_InputSampleType = SampleTypeFloat32;
	}
	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels);
	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);
};

struct FilterCoefficients
{
	BOOL m_bZeroPhase;
	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs
	// results of the filter sections are ADDED
	// if order==0, no filter
	int     m_nLpfOrder;    // low pass filter order
	double m_LpfCoeffs[MaxFilterOrder][6];

	// results of the filter sections are ADDED
	int     m_nHpfOrder;    // high pass filter order
	double m_HpfCoeffs[MaxFilterOrder][6];

	// results of the filter sections are MULTIPLIED
	int     m_nNotchOrder;
	double m_NotchCoeffs[MaxFilterOrder][6];
};

class CFilterProc : public CWaveProc, public FilterCoefficients
{
	typedef CFilterProc ThisClass;
	typedef CWaveProc BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CFilterProc()
	{
		m_InputSampleType = SampleTypeFloat32;
	}

	virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);

	virtual BOOL Init();
	void SetFilterCoefficients(FilterCoefficients const & coeffs)
	{
		static_cast<FilterCoefficients&>(*this) = coeffs;
	}

private:
	double CalculateResult(unsigned ch, double Input);
	double m_PrevLpfSamples[MAX_NUMBER_OF_CHANNELS][MaxFilterOrder][4];
	double m_PrevHpfSamples[MAX_NUMBER_OF_CHANNELS][MaxFilterOrder][4];
	double m_PrevNotchSamples[MAX_NUMBER_OF_CHANNELS][MaxFilterOrder][4];
};

class CGilbertPrefilter : public CWaveProc
{
	typedef CGilbertPrefilter ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CGilbertPrefilter()
	{
		m_InputSampleType = SampleTypeFloat32;
	}

	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);

	void ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels);

private:
};

class CGilbertPostfilter : public CWaveProc
{
	typedef CGilbertPostfilter ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CGilbertPostfilter()
	{
		m_InputSampleType = SampleTypeFloat32;
	}

	virtual BOOL SetInputWaveformat(CWaveFormat const & Wf, CHANNEL_MASK channels = ALL_CHANNELS);

	void ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels);

private:
};

class CLameEncConvertor : public CWaveProc
{
	typedef CLameEncConvertor ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CLameEncConvertor(LPCTSTR pDll = _T("LAME_ENC.DLL"))
		: m_InputBufferSize(0),
		m_InputBufferFilled(0),
		m_pOutputBuffer(NULL),
		m_OutputBufferFilled(0),
		m_OutputBufferSize(0),
		m_pInputBuffer(NULL)
	{
		m_Enc.Open(pDll);
	}
	~CLameEncConvertor();
	BOOL SetFormat(WAVEFORMATEX const * pWF);
protected:
	BladeMp3Encoder m_Enc;
	char * m_pInputBuffer;
	unsigned m_InputBufferSize;
	unsigned m_InputBufferFilled;

	BYTE * m_pOutputBuffer;
	unsigned m_OutputBufferFilled;
	unsigned m_OutputBufferSize;
	virtual unsigned ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes);

	virtual BOOL Init();
	virtual void DeInit();
};

#endif //#ifndef __WAVEPROC_H_
