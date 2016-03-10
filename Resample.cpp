// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// Resample.cpp
#include "stdafx.h"
#include "WaveSoapFrontDoc.h"
#include "resource.h"
#include "Resample.h"
#include "MessageBoxSynch.h"

#define _USE_MATH_DEFINES   // for M_PI definition
#include <math.h>

// The purpose of this class is to process data and, in case of output data clipped, ask if it's OK to proceed
class CResampleProcContext : public CWaveProcContext
{
	typedef CWaveProcContext BaseClass;
	typedef CResampleProcContext ThisClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CResampleProcContext(CWaveSoapFrontDoc * pDoc,
						UINT StatusStringId, UINT OperationNameId,
						CWaveFile & SrcFile, CWaveFile & DstFile,
						long NewSampleRate, BOOL KeepSamplesPerSec);

	virtual void DeInit();
};

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

CResampleContext::CResampleContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									CWaveFile & SrcFile, CWaveFile &DstFile,
									long NewSampleRate, BOOL KeepSamplesPerSec)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
	long InputSampleRate = SrcFile.SampleRate();

	long common = GreatestCommonFactor(NewSampleRate, InputSampleRate);
	long Pre_DeDecimateRate = NewSampleRate / common;
	long PostDecimateRate = InputSampleRate / common;
	if (Pre_DeDecimateRate <= 6 && PostDecimateRate <= 6)
	{
		// if doing downsample:
		// find out the fraction factors and see if we can go without fractional filter
		Pre_DeDecimateRate = (NewSampleRate + InputSampleRate - 1) / InputSampleRate;
		while (InputSampleRate * Pre_DeDecimateRate * 2 < NewSampleRate * 3)
		{
			Pre_DeDecimateRate += 1;
		}

		PostDecimateRate = (InputSampleRate * Pre_DeDecimateRate + NewSampleRate - 1) / NewSampleRate;
		while (InputSampleRate * Pre_DeDecimateRate * 2 < NewSampleRate * PostDecimateRate * 3)
		{
			PostDecimateRate -= 1;
		}
	}
	else
	{
		// will use IIR and combination of decimation and de-decimation
		// no constraints on the ratio
	}

	try
	{
		CWaveProcContext * pFirstPass = new CWaveProcContext(pDoc);
		AddContext(pFirstPass);
		if (Pre_DeDecimateRate != 1)
		{
			pFirstPass->AddWaveProc(new CDeDecimator(Pre_DeDecimateRate));
			InputSampleRate *= Pre_DeDecimateRate;
		}
		long IirCutoffFrequency = NewSampleRate / 2;

		FilterCoefficients coeffs = { 0 };
		LowpassFilter lpf;
		double PassbandLoss = 0.995;
		double StopbandLoss = 0.003;	// -50 dB for each pass, -100 dB result

		if (InputSampleRate * Pre_DeDecimateRate == NewSampleRate * PostDecimateRate)
		{
			// will not use fractional resample filter, so can have more passband loss
			// and need more stopband loss
			PassbandLoss = 0.99;
			StopbandLoss = 0.001;	// -60 dB for each pass, -120 dB result
		}

		lpf.CreateElliptic(IirCutoffFrequency * 0.99*2.*M_PI / InputSampleRate, PassbandLoss,
							IirCutoffFrequency * 2.*M_PI / InputSampleRate, StopbandLoss);

		lpf.GetCoefficients(coeffs.m_LpfCoeffs);
		coeffs.m_nLpfOrder = lpf.GetFilterOrder();

		// now add forward pass IIR to the temp file
		CFilterProc * pFilter1 = new CFilterProc;
		pFilter1->SetFilterCoefficients(coeffs);
		pFirstPass->AddWaveProc(pFilter1);

		CWaveFormat TempFileFormat;
		// TODO: add post-roll
		TempFileFormat.InitFormat(SampleTypeFloat32, InputSampleRate, SrcFile.Channels());
		NUMBER_OF_SAMPLES TempFileNumberOfSamples = SrcFile.NumberOfSamples() * Pre_DeDecimateRate;

		CWaveFile TempFile;
		if (!TempFile.CreateWaveFile(NULL, TempFileFormat, ALL_CHANNELS, TempFileNumberOfSamples,
									CreateWaveFileDeleteAfterClose
									| CreateWaveFileAllowMemoryFile
									| CreateWaveFileTempDir
									| CreateWaveFileTemp, NULL))
		{
			AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
			throw std::bad_alloc();
		}

		pFirstPass->InitSource(SrcFile, 0, SrcFile.NumberOfSamples(), ALL_CHANNELS);
		pFirstPass->InitDestination(TempFile, 0, TempFileNumberOfSamples /*+ 1000*/, ALL_CHANNELS, FALSE);

		CWaveProcContext * pSecondPass = new CWaveProcContext(pDoc);
		pSecondPass->m_NumberOfBackwardPasses = 1;
		pSecondPass->m_NumberOfForwardPasses = 0;
		AddContext(pSecondPass);

		// now add backward pass IIR to the temp file
		CFilterProc * pFilter2 = new CFilterProc;
		pFilter2->SetFilterCoefficients(coeffs);
		pSecondPass->AddWaveProc(pFilter2);

		pSecondPass->InitSource(TempFile, 0, TempFileNumberOfSamples /*+ 1000*/, ALL_CHANNELS);
		pSecondPass->InitDestination(DstFile, 0, NewSampleRate*LONGLONG(TempFileNumberOfSamples) / InputSampleRate, ALL_CHANNELS, FALSE);

		if (PostDecimateRate != 1)
		{
			InputSampleRate /= PostDecimateRate;
			pSecondPass->AddWaveProc(new CDecimator(PostDecimateRate));
		}

		if (InputSampleRate != NewSampleRate)
		{
			// fractional resample, add CResampleFilter
			pSecondPass->AddWaveProc(new CResampleFilter(NewSampleRate, KeepSamplesPerSec));
		}

	}
	catch (std::bad_alloc)
	{
		RetireAllChildren();
		throw;
	}
}

CResampleProcContext::CResampleProcContext(CWaveSoapFrontDoc * pDoc,
											UINT StatusStringId, UINT OperationNameId,
											CWaveFile & SrcFile, CWaveFile &DstFile,
											long NewSampleRate, BOOL KeepSamplesPerSec)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
	InitSource(SrcFile, 0, LAST_SAMPLE, ALL_CHANNELS);
	InitDestination(DstFile, 0, LAST_SAMPLE, ALL_CHANNELS, FALSE);
	AddWaveProc(new CResampleFilter(NewSampleRate, KeepSamplesPerSec));
}

void CResampleProcContext::DeInit()
{
	BaseClass::DeInit();
	// if overflow happened, ask the user if it should continue
	if ( ! WasClipped())
	{
		return;
	}

	class QueryOverflow : public MainThreadCall
	{
	public:
		QueryOverflow(CResampleProcContext * pContext)
			: m_pContext(pContext)
		{
		}
	protected:
		virtual LRESULT Exec()
		{
			// bring document frame to the top, then return
			CDocumentPopup pop(m_pContext->m_pDocument);

			CString s;
			s.Format(IDS_SOUND_CLIPPED, m_pContext->m_pDocument->GetTitle(),
					int(m_pContext->GetMaxClipped() * (100. / 32678)));

			CString s1;
			s1.LoadString(IDS_CONTINUE_QUESTION);

			s += s1;
			LRESULT result = AfxMessageBox(s, MB_YESNO | MB_ICONEXCLAMATION);
			return result;
		}
		CResampleProcContext * const m_pContext;
	} call(this);

	if (IDNO == call.Call())
	{
		m_Flags |= OperationContextStop;
	}
}
