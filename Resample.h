// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
class CResampleContext : public CStagedContext
{
	typedef CStagedContext BaseClass;
	typedef CResampleContext ThisClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CResampleContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					CWaveFile & SrcFile, CWaveFile & DstFile,
					long NewSampleRate, bool KeepSamplesPerSec);
};

#endif //AFX_RESAMPLE_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_
