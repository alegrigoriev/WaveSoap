// OperationContext.cpp
#include "stdafx.h"
#include "OperationContext.h"

static int fround(double d)
{
	if (d >= 0.)
	{
		return int(d + 0.5);
	}
	else
	{
		return int(d - 0.5);
	}
}

void COperationContext::Retire()
{
	// all context go to retirement list
	if (m_Flags & OperationContextDontKeepAfterRetire)
	{
		InterlockedDecrement( & pDocument->m_OperationInProgress);
		delete this;
		return;
	}
	// queue it to the Doc
	CSimpleCriticalSectionLock lock(pDocument->m_cs);
	COperationContext * pLast = pDocument->m_pRetiredList;
	pNext = NULL;
	if (NULL == pLast)
	{
		pDocument->m_pRetiredList = this;
	}
	else
	{
		while (NULL != pLast->pNext)
		{
			pLast = pLast->pNext;
		}
		pLast->pNext = this;
	}
}

void COperationContext::PostRetire(BOOL bChildContext)
{
	if ( ! bChildContext)
	{
		InterlockedDecrement( & pDocument->m_OperationInProgress);
	}
	delete this;
}

inline BOOL CUndoRedoContext::NeedToSave(DWORD Position, size_t length)
{
	if (Position >= m_SrcSaveEnd
		|| Position + length <= m_SrcSavePos
		|| length <= 0)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CScanPeaksContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Position < m_End)
	{
		DWORD dwStartTime = timeGetTime();
		DWORD dwOperationBegin = m_Position;
		do
		{
			DWORD SizeToRead = m_End - m_Position;
			void * pBuf;
			long lRead = pDocument->m_WavFile.m_File.GetDataBuffer( & pBuf,
							SizeToRead, m_Position, CDirectFile::GetBufferAndPrefetchNext);
			if (lRead > 0)
			{
				unsigned i;
				long DataToProcess = lRead;
				__int16 * pWaveData = (__int16 *) pBuf;
				DWORD DataOffset = m_Position - pDocument->WaveDataChunk()->dwDataOffset;
				unsigned DataForGranule = m_GranuleSize - DataOffset % m_GranuleSize;

				if (2 == pDocument->WaveChannels())
				{
					WavePeak * pPeak = pDocument->m_pPeaks + (DataOffset / m_GranuleSize) * 2;
					while (0 != DataToProcess)
					{
						int wpl_l = pPeak[0].low;
						int wpl_h = pPeak[0].high;
						int wpr_l = pPeak[1].low;
						int wpr_h = pPeak[1].high;

						if (DataForGranule > DataToProcess)
						{
							DataForGranule = DataToProcess;
						}
						DataToProcess -= DataForGranule;

						if (DataOffset & 2)
						{
							if (pWaveData[0] < wpr_l)
							{
								wpr_l = pWaveData[0];
							}
							if (pWaveData[0] > wpr_h)
							{
								wpr_h = pWaveData[0];
							}
							pWaveData++;
							DataForGranule -= 2;
						}

						for (i = 0; i < DataForGranule / (sizeof(__int16) * 2); i++, pWaveData += 2)
						{
							if (pWaveData[0] < wpl_l)
							{
								wpl_l = pWaveData[0];
							}
							if (pWaveData[0] > wpl_h)
							{
								wpl_h = pWaveData[0];
							}
							if (pWaveData[1] < wpr_l)
							{
								wpr_l = pWaveData[1];
							}
							if (pWaveData[1] > wpr_h)
							{
								wpr_h = pWaveData[1];
							}
						}

						if (DataForGranule & 2)
						{
							if (pWaveData[0] < wpl_l)
							{
								wpl_l = pWaveData[0];
							}
							if (pWaveData[0] > wpl_h)
							{
								wpl_h = pWaveData[0];
							}
							pWaveData++;
						}

						ASSERT(pPeak - pDocument->m_pPeaks < pDocument->m_WavePeakSize);
						if (pPeak[0].low > wpl_l)
						{
							pPeak[0].low = wpl_l;
						}
						if (pPeak[0].high < wpl_h)
						{
							pPeak[0].high = wpl_h;
						}
						if (pPeak[1].low > wpr_l)
						{
							pPeak[1].low = wpr_l;
						}
						if (pPeak[1].high < wpr_h)
						{
							pPeak[1].high = wpr_h;
						}
						pPeak += 2;
						DataForGranule = m_GranuleSize;
					}
				}
				else
				{
					WavePeak * pPeak = pDocument->m_pPeaks + DataOffset / m_GranuleSize;
					while (0 != DataToProcess)
					{
						int wp_l = pPeak[0].low;
						int wp_h = pPeak[0].high;

						if (DataForGranule > DataToProcess)
						{
							DataForGranule = DataToProcess;
						}
						DataToProcess -= DataForGranule;

						for (i = 0; i < DataForGranule / sizeof(__int16); i++, pWaveData ++)
						{
							if (pWaveData[0] < wp_l)
							{
								wp_l = pWaveData[0];
							}
							if (pWaveData[0] > wp_h)
							{
								wp_h = pWaveData[0];
							}
						}

						ASSERT(pPeak - pDocument->m_pPeaks < pDocument->m_WavePeakSize);
						if (pPeak[0].low > wp_l)
						{
							pPeak[0].low = wp_l;
						}
						if (pPeak[0].high < wp_h)
						{
							pPeak[0].high = wp_h;
						}
						pPeak ++;
						DataForGranule = m_GranuleSize;
					}
				}

				m_Position += lRead;
				pDocument->m_WavFile.m_File.ReturnDataBuffer(pBuf, lRead, 0);
			}
			else
			{
				m_Flags |= OperationContextStop;
				break;
				return FALSE;
			}
		}
		while (m_Position < m_End
				&& timeGetTime() - dwStartTime < 200);
		if (m_End > m_Start)
		{
			PercentCompleted = 100i64 * (m_Position - m_Start) / (m_End - m_Start);
		}
		// notify the view
		int nSampleSize = pDocument->WaveSampleSize();
		int nFirstSample = (dwOperationBegin - pDocument->WaveDataChunk()->dwDataOffset)
							/ nSampleSize;
		int nLastSample = (m_Position - pDocument->WaveDataChunk()->dwDataOffset)
						/ nSampleSize;
		pDocument->SoundChanged(pDocument->WaveFileID(),
								nFirstSample, nLastSample, -1, UpdateSoundDontRescanPeaks);
		return TRUE;
	}
	else
	{
		pDocument->SavePeakInfo();
		m_Flags |= OperationContextFinished;
		PercentCompleted = 100;
		return TRUE;
	}

}

CResizeContext::~CResizeContext()
{
	if (m_pUndoContext)
	{
		delete m_pUndoContext;
		m_pUndoContext = NULL;
	}
}

CString CResizeContext::GetStatusString() {
	if (NULL != m_pUndoContext
		&& (m_Flags & CopyCreatingUndo))
	{
		return m_pUndoContext->GetStatusString();
	}
	else
		return sOp;
}

BOOL CResizeContext::InitExpand(CWaveFile & File, LONG StartSample, LONG Length, int Channel)
{
	m_DstFile = File;
	DWORD SampleSize = m_DstFile.SampleSize();
	m_DstChan = Channel;
	// first, expand the file itself
	LPMMCKINFO pDatachunk = m_DstFile.GetDataChunk();
	if (NULL == pDatachunk)
	{
		return FALSE;
	}

	m_SrcEnd = pDatachunk->dwDataOffset + pDatachunk->cksize;
	m_SrcCopyPos = m_SrcEnd;
	m_SrcStart = pDatachunk->dwDataOffset +
				StartSample * SampleSize;

	size_t NewDataLength = (m_DstFile.NumberOfSamples() + Length) * SampleSize;

	if (FALSE == m_DstFile.m_File.SetFileLength(pDatachunk->dwDataOffset + NewDataLength))
	{
		return FALSE;
	}
	pDatachunk->cksize = NewDataLength;
	// when expanding, data is moved starting from the end of the file,
	// but I use SrcStart as begin of the moved area and SrcEns as end (SrcStart <= SrcEnd)

	m_DstStart = pDatachunk->dwDataOffset +
				(StartSample + Length) * SampleSize;

	m_DstEnd = pDatachunk->dwDataOffset + NewDataLength;
	m_DstCopyPos = m_DstEnd;

	m_Flags |= CopyExpandFile;
	return TRUE;
}

BOOL CResizeContext::InitUndoRedo(CString UndoOperationString)
{
	if (! pDocument->UndoEnabled())
	{
		return TRUE;
	}
	CUndoRedoContext * pUndo = new CUndoRedoContext(pDocument, UndoOperationString);
	if (NULL == pUndo)
	{
		return FALSE;
	}
	CResizeContext * pResize = new CResizeContext(pDocument, _T("File Resize"), "File Resize");
	if (NULL == pResize)
	{
		delete pUndo;
		return FALSE;
	}
	pResize->m_DstChan = m_DstChan;
	pResize->m_DstFile = m_DstFile;
	pUndo->m_pExpandShrinkContext = pResize;

	pResize->m_DstStart = m_SrcStart;
	pResize->m_DstEnd = m_SrcEnd;

	pResize->m_SrcStart = m_DstStart;
	pResize->m_SrcEnd = m_DstEnd;

	if (m_Flags & CopyShrinkFile)
	{
		pResize->m_DstCopyPos = m_SrcStart;
		pResize->m_SrcCopyPos = m_DstStart;
		pResize->m_Flags |= CopyExpandFile;
		pUndo->m_Flags |= CopyExpandFile;

		// Init undo to expand the file at position DstStart, expand by
		if ( ! pUndo->InitUndoCopy(m_DstFile,
									m_DstStart,
									m_SrcStart, // expand by
									m_DstChan))
		{
			delete pUndo;   // will delete m_pExpandShrinkContext, too
			return FALSE;
		}
	}
	else
	{
		pResize->m_Flags |= CopyShrinkFile;
		pUndo->m_Flags |= CopyShrinkFile;

		pResize->m_DstCopyPos = m_SrcEnd;
		pResize->m_SrcCopyPos = m_DstEnd;
		pUndo->m_SrcStart = 0;
		pUndo->m_SrcEnd = 0;
		pUndo->m_SrcCopyPos = 0;
		pUndo->m_DstStart = 0;
		pUndo->m_DstEnd = 0;
		pUndo->m_DstCopyPos = 0;
	}
	pUndo->m_RestoredLength = pDocument->m_WavFile.m_File.GetLength();
	m_pUndoContext = pUndo;
	return TRUE;
}

BOOL CResizeContext::InitShrink(CWaveFile & File, LONG StartSample, LONG Length, int Channel)
{
	m_DstFile = File;
	DWORD SampleSize = m_DstFile.SampleSize();
	m_DstChan = Channel;
	LPMMCKINFO pDatachunk = m_DstFile.GetDataChunk();
	if (NULL == pDatachunk)
	{
		return FALSE;
	}

	m_SrcEnd = pDatachunk->dwDataOffset + pDatachunk->cksize;
	m_SrcStart = pDatachunk->dwDataOffset +
				(StartSample + Length) * SampleSize;
	m_SrcCopyPos = m_SrcStart;

	// when shrinking, data is moved starting from the begin of the file,
	// but I use SrcStart as begin of the moved area and SrcEns as end (SrcStart <= SrcEnd)

	m_DstStart = pDatachunk->dwDataOffset + StartSample * SampleSize;

	m_DstEnd = m_SrcEnd - Length * SampleSize;
	m_DstCopyPos = m_DstStart;

	m_Flags |= CopyShrinkFile;

	return TRUE;
}

void CResizeContext::PostRetire(BOOL bChildContext)
{
	// save undo context
	if (m_pUndoContext)
	{
		pDocument->AddUndoRedo(m_pUndoContext);
		m_pUndoContext = NULL;
	}
	COperationContext::PostRetire(bChildContext);
}

BOOL CResizeContext::OperationProc()
{
	// change size of the file
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Flags & CopyExpandFile)
	{
		return ExpandProc();
	}
	if (m_Flags & CopyShrinkFile)
	{
		return ShrinkProc();
	}
	return FALSE;
}

BOOL CResizeContext::ShrinkProc()
{
	if (NULL != m_pUndoContext
		&& m_pUndoContext->m_SrcSavePos < m_pUndoContext->m_SrcSaveEnd)
	{
		// copy data to undo file
		DWORD dwStartTime = timeGetTime();
		DWORD dwOperationBegin = m_pUndoContext->m_SrcSavePos;

		while (m_pUndoContext->m_SrcSavePos < m_pUndoContext->m_SrcSaveEnd
				&& timeGetTime() - dwStartTime < 200)
		{
			void * pSrcBuf;
			DWORD SizeToRead = m_pUndoContext->m_SrcSaveEnd - m_pUndoContext->m_SrcSavePos;
			long WasRead = m_DstFile.m_File.GetDataBuffer( & pSrcBuf,
															SizeToRead, m_pUndoContext->m_SrcSavePos, 0);
			if (0 == WasRead)
			{
				return FALSE;
			}

			m_pUndoContext->SaveUndoData(pSrcBuf, WasRead,
										m_pUndoContext->m_SrcSavePos, m_DstChan);

			m_DstFile.m_File.ReturnDataBuffer(pSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
			m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
			//m_pUndoContext->m_SrcSavePos += WasRead;
		}

		if (m_pUndoContext->m_SrcSaveEnd > m_pUndoContext->m_SrcSaveStart)
		{
			PercentCompleted = 100i64 * (m_pUndoContext->m_SrcSavePos - m_pUndoContext->m_SrcSaveStart)
								/ (m_pUndoContext->m_SrcSaveEnd - m_pUndoContext->m_SrcSaveStart);
		}
		// set undo flag
		m_Flags |= CopyCreatingUndo;
		return TRUE;
	}

	// reset undo flag
	m_Flags &= ~CopyCreatingUndo;
	if (m_DstCopyPos >= m_DstEnd)
	{
		// file size is changed only if all channels are moved
		if (0 == (m_Flags & OperationContextFinished)
			&& 2 == m_DstChan)
		{
			MMCKINFO * pDatachunk = pDocument->WaveDataChunk();
			pDatachunk->cksize = m_DstEnd - pDatachunk->dwDataOffset;
			long NewLength = pDatachunk->cksize / pDocument->WaveSampleSize();
			// first, expand the file itself
			if (FALSE == m_DstFile.m_File.SetFileLength(m_DstEnd))
			{
				m_Flags |= OperationContextFinished;
				return FALSE;
			}
			pDocument->SoundChanged(m_DstFile.GetFileID(), 0, 0, NewLength);
		}
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	DWORD DstFlags = 0;
	if (m_DstChan == 2 // all data is copied
		&& m_SrcCopyPos - m_DstCopyPos >= 0x10000)
	{
		DstFlags = CDirectFile::GetBufferWriteOnly;
	}
	if (m_SrcCopyPos > m_SrcEnd
		|| m_DstCopyPos > m_DstEnd)
	{
		return FALSE;
	}
	do
	{
		if (0 == LeftToRead)
		{
			DWORD SizeToRead = m_SrcEnd - m_SrcCopyPos;
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			WasRead = m_DstFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				if (0 != WasLockedToWrite)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
				}
				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, DstFlags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if (m_DstChan == 2)
		{
			size_t ToCopy = __min(LeftToRead, LeftToWrite);
			memmove(pDstBuf, pSrcBuf, ToCopy);
			pDstBuf += ToCopy;
			pSrcBuf += ToCopy;
			LeftToRead -= ToCopy;
			LeftToWrite -= ToCopy;
			m_SrcCopyPos += ToCopy;
			m_DstCopyPos += ToCopy;
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) pDstBuf;
			int i;
			// both are 2 channel
			if (((m_DstCopyPos - m_DstStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pDst++;
				m_DstCopyPos += 2;
				LeftToWrite -= 2;
			}
			if (((m_SrcCopyPos - m_SrcStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pSrc++;
				m_SrcCopyPos += 2;
				LeftToRead -= 2;
			}
			size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
								LeftToWrite / (2 * sizeof(__int16)));
			for (i = 0; i < ToCopy; i++, pDst += 2, pSrc += 2)
			{
				pDst[0] = pSrc[0];
			}
			LeftToRead -= ToCopy * (2 * sizeof (__int16));
			m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
			LeftToWrite -= ToCopy * (2 * sizeof (__int16));
			m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
			if (LeftToRead >= 2 && LeftToWrite >= 2)
			{
				pDst[0] = pSrc[0];
				pDst++;
				pSrc++;
				m_DstCopyPos += 2;
				LeftToWrite -= 2;
				m_SrcCopyPos += 2;
				LeftToRead -= 2;
			}

			pSrcBuf = (char *) pSrc;
			pDstBuf = (char *) pDst;
		}
		ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
		ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
		if (0 == LeftToWrite)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
		if (0 == LeftToRead)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
	}
	while (((DstFlags & CDirectFile::GetBufferWriteOnly)
				// cannot exit while write-only buffer is incomplete
				&& 0 != WasLockedToWrite)
			|| (m_SrcCopyPos < m_SrcEnd
				&& timeGetTime() - dwStartTime < 200)
			);
	if (0 != WasRead)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}

	if (NULL != m_pUndoContext
		&& NULL != m_pUndoContext->m_pExpandShrinkContext)
	{
		// update the position to begin operation
		m_pUndoContext->m_pExpandShrinkContext->m_SrcCopyPos = m_DstCopyPos;
		m_pUndoContext->m_pExpandShrinkContext->m_DstCopyPos = m_SrcCopyPos;
	}
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcCopyPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

BOOL CResizeContext::ExpandProc()
{
	// copying is done from end backward
	if (m_SrcCopyPos <= m_SrcStart)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	DWORD DstFlags = 0;
	if (m_DstChan == 2 // all data is copied
		&& m_SrcCopyPos - m_DstCopyPos >= 0x10000)
	{
		DstFlags = CDirectFile::GetBufferWriteOnly;
	}

	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	LONG LeftToRead = 0;
	LONG LeftToWrite = 0;
	LONG WasRead = 0;
	LONG WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	if (m_SrcCopyPos < m_SrcStart
		|| m_DstCopyPos < m_DstStart)
	{
		return FALSE;
	}
	do
	{
		if (0 == LeftToRead)
		{
			LONG SizeToRead = m_SrcStart - m_SrcCopyPos;
			// SizeToRead < 0 - reading backward
			if (SizeToRead < -0x10000)
			{
				SizeToRead = -0x10000;
			}
			WasRead = m_DstFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				if (0 != WasLockedToWrite)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
				}
				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			LONG SizeToWrite = m_DstStart - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, DstFlags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if (m_DstChan == 2)
		{
			size_t ToCopy = __min(-LeftToRead, -LeftToWrite);
			pDstBuf -= ToCopy;
			pSrcBuf -= ToCopy;
			memmove(pDstBuf, pSrcBuf, ToCopy);
			LeftToRead += ToCopy;
			LeftToWrite += ToCopy;
			m_SrcCopyPos -= ToCopy;
			m_DstCopyPos -= ToCopy;
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) pDstBuf;
			int i;
			// both are 2 channel
			if (((m_DstCopyPos - m_DstStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pDst++;
				m_DstCopyPos += 2;
				LeftToWrite -= 2;
			}
			if (((m_SrcCopyPos - m_SrcStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pSrc++;
				m_SrcCopyPos += 2;
				LeftToRead -= 2;
			}
			size_t ToCopy = __min(( -LeftToRead) / (2 * sizeof(__int16)),
								( -LeftToWrite) / (2 * sizeof(__int16)));
			for (i = 0; i < ToCopy; i++)
			{
				pDst -= 2;
				pSrc -= 2;
				pDst[0] = pSrc[0];
			}
			LeftToRead += ToCopy * (2 * sizeof (__int16));
			m_SrcCopyPos -= ToCopy * (2 * sizeof (__int16));
			LeftToWrite += ToCopy * (2 * sizeof (__int16));
			m_DstCopyPos -= ToCopy * (2 * sizeof (__int16));
			if (LeftToRead <= -2 && LeftToWrite <= -2)
			{
				pDst--;
				pSrc--;
				pDst[0] = pSrc[0];
				m_DstCopyPos -= 2;
				LeftToWrite += 2;
				m_SrcCopyPos -= 2;
				LeftToRead += 2;
			}

			pSrcBuf = (char *) pSrc;
			pDstBuf = (char *) pDst;
		}
		ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
		ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
		if (0 == LeftToRead)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
	}
	while (((DstFlags & CDirectFile::GetBufferWriteOnly)
				// cannot exit while write-only buffer is incomplete
				&& 0 != WasLockedToWrite)
			|| (m_SrcCopyPos > m_SrcStart
				&& timeGetTime() - dwStartTime < 200)
			);
	if (0 != WasRead)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}

	if (NULL != m_pUndoContext
		&& NULL != m_pUndoContext->m_pExpandShrinkContext)
	{
		// update the position to begin operation
		m_pUndoContext->m_pExpandShrinkContext->m_SrcCopyPos = m_DstCopyPos;
		m_pUndoContext->m_pExpandShrinkContext->m_DstCopyPos = m_SrcCopyPos;
	}
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	// operation goes backward
	int nLastSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	int nFirstSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcEnd - m_SrcCopyPos) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

///////// CCopyContext
CString CCopyContext::GetStatusString()
{
	if (NULL != m_pUndoContext
		&& (m_Flags & CopyCreatingUndo))
	{
		return m_pUndoContext->GetStatusString();
	}
	else if (NULL != m_pExpandShrinkContext
			&& 0 == (m_pExpandShrinkContext->m_Flags & (OperationContextStop | OperationContextFinished)))
	{
		return m_pExpandShrinkContext->GetStatusString();
	}
	return sOp;
}

BOOL CCopyContext::InitCopy(CWaveFile & DstFile,
							LONG DstStartSample, LONG DstLength, LONG DstChannel,
							CWaveFile & SrcFile,
							LONG SrcStartSample, LONG SrcLength, LONG SrcChannel
							)
{
	m_DstFile = DstFile;
	m_SrcFile = SrcFile;

	m_SrcChan = SrcChannel;

	DWORD SrcSampleSize = m_SrcFile.SampleSize();
	m_SrcStart = m_SrcFile.GetDataChunk()->dwDataOffset +
				SrcStartSample * SrcSampleSize;
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.GetDataChunk()->dwDataOffset +
				(SrcStartSample + SrcLength) * SrcSampleSize;

	DWORD DstSampleSize = m_DstFile.SampleSize();

	m_DstCopyPos = m_DstFile.GetDataChunk()->dwDataOffset +
					DstStartSample * DstSampleSize;
	m_DstStart = m_DstCopyPos;

	m_DstEnd = m_DstFile.GetDataChunk()->dwDataOffset +
				(DstStartSample + SrcLength) * DstSampleSize;
	m_DstChan = DstChannel;
	if (SrcLength > DstLength)
	{
		m_pExpandShrinkContext = new CResizeContext(pDocument, _T("Expanding the file..."), "");
		if (NULL == m_pExpandShrinkContext)
		{
			return FALSE;
		}
		if ( ! m_pExpandShrinkContext->InitExpand(DstFile, DstStartSample + DstLength,
												SrcLength - DstLength, DstChannel))
		{
			delete m_pExpandShrinkContext;
			m_pExpandShrinkContext = NULL;
			return FALSE;
		}

		m_Flags |= CopyExpandFile;
	}
	else if (SrcLength < DstLength)
	{
		m_pExpandShrinkContext = new CResizeContext(pDocument, _T("Shrinking  the file..."), "");
		if (NULL == m_pExpandShrinkContext)
		{
			return FALSE;
		}
		if ( ! m_pExpandShrinkContext->InitShrink(DstFile, DstStartSample + SrcLength,
												DstLength - SrcLength, DstChannel))
		{
			delete m_pExpandShrinkContext;
			m_pExpandShrinkContext = NULL;
			return FALSE;
		}
		m_Flags |= CopyShrinkFile;
	}

	return TRUE;
}

CCopyContext::~CCopyContext()
{
	if (m_pExpandShrinkContext)
	{
		delete m_pExpandShrinkContext;
		m_pExpandShrinkContext = NULL;
	}
	if (m_pUndoContext)
	{
		delete m_pUndoContext;
		m_pUndoContext = NULL;
	}
}

BOOL CCopyContext::OperationProc()
{
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Flags & (CopyExpandFile | CopyShrinkFile))
	{
		if (NULL != m_pExpandShrinkContext)
		{
			if ( 0 == (m_pExpandShrinkContext->m_Flags & (OperationContextStop | OperationContextFinished)))
			{
				if ( ! m_pExpandShrinkContext->OperationProc())
				{
					m_pExpandShrinkContext->m_Flags |= OperationContextStop;
				}
			}
			PercentCompleted = m_pExpandShrinkContext->PercentCompleted;
			if (m_pExpandShrinkContext->m_Flags &
				(OperationContextStop | OperationContextFinished))
			{
				m_Flags &= ~(CopyExpandFile | CopyShrinkFile);
			}
			return TRUE;
		}
		m_Flags &= ~(CopyExpandFile | CopyShrinkFile);
		PercentCompleted = 0;
	}

	if (m_SrcCopyPos >= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	DWORD DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;
	if (2 == m_DstChan
		&& m_SrcFile.GetFileID() != m_DstFile.GetFileID()
		&& NULL == m_pUndoContext)
	{
		DstFileFlags = CDirectFile::GetBufferWriteOnly;
	}

	if (m_SrcCopyPos > m_SrcEnd
		|| m_DstCopyPos > m_DstEnd)
	{
		return FALSE;
	}
	do
	{
		if (0 == LeftToRead)
		{
			DWORD SizeToRead = m_SrcEnd - m_SrcCopyPos;
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			WasRead = m_SrcFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				if (0 != WasLockedToWrite)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
				}
				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;
			if (2 == m_DstChan
				&& m_SrcFile.GetFileID() != m_DstFile.GetFileID()
				&& (NULL == m_pUndoContext
					|| ! m_pUndoContext->NeedToSave(m_DstCopyPos, 0x10000)))
			{
				DstFileFlags = CDirectFile::GetBufferWriteOnly;
			}
			DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, DstFileFlags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
			// save the changed data to undo buffer
			if (0 == (DstFileFlags & CDirectFile::GetBufferWriteOnly)
				&& NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pOriginalDstBuf, WasLockedToWrite,
											m_DstCopyPos, m_DstChan);
				m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
				m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
			}

		}
		// copy whatever possible
		if ((m_DstFile.Channels() == 1
				&& m_SrcFile.Channels() == 1)
			|| (m_SrcChan == 2
				&& m_DstFile.Channels() == 2
				&& m_DstChan == 2
				&& m_SrcFile.Channels() == 2))
		{
			size_t ToCopy = __min(LeftToRead, LeftToWrite);
			memmove(pDstBuf, pSrcBuf, ToCopy);
			pDstBuf += ToCopy;
			pSrcBuf += ToCopy;
			LeftToRead -= ToCopy;
			LeftToWrite -= ToCopy;
			m_SrcCopyPos += ToCopy;
			m_DstCopyPos += ToCopy;
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) pDstBuf;
			int i;
			if (m_SrcFile.Channels() == 1)
			{
				ASSERT(m_DstFile.Channels() == 2);
				// source is mono, destination is stereo.
				// copy can go either to one channel, or to both
				if (2 == m_DstChan)
				{
					// copy to both channels
					if ((m_DstCopyPos - m_DstStart) & 2)
					{
						// right channel sample left not copied
						pDst[0] = pSrc[0];
						pDst++;
						pSrc++;
						LeftToRead -= 2;
						LeftToWrite -= 2;
						m_DstCopyPos += 2;
						m_SrcCopyPos += 2;
					}
					size_t ToCopy = __min(LeftToRead / sizeof (__int16),
										LeftToWrite / (2 * sizeof(__int16)));
					for (i = 0; i < ToCopy; i++, pSrc++, pDst += 2)
					{
						pDst[0] = pSrc[0];
						pDst[1] = pSrc[0];
					}
					LeftToRead -= ToCopy * sizeof (__int16);
					m_SrcCopyPos += ToCopy * sizeof (__int16);
					LeftToWrite -= ToCopy * (2 * sizeof (__int16));
					m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
					if (2 == LeftToWrite && LeftToRead != 0)
					{
						// only room for left channel sample
						pDst[0] = pSrc[0];
						m_DstCopyPos += 2;
						LeftToWrite -= 2;
						pDst++;
						// rigth channel will be copied later
					}
				}
				else
				{
					// copy to one channel
					// source - one channel,
					// destination - one of two
					ASSERT(m_DstFile.Channels() == 2);
					if (((m_DstCopyPos - m_DstStart) & 2)
						!= m_DstChan * 2)
					{
						// skip this word
						pDst++;
						m_DstCopyPos += 2;
						LeftToWrite -= 2;
					}
					size_t ToCopy = __min(LeftToRead / sizeof (__int16),
										LeftToWrite / (2 * sizeof(__int16)));
					for (i = 0; i < ToCopy; i++, pSrc++, pDst += 2)
					{
						pDst[0] = pSrc[0];
					}
					LeftToRead -= ToCopy * sizeof (__int16);
					m_SrcCopyPos += ToCopy * sizeof (__int16);
					LeftToWrite -= ToCopy * (2 * sizeof (__int16));
					m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
					if (2 == LeftToRead && LeftToWrite != 0)
					{
						// only one channel sample available
						pDst[0] = pSrc[0];
						pSrc++;
						pDst++;
						m_SrcCopyPos += 2;
						m_DstCopyPos += 2;
						LeftToRead -= 2;
						LeftToWrite -= 2;
					}
				}
			}
			else if (m_DstFile.Channels() == 1)
			{
				ASSERT(m_SrcFile.Channels() == 2);
				int i;
				// source is stereo, destination is mono.
				// copy can go either from one channel, or from both
				if (2 == m_SrcChan)
				{
					// copy From both channels (sum)
					if ((m_SrcCopyPos - m_SrcStart) & 2)
					{
						// right channel sample left not copied
						// left channel sample was copied before to dst[0]
						pDst[0] = (pDst[0] + pSrc[0]) / 2;
						pDst++;
						pSrc++;
						LeftToRead -= 2;
						LeftToWrite -= 2;
						m_DstCopyPos += 2;
						m_SrcCopyPos += 2;
					}
					size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
										LeftToWrite / sizeof (__int16));
					for (i = 0; i < ToCopy; i++, pDst++, pSrc += 2)
					{
						pDst[0] = (pSrc[0] + pSrc[1]) / 2;
					}
					LeftToRead -= ToCopy * (2 * sizeof (__int16));
					m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
					LeftToWrite -= ToCopy * sizeof (__int16);
					m_DstCopyPos += ToCopy * sizeof (__int16);

					if (2 == LeftToRead && LeftToWrite != 0)
					{
						// only left channel sample available
						pDst[0] = pSrc[0];
						m_SrcCopyPos += 2;
						LeftToRead -= 2;
						pSrc++;
						// rigth channel will be added later
					}
				}
				else
				{
					// copy from one channel
					// source - one of two,
					// destination - one channel
					ASSERT(m_SrcFile.Channels() == 2);
					if (((m_SrcCopyPos - m_SrcStart) & 2)
						!= m_SrcChan * 2)
					{
						// skip this word
						pSrc++;
						m_SrcCopyPos += 2;
						LeftToRead -= 2;
					}
					size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
										LeftToWrite / sizeof (__int16));
					for (i = 0; i < ToCopy; i++, pDst++, pSrc += 2)
					{
						pDst[0] = pSrc[0];
					}
					LeftToRead -= ToCopy * (2 * sizeof (__int16));
					m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
					LeftToWrite -= ToCopy * sizeof (__int16);
					m_DstCopyPos += ToCopy * sizeof (__int16);
					if (2 == LeftToRead && LeftToWrite != 0)
					{
						// only one channel sample available
						pDst[0] = pSrc[0];
						pSrc++;
						pDst++;
						m_SrcCopyPos += 2;
						m_DstCopyPos += 2;
						LeftToRead -= 2;
						LeftToWrite -= 2;
					}
				}
			}
			else
			{
				// both are 2 channel
				if (((m_DstCopyPos - m_DstStart) & 2)
					!= m_DstChan * 2)
				{
					// skip this word
					pDst++;
					m_DstCopyPos += 2;
					LeftToWrite -= 2;
				}
				if (((m_SrcCopyPos - m_SrcStart) & 2)
					!= m_SrcChan * 2)
				{
					// skip this word
					pSrc++;
					m_SrcCopyPos += 2;
					LeftToRead -= 2;
				}
				size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
									LeftToWrite / (2 * sizeof(__int16)));
				for (i = 0; i < ToCopy; i++, pDst += 2, pSrc += 2)
				{
					pDst[0] = pSrc[0];
				}
				LeftToRead -= ToCopy * (2 * sizeof (__int16));
				m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
				LeftToWrite -= ToCopy * (2 * sizeof (__int16));
				m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
				if (LeftToRead >= 2 && LeftToWrite >= 2)
				{
					pDst[0] = pSrc[0];
					pDst++;
					pSrc++;
					m_DstCopyPos += 2;
					LeftToWrite -= 2;
					m_SrcCopyPos += 2;
					LeftToRead -= 2;
				}
			}

			pSrcBuf = (char *) pSrc;
			pDstBuf = (char *) pDst;
		}
		ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
		ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
		if (0 == LeftToRead)
		{
			m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
	}
	while (((DstFileFlags & CDirectFile::GetBufferWriteOnly)
				// cannot exit while write-only buffer is incomplete
				&& 0 != WasLockedToWrite)
			|| (m_SrcCopyPos < m_SrcEnd
				&& timeGetTime() - dwStartTime < 200)
			);
	if (0 != WasRead)
	{
		m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcCopyPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

void CCopyContext::PostRetire(BOOL bChildContext)
{
	// only one undo context at a time
	ASSERT ((NULL != m_pUndoContext) + (NULL != m_pExpandShrinkContext
				&& m_pExpandShrinkContext->m_pUndoContext) <= 1);
	if (NULL != m_pUndoContext)
	{
		pDocument->AddUndoRedo(m_pUndoContext);
		m_pUndoContext = NULL;
	}
	if (NULL != m_pExpandShrinkContext)
	{
		m_pExpandShrinkContext->PostRetire(TRUE);
		m_pExpandShrinkContext = NULL;
	}
	COperationContext::PostRetire(bChildContext);
}
///////////// CDecompressContext
BOOL CDecompressContext::OperationProc()
{

	DWORD dwOperationBegin = m_DstPos;
	DWORD StartTime = GetTickCount();
	while (GetTickCount() - StartTime < 200)
	{
		// fill the source buffer
		size_t leftToRead = m_SrcEnd - m_SrcPos;
		size_t ToRead = m_SrcBufSize;
		if (ToRead > leftToRead)
		{
			ToRead = leftToRead;
		}
		if (ToRead != 0)
		{
			long read = m_SrcFile.m_File.ReadAt(m_ash.pbSrc,
												ToRead, m_SrcPos);
			m_ash.cbSrcLength = read;
		}
		else
		{
			m_ConvertFlags |= ACM_STREAMCONVERTF_END;
			m_ConvertFlags &= ~ACM_STREAMCONVERTF_BLOCKALIGN;
			m_ash.cbSrcLength = 0;
		}
		// do the conversion
		m_ash.cbSrcLengthUsed = 0;
		m_ash.cbDstLength = m_DstBufSize;
		m_ash.cbDstLengthUsed = 0;
		if (0 == acmStreamConvert(m_acmStr, & m_ash, m_ConvertFlags)
			&& (0 != m_ash.cbDstLengthUsed || 0 != m_ash.cbSrcLengthUsed))
		{
			// write the result
			if (0 != m_ash.cbDstLengthUsed)
			{
				long written = m_DstFile.m_File.WriteAt(m_ash.pbDst,
														m_ash.cbDstLengthUsed, m_DstPos);
				m_DstPos += written;
				if (written != m_ash.cbDstLengthUsed)
				{
					// error
				}
			}
			m_SrcPos += m_ash.cbSrcLengthUsed;
		}
		else
		{
			// error
		}
		m_ConvertFlags &= ~ACM_STREAMCONVERTF_START;
		if (0 == m_ash.cbSrcLengthUsed
			&& 0 == m_ash.cbDstLengthUsed)
		{
			m_Flags |= OperationContextFinished;
			m_CurrentSamples = 0;   // to force update file length
			break;
		}
	}
	// notify the view

	int nSampleSize = m_DstFile.SampleSize();
	int TotalSamples = -1;
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	// check if we need to change file size
	if (nLastSample > m_CurrentSamples)
	{
		// calculate new length
		TotalSamples = MulDiv(nLastSample, m_SrcEnd - m_SrcStart, m_SrcPos - m_SrcStart);
		LPMMCKINFO pck = m_DstFile.GetDataChunk();
		if (TotalSamples > 0x7FFFFFFF / nSampleSize)
		{
			TotalSamples = 0x7FFFFFFF / nSampleSize;
		}
		DWORD datasize = TotalSamples * nSampleSize;
		m_DstFile.m_File.SetFileLength(datasize + pck->dwDataOffset);
		m_CurrentSamples = TotalSamples;
		// update data chunk length
		pck->cksize = datasize;
		pDocument->AllocatePeakData(nLastSample);
	}
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, TotalSamples);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;
}

BOOL CDecompressContext::Init()
{
	// Open codec, allocate buffers, ets
	WAVEFORMATEX * pSrcFormat = m_SrcFile.GetWaveFormat();
	WAVEFORMATEX wf =
	{
		WAVE_FORMAT_PCM,
		pSrcFormat->nChannels,
		0,  // nSamplesPerSec
		0,  // nAvgBytesPerSec
		0, // nBlockAlign
		16, // bits per sample
		0   // cbSize
	};
	acmFormatSuggest(NULL, pSrcFormat, & wf, sizeof wf,
					ACM_FORMATSUGGESTF_NCHANNELS
					| ACM_FORMATSUGGESTF_WBITSPERSAMPLE
					| ACM_FORMATSUGGESTF_WFORMATTAG);
	TRACE("acmFormatSuggest:nSamplesPerSec=%d, BytesPerSec=%d, nBlockAlign=%d\n",
		wf.nSamplesPerSec, wf.nAvgBytesPerSec, wf.nBlockAlign);

	m_ash.cbSrcLength = 0;
	m_ash.cbDstLength = 0x10000;  // 64K

	if (0 != acmStreamOpen( & m_acmStr, NULL, pSrcFormat, & wf, NULL, NULL, NULL,
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
	// allocate buffers
	m_SrcBufSize = m_ash.cbSrcLength;
	m_DstBufSize = m_ash.cbDstLength;
	m_ash.pbSrc = new BYTE[m_SrcBufSize];
	m_ash.pbDst = new BYTE[m_DstBufSize];

	m_ash.cbStruct = sizeof m_ash;
	if (NULL == m_ash.pbSrc
		|| NULL == m_ash.pbDst)
	{
		delete m_ash.pbSrc;
		m_ash.pbSrc = NULL;
		delete m_ash.pbDst;
		m_ash.pbDst = NULL;
		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
		return FALSE;
	}
	// prepare the buffer
	acmStreamPrepareHeader(m_acmStr, & m_ash, 0);
	m_ConvertFlags = ACM_STREAMCONVERTF_START;
	return TRUE;
}

BOOL CDecompressContext::DeInit()
{
	if (m_acmStr)
	{
		m_ash.cbDstLength = m_DstBufSize;
		m_ash.cbSrcLength = m_DstBufSize;
		acmStreamUnprepareHeader(m_acmStr, & m_ash, 0);
		delete[] m_ash.pbDst;
		delete[] m_ash.pbSrc;
		m_ash.pbDst = NULL;
		m_ash.pbSrc = NULL;
		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
	}
	return TRUE;
}

////////// CSoundPlayContext
BOOL CSoundPlayContext::Init()
{
	m_OldThreadPriority = GetThreadPriority(GetCurrentThread());
	// if mono playback requested, open it as mono
	WAVEFORMATEX wfx = *(pDocument->WaveFormat());
	if (m_Chan != 2) wfx.nChannels = 1;
	MMRESULT mmres = m_WaveOut.Open(m_PlaybackDevice, & wfx, 0);
	if (MMSYSERR_NOERROR != mmres)
	{
		// TODO:
		// notify document
		return FALSE;
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	return TRUE;
}

BOOL CSoundPlayContext::DeInit()
{
	pDocument->PlaybackPositionNotify(m_SamplePlayed, -1);
	SetThreadPriority(GetCurrentThread(), m_OldThreadPriority);
	m_WaveOut.Reset();
	return TRUE;
}

void CSoundPlayContext::PostRetire(BOOL bChildContext)
{
	pDocument->m_PlayingSound = false;
	if (m_bPauseRequested)
	{
		if (pDocument->m_SelectionEnd == pDocument->m_SelectionStart)
		{
			pDocument->m_SelectionEnd = m_SamplePlayed;
		}
		pDocument->SetSelection(m_SamplePlayed, pDocument->m_SelectionEnd,
								pDocument->m_SelectedChannel, m_SamplePlayed, TRUE);
	}
	else
	{
		// return caret to the end of selected area
		pDocument->SetSelection(pDocument->m_SelectionStart, pDocument->m_SelectionEnd,
								pDocument->m_SelectedChannel, pDocument->m_SelectionEnd, TRUE);
	}
	COperationContext::PostRetire(bChildContext);
}

BOOL CSoundPlayContext::OperationProc()
{
	do
	{
		if (m_Flags & OperationContextStopRequested)
		{
			m_Flags |= OperationContextFinished;
			break;
		}
		if (m_CurrentPlaybackPos >= m_End)
		{
			// wait until all the buffer is finished
			if (FALSE == m_WaveOut.WaitForQueueEmpty(50))
			{
				break;    // not finished yet
			}
			m_Flags |= OperationContextFinished;
			PercentCompleted = 100;
			break;
		}
		char * pBuf;
		size_t size;
		int nBuf = m_WaveOut.GetBuffer( & pBuf, & size, FALSE);
		if (nBuf <= 0)
		{
			// buffer not available yet
			Sleep(50);
			break;
		}
		else
		{
			// fill the buffer
			int TotalInBuf = 0;
			while (size != 0 && m_CurrentPlaybackPos < m_End)
			{
				size_t len = m_End - m_CurrentPlaybackPos;
				if (2 == m_Chan)
				{
					if (len > size) len = size;
				}
				else
				{
					if (len > size * 2) len = size * 2;
				}
				void * pFileBuf = NULL;
				long lRead = pDocument->m_WavFile.m_File.GetDataBuffer( & pFileBuf,
								len, m_CurrentPlaybackPos, CDirectFile::GetBufferAndPrefetchNext);
				if (lRead <= 0)
				{
					m_WaveOut.ReturnBuffer(nBuf);
					return FALSE;
				}
				if (2 == m_Chan)
				{
					memcpy(pBuf, pFileBuf, lRead);
					pDocument->m_WavFile.m_File.ReturnDataBuffer(pFileBuf,
																lRead, CDirectFile::ReturnBufferDiscard);
					m_CurrentPlaybackPos += lRead;
					pBuf += lRead;
					size -= lRead;
					TotalInBuf += lRead;
				}
				else
				{
					long lUsedRead = lRead;
					__int16 * pSrcBuf = (__int16 *) pFileBuf;
					__int16 * pDstBuf = (__int16 *) pBuf;

					if ((m_CurrentPlaybackPos & 2) != m_Chan * 2)
					{
						pSrcBuf++;
					}
					for (int i = 0; i < lUsedRead / (2 * sizeof(__int16)); i++)
					{
						pDstBuf[i] = pSrcBuf[i * 2];
					}
					size -= i * 2;
					TotalInBuf += i * 2;
					pBuf += i * 2;

					pDocument->m_WavFile.m_File.ReturnDataBuffer(pFileBuf,
																lRead, CDirectFile::ReturnBufferDiscard);
					m_CurrentPlaybackPos += lRead;
				}
			}
			m_WaveOut.Play(nBuf, TotalInBuf, 0);


		}
	}
	while(0);
	// notify the document
	m_SamplePlayed = m_WaveOut.GetPosition() +
					(m_Begin - pDocument->m_WavFile.GetDataChunk()->dwDataOffset)
					/ pDocument->WaveSampleSize();
	pDocument->PlaybackPositionNotify(m_SamplePlayed, m_Chan);
	return TRUE;
}

///////////// CUndoRedoContext
BOOL CUndoRedoContext::InitUndoCopy(CWaveFile & SrcFile,
									DWORD SaveStartPos, // source file position of data needed to save and restore
									DWORD SaveEndPos,
									int SaveChannel)
{
	m_DstFile = SrcFile;

	m_SrcSaveStart = SaveStartPos;
	m_DstStart = SaveStartPos;
	m_DstCopyPos = SaveStartPos;
	m_SrcSavePos = SaveStartPos;
	m_SrcSaveEnd = SaveEndPos;
	m_DstEnd = SaveEndPos;

	if (SaveEndPos > SaveStartPos)
	{
		if ( ! m_SrcFile.CreateWaveFile( & SrcFile,
										SaveChannel,
										(SaveEndPos - SaveStartPos) / SrcFile.SampleSize(),
										CreateWaveFileTempDir
										| CreateWaveFileDeleteAfterClose
										| CreateWaveFileDontCopyInfo
										| CreateWaveFilePcmFormat
										| CreateWaveFileTemp,
										NULL))
		{
			return FALSE;
		}
		m_DstSavePos = m_DstFile.GetDataChunk()->dwDataOffset;
		m_SrcStart = m_DstSavePos;
		m_SrcCopyPos = m_SrcStart;
		m_SrcChan = 2;
		m_DstChan = SaveChannel;
		m_SaveChan = SaveChannel;
		m_SrcEnd = m_DstSavePos + (SaveEndPos - SaveStartPos);
	}
	return TRUE;
}

BOOL CUndoRedoContext::SaveUndoData(void * pBuf, long BufSize, DWORD Position, int Channel)
{
	if (Position >= m_SrcSaveEnd
		|| Position + BufSize <= m_SrcSavePos
		|| BufSize <= 0)
	{
		return FALSE;
	}

	char * pSrcBuf = (char*) pBuf;
	if (Position + BufSize > m_SrcSaveEnd)
	{
		BufSize = m_SrcSaveEnd - Position;
	}
	if (Position < m_SrcSavePos)
	{
		BufSize -= m_SrcSavePos - Position;
		pSrcBuf += m_SrcSavePos - Position;
		Position = m_SrcSavePos;
	}
	else
	{
		ASSERT(Position == m_SrcSavePos);
	}
	while (BufSize > 0)
	{
		void * buf;
		long SizeToLock = BufSize;
		if (Channel != 2
			&& (Position & 2) != Channel * 2
			&& BufSize >= 2)
		{
			BufSize -= 2;
			Position += 2;
			m_SrcSavePos += 2;
			pSrcBuf += 2;
		}

		if (Channel != 2)
		{
			SizeToLock = BufSize / 2;
		}
		long LockedToWrite = m_SrcFile.m_File.GetDataBuffer( & buf, SizeToLock,
															m_DstSavePos, CDirectFile::GetBufferWriteOnly);
		if (LockedToWrite <= 0)
		{
			TRACE("Unable to lock buffer in SaveUndoSata\n");
			return FALSE;
		}
		if (2 == Channel)
		{
			memmove(buf, pSrcBuf, LockedToWrite);
			pSrcBuf += LockedToWrite;
			BufSize -= LockedToWrite;
			m_DstSavePos += LockedToWrite;
			m_SrcSavePos += LockedToWrite;
			Position += LockedToWrite;
		}
		else
		{
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) buf;
			for (int i = 0; i < LockedToWrite / 2; i++)
			{
				pDst[i] = pSrc[i * 2];
			}
			pSrcBuf += LockedToWrite * 2;
			BufSize -= LockedToWrite * 2;
			m_SrcSavePos += LockedToWrite * 2;
			Position += LockedToWrite * 2;
			m_DstSavePos += LockedToWrite;
			if (2 == BufSize)
			{
				BufSize = 0;
				pSrcBuf += 2;
				m_SrcSavePos += 2;
				Position += 2;
			}
		}
		m_SrcFile.m_File.ReturnDataBuffer(buf, LockedToWrite, CDirectFile::ReturnBufferDirty);
	}
	return TRUE;
}

void CUndoRedoContext::PostRetire(BOOL bChildContext)
{
	// check if the operation completed
	if ((m_Flags & OperationContextFinished)
		&& (NULL == m_pExpandShrinkContext
			|| (m_pExpandShrinkContext->m_Flags & OperationContextFinished)))
	{
		CCopyContext::PostRetire(bChildContext);
		return;
	}

	// save undo context
	m_Flags &= ~(OperationContextStop | OperationContextStopRequested);
	// only one undo context at a time
	ASSERT ((NULL != m_pUndoContext) + (NULL != m_pExpandShrinkContext
				&& m_pExpandShrinkContext->m_pUndoContext) <= 1);
	if (NULL != m_pUndoContext)
	{
		pDocument->AddUndoRedo(m_pUndoContext);
		m_pUndoContext = NULL;
	}
	if (NULL != m_pExpandShrinkContext
		&& m_pExpandShrinkContext->m_pUndoContext)
	{
		pDocument->AddUndoRedo(m_pExpandShrinkContext->m_pUndoContext);
		m_pExpandShrinkContext->m_pUndoContext = NULL;
	}
	// put the context back to undo/redo list
	pDocument->AddUndoRedo(this);
	// modify modification number, depending on operation
	if (m_Flags & RedoContext)
	{
		if (pDocument->m_ModificationSequenceNumber <= 0)
		{
			pDocument->DecrementModified();
		}
	}
	else
	{
		if (pDocument->m_ModificationSequenceNumber >= 0)
		{
			pDocument->IncrementModified(FALSE);    // bDeleteRedo=FALSE
		}
	}
}

CString CUndoRedoContext::GetStatusString()
{
	if (m_Flags & RedoContext)
	{
		return "Redoing " + m_OperationName + "...";
	}
	else
	{
		return "Undoing " + m_OperationName + "...";
	}
}


CVolumeChangeContext::CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
											LPCTSTR StatusString, LPCTSTR OperationName)
	: COperationContext(pDoc, OperationName, OperationContextDiskIntensive),
	m_VolumeLeft(1.),
	m_VolumeRight(1.),
	m_bClipped(FALSE),
	m_ss(StatusString)
{
}

CVolumeChangeContext::~CVolumeChangeContext()
{
}

BOOL CVolumeChangeContext::OperationProc()
{
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_DstCopyPos >= m_DstEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	DWORD LeftToWrite = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalDstBuf;
	const DWORD DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;

	do
	{
		DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
		WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
															SizeToWrite, m_DstCopyPos, DstFileFlags);

		if (0 == WasLockedToWrite)
		{
			return FALSE;
		}

		LeftToWrite = WasLockedToWrite;
		// save the changed data to undo buffer
		if (NULL != m_pUndoContext)
		{
			m_pUndoContext->SaveUndoData(pOriginalDstBuf, WasLockedToWrite,
										m_DstCopyPos, m_DstChan);
			m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
			m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
		}

		__int16 * pDst = (__int16 *) pOriginalDstBuf;
		if (m_DstFile.Channels() == 1)
		{
			float volume = m_VolumeLeft;
			for (int i = 0; i < WasLockedToWrite / sizeof pDst[0]; i++)
			{
				long tmp = fround(pDst[i] * volume);
				if (tmp > 0x7FFF)
				{
					pDst[i] = 0x7FFF;
					m_bClipped = TRUE;
				}
				else if (tmp < -0x8000)
				{
					pDst[i] = -0x8000;
					m_bClipped = TRUE;
				}
				else
				{
					pDst[i] = __int16(tmp);
				}
			}
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			int i;
			if (2 == m_DstChan)
			{
				// process both channels
				if ((m_DstCopyPos - m_DstStart) & 2)
				{
					long tmp = fround(pDst[0] * m_VolumeRight);
					if (tmp > 0x7FFF)
					{
						pDst[0] = 0x7FFF;
						m_bClipped = TRUE;
					}
					else if (tmp < -0x8000)
					{
						pDst[0] = -0x8000;
						m_bClipped = TRUE;
					}
					else
					{
						pDst[0] = __int16(tmp);
					}
					pDst++;
					LeftToWrite -= 2;
				}

				for (i = 0; i < LeftToWrite / (2 * sizeof pDst[0]); i++, pDst += 2)
				{
					long tmp = fround(pDst[0] * m_VolumeLeft);
					if (tmp > 0x7FFF)
					{
						pDst[0] = 0x7FFF;
						m_bClipped = TRUE;
					}
					else if (tmp < -0x8000)
					{
						pDst[0] = -0x8000;
						m_bClipped = TRUE;
					}
					else
					{
						pDst[0] = __int16(tmp);
					}

					tmp = fround(pDst[1] * m_VolumeRight);
					if (tmp > 0x7FFF)
					{
						pDst[1] = 0x7FFF;
						m_bClipped = TRUE;
					}
					else if (tmp < -0x8000)
					{
						pDst[1] = -0x8000;
						m_bClipped = TRUE;
					}
					else
					{
						pDst[1] = __int16(tmp);
					}
				}

				LeftToWrite -= i * (2 * sizeof pDst[0]);
				if (2 == LeftToWrite)
				{
					long tmp = fround(pDst[0] * m_VolumeLeft);
					if (tmp > 0x7FFF)
					{
						pDst[0] = 0x7FFF;
						m_bClipped = TRUE;
					}
					else if (tmp < -0x8000)
					{
						pDst[0] = -0x8000;
						m_bClipped = TRUE;
					}
					else
					{
						pDst[0] = __int16(tmp);
					}
				}
			}
			else
			{
				// change one channel
				if (((m_DstCopyPos - m_DstStart) & 2)
					!= m_DstChan * 2)
				{
					// skip this word
					pDst++;
					LeftToWrite -= 2;
				}

				float volume;
				if (0 == m_DstChan)
				{
					volume = m_VolumeLeft;
				}
				else
				{
					volume = m_VolumeLeft;
				}

				for (i = 0; i < LeftToWrite / (2 * sizeof pDst[0]); i++, pDst += 2)
				{
					long tmp = fround(pDst[0] * volume);
					if (tmp > 0x7FFF)
					{
						pDst[0] = 0x7FFF;
						m_bClipped = TRUE;
					}
					else if (tmp < -0x8000)
					{
						pDst[0] = -0x8000;
						m_bClipped = TRUE;
					}
					else
					{
						pDst[0] = __int16(tmp);
					}
				}

				LeftToWrite -= i * (2 * sizeof pDst[0]);
				if (2 == LeftToWrite)
				{
					long tmp = fround(pDst[0] * volume);
					if (tmp > 0x7FFF)
					{
						pDst[0] = 0x7FFF;
						m_bClipped = TRUE;
					}
					else if (tmp < -0x8000)
					{
						pDst[0] = -0x8000;
						m_bClipped = TRUE;
					}
					else
					{
						pDst[0] = __int16(tmp);
					}
				}
			}
		}

		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
		m_DstCopyPos += WasLockedToWrite;
	}
	while (m_DstCopyPos < m_DstEnd
			&& timeGetTime() - dwStartTime < 200);
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_DstEnd > m_DstStart)
	{
		PercentCompleted = 100i64 * (m_DstCopyPos - m_DstStart) / (m_DstEnd - m_DstStart);
	}
	return TRUE;

}

void CVolumeChangeContext::PostRetire(BOOL bChildContext)
{
	// save undo context
	if (m_pUndoContext)
	{
		pDocument->AddUndoRedo(m_pUndoContext);
		m_pUndoContext = NULL;
	}
	if (m_bClipped)
	{
		CString s;
		s.Format(IDS_SOUND_CLIPPED, pDocument->GetTitle());
		AfxMessageBox(s, IDOK | MB_ICONEXCLAMATION);
	}
	COperationContext::PostRetire(bChildContext);
}
