#if !defined(AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Resample.h : header file
//
#include "OperationContext.h"
#include "waveproc.h"
/////////////////////////////////////////////////////////////////////////////
// Resample sound
class CResampleContext : public CCopyContext
{
public:
	CResampleContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName)
		: CCopyContext(pDoc, StatusString, OperationName)
	{
		m_GetBufferFlags = CDirectFile::GetBufferWriteOnly;
	}
	virtual ~CResampleContext()
	{
	}

	CResampleFilter m_Resample;

	virtual BOOL OperationProc();
	BOOL InitResample(CWaveFile & SrcFile, CWaveFile &DstFile,
					double FrequencyRatio, double FilterLength);
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

#endif //AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_
