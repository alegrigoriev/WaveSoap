#if !defined(AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Resample.h : header file
//
#include "OperationContext.h"

/////////////////////////////////////////////////////////////////////////////
// Resample sound
class CResampleContext : public CCopyContext
{
public:
	CResampleContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName)
		: CCopyContext(pDoc, StatusString, OperationName),
		m_pDstBuf(NULL),
		m_pSrcBuf(NULL)
	{
	}
	virtual ~CResampleContext()
	{
		if (m_pDstBuf)
		{
			delete[] m_pDstBuf;
			m_pDstBuf = NULL;
		}
		if (m_pSrcBuf)
		{
			delete[] m_pSrcBuf;
			m_pSrcBuf = NULL;
		}
	}
	enum {ResampleTableBits = 10,
		ResampleFilterSize = (1 << ResampleTableBits),
		ResampleIndexShift = (32 - ResampleTableBits),
		SrcBufSize = 0x4000,
		DstBufSize = 0x4000 };
	float *m_pSrcBuf;
	float *m_pDstBuf;

	int m_SrcBufUsed;
	int m_DstBufUsed;

	int m_SrcBufFilled;
	int m_SrcFilterLength;

	float m_FilterBuf[ResampleFilterSize];
	float m_FilterDifBuf[ResampleFilterSize];
	float m_FilterDif2Buf[ResampleFilterSize];

	unsigned __int32 m_InputPeriod;
	unsigned __int32 m_OutputPeriod;
	unsigned __int32 m_Phase;

	virtual BOOL OperationProc();
	BOOL InitResample(CWaveFile & SrcFile, CWaveFile &DstFile,
					double FrequencyRatio, double FilterLength);
	virtual void PostRetire(BOOL bChildContext = FALSE);
	void FilterSoundResample();
};

#endif //AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_
