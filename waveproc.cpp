// waveproc.cpp
#include <math.h>
#pragma intrinsic(sin, cos, exp, log, atan2)

#include <afxwin.h>
#include <mmsystem.h>
#include "complex.h"
#include "waveproc.h"

#ifndef _CONSOLE
#define puts(t) AfxMessageBox(_T(t), MB_OK | MB_ICONEXCLAMATION)

#endif

#include <FFT.h>
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
	m_PrevDeriv[0] = 0.;
	m_PrevDeriv[1] = 0.;
	m_PrevDeriv2[0] = 0.;
	m_PrevDeriv2[1] = 0.;
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
			m_Window[i] = sin((i + 0.5) * M_PI / nFftOrder);
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
	m_PowerScale = 1. / nFftOrder;
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
				m_FarMaskingCoeffs[i][k] = MaskingFactor * 5;
			}
			else
			{
				int n = k - i;
				if (n < 0) n = -n;
				float x = 1. / (n + 1);
				m_FarMaskingCoeffs[i][k] = MaskingFactor * x;
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
	return pWf->wFormatTag == WAVE_FORMAT_PCM
			&& pWf->wBitsPerSample == 16
			&& pWf->nBlockAlign == pWf->nChannels * 2 //pWf->wBitsPerSample / 8
			&& pWf->nAvgBytesPerSec == pWf->nSamplesPerSec * pWf->nBlockAlign;
}
BOOL CHumRemoval::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	return CWaveProc::SetAndValidateWaveformat(pWf)
			&& pWf->nChannels == 2
			&& pWf->nSamplesPerSec == 44100;
}

BOOL CClickRemoval::SetAndValidateWaveformat(WAVEFORMATEX const * pWf)
{
	return CWaveProc::SetAndValidateWaveformat(pWf)
			&& pWf->nSamplesPerSec == 44100;
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
		if (FALSE == m_Stages[i]->SetAndValidateWaveformat(pWf))
			return FALSE;
	}
	return TRUE;
}

int CHumRemoval::ProcessSound(__int16 const * pInBuf, __int16 * pOutBuf,
							int nSamples, int nChans)
{
	ASSERT(nChans == 2);
	if (nChans != 2)
	{
		return -1;
	}

	if (NULL == pInBuf)
	{
		return 0;   // no delayed samples in history
	}

	// process the data
	for (int i = 0; i < nSamples * 2; i += 2)
	{
		float curr_l;
		float curr_r;
		if (m_ApplyHighpassFilter)
		{
			// apply additional highpass 2nd order filter to both channels
			// with cutoff frequency 50 Hz
			// (1-2z + z^2)/(a^2 -2az + z^2)
			double outL = (float(pInBuf[i] - m_PrevHpfL[0] - m_PrevHpfL[0] + m_PrevHpfL[1])
							+ 1.99636 * m_PrevHpOutL[0] - 0.996363312 * m_PrevHpOutL[1]);
			m_PrevHpOutL[1] = m_PrevHpOutL[0];
			m_PrevHpOutL[0] = outL;
			curr_l = float(outL);
			double outR = (float(pInBuf[i+1] - m_PrevHpfR[0] - m_PrevHpfR[0] + m_PrevHpfR[1])
							+ 1.99636 * m_PrevHpOutR[0] - 0.996363312 * m_PrevHpOutR[1]);
			m_PrevHpOutR[1] = m_PrevHpOutR[0];
			m_PrevHpOutR[0] = outR;
			curr_r = float(outR);

			m_PrevHpfL[1] = m_PrevHpfL[0];
			m_PrevHpfL[0] = pInBuf[i];
			m_PrevHpfR[1] = m_PrevHpfR[0];
			m_PrevHpfR[0] = pInBuf[i + 1];
		}
		else
		{
			curr_l = pInBuf[i];
			curr_r = pInBuf[i + 1];
		}
		m_prev_outl =
			(curr_l + m_prev_inl) * 0.003575f + 0.9857f * m_prev_outl;
		m_prev_outr =
			(curr_r + m_prev_inr) * 0.003575f + 0.9857f * m_prev_outr;
		float hpf_l = m_prev_inl - m_prev_outl;
		float hpf_r = m_prev_inr - m_prev_outr;
		m_prev_inl = curr_l;
		m_prev_inr = curr_r;
		long out_l = (long) floor(hpf_l + m_prev_outr + 0.5);
		if (out_l < -32768)
		{
			out_l = -32768;
		}
		if (out_l > 32767)
		{
			out_l = 32767;
		}
		pOutBuf[i] = __int16 (out_l);
		long out_r = (long) floor(hpf_r + m_prev_outl + 0.5);
		if (out_r < -32768)
		{
			out_r = -32768;
		}
		if (out_r > 32767)
		{
			out_r = 32767;
		}
		pOutBuf[i + 1] = __int16(out_r);
	}
	return nSamples;
}

void InterpolateBigGap(CBackBuffer<int, int> & data, int nLeftIndex, int ClickLength)
{
	float x[1024];
	if (ClickLength > 256)
	{
		return;
	}
	// FFT order is >=64 and >= ClickLength * 4
	// take 2 FFT in [nLeftIndex-FftOrder... nLeftIndex-1] range
	// and [nLeftIndex-FftOrder-ClickLength...nLeftIndex-ClickLength-1] offset
	// Find next FFT estimation as FFT2*(FFT2/FFT1/abs(FFT2/FFT1))
	// Then take 2 FFT in [nLeftIndex+ClickLength...nLeftIndex+ClickLength+FftOrder-1]
	// and [nLeftIndex+2*ClickLength...nLeftIndex+2*ClickLength+FftOrder-1]
	// and find another FFT estimation. Perform backward FFT and combine source and
	// FFT results using squared-sine window
	complex<float> y1[1024];
	complex<float> y2[1024];
	float xl[512];  // to save extrapolation from the left neighborhood

	int FftOrder = 64;
	while (FftOrder < ClickLength * 4)
	{
		FftOrder +=FftOrder;
	}
	int Offset = FftOrder/4;
	int i;
	for (i = 0; i < FftOrder; i++)
	{
		y2[i] = data[nLeftIndex +ClickLength-Offset- FftOrder + i];
	}
	FastFourierTransform(y2, y2, FftOrder);
	for (i = 0; i < FftOrder; i++)
	{
		y1[i] = data[nLeftIndex +ClickLength- Offset*2 - FftOrder + i];
	}
	FastFourierTransform(y1, y1, FftOrder);
	// calculate another set of coefficients
	// leave only those frequencies with up to ClickLength/10 period
	int nMaxFreq = FftOrder /2 /* * 5 / ClickLength */;
	if (nMaxFreq > FftOrder) nMaxFreq = FftOrder;
	for (i = 1; i < nMaxFreq; i++)
	{
		if (y1[i] != complex<float>(0., 0.))
		{
#if 1
			complex<float> rot = y2[i] / y1[i];
			double a = abs(rot);
			rot /= a;
			y2[i] = y2[i] * rot;

			rot = y2[FftOrder-1-i] / y1[FftOrder-1-i];
			a = abs(rot);
			rot /= a;
			y2[FftOrder-1-i] = y2[FftOrder-1-i] * rot;
#else
			y2[FftOrder-1-i] *= y2[FftOrder-1-i] / y1[FftOrder-1-i];
#endif
		}
	}
	// extrapolate DC
	y2[0] += y2[0] - y1[0];
	// zero all higher frequencies
	for ( ; i <= FftOrder/2; i++)
	{
		y2[i] = complex<float>(0., 0.);
	}
	FastInverseFourierTransform(y2, y1, FftOrder);
	// save the result
	for (i = 0; i < ClickLength * 2; i++)
	{
		xl[i] = y2[FftOrder-ClickLength * 2+i].real();
	}

	// do calculations for the right side neighborhood
	for (i = 0; i < FftOrder; i++)
	{
		x[i] = data[nLeftIndex + Offset + i];
	}
	FastFourierTransform(x, y2, FftOrder);
	for (i = 0; i < FftOrder; i++)
	{
		x[i] = data[nLeftIndex + Offset*2 + i];
	}
	FastFourierTransform(x, y1, FftOrder);
	// calculate another set of coefficients
	// leave only those frequencies with up to ClickLength/10 period

	for (i = 1; i <= nMaxFreq; i++)
	{
		if (y1[i] != complex<float>(0., 0.))
		{
			complex<float> rot = y2[i] / y1[i];
			rot /= abs(rot);
			y2[i] = y2[i] * rot;
		}
	}
	// extrapolate DC
	y2[0] += y2[0] - y1[0];
	// zero all higher frequencies
	for ( ; i <= FftOrder/2; i++)
	{
		y2[i] = complex<float>(0., 0.);
	}
	FastInverseFourierTransform(y2, x, FftOrder);

	// the result is in x[]

#if 0
// combine the source and interpolations, using squared sine window
	for (i = 0; i < ClickLength; i++)
	{
		double W = 0.5 + 0.5 * cos(M_PI / ClickLength * (i + 0.5));
		// click area:
		data[nLeftIndex + i] = int(xl[i + ClickLength] * W + x[i] * (1. - W));
		// Left neighborhood:
		data[nLeftIndex - ClickLength + i] =
			int(data[nLeftIndex - ClickLength + i] * W + xl[i] * (1. - W));
		// Right neighborhood:
		data[nLeftIndex + ClickLength + i] =
			int(data[nLeftIndex + ClickLength + i] * (1. - W) + x[i+ClickLength] * W);
	}
#elif 0
	// For now, just replace the samples to evaluate
	for (i = 0; i < ClickLength * 2; i++)
	{
		data[nLeftIndex+i] =
			int(x[i]);
	}
#else
	// For now, just replace the samples to evaluate
	for (i = 0; i < ClickLength * 2; i++)
	{
		data[nLeftIndex-ClickLength+i] =
			int(xl[i]);
	}
#endif
}

void InterpolateGap(CBackBuffer<int, int> & data, int nLeftIndex, int ClickLength)
{
	//float x[1024];
	// Perform spike interpolation
	// Use interpolating polynom by Lagrange
	// Zero point == nLeftIndex
	// Take 5 points to left with ClickLength/2 step
	// and 5 points to right
	double Y[10], X[10];
	int n;
	for (n = 0; n < 5; n++)
	{
		X[4 - n] = - (ClickLength / 2 * n + 1);
		Y[4 - n] = data[nLeftIndex - (ClickLength / 2 * n + 1)];
		X[n + 5] = ClickLength + ClickLength / 2 * n;
		Y[n + 5] = data[nLeftIndex + ClickLength + ClickLength / 2 * n];
	}
	// perform Lagrange interpolation
	for (n = 0; n < ClickLength; n++)
	{
		double x = n;
		double y = 0;
		for (int k = 0; k < 10; k++)
		{
			double a = Y[k];
			for (int j = 0; j < 10; j++)
			{
				if (j != k)
				{
					a *= (x - X[j]) / (X[k] - X[j]);
				}
			}
			y += a;
		}
		data[nLeftIndex + n] = y;
	}
}

int CClickRemoval::ProcessSound(__int16 const * pInBuf, __int16 * pOutBuf,
								int nSamples, int nChans)
{
	if (NULL == pInBuf)
	{
		// flush backlogged samples
		// nSamples - size of out buffer / size of samples
		int nBackSamples = m_PrevIndex - m_nStoredSamples;
		if (nBackSamples > nSamples)
		{
			nBackSamples = nSamples;
		}
		if (nBackSamples <= 0)
		{
			return 0;
		}
		for (int i = 0; i < nBackSamples; i++)
		{
			for (int ch = 0; ch < nChans; ch++)
			{
				*pOutBuf = m_prev[ch][i-1];
				pOutBuf++;
			}
		}
		m_prev[0].Advance(nBackSamples);
		m_prev[1].Advance(nBackSamples);
		m_nStoredSamples += nBackSamples;
		return nBackSamples;
	}

	int nClickIndex;
	int nStoreIndex = 0;
	int PrevIndex = m_PrevIndex;
	for (int ch = 0; ch < nChans; ch++)
	{
		int FftIn[CLICK_LENGTH];   // additional space for click length search
		//float FftOut[FFT_ORDER / 2];
		PrevIndex = m_PrevIndex;
		nStoreIndex = 0;
		nClickIndex = PredefinedClickCurrentIndex;
		for (int i = 0; i < nSamples * nChans; i += nChans)
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
			int Deriv3Threshold = m_MeanPower[ch] * m_PowerToDeriv3RatioThreshold;

			float power = m_prev3[ch][0];
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
#ifdef _DEBUG
					if (0) if (ch == 0 && 45321070 == PredefinedClicks[nClickIndex].Position)
						{
							__asm int 3
						}
#endif
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
					Deriv3Threshold = m_MinDeriv3Threshold;
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
					deriv3 = fabs(deriv2 - OldDeriv2);
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
					Deriv3Threshold = m_MinClickDeriv3BoundThreshold;
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
				if (ClickLength <= 16)
				{
					InterpolateGap(m_prev[ch], nLeftIndex, ClickLength);
				}
				else
				{
					InterpolateBigGap(m_prev[ch], nLeftIndex, ClickLength);
				}
			}
#if 0
			if ((PrevIndex & (FFT_ORDER / 2 - 1)) == 0)
			{
				m_MeanFftPower[ch] +=
					m_MeanFftPowerDecayRate * (dbFftPower - m_MeanFftPower[ch]);
			}
#endif
			// output is delayed by 64 samples
			if (PrevIndex > ANALYZE_LAG*2-2)
			{
#if 1
				pOutBuf[nStoreIndex * nChans] =
					__int16(m_prev[ch][ 1-ANALYZE_LAG /*m_nStoredSamples + nStoreIndex - PrevIndex + ANALYZE_LAG*/]);
#else
				// store 3rd derivative
				pOutBuf[nStoreIndex * nChans] =
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
	return nStoredSamples;
}

// smp - cource FFT sample
// pNr - points to the parent CNoiseReduction object
// nSample - number of the sample in FFT set
void CNoiseReduction::SIGNAL_PARAMS::AnalyzeFftSample(complex<DATA> smp, CNoiseReduction * pNr, int nSample)
{
	// find momentary frequency
	complex<DATA> cZero(0., 0.);
	double nrm = pNr->m_PowerScale *
				(real(smp) * real(smp) + imag(smp) * imag(smp));
	sp_Power = nrm;
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
			sp_FilteredLevel = log(abs(smp));
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
		sp_FilteredFreqError +=
			pNr->m_FreqErrorDecayRate * (FreqError - sp_FilteredFreqError);
		sp_FreqDev += (sp_FilteredFreqError * sp_FilteredFreqError - sp_FreqDev)
					* pNr->m_FreqDevDecayRate;
#else
		sp_FreqDev += (FreqError * FreqError - sp_FreqDev)
					* pNr->m_FreqDevDecayRate;
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
		sp_FilteredLevelError += pNr->m_LevelErrorDecayRate *
								(LevelError - sp_FilteredLevelError);
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

int CNoiseReduction::ProcessSound(__int16 const * pInBuf, __int16 * pOutBuf,
								int nSamples, int nChans)
{
	const int PREV_MASK = (m_nFftOrder * 2) - 1;
	const unsigned int ANALYZE_LAG = m_nFftOrder;
	int MinFrequencyToProcess = int(m_MinFrequencyToProcess / 44100. * m_nFftOrder);

	if (NULL == pInBuf)
	{
		// flush backlogged samples
		// nSamples - size of out buffer / size of samples
		int nBackSamples = m_nBackSampleCount - m_nStoredSamples;
		if (nBackSamples > nSamples)
		{
			nBackSamples = nSamples;
		}
		if (nBackSamples <= 0)
		{
			return 0;
		}
		for (int i = 0; i < nBackSamples; i++)
		{
			for (int ch = 0; ch < nChans; ch++)
			{
				*pOutBuf = m_BackBuffer[(m_nStoredSamples + ANALYZE_LAG) & PREV_MASK][ch];
				pOutBuf++;
			}
			m_nStoredSamples++;
		}
		return nBackSamples;
	}

	int ch;
	int nBacklogIndex = m_nBackSampleCount;
	int nStoredSamples = 0;
	int n;
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

#if 1
				// null the high frequencies (test)
				if (0) for (n = m_nFftOrder/4; n < m_nFftOrder/2+1; n++)
					{
						m_FftOutBuffer[ch][n]= 0;
					}
#endif
			}

			// process FFT result
			int f;
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
				for (int k = 0; k < m_nFftOrder / (FAR_MASKING_GRANULARITY*2); k++, f++)
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
					m_pParams[ch][f].sp_MaskingPower =
						m_pParams[ch][f].sp_Power * m_NearMaskingCoeff
						+ (1. - m_NearMaskingCoeff) * FarMasking[f * FAR_MASKING_GRANULARITY*2 / m_nFftOrder]
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
			float ToneEmphasis = exp(m_ToneOverNoisePreference);
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
					m_pParams[ch][f].sp_MaskingPower = PrevFilteredPower[ch];
				}
				MaskingSpectralDecayNormLow += MaskingDistanceDelta;
			}
			PrevFilteredPower[0] = 0.;
			PrevFilteredPower[1] = 0.;
			for (f = m_nFftOrder / 2; f >= 0; f--)
			{
				MaskingSpectralDecayNormHigh -= MaskingDistanceDelta;
				double decay = 1. / MaskingSpectralDecayNormHigh;
				for (ch = 0; ch < nChans; ch++)
				{
					PrevFilteredPower[ch] += (m_pParams[ch][f].sp_MaskingPower - PrevFilteredPower[ch])
											* decay;
					m_pParams[ch][f].sp_MaskingPower = PrevFilteredPower[ch];
				}
			}
			float MaskingTemporalDecayNormLow =
				// coeff to filter masking function in time
				//m_NearMaskingDecayTimeLow * 0.001 * m_SamplesPerSec / (m_nFftOrder / 2);
				m_NearMaskingDecayTimeLow * 0.002 * m_SamplesPerSec / m_nFftOrder;
			if (MaskingTemporalDecayNormLow < 1.)
			{
				MaskingTemporalDecayNormLow = 1.;
			}

			float MaskingTemporalDecayNormHigh =
				// coeff to filter masking function in time
				m_NearMaskingDecayTimeHigh * 0.002 * m_SamplesPerSec / m_nFftOrder;
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
						m_pParams[ch][f].sp_PrevMaskingPower += decay *
							(m_pParams[ch][f].sp_MaskingPower - m_pParams[ch][f].sp_PrevMaskingPower);
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
							m_FftOutBuffer[ch][f] *= suppress;
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
				double tmp = m_BackBuffer[(m_nStoredSamples + ANALYZE_LAG) & PREV_MASK][ch] + 0.5;
				pOutBuf[0] = __int16(floor(tmp));
				pOutBuf++;
			}
			m_nStoredSamples++;
			nStoredSamples++;
		}
		nBacklogIndex++;
	}

	m_nBackSampleCount += nSamples;
	return nStoredSamples;
}

int CBatchProcessing::ProcessSound(__int16 const * pInBuf, __int16 * pOutBuf,
									int nSamples, int nChans)
{
	int nTotalOutputSamples = 0;
	if (NULL != pInBuf)
	{
		// regular processing of input data
		while(nSamples)
		{
			int nInputSamples = nSamples;
			if (nInputSamples > int(sizeof m_InternBuf1 / sizeof m_InternBuf1[0] / nChans))
			{
				nInputSamples = int(sizeof m_InternBuf1 / sizeof m_InternBuf1[0] / nChans);
			}
			int nOutputSamples = 0;
			int nCurrInputSamples = nInputSamples;
			for (int i = 0; i < m_Stages.GetSize(); i++)
			{
				__int16 const * inbuf;
				__int16 * outbuf;
				if (i != 0)
				{
					if (i & 1)
					{
						inbuf = m_InternBuf1;
					}
					else
					{
						inbuf = m_InternBuf2;
					}
				}
				else
				{
					inbuf = pInBuf;
				}

				if (i != m_Stages.GetSize() - 1)
				{
					if (i & 1)
					{
						outbuf = m_InternBuf2;
					}
					else
					{
						outbuf = m_InternBuf1;
					}
				}
				else
				{
					outbuf = pOutBuf;
				}

				nOutputSamples = m_Stages[i]->ProcessSound(inbuf, outbuf, nCurrInputSamples, nChans);
				nCurrInputSamples = nOutputSamples;
				if (0 == nOutputSamples)
				{
					break;  // no data went through the last filter
				}
			}
			pInBuf += nInputSamples * nChans;
			nSamples -= nInputSamples;
			pOutBuf += nOutputSamples * nChans;
			nTotalOutputSamples += nOutputSamples;
		}
	}
	else
	{
		// flush the data
		while(nSamples)
		{
			int nInputSamples = nSamples;
			if (nInputSamples > int(sizeof m_InternBuf1 / sizeof m_InternBuf1[0] / nChans))
			{
				nInputSamples = int(sizeof m_InternBuf1 / sizeof m_InternBuf1[0] / nChans);
			}
			int nOutputSamples = 0;
			int nCurrInputSamples = nInputSamples;
			for (int i = 0; i < m_Stages.GetSize(); i++)
			{
				__int16 const * inbuf;
				__int16 * outbuf;
				if (i != 0 && nCurrInputSamples != 0)
				{
					if (i & 1)
					{
						inbuf = m_InternBuf1;
					}
					else
					{
						inbuf = m_InternBuf2;
					}
				}
				else
				{
					nCurrInputSamples = nSamples;
					if (nCurrInputSamples > int(sizeof m_InternBuf1 / sizeof m_InternBuf1[0] / nChans))
					{
						nCurrInputSamples = int(sizeof m_InternBuf1 / sizeof m_InternBuf1[0] / nChans);
					}
					inbuf = NULL;
				}

				if (i != m_Stages.GetSize() - 1)
				{
					if (i & 1)
					{
						outbuf = m_InternBuf2;
					}
					else
					{
						outbuf = m_InternBuf1;
					}
				}
				else
				{
					outbuf = pOutBuf;
				}

				nOutputSamples = m_Stages[i]->ProcessSound(inbuf, outbuf, nCurrInputSamples, nChans);
				nCurrInputSamples = nOutputSamples;
			}
			nSamples -= nOutputSamples;
			pOutBuf += nOutputSamples * nChans;
			nTotalOutputSamples += nOutputSamples;
			if (0 == nOutputSamples)
			{
				break;
			}
		}
	}
	return nTotalOutputSamples;
}

void CBatchProcessing::AddWaveProc(CWaveProc * pProc, int index)
{
	if (-1 == index || index > m_Stages.GetSize())
	{
		index = m_Stages.GetSize();
	}
	m_Stages.InsertAt(index, pProc);
}


char const title[] = "Vinyl mechanical noise suppressor, AleGr Software, 1998";

int ProcessWaveFile(LPCTSTR NameIn, LPCTSTR NameOut, CWaveProc * pProc)
{
	HANDLE hOutFile;
	CString TmpName;
	CString OutName(NameOut);
	hOutFile = CreateFile(NameOut, GENERIC_READ | GENERIC_WRITE, 0, NULL,
						CREATE_NEW,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (INVALID_HANDLE_VALUE == hOutFile)
	{
		if (ERROR_FILE_EXISTS == GetLastError())
		{
			// create temporary file
			TmpName = CString(NameOut) + ".tmp";
			hOutFile = CreateFile(TmpName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
			if (INVALID_HANDLE_VALUE == hOutFile)
			{
				puts("Cannot create temporary output file");
				return 255;
			}
			OutName = TmpName;
		}
		else
		{
			puts("Cannot create output file");
			return 255;
		}
	}

	HANDLE hInFile = CreateFile(NameIn, GENERIC_READ, FILE_SHARE_READ, NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (INVALID_HANDLE_VALUE == hInFile)
	{
		CloseHandle(hOutFile);
		DeleteFile(OutName);
		puts("Cannot open input file");
		return 255;
	}

	BOOL bError = FALSE;

	HMMIO hmmin = NULL;
	HMMIO hmmout = NULL;
	MMIOINFO mmiin, mmiout;
	memset(& mmiin, 0, sizeof mmiin);
	memset(& mmiout, 0, sizeof mmiout);

	mmiin.adwInfo[0] = (DWORD)hInFile;
	mmiin.cchBuffer = 0x20000; // 128K
	hmmin = mmioOpen(NULL, & mmiin, MMIO_ALLOCBUF | MMIO_READ);
	if (NULL == hmmin)
	{
		CloseHandle(hOutFile);
		DeleteFile(OutName);
		puts("Cannot open input file");
		CloseHandle(hInFile);
		return 255;
	}
	mmiout.adwInfo[0] = (DWORD)hOutFile;
	mmiout.cchBuffer = 0x200000; // 2M
	hmmout = mmioOpen(NULL, & mmiout, MMIO_ALLOCBUF | MMIO_WRITE);

	// process the file
	WAVEFORMATEX wfe;
	MMCKINFO rifflist = {FOURCC_RIFF, 0, 0, 0, 0};
	mmioDescend(hmmin, & rifflist, NULL, MMIO_FINDRIFF);
	if (rifflist.fccType != mmioFOURCC('W', 'A', 'V', 'E'))
	{
		puts("The file is not a valid WAV file");
		mmioClose(hmmout, MMIO_FHOPEN);
		mmioClose(hmmin, MMIO_FHOPEN);
		CloseHandle(hOutFile);
		DeleteFile(OutName);
		CloseHandle(hInFile);
		return 255;
	}

	MMCKINFO rifflist_out = rifflist;
	mmioCreateChunk(hmmout, & rifflist_out, MMIO_CREATERIFF);
	// scan all chunks, trying to find fmt chunk
	BOOL bFmtFound = FALSE;
	while(! bError)
	{
		MMCKINFO cki;
		if (MMSYSERR_NOERROR == mmioDescend(hmmin, & cki, & rifflist, 0))
		{
			MMCKINFO cko = cki;
			DWORD flags = 0;
			if (FOURCC_LIST == cko.ckid)
			{
				flags = MMIO_CREATELIST;
			}

			mmioCreateChunk(hmmout, & cko, flags);
			if (mmioFOURCC('f', 'm', 't', ' ') == cki.ckid)
			{
				if (bFmtFound)
				{
					puts("Extra Wave format descriptor in the file");
					bError = TRUE;
					break;
				}
				bFmtFound = TRUE;
				// only 44100*16*2 PCM files can be processed
				if (sizeof (PCMWAVEFORMAT) > cki.cksize)
				{
					puts("Wrong wave format");
					bError = TRUE;
					break;
				}
				if (sizeof(PCMWAVEFORMAT) != mmioRead(hmmin, (HPSTR) & wfe, sizeof (PCMWAVEFORMAT))
					|| FALSE == pProc->SetAndValidateWaveformat( & wfe))
				{
					puts("Unsupported wave format, only 44100 sps stereo 16 bit PCM allowed");
					bError = TRUE;
					break;
				}
				mmioWrite(hmmout, (HPSTR) & wfe, sizeof (PCMWAVEFORMAT));
			}
			else if (mmioFOURCC('d', 'a', 't', 'a') == cki.ckid)
			{
				if (FALSE == bFmtFound)
				{
					// unknown format
					puts("No wave format descriptor found before wave data");
					bError = TRUE;
					break;
				}
				// copy and process audio data
				__int16 InData[2048];
				__int16 OutData[2048];
				int dwBytes = cki.cksize;

				while (! bError && dwBytes != 0)
				{
					if (FALSE == pProc->m_Callback(pProc, WAVEPROC_MSG_PROGRESS,
													cki.cksize, cki.cksize - dwBytes))
					{
						break;
					}
					int dwCurrentBytes = sizeof InData;
					if (dwCurrentBytes > dwBytes)
					{
						dwCurrentBytes = dwBytes;
					}

					if (dwCurrentBytes != mmioRead(hmmin, (char *)InData, dwCurrentBytes))
					{
						puts("Error when reading source file");
						bError = TRUE;
						break;
					}
					int nOutSamples = pProc->ProcessSound(InData, OutData,
														dwCurrentBytes / (wfe.nChannels * sizeof InData[0]), wfe.nChannels);
					if (-1 == nOutSamples)
					{
						break;
					}

					int nBytesToWrite = nOutSamples * (wfe.nChannels * wfe.wBitsPerSample / 8);

					if( nBytesToWrite != mmioWrite(hmmout, (char *)OutData, nBytesToWrite))
					{
						puts("No space on disk for writing the output file");
						bError = TRUE;
						break;
					}
					dwBytes -= dwCurrentBytes;
				}
				// flush the data
				int nOutSamples;
				while (0 != (nOutSamples = pProc->ProcessSound(NULL, OutData,
												sizeof OutData / (wfe.nChannels * sizeof OutData[0]), wfe.nChannels)))
				{
					int nBytesToWrite = nOutSamples * (wfe.nChannels * wfe.wBitsPerSample / 8);
					if( nBytesToWrite != mmioWrite(hmmout, (char *)OutData, nBytesToWrite))
					{
						puts("No space on disk for writing the output file");
						bError = TRUE;
						break;
					}
				}
			}
			else
			{
				// just copy the data
				MMIOINFO mmi;
				int dwBytes = cki.cksize;
				if (FOURCC_LIST == cki.ckid)
				{
					dwBytes -= 4;
				}
				mmioGetInfo(hmmin, & mmi, 0);
				while (dwBytes != 0)
				{
					if (mmi.pchNext == mmi.pchEndRead)
					{
						if(MMSYSERR_NOERROR != mmioAdvance(hmmin, &mmi, MMIO_READ))
						{
							puts("Error when reading source file");
							bError = TRUE;
							break;
						}
					}

					int dwCurrentBytes = mmi.pchEndRead - mmi.pchNext;
					if (dwCurrentBytes > dwBytes)
					{
						dwCurrentBytes = dwBytes;
					}

					if( dwCurrentBytes != mmioWrite(hmmout, mmi.pchNext, dwCurrentBytes))
					{
						puts("No space on disk for writing the output file");
						bError = TRUE;
						break;
					}
					dwBytes -= dwCurrentBytes;
					mmi.pchNext += dwCurrentBytes;
				}
			}
			mmioAscend(hmmout, & cko, 0);
			mmioAscend(hmmin, & cki, 0);
		}
		else
		{
			break;
		}
	}

	mmioAscend(hmmout, &rifflist_out, 0);

	if (MMSYSERR_NOERROR != mmioFlush(hmmout, 0))
	{
	}

	mmioClose(hmmout, MMIO_FHOPEN);
	CloseHandle(hOutFile);
	mmioClose(hmmin, MMIO_FHOPEN);
	CloseHandle(hInFile);
	if (! TmpName.IsEmpty())
	{
		DeleteFile(NameOut);
		MoveFile(TmpName, NameOut);
		//MoveFileEx(TmpName, NameOut, MOVEFILE_REPLACE_EXISTING);
		GetLastError();
	}
	pProc->m_Callback(pProc, WAVEPROC_MSG_FINISHED, 0, 0);
	return 0;
}
#ifdef _CONSOLE
int main (int argc, char ** argv)
{
	puts(title);
	if (argc <= 1)
	{
		puts("WAVEPROC input_file [output_file]");
		return 0;
	}
	LPCTSTR name_in = argv[1];
	LPCTSTR name_out = name_in;
	if (argc >= 3)
	{
		name_out = argv[2];
	}

#if 0 //def _DEBUG
	float source[64];
	float dst[64];
	complex<float> fft[33];
	for (int i = 0; i < 64; i++)
	{
		source[i] = rand();
	}
	FastFourierTransform(source, fft, 64);
	FastInverseFourierTransform(fft, dst, 64);
	for (i = 0; i <64; i++)
	{
		TRACE("src[%d]=%f, dst=%f\n", i, source[i], dst[i]);
	}
#endif

	int TimeStart = GetTickCount();
	//CHumRemoval hr;
	CClickRemoval cr;
	CBatchProcessing bp;
	CNoiseReduction nr;
	bp.AddWaveProc( & cr);
	//bp.AddWaveProc( & hr);
	//ProcessWaveFile(name_in, name_out, & bp);
	ProcessWaveFile(name_in, name_out, & nr);
	printf("File processed in %d seconds\n", int(GetTickCount() - TimeStart) / 1000);
	TRACE("File processed in %d seconds\n", int(GetTickCount() - TimeStart) / 1000);
#ifdef _DEBUG
	TRACE("Total Bands Processed=%d\n"
		"Transient Bands Found=%d\n"
		"Phase Filtered In Bands=%d\n"
		"Stationary Bands Cancelled=%d\n"
		"Noiselike Bands Cancelled=%d\n"
		"Max Level In Band=%f\n"
		"Min Level In Band=%f\n",
		nr.m_TotalBandProcessed,
		nr.m_TransientBandFound,
		nr.m_PhaseFilteredInBands,
		nr.m_StationaryBandCancelled,
		nr.m_NoiselikeBandCancelled,
		nr.m_MaxLevelInBand,
		nr.m_MinLevelInBand,
		0);
#endif
	return 0;
}

#endif
