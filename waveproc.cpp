// waveproc.cpp

#include <math.h>
#pragma intrinsic(sin, cos, exp, log, atan2)
#include "stdafx.h"
#include <complex>

#ifndef _CONSOLE
#define puts(t) AfxMessageBox(_T(t), MB_OK | MB_ICONEXCLAMATION)

#endif

#include "FFT.h"
#if 0
void __cdecl DoFFT(const float * src, float * dst, int count)
{
	complex<float> * tmp = new complex<float>[count /2 + 1];
	if (NULL == tmp)
	{
		ASSERT(FALSE);
		return;
	}
	FastFourierTransform(const_cast<float*>(src), tmp, count);
	for (int i = 0; i < count / 2; i++)
	{
		dst[i] = abs(tmp[i]);
	}
	delete[] tmp;
}
#endif
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

CHumRemoval::CHumRemoval()
{
	m_ApplyHighpassFilter = FALSE;
	m_ApplyCommonModeFilter = TRUE;
	m_prev_outl = 0;
	m_prev_outr = 0;
	m_prev_inl = 0;
	m_prev_inr = 0;
	m_PrevHpfR[0] = 0;
	m_PrevHpfR[1] = 0;
	m_PrevHpfL[0] = 0;
	m_PrevHpfL[1] = 0;
	m_PrevHpOutL[0] = 0.;
	m_PrevHpOutL[1] = 0.;
	m_PrevHpOutR[0] = 0.;
	m_PrevHpOutR[1] = 0.;
	m_DiffCutoffCoeffs[0] = 0.003575f;
	m_DiffCutoffCoeffs[1] = 0.9857f;
	m_HighpassCoeffs[0] = 1.99636f;
	m_HighpassCoeffs[1] = 0.996363312f;
	m_HighpassCoeffs[2] = 1.;
}

CClickRemoval::CClickRemoval()
{
//    memset(m_prev, 0, sizeof m_prev);
//    memset(m_prev3, 0, sizeof m_prev3);
	m_prev[0].Allocate(PREV_BUF_SIZE);
	m_prev[1].Allocate(PREV_BUF_SIZE);
	m_prev3[0].Allocate(PREV_BUF_SIZE);
	m_prev3[1].Allocate(PREV_BUF_SIZE);
	m_PrevIndex = 0;
	m_MeanPower[0] = 0;
	m_MeanPower[1] = 0;
	m_nStoredSamples = 0;
	//m_SpikeThreshold = 6.;
	//m_Deriv2Threshold = 60.;
	m_NextPossibleClickPosition[0] = 16 * CLICK_LENGTH;
	m_NextPossibleClickPosition[1] = 16 * CLICK_LENGTH;
	m_PrevDeriv[0] = 0;
	m_PrevDeriv[1] = 0;
	m_PrevDeriv2[0] = 0;
	m_PrevDeriv2[1] = 0;
	m_ClickDeriv3ThresholdScale = 0.1f;
	m_PowerToDeriv3RatioThreshold = 30.;
	m_MinDeriv3Threshold = 200*200;
	m_MinClickDeriv3BoundThreshold = 80.;
	m_nMaxClickLength = 32;
	m_nMinClickLength = 4;
	m_MeanPowerDecayRate = float(1./100.);  // 1 ms
	m_MeanPowerAttackRate = float(1./15.);  // 1 ms
	pInClicksFile = NULL;
	pOutClicksFile = NULL;
	PredefinedClickCurrentIndex = 0;
	m_PassTrough = FALSE;
	m_NoiseFloorThresholdScale = 3.;
}

CClickRemoval::~CClickRemoval()
{
	if (pInClicksFile)
	{
		fclose(pInClicksFile);
		pInClicksFile = NULL;
	}
	if (pOutClicksFile)
	{
		fclose(pOutClicksFile);
		pOutClicksFile = NULL;
	}
}

BOOL CClickRemoval::SetClickSourceFile(LPCTSTR szFilename)
{
	if (pInClicksFile)
	{
		fclose(pInClicksFile);
		pInClicksFile = NULL;
	}
	PredefinedClicks.RemoveAll();
	InClickFilename = szFilename;
	pInClicksFile = fopen(szFilename, "rt");
	if (NULL == pInClicksFile)
	{
		InClickFilename.Empty();
		return FALSE;
	}
	// read the file
	// every line contains 3 numbers: click position (in samples),
	//      Click length in the left channel, click length in the right

	// set allocation increment in 256 items
	PredefinedClicks.SetSize(0, 256);

	char line[256];
	while (NULL != fgets(line, 255, pInClicksFile))
	{
		StoredClickData data;
		int pos = 0, length_r = 0, length_l = 0;
		if ( 2 > sscanf(line, "%d %d %d", &pos, &length_l, &length_r))
		{
			continue;
		}
		data.Position = pos;
		data.Length[0] = length_l;
		data.Length[1] = length_r;
		// find position to add the data
		int i;
		for (i = PredefinedClicks.GetUpperBound() ; i >= 0; i--)
		{
			if (PredefinedClicks[i].Position <= data.Position)
			{
				break;
			}
		}
		if (i < 0 || PredefinedClicks[i].Position < data.Position)
		{
			TRACE("Inserting Item Pos=%d at %d\n", data.Position, i+1);
			PredefinedClicks.InsertAt(i + 1, data);
		}
		else if (PredefinedClicks[i].Position == data.Position)
		{
			// merge
			if (PredefinedClicks[i].Length[0] < data.Length[0])
			{
				PredefinedClicks[i].Length[0] = data.Length[0];
			}
			if (PredefinedClicks[i].Length[1] < data.Length[1])
			{
				PredefinedClicks[i].Length[1] = data.Length[1];
			}
		}
		else
		{
			TRACE("Inserting Item Pos=%d at %d\n", data.Position, i);
			PredefinedClicks.InsertAt(i, data);
		}
	}
	PredefinedClickCurrentIndex = 0;
	fclose(pInClicksFile);
	pInClicksFile = NULL;
	return TRUE;
}

BOOL CClickRemoval::SetClickLogFile(LPCTSTR szFilename)
{
	if (pOutClicksFile)
	{
		fclose(pOutClicksFile);
		pOutClicksFile = NULL;
	}
	OutClickFilename.Empty();
	if (NULL != szFilename && szFilename[0] != 0)
	{
		OutClickFilename = szFilename;
		pOutClicksFile = fopen(szFilename, "wt");
	}
	return pOutClicksFile != 0;
}


CNoiseReduction::CNoiseReduction(int nFftOrder)
	:m_nFftOrder(nFftOrder),
	m_nBackSampleCount(0),
	m_nStoredSamples(0),
	m_MinFrequencyToProcess(3000.),
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
	m_NearMaskingCoeff(1.),
	m_NearMaskingDecayTimeHigh(40.),    // miliseconds
	m_NearMaskingDecayTimeLow(100.),    // miliseconds
	m_ToneOverNoisePreference(2.5),     // in nepers
#ifdef _DEBUG
	m_TotalBandProcessed(0),
	m_TransientBandFound(0),
	m_PhaseFilteredInBands(0),
	m_StationaryBandCancelled(0),
	m_NoiselikeBandCancelled(0),
	m_MaxLevelInBand(-20),
	m_MinLevelInBand(20),
#endif

	m_Window(new float[nFftOrder]),
	m_BackBuffer(new float[nFftOrder*2][2]),
	m_AccumBuffer(new float[nFftOrder / 2][2])
{
	m_FftInBuffer[0] = new DATA[nFftOrder];
	m_FftInBuffer[1] = new DATA[nFftOrder];
	m_pParams[0] = new SIGNAL_PARAMS[nFftOrder / 2 + 1];
	m_pParams[1] = new SIGNAL_PARAMS[nFftOrder / 2 + 1];
	m_FftOutBuffer[0] = new complex<DATA>[nFftOrder / 2 + 1];
	m_FftOutBuffer[1] = new complex<DATA>[nFftOrder / 2 + 1];
	if (NULL != m_Window)
	{
		for (int i = 0; i < nFftOrder; i++)
		{
			// sine window
			m_Window[i] = float(sin((i + 0.5) * M_PI / nFftOrder));
		}
	}

	if (NULL != m_BackBuffer)
	{
		memset(m_BackBuffer, 0, nFftOrder * (2 * 2 * sizeof (float)));
	}

	if (NULL != m_AccumBuffer)
	{
		memset(m_AccumBuffer, 0, nFftOrder * (sizeof (float)));
	}

	if (NULL != m_pParams[0])
	{
		memset(m_pParams[0], 0, (nFftOrder /2 + 1) * sizeof m_pParams[0][0]);
	}

	if (NULL != m_pParams[1])
	{
		memset(m_pParams[1], 0, (nFftOrder /2 + 1) * sizeof m_pParams[1][0]);
	}
	// norm masking factor to make it independent of FFT order
	m_PowerScale = float(1. / nFftOrder);
	double MaskingFactor = 2 * m_PowerScale;
	//m_FarMaskingScale = MaskingFactor;

	for (int i = 0; i < FAR_MASKING_GRANULARITY; i++)
	{
		m_FarMaskingCoeffs[i][0] = 0.;
		for (int k = 1; k < FAR_MASKING_GRANULARITY; k++)
		{
			// power drops as reciprocal of the distance
			if (k == i)
			{
				m_FarMaskingCoeffs[i][k] = float(MaskingFactor * 5);
			}
			else
			{
				int n = k - i;
				if (n < 0) n = -n;
				float x = 1.f / (n + 1);
				m_FarMaskingCoeffs[i][k] = float(MaskingFactor * x);
			}
		}
	}
}

CNoiseReduction::~CNoiseReduction()
{
	delete[] m_Window; m_Window = NULL;
	delete[] m_BackBuffer; m_BackBuffer = NULL;
	delete[] m_AccumBuffer; m_AccumBuffer = NULL;
	delete[] m_FftInBuffer[0]; m_FftInBuffer[0] = NULL;
	delete[] m_FftInBuffer[1]; m_FftInBuffer[1] = NULL;
	delete[] m_FftOutBuffer[0]; m_FftOutBuffer[0] = NULL;
	delete[] m_FftOutBuffer[1]; m_FftOutBuffer[1] = NULL;
	delete[] m_pParams[0]; m_pParams[0] = NULL;
	delete[] m_pParams[1]; m_pParams[1] = NULL;
}

BOOL CWaveProc::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	m_SamplesPerSecond = pWf->nSamplesPerSec;
	m_InputChannels = pWf->nChannels;
	m_OutputChannels = pWf->nChannels;
	return pWf->wFormatTag == WAVE_FORMAT_PCM
			&& pWf->wBitsPerSample == 16
			&& pWf->nBlockAlign == pWf->nChannels * 2 //pWf->wBitsPerSample / 8
			&& pWf->nAvgBytesPerSec == pWf->nSamplesPerSec * pWf->nBlockAlign;
}
BOOL CHumRemoval::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	return CWaveProc::SetAndValidateWaveformat(pWf);
	//&& pWf->nChannels == 2
	//&& pWf->nSamplesPerSec == 44100;
}

BOOL CClickRemoval::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	return CWaveProc::SetAndValidateWaveformat(pWf)
			//&& pWf->nSamplesPerSec == 44100
	;
}

BOOL CNoiseReduction::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	m_SamplesPerSec = pWf->nSamplesPerSec;
	return CWaveProc::SetAndValidateWaveformat(pWf)
			&& pWf->nSamplesPerSec == 44100;
}

BOOL CBatchProcessing::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	for (int i = 0; i < m_Stages.GetSize(); i++)
	{
		if (FALSE == m_Stages[i].Proc->SetAndValidateWaveformat(pWf))
			return FALSE;
	}
	return TRUE;
}

int CWaveProc::ProcessSound(char const * pInBuf, char * pOutBuf,
							int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;
	if ( ! CheckForMinBufferSize(pInBuf, pOutBuf, nInBytes, nOutBytes,
								pUsedBytes, & nSavedBytes,
								GetMinInputBufSize(), GetMinOutputBufSize()))
	{
		return nSavedBytes;
	}
	return ProcessSoundBuffer(pInBuf, pOutBuf, nInBytes, nOutBytes, pUsedBytes);
}

BOOL CWaveProc::CheckForMinBufferSize(char const * &pIn, char * &pOut,
									int &nInBytes, int &nOutBytes,
									int * pUsedBytes, int * pSavedBytes,
									int nMinInBytes, int nMinOutBytes)
{
	int nSavedBytes = 0;
	*pSavedBytes = 0;
	*pUsedBytes = 0;
	if (m_TmpOutBufPut != m_TmpOutBufGet)
	{
		for ( ;m_TmpOutBufPut > m_TmpOutBufGet
			&& nOutBytes > 0; m_TmpOutBufGet++, pOut++, nOutBytes--, nSavedBytes++)
		{
			*pOut = m_TmpOutBuf[m_TmpOutBufGet];
		}
		if (m_TmpOutBufPut == m_TmpOutBufGet)
		{
			m_TmpOutBufPut = 0;
			m_TmpOutBufGet = 0;
		}
	}
	if (nOutBytes <= 0)
	{
		*pSavedBytes = nSavedBytes;
		return FALSE;
	}

	if (NULL != pIn
		&& m_TmpInBufPut > 0
		&& m_TmpInBufPut < nMinInBytes)
	{
		for (; nInBytes > 0 && m_TmpInBufPut < nMinInBytes; nInBytes--,
			m_TmpInBufPut++, *pUsedBytes++, pIn++)
		{
			m_TmpInBuf[m_TmpInBufPut] = *pIn;
		}
		if (nMinInBytes == m_TmpInBufPut)
		{
			int nUsed = 0;
			int nSaved = ProcessSoundBuffer(m_TmpInBuf, pOut, nMinInBytes, nOutBytes, & nUsed);
			if (nUsed != nMinInBytes)
			{
				TRACE("Couldn't process min bytes!\n");
				return FALSE;  // error!!
			}
			m_TmpInBufPut = 0;
			nSavedBytes += nSaved;
			pOut += nSaved;
			nOutBytes -= nSaved;
		}
		else
		{
			*pSavedBytes = nSavedBytes;
			return FALSE;
		}
	}
	// if there is too little input data, save it in the temp buffer and return
	// if there is too little space for output data,
	if (NULL != pIn
		&& nInBytes != 0 && nInBytes < nMinInBytes)
	{
		for (; nInBytes > 0 && m_TmpInBufPut < nMinInBytes; nInBytes--,
			m_TmpInBufPut++, *pUsedBytes++, pIn++)
		{
			m_TmpInBuf[m_TmpInBufPut] = *pIn;
		}
		*pSavedBytes = nSavedBytes;
		return FALSE;
	}

	if (nOutBytes < nMinOutBytes)
	{
		int nUsed = 0;
		m_TmpOutBufGet = 0;
		m_TmpOutBufPut = 0;
		int nSaved = ProcessSoundBuffer(pIn, m_TmpOutBuf, nInBytes, nMinOutBytes, & nUsed);
		m_TmpOutBufPut = nSaved;
		* pUsedBytes += nUsed;
		for ( ; nOutBytes > 0 && m_TmpOutBufGet < m_TmpOutBufPut; m_TmpOutBufGet++,
			nOutBytes--, nSavedBytes++, pOut++)
		{
			*pOut = m_TmpOutBuf[m_TmpOutBufGet];
		}
		if (m_TmpOutBufGet == m_TmpOutBufPut)
		{
			m_TmpOutBufGet = 0;
			m_TmpOutBufPut = 0;
		}
		*pSavedBytes = nSavedBytes;
		return FALSE;
	}
	return TRUE;
}

void CHumRemoval::SetDifferentialCutoff(double frequency)
{
	m_DiffCutoffCoeffs[1] = 1. - M_PI * frequency / m_SamplesPerSecond;
	m_DiffCutoffCoeffs[0] = 0.25 * (1. - m_DiffCutoffCoeffs[1]);
	TRACE("m_DiffCutoffCoeffs=%.6f, %.6f\n", m_DiffCutoffCoeffs[0],
		m_DiffCutoffCoeffs[1]);
}
void CHumRemoval::SetHighpassCutoff(double frequency)
{
	// pole for second order filter
	double a = 1. - 1.5537739 * M_PI * frequency / m_SamplesPerSecond;
	m_HighpassCoeffs[0] = 2. * a;
	m_HighpassCoeffs[1] = a * a;
	// norm coefficient
	m_HighpassCoeffs[2] = (m_HighpassCoeffs[0] + m_HighpassCoeffs[1] + 1.) * 0.25;
	TRACE("m_HighpassCoeffs=%.6f, %.6f, %.6f\n", m_HighpassCoeffs[0],
		m_HighpassCoeffs[1],  m_HighpassCoeffs[2]);
}

int CHumRemoval::ProcessSoundBuffer(char const * pIn, char * pOut,
									int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;

	int nInSamples = nInBytes / sizeof (__int16);
	int nOutSamples = nOutBytes / sizeof (__int16);
	__int16 const * pInBuf = (__int16 *) pIn;
	__int16 * pOutBuf = (__int16 *) pOut;


	// process the data
	int nSamples = __min(nInSamples, nOutSamples) / m_InputChannels;
	if (0 == nSamples || NULL == pInBuf)
	{
		return nSavedBytes;   // no delayed samples in history
	}
	float curr_l;
	float curr_r = 0.;
	for (int i = 0; i < nSamples * m_InputChannels; i += m_InputChannels)
	{
		if (m_ApplyHighpassFilter)
		{
			// apply additional highpass 2nd order filter to both channels
			// with cutoff frequency 50 Hz
			// (1-2z + z^2)/(a^2 -2az + z^2)
			double outL = (float(pInBuf[i] - m_PrevHpfL[0] - m_PrevHpfL[0] + m_PrevHpfL[1])
							+ m_HighpassCoeffs[0] * m_PrevHpOutL[0] - m_HighpassCoeffs[1] * m_PrevHpOutL[1]);
			m_PrevHpOutL[1] = m_PrevHpOutL[0];
			m_PrevHpOutL[0] = float(outL);
			curr_l = float(outL * m_HighpassCoeffs[2]);
			m_PrevHpfL[1] = m_PrevHpfL[0];
			m_PrevHpfL[0] = pInBuf[i];

			if (2 == m_InputChannels)
			{
				double outR = (float(pInBuf[i+1] - m_PrevHpfR[0] - m_PrevHpfR[0] + m_PrevHpfR[1])
								+ m_HighpassCoeffs[0] * m_PrevHpOutR[0] - m_HighpassCoeffs[1] * m_PrevHpOutR[1]);
				m_PrevHpOutR[1] = m_PrevHpOutR[0];
				m_PrevHpOutR[0] = float(outR);
				curr_r = float(outR * m_HighpassCoeffs[2]);

				m_PrevHpfR[1] = m_PrevHpfR[0];
				m_PrevHpfR[0] = pInBuf[i + 1];
			}
		}
		else
		{
			curr_l = pInBuf[i];
			curr_r = pInBuf[i + 1];
		}
		if (m_ApplyCommonModeFilter
			&& 2 == m_InputChannels)
		{
			m_prev_outl = float(
								(curr_l + m_prev_inl) * m_DiffCutoffCoeffs[0]
								+ m_DiffCutoffCoeffs[1] * m_prev_outl);
			m_prev_outr = float(
								(curr_r + m_prev_inr) * m_DiffCutoffCoeffs[0]
								+ m_DiffCutoffCoeffs[1] * m_prev_outr);
			float hpf_l = m_prev_inl - m_prev_outl;
			float hpf_r = m_prev_inr - m_prev_outr;
			m_prev_inl = curr_l;
			m_prev_inr = curr_r;
			curr_l = (float)floor(hpf_l + m_prev_outr + 0.5);
			curr_r = (float)floor(hpf_r + m_prev_outl + 0.5);
		}
		if (m_ChannelsToProcess != 1)
		{
			pOutBuf[i] = DoubleToShort (curr_l);
		}
		else
		{
			pOutBuf[i] = pInBuf[i];
		}
		if (2 == m_InputChannels)
		{
			if (m_ChannelsToProcess != 0)
			{
				pOutBuf[i + 1] = DoubleToShort(curr_r);
			}
			else
			{
				pOutBuf[i + 1] = pInBuf[i + 1];
			}
		}
	}

	*pUsedBytes += nSamples * m_InputChannels * sizeof (__int16);
	return nSavedBytes + nSamples * m_InputChannels * sizeof (__int16);
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

	__int16 TempBuf[BufferSamples];
	const int InterpolateOffset = InterpolationOverlap;
	const int ReadStartOffset = nLeftIndex - InterpolationOverlap;
	const int WriteStartOffset = nLeftIndex - PreInterpolateSamples;
	const int WriteBufferOffset = InterpolationOverlap  - PreInterpolateSamples;

	int i;
	for (i = 0; i < InterpolateSamples + 2 * InterpolationOverlap; i++)
	{
		TempBuf[i] = __int16(data[ReadStartOffset + i]);
	}

	InterpolateGap(TempBuf, InterpolateOffset, InterpolateSamples, 1, BigGap);
	// copy back
	for (i = 0; i < InterpolateSamples + PreInterpolateSamples + PostInterpolateSamples; i++)
	{
		data[WriteStartOffset + i] = TempBuf[WriteBufferOffset + i];
	}
}

void CClickRemoval::InterpolateBigGap(__int16 data[], int nLeftIndex, int ClickLength, int nChans)
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
		FftOrder +=FftOrder;
	}
	TRACE("FFtOrder used for interpolation: %d\n", FftOrder);
	if (FftOrder > MAX_FFT_ORDER)
	{
		return;
	}
	// extrapolate ClickLength + ClickLength / 2 - the gap and
	// the right neighborhood
	int ExtrapolatedLength = ClickLength + ClickLength / 2;
	int i;
	for (i = 0; i < FftOrder + ExtrapolatedLength; i++)
	{
		x[i] = float(data[nChans * (nLeftIndex - FftOrder - ExtrapolatedLength+ i)]);
	}
	FastFourierTransform(x, y2, FftOrder);
	FastFourierTransform(x + ExtrapolatedLength, y1, FftOrder);
	// calculate another set of coefficients
	// leave only those frequencies with up to ClickLength/10 period
	//if (nMaxFreq > FftOrder/2)

	for (i = 1; i <= FftOrder/2; i++)
	{
		if (y1[i].real() != 0.
			|| y1[i].imag() != 0.)
		{
			complex<float> rot = y2[i] / y1[i];
			y2[i] *= rot / abs(rot);
		}
	}
	FastInverseFourierTransform(y2, x, FftOrder);
	// last ClickLength*2 samples are of interest
	// save the result
	// ClickLength is copied to the extrapolated area,
	for (i = 0; i < ClickLength; i++)
	{
		data[nChans * (nLeftIndex + i)] =
			DoubleToShort(
						x[FftOrder - (ClickLength + ClickLength / 2) + i]);
	}

	// ClickLength/2 are merged with the samples before the extrapolation,
	for (i = 0; i < ClickLength / 2; i++)
	{
		data[nChans * (nLeftIndex + ClickLength + i)] =
			DoubleToShort(
						(x[FftOrder - ClickLength / 2 + i] * (ClickLength / 2 - i - 0.5)
							+ data[nChans * (nLeftIndex + ClickLength + i)] * (i + 0.5))
						/ float(ClickLength / 2));
	}

	// ClickLength/2 are merged with the samples after the extrapolation,
	int ClickLen1 = ClickLength - ClickLength / 2;
	for (i = 0; i < ClickLen1; i++)
	{
		data[nChans * (nLeftIndex - ClickLen1 + i)] =
			DoubleToShort(
						(x[FftOrder - ClickLength * 2 + i] * (i + 0.5)
							+ data[nChans * (nLeftIndex - ClickLen1 + i)] * (ClickLen1 - i - 0.5))
						/ float(ClickLen1));
	}
}

void CClickRemoval::InterpolateGap(__int16 data[], int nLeftIndex, int ClickLength, int nChans, bool BigGap)
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
	double Y[20], X[20];
	int n;
	int InterpolationOrder;
	if (ClickLength <= 32)
	{
		InterpolationOrder = 10;
		ASSERT(nLeftIndex - (ClickLength / 2 * (InterpolationOrder - 2) + 1) >= 0);
		for (n = 0; n < InterpolationOrder - 1; n += 2)
		{
			X[n] = - (ClickLength / 2 * n + 1);
			Y[n] = data[nChans * (nLeftIndex - (ClickLength / 2 * n + 1))];
			X[n + 1] = ClickLength + ClickLength / 2 * n;
			Y[n + 1] = data[nChans * (nLeftIndex + ClickLength + ClickLength / 2 * n)];
		}
	}
	else
	{
		InterpolationOrder = 20;
		ASSERT(nLeftIndex - (ClickLength / 4 * (InterpolationOrder - 2) + 1) >= 0);
		for (n = 0; n < InterpolationOrder - 1; n += 2)
		{
			X[n] = - (ClickLength / 4 * n + 1);
			Y[n] = data[nChans * (nLeftIndex - (ClickLength / 4 * n + 1))];
			X[n + 1] = ClickLength + ClickLength / 4 * n;
			Y[n + 1] = data[nChans * (nLeftIndex + ClickLength + ClickLength / 4 * n)];
		}
	}
	// perform Lagrange interpolation
	for (n = 0; n < ClickLength; n++)
	{
		double x = n;
		double y = 0;
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
			y += a;
		}
		data[nChans * (nLeftIndex + n)] = DoubleToShort(y);
	}
}

int CClickRemoval::ProcessSoundBuffer(char const * pIn, char * pOut,
									int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;

	int nInSamples = nInBytes / (m_InputChannels * sizeof (__int16));
	int nOutSamples = nOutBytes / (m_InputChannels * sizeof (__int16));
	__int16 const * pInBuf = (__int16 *) pIn;
	__int16 * pOutBuf = (__int16 *) pOut;

	// process the data

	if (NULL == pInBuf)
	{
		// flush backlogged samples
		// nSamples - size of out buffer / size of samples
		int nBackSamples = m_PrevIndex - m_nStoredSamples;
		if (nBackSamples > nOutSamples)
		{
			nBackSamples = nOutSamples;
		}
		if (nBackSamples <= 0)
		{
			return nSavedBytes;
		}
		for (int i = 0; i < nBackSamples; i++)
		{
			for (int ch = 0; ch < m_InputChannels; ch++)
			{
				*pOutBuf = m_prev[ch][i-1];
				pOutBuf++;
			}
		}
		m_prev[0].Advance(nBackSamples);
		m_prev[1].Advance(nBackSamples);
		m_nStoredSamples += nBackSamples;
		return nSavedBytes + nBackSamples * m_InputChannels * sizeof (__int16);
	}

	int nClickIndex;
	int nStoreIndex = 0;
	int PrevIndex = m_PrevIndex;
	int nSamples = __min(nInSamples, nOutSamples);
	for (int ch = 0; ch < m_InputChannels; ch++)
	{
		int FftIn[CLICK_LENGTH];   // additional space for click length search
		//float FftOut[FFT_ORDER / 2];
		PrevIndex = m_PrevIndex;
		nStoreIndex = 0;
		nClickIndex = PredefinedClickCurrentIndex;
		for (int i = 0; i < nSamples * m_InputChannels; i += m_InputChannels)
		{
			m_prev[ch][ANALYZE_LAG] = pInBuf[i];
			// FFT is performed for every FFT_ORDER/2 samples
			int f;
			int deriv = m_prev[ch][CLICK_LENGTH / 2] - m_prev[ch][CLICK_LENGTH / 2 - 1];
			int deriv2 = deriv - m_PrevDeriv[ch];
			m_PrevDeriv[ch] = deriv;
			int deriv3 = deriv2 - m_PrevDeriv2[ch];
			m_PrevDeriv2[ch] = deriv2;
			m_prev3[ch][CLICK_LENGTH / 2] = deriv3;
			int Deriv3Threshold = int(m_MeanPower[ch] * m_PowerToDeriv3RatioThreshold);

			float power = float(m_prev3[ch][0]);
			power = power * power;

			if (power > m_MeanPower[ch])
			{
				m_MeanPower[ch] += (power - m_MeanPower[ch])
									* m_MeanPowerAttackRate;
			}
			else
			{
				m_MeanPower[ch] += (power - m_MeanPower[ch])
									* m_MeanPowerDecayRate;
			}

			BOOL ClickFound = FALSE;
			int nLeftIndex = CLICK_LENGTH;
			int nRightIndex = 0;
			int ClickLength = 0;
			// check predefined click list
			while(nClickIndex <= PredefinedClicks.GetUpperBound()
				&& PredefinedClicks[nClickIndex].Position < PrevIndex - ANALYZE_LAG)
			{
				nClickIndex++;
			}
			if (nClickIndex <= PredefinedClicks.GetUpperBound())
			{
				if (PredefinedClicks[nClickIndex].Position == PrevIndex - ANALYZE_LAG
					&& 0 != PredefinedClicks[nClickIndex].Length[ch])
				{
					nLeftIndex = 0;
					ClickLength = PredefinedClicks[nClickIndex].Length[ch];
					nRightIndex = nLeftIndex + ClickLength;
					// force interpolation on this position
					ClickFound = TRUE;
					m_NextPossibleClickPosition[ch] = PrevIndex + ClickLength * 2;
					nClickIndex++;
				}
			}

			if (!ClickFound) do
			{
				nLeftIndex = CLICK_LENGTH;
				nRightIndex = 0;
				ClickLength = 0;
				if (Deriv3Threshold < m_MinDeriv3Threshold)
				{
					Deriv3Threshold = (int)m_MinDeriv3Threshold;
				}

				if (PrevIndex < m_NextPossibleClickPosition[ch]
					//&& deriv2 > m_MaxDeriv2Estimate[ch]
					//&& deriv3 > m_MaxDeriv2Estimate[ch] * 0.7
					|| deriv3 * deriv3 < Deriv3Threshold
					)
				{
					break;
				}
				// spike detected
				// find its exact position as max of
				// third derivative
				for (f = 0; f < CLICK_LENGTH; f++)
				{
					FftIn[f] = m_prev[ch][f];
				}
				int nMaxDeriv3Pos = 0;
				int OldDeriv = FftIn[2] - FftIn[1];
				int deriv3, OldDeriv2 = FftIn[2] - 2 * FftIn[1] + FftIn[0];
				int deriv2, MaxDeriv3 = 0;

				// find max abs third derivative point,
				for (f = 2; f < CLICK_LENGTH-1; f++)
				{
					int NewDeriv =  FftIn[f + 1] - FftIn[f];

					deriv2 = NewDeriv - OldDeriv;
					deriv3 = int(fabs(deriv2 - OldDeriv2));
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

				int Deriv3Threshold = int(MaxDeriv3 * m_ClickDeriv3ThresholdScale);

				if (Deriv3Threshold < m_MinClickDeriv3BoundThreshold)
				{
					Deriv3Threshold = int(m_MinClickDeriv3BoundThreshold);
				}
				int NoiseFloorThreshold = int(sqrt(m_MeanPower[ch]) * m_NoiseFloorThresholdScale);
				if (Deriv3Threshold < NoiseFloorThreshold)
				{
					Deriv3Threshold = NoiseFloorThreshold;
				}

				OldDeriv = FftIn[2] - FftIn[1];
				OldDeriv2 = FftIn[2] - 2 * FftIn[1] + FftIn[0];

				// find max abs third derivative point,
				for (f = 2; f < CLICK_LENGTH-1; f++)
				{
					int NewDeriv =  FftIn[f + 1] - FftIn[f];

					deriv2 = NewDeriv - OldDeriv;
					deriv3 = abs(deriv2 - OldDeriv2);
					OldDeriv = NewDeriv;
					OldDeriv2 = deriv2;

					if (abs(deriv3) >= Deriv3Threshold)
					{
						if (nLeftIndex >= f) nLeftIndex = f - 2;
						if (nRightIndex <= f) nRightIndex = f+2;
					}
				}

				ClickLength = (nRightIndex - nLeftIndex + 1) & ~1;
				m_NextPossibleClickPosition[ch] = PrevIndex + 2 * ClickLength;
				if (ClickLength > m_nMaxClickLength)
				{
					break;  // too wide clicks are ignored
					//ClickLength = m_nMaxClickLength;
					//nLeftIndex = nMaxDeriv3Pos - ClickLength/2;
				}
				else if (ClickLength < m_nMinClickLength)
				{
					ClickLength = m_nMinClickLength;
					nLeftIndex = nMaxDeriv3Pos - ClickLength/2;
				}

				nRightIndex = nLeftIndex + ClickLength;
				ClickFound = TRUE;
			}
			while(0);

			if (ClickFound && pOutClicksFile)
			{
				if (0 == ch)
				{
					fprintf(pOutClicksFile, "%d %d 0\n",
							PrevIndex + nLeftIndex - ANALYZE_LAG, ClickLength);
				}
				else
				{
					fprintf(pOutClicksFile, "%d 0 %d\n",
							PrevIndex + nLeftIndex - ANALYZE_LAG, ClickLength);
				}
			}

			if (ClickFound && ! m_PassTrough)
			{
				InterpolateGap(m_prev[ch], nLeftIndex, ClickLength,
								ClickLength > 16);
			}
			// output is delayed by 64 samples
			if (PrevIndex > ANALYZE_LAG*2-2)
			{
#if 1
				pOutBuf[nStoreIndex * m_InputChannels] =
					__int16(m_prev[ch][ 1-ANALYZE_LAG /*m_nStoredSamples + nStoreIndex - PrevIndex + ANALYZE_LAG*/]);
#else
				// store 3rd derivative
				pOutBuf[nStoreIndex * m_InputChannels] =
					__int16(m_prev3[ch][1-ANALYZE_LAG]);
#endif
				nStoreIndex++;
			}
			m_prev[ch].Advance();
			m_prev3[ch].Advance();
			//pOutBuf[i] = pInBuf[i];
			PrevIndex++;
		}
		pInBuf++;
		pOutBuf++;
	}

	PredefinedClickCurrentIndex = nClickIndex;

	int nStoredSamples = nStoreIndex;
	m_PrevIndex = PrevIndex;
	m_nStoredSamples += nStoredSamples;
	*pUsedBytes += nSamples * (m_InputChannels * sizeof (__int16));
	return nSavedBytes + m_InputChannels * sizeof(__int16) * nStoredSamples;
}

// smp - source FFT sample
// pNr - points to the parent CNoiseReduction object
// nSample - number of the sample in FFT set
void CNoiseReduction::SIGNAL_PARAMS::AnalyzeFftSample(complex<DATA> smp, CNoiseReduction * pNr, int nSample)
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
	if (0 && (dLevelChange > pNr->m_ThresholdOfTransient
			|| dLevelChange < -pNr->m_ThresholdOfTransient))
	{
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
		double PhaseError = dPhase - sp_AvgPhase;
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

int CNoiseReduction::ProcessSoundBuffer(char const * pIn, char * pOut,
										int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;
	int nChans = m_InputChannels;

	int nInSamples = nInBytes / (nChans * sizeof (__int16));
	int nOutSamples = nOutBytes / (nChans * sizeof (__int16));
	__int16 const * pInBuf = (__int16 *) pIn;
	__int16 * pOutBuf = (__int16 *) pOut;

	// process the data
	int nSamples = __min(nInSamples, nOutSamples);
	const int PREV_MASK = (m_nFftOrder * 2) - 1;
	const unsigned int ANALYZE_LAG = m_nFftOrder;
	unsigned MinFrequencyToProcess = unsigned(m_MinFrequencyToProcess / 44100. * m_nFftOrder);

	if (NULL == pInBuf)
	{
		// flush backlogged samples
		// nSamples - size of out buffer / size of samples
		int nBackSamples = m_nBackSampleCount - m_nStoredSamples;
		if (nBackSamples > nOutSamples)
		{
			nBackSamples = nOutSamples;
		}
		if (nBackSamples <= 0)
		{
			return nSavedBytes;
		}
		for (int i = 0; i < nBackSamples; i++)
		{
			for (int ch = 0; ch < nChans; ch++)
			{
				// TODO: use conversion routine
				*pOutBuf = __int16(m_BackBuffer[(m_nStoredSamples + ANALYZE_LAG) & PREV_MASK][ch]);
				pOutBuf++;
			}
			m_nStoredSamples++;
		}
		return nSavedBytes + nChans * sizeof(__int16) * nBackSamples;
	}

	int ch;
	unsigned nBacklogIndex = m_nBackSampleCount;
	int nStoredSamples = 0;
	unsigned n;
	for (int i = 0; i < nSamples; i ++)
	{
		for (ch = 0; ch < nChans; ch++)
		{
			// process the samples
			m_BackBuffer[(nBacklogIndex + ANALYZE_LAG) & PREV_MASK][ch] = pInBuf[0];
			pInBuf++;
		}
		if (0 == ((nBacklogIndex + 1) & (m_nFftOrder / 2 - 1)))
		{
			for (ch = 0; ch < nChans; ch++)
			{
				// process the current FFT
				for (n = 0; n < m_nFftOrder; n++)
				{
					m_FftInBuffer[ch][n] = m_Window[n] *
											m_BackBuffer[(nBacklogIndex + 1 + n) & PREV_MASK][ch];
				}
				FastFourierTransform(m_FftInBuffer[ch], m_FftOutBuffer[ch], m_nFftOrder);

			}

			// process FFT result
			unsigned f;
			float FarMasking[FAR_MASKING_GRANULARITY];
			float SubbandPower[FAR_MASKING_GRANULARITY];

			for (f = 0; f < m_nFftOrder / 2 + 1; f++)
			{
				for (ch = 0; ch < nChans; ch++)
				{
					m_pParams[ch][f].AnalyzeFftSample(m_FftOutBuffer[ch][f], this, f);
				}
				//m_pParams[0][f].
				// find vector deviation
				// check if the signal is noise-like or is narrow-band.
				// if FreqDev is less than PI/8, signal is narrow-band,
			}

			f = 0;
			int n;
			for (n = 0; n < FAR_MASKING_GRANULARITY; n++)
			{
				FarMasking[n] = 0.;
				SubbandPower[n] = 0.;
				for (unsigned k = 0; k < m_nFftOrder / (FAR_MASKING_GRANULARITY*2); k++, f++)
				{
					for (ch = 0; ch < nChans; ch++)
					{
						SubbandPower[n] += m_pParams[ch][f].sp_Power;
					}
				}
			}
			for (f = 0; f < FAR_MASKING_GRANULARITY; f++)
			{
				FarMasking[f] = 0.;
				for (n = 0; n < FAR_MASKING_GRANULARITY; n++)
				{
					FarMasking[f] +=
						SubbandPower[n] * m_FarMaskingCoeffs[f][n];
				}
				FarMasking[f] /= nChans;
			}
			// calculate fine masking function, using far masking table
			// and near masking factors. Just filter the power in frequency and time
			// add far masking value
			for (f = 0; f < m_nFftOrder / 2; f++)
			{
				for (ch = 0; ch < nChans; ch++)
				{
					m_pParams[ch][f].sp_MaskingPower = float(
															m_pParams[ch][f].sp_Power * m_NearMaskingCoeff
															+ (1. - m_NearMaskingCoeff) * FarMasking[f * FAR_MASKING_GRANULARITY*2 / m_nFftOrder])
					;
				}
			}
			// filter in frequency in two directions
			double PrevFilteredPower[2];
			// those are calculated from m_NearMaskingDecayTime* :
			float MaskingSpectralDecayNormLow =
				// coeff to filter masking function in frequencies
				m_NearMaskingDecayDistanceLow / m_SamplesPerSec * m_nFftOrder;
			if (MaskingSpectralDecayNormLow < 1.)
			{
				MaskingSpectralDecayNormLow = 1.;
			}

			float MaskingSpectralDecayNormHigh =
				// coeff to filter masking function in frequencies
				m_NearMaskingDecayDistanceHigh / m_SamplesPerSec * m_nFftOrder;
			if (MaskingSpectralDecayNormHigh < 1.)
			{
				MaskingSpectralDecayNormHigh = 1.;
			}
			float MaskingDistanceDelta =
				(MaskingSpectralDecayNormHigh - MaskingSpectralDecayNormLow) / (m_nFftOrder /2);

			PrevFilteredPower[0] = 0.;
			PrevFilteredPower[1] = 0.;
			float ToneEmphasis = (float)exp(m_ToneOverNoisePreference);
			for (f = 0; f < m_nFftOrder / 2+1; f++)
			{
				double decay = 1. / MaskingSpectralDecayNormLow;
				for (ch = 0; ch < nChans; ch++)
				{
					double power;
					if (m_pParams[ch][f].m_TonalBand)
					{
						power = m_pParams[ch][f].sp_MaskingPower * ToneEmphasis;
					}
					else
					{
						power = m_pParams[ch][f].sp_MaskingPower;
					}
					PrevFilteredPower[ch] += (power - PrevFilteredPower[ch]) * decay;
					m_pParams[ch][f].sp_MaskingPower = float(PrevFilteredPower[ch]);
				}
				MaskingSpectralDecayNormLow += MaskingDistanceDelta;
			}
			PrevFilteredPower[0] = 0.;
			PrevFilteredPower[1] = 0.;
			for (int ff = m_nFftOrder / 2; ff >= 0; ff--)
			{
				MaskingSpectralDecayNormHigh -= MaskingDistanceDelta;
				double decay = 1. / MaskingSpectralDecayNormHigh;
				for (ch = 0; ch < nChans; ch++)
				{
					PrevFilteredPower[ch] += (m_pParams[ch][ff].sp_MaskingPower - PrevFilteredPower[ch])
											* decay;
					m_pParams[ch][ff].sp_MaskingPower = float(PrevFilteredPower[ch]);
				}
			}
			float MaskingTemporalDecayNormLow = float(
													// coeff to filter masking function in time
													//m_NearMaskingDecayTimeLow * 0.001 * m_SamplesPerSec / (m_nFftOrder / 2);
													m_NearMaskingDecayTimeLow * 0.002 * m_SamplesPerSec / m_nFftOrder);
			if (MaskingTemporalDecayNormLow < 1.)
			{
				MaskingTemporalDecayNormLow = 1.;
			}

			float MaskingTemporalDecayNormHigh = float(
														// coeff to filter masking function in time
														m_NearMaskingDecayTimeHigh * 0.002 * m_SamplesPerSec / m_nFftOrder);
			if (MaskingTemporalDecayNormHigh < 1.)
			{
				MaskingTemporalDecayNormHigh = 1.;
			}

			float DecayDelta =
				(MaskingTemporalDecayNormHigh - MaskingTemporalDecayNormLow) / (m_nFftOrder /2);
			// filter in time
			for (f = 0; f < m_nFftOrder / 2 + 1; f++)
			{
				double decay = 1. / MaskingTemporalDecayNormLow;
				for (ch = 0; ch < nChans; ch++)
				{
					if (m_pParams[ch][f].sp_MaskingPower < m_pParams[ch][f].sp_PrevMaskingPower)
					{
						m_pParams[ch][f].sp_PrevMaskingPower += float(decay *
								(m_pParams[ch][f].sp_MaskingPower - m_pParams[ch][f].sp_PrevMaskingPower));
						m_pParams[ch][f].sp_MaskingPower = m_pParams[ch][f].sp_PrevMaskingPower;
					}
					else
					{
						m_pParams[ch][f].sp_PrevMaskingPower = m_pParams[ch][f].sp_MaskingPower;
					}
				}
				MaskingTemporalDecayNormLow += DecayDelta;
			}
#ifdef _DEBUG
			// compute statistics:
			// total power (original and after processing)
			// max power in band, min power in band,
			// max masking, min masking
			if (0) {
				double TotalPower1=0, TotalPower2=0;
				double MaxBandPower1=0, MaxBandPower2=0;
				double MinBandPower1=1.e10,MinBandPower2=1.e10;
				for (ch = 0; ch < nChans; ch++)
				{
					for (f = 2; f < m_nFftOrder / 2 - 1; f++)
					{
						TotalPower1 += m_pParams[ch][f].sp_Power;
						TotalPower2 += m_pParams[ch][f].sp_MaskingPower;
						if (MaxBandPower1 < m_pParams[ch][f].sp_Power)
						{
							MaxBandPower1 = m_pParams[ch][f].sp_Power;
						}
						if (MaxBandPower2 < m_pParams[ch][f].sp_MaskingPower)
						{
							MaxBandPower2 = m_pParams[ch][f].sp_MaskingPower;
						}
						if (MinBandPower1 > m_pParams[ch][f].sp_Power)
						{
							MinBandPower1 = m_pParams[ch][f].sp_Power;
						}
						if (MinBandPower2 > m_pParams[ch][f].sp_MaskingPower)
						{
							MinBandPower2 = m_pParams[ch][f].sp_MaskingPower;
						}
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
				for (ch = 0; ch < nChans; ch++)
				{
					for (f = 0; f < m_nFftOrder / 2 - 1; f++)
					{
						AvgFreqDev += m_pParams[ch][f].sp_FreqDev;
						if (MinFreqDev > m_pParams[ch][f].sp_FreqDev)
						{
							MinFreqDev = m_pParams[ch][f].sp_FreqDev;
						}
						if (MaxFreqDev < m_pParams[ch][f].sp_FreqDev)
						{
							MaxFreqDev = m_pParams[ch][f].sp_FreqDev;
						}
						if (m_pParams[ch][f].m_TonalBand)
						{
							TonalBands++;
						}
					}
				}
				TRACE("Min Freq Dev =%g, Max Freq Dev=%g, avg=%g, tonal bands=%d\n",
					MinFreqDev, MaxFreqDev, AvgFreqDev / (m_nFftOrder / 2),
					TonalBands);
			}
#endif
//#define DB_TO_NEPER 0.115129254
			// post-process output data
			double NoiseFloorDelta = exp((m_LevelThresholdForNoiseHigh - m_LevelThresholdForNoiseLow)
										/ (m_nFftOrder / 2 - MinFrequencyToProcess));
			double SuppressionLimit = exp(-m_MaxNoiseSuppression);

			for (ch = 0; ch < nChans; ch++)
			{
				double NoiseFloor = exp(m_LevelThresholdForNoiseLow + m_MaxNoiseSuppression / m_NoiseReductionRatio);
				for (f = 0; f < m_nFftOrder / 2 + 1; f++)
				{

					m_FftOutBuffer[ch][f] = m_pParams[ch][f].sp_FftIn[1];

					if (f >= MinFrequencyToProcess)
					{
						if (m_pParams[ch][f].sp_MaskingPower < NoiseFloor)
						{
							double suppress =
								pow( m_pParams[ch][f].sp_MaskingPower / NoiseFloor,
									m_NoiseReductionRatio);
							if (suppress < SuppressionLimit)
							{
								suppress = SuppressionLimit;
							}
							m_FftOutBuffer[ch][f] *= float(suppress);
						}
						else
						{
						}
						NoiseFloor *= NoiseFloorDelta;
					}
				}
				// perform inverse transform
				FastInverseFourierTransform(m_FftOutBuffer[ch],
											m_FftInBuffer[ch], m_nFftOrder);
				// add the processed data back to the output buffer
				const int nSampleOffset = nBacklogIndex + 1 - m_nFftOrder / 2;
				for (f = 0; f < m_nFftOrder / 2; f++)
				{
					m_BackBuffer[(nSampleOffset + f) & PREV_MASK][ch] =
						m_Window[f] *
						m_FftInBuffer[ch][f] + m_AccumBuffer[f][ch];

					m_AccumBuffer[f][ch] =
						m_Window[f + m_nFftOrder / 2] *
						m_FftInBuffer[ch][f + m_nFftOrder / 2];
				}
			}
		}

		if (nBacklogIndex >= ANALYZE_LAG + m_nFftOrder / 2)
		{
			for (ch = 0; ch < nChans; ch++)
			{
				// output is delayed by ANALYZE_LAG samples
				pOutBuf[0] = DoubleToShort(m_BackBuffer[(m_nStoredSamples + ANALYZE_LAG) & PREV_MASK][ch]);
				pOutBuf++;
			}
			m_nStoredSamples++;
			nStoredSamples++;
		}
		nBacklogIndex++;
	}

	m_nBackSampleCount += nSamples;
	*pUsedBytes += nSamples * (nChans * sizeof (__int16));
	return nSavedBytes + nChans * sizeof(__int16) * nStoredSamples;
}

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

int CBatchProcessing::ProcessSound(char const * pIn, char * pOut,
									int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;

	if (NULL != pIn)
	{
		// regular processing of input data
		if (0 == m_Stages.GetSize())
		{
			// just pass through
			int ToCopy = __min(nInBytes, nOutBytes);
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
				int nProcessedBytes;
				int nCurrInputBytes;
				int nCurrOutputBytes;
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
				int nProcessedBytes;
				int nCurrInputBytes;
				int nCurrOutputBytes;
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

CResampleFilter::~CResampleFilter()
{
}

BOOL CResampleFilter::InitResample(double ResampleRatio, double FilterLength, int nChannels)
{
	// FrequencyRatio is out freq/ input freq. If >1, it is upsampling,
	// if < 1 it is downsampling
	// FilterLength is how many Sin periods are in the array
	double PrevVal = 0.;
	int i;
	for (i = 0; i < ResampleFilterSize; i++)
	{
		double arg = M_PI * FilterLength / ResampleFilterSize * (i + 1 - ResampleFilterSize / 2);
		double Window = sin(M_PI * (i +1) / ResampleFilterSize);
		Window *= Window;   // squared sin window

		double arg1 = M_PI * FilterLength / ResampleFilterSize * (i + 0.5 - ResampleFilterSize / 2);
		double Window1 = sin(M_PI * (i +0.5) / ResampleFilterSize);
		Window1 *= Window1;   // squared sin window
		double val;
		if (arg != 0)
		{
			val = Window * sin(arg) / arg;
		}
		else
		{
			val = Window;   // window must be 1.
		}
		double val1 = Window1 * sin(arg1) / arg1;
		// val *= FilterScale;
		m_FilterBuf[i] = float(PrevVal);
		//TRACE("Resample filter[%03d]=%f\n", i, m_FilterBuf[i]);
		double dif1 = val1 - PrevVal;
		double dif2 = val - val1;
		double sqrdif = (dif2 - dif1) * 4. / 3.;
		m_FilterDifBuf[i] = float(val - PrevVal - sqrdif) / (1 << ResampleIndexShift);
		m_FilterDif2Buf[i] = float(sqrdif / (1 << ResampleIndexShift) / (1 << ResampleIndexShift));
		PrevVal = val;
	}
	if (ResampleRatio >= 1.)
	{
		// upsampling.
		//
		double InputPeriod = 0x100000000i64 / (FilterLength * (1. + 1. / FilterLength));
		m_InputPeriod = unsigned __int32(InputPeriod);
		m_OutputPeriod = unsigned __int32(InputPeriod / ResampleRatio);
	}
	else
	{
		// downsampling
		double OutputPeriod = 0x100000000i64 / (FilterLength * (1. + 1. / FilterLength));
		m_OutputPeriod = unsigned __int32(OutputPeriod);
		m_InputPeriod = unsigned __int32(OutputPeriod * ResampleRatio);
	}
	//TRACE("InputPeriod=%08x, OutputPeriod=%08x\n", m_InputPeriod, m_OutputPeriod);

	m_SrcFilterLength = int(0x100000000i64 / m_InputPeriod);
	m_Phase = 0x80000000u % m_InputPeriod;
	m_ResampleRatio = ResampleRatio;

	m_SrcBufUsed = 0;
	m_DstBufUsed = 0;
	m_DstBufSaved = 0;
	m_InputChannels = nChannels;
	// compute normalization coefficient
	// apply DC to the filter
	for (i = 0; i < SrcBufSize; i++)
	{
		m_pSrcBuf[i] = 1.;
	}
	m_SrcBufFilled = SrcBufSize;
	FilterSoundResample();
	float Max = 0;
	for (i = 2; i < m_DstBufUsed; i++)
	{
		if (Max < m_pDstBuf[i])
		{
			Max = m_pDstBuf[i];
		}
	}
	for (i = 0; i < ResampleFilterSize; i++)
	{
		m_FilterBuf[i] /= Max;
		m_FilterDifBuf[i] /= Max;
		m_FilterDif2Buf[i] /= Max;
	}

	m_SrcBufUsed = 0;
	m_DstBufUsed = 0;
	// prefill at 1/2 filter length
	memset(m_pSrcBuf, 0, SrcBufSize * sizeof * m_pSrcBuf);
	m_SrcBufFilled = (0x80000000u / m_InputPeriod) * m_InputChannels;
	m_Phase = 0x80000000u % m_InputPeriod;

	m_TotalProcessedSamples = 0;
	m_TotalSavedSamples = 0;

	return TRUE;
}

void CResampleFilter::FilterSoundResample()
{
	int SrcSamples = m_SrcBufFilled - m_SrcBufUsed - m_InputChannels * m_SrcFilterLength;
	const float * src = m_pSrcBuf + m_SrcBufUsed;

	if (SrcSamples <= 0)
	{
		return;
	}

	for (int i = m_DstBufUsed; i < DstBufSize; i+= m_InputChannels)
	{
		for (int ch = 0; ch < m_InputChannels; ch++)
		{
			unsigned __int32 Phase1 = m_Phase;
			double OutSample = 0.;
			for (int j = 0; ; j+= m_InputChannels)
			{
				int TableIndex = Phase1 >> ResampleIndexShift;
				double PhaseFraction = int(Phase1 & ~(0xFFFFFFFF << ResampleIndexShift));
				ASSERT(src + j + ch < m_pSrcBuf + SrcBufSize);
				OutSample += src[j+ch] * (m_FilterBuf[TableIndex] +
										PhaseFraction * (m_FilterDifBuf[TableIndex]
											+ PhaseFraction * m_FilterDif2Buf[TableIndex]));
				unsigned __int32 Phase2 = Phase1 + m_InputPeriod;
				if (Phase2 < Phase1)
				{
					break;
				}
				Phase1 = Phase2;
			}
			m_pDstBuf[i+ch] = float(OutSample);
		}
		m_DstBufUsed += m_InputChannels;
		m_Phase -= m_OutputPeriod;
		while (m_Phase & 0x80000000)
		{
			src += m_InputChannels;
			m_Phase += m_InputPeriod;
			SrcSamples -= m_InputChannels;
			m_SrcBufUsed += m_InputChannels;
		}
		if (SrcSamples < m_InputChannels)
		{
			return;
		}
	}
}

int CResampleFilter::ProcessSoundBuffer(char const * pIn, char * pOut,
										int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;

	int nInSamples = nInBytes / sizeof (__int16);
	int nOutSamples = nOutBytes / sizeof (__int16);
	__int16 const * pInBuf = (__int16 *) pIn;
	__int16 * pOutBuf = (__int16 *) pOut;

	int nUsedSamples = 0;
	int nSavedSamples = 0;
	if (0 == pInBuf)
	{
		// adjust nOutSamples
		LONGLONG MaxOutSamples = LONGLONG(m_ResampleRatio * m_TotalProcessedSamples);
		if (2 == m_InputChannels)
		{
			MaxOutSamples &= ~1;
		}
		MaxOutSamples -= m_TotalSavedSamples;
		if (MaxOutSamples <= 0)
		{
			MaxOutSamples = 0;
		}
		if (nOutSamples > MaxOutSamples)
		{
			nOutSamples = int(MaxOutSamples);
		}
	}

	while(nOutSamples > 0)
	{
		// move data in the internal buffer, if necessary
		if ((m_SrcBufFilled - m_SrcBufUsed) / m_InputChannels
			<= m_SrcFilterLength * 2)
		{
			for (int i = 0, j = m_SrcBufUsed; j < m_SrcBufFilled; i++, j++)
			{
				m_pSrcBuf[i] = m_pSrcBuf[j];
			}
			m_SrcBufUsed = 0;
			m_SrcBufFilled = i;
		}
		if (m_SrcBufFilled < SrcBufSize)
		{

			int ToCopy = __min(SrcBufSize - m_SrcBufFilled, nInSamples);
			if (0 == pInBuf)
			{
				// fill the rest of the input buffer with zeros
				for (int j = m_SrcBufFilled; j < SrcBufSize; j++)
				{
					m_pSrcBuf[j] = 0.;
				}
				m_SrcBufFilled = j;
			}
			else
			{
				for (int i = 0, j = m_SrcBufFilled; i < ToCopy; i++, j++)
				{
					m_pSrcBuf[j] = pInBuf[i];
				}
				m_SrcBufFilled = j;
				pInBuf += i;
				nInSamples -= i;
				nUsedSamples += i;
				m_TotalProcessedSamples += i;
			}
		}

		FilterSoundResample();
		if (m_DstBufSaved == m_DstBufUsed)
		{
			m_DstBufSaved = 0;
			m_DstBufUsed = 0;
			break;
		}

		int ToCopy = __min(m_DstBufUsed - m_DstBufSaved, nOutSamples);
		for (int i = 0, j = m_DstBufSaved; i < ToCopy; i++, j++)
		{
			pOutBuf[i] = DoubleToShort(m_pDstBuf[j]);
		}
		m_DstBufSaved = j;
		pOutBuf += i;
		nOutSamples -= i;
		nSavedSamples += i;
		m_TotalSavedSamples += i;
		if (m_DstBufSaved == m_DstBufUsed)
		{
			m_DstBufSaved = 0;
			m_DstBufUsed = 0;
		}
	}

	* pUsedBytes += nUsedSamples * sizeof(__int16);
	return nSavedBytes + nSavedSamples * sizeof(__int16);
}

CAudioConvertor::CAudioConvertor()
	:m_SrcBufSize(0),
	m_DstBufSize(0),
	m_SrcFormat(NULL),
	m_DstFormat(NULL),
	m_acmStr(NULL)
{
	memzero(m_ash);
}

CAudioConvertor::~CAudioConvertor()
{
	if (NULL != m_acmStr)
	{
		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
	}
	delete[] m_ash.pbSrc;
	delete[] m_ash.pbDst;
	delete[] (char*) m_SrcFormat;
	delete[] (char*) m_DstFormat;

}

BOOL CAudioConvertor::InitConversion(WAVEFORMATEX * SrcFormat, WAVEFORMATEX * DstFormat,
									HACMDRIVER had)
{
	if (WAVE_FORMAT_PCM == SrcFormat->wFormatTag)
	{
		if (SrcFormat->nChannels != DstFormat->nChannels
			|| SrcFormat->nSamplesPerSec != DstFormat->nSamplesPerSec)
		{
			// unable to convert
			return FALSE;
		}

		m_ash.cbDstLength = 0;
		m_ash.cbSrcLength = 0x10000;  // 64K
		if (0 != acmStreamOpen( & m_acmStr, had, SrcFormat, DstFormat, NULL, NULL, NULL,
								ACM_STREAMOPENF_NONREALTIME)
			|| 0 != acmStreamSize(m_acmStr, m_ash.cbSrcLength, & m_ash.cbDstLength,
								ACM_STREAMSIZEF_SOURCE)
			|| 0 != acmStreamSize(m_acmStr, m_ash.cbDstLength, & m_ash.cbSrcLength,
								ACM_STREAMSIZEF_DESTINATION))
		{
			if (m_acmStr != NULL)
			{
				acmStreamClose(m_acmStr, 0);
				m_acmStr = NULL;
			}
			return FALSE;
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
		acmFormatSuggest(had, SrcFormat, & wf, sizeof wf,
						ACM_FORMATSUGGESTF_NCHANNELS
						| ACM_FORMATSUGGESTF_WBITSPERSAMPLE
						| ACM_FORMATSUGGESTF_WFORMATTAG);
		TRACE("acmFormatSuggest:nSamplesPerSec=%d, BytesPerSec=%d, nBlockAlign=%d\n",
			wf.nSamplesPerSec, wf.nAvgBytesPerSec, wf.nBlockAlign);
		if (wf.nChannels != DstFormat->nChannels
			|| wf.nSamplesPerSec != DstFormat->nSamplesPerSec)
		{
			// unable to convert
			return FALSE;
		}

		m_ash.cbSrcLength = 0;
		m_ash.cbDstLength = 0x10000;  // 64K
		if (0 != acmStreamOpen( & m_acmStr, had, SrcFormat, & wf, NULL, NULL, NULL,
								ACM_STREAMOPENF_NONREALTIME)
			|| 0 != acmStreamSize(m_acmStr, m_ash.cbDstLength, & m_ash.cbSrcLength,
								ACM_STREAMSIZEF_DESTINATION)
			|| 0 != acmStreamSize(m_acmStr, m_ash.cbSrcLength, & m_ash.cbDstLength,
								ACM_STREAMSIZEF_SOURCE))
		{
			if (m_acmStr != NULL)
			{
				acmStreamClose(m_acmStr, 0);
				m_acmStr = NULL;
			}
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	// allocate buffers
	m_SrcBufSize = m_ash.cbSrcLength;
	m_DstBufSize = m_ash.cbDstLength;
	m_ash.pbSrc = new BYTE[m_SrcBufSize];
	m_ash.pbDst = new BYTE[m_DstBufSize];

	m_ash.cbStruct = sizeof m_ash;
	if (0 == m_SrcBufSize
		|| 0 == m_DstBufSize
		|| NULL == m_ash.pbSrc
		|| NULL == m_ash.pbDst
		// prepare the buffer
		|| 0 != acmStreamPrepareHeader(m_acmStr, & m_ash, 0))
	{
		delete m_ash.pbSrc;
		m_ash.pbSrc = NULL;
		delete m_ash.pbDst;
		m_ash.pbDst = NULL;
		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
		return FALSE;
	}
	m_ConvertFlags = ACM_STREAMCONVERTF_START;
	m_DstSaved = 0;
	m_ash.cbSrcLength = 0;  // buffer is empty
	m_ash.cbDstLengthUsed = 0;  // buffer is empty
	return TRUE;
}

int CAudioConvertor::ProcessSound(char const * pIn, char * pOut,
								int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;
	while (1)
	{
		// empty the output buffer
		if (m_ash.cbDstLengthUsed > m_DstSaved)
		{
			int ToCopy = m_ash.cbDstLengthUsed - m_DstSaved;
			if (nOutBytes < ToCopy)
			{
				ToCopy = nOutBytes;
			}
			memmove(pOut, m_ash.pbDst + m_DstSaved, ToCopy);
			pOut += ToCopy;
			nSavedBytes += ToCopy;
			nOutBytes -= ToCopy;
			m_DstSaved += ToCopy;
		}
		// fill the source buffer
		if (m_ash.cbSrcLength < m_SrcBufSize && NULL != pIn)
		{
			int ToCopy = m_SrcBufSize - m_ash.cbSrcLength;
			if (ToCopy > nInBytes)
			{
				ToCopy = nInBytes;
			}
			memmove(m_ash.pbSrc + m_ash.cbSrcLength, pIn, ToCopy);
			m_ash.cbSrcLength += ToCopy;
			pIn += ToCopy;
			nInBytes -= ToCopy;
			* pUsedBytes += ToCopy;
			if (m_ash.cbSrcLength < m_SrcBufSize)
			{
				break;  // still not enough data
			}
		}
		if (m_DstSaved < m_ash.cbDstLengthUsed)
		{
			break;  // not enough space
		}
		m_DstSaved = 0;
		m_ash.cbDstLengthUsed = 0;
		m_ash.cbDstLength = m_DstBufSize;
		if (NULL == pIn)
		{
			m_ConvertFlags |= ACM_STREAMCONVERTF_END;
			m_ConvertFlags &= ~ACM_STREAMCONVERTF_BLOCKALIGN;
		}
		// do the conversion
		m_ash.cbSrcLengthUsed = 0;

		if (0 == acmStreamConvert(m_acmStr, & m_ash, m_ConvertFlags))
		{
			if (0 == m_ash.cbSrcLengthUsed
				&& 0 == m_ash.cbDstLengthUsed)
			{
				break;
			}
			// move data in the source buffer
			if (m_ash.cbSrcLengthUsed != 0)
			{
				if (m_ash.cbSrcLength > m_ash.cbSrcLengthUsed)
				{
					memmove(m_ash.pbSrc, m_ash.pbSrc + m_ash.cbSrcLengthUsed,
							m_ash.cbSrcLength - m_ash.cbSrcLengthUsed);
				}
				m_ash.cbSrcLength -= m_ash.cbSrcLengthUsed;
				m_ash.cbSrcLengthUsed = 0;
			}
		}
		else
		{
			// error
			return -1;
		}
		m_ConvertFlags &= ~ACM_STREAMCONVERTF_START;
	}
	return nSavedBytes;
}

int CChannelConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
										int nInBytes, int nOutBytes, int * pUsedBytes)
{
	int nSavedBytes = 0;
	*pUsedBytes = 0;

	int nInSamples = nInBytes / (m_InputChannels * sizeof (__int16));
	int nOutSamples = nOutBytes / (m_OutputChannels * sizeof (__int16));
	__int16 const * pInBuf = (__int16 *) pIn;
	__int16 * pOutBuf = (__int16 *) pOut;

	int nSamples = __min(nInSamples, nOutSamples);
	int i;
	if (2 == m_InputChannels
		&& 1 == m_OutputChannels)
	{
		if (0 == m_ChannelsToProcess
			|| 1 == m_ChannelsToProcess)
		{
			pInBuf += m_ChannelsToProcess;
			for (i = 0; i < nSamples; i++,
				pInBuf += 2, pOutBuf += 1)
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
		nSavedBytes += i * sizeof(__int16);
		*pUsedBytes += 2 * i * sizeof(__int16);
	}
	else if (1 == m_InputChannels
			&& 2 == m_OutputChannels)
	{
		for (i = 0; i < nSamples; i++,
			pInBuf += 1, pOutBuf += 2)
		{
			pOutBuf[0] = * pInBuf;
			pOutBuf[1] = * pInBuf;
		}
		nSavedBytes += 2 * i * sizeof(__int16);
		*pUsedBytes += i * sizeof(__int16);
	}
	else
	{
		return -1;
	}
	return nSavedBytes;
}

BladeMp3Encoder::BladeMp3Encoder()
	: m_pStream(NULL),
	m_DllModule(NULL),
	m_InBufferSize(0),
	m_OutBufferSize(0),
	m_bFlushStreamCalled(FALSE),
	beInitStream(InitStreamStub),
	beEncodeChunk(EncodeChunkStub),
	beDeinitStream(DeinitStreamStub),
	beCloseStream(CloseStreamStub),
	beGetVersion(GetVersionStub),
	beWriteVBRHeader(WriteVBRHeaderStub)
{
}

void BladeMp3Encoder::Close()
{
	if (m_pStream)
	{
		CloseStream();
	}
	beInitStream = InitStreamStub;
	beEncodeChunk = EncodeChunkStub;
	beDeinitStream = DeinitStreamStub;
	beCloseStream = CloseStreamStub;
	beGetVersion = GetVersionStub;
	beWriteVBRHeader = WriteVBRHeaderStub;
	if (m_DllModule)
	{
		FreeLibrary(m_DllModule);
		m_DllModule = NULL;
	}
}
BOOL BladeMp3Encoder::OpenStream(PBE_CONFIG pConfig)
{
	DWORD dwSamples;
	if (NULL != m_pStream)
	{
		return FALSE;
	}
	if (BE_ERR_SUCCESSFUL != beInitStream(pConfig, & dwSamples, & m_OutBufferSize, & m_pStream))
	{
		return FALSE;
	}
	m_bFlushStreamCalled = false;

	m_InBufferSize = dwSamples * 2;
	if (pConfig->format.LHV1.nMode != BE_MP3_MODE_MONO)
	{
		m_InBufferSize *= 2;
	}
	return TRUE;
}

BOOL BladeMp3Encoder::Open(LPCTSTR DllName)
{
	if (m_DllModule != NULL)
	{
		// cannot open twice
		return FALSE;
	}
	m_DllModule = LoadLibrary(DllName);
	if (NULL == m_DllModule)
	{
		return FALSE;
	}

	beInitStream = (BEINITSTREAM)GetProcAddress(m_DllModule, TEXT_BEINITSTREAM);
	beEncodeChunk = (BEENCODECHUNK)GetProcAddress(m_DllModule, TEXT_BEENCODECHUNK);
	beDeinitStream = (BEDEINITSTREAM)GetProcAddress(m_DllModule, TEXT_BEDEINITSTREAM);
	beCloseStream = (BECLOSESTREAM)GetProcAddress(m_DllModule, TEXT_BECLOSESTREAM);
	beGetVersion = (BEVERSION)GetProcAddress(m_DllModule, TEXT_BEVERSION);
	beWriteVBRHeader = (BEWRITEVBRHEADER)GetProcAddress(m_DllModule, TEXT_BEWRITEVBRHEADER);

	if (NULL == beInitStream
		|| NULL == beEncodeChunk
		|| NULL == beDeinitStream
		|| NULL == beCloseStream
		|| NULL == beGetVersion
		|| NULL == beWriteVBRHeader)
	{
		Close();
		return FALSE;
	}
	return TRUE;
}

BOOL BladeMp3Encoder::EncodeChunk(short const * pSrc, int nSamples, BYTE * pDst, DWORD * pBytesEncoded)
{
	return BE_ERR_SUCCESSFUL == beEncodeChunk(m_pStream, nSamples, pSrc, pDst, pBytesEncoded);
}

BOOL BladeMp3Encoder::FlushStream(BYTE * pDst, DWORD * pBytesEncoded)
{
	if (m_bFlushStreamCalled)
	{
		*pBytesEncoded = 0;
		return TRUE;
	}
	m_bFlushStreamCalled = TRUE;
	return BE_ERR_SUCCESSFUL == beDeinitStream(m_pStream, pDst, pBytesEncoded);
}

CString BladeMp3Encoder::GetVersionString()
{
	BE_VERSION ver;
	memzero(ver);
	GetVersion( & ver);

	SYSTEMTIME time;
	memzero(time);
	time.wYear = ver.wYear;
	time.wDay = ver.byDay;
	time.wMonth = ver.byMonth;

	int const TimeBufSize = 256;
	TCHAR str[TimeBufSize] = {0};

	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, & time, NULL, str, TimeBufSize - 1);

	CString s;
	s.Format("LameEnc DLL Version %d.%02d, (%s) Engine %d.%02d",
			ver.byDLLMajorVersion, ver.byDLLMinorVersion,
			str, ver.byMajorVersion, ver.byMinorVersion);
	return s;
}

CLameEncConvertor::~CLameEncConvertor()
{
	delete[] m_pInputBuffer;
	m_pInputBuffer = NULL;
	delete[] m_pOutputBuffer;
	m_pOutputBuffer = NULL;
}

BOOL CLameEncConvertor::Open(WAVEFORMATEX * pWF)
{
	BE_CONFIG cfg;
	memzero(cfg);

	cfg.dwConfig = BE_CONFIG_LAME;
	cfg.format.LHV1.dwStructVersion = 1;
	cfg.format.LHV1.dwStructSize = sizeof cfg;
	cfg.format.LHV1.dwSampleRate = pWF->nSamplesPerSec;
	cfg.format.LHV1.nMode = BE_MP3_MODE_MONO;
	if (pWF->nChannels > 1)
	{
		cfg.format.LHV1.nMode = BE_MP3_MODE_STEREO;
	}
	cfg.format.LHV1.dwBitrate = pWF->nAvgBytesPerSec / (1000 / 8);

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
	m_pOutputBuffer = new char[m_OutputBufferSize];

	return TRUE;
}

int CLameEncConvertor::ProcessSound(char const * pInBuf, char * pOutBuf,
									int nInBytes, int nOutBytes, int * pUsedBytes)
{
	// save extra data from the output buffer
	*pUsedBytes = 0;
	int nSavedBytes = 0;
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
			int ToCopy = __min(nOutBytes, m_OutputBufferFilled);
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
		int ToCopy = m_InputBufferSize - m_InputBufferFilled;
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
							m_InputBufferFilled / (sizeof (__int16) * m_InputChannels),
							(BYTE*)m_pOutputBuffer, & OutFilled);
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
			m_Enc.FlushStream((BYTE*)m_pOutputBuffer, & OutFilled);
			m_OutputBufferFilled = OutFilled;
			FlushBuffer = FALSE;
		}
	}
	return nSavedBytes;
}

int CByteSwapConvertor::ProcessSoundBuffer(char const * pIn, char * pOut,
											int nInBytes, int nOutBytes, int * pUsedBytes)
{
	if (NULL == pIn)
	{
		*pUsedBytes = 0;
		return 0;
	}
	int nBytes = __min(nInBytes, nOutBytes);
	for (int i = 0; i < nBytes; i+= sizeof (__int16))
	{
		pOut[i + 1] = pIn[i];
		pOut[i] = pIn[i + 1];
	}
	*pUsedBytes = nBytes;
	return nBytes;
}
