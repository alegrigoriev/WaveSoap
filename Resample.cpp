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

	WAVEFORMATEX * pWf = m_SrcFile.GetWaveFormat();
	MMCKINFO * pCk = m_SrcFile.GetDataChunk();
	m_SrcStart = pCk->dwDataOffset;
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcStart + pCk->cksize;

	pCk = m_DstFile.GetDataChunk();
	m_DstStart = pCk->dwDataOffset;
	m_DstCopyPos = m_DstStart;
	m_DstEnd = m_DstStart + pCk->cksize;

	return TRUE;
}

BOOL CResampleContext::OperationProc()
{
	DWORD dwOperationBegin = m_DstCopyPos;
	DWORD dwStartTime = GetTickCount();
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_DstCopyPos >= m_DstEnd)
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
			DWORD SizeToRead = m_SrcEnd - m_SrcCopyPos;
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			if (SizeToRead != 0)
			{
				WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
				if (0 == WasRead)
				{
					if (0 != WasLockedToWrite)
					{
						m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
					}
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
			DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
			if (0 == SizeToWrite)
			{
				break;  // all data written
			}
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstCopyPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
												CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;

		}

		int SrcBufUsed = 0;

		int DstBufUsed = m_Resample.ProcessSound(pSrcBuf, pDstBuf, LeftToRead,
												LeftToWrite, & SrcBufUsed);

		TRACE("ResampleContext: SrcPos=%d (0x%X), DstPos=%d (0x%X), src: %d bytes, dst: %d bytes\n",
			m_SrcCopyPos, m_SrcCopyPos, m_DstCopyPos, m_DstCopyPos,
			SrcBufUsed, DstBufUsed);
		LeftToRead -= SrcBufUsed;
		m_SrcCopyPos += SrcBufUsed;
		pSrcBuf += SrcBufUsed;

		if (0 == LeftToRead && 0 != WasRead)
		{
			m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}

		LeftToWrite -= DstBufUsed;
		m_DstCopyPos += DstBufUsed;
		pDstBuf += DstBufUsed;

		if (0 == LeftToWrite)
		{
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}

	}
	while (m_DstCopyPos < m_DstEnd
			&& timeGetTime() - dwStartTime < 200
			);

	if (0 != WasRead)
	{
		m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
									CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
									CDirectFile::ReturnBufferDirty);
	}

	if (m_DstEnd > m_DstStart)
	{
		PercentCompleted = 100i64 * (m_DstCopyPos - m_DstStart) / (m_DstEnd - m_DstStart);
	}
	return TRUE;
}

void CResampleContext::PostRetire(BOOL bChildContext)
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
			m_pUndoContext->m_OldAllocatedWavePeakSize =
				pDocument->m_AllocatedWavePeakSize;
			m_pUndoContext->m_OldWavePeakSize = pDocument->m_WavePeakSize;
			pDocument->m_WavePeakSize = 0;
			m_pUndoContext->m_OldPeakDataGranularity =
				pDocument->m_PeakDataGranularity;
			m_pUndoContext->m_pOldPeaks = pDocument->m_pPeaks;

			// detach peaks
			pDocument->m_pPeaks = NULL;
			pDocument->m_AllocatedWavePeakSize = 0;
			pDocument->m_WavePeakSize = 0;

			pDocument->AddUndoRedo(m_pUndoContext);
			m_pUndoContext = NULL;
		}
		if (pDocument->m_bDirectMode)
		{
			pDocument->m_bDirectMode = false;
			pDocument->UpdateFrameTitles();        // will cause name change in views
		}
		long nSamples = pDocument->WaveFileSamples();
		pDocument->SoundChanged(pDocument->WaveFileID(),
								0, nSamples, nSamples, UpdateSoundDontRescanPeaks);
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
	CCopyContext::PostRetire(bChildContext);
}
