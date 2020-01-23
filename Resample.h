// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
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

