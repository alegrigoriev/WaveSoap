// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext.cpp
#include "stdafx.h"
#include "OperationContext.h"
#include "OperationDialogs.h"
#include "ReopenSavedFileCopyDlg.h"
#include "MessageBoxSynch.h"

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

#ifdef _DEBUG
void COperationContext::SetBeginTime()
{
	FILETIME tmp;
	GetThreadTimes(GetCurrentThread(),
					& tmp, & tmp, & tmp, & m_ThreadUserTime);
	m_SystemTime = GetTickCount();
}

void COperationContext::PrintElapsedTime()
{
	FILETIME tmp, UserTime;
	GetThreadTimes(GetCurrentThread(),
					& tmp, & tmp, & tmp, & UserTime);
	DWORD TickCount = GetTickCount();
	TRACE("Elapsed thread time : %d ms, elapsed real time=%d\n",
		(UserTime.dwLowDateTime - m_ThreadUserTime.dwLowDateTime) / 10000,
		TickCount - m_SystemTime);
}

#endif

COperationContext::COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, DWORD Flags, LPCTSTR OperationName)
	: pDocument(pDoc),
	m_Flags(Flags),
	m_pChainedContext(NULL),
	m_pUndoContext(NULL),
	m_OperationName(OperationName),
	m_OperationString(StatusString),
	PercentCompleted(0),
	m_NumberOfForwardPasses(1),
	m_NumberOfBackwardPasses(0),
	m_CurrentPass(1),
	m_bClipped(false),
	m_MaxClipped(0.),
	m_GetBufferFlags(CDirectFile::GetBufferAndPrefetchNext),
	m_ReturnBufferFlags(0)
{
}

COperationContext::~COperationContext()
{
	if (m_pChainedContext)
	{
		COperationContext * pContext = m_pChainedContext;
		m_pChainedContext = NULL;
		delete pContext;
	}
}

CString COperationContext::GetStatusString()
{
	return m_OperationString;
}

BOOL COperationContext::InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
										CHANNEL_MASK chan, BOOL NeedUndo)
{
	m_DstFile = DstFile;
	// set begin and end offsets

	m_DstCopyPos = m_DstFile.SampleToPosition(StartSample);
	m_DstStart = m_DstCopyPos;

	m_DstEnd = m_DstFile.SampleToPosition(EndSample);

	m_DstChan = chan;
	// create undo
	if (NeedUndo)
	{
		CUndoRedoContext * pUndo = new CUndoRedoContext(pDocument, m_OperationName);
		if (NULL != pUndo
			&& pUndo->InitUndoCopy(m_DstFile, m_DstStart,
									m_DstEnd, m_DstChan))
		{
			m_pUndoContext = pUndo;
		}
		else
		{
			delete pUndo;
			//if (IDYES != AfxMessageBox(IDS_M_UNABLE_TO_CREATE_UNDO, MB_YESNO | MB_ICONEXCLAMATION))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL COperationContext::OperationProc()
{
	// generic procedure working on one file
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	BOOL res = TRUE;    // function result
	DWORD dwStartTime = timeGetTime();
	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;
	int SampleSize = m_DstFile.SampleSize();

	LONG SizeToProcess = 0;
	LONG WasLockedToWrite = 0;
	void * pDstBuf;
	if (m_CurrentPass > 0)
	{
		if (m_DstCopyPos >= m_DstEnd)
		{
			m_CurrentPass++;
			m_DstCopyPos = m_DstStart;
			if (m_CurrentPass <= m_NumberOfForwardPasses)
			{
				if ( ! InitPass(m_CurrentPass))
				{
					return FALSE;
				}
			}
		}
		if (m_CurrentPass > m_NumberOfForwardPasses)
		{
			m_DstCopyPos = m_DstEnd;
			m_CurrentPass = -1;
			if (m_CurrentPass >= -m_NumberOfBackwardPasses)
			{
				if ( ! InitPass(m_CurrentPass))
				{
					return FALSE;
				}
			}
		}
	}

	WAVE_SAMPLE TempBuf[4];
	if (m_CurrentPass > 0)
	{
		do
		{
			// make sure ProcessBuffer gets integer number of complete samples, from sample boundary

			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstEnd) - MEDIA_FILE_SIZE(m_DstCopyPos);
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pDstBuf,
														SizeToWrite, m_DstCopyPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				return FALSE;
			}

			if (WasLockedToWrite < SampleSize)
			{
				m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite, 0);
				if (SampleSize != m_DstFile.ReadAt(TempBuf, SampleSize, m_DstCopyPos))
				{
					return FALSE;
				}
				if (1 == m_CurrentPass
					&& NULL != m_pUndoContext)
				{
					m_pUndoContext->SaveUndoData(TempBuf, SampleSize,
												m_DstCopyPos, m_DstChan);
					m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
					m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
				}

				res = ProcessBuffer(TempBuf, SampleSize, m_DstCopyPos - m_DstStart, FALSE);

				if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
				{
					m_DstFile.WriteAt(TempBuf, SampleSize, m_DstCopyPos);
				}
				m_DstCopyPos += SampleSize;
				continue;
			}
			SizeToProcess = WasLockedToWrite - WasLockedToWrite % SampleSize;
			// save the data to be changed to undo buffer, but only on the first forward pass
			if (1 == m_CurrentPass
				&& NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, WasLockedToWrite,
											m_DstCopyPos, m_DstChan);
				m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
				m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
			}
			// virtual function which modifies the actual data:
			res = ProcessBuffer(pDstBuf, SizeToProcess, m_DstCopyPos - m_DstStart, FALSE);

			m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite,
										m_ReturnBufferFlags);
			m_DstCopyPos += SizeToProcess;
		}
		while (res && m_DstCopyPos < m_DstEnd
				&& timeGetTime() - dwStartTime < 200);

		if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
		{
			// notify the view
			pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstCopyPos);
		}

		if (m_DstEnd > m_DstStart)
		{
			PercentCompleted = int(100. * (m_DstCopyPos - m_DstStart + double(m_DstEnd - m_DstStart) * (m_CurrentPass - 1))
									/ (double(m_DstEnd - m_DstStart) * (m_NumberOfForwardPasses + m_NumberOfBackwardPasses)));
		}
	}
	else
	{
		// backward pass
		if (m_DstCopyPos <= m_DstStart)
		{
			m_DstCopyPos = m_DstEnd;
			m_CurrentPass--;
			if (m_CurrentPass >= -m_NumberOfBackwardPasses)
			{
				if ( ! InitPass(m_CurrentPass))
				{
					return FALSE;
				}
			}
		}
		if (m_CurrentPass < -m_NumberOfBackwardPasses)
		{
			m_Flags |= OperationContextFinished;
			return TRUE;
		}

		do
		{
			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstStart) - MEDIA_FILE_SIZE(m_DstCopyPos);

			// length requested and length returned are <0,
			// pointer returned points on the end of the buffer
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pDstBuf,
														SizeToWrite, m_DstCopyPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				return FALSE;
			}

			if (-WasLockedToWrite < SampleSize)
			{
				m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite, 0);

				if (SampleSize != m_DstFile.ReadAt(TempBuf, SampleSize, m_DstCopyPos - SampleSize))
				{
					return FALSE;
				}

				if (1 == m_CurrentPass
					&& NULL != m_pUndoContext)
				{
					m_pUndoContext->SaveUndoData(TempBuf, SampleSize,
												m_DstCopyPos, m_DstChan);
					m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
					m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
				}

				res = ProcessBuffer(TempBuf, SampleSize, m_DstCopyPos - m_DstStart, TRUE);

				if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
				{
					m_DstFile.WriteAt(TempBuf, SampleSize, m_DstCopyPos - SampleSize);
				}
				m_DstCopyPos -= SampleSize;
				continue;
			}
			SizeToProcess = -WasLockedToWrite - (-WasLockedToWrite) % SampleSize;
			// save the data to be changed to undo buffer, but only on the first forward pass
			// TODO: make for backward pass only
			if (0 && 1 == m_CurrentPass
				&& NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, SizeToProcess,
											m_DstCopyPos, m_DstChan);
				m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
				m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
			}
			// virtual function which modifies the actual data:
			res = ProcessBuffer(-SizeToProcess + (PCHAR)pDstBuf, SizeToProcess, m_DstCopyPos - m_DstStart, TRUE);   // backward=TRUE

			m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite,
										m_ReturnBufferFlags);
			// length requested and length returned are <0,
			m_DstCopyPos -= SizeToProcess;
		}
		while (res && m_DstCopyPos > m_DstStart
				&& timeGetTime() - dwStartTime < 200);

		if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
		{
			// notify the view
			pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstCopyPos);
		}

		if (m_DstEnd > m_DstStart)
		{
			PercentCompleted = int(100. * (m_DstEnd - m_DstCopyPos + double(m_DstEnd - m_DstStart)
										* (- 1 - m_CurrentPass + m_NumberOfForwardPasses))
									/ (double(m_DstEnd - m_DstStart) * (m_NumberOfForwardPasses + m_NumberOfBackwardPasses)));
		}
	}
	return res;
}

void COperationContext::Retire()
{
	// all context go to retirement list
	// queue it to the Doc
	if ((m_Flags & OperationContextFinished)
		&& NULL != m_pChainedContext)
	{
		COperationContext * pContext = m_pChainedContext;
		m_pChainedContext = NULL;
		pContext->Execute();
	}
	CSimpleCriticalSectionLock lock(pDocument->m_cs);
	pDocument->m_RetiredList.InsertTail(this);
}

void COperationContext::PostRetire(BOOL bChildContext)
{
	if (WasClipped())
	{
		// bring document frame to the top, then return
		CDocumentPopup pop(pDocument);

		CString s;
		s.Format(IDS_SOUND_CLIPPED, pDocument->GetTitle(), int(GetMaxClipped() * (100. / 32678)));
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
	}

	// save undo context
	if (m_pUndoContext)
	{
		pDocument->AddUndoRedo(m_pUndoContext);
		m_pUndoContext = NULL;
	}
	if ( ! bChildContext)
	{
		--pDocument->m_OperationInProgress;
	}
	delete this;
}

void COperationContext::Execute()
{
	pDocument->QueueOperation(this);
}

void CUndoRedoContext::Execute()
{
	if (m_Flags & UndoContextReplaceWholeFile)
	{
		if ((m_Flags & RedoContext)
			? pDocument->UndoEnabled()
			: pDocument->RedoEnabled())
		{
			// use this context as redo context
			CWaveFile TmpFile;
			TmpFile = m_SrcFile;
			m_SrcFile = pDocument->m_WavFile;
			pDocument->m_WavFile = TmpFile;

			NUMBER_OF_SAMPLES nSamples = pDocument->WaveFileSamples();

			pDocument->SoundChanged(pDocument->WaveFileID(),
									0, nSamples, nSamples, UpdateSoundDontRescanPeaks);

			// save/restore "Direct" flag and update title
			bool bTemp = pDocument->m_bDirectMode;
			pDocument->m_bDirectMode = m_bOldDirectMode;
			m_bOldDirectMode = bTemp;

			m_Flags ^= RedoContext;
			pDocument->AddUndoRedo(this);
			pDocument->UpdateFrameTitles();        // will cause name change in views
		}
		else
		{
			pDocument->m_WavFile = m_SrcFile;

			NUMBER_OF_SAMPLES nSamples = pDocument->WaveFileSamples();

			pDocument->SoundChanged(pDocument->WaveFileID(),
									0, nSamples, nSamples, UpdateSoundDontRescanPeaks);
			// save/restore "Direct" flag and update title
			pDocument->m_bDirectMode = m_bOldDirectMode;

			pDocument->UpdateFrameTitles();        // will cause name change in views
			delete this;
		}
		pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateWholeFileChanged);
		return;
	}
	else if (m_Flags & UndoContextReplaceFormat)
	{
		// replace format to the one in m_OldWaveFormat
		if ((m_Flags & RedoContext)
			? pDocument->UndoEnabled()
			: pDocument->RedoEnabled())
		{
			// use this context as redo context
			WAVEFORMATEX TmpFormat = *pDocument->WaveFormat();
			*pDocument->WaveFormat() = m_OldWaveFormat;
			m_OldWaveFormat = TmpFormat;

			m_Flags ^= RedoContext;
			pDocument->AddUndoRedo(this);
			pDocument->UpdateFrameTitles();        // will cause name change in views
			pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateSampleRateChanged);
			return;
		}
		else
		{
			*pDocument->WaveFormat() = m_OldWaveFormat;
			pDocument->UpdateFrameTitles();        // will cause name change in views
			pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateSampleRateChanged);
			delete this;
			return;
		}
	}
	CCopyContext::Execute();
}

inline BOOL CUndoRedoContext::NeedToSave(SAMPLE_POSITION Position, size_t length)
{
	ASSERT(m_SrcSavePos <= m_SrcSaveEnd);

	if (Position >= m_SrcSaveEnd
		|| Position + length <= m_SrcSavePos
		|| length <= 0)
	{
		return FALSE;
	}
	return TRUE;
}

// TODO: detach from a document, associate with a file
BOOL CScanPeaksContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_Position >= m_End)
	{
		m_Flags |= OperationContextFinished;
		PercentCompleted = 100;
		return TRUE;
	}

	DWORD dwStartTime = timeGetTime();
	SAMPLE_POSITION dwOperationBegin = m_Position;
	do
	{
		DWORD SizeToRead = m_End - m_Position;
		void * pBuf;
		long lRead = pDocument->m_WavFile.GetDataBuffer( & pBuf,
														SizeToRead, m_Position, CDirectFile::GetBufferAndPrefetchNext);
		if (lRead > 0)
		{
			unsigned i;
			ULONG DataToProcess = lRead;
			WAVE_SAMPLE * pWaveData = (WAVE_SAMPLE *) pBuf;

			DWORD DataOffset = m_Position - pDocument->WaveDataChunk()->dwDataOffset;
			unsigned DataForGranule = m_GranuleSize - DataOffset % m_GranuleSize;

			if (2 == pDocument->WaveChannels())
			{
				unsigned PeakIndex = (DataOffset / m_GranuleSize) * 2;

				while (0 != DataToProcess)
				{
					WavePeak peak = pDocument->m_WavFile.GetPeakMinMax(PeakIndex, PeakIndex + 1);
					int wpl_l = peak.low;
					int wpl_h = peak.high;

					peak = pDocument->m_WavFile.GetPeakMinMax(PeakIndex + 1, PeakIndex + 2);
					int wpr_l = peak.low;
					int wpr_h = peak.high;

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

					for (i = 0; i < DataForGranule / (sizeof(WAVE_SAMPLE) * 2); i++, pWaveData += 2)
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

					pDocument->m_WavFile.SetPeakData(PeakIndex, wpl_l, wpl_h);
					pDocument->m_WavFile.SetPeakData(PeakIndex + 1, wpr_l, wpr_h);

					PeakIndex += 2;
					DataForGranule = m_GranuleSize;
				}
			}
			else
			{
				unsigned PeakIndex = DataOffset / m_GranuleSize;
				while (0 != DataToProcess)
				{
					WavePeak peak = pDocument->m_WavFile.GetPeakMinMax(PeakIndex, PeakIndex + 1);
					int wp_l = peak.low;
					int wp_h = peak.high;

					if (DataForGranule > DataToProcess)
					{
						DataForGranule = DataToProcess;
					}
					DataToProcess -= DataForGranule;

					for (i = 0; i < DataForGranule / sizeof(pWaveData[0]); i++, pWaveData ++)
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

					pDocument->m_WavFile.SetPeakData(PeakIndex, wp_l, wp_h);

					PeakIndex ++;
					DataForGranule = m_GranuleSize;
				}
			}

			m_Position += lRead;
			pDocument->m_WavFile.ReturnDataBuffer(pBuf, lRead, 0);
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

	TRACE("CScanPeaksContext current position=%X\n", m_Position);
	if (m_End > m_Start)
	{

		PercentCompleted = 100i64 * (m_Position - m_Start) / (m_End - m_Start);
	}
	// notify the view
	SAMPLE_INDEX nFirstSample = pDocument->m_WavFile.PositionToSample(dwOperationBegin);
	SAMPLE_INDEX nLastSample = pDocument->m_WavFile.PositionToSample(m_Position);

	pDocument->SoundChanged(pDocument->WaveFileID(),
							nFirstSample, nLastSample, -1, UpdateSoundDontRescanPeaks);
	return TRUE;
}

void CScanPeaksContext::PostRetire(BOOL bChildContext)
{
	if ((m_Flags & ScanPeaksSavePeakFile)
		&& (m_Flags & OperationContextFinished)
		&& pDocument->m_OriginalWavFile.IsOpen())
	{
		pDocument->m_WavFile.SavePeakInfo(pDocument->m_OriginalWavFile);
	}
	COperationContext::PostRetire(bChildContext);
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

BOOL CResizeContext::InitExpand(CWaveFile & File, SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	m_DstFile = File;

	m_DstChan = Channel;
	// first, expand the file itself
	LPMMCKINFO pDatachunk = m_DstFile.GetDataChunk();
	if (NULL == pDatachunk)
	{
		return FALSE;
	}

	m_SrcEnd = m_DstFile.SampleToPosition(LAST_SAMPLE);
	m_SrcCopyPos = m_SrcEnd;
	m_SrcStart = m_DstFile.SampleToPosition(StartSample);

	WAV_FILE_SIZE NewDataLength = m_DstFile.SampleToPosition(m_DstFile.NumberOfSamples() + Length);

	if (FALSE == m_DstFile.SetFileLength(pDatachunk->dwDataOffset + NewDataLength))
	{
		NotEnoughDiskSpaceMessageBox();
		return FALSE;
	}
	pDatachunk->cksize = NewDataLength;
	pDatachunk->dwFlags |= MMIO_DIRTY;
	// when expanding, data is moved starting from the end of the file,
	// but I use SrcStart as begin of the moved area and SrcEns as end (SrcStart <= SrcEnd)

	m_DstStart = m_DstFile.SampleToPosition(StartSample + Length);

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
	CResizeContext * pResize = new CResizeContext(pDocument, _T("File Resize"), _T("File Resize"));
	if (NULL == pResize)
	{
		delete pUndo;
		return FALSE;
	}
	pResize->m_DstChan = m_DstChan;
	// don't keep a reference to the file
	//pResize->m_DstFile = m_DstFile;
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
		if ( ! pUndo->InitUndoCopy(pDocument->m_WavFile,
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
	pUndo->m_RestoredLength = pDocument->m_WavFile.GetLength();
	m_pUndoContext = pUndo;
	return TRUE;
}

BOOL CResizeContext::InitShrink(CWaveFile & File, SAMPLE_INDEX StartSample,
								NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	m_DstFile = File;

	m_DstChan = Channel;

	m_SrcEnd = m_DstFile.SampleToPosition(LAST_SAMPLE);
	m_SrcStart = m_DstFile.SampleToPosition(StartSample + Length);

	m_SrcCopyPos = m_SrcStart;

	// when shrinking, data is moved starting from the begin of the file,
	// but I use SrcStart as begin of the moved area and SrcEns as end (SrcStart <= SrcEnd)

	m_DstStart = m_DstFile.SampleToPosition(StartSample);
	m_DstCopyPos = m_DstStart;

	ASSERT(m_DstFile.NumberOfSamples() >= Length);

	m_DstEnd = m_DstFile.SampleToPosition(m_DstFile.NumberOfSamples() - Length);

	m_Flags |= CopyShrinkFile;

	return TRUE;
}

BOOL CResizeContext::OperationProc()
{
	// change size of the file
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
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
		SAMPLE_POSITION dwOperationBegin = m_pUndoContext->m_SrcSavePos;

		while (m_pUndoContext->m_SrcSavePos < m_pUndoContext->m_SrcSaveEnd
				&& timeGetTime() - dwStartTime < 200)
		{
			void * pSrcBuf;
			DWORD SizeToRead = m_pUndoContext->m_SrcSaveEnd - m_pUndoContext->m_SrcSavePos;
			long WasRead = m_DstFile.GetDataBuffer( & pSrcBuf,
													SizeToRead, m_pUndoContext->m_SrcSavePos, 0);
			if (0 == WasRead)
			{
				return FALSE;
			}

			m_pUndoContext->SaveUndoData(pSrcBuf, WasRead,
										m_pUndoContext->m_SrcSavePos, m_DstChan);

			m_DstFile.ReturnDataBuffer(pSrcBuf, WasRead,
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
			&& ALL_CHANNELS == m_DstChan)
		{
			MMCKINFO * pDatachunk = pDocument->WaveDataChunk();
			pDatachunk->cksize = m_DstEnd - pDatachunk->dwDataOffset;
			pDatachunk->dwFlags |= MMIO_DIRTY;

			NUMBER_OF_SAMPLES NewLength = pDatachunk->cksize / pDocument->WaveSampleSize();
			// first, shrink the file itself
			if (FALSE == m_DstFile.SetFileLength(m_DstEnd))
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

	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	DWORD DstFlags = 0;
	if (m_DstChan == ALL_CHANNELS // all data is copied
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
			MEDIA_FILE_SIZE SizeToRead = MEDIA_FILE_SIZE(m_SrcEnd) - MEDIA_FILE_SIZE(m_SrcCopyPos);
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			WasRead = m_DstFile.GetDataBuffer( & pOriginalSrcBuf,
												SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);

				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstEnd) - MEDIA_FILE_SIZE(m_DstCopyPos);

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstCopyPos, DstFlags);

			if (0 == WasLockedToWrite)
			{
				m_DstFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);

				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if (m_DstChan == ALL_CHANNELS)
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
			WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) pSrcBuf;
			WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) pDstBuf;
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
			unsigned i;
			size_t ToCopy = __min(LeftToRead / (2 * sizeof pSrc[0]),
								LeftToWrite / (2 * sizeof pDst[0]));

			for (i = 0; i < ToCopy; i++, pDst += 2, pSrc += 2)
			{
				pDst[0] = pSrc[0];
			}
			LeftToRead -= ToCopy * (2 * sizeof pSrc[0]);
			m_SrcCopyPos += ToCopy * (2 * sizeof pSrc[0]);

			LeftToWrite -= ToCopy * (2 * sizeof pDst[0]);
			m_DstCopyPos += ToCopy * (2 * sizeof pDst[0]);

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
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
		if (0 == LeftToRead)
		{
			m_DstFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
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

	m_DstFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	if (NULL != m_pUndoContext
		&& NULL != m_pUndoContext->m_pExpandShrinkContext)
	{
		// update the position to begin operation
		m_pUndoContext->m_pExpandShrinkContext->m_SrcCopyPos = m_DstCopyPos;
		m_pUndoContext->m_pExpandShrinkContext->m_DstCopyPos = m_SrcCopyPos;
	}
	// notify the view

	pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstCopyPos);

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
	if (m_DstChan == ALL_CHANNELS // all data is copied
		&& m_SrcCopyPos - m_DstCopyPos >= 0x10000)
	{
		DstFlags = CDirectFile::GetBufferWriteOnly;
	}

	DWORD dwStartTime = timeGetTime();

	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;

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
			MEDIA_FILE_SIZE SizeToRead = MEDIA_FILE_SIZE(m_SrcStart) - MEDIA_FILE_SIZE(m_SrcCopyPos);
			ASSERT(SizeToRead < 0);

			// SizeToRead < 0 - reading backward
			if (SizeToRead < -0x10000)
			{
				SizeToRead = -0x10000;
			}
			WasRead = m_DstFile.GetDataBuffer( & pOriginalSrcBuf,
												SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);

				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstStart) - MEDIA_FILE_SIZE(m_DstCopyPos);

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstCopyPos, DstFlags);

			if (0 == WasLockedToWrite)
			{
				m_DstFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);

				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if (m_DstChan == ALL_CHANNELS)
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
			WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) pSrcBuf;
			WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) pDstBuf;
			unsigned i;
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
			size_t ToCopy = __min(( -LeftToRead) / (2 * sizeof pSrc[0]),
								( -LeftToWrite) / (2 * sizeof pDst[0]));
			for (i = 0; i < ToCopy; i++)
			{
				pDst -= 2;
				pSrc -= 2;
				pDst[0] = pSrc[0];
			}

			LeftToRead += ToCopy * (2 * sizeof pSrc[0]);
			m_SrcCopyPos -= ToCopy * (2 * sizeof pSrc[0]);

			LeftToWrite += ToCopy * (2 * sizeof pDst[0]);
			m_DstCopyPos -= ToCopy * (2 * sizeof pDst[0]);

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
			m_DstFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
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

	m_DstFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	if (NULL != m_pUndoContext
		&& NULL != m_pUndoContext->m_pExpandShrinkContext)
	{
		// update the position to begin operation
		m_pUndoContext->m_pExpandShrinkContext->m_SrcCopyPos = m_DstCopyPos;
		m_pUndoContext->m_pExpandShrinkContext->m_DstCopyPos = m_SrcCopyPos;
	}
	// notify the view
	pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstCopyPos);

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

void CCopyContext::InitSource(CWaveFile & SrcFile, SAMPLE_INDEX StartSample,
							SAMPLE_INDEX EndSample, CHANNEL_MASK SrcChannel)
{
	m_SrcFile = SrcFile;

	m_SrcChan = SrcChannel;

	m_SrcStart = m_SrcFile.SampleToPosition(StartSample);
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.SampleToPosition(EndSample);
}

BOOL CCopyContext::InitCopy(CWaveFile & DstFile,
							SAMPLE_INDEX DstStartSample, NUMBER_OF_SAMPLES DstLength, CHANNEL_MASK DstChannel,
							CWaveFile & SrcFile,
							SAMPLE_INDEX SrcStartSample, NUMBER_OF_SAMPLES SrcLength, CHANNEL_MASK SrcChannel
							)
{
	m_DstFile = DstFile;
	m_SrcFile = SrcFile;

	m_SrcChan = SrcChannel;

	m_SrcStart = m_SrcFile.SampleToPosition(SrcStartSample);
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.SampleToPosition(SrcStartSample + SrcLength);

	m_DstCopyPos = m_DstFile.SampleToPosition(DstStartSample);
	m_DstStart = m_DstCopyPos;

	m_DstEnd = m_DstFile.SampleToPosition(DstStartSample + SrcLength);
	m_DstChan = DstChannel;

	if (SrcLength > DstLength)
	{
		m_pExpandShrinkContext = new CResizeContext(pDocument, _T("Expanding the file..."), _T(""));
		if (NULL == m_pExpandShrinkContext)
		{
			NotEnoughMemoryMessageBox();
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
		m_pExpandShrinkContext = new CResizeContext(pDocument, _T("Shrinking  the file..."), _T(""));
		if (NULL == m_pExpandShrinkContext)
		{
			NotEnoughMemoryMessageBox();
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
		m_Flags |= OperationContextStop;
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
	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	DWORD DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;
	if (ALL_CHANNELS == m_DstChan
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
			MEDIA_FILE_SIZE SizeToRead = MEDIA_FILE_SIZE(m_SrcEnd) - MEDIA_FILE_SIZE(m_SrcCopyPos);
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
												SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);

				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;
			if (ALL_CHANNELS == m_DstChan
				&& m_SrcFile.GetFileID() != m_DstFile.GetFileID()
				&& (NULL == m_pUndoContext
					|| ! m_pUndoContext->NeedToSave(m_DstCopyPos, 0x10000)))
			{
				DstFileFlags = CDirectFile::GetBufferWriteOnly;
			}

			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstEnd) - MEDIA_FILE_SIZE(m_DstCopyPos);

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstCopyPos, DstFileFlags);

			if (0 == WasLockedToWrite)
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
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
			|| (m_SrcChan == ALL_CHANNELS
				&& m_DstFile.Channels() == 2
				&& m_DstChan == ALL_CHANNELS
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
			WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) pSrcBuf;
			WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) pDstBuf;
			unsigned i;
			if (m_SrcFile.Channels() == 1)
			{
				ASSERT(m_DstFile.Channels() == 2);
				// source is mono, destination is stereo.
				// copy can go either to one channel, or to both
				if (ALL_CHANNELS == m_DstChan)
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
					size_t ToCopy = __min(LeftToRead / sizeof pSrc[0],
										LeftToWrite / (2 * sizeof pDst[0]));

					for (i = 0; i < ToCopy; i++, pSrc++, pDst += 2)
					{
						pDst[0] = pSrc[0];
						pDst[1] = pSrc[0];
					}

					LeftToRead -= ToCopy * sizeof pSrc[0];
					m_SrcCopyPos += ToCopy * sizeof pSrc[0];

					LeftToWrite -= ToCopy * (2 * sizeof pDst[0]);
					m_DstCopyPos += ToCopy * (2 * sizeof pDst[0]);
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
					size_t ToCopy = __min(LeftToRead / sizeof pSrc[0],
										LeftToWrite / (2 * sizeof pDst[0]));
					for (i = 0; i < ToCopy; i++, pSrc++, pDst += 2)
					{
						pDst[0] = pSrc[0];
					}
					LeftToRead -= ToCopy * sizeof pSrc[0];
					m_SrcCopyPos += ToCopy * sizeof pSrc[0];
					LeftToWrite -= ToCopy * (2 * sizeof pDst[0]);
					m_DstCopyPos += ToCopy * (2 * sizeof pDst[0]);
					if (2 == LeftToWrite && LeftToRead != 0)
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
				unsigned i;
				// source is stereo, destination is mono.
				// copy can go either from one channel, or from both
				if (ALL_CHANNELS == m_SrcChan)
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
					size_t ToCopy = __min(LeftToRead / (2 * sizeof pSrc[0]),
										LeftToWrite / sizeof pDst[0]);

					for (i = 0; i < ToCopy; i++, pDst++, pSrc += 2)
					{
						pDst[0] = (pSrc[0] + pSrc[1]) / 2;
					}

					LeftToRead -= ToCopy * (2 * sizeof pSrc[0]);
					m_SrcCopyPos += ToCopy * (2 * sizeof pSrc[0]);

					LeftToWrite -= ToCopy * sizeof pDst[0];
					m_DstCopyPos += ToCopy * sizeof pDst[0];

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
					size_t ToCopy = __min(LeftToRead / (2 * sizeof pSrc[0]),
										LeftToWrite / sizeof pDst[0]);
					for (i = 0; i < ToCopy; i++, pDst++, pSrc += 2)
					{
						pDst[0] = pSrc[0];
					}
					LeftToRead -= ToCopy * (2 * sizeof pSrc[0]);
					m_SrcCopyPos += ToCopy * (2 * sizeof pSrc[0]);

					LeftToWrite -= ToCopy * sizeof pDst[0];
					m_DstCopyPos += ToCopy * sizeof pDst[0];

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
				size_t ToCopy = __min(LeftToRead / (2 * sizeof pSrc[0]),
									LeftToWrite / (2 * sizeof pDst[0]));

				for (i = 0; i < ToCopy; i++, pDst += 2, pSrc += 2)
				{
					pDst[0] = pSrc[0];
				}

				LeftToRead -= ToCopy * (2 * sizeof pSrc[0]);
				m_SrcCopyPos += ToCopy * (2 * sizeof pSrc[0]);

				LeftToWrite -= ToCopy * (2 * sizeof pDst[0]);
				m_DstCopyPos += ToCopy * (2 * sizeof pDst[0]);

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
			m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
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

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	// notify the view
	pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstCopyPos);

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

	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;
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
			long read = m_SrcFile.ReadAt(m_ash.pbSrc,
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
		BOOL Success;
		if (NULL == m_acmStr)
		{
			memcpy(m_ash.pbDst, m_ash.pbSrc, m_ash.cbSrcLength);
			m_ash.cbSrcLengthUsed = m_ash.cbSrcLength;
			m_ash.cbDstLengthUsed = m_ash.cbSrcLength;
			Success = TRUE;
		}
		else
		{
			Success = (0 == (m_MmResult = acmStreamConvert(m_acmStr, & m_ash, m_ConvertFlags))
						&& (0 != m_ash.cbDstLengthUsed || 0 != m_ash.cbSrcLengthUsed));
		}
		if (Success)
		{
			// write the result
			if (0 != m_ash.cbDstLengthUsed)
			{
				if (m_bSwapBytes)
				{
					for (unsigned i = 0; i < m_ash.cbDstLengthUsed - 1; i+= 2)
					{
						BYTE tmp = m_ash.pbDst[i];
						m_ash.pbDst[i] = m_ash.pbDst[i + 1];
						m_ash.pbDst[i + 1] = tmp;
					}
				}
				long written = m_DstFile.WriteAt(m_ash.pbDst,
												m_ash.cbDstLengthUsed, m_DstCopyPos);
				m_DstCopyPos += written;
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

	SAMPLE_INDEX nFirstSample = m_DstFile.PositionToSample(dwOperationBegin);
	SAMPLE_INDEX nLastSample = m_DstFile.PositionToSample(m_DstCopyPos);
	// check if we need to change file size
	NUMBER_OF_SAMPLES TotalSamples = -1;

	if (nLastSample > m_CurrentSamples)
	{
		// calculate new length
		int nSampleSize = m_DstFile.SampleSize();
		TotalSamples = MulDiv(nLastSample, m_SrcEnd - m_SrcStart, m_SrcPos - m_SrcStart);
		if (TotalSamples > 0x7FFFFFFF / nSampleSize)
		{
			TotalSamples = 0x7FFFFFFF / nSampleSize;
		}

		LPMMCKINFO pck = m_DstFile.GetDataChunk();
		WAV_FILE_SIZE datasize = TotalSamples * nSampleSize;
		m_DstFile.SetFileLength(datasize + pck->dwDataOffset);
		m_CurrentSamples = TotalSamples;

		// update data chunk length
		pck->cksize = datasize;
		pck->dwFlags |= MMIO_DIRTY;
		pDocument->m_WavFile.AllocatePeakData(nLastSample);
	}
	TRACE("Decompress: sound changed from %d to %d, length=%d\n",
		nFirstSample, nLastSample, TotalSamples);
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
	WAVEFORMATEX wf =
	{
		WAVE_FORMAT_PCM,
		m_Wf.NumChannels(),
		0,  // nSamplesPerSec
		0,  // nAvgBytesPerSec
		0, // nBlockAlign
		16, // bits per sample
		0   // cbSize
	};
	m_ash.cbSrcLength = 0x10000;
	m_ash.cbDstLength = 0x10000;  // 64K
	if (WAVE_FORMAT_PCM != m_Wf.FormatTag()
		|| 16 != m_Wf.BitsPerSample())
	{
		if (0 == m_acmStr)
		{
			if (MMSYSERR_NOERROR != (m_MmResult = acmFormatSuggest(m_acmDrv, m_Wf,
													& wf, sizeof wf,
													ACM_FORMATSUGGESTF_NCHANNELS
													| ACM_FORMATSUGGESTF_WBITSPERSAMPLE
													| ACM_FORMATSUGGESTF_WFORMATTAG))
				|| MMSYSERR_NOERROR != (m_MmResult = acmStreamOpen( & m_acmStr, m_acmDrv,
														m_Wf, & wf, NULL, NULL, NULL, ACM_STREAMOPENF_NONREALTIME)))
			{
				return FALSE;
			}
		}

		TRACE("acmFormatSuggest:nSamplesPerSec=%d, BytesPerSec=%d, nBlockAlign=%d\n",
			wf.nSamplesPerSec, wf.nAvgBytesPerSec, wf.nBlockAlign);

		if (MMSYSERR_NOERROR != (m_MmResult = acmStreamSize(m_acmStr, m_ash.cbDstLength, & m_ash.cbSrcLength,
															ACM_STREAMSIZEF_DESTINATION))
			|| MMSYSERR_NOERROR != (m_MmResult = acmStreamSize(m_acmStr, m_ash.cbSrcLength, & m_ash.cbDstLength,
																ACM_STREAMSIZEF_SOURCE)))
		{
			if (m_acmStr != NULL)
			{
				acmStreamClose(m_acmStr, 0);
				m_acmStr = NULL;
			}
			// todo:error
			return FALSE;
		}
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
		delete[] m_ash.pbSrc;
		m_ash.pbSrc = NULL;
		delete[] m_ash.pbDst;
		m_ash.pbDst = NULL;
		if (NULL != m_acmStr)
		{
			acmStreamClose(m_acmStr, 0);
		}
		m_acmStr = NULL;
		// todo:error
		return FALSE;
	}
	// prepare the buffer
	if (NULL != m_acmStr)
	{
		acmStreamPrepareHeader(m_acmStr, & m_ash, 0);
	}
	m_ConvertFlags = ACM_STREAMCONVERTF_START;
	return TRUE;
}

void CDecompressContext::DeInit()
{
	m_ash.cbDstLength = m_DstBufSize;
	m_ash.cbSrcLength = m_DstBufSize;
	if (NULL != m_acmStr)
	{
		acmStreamUnprepareHeader(m_acmStr, & m_ash, 0);
	}
	delete[] m_ash.pbDst;
	delete[] m_ash.pbSrc;
	m_ash.pbDst = NULL;
	m_ash.pbSrc = NULL;
	if (NULL != m_acmStr)
	{
		acmStreamClose(m_acmStr, 0);
	}
	m_acmStr = NULL;
	if (NULL != m_acmDrv)
	{
		acmDriverClose(m_acmDrv, 0);
		m_acmDrv = NULL;
	}
}

void CDecompressContext::PostRetire(BOOL bChildContext)
{
	if (m_MmResult != MMSYSERR_NOERROR)
	{
		CString s;
		if (m_Flags & OperationContextInitFailed)
		{
			s.Format(IDS_CANT_DECOMPRESS_FILE, LPCTSTR(m_SrcFile.GetName()),
					m_SrcFile.GetWaveFormat()->wFormatTag, m_MmResult);
			pDocument->m_bCloseThisDocumentNow = true;
		}
		else
		{
			s.Format(IDS_ERROR_WHILE_DECOMPRESSING_FILE,
					LPCTSTR(m_SrcFile.GetName()), m_MmResult);
		}
		AfxMessageBox(s, MB_ICONSTOP);
	}
	else if (m_Flags & DecompressSavePeakFile)
	{
		pDocument->m_WavFile.SavePeakInfo(pDocument->m_OriginalWavFile);
	}
	COperationContext::PostRetire(bChildContext);
}

////////// CSoundPlayContext
CSoundPlayContext::CSoundPlayContext(CWaveSoapFrontDoc * pDoc, CWaveFile & WavFile,
									SAMPLE_INDEX PlaybackStart, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK Channel,
									int PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize)
	: COperationContext(pDoc, _T("Playing"),
						OperationContextDontAdjustPriority, _T("Play"))
	, m_bPauseRequested(false)
	, m_FirstSamplePlayed(PlaybackStart)
	, m_SamplePlayed(PlaybackStart)
	, m_LastSamplePlayed(PlaybackEnd)
	, m_CurrentPlaybackPos(0)
	, m_Begin(0)
	, m_End(0)
	, m_Chan(Channel)
	, m_PlaybackDevice(PlaybackDevice)
	, m_PlaybackBuffers(PlaybackBuffers)
	, m_PlaybackBufferSize(PlaybackBufferSize)
{
	PercentCompleted = -1;  // no percents
	m_PlayFile = WavFile;
}

BOOL CSoundPlayContext::Init()
{
	MMCKINFO * pDatack = m_PlayFile.GetDataChunk();

	m_Begin = m_PlayFile.SampleToPosition(m_FirstSamplePlayed);

	m_CurrentPlaybackPos = m_Begin;

	if (m_FirstSamplePlayed == m_LastSamplePlayed)
	{
		m_End = m_PlayFile.SampleToPosition(LAST_SAMPLE);
	}
	else
	{
		m_End = m_PlayFile.SampleToPosition(m_LastSamplePlayed);
	}

	m_OldThreadPriority = GetThreadPriority(GetCurrentThread());

	// if mono playback requested, open it as mono
	WAVEFORMATEX wfx = * m_PlayFile.GetWaveFormat();

	if (m_Chan != ALL_CHANNELS) wfx.nChannels = 1;
	wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	MMRESULT mmres = m_WaveOut.Open(m_PlaybackDevice, & wfx, 0);

	if (MMSYSERR_NOERROR != mmres)
	{
		MessageBoxSync(IDS_STRING_UNABLE_TO_OPEN_AUDIO_DEVICE,
						MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (false == m_WaveOut.AllocateBuffers(m_PlaybackBufferSize,
											m_PlaybackBuffers))
	{
		MessageBoxSync(IDS_STRING_UNABLE_TO_ALLOCATE_AUDIO_BUFFERS,
						MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	return TRUE;
}

void CSoundPlayContext::DeInit()
{
	pDocument->PlaybackPositionNotify(-1, -2);// sample=-1, channel=-2
	SetThreadPriority(GetCurrentThread(), m_OldThreadPriority);
	m_WaveOut.Reset();
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
		pDocument->SetSelection(m_SamplePlayed, m_SamplePlayed,
								pDocument->m_SelectedChannel, m_SamplePlayed, SetSelection_MoveCaretToCenter);
	}
	else
	{
		// return caret to the end of selected area
		pDocument->SetSelection(pDocument->m_SelectionStart, pDocument->m_SelectionEnd,
								pDocument->m_SelectedChannel, pDocument->m_SelectionEnd, SetSelection_MoveCaretToCenter);
	}
	COperationContext::PostRetire(bChildContext);
}

BOOL CSoundPlayContext::OperationProc()
{
	do
	{
		if (m_Flags & OperationContextStopRequested)
		{
			m_Flags |= OperationContextStop;
			if (m_bPauseRequested)
			{
				m_OperationString = _T("Playback Paused");
			}
			else
			{
				m_OperationString = _T("Playback Stopped");
			}
			PercentCompleted = -2;
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
			PercentCompleted = -2;
			m_OperationString = _T("Playback Stopped");
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
				if (ALL_CHANNELS == m_Chan)
				{
					if (len > size) len = size;
				}
				else
				{
					if (len > size * 2) len = size * 2;
				}
				void * pFileBuf = NULL;
				long lRead = m_PlayFile.GetDataBuffer( & pFileBuf,
														len, m_CurrentPlaybackPos, CDirectFile::GetBufferAndPrefetchNext);
				if (lRead <= 0)
				{
					m_WaveOut.ReturnBuffer(nBuf);
					return FALSE;
				}
				if (ALL_CHANNELS == m_Chan)
				{
					memcpy(pBuf, pFileBuf, lRead);
					m_PlayFile.ReturnDataBuffer(pFileBuf,
												lRead, CDirectFile::ReturnBufferDiscard);
					m_CurrentPlaybackPos += lRead;
					pBuf += lRead;
					size -= lRead;
					TotalInBuf += lRead;
				}
				else
				{
					long const lUsedRead = lRead;
					WAVE_SAMPLE * pSrcBuf = (WAVE_SAMPLE *) pFileBuf;
					WAVE_SAMPLE * pDstBuf = (WAVE_SAMPLE *) pBuf;

					if ((m_CurrentPlaybackPos & 2) != m_Chan * 2)
					{
						pSrcBuf++;
					}
					for (unsigned i = 0; i < lUsedRead / (2 * sizeof pSrcBuf[0]); i++)
					{
						pDstBuf[i] = pSrcBuf[i * 2];
					}
					size -= i * 2;
					TotalInBuf += i * 2;
					pBuf += i * 2;

					m_PlayFile.ReturnDataBuffer(pFileBuf,
												lRead, CDirectFile::ReturnBufferDiscard);
					m_CurrentPlaybackPos += lRead;
				}
			}
			m_WaveOut.Play(nBuf, TotalInBuf, 0);


		}
	}
	while(0);
	// notify the document
	m_SamplePlayed = m_WaveOut.GetPosition() + m_FirstSamplePlayed;

	pDocument->PlaybackPositionNotify(m_SamplePlayed, m_Chan);
	return TRUE;
}

///////////// CUndoRedoContext
BOOL CUndoRedoContext::InitUndoCopy(CWaveFile & SrcFile,
									SAMPLE_POSITION SaveStartPos, // source file position of data needed to save and restore
									SAMPLE_POSITION SaveEndPos,
									CHANNEL_MASK SaveChannel)
{
	// don't keep reference to the file
	//m_DstFile = SrcFile;

	m_SrcSaveStart = SaveStartPos;
	m_SrcSavePos = SaveStartPos;
	m_SrcSaveEnd = SaveEndPos;
	m_DstStart = SaveStartPos;
	m_DstCopyPos = SaveStartPos;
	m_DstEnd = SaveEndPos;
	// TODO: Check if file is expanded as a result of operation
	if (SaveEndPos > SaveStartPos)
	{
		if ( ! m_SrcFile.CreateWaveFile( & SrcFile, NULL,
										SaveChannel,
										(SaveEndPos - SaveStartPos) / SrcFile.SampleSize(),
										CreateWaveFileTempDir
										| CreateWaveFileDeleteAfterClose
										| CreateWaveFileDontCopyInfo
										| CreateWaveFilePcmFormat
										| CreateWaveFileAllowMemoryFile
										| CreateWaveFileTemp,
										NULL))
		{
			NotEnoughUndoSpaceMessageBox();
			return FALSE;
		}

		m_DstSavePos = SrcFile.SampleToPosition(0);
		m_SrcStart = m_DstSavePos;
		m_SrcCopyPos = m_SrcStart;
		m_SrcChan = ALL_CHANNELS;
		m_DstChan = SaveChannel;
		m_SaveChan = SaveChannel;
		m_SrcEnd = m_DstSavePos + (SaveEndPos - SaveStartPos)
					/ SrcFile.SampleSize() * m_SrcFile.SampleSize();
	}
	return TRUE;
}

BOOL CUndoRedoContext::SaveUndoData(void * pBuf, long BufSize, SAMPLE_POSITION Position, CHANNEL_MASK Channel)
{
	if ((m_Flags & UndoContextReplaceWholeFile)
		|| Position >= m_SrcSaveEnd
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
		if (Channel != ALL_CHANNELS
			&& (Position % (2 * sizeof (WAVE_SAMPLE))) != Channel * sizeof (WAVE_SAMPLE))
		{
			BufSize -= sizeof (WAVE_SAMPLE);
			Position += sizeof (WAVE_SAMPLE);
			m_SrcSavePos += sizeof (WAVE_SAMPLE);
			pSrcBuf += sizeof (WAVE_SAMPLE);
			if (0 == BufSize)
			{
				break;
			}
		}

		if (Channel != ALL_CHANNELS)
		{
			// TODO: for NumChannels != 1, 2
			SizeToLock = ((BufSize + 2) & ~3)/ sizeof (WAVE_SAMPLE);
		}
		long const LockedToWrite = m_SrcFile.GetDataBuffer( & buf, SizeToLock,
															m_DstSavePos, CDirectFile::GetBufferWriteOnly);
		if (LockedToWrite <= 0)
		{
			TRACE("Unable to lock buffer in SaveUndoSata\n");
			return FALSE;
		}
		if (ALL_CHANNELS == Channel)
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
			WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) pSrcBuf;
			WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
			for (unsigned i = 0; i < LockedToWrite / sizeof pSrc[0]; i++)
			{
				pDst[i] = pSrc[i * 2];
			}
			pSrcBuf += LockedToWrite * 2;
			BufSize -= LockedToWrite * 2;
			m_SrcSavePos += LockedToWrite * 2;
			Position += LockedToWrite * 2;
			m_DstSavePos += LockedToWrite;
#if 0
			if (2 == BufSize)
			{
				BufSize = 0;
				pSrcBuf += 2;
				m_SrcSavePos += 2;
				Position += 2;
			}
#endif
		}
		m_SrcFile.ReturnDataBuffer(buf, LockedToWrite, CDirectFile::ReturnBufferDirty);
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
			pDocument->IncrementModified(FALSE, TRUE);    // bDeleteRedo=FALSE
		}
	}
}

CUndoRedoContext::~CUndoRedoContext()
{
	if (m_pOldPeaks)
	{
		delete[] m_pOldPeaks;
		m_pOldPeaks = NULL;
	}
}

CString CUndoRedoContext::GetStatusString()
{
	if (m_Flags & RedoContext)
	{
		return _T("Redoing ") + m_OperationName + _T("...");
	}
	else
	{
		return _T("Undoing ") + m_OperationName + _T("...");
	}
}


CVolumeChangeContext::CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
											LPCTSTR StatusString, LPCTSTR OperationName)
	: COperationContext(pDoc, StatusString, OperationContextDiskIntensive, OperationName),
	m_VolumeLeft(1.),
	m_VolumeRight(1.)
{
	m_GetBufferFlags = 0;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
}

CVolumeChangeContext::~CVolumeChangeContext()
{
}

BOOL CVolumeChangeContext::ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward)
{
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	if (m_DstFile.Channels() == 1)
	{
		float volume = m_VolumeLeft;
		// special code for mute and inverse
		if (0 == volume)
		{
			memset(buf, 0, BufferLength);
		}
		else if (-1. == volume)
		{
			for (unsigned i = 0; i < BufferLength / sizeof pDst[0]; i++)
			{
				pDst[i] = LongToShort(-long(pDst[i]));
			}
		}
		else
		{
			for (unsigned i = 0; i < BufferLength / sizeof pDst[0]; i++)
			{
				pDst[i] = DoubleToShort(pDst[i] * volume);
			}
		}
		return TRUE;
	}
	// special code for mute
	if (0 == m_VolumeLeft && 0 == m_VolumeRight)
	{
		if (ALL_CHANNELS == m_DstChan)
		{
			memset(pDst, 0, BufferLength);
		}
		else
		{
			for (unsigned i = m_DstChan; i < BufferLength / sizeof pDst[0]; i+=2)
			{
				pDst[i] = 0;
			}
		}
		return TRUE;
	}
	// process both channels
	// special code for mute and inverse
	unsigned i;
	if ((-1. == m_VolumeLeft || 1 == m_DstChan)
		&& (-1. == m_VolumeRight || 0 == m_DstChan))
	{
		for (i = 0; i < BufferLength / sizeof pDst[0]; i += 2)
		{
			if (1 != m_DstChan)
			{
				pDst[i] = LongToShort(-long(pDst[i]));
			}

			if (0 != m_DstChan)
			{
				pDst[i + 1] = LongToShort(-long(pDst[i + 1]));
			}
		}
		return TRUE;
	}
	for (i = 0; i < BufferLength / sizeof pDst[0]; i += 2)
	{
		if (1 != m_DstChan)
		{
			pDst[i] = DoubleToShort(pDst[i] * m_VolumeLeft);
		}

		if (0 != m_DstChan)
		{
			pDst[i + 1] = DoubleToShort(pDst[i + 1] * m_VolumeRight);
		}
	}
	return TRUE;
}

CDcOffsetContext::CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
									LPCTSTR StatusString, LPCTSTR OperationName)
	: COperationContext(pDoc, StatusString, OperationContextDiskIntensive, OperationName),
	m_OffsetLeft(0),
	m_OffsetRight(0),
	m_pScanContext(NULL)
{
	m_GetBufferFlags = 0;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
}

CDcOffsetContext::~CDcOffsetContext()
{
	if (m_pScanContext)
	{
		delete m_pScanContext;
		m_pScanContext = NULL;
	}
}
BOOL CDcOffsetContext::OperationProc()
{
	if (NULL != m_pScanContext
		&& m_Flags & ContextScanning)
	{
		if (m_Flags & OperationContextStopRequested)
		{
			m_Flags |= OperationContextStop;
			return TRUE;
		}
		if ( ! m_pScanContext->OperationProc())
		{
			return FALSE;
		}
		PercentCompleted = m_pScanContext->PercentCompleted;
		if (0 == (m_pScanContext->m_Flags & OperationContextFinished))
		{
			return TRUE;
		}
		m_Flags &= ~ ContextScanning;
		// calculate DC offset
		NUMBER_OF_SAMPLES nSamples =
			(m_pScanContext->m_DstCopyPos - m_pScanContext->m_DstStart)
			/ m_pScanContext->m_DstFile.SampleSize();
		if (0 == nSamples)
		{
			return FALSE;
		}
		m_OffsetLeft = -int(m_pScanContext->m_SumLeft / nSamples);
		m_OffsetRight = -int(m_pScanContext->m_SumRight / nSamples);

		delete m_pScanContext;
		m_pScanContext = NULL;

		if ((m_DstChan == 1 || 0 == m_OffsetLeft)
			&& (m_DstFile.Channels() == 1
				|| 0 == m_OffsetRight
				|| m_DstChan == 0))
		{
			// all offsets are zero, nothing to change
			m_Flags |= OperationContextFinished;
		}
		return TRUE;
	}
	return COperationContext::OperationProc();
}

BOOL CDcOffsetContext::ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward)
{
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	unsigned i;
	if (m_DstFile.Channels() == 1)
	{
		int DcOffset = m_OffsetLeft;
		for (i = 0; i < BufferLength / sizeof pDst[0]; i++)
		{
			pDst[i] = LongToShort(pDst[i] + DcOffset);
		}
		return TRUE;
	}
	if (ALL_CHANNELS == m_DstChan)
	{
		// process both channels

		for (i = 0; i < BufferLength / sizeof pDst[0]; i+=2)
		{
			pDst[i] = LongToShort(pDst[i] + m_OffsetLeft);
			pDst[i + 1] = LongToShort(pDst[i + 1] + m_OffsetRight);
		}
	}
	else if (0 == m_DstChan)
	{
		// change one channel
		for (i = 0; i < BufferLength / sizeof pDst[0]; i+=2)
		{
			pDst[i] = LongToShort(pDst[i] + m_OffsetLeft);
		}
	}
	else if (1 == m_DstChan)
	{
		// change one channel
		for (i = 0; i < BufferLength / sizeof pDst[0]; i+=2)
		{
			pDst[i + 1] = LongToShort(pDst[i + 1] + m_OffsetRight);
		}
	}

	return TRUE;
}

CString CDcOffsetContext::GetStatusString()
{
	if (NULL != m_pScanContext
		&& m_Flags & ContextScanning)
	{
		return m_pScanContext->GetStatusString();
	}
	return COperationContext::GetStatusString();
}

CStatisticsContext::CStatisticsContext(CWaveSoapFrontDoc * pDoc,
										LPCTSTR StatusString, LPCTSTR OperationName)
	: COperationContext(pDoc, StatusString, OperationContextDiskIntensive, OperationName),
	m_ZeroCrossingLeft(0),
	m_ZeroCrossingRight(0),
	m_MinLeft(INT_MAX),
	m_MaxLeft(INT_MIN),
	m_MinRight(INT_MAX),
	m_MaxRight(INT_MIN),

	m_PrevSampleLeft(0),
	m_PrevSampleRight(0),

	m_PosMinLeft(0),
	m_PosMaxLeft(0),
	m_PosMinRight(0),
	m_PosMaxRight(0),

	m_EnergyLeft(0),
	m_EnergyRight(0),

	m_CurrentLeftCrc(0),
	m_CRC32Left(0),
	m_CurrentRightCrc(0),
	m_CRC32Right(0),
	m_CurrentCommonCRC(0),
	m_CRC32Common(0),

	m_Checksum(0),
	m_ChecksumSampleNumber(0),

	m_SumLeft(0),
	m_SumRight(0)
{
	m_DstChan = ALL_CHANNELS;
}

// CRC32 Lookup Table generated from Charles Michael
//  Heard's CRC-32 code
static DWORD CRC32_Table[256] =
{
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
	0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
	0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
	0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
	0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
	0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
	0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
	0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
	0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
	0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
	0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
	0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
	0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
	0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
	0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
	0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
	0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
	0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
	0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
	0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
	0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
	0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
	0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
	0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
	0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
	0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
	0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
	0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
	0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
	0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
	0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
	0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
	0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
	0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
	0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
	0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
	0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
	0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
	0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
	0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
	0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
	0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
	0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static inline DWORD CalcCrc32(DWORD PrevCrc, UCHAR byte)
{
	return (PrevCrc << 8) ^ CRC32_Table[(( PrevCrc >> 24) ^ byte) & 0xff];
}

BOOL CStatisticsContext::ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward)
{
	WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) buf;
	if (m_DstFile.Channels() == 1)
	{
		if (m_Flags & StatisticsContext_DcOnly)
		{
			for (unsigned i = 0; i < BufferLength / sizeof pSrc[0]; i++)
			{
				m_SumLeft += pSrc[i];
			}
		}
		else if (m_Flags & StatisticsContext_MinMaxOnly)
		{
			for (unsigned i = 0; i < BufferLength / sizeof pSrc[0]; i++)
			{
				int sample = pSrc[i];

				if (m_MinLeft > sample)
				{
					m_MinLeft = sample;
				}
				if (m_MaxLeft < sample)
				{
					m_MaxLeft = sample;
				}
			}
		}
		else for (unsigned i = 0; i < BufferLength / sizeof pSrc[0]; i++)
		{
			int sample = pSrc[i];
			m_SumLeft += sample;
			m_EnergyLeft += sample * sample;
			if (m_MinLeft > sample)
			{
				m_MinLeft = sample;
				m_PosMinLeft = offset + i * sizeof pSrc[0];
			}
			if (m_MaxLeft < sample)
			{
				m_PosMaxLeft = offset + i * sizeof pSrc[0];
				m_MaxLeft = sample;
			}
			if (0x8000 & (sample ^ m_PrevSampleLeft))
			{
				m_ZeroCrossingLeft++;
			}
			m_PrevSampleLeft = sample;
			m_CurrentLeftCrc = CalcCrc32(CalcCrc32(m_CurrentLeftCrc, sample & 0xFF), (sample >> 8) & 0xFF);
			if (sample != 0)
			{
				m_CRC32Left = m_CurrentLeftCrc;
			}

			if (0 == m_ChecksumSampleNumber
				&& (sample > 127 || sample < -127))
			{
				m_ChecksumSampleNumber = 1;
			}
			m_Checksum += m_ChecksumSampleNumber * sample;
			m_ChecksumSampleNumber++;
			if (m_ChecksumSampleNumber > 256)
			{
				m_ChecksumSampleNumber = 1;
			}
		}
	}
	else
	{
		if (m_Flags & StatisticsContext_DcOnly)
		{
			if (BufferLength >= sizeof pSrc[0] && 0 != (offset % (2 * sizeof pSrc[0])))
			{
				m_SumRight += pSrc[0];
				pSrc++;
				offset += sizeof pSrc[0];
				BufferLength -= sizeof pSrc[0];
			}
			for (unsigned i = 0; i < BufferLength / (2 * sizeof pSrc[0]); i++)
			{
				m_SumLeft += pSrc[i * 2];
				m_SumRight += pSrc[i * 2 + 1];
			}
			BufferLength -= i * (2 * sizeof pSrc[0]);
			if (BufferLength >= sizeof pSrc[0])
			{
				m_SumLeft += pSrc[i * 2];
			}
		}
		else if (m_Flags & StatisticsContext_MinMaxOnly)
		{
			for (unsigned i = 0; i < BufferLength / sizeof pSrc[0]; i++, offset += sizeof pSrc[0])
			{
				int sample = pSrc[i];
				if (0 != (offset % (2 * sizeof pSrc[0])))
				{
					if (m_MinRight > sample)
					{
						m_MinRight = sample;
					}
					if (m_MaxRight < sample)
					{
						m_MaxRight = sample;
					}
				}
				else
				{
					if (m_MinLeft > sample)
					{
						m_MinLeft = sample;
					}
					if (m_MaxLeft < sample)
					{
						m_MaxLeft = sample;
					}
				}
			}
		}
		else for (unsigned i = 0; i < BufferLength / sizeof pSrc[0]; i++, offset += sizeof pSrc[0])
		{
			int sample = pSrc[i];
			if (0 != (offset % (2 * sizeof pSrc[0])))
			{
				m_SumRight += sample;
				m_EnergyRight += sample * sample;
				if (m_MinRight > sample)
				{
					m_MinRight = sample;
					m_PosMinRight = offset;
				}
				if (m_MaxRight < sample)
				{
					m_MaxRight = sample;
					m_PosMaxRight = offset;
				}
				if (0x8000 & (sample ^ m_PrevSampleRight))
				{
					m_ZeroCrossingRight++;
				}
				m_PrevSampleRight = sample;
				m_CurrentRightCrc = CalcCrc32(CalcCrc32(m_CurrentRightCrc, sample & 0xFF), (sample >> 8) & 0xFF);
				m_CurrentCommonCRC = CalcCrc32(CalcCrc32(m_CurrentCommonCRC, sample & 0xFF), (sample >> 8) & 0xFF);
				if (sample != 0)
				{
					m_CRC32Right = m_CurrentRightCrc;
					m_CRC32Common = m_CurrentCommonCRC;
				}
			}
			else
			{
				m_SumLeft += sample;
				m_EnergyLeft += sample * sample;
				if (m_MinLeft > sample)
				{
					m_MinLeft = sample;
					m_PosMinLeft = offset;
				}
				if (m_MaxLeft < sample)
				{
					m_MaxLeft = sample;
					m_PosMaxLeft = offset;
				}
				if (0x8000 & (sample ^ m_PrevSampleLeft))
				{
					m_ZeroCrossingLeft++;
				}
				m_PrevSampleLeft = sample;
				m_CurrentLeftCrc = CalcCrc32(CalcCrc32(m_CurrentLeftCrc, sample & 0xFF), (sample >> 8) & 0xFF);
				m_CurrentCommonCRC = CalcCrc32(CalcCrc32(m_CurrentCommonCRC, sample & 0xFF), (sample >> 8) & 0xFF);
				if (sample != 0)
				{
					m_CRC32Left = m_CurrentLeftCrc;
					m_CRC32Common = m_CurrentCommonCRC;
				}
			}

			if (0 == m_ChecksumSampleNumber
				&& (sample > 127 || sample < -127))
			{
				m_ChecksumSampleNumber = 1;
			}
			m_Checksum += m_ChecksumSampleNumber * sample;
			m_ChecksumSampleNumber++;
			if (m_ChecksumSampleNumber > 256)
			{
				m_ChecksumSampleNumber = 1;
			}
		}
	}
	return TRUE;
}

void CStatisticsContext::PostRetire(BOOL bChildContext)
{
	// read sample value at cursor
	WAVE_SAMPLE Value[2] = {0, 0};
	if (pDocument->m_CaretPosition < pDocument->WaveFileSamples())
	{
		int SampleSize = pDocument->WaveSampleSize();
		DWORD offset = pDocument->m_WavFile.SampleToPosition(pDocument->m_CaretPosition);

		if (SampleSize > sizeof Value)
		{
			SampleSize = sizeof Value;
		}
		pDocument->m_WavFile.ReadAt(Value, SampleSize, offset);
	}
	// show dialog
	CStatisticsDialog dlg;
	dlg.m_pContext = this;
	dlg.m_SamplesPerSec = pDocument->WaveSampleRate();
	dlg.m_CaretPosition = pDocument->m_CaretPosition;
	dlg.m_ValueAtCursorLeft = Value[0];
	dlg.m_ValueAtCursorRight = Value[1];
	dlg.m_sFilename = pDocument->GetTitle();

	CDocumentPopup pop(pDocument);
	dlg.DoModal();

	COperationContext::PostRetire(bChildContext);
}

BOOL CNormalizeContext::OperationProc()
{
	if (NULL != m_pScanContext
		&& m_Flags & ContextScanning)
	{
		if (m_Flags & OperationContextStopRequested)
		{
			m_Flags |= OperationContextStop;
			return TRUE;
		}
		if ( ! m_pScanContext->OperationProc())
		{
			return FALSE;
		}
		PercentCompleted = m_pScanContext->PercentCompleted;
		if (0 == (m_pScanContext->m_Flags & OperationContextFinished))
		{
			return TRUE;
		}
		m_Flags &= ~ ContextScanning;

		// calculate normalization
		int MaxLeft = abs(m_pScanContext->m_MaxLeft);
		int MinLeft = abs(m_pScanContext->m_MinLeft);
		if (MaxLeft < MinLeft) MaxLeft = MinLeft;
		if (MaxLeft != 0)
		{
			m_VolumeLeft = float(32767. * m_LimitLevel / MaxLeft);
		}
		else
		{
			m_VolumeLeft = 100.;
		}

		int MaxRight = abs(m_pScanContext->m_MaxRight);
		int MinRight = abs(m_pScanContext->m_MinRight);
		if (MaxRight < MinRight) MaxRight = MinRight;
		if (MaxRight != 0)
		{
			m_VolumeRight = float(32767. * m_LimitLevel / MaxRight);
		}
		else
		{
			m_VolumeRight = 100.;
		}

		delete m_pScanContext;
		m_pScanContext = NULL;

		if (m_DstFile.Channels() > 1
			&& m_DstChan == ALL_CHANNELS
			&& m_bEqualChannels)
		{
			if (m_VolumeLeft > m_VolumeRight)
			{
				m_VolumeLeft = m_VolumeRight;
			}
			else
			{
				m_VolumeRight = m_VolumeLeft;
			}
		}
		if ((m_DstChan == 1 || 1. == m_VolumeLeft || 0 == MaxLeft)
			&& (m_DstFile.Channels() == 1
				|| 1. == m_VolumeRight || 0 == MaxRight
				|| m_DstChan == 0))
		{
			// nothing to change
			m_Flags |= OperationContextFinished;
		}
		return TRUE;
	}
	return COperationContext::OperationProc();
}

CString CNormalizeContext::GetStatusString()
{
	if (NULL != m_pScanContext
		&& m_Flags & ContextScanning)
	{
		return m_pScanContext->GetStatusString();
	}
	return CVolumeChangeContext::GetStatusString();
}

BOOL CFileSaveContext::OperationProc()
{
	if (m_pConvert)
	{
		m_Flags |= m_pConvert->m_Flags & (OperationContextStop | OperationContextFinished);
		if (m_Flags & (OperationContextStop | OperationContextFinished))
		{
			return TRUE;
		}
		m_pConvert->m_Flags |= m_Flags & OperationContextStopRequested;
		BOOL result = m_pConvert->OperationProc();
		PercentCompleted = m_pConvert->PercentCompleted;
		return result;
	}
	return CCopyContext::OperationProc();
}

void CFileSaveContext::Execute()
{
	if (NULL != m_pConvert)
	{
		// reset OperationContextDiskIntensive
		// if it's not set in m_pConvert
		m_Flags &= m_pConvert->m_Flags | ~OperationContextDiskIntensive;
	}
	CCopyContext::Execute();
}

void CFileSaveContext::PostRetire(BOOL bChildContext)
{
	if (NULL != m_pConvert)
	{
		// update data chunk and number of samples
		MMCKINFO * datack = m_pConvert->m_DstFile.GetDataChunk();

		if (NULL != datack && datack->ckid != 0)
		{
			datack->dwFlags |= MMIO_DIRTY;
			datack->cksize = m_pConvert->m_DstCopyPos - datack->dwDataOffset;
			//MMCKINFO * fact = m_pConvert->m_DstFile.GetFactChunk();
			m_DstFile.GetFactChunk()->dwFlags |= MMIO_DIRTY;
			// save number of samples in the main context
			m_DstFile.m_FactSamples =
				(m_pConvert->m_SrcCopyPos - m_pConvert->m_SrcStart)
				/ m_pConvert->m_SrcFile.SampleSize();
			m_pConvert->m_DstCopyPos = (m_pConvert->m_DstCopyPos + 1) & ~1;
		}
		// set length of file (even)
		m_DstFile.SetFileLength(m_pConvert->m_DstCopyPos);
		// release references
		m_pConvert->PostRetire(TRUE);
		m_pConvert = NULL;
	}
	// rename files, change them
	if (m_Flags & OperationContextFinished)
	{
		// but if a copy is created, just close it
		if (m_Flags & FileSaveContext_SavingCopy)
		{
			// will be closed by the destructor
			m_DstFile.CommitChanges();
			pDocument->m_WavFile.SavePeakInfo(m_DstFile);

			// ask about opening the file
			CReopenSavedFileCopyDlg dlg;
			UINT fmt = IDS_OPEN_SAVED_FILE_COPY;
			if (m_NewFileTypeFlags != 0
				|| m_DstFile.GetWaveFormat()->wFormatTag != WAVE_FORMAT_PCM
				|| m_DstFile.GetWaveFormat()->wBitsPerSample != 16)
			{
				fmt = IDS_OPEN_SAVED_FILE_COPY_NONDIRECT;
				dlg.m_bDisableDirect = TRUE;
			}

			dlg.m_Prompt.Format(fmt, LPCTSTR(pDocument->GetTitle()),
								LPCTSTR(m_NewName));
			int res;
			{
				CDocumentPopup pop(pDocument);
				res = dlg.DoModal();
			}
			if (IDCANCEL != res)
			{
				m_DstFile.Close();  // to allow open
				int OpenFlags = 0;
				if (IDOK == res)
				{
					// direct mode
					OpenFlags = OpenDocumentDirectMode;
				}
				// file type will be determined there
				GetApp()->OpenDocumentFile(m_NewName, OpenFlags);
			}
		}
		else
		{
			// exchange files, close and delete old ones, rename new ones
			pDocument->m_FileTypeFlags = m_NewFileTypeFlags;
			pDocument->PostFileSave(this);
		}
	}
	else
	{
		// Operation canceled, discard the file
		pDocument->m_bClosePending = false;
		m_DstFile.DeleteOnClose();
		m_DstFile.Close();
	}
	CCopyContext::PostRetire(bChildContext);
}

BOOL CFileSaveContext::Init()
{
	CCopyContext::Init();
	if (NULL != m_pConvert
		&& ! m_pConvert->Init())
	{
		return FALSE;
	}
	return TRUE;
}

void CFileSaveContext::DeInit()
{
	if (NULL != m_pConvert)
	{
		m_pConvert->DeInit();
	}
	CCopyContext::DeInit();
}

BOOL CConversionContext::OperationProc()
{
	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;
	DWORD dwStartTime = GetTickCount();
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
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
			MEDIA_FILE_SIZE SizeToRead = m_SrcEnd - m_SrcCopyPos;
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
			MEDIA_FILE_SIZE SizeToWrite = m_DstEnd - m_DstCopyPos;
			if (0 == SizeToWrite)
			{
				m_DstEnd = m_DstCopyPos + 0x10000;
				SizeToWrite = 0x10000;
			}
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstCopyPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);

				return FALSE;
			}
			// save UNDO
			// save the changed data to undo buffer
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, WasLockedToWrite,
											m_DstCopyPos, m_DstChan);
				m_pUndoContext->m_DstEnd = m_pUndoContext->m_SrcSavePos;
				m_pUndoContext->m_SrcEnd = m_pUndoContext->m_DstSavePos;
			}

		}

		size_t SrcBufUsed = 0;

		size_t DstBufUsed = m_ProcBatch.ProcessSound(pSrcBuf, pDstBuf, LeftToRead,
													LeftToWrite, & SrcBufUsed);

		if (0 == SrcBufUsed && 0 == DstBufUsed)
		{
			m_Flags |= OperationContextFinished;
			break;
		}
		if (0) TRACE("ConversionContext: SrcPos=%d (0x%X), DstPos=%d (0x%X), src: %d bytes, dst: %d bytes\n",
					m_SrcCopyPos, m_SrcCopyPos, m_DstCopyPos, m_DstCopyPos,
					SrcBufUsed, DstBufUsed);
		LeftToRead -= SrcBufUsed;
		m_SrcCopyPos += SrcBufUsed;
		pSrcBuf += SrcBufUsed;

		if (0 == LeftToRead)
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
	while (m_SrcCopyPos < m_SrcEnd
			&& timeGetTime() - dwStartTime < 500
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	// notify the view
	pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstCopyPos);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcCopyPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;
}

BOOL CWmaDecodeContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		TRACE("CWmaDecodeContext::OperationProc: Stop Requested\n");
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	DWORD dwStartTime = timeGetTime();

	do
	{
		m_Decoder.DeliverNextSample(200);
	}
	while (m_Decoder.IsStarted()
			&& timeGetTime() - dwStartTime < 500);

	// notify the view

	SAMPLE_INDEX nFirstSample = m_DstCopySample;
	SAMPLE_INDEX nLastSample = m_Decoder.GetCurrentSample();
	m_DstCopySample = nLastSample;

	NUMBER_OF_SAMPLES OldSampleCount = m_CurrentSamples;
	NUMBER_OF_SAMPLES NewSampleCount = m_Decoder.GetTotalSamples();
	m_CurrentSamples = NewSampleCount;

	if (NewSampleCount == OldSampleCount)
	{
		NewSampleCount = -1;
	}
	if ( ! m_Decoder.IsStarted())
	{
		m_Flags |= OperationContextFinished;
		NewSampleCount = m_CurrentSamples;
	}
	if (nFirstSample != nLastSample
		|| -1 != NewSampleCount)
	{
		TRACE("Changed from %d to %d, length=%d\n", nFirstSample, nLastSample, NewSampleCount);

		pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, NewSampleCount);
	}

	if (m_CurrentSamples)
	{
		PercentCompleted = MulDiv(100, m_DstCopySample, m_CurrentSamples);
	}
	return TRUE;
}

BOOL CWmaDecodeContext::Init()
{
	m_CoInit.InitializeCom(COINIT_APARTMENTTHREADED);

	if ( ! m_Decoder.Init()
		|| S_OK != m_Decoder.Open(m_WmaFile))
	{

		CString s;
		s.Format(IDS_CANT_OPEN_WMA_DECODER, m_WmaFile.GetName());
		MessageBoxSync(s, MB_ICONEXCLAMATION | MB_OK);

		pDocument->m_bCloseThisDocumentNow = true;
		return FALSE;
	}

	m_CurrentSamples = m_Decoder.GetTotalSamples();

	// create a wave file in the document
	if ( ! pDocument->m_WavFile.CreateWaveFile(NULL, m_Decoder.GetDstFormat(),
												ALL_CHANNELS, m_CurrentSamples,  // initial sample count
												CreateWaveFileTempDir
												| CreateWaveFileDeleteAfterClose
												| CreateWaveFilePcmFormat
												| CreateWaveFileTemp, NULL))
	{
		MessageBoxSync(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);

		pDocument->m_bCloseThisDocumentNow = true;
		return FALSE;
	}

	pDocument->m_OriginalWaveFormat = m_Decoder.GetDstFormat();
	SetDstFile(pDocument->m_WavFile);

	pDocument->SoundChanged(pDocument->WaveFileID(), 0, 0, m_CurrentSamples);

	pDocument->QueueSoundUpdate(pDocument->UpdateWholeFileChanged,
								pDocument->WaveFileID(), 0, 0, m_CurrentSamples);

	pDocument->m_WavFile.LoadPeaksForCompressedFile(pDocument->m_OriginalWavFile, m_CurrentSamples);

	if (S_OK == m_Decoder.Start())
	{
		return TRUE;
	}
	m_CoInit.UninitializeCom();
	return FALSE;
}

void CWmaDecodeContext::DeInit()
{
	m_Decoder.Stop();
	m_Decoder.DeInit();
	m_CoInit.UninitializeCom();
}

// TODO: see if m_Dst members are not needed
void CWmaDecodeContext::SetDstFile(CWaveFile & file)
{
	m_DstFile = file;
	m_DstStart = m_DstFile.SampleToPosition(0);
	m_DstCopyPos = m_DstStart;
	m_DstCopySample = 0;

	m_Decoder.SetDstFile(file);
}

void CWmaDecodeContext::PostRetire(BOOL bChildContext)
{
	if (0
		//&& m_MmResult != MMSYSERR_NOERROR
		)
	{
		CString s;
		if (m_Flags & OperationContextInitFailed)
		{
			s.Format(IDS_CANT_DECOMPRESS_FILE, LPCTSTR(pDocument->m_OriginalWavFile.GetName()),
					-1, 0);
			pDocument->m_bCloseThisDocumentNow = true;
		}
		else
		{
			s.Format(IDS_ERROR_WHILE_DECOMPRESSING_FILE,
					LPCTSTR(pDocument->m_OriginalWavFile.GetName()), 0);
			AfxMessageBox(s, MB_ICONSTOP);
		}
	}
	else if (m_Flags & DecompressSavePeakFile)
	{
		m_DstFile.SavePeakInfo(pDocument->m_OriginalWavFile);
	}
	COperationContext::PostRetire(bChildContext);
}

BOOL CWmaSaveContext::Init()
{
	SetBeginTime();
	m_Enc.m_SrcWfx = * m_SrcFile.GetWaveFormat();

	m_CoInit.InitializeCom(COINIT_APARTMENTTHREADED);

	if (! m_Enc.Init())
	{
		return FALSE;
	}

	m_Enc.SetFormat(m_DstFile.GetWaveFormat());
	if ( ! m_Enc.OpenWrite(m_DstFile))
	{
		return FALSE;
	}
	return TRUE;
}

void CWmaSaveContext::DeInit()
{
	m_Enc.DeInit();
	m_CoInit.UninitializeCom();
	m_DstCopyPos = m_Enc.GetWrittenLength();
}

void CWmaSaveContext::PostRetire(BOOL bChildContext)
{
	PrintElapsedTime();
	CConversionContext::PostRetire(bChildContext);
}

BOOL CWmaSaveContext::ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward)
{
	return m_Enc.Write(buf, len);
}

BOOL CWmaSaveContext::OperationProc()
{
	// generic procedure working on one file
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_SrcCopyPos >= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD dwStartTime = timeGetTime();
	SAMPLE_POSITION dwOperationBegin = m_DstCopyPos;
	int SampleSize = m_SrcFile.SampleSize();

	LONG SizeToProcess = 0;
	LONG WasLockedToWrite = 0;
	void * pSrcBuf;

	WAVE_SAMPLE TempBuf[4];
	do
	{
		// make sure ProcessBuffer gets integer number of complete samples, from sample boundary

		LONGLONG SizeToWrite = m_SrcEnd - m_SrcCopyPos;
		WasLockedToWrite = m_SrcFile.GetDataBuffer( & pSrcBuf,
													SizeToWrite, m_SrcCopyPos, m_GetBufferFlags);

		if (0 == WasLockedToWrite)
		{
			return FALSE;
		}

		if (WasLockedToWrite < SampleSize)
		{
			m_SrcFile.ReturnDataBuffer(pSrcBuf, WasLockedToWrite, 0);
			if (SampleSize != m_SrcFile.ReadAt(TempBuf, SampleSize, m_SrcCopyPos))
			{
				return FALSE;
			}
			ProcessBuffer(TempBuf, SampleSize, m_SrcCopyPos - m_SrcStart, FALSE);
			m_SrcCopyPos += SampleSize;
			continue;
		}
		SizeToProcess = WasLockedToWrite - WasLockedToWrite % SampleSize;
		// save the data to be changed to undo buffer, but only on the first forward pass
		// virtual function which modifies the actual data:
		ProcessBuffer(pSrcBuf, SizeToProcess, m_SrcCopyPos - m_SrcStart, FALSE);

		m_SrcFile.ReturnDataBuffer(pSrcBuf, WasLockedToWrite, 0);
		m_SrcCopyPos += SizeToProcess;
	}
	while (m_SrcCopyPos < m_SrcEnd
			&& timeGetTime() - dwStartTime < 1000);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = int(100. * (m_SrcCopyPos - m_SrcStart)
								/ double(m_SrcEnd - m_SrcStart));
	}
	return TRUE;
}
