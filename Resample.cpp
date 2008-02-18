// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// Resample.cpp
#include "stdafx.h"
#include "WaveSoapFrontDoc.h"
#include "resource.h"
#include "Resample.h"
#include "MessageBoxSynch.h"

#define _USE_MATH_DEFINES   // for M_PI definition
#include <math.h>

CResampleContext::CResampleContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									CWaveFile & SrcFile, CWaveFile &DstFile,
									long OriginalSampleRate, long NewSampleRate,
									int FilterLength,
									BOOL KeepSamplesPerSec)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
	InitSource(SrcFile, 0, LAST_SAMPLE, ALL_CHANNELS);
	InitDestination(DstFile, 0, LAST_SAMPLE, ALL_CHANNELS, FALSE);
	AddWaveProc(new CResampleFilter(OriginalSampleRate, NewSampleRate, FilterLength, SrcFile.Channels(), KeepSamplesPerSec));
}

void CResampleContext::DeInit()
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
		QueryOverflow(CResampleContext * pContext)
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
		CResampleContext * const m_pContext;
	} call(this);

	if (IDNO == call.Call())
	{
		m_Flags |= OperationContextStop;
	}
}
