// waveproc.h
#ifndef __WAVEPROC_H_
#define __WAVEPROC_H_

#include <complex>
#include <afxtempl.h>
#include <mmreg.h>
#include <msacm.h>
using namespace std;

#if 0
template <class T_ext, class T_int, int SIZE> class CBackBuffer
{
public:
	CBackBuffer() : nCurrIndex(0) {}
	~CBackBuffer() {}
	int GetCounter() { return nCurrIndex; }
	void SetCounter(int index) { nCurrIndex = index; }
	void Advance(int n = 1)
	{
		nCurrIndex += n;
	}
	T_ext Get(int index)
	{
		return pBuf[(index + nCurrIndex) & (SIZE - 1)];
	}

	void Put(int index, T_ext data)
	{
		pBuf[(index + nCurrIndex) & (SIZE - 1)] = data;
	}

	T_int& operator[](int index)
	{
		return pBuf[(index + nCurrIndex) & (SIZE - 1)];
	}

private:
	T_int Buf[SIZE];
	int nCurrIndex;
};
#endif
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
	operator BOOL() {return pBuf != 0; }
	T_int& operator[](int index);
private:
	T_int * pBuf;
	int BufSize;
	int nCurrIndex;
	DWORD dwIndexMask;
};

class CWaveProc
{
public:
	CWaveProc() : m_Callback(NullCallback),
		m_TmpInBufPut(0),
		m_TmpInBufGet(0),
		m_TmpOutBufPut(0),
		m_TmpOutBufGet(0)
	{}
	virtual ~CWaveProc() {}
	// the function returns number of returned samples
	// if NULL == pInBuf, the function should flush back stored samples
	// and return their number, or 0 if no more samples
	// any latency should be compensated in the function
	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans) = 0;
	// SetAndValidateWaveformat returns FALSE if the wave cannot be
	// processed
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	BOOL (*m_Callback)(CWaveProc * pProc, UINT msg,
						size_t TotalSize, size_t pos);
	BOOL CheckForMinBufferSize(char const * &pInBuf, char * &pOutBuf,
								int &nInBytes, int &nOutBytes,
								int * pUsedBytes, int * pSavedBytes,
								int nMinInBytes, int nMinOutBytes, int nChan);

	static BOOL NullCallback(CWaveProc * , UINT ,
							size_t , size_t ) { return TRUE; }
	DWORD m_dwCallbackData;
	char m_TmpInBuf[4];
	char m_TmpOutBuf[4];
	int m_TmpInBufPut;
	int m_TmpInBufGet;
	int m_TmpOutBufPut;
	int m_TmpOutBufGet;
};

#define WAVEPROC_MSG_PROGRESS 0
#define WAVEPROC_MSG_FILE_NOT_FOUND 1
#define WAVEPROC_MSG_WRONG_FILE 2
#define WAVEPROC_MSG_WRONG_FILE_FORMAT 3
#define WAVEPROC_MSG_FINISHED 4

class CHumRemoval: public CWaveProc
{
public:
	CHumRemoval();
	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	float m_prev_outl, m_prev_outr;
	float m_prev_inl, m_prev_inr;
	int m_PrevHpfL[2];
	int m_PrevHpfR[2];
	double m_PrevHpOutL[2];
	double m_PrevHpOutR[2];
	BOOL m_ApplyHighpassFilter;
};

struct StoredClickData
{
	long Position;  // in samples in the file
	short Length[2];   // length in left and right channel (0 if none)
};

class CClickRemoval: public CWaveProc
{
public:
	CClickRemoval();
	virtual ~CClickRemoval();
	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);

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

void InterpolateGap(CBackBuffer<int, int> & data, int nLeft, int n_Right);

class CNoiseReduction: public CWaveProc
{
public:
	CNoiseReduction(int nFftOrder = 1024);
	virtual ~CNoiseReduction();
	typedef float DATA;
	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	//protected:
	long m_SamplesPerSec;
	int m_nFftOrder;
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

	float m_NoiseReductionRatio;    // aggressivness of noise suppression
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

	float m_NearMaskingCoeff;  // weights near masking against far masking
	//float m_FarMaskingScale;    // overall scale to calculate far masking

	float m_PowerScale;             // to make the values independent of FFT order
	int m_nBackSampleCount;
	int m_nStoredSamples;
	float * m_Window;
	// pointer to array of float pairs
	// for storing input data
	float (* m_BackBuffer)[2];
	// pointer to array of float pairs
	// for accumulating output result
	float (* m_AccumBuffer)[2];
	DATA * m_FftInBuffer[2];
	enum {FAR_MASKING_GRANULARITY = 64};
	float m_FarMaskingCoeffs[FAR_MASKING_GRANULARITY][FAR_MASKING_GRANULARITY];

	struct SIGNAL_PARAMS
	{
		// current and previous samples are stored. Result of current analysis
		// is used for the previous sample
		complex<DATA> sp_FftIn[2];
		//complex<DATA> sp_PrevFftOut;
		void AnalyzeFftSample(complex<DATA> smp, CNoiseReduction * pNr, int nSample);
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
		char m_TonalBand;
	};
	SIGNAL_PARAMS * m_pParams[2];
	friend struct SIGNAL_PARAMS;
	complex<DATA> * m_FftOutBuffer[2];
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


class CBatchProcessing: public CWaveProc
{
public:
	CBatchProcessing()
		:m_bAutoDeleteProcs(FALSE)
	{}
	virtual ~CBatchProcessing();

	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans);
	virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	void AddWaveProc(CWaveProc * pProc, int index = -1);
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
public:
	CResampleFilter()
	{
	}
	virtual ~CResampleFilter();
	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans);
	//virtual BOOL SetAndValidateWaveformat(WAVEFORMATEX const * pWf);
	BOOL InitResample(double ResampleRatio, double FilterLength, int nChannels);
	void FilterSoundResample();
	enum {ResampleTableBits = 10,
		ResampleFilterSize = (1 << ResampleTableBits),
		ResampleIndexShift = (32 - ResampleTableBits),
		SrcBufSize = 0x4000,
		DstBufSize = 0x4000 };
	float m_pSrcBuf[SrcBufSize];
	float m_pDstBuf[DstBufSize];

	int m_SrcBufUsed;   // position to get samples
	int m_DstBufUsed;   // position to put out samples
	int m_DstBufSaved;  // position to get the samples and convert to __int16

	int m_SrcBufFilled; // position to put new samples converted from __int16
	int m_SrcFilterLength;

	float m_FilterBuf[ResampleFilterSize];
	float m_FilterDifBuf[ResampleFilterSize];
	float m_FilterDif2Buf[ResampleFilterSize];

	unsigned __int32 m_InputPeriod;
	unsigned __int32 m_OutputPeriod;
	unsigned __int32 m_Phase;
	double m_ResampleRatio;

	long m_TotalProcessedSamples;
	long m_TotalSavedSamples;

	int m_Channels;
};

class CAudioConvertor : public CWaveProc
{
public:
	CAudioConvertor();
	virtual ~CAudioConvertor();

	HACMSTREAM m_acmStr;
	size_t m_SrcBufSize;
	size_t m_DstBufSize;
	ACMSTREAMHEADER m_ash;
	DWORD m_DstSaved;
	DWORD m_ConvertFlags;
	WAVEFORMATEX * m_SrcFormat;
	WAVEFORMATEX * m_DstFormat;

	BOOL InitConversion(WAVEFORMATEX * SrcFormat, WAVEFORMATEX * DstFormat,
						HACMDRIVER had = NULL);
	virtual int ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedSamples, int nChans);
};

class CChannelConvertor : public CWaveProc
{
public:
	CChannelConvertor()
		: nSrcChannel(2),
		nSourceChannels(1),
		nTargetChannels(1),
		dwSrcProcessed(0),
		dwDstProcessed(0)
	{
	}
	virtual ~CChannelConvertor() {}
	int nSrcChannel;
	int nSourceChannels;
	int nTargetChannels;
	DWORD dwSrcProcessed;
	DWORD dwDstProcessed;
	// conversion either mono->stereo, or stereo->mono.
	// if converting stereo->mono, the data can be left, right, or average
	virtual int ProcessSound(char const * pIn, char * pOut,
							int nInBytes, int nOutBytes, int * pUsedBytes, int nChans);
};
#endif //#ifndef __WAVEPROC_H_
