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
	for (int i = 1; i <= 0x10000000; i <<= 1)
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
	// the function returns number of returned samples
	// if NULL == pInBuf, the function should flush back stored samples
	// and return their number, or 0 if no more samples
	// any latency should be compensated in the function
	// The function checks for minimum buffer size
	// and calls ProcessSoundBuffer with at least MinInBytes()
	// and MinOutBytes()
	virtual size_t ProcessSound(char const * pInBuf, char * pOutBuf,
								size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	// SetAndValidateWaveformat returns FALSE if the wave cannot be
	// processed
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	NUMBER_OF_CHANNELS m_InputChannels;
	NUMBER_OF_CHANNELS m_OutputChannels;
	CHANNEL_MASK m_ChannelsToProcess;
	long m_SamplesPerSecond;

	BOOL CheckForMinBufferSize(char const * &pInBuf, char * &pOutBuf,
								size_t & nInBytes, size_t & nOutBytes,
								size_t * pUsedBytes, size_t * pSavedBytes,
								size_t nMinInBytes, size_t nMinOutBytes);

	virtual size_t GetMinInputBufSize() const
	{
		return m_InputChannels * sizeof (WAVE_SAMPLE);
	}
	virtual size_t GetMinOutputBufSize() const
	{
		return m_OutputChannels * sizeof (WAVE_SAMPLE);
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

	char m_TmpInBuf[32];
	char m_TmpOutBuf[32];
	size_t m_TmpInBufPut;
	size_t m_TmpInBufGet;
	size_t m_TmpOutBufPut;
	size_t m_TmpOutBufGet;
	BOOL m_bClipped;
	double m_MaxClipped;
};

class CHumRemoval : public CWaveProc
{
	typedef CHumRemoval ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CHumRemoval();

	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);

	float m_prev_outl, m_prev_outr;
	float m_prev_inl, m_prev_inr;

	int m_PrevHpfL[2];
	int m_PrevHpfR[2];

	float m_PrevHpOutL[2];
	float m_PrevHpOutR[2];

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

class CClickRemoval: public CWaveProc
{
	typedef CClickRemoval ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CClickRemoval();
	virtual ~CClickRemoval();

	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);

	void InterpolateGap(CBackBuffer<int, int> & data, int nLeftIndex, int InterpolateSamples, bool BigGap);
	void InterpolateGap(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans, bool BigGap);
	void InterpolateBigGap(WAVE_SAMPLE data[], int nLeftIndex, int ClickLength, int nChans);

	BOOL SetClickSourceFile(LPCTSTR szFilename);
	BOOL SetClickLogFile(LPCTSTR szFilename);

	enum {PREV_BUF_SIZE = 2048,
		PREV_MASK = PREV_BUF_SIZE-1,
		CLICK_LENGTH = 64,
		ANALYZE_LAG = 1024};
	CBackBuffer<int, int> m_prev[2];
	CBackBuffer<int, int> m_prev3[2];
//    int m_prev[2][PREV_BUF_SIZE];
//    int m_prev3[2][PREV_BUF_SIZE];
	float m_PowerToDeriv3RatioThreshold;
	float m_MinDeriv3Threshold;
	float m_ClickDeriv3ThresholdScale;  // is used to find click boundary
	float m_MinClickDeriv3BoundThreshold;
	float m_NoiseFloorThresholdScale;

	int m_nMaxClickLength;
	int m_nMinClickLength;

	int m_NextPossibleClickPosition[2];
	int m_PrevDeriv[2];
	int m_PrevDeriv2[2];
	int m_PrevIndex;

	int m_nStoredSamples;
	float m_MeanPower[2];
	float m_MeanPowerDecayRate;
	float m_MeanPowerAttackRate;
	// array for the clicks defined in a file
	BOOL m_PassTrough;

protected:
	CArray<StoredClickData, StoredClickData&> PredefinedClicks;
	int PredefinedClickCurrentIndex;
	CString InClickFilename;
	CString OutClickFilename;
	FILE * pInClicksFile;
	FILE * pOutClicksFile;
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

	float m_ThresholdOfTransient;
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
	NoiseReductionCore(int nFftOrder, int nChannels, long SampleRate,
						NoiseReductionParameters const & nr = NoiseReductionParameters());
	~NoiseReductionCore();
	void Dump(unsigned indent=0) const;

	int FlushSamples(DATA * pBuf, int nOutSamples);
	int FillInBuffer(WAVE_SAMPLE const * pBuf, int nInSamples);
	int DrainOutBuffer(DATA * pBuf, int nOutSamples);

	// get noise masking
	void GetAudioMasking(DATA * pBuf);  // nChannels * FftOrder
	void GetNoiseThreshold(DATA * pBuf); // precomputed treshold, nChannels *FftOrder count
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
	unsigned m_nFftOrder;
	float m_PowerScale;             // to make the values independent of FFT order

	float * m_Window;
	float * m_pNoiseFloor;
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

	CNoiseReduction(unsigned nFftOrder, NoiseReductionParameters const & nr = NoiseReductionParameters());
	~CNoiseReduction();

	virtual void Dump(unsigned indent=0) const;

	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);

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
		:m_bAutoDeleteProcs(FALSE)
	{}
	virtual ~CBatchProcessing();
	virtual void Dump(unsigned indent=0) const;

	virtual size_t ProcessSound(char const * pInBuf, char * pOutBuf,
								size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);

	void AddWaveProc(CWaveProc * pProc, int index = -1);
	virtual BOOL WasClipped() const;
	virtual double GetMaxClipped() const;

	virtual BOOL Init();
	virtual void DeInit();

	BOOL m_bAutoDeleteProcs;
protected:
	struct Item
	{
		CWaveProc * Proc;
		char * Buf;
		int BufGetIndex;
		int BufPutIndex;
	};
	enum {IntermediateBufSize = 0x1000};
	CArray<Item, Item&> m_Stages;
};

int ProcessWaveFile(LPCTSTR NameIn, LPCTSTR NameOut, CWaveProc * pProc);

class CResampleFilter: public CWaveProc
{
	typedef CResampleFilter ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CResampleFilter()
	{
	}
	virtual ~CResampleFilter();

	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
	//virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	BOOL InitResample(double ResampleRatio, double FilterLength,
					NUMBER_OF_CHANNELS nChannels);

	void FilterSoundResample();

	enum {ResampleTableBits = 10,
		ResampleFilterSize = (1 << ResampleTableBits),
		ResampleIndexShift = (32 - ResampleTableBits),
		SrcBufSize = 0x4000,
		DstBufSize = 0x4000 };

	float m_pSrcBuf[SrcBufSize];
	float m_pDstBuf[DstBufSize];

	size_t m_SrcBufUsed;   // position to get samples
	size_t m_DstBufUsed;   // position to put out samples
	size_t m_DstBufSaved;  // position to get the samples and convert to __int16

	size_t m_SrcBufFilled; // position to put new samples converted from __int16
	size_t m_SrcFilterLength;

	float m_FilterBuf[ResampleFilterSize];
	float m_FilterDifBuf[ResampleFilterSize];
	float m_FilterDif2Buf[ResampleFilterSize];

	unsigned __int32 m_InputPeriod;
	unsigned __int32 m_OutputPeriod;
	unsigned __int32 m_Phase;
	double m_ResampleRatio;

	NUMBER_OF_SAMPLES m_TotalProcessedSamples;
	NUMBER_OF_SAMPLES m_TotalSavedSamples;

};

class CAudioConvertor : public CWaveProc
{
	typedef CAudioConvertor ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CAudioConvertor(HACMDRIVER had = NULL);
	virtual ~CAudioConvertor();

	AudioStreamConvertor m_AcmConvertor;

	DWORD m_DstSaved;
	DWORD m_ConvertFlags;
	size_t m_LeftInDstBuffer;
	UCHAR const * m_DstBufPtr;

	BOOL InitConversion(WAVEFORMATEX const * SrcFormat, WAVEFORMATEX const * DstFormat);
	virtual size_t ProcessSound(char const * pInBuf, char * pOutBuf,
								size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
};

class CChannelConvertor : public CWaveProc
{
	typedef CChannelConvertor ThisClass;
	typedef CWaveProc BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CChannelConvertor(NUMBER_OF_CHANNELS OldChannels,
					NUMBER_OF_CHANNELS NewChannels, CHANNEL_MASK ChannelsToProcess)
	{
		m_InputChannels = OldChannels;
		m_OutputChannels = NewChannels;
		m_ChannelsToProcess = ChannelsToProcess;
	}

	//virtual ~CChannelConvertor() {}
	// conversion either mono->stereo, or stereo->mono.
	// if converting stereo->mono, the data can be left, right, or average
	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
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
	virtual ~CByteSwapConvertor() {}
	virtual size_t ProcessSoundBuffer(char const * pInBuf, char * pOutBuf,
									size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);
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
	size_t m_InputBufferSize;
	size_t m_InputBufferFilled;

	BYTE * m_pOutputBuffer;
	size_t m_OutputBufferFilled;
	size_t m_OutputBufferSize;
	virtual size_t ProcessSound(char const * pInBuf, char * pOutBuf,
								size_t nInBytes, size_t nOutBytes, size_t * pUsedBytes);

	virtual BOOL Init();
	virtual void DeInit();

	CWaveFormat m_Wf;
};

#endif //#ifndef __WAVEPROC_H_
