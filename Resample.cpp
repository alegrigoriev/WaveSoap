// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// Resample.cpp
#include "stdafx.h"
#include "Resample.h"

#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
BOOL CResampleContext::InitResample(CWaveFile & SrcFile, CWaveFile &DstFile,
									double FrequencyRatio, double FilterLength)
{
	m_SrcFile = SrcFile;
	m_DstFile = DstFile;

	m_Resample.InitResample(FrequencyRatio, FilterLength, SrcFile.Channels());

	m_SrcStart = m_SrcFile.SampleToPosition(0);
	m_SrcPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.SampleToPosition(LAST_SAMPLE);

	m_DstStart = m_DstFile.SampleToPosition(0);
	m_DstPos = m_DstStart;
	m_DstEnd = m_DstFile.SampleToPosition(LAST_SAMPLE);

	return TRUE;
}

BOOL CResampleContext::OperationProc()
{
	SAMPLE_POSITION dwOperationBegin = m_DstPos;
	DWORD dwStartTime = GetTickCount();
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_DstPos >= m_DstEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf = NULL;
	char * pDstBuf;
	do
	{
		if (0 == LeftToRead)
		{
			MEDIA_FILE_SIZE SizeToRead = m_SrcEnd - m_SrcPos;
			if (SizeToRead > CDirectFile::CacheBufferSize())
			{
				SizeToRead = CDirectFile::CacheBufferSize();
			}
			if (SizeToRead != 0)
			{
				WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcPos, CDirectFile::GetBufferAndPrefetchNext);
				if (0 == WasRead)
				{
					m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
												CDirectFile::ReturnBufferDirty);

					return FALSE;
				}
				pSrcBuf = (char *) pOriginalSrcBuf;
				LeftToRead = WasRead;
			}
			else
			{
				pSrcBuf = NULL;
				LeftToRead = 0;
				WasRead = 0;
			}
		}

		if (0 == LeftToWrite)
		{
			MEDIA_FILE_SIZE SizeToWrite = m_DstEnd - m_DstPos;
			if (0 == SizeToWrite)
			{
				break;  // all data written
			}
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);

				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;

		}

		size_t SrcBufUsed = 0;

		size_t DstBufUsed = m_Resample.ProcessSound(pSrcBuf, pDstBuf, LeftToRead,
													LeftToWrite, & SrcBufUsed);

		TRACE("ResampleContext: SrcPos=%d (0x%X), DstPos=%d (0x%X), src: %d bytes, dst: %d bytes\n",
			m_SrcPos, m_SrcPos, m_DstPos, m_DstPos,
			SrcBufUsed, DstBufUsed);
		LeftToRead -= SrcBufUsed;
		m_SrcPos += SrcBufUsed;
		pSrcBuf += SrcBufUsed;

		if (0 == LeftToRead)
		{
			m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}

		LeftToWrite -= DstBufUsed;
		m_DstPos += DstBufUsed;
		pDstBuf += DstBufUsed;

		if (0 == LeftToWrite)
		{
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}

	}
	while (m_DstPos < m_DstEnd
			&& timeGetTime() - dwStartTime < 200
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	UpdateCompletedPercent(m_DstPos, m_DstStart, m_DstEnd);

	return TRUE;
}

void CResampleContext::PostRetire(BOOL bChildContext)
{
	if (m_Flags & ConvertContextReplaceWholeFile)
	{
		if (m_Flags & OperationContextFinished)
		{
			pDocument->m_WavFile.DetachSourceFile();
			pDocument->m_WavFile = m_DstFile;
			// since we replaced the file, it's no more direct
			pDocument->SetModifiedFlag(TRUE, m_pUndoContext != NULL);
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->m_bOldDirectMode = pDocument->m_bDirectMode;

				pDocument->AddUndoRedo(m_pUndoContext);
				m_pUndoContext = NULL;
			}
			if (pDocument->m_bDirectMode)
			{
				pDocument->m_bDirectMode = false;
				pDocument->UpdateFrameTitles();        // will cause name change in views
			}
			NUMBER_OF_SAMPLES nSamples = pDocument->WaveFileSamples();
			pDocument->SoundChanged(pDocument->WaveFileID(),
									0, nSamples, nSamples,
									UpdateSoundDontRescanPeaks
									| UpdateSoundSamplingRateChanged);
			// TODO: change selection
			pDocument->BuildPeakInfo(FALSE);    // don't save it yet
			//pDocument->UpdateAllViews(NULL);
		}
		else
		{
			// we don't replace the file, nothing changed
			if (NULL != m_pUndoContext)
			{
				delete m_pUndoContext;
				m_pUndoContext = NULL;
			}
		}
	}
	CCopyContext::PostRetire(bChildContext);
}
