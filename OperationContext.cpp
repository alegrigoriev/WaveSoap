// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext.cpp
#include "stdafx.h"
#include "OperationContext.h"
#include "OperationDialogs.h"
#include "ReopenSavedFileCopyDlg.h"
#include "MessageBoxSynch.h"
#include "DialogWithSelection.inl"
#include "WaveSoapFrontDoc.h"
#include <algorithm>

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
	m_BeginSystemTime = GetTickCount();
}

void COperationContext::PrintElapsedTime()
{
	FILETIME tmp, UserTime;
	GetThreadTimes(GetCurrentThread(),
					& tmp, & tmp, & tmp, & UserTime);
	DWORD TickCount = GetTickCount();
	TRACE("Elapsed thread time : %d ms, elapsed real time=%d\n",
		(UserTime.dwLowDateTime - m_ThreadUserTime.dwLowDateTime) / 10000,
		TickCount - m_BeginSystemTime);
}

#endif

COperationContext::COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, DWORD Flags, LPCTSTR OperationName)
	: pDocument(pDoc),
	m_Flags(Flags),
	m_OperationName(OperationName),
	sOp(StatusString),
	m_PercentCompleted(0),
	m_bClipped(false),
	m_MaxClipped(0.),
	m_GetBufferFlags(CDirectFile::GetBufferAndPrefetchNext),
	m_ReturnBufferFlags(0)
{
}

COperationContext::~COperationContext()
{
	DeleteUndo();
}

CString COperationContext::GetStatusString()
{
	return sOp;
}

void COperationContext::DeleteUndo()
{
	while ( ! m_UndoChain.IsEmpty())
	{
		delete m_UndoChain.RemoveTail();
	}
}

void COperationContext::UpdateCompletedPercent(SAMPLE_INDEX CurrentSample,
												SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample)
{
	if (EndSample != StartSample)
	{
		m_PercentCompleted = MulDiv(100, CurrentSample - StartSample,
									EndSample - StartSample);
	}
}

void COperationContext::UpdateCompletedPercent(SAMPLE_POSITION CurrentPos,
												SAMPLE_POSITION StartPos, SAMPLE_POSITION EndPos)
{
	if (EndPos != StartPos)
	{
		m_PercentCompleted = int(100 * (MEDIA_FILE_SIZE(CurrentPos) - StartPos)
								/ (MEDIA_FILE_SIZE(EndPos) - StartPos));
	}
}

void COperationContext::Retire()
{
	// all context go to retirement list
	// queue it to the Doc
	PrintElapsedTime();

	pDocument->m_RetiredList.InsertTail(this);
}

void COperationContext::PostRetire()
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
	CUndoRedoContext * pUndo = GetUndo();
	if (pUndo)
	{
		pDocument->AddUndoRedo(pUndo);
	}

	delete this;
}

void COperationContext::Execute()
{
	SetBeginTime();
	pDocument->QueueOperation(this);
}

void COperationContext::ExecuteSynch()
{
	class SynchExec: public MainThreadCall
	{
	public:
		SynchExec(COperationContext * pContext)
			: m_pContext(pContext)
		{
		}
	protected:
		virtual LRESULT Exec()
		{
			LRESULT result = m_pContext->Init();
			if (result)
			{
				result = m_pContext->OperationProc();
			}
			m_pContext->DeInit();
			return result;
		}
		COperationContext * const m_pContext;
	} call(this);

	call.Call();
}

CUndoRedoContext * COperationContext::GetUndo()
{
	ListHead<COperationContext> * pUndoChain =
		GetUndoChain();

	if (NULL == pUndoChain
		|| pUndoChain->IsEmpty())
	{
		return NULL;
	}

	CUndoRedoContext::auto_ptr pUndo(new CUndoRedoContext(pDocument,
														m_OperationName));

	while ( ! pUndoChain->IsEmpty())
	{
		pUndo->AddContext(pUndoChain->RemoveHead());
	}

	return pUndo.release();
}

BOOL COperationContext::CreateUndo()
{
	return TRUE;
}

//////////// COneFileOperation
COneFileOperation::COneFileOperation(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
									ULONG Flags, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, Flags, OperationName)
	, m_SrcChan(ALL_CHANNELS)
	, m_SrcStart(0)
	, m_SrcEnd(0)
	, m_SrcPos(0)
{
}

bool COneFileOperation::KeepsPermanentFileReference() const
{
	return m_SrcFile.IsOpen()
			&& ! m_SrcFile.IsTemporaryFile();
}

LONGLONG COneFileOperation::GetTempDataSize() const
{
	if (m_SrcFile.IsOpen()
		&& m_SrcFile.IsTemporaryFile())
	{
		return m_SrcFile.GetLength();
	}
	return 0;
}

void COneFileOperation::InitSource(CWaveFile & SrcFile, SAMPLE_INDEX StartSample,
									SAMPLE_INDEX EndSample, CHANNEL_MASK SrcChannel)
{
	m_SrcFile = SrcFile;

	m_SrcChan = SrcChannel;

	m_SrcStart = m_SrcFile.SampleToPosition(StartSample);
	m_SrcPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.SampleToPosition(EndSample);

	TRACE("SrcStart=%X, SrcEnd=%X, SrcPos=%X\n",
		m_SrcStart, m_SrcEnd, m_SrcPos);
}

void COneFileOperation::UpdateCompletedPercent()
{
	UpdateCompletedPercent(m_SrcPos, m_SrcStart, m_SrcEnd);
}

MEDIA_FILE_SIZE COneFileOperation::GetTotalOperationSize() const
{
	if (m_SrcStart <= m_SrcEnd)
	{
		return m_SrcEnd - m_SrcStart;
	}
	else
	{
		return m_SrcStart - m_SrcEnd;
	}
}

MEDIA_FILE_SIZE COneFileOperation::GetCompletedOperationSize() const
{
	if (m_SrcStart <= m_SrcEnd)
	{
		return m_SrcPos - m_SrcStart;
	}
	else
	{
		return m_SrcStart - m_SrcPos;
	}
}

//////////// CTwoFilesOperation
CTwoFilesOperation::CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
										ULONG Flags, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, Flags, OperationName)
	, m_DstChan(ALL_CHANNELS)
	, m_DstStart(0)
	, m_DstEnd(0)
	, m_DstPos(0)
	, m_pUndoContext(NULL)
	, m_UndoStartPos(0)
	, m_UndoEndPos(0)
{
}

CTwoFilesOperation::~CTwoFilesOperation()
{
	delete m_pUndoContext;
}

LONGLONG CTwoFilesOperation::GetTempDataSize() const
{
	LONGLONG sum = BaseClass::GetTempDataSize();

	if (m_DstFile.IsOpen()
		&& m_DstFile.IsTemporaryFile()
		&& m_DstFile.GetFileID() != m_SrcFile.GetFileID())
	{
		sum += m_DstFile.GetLength();
	}
	return sum;
}

BOOL CTwoFilesOperation::InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
										CHANNEL_MASK chan, BOOL NeedUndo,
										SAMPLE_INDEX StartUndoSample, SAMPLE_INDEX EndUndoSample)
{
	m_DstFile = DstFile;
	// set begin and end offsets

	m_DstPos = m_DstFile.SampleToPosition(StartSample);
	m_DstStart = m_DstPos;

	m_DstEnd = m_DstFile.SampleToPosition(EndSample);

	m_DstChan = chan;

	TRACE("DstStart=%X, DstEnd=%X, DstPos=%X\n",
		m_DstStart, m_DstEnd, m_DstPos);

	// create undo
	if (NeedUndo)
	{
		if (0 == StartUndoSample
			&& LAST_SAMPLE == EndUndoSample)
		{
			StartUndoSample = StartSample;
			EndUndoSample = EndSample;
		}

		SetSaveForUndo(StartUndoSample, EndUndoSample);
		return CreateUndo();
	}
	else
	{
		m_UndoStartPos = 0;
		m_UndoEndPos = 0;
	}
	return TRUE;
}

BOOL CTwoFilesOperation::CreateUndo()
{
	if (NULL != m_pUndoContext
		|| ! m_DstFile.IsOpen()
		|| m_DstFile.GetFileID() != pDocument->WaveFileID()
		|| m_UndoStartPos == m_UndoEndPos)
	{
		return TRUE;
	}

	CCopyUndoContext::auto_ptr pUndo(new CCopyUndoContext(pDocument, _T(""), m_OperationName));

	if ( ! pUndo->InitUndoCopy(m_DstFile, m_UndoStartPos, m_UndoEndPos, m_DstChan))
	{
		return FALSE;
	}

	m_pUndoContext = pUndo.release();

	return TRUE;
}

ListHead<COperationContext> * CTwoFilesOperation::GetUndoChain()
{
	if (NULL != m_pUndoContext)
	{
		m_pUndoContext->m_SrcEnd = m_pUndoContext->m_SrcPos;
		m_pUndoContext->m_DstEnd = m_pUndoContext->m_DstPos;

		m_UndoChain.InsertHead(m_pUndoContext);

		m_pUndoContext = NULL;
	}

	return BaseClass::GetUndoChain();
}

void CTwoFilesOperation::DeleteUndo()
{
	delete m_pUndoContext;
	m_pUndoContext = NULL;
}

void CTwoFilesOperation::SetSaveForUndo(SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample)
{
	m_UndoStartPos = m_DstFile.SampleToPosition(StartSample);
	m_UndoEndPos = m_DstFile.SampleToPosition(EndSample);
}

void CTwoFilesOperation::DeInit()
{
	// adjust undo boundaries
	if (m_DstStart <= m_DstEnd
		&& m_UndoStartPos != m_UndoEndPos)
	{
		if (m_UndoStartPos < m_DstPos)
		{
			m_UndoStartPos = min(m_DstPos, m_UndoEndPos);
		}
	}

	BaseClass::DeInit();
}

/////////// CThroughProcessOperation
CThroughProcessOperation::CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
													ULONG Flags, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, Flags, OperationName)
	, m_NumberOfForwardPasses(1)
	, m_NumberOfBackwardPasses(0)
	, m_CurrentPass(1)
{
}

BOOL CThroughProcessOperation::OperationProc()
{
	// generic procedure working on one file
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	BOOL res = TRUE;    // function result
	DWORD dwStartTime = GetTickCount();
	SAMPLE_POSITION dwOperationBegin = m_DstPos;
	int SampleSize = m_DstFile.SampleSize();

	LONG SizeToProcess = 0;
	LONG WasLockedToWrite = 0;
	void * pDstBuf;
	if (m_CurrentPass > 0)
	{
		if (m_DstPos >= m_DstEnd)
		{
			m_CurrentPass++;
			m_DstPos = m_DstStart;
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
			m_DstPos = m_DstEnd;
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

	WAVE_SAMPLE TempBuf[MAX_NUMBER_OF_CHANNELS];

	if (m_CurrentPass > 0)
	{
		ASSERT(0 == ((m_DstEnd - m_DstPos) % SampleSize));
		do
		{
			// make sure ProcessBuffer gets integer number of complete samples, from sample boundary

			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstEnd) - MEDIA_FILE_SIZE(m_DstPos);
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pDstBuf,
														SizeToWrite, m_DstPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				return FALSE;
			}

			if (WasLockedToWrite < SampleSize)
			{
				m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite, 0);

				ASSERT(SampleSize <= sizeof TempBuf);

				if (SampleSize != m_DstFile.ReadAt(TempBuf, SampleSize, m_DstPos))
				{
					return FALSE;
				}

				if (1 == m_CurrentPass
					&& NULL != m_pUndoContext)
				{
					m_pUndoContext->SaveUndoData(TempBuf, SampleSize,
												m_DstPos, m_DstFile.Channels());
				}

				res = ProcessBuffer(TempBuf, SampleSize, m_DstPos - m_DstStart, FALSE);

				if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
				{
					m_DstFile.WriteAt(TempBuf, SampleSize, m_DstPos);
				}
				m_DstPos += SampleSize;
				continue;
			}

			SizeToProcess = WasLockedToWrite - WasLockedToWrite % SampleSize;
			// save the data to be changed to undo buffer, but only on the first forward pass
			if (1 == m_CurrentPass
				&& NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, SizeToProcess,
											m_DstPos, m_DstFile.Channels());
			}
			// virtual function which modifies the actual data:
			res = ProcessBuffer(pDstBuf, SizeToProcess, m_DstPos - m_DstStart, FALSE);

			m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite,
										m_ReturnBufferFlags);
			m_DstPos += SizeToProcess;
		}
		while (res && m_DstPos < m_DstEnd
				&& GetTickCount() - dwStartTime < 200);

		if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
		{
			// notify the view
			pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);
		}

		if (m_DstEnd > m_DstStart)
		{
			m_PercentCompleted = int(100. * (m_DstPos + double(m_DstEnd - m_DstStart) * (m_CurrentPass - 1) - m_DstStart)
									/ (double(m_DstEnd - m_DstStart) * (m_NumberOfForwardPasses + m_NumberOfBackwardPasses)));
		}
	}
	else
	{
		// backward pass
		if (m_DstPos <= m_DstStart)
		{
			m_DstPos = m_DstEnd;
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

		ASSERT(0 == ((m_DstPos - m_DstStart) % SampleSize));
		do
		{
			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstStart) - MEDIA_FILE_SIZE(m_DstPos);

			// length requested and length returned are <0,
			// pointer returned points on the end of the buffer
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pDstBuf,
														SizeToWrite, m_DstPos, m_GetBufferFlags);

			if (0 == WasLockedToWrite)
			{
				return FALSE;
			}

			if (-WasLockedToWrite < SampleSize)
			{
				m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite, 0);

				ASSERT(SampleSize <= sizeof TempBuf);
				if (SampleSize != m_DstFile.ReadAt(TempBuf, SampleSize, m_DstPos - SampleSize))
				{
					return FALSE;
				}

				// save the data to be changed to undo buffer, but only on the first forward pass
				if (1 == m_CurrentPass
					&& NULL != m_pUndoContext)
				{
					m_pUndoContext->SaveUndoData(TempBuf + m_DstFile.Channels(),
												SampleSize,
												m_DstPos, m_DstFile.Channels());
				}

				res = ProcessBuffer(TempBuf, SampleSize, m_DstPos - m_DstStart, TRUE);

				if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
				{
					m_DstFile.WriteAt(TempBuf, SampleSize, m_DstPos - SampleSize);
				}

				m_DstPos -= SampleSize;
				continue;
			}

			SizeToProcess = -WasLockedToWrite - (-WasLockedToWrite) % SampleSize;
			// save the data to be changed to undo buffer, but only on the first forward pass
			if (1 == m_CurrentPass
				&& NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, SizeToProcess,
											m_DstPos, m_DstFile.Channels());
			}
			// virtual function which modifies the actual data:
			res = ProcessBuffer(-SizeToProcess + (PCHAR)pDstBuf, SizeToProcess, m_DstPos - m_DstStart, TRUE);   // backward=TRUE

			m_DstFile.ReturnDataBuffer(pDstBuf, WasLockedToWrite,
										m_ReturnBufferFlags);
			// length requested and length returned are <0,
			m_DstPos -= SizeToProcess;
		}
		while (res && m_DstPos > m_DstStart
				&& GetTickCount() - dwStartTime < 200);

		if (m_ReturnBufferFlags & CDirectFile::ReturnBufferDirty)
		{
			// notify the view
			pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);
		}

		if (m_DstEnd > m_DstStart)
		{
			m_PercentCompleted = int(100. * (m_DstEnd + double(m_DstEnd - m_DstStart)
										* (- 1 - m_CurrentPass + m_NumberOfForwardPasses) - m_DstPos)
									/ ((double(m_DstEnd) - m_DstStart) * (m_NumberOfForwardPasses + m_NumberOfBackwardPasses)));
		}
	}
	return res;
}

MEDIA_FILE_SIZE CThroughProcessOperation::GetTotalOperationSize() const
{
	return (m_NumberOfForwardPasses + m_NumberOfBackwardPasses)
		* BaseClass::GetTotalOperationSize();
}

MEDIA_FILE_SIZE CThroughProcessOperation::GetCompletedOperationSize() const
{
	return m_CurrentPass * BaseClass::GetTotalOperationSize()
			+ BaseClass::GetCompletedOperationSize();
}

BOOL CThroughProcessOperation::Init()
{
	return InitPass(1);
}

BOOL CThroughProcessOperation::InitPass(int /*nPass*/)
{
	return TRUE;
}

/////////////// CStagedContext ///////////////
CStagedContext::CStagedContext(CWaveSoapFrontDoc * pDoc,
								LPCTSTR StatusString, DWORD Flags, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, Flags, OperationName)
	, m_DoneSize(0)
{
}

CStagedContext::~CStagedContext()
{
	while( ! m_ContextList.IsEmpty())
	{
		delete m_ContextList.RemoveHead();
	}

	while( ! m_DoneList.IsEmpty())
	{
		delete m_DoneList.RemoveHead();
	}
}

LONGLONG CStagedContext::GetTempDataSize() const
{
	LONGLONG sum = 0;
	for (COperationContext * pContext = m_ContextList.First();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Next(pContext))
	{
		sum += pContext->GetTempDataSize();
	}
	return sum;
}

MEDIA_FILE_SIZE CStagedContext::GetTotalOperationSize() const
{
	MEDIA_FILE_SIZE sum = m_DoneSize;

	for (COperationContext * pContext = m_ContextList.First();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Next(pContext))
	{
		sum += pContext->GetTotalOperationSize();
	}
	return sum;
}

MEDIA_FILE_SIZE CStagedContext::GetCompletedOperationSize() const
{
	MEDIA_FILE_SIZE sum = m_DoneSize;

	if ( ! m_ContextList.IsEmpty())
	{
		sum += m_ContextList.First()->GetCompletedOperationSize();
	}

	return sum;
}

bool CStagedContext::KeepsPermanentFileReference() const
{
	for (COperationContext * pContext = m_ContextList.First();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Next(pContext))
	{
		if (pContext->KeepsPermanentFileReference())
		{
			return true;
		}
	}
	return false;
}

BOOL CStagedContext::CreateUndo()
{
	for (COperationContext * pContext = m_ContextList.First();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Next(pContext))
	{
		if ( ! pContext->CreateUndo())
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CStagedContext::PrepareUndo()
{
	for (COperationContext * pContext = m_ContextList.First();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Next(pContext))
	{
		if ( ! pContext->PrepareUndo())
		{
			return FALSE;
		}
	}
	return TRUE;
}

void CStagedContext::UnprepareUndo()
{
	for (COperationContext * pContext = m_ContextList.Last();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Prev(pContext))
	{
		pContext->UnprepareUndo();
	}
}

void CStagedContext::DeleteUndo()
{
	for (COperationContext * pContext = m_ContextList.Last();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Prev(pContext))
	{
		pContext->DeleteUndo();
	}
}

BOOL CStagedContext::OperationProc()
{
	if (m_ContextList.IsEmpty())
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	COperationContext * pContext = m_ContextList.First();
	BOOL result = TRUE;
	// if a context should be executed synchronously
	if (pContext->m_Flags & OperationContextSynchronous)
	{
		pContext->ExecuteSynch();
		pContext->m_Flags |= OperationContextFinished;

		m_DoneList.InsertTail(m_ContextList.RemoveHead());

		return TRUE;
	}
	else if (0 == (pContext->m_Flags & OperationContextInitialized))
	{
		if ( ! pContext->Init())
		{
			pContext->m_Flags |= OperationContextInitFailed | OperationContextStop;
			result = FALSE;
		}
		pContext->m_Flags |= OperationContextInitialized;
	}

	if (pDocument->m_StopOperation)
	{
		pContext->m_Flags |= OperationContextStopRequested;
	}

	if ( 0 == (pContext->m_Flags & (OperationContextStop | OperationContextFinished)))
	{
		if ( ! pContext->OperationProc())
		{
			pContext->m_Flags |= OperationContextStop;
			result = FALSE;
		}
		m_PercentCompleted = pContext->m_PercentCompleted;
	}

	if (0 != (pContext->m_Flags & (OperationContextStop | OperationContextFinished)))
	{
		COperationContext * pDone = m_ContextList.RemoveHead();
		m_DoneSize += pDone->GetTotalOperationSize();

		pDone->DeInit();
		m_DoneList.InsertTail(pDone);
	}

	return result;
}

void CStagedContext::PostRetire()
{
	// collect all undo before done list is emptied
	// because the current context in m_ContextList can be partially done,
	// need to gather undo from it, too
	GetUndoChain();

	while( ! m_ContextList.IsEmpty())
	{
		delete m_ContextList.RemoveHead();
	}


	while( ! m_DoneList.IsEmpty())
	{
		COperationContext * pContext = m_DoneList.RemoveHead();
		pContext->PostRetire();
	}

	BaseClass::PostRetire();
}

CString CStagedContext::GetStatusString()
{
	if (m_ContextList.IsEmpty())
	{
		return BaseClass::GetStatusString();
	}
	return m_ContextList.First()->GetStatusString();
}

void CStagedContext::AddContext(COperationContext * pContext)
{
	m_ContextList.InsertTail(pContext);
}

void CStagedContext::AddContextInFront(COperationContext * pContext)
{
	m_ContextList.InsertHead(pContext);
}

void CStagedContext::Execute()
{
	// if any of
	m_Flags |= OperationContextNonCritical;

	for (COperationContext * pContext = m_ContextList.First();
		m_ContextList.NotEnd(pContext);
		pContext = m_ContextList.Next(pContext))
	{
		// OperationContextDiskIntensive and OperationContextClipboard
		// are set if set in any context
		m_Flags |= pContext->m_Flags
					& (OperationContextDiskIntensive | OperationContextClipboard);
		// OperationContextNonCritical is set if set in all contexts
		m_Flags &= pContext->m_Flags | ~OperationContextNonCritical;
	}

	BaseClass::Execute();
}

ListHead<COperationContext> * CStagedContext::GetUndoChain()
{
	// because the current context in m_ContextList can be partially done,
	// need to gather undo from it, too
	if ( ! m_ContextList.IsEmpty())
	{
		ListHead<COperationContext> * pUndo =
			m_ContextList.First()->GetUndoChain();

		if (NULL != pUndo)
		{
			while ( ! pUndo->IsEmpty())
			{
				m_UndoChain.InsertTail(pUndo->RemoveHead());
			}
		}
	}

	for (COperationContext * pContext = m_DoneList.Last();
		m_DoneList.NotEnd(pContext);
		pContext = m_DoneList.Prev(pContext))
	{
		ListHead<COperationContext> * pUndo = pContext->GetUndoChain();
		if (NULL != pUndo)
		{
			while ( ! pUndo->IsEmpty())
			{
				m_UndoChain.InsertTail(pUndo->RemoveHead());
			}
		}
	}

	return & m_UndoChain;
}

///////////////// CScanPeaksContext
CScanPeaksContext::CScanPeaksContext(CWaveSoapFrontDoc * pDoc,
									CWaveFile & WavFile,
									CWaveFile & OriginalFile,
									BOOL bSavePeaks)
	: BaseClass(pDoc, _T("Scanning the file for peaks..."), OperationContextDiskIntensive, _T("Peak Scan"))
	, m_GranuleSize(WavFile.SampleSize() * WavFile.GetPeakGranularity())
	, m_bSavePeakFile(bSavePeaks)
{
	WavFile.SetPeaks(0, WavFile.NumberOfSamples() * WavFile.Channels(),
					1, WavePeak(0x7FFF, -0x8000));

	m_OriginalFile = OriginalFile;
	m_SrcFile = WavFile;
	m_SrcStart = WavFile.SampleToPosition(0);
	m_SrcPos = m_SrcStart;

	m_SrcEnd = WavFile.SampleToPosition(LAST_SAMPLE);
}

BOOL CScanPeaksContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_SrcPos >= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		m_PercentCompleted = 100;
		return TRUE;
	}

	DWORD dwStartTime = GetTickCount();

	SAMPLE_POSITION dwOperationBegin = m_SrcPos;
	do
	{
		MEDIA_FILE_SIZE SizeToRead = m_SrcEnd - m_SrcPos;

		void * pBuf;
		long lRead = m_SrcFile.GetDataBuffer( & pBuf,
											SizeToRead, m_SrcPos, CDirectFile::GetBufferAndPrefetchNext);

		if (lRead > 0)
		{
			unsigned i;
			ULONG DataToProcess = lRead;
			WAVE_SAMPLE * pWaveData = (WAVE_SAMPLE *) pBuf;

			DWORD DataOffset = m_SrcPos - m_SrcStart;
			unsigned DataForGranule = m_GranuleSize - DataOffset % m_GranuleSize;

			if (2 == m_SrcFile.Channels())
			{
				unsigned PeakIndex = (DataOffset / m_GranuleSize) * 2;

				while (0 != DataToProcess)
				{
					WavePeak peak = m_SrcFile.GetPeakMinMax(PeakIndex, PeakIndex + 1);
					int wpl_l = peak.low;
					int wpl_h = peak.high;

					peak = m_SrcFile.GetPeakMinMax(PeakIndex + 1, PeakIndex + 2);
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

					m_SrcFile.SetPeakData(PeakIndex,
										WAVE_SAMPLE(wpl_l), WAVE_SAMPLE(wpl_h));
					m_SrcFile.SetPeakData(PeakIndex + 1,
										WAVE_SAMPLE(wpr_l), WAVE_SAMPLE(wpr_h));

					PeakIndex += 2;
					DataForGranule = m_GranuleSize;
				}
			}
			else
			{
				unsigned PeakIndex = DataOffset / m_GranuleSize;
				while (0 != DataToProcess)
				{
					WavePeak peak = m_SrcFile.GetPeakMinMax(PeakIndex, PeakIndex + 1);
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

					m_SrcFile.SetPeakData(PeakIndex,
										WAVE_SAMPLE(wp_l), WAVE_SAMPLE(wp_h));

					PeakIndex ++;
					DataForGranule = m_GranuleSize;
				}
			}

			m_SrcPos += lRead;
			m_SrcFile.ReturnDataBuffer(pBuf, lRead, 0);
		}
		else
		{
			m_Flags |= OperationContextStop;
			break;
		}
	}
	while (m_SrcPos < m_SrcEnd
			&& GetTickCount() - dwStartTime < 200);

	TRACE("CScanPeaksContext current position=%X\n", m_SrcPos);

	UpdateCompletedPercent();

	// notify the view
	pDocument->FileChanged(m_SrcFile, dwOperationBegin,
							m_SrcPos, -1, UpdateSoundDontRescanPeaks);

	return TRUE;
}

void CScanPeaksContext::PostRetire()
{
	if (m_bSavePeakFile
		&& (m_Flags & OperationContextFinished)
		&& m_OriginalFile.IsOpen())
	{
		m_SrcFile.SavePeakInfo(m_OriginalFile);
	}
	BaseClass::PostRetire();
}

BOOL CScanPeaksContext::Init()
{
	if ( ! m_SrcFile.AllocatePeakData(m_SrcFile.NumberOfSamples()))
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	return TRUE;
}

///////// CCopyContext
// is called for a CCopyContext which is actually Undo/Redo context
// copies data from one file to another, with possible changing number of channels
CCopyContext::CCopyContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName)
{
}

BOOL CCopyContext::InitCopy(CWaveFile & DstFile,
							SAMPLE_INDEX DstStartSample, CHANNEL_MASK DstChannel,
							CWaveFile & SrcFile,
							SAMPLE_INDEX SrcStartSample, CHANNEL_MASK SrcChannel,
							NUMBER_OF_SAMPLES SrcDstLength)
{
	InitSource(SrcFile, SrcStartSample, SrcStartSample + SrcDstLength, SrcChannel);

	return InitDestination(DstFile, DstStartSample,
							DstStartSample + SrcDstLength, DstChannel, FALSE);
}

void CCopyContext::DeInit()
{
	BaseClass::DeInit();

	m_SrcStart = m_SrcPos;
	m_DstStart = m_DstPos;
}

// copy the actual data, while probably changing number of channels
BOOL CCopyContext::OperationProc()
{
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	if (m_SrcPos >= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	if (m_DstPos > m_DstEnd)
	{
		return FALSE;
	}

	DWORD dwStartTime = GetTickCount();
	SAMPLE_POSITION dwOperationBegin = m_DstPos;

	long LeftToRead = 0;
	long LeftToWrite = 0;
	long WasRead = 0;
	long WasLockedToWrite = 0;
	void * pOriginalSrcBuf = NULL;
	char * pSrcBuf = NULL;
	void * pOriginalDstBuf = NULL;
	char * pDstBuf = NULL;

	DWORD DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;
	WAVE_SAMPLE tmp[MAX_NUMBER_OF_CHANNELS];
	int const SrcSampleSize = m_SrcFile.SampleSize();
	int const DstSampleSize = m_DstFile.SampleSize();
	NUMBER_OF_CHANNELS const NumSrcChannels = m_SrcFile.Channels();
	NUMBER_OF_CHANNELS const NumDstChannels = m_DstFile.Channels();

	do
	{
		if (0 == LeftToRead)
		{
			MEDIA_FILE_SIZE SizeToRead = MEDIA_FILE_SIZE(m_SrcEnd) - MEDIA_FILE_SIZE(m_SrcPos);
			if (SizeToRead > CDirectFile::CacheBufferSize())
			{
				SizeToRead = CDirectFile::CacheBufferSize();
			}
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

		if (0 == LeftToWrite)
		{
			DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;

			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstEnd) - MEDIA_FILE_SIZE(m_DstPos);
			if (SizeToWrite > CDirectFile::CacheBufferSize())
			{
				SizeToWrite = CDirectFile::CacheBufferSize();
			}

			if (m_DstFile.AllChannels(m_DstChan)
				&& m_SrcFile.GetFileID() != m_DstFile.GetFileID()
				&& (NULL == m_pUndoContext
					|| ! m_pUndoContext->NeedToSaveUndo(m_DstPos, long(SizeToWrite))))
			{
				DstFileFlags = CDirectFile::GetBufferWriteOnly;
			}

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstPos, DstFileFlags);

			if (0 == WasLockedToWrite)
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;

		}

		unsigned SrcSamples = LeftToRead / SrcSampleSize;
		unsigned DstSamples = LeftToWrite / DstSampleSize;

		if (SrcSamples != 0
			&& DstSamples != 0)
		{
			unsigned Samples = std::min(SrcSamples, DstSamples);

			// save the changed data to undo buffer
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf,
											Samples * DstSampleSize, m_DstPos, NumDstChannels);
			}

			CopyWaveSamples(pDstBuf, m_DstChan, NumDstChannels,
							pSrcBuf, m_SrcChan, NumSrcChannels,
							Samples, m_DstFile.GetSampleType(), m_SrcFile.GetSampleType());

			unsigned DstCopied = Samples * DstSampleSize;
			pDstBuf += DstCopied;
			LeftToWrite -= DstCopied;
			m_DstPos += DstCopied;

			unsigned SrcCopied = Samples * SrcSampleSize;
			pSrcBuf += SrcCopied;
			LeftToRead -= SrcCopied;
			m_SrcPos += SrcCopied;
		}
		else
		{
			// read one sample directly
			// save the changed data to undo buffer
			if (NULL != m_pUndoContext
				&& m_pUndoContext->NeedToSaveUndo(m_DstPos, DstSampleSize))
			{
				m_DstFile.ReadSamples(ALL_CHANNELS,
									m_DstPos, 1, tmp, m_DstFile.GetSampleType());

				m_pUndoContext->SaveUndoData(tmp,
											DstSampleSize, m_DstPos, NumDstChannels);
			}

			if (1 != m_SrcFile.ReadSamples(ALL_CHANNELS,
											m_SrcPos, 1, tmp, m_SrcFile.GetSampleType())
				|| 1 != m_DstFile.WriteSamples(m_DstChan, m_DstPos, 1,
												tmp, m_SrcChan, NumSrcChannels, m_SrcFile.GetSampleType()))
			{
				// error
				TRACE("Transfering a split sample was unsuccessful!\n");
				m_Flags |= OperationContextFinished;
				break;
			}

			pSrcBuf += SrcSampleSize;
			m_SrcPos += SrcSampleSize;
			if (LeftToRead > SrcSampleSize)
			{
				LeftToRead -= SrcSampleSize;
			}
			else
			{
				LeftToRead = 0;
			}

			pDstBuf += DstSampleSize;
			m_DstPos += DstSampleSize;
			if (LeftToWrite > DstSampleSize)
			{
				LeftToWrite -= DstSampleSize;
			}
			else
			{
				LeftToWrite = 0;
			}
		}

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
			|| (m_SrcPos < m_SrcEnd
				&& GetTickCount() - dwStartTime < 200)
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	// notify the view
	pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);

	UpdateCompletedPercent();

	return TRUE;
}

////////////// CCopyUndoContext

BOOL CCopyUndoContext::PrepareUndo()
{
	m_Flags &= ~(OperationContextStop | OperationContextFinished);
	m_DstFile = pDocument->m_WavFile;
	m_SrcPos = m_SrcStart;
	m_DstPos = m_DstStart;
	return TRUE;
}

void CCopyUndoContext::UnprepareUndo()
{
	m_DstFile.Close();
}

// check if any part of the (Position, Position + length) range
// is inside range to be saved
BOOL CCopyUndoContext::NeedToSaveUndo(SAMPLE_POSITION Position, long length)
{
	if (length < 0)
	{
		if (Position > unsigned long(-length))
		{
			Position += length;
			length = -length;
		}
		else
		{
			length = Position;
			Position = 0;
		}
	}

	if (m_DstEnd >= m_DstStart)
	{
		ASSERT(m_DstPos <= m_DstEnd);

		if (Position >= m_DstEnd
			|| Position + length <= m_DstPos
			|| length == 0)
		{
			return FALSE;
		}
		return TRUE;
	}
	else
	{
		ASSERT(m_DstPos >= m_DstEnd);

		if (Position >= m_DstPos
			|| Position + length <= m_DstEnd)
		{
			return FALSE;
		}
		return TRUE;
	}
}

// init pointers and allocate a file to save the undo information
// is called for a UNDO context when it's created
BOOL CCopyUndoContext::InitUndoCopy(CWaveFile & SrcFile,
									SAMPLE_POSITION SaveStartPos, // source file position of data needed to save and restore
									SAMPLE_POSITION SaveEndPos,
									CHANNEL_MASK SaveChannel)
{
	ASSERT(SrcFile.IsOpen());

	m_DstStart = SaveStartPos;
	m_DstPos = SaveStartPos;
	m_DstEnd = SaveEndPos;

	ASSERT(SaveEndPos >= SaveStartPos);

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

		m_SrcStart = m_SrcFile.SampleToPosition(0);
		m_SrcPos = m_SrcStart;
		m_SrcChan = ALL_CHANNELS;
		m_DstChan = SaveChannel;

		m_SrcEnd = m_SrcFile.SampleToPosition(LAST_SAMPLE);
	}
	// TODO: support saving backward
	return TRUE;
}

// save the data being overwritten by other operation
// Position is source position. It goes to DstPos of this context.
// Channels saved from buffer are specified in m_DstChan
BOOL CCopyUndoContext::SaveUndoData(void const * pBuf, long BufSize,
									SAMPLE_POSITION Position,
									NUMBER_OF_CHANNELS NumSrcChannels)
{
	int const SrcSampleSize = m_SrcFile.BitsPerSample() / 8 * NumSrcChannels;
	NUMBER_OF_SAMPLES Samples = BufSize / SrcSampleSize;
	ASSERT(0 == BufSize % SrcSampleSize);

	char const * pSrcBuf = (char const*) pBuf;

	if (m_DstEnd < m_DstStart)
	{
		// pBuf points after the buffer.
		// Position is the block end
		if (BufSize >= 0
			|| Position <= m_DstEnd
			|| Position >= m_DstPos + -BufSize)
		{
			return FALSE;   // no need to save
		}

		if (Position < m_DstEnd + -BufSize)
		{
			BufSize = m_DstEnd - Position;
		}

		if (Position > m_DstPos)
		{
			BufSize -= m_DstPos - Position;
			pSrcBuf += m_DstPos - Position;
			Position = m_DstPos;
		}
		else
		{
			ASSERT(Position == m_DstPos);
		}
		ASSERT(0 == (m_DstStart - Position) % SrcSampleSize);
	}
	else
	{
		if (Position >= m_DstEnd
			|| Position + BufSize <= m_DstPos
			|| BufSize <= 0)
		{
			return FALSE;
		}

		ASSERT(Position == m_DstPos);

		// saving data from the beginning to the end
		if (Position + BufSize > m_DstEnd)
		{
			BufSize = m_DstEnd - Position;
		}
		if (Position < m_DstPos)
		{
			BufSize -= m_DstPos - Position;
			pSrcBuf += m_DstPos - Position;
			Position = m_DstPos;
		}
		else
		{
			ASSERT(Position == m_DstPos);
		}
		ASSERT(0 == (Position - m_DstStart) % SrcSampleSize);
	}

	ASSERT(0 == (BufSize % SrcSampleSize));

	long SamplesWritten = m_SrcFile.WriteSamples(ALL_CHANNELS,
												m_SrcPos, Samples, pSrcBuf, m_DstChan, NumSrcChannels,
												m_DstFile.GetSampleType());

	m_DstPos += SamplesWritten * SrcSampleSize;
	m_SrcPos += SamplesWritten * m_SrcFile.SampleSize();

	return SamplesWritten == Samples;
}

///////////// CDecompressContext
CDecompressContext::CDecompressContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
										CWaveFile & SrcFile,
										CWaveFile & DstFile,
										SAMPLE_POSITION SrcStart,
										SAMPLE_POSITION SrcEnd,
										NUMBER_OF_SAMPLES NumSamples,
										WAVEFORMATEX const * pSrcWf,
										BOOL SwapBytes)
	: BaseClass(pDoc, StatusString, _T("")),

	m_MmResult(MMSYSERR_NOERROR)
{
	m_CurrentSamples  = NumSamples;
	// operation can be terminated by Close
	m_Flags = OperationContextDiskIntensive | OperationContextNonCritical;
	m_DstFile = DstFile;
	m_SrcFile = SrcFile;
	m_Wf = pSrcWf;

	m_DstStart = m_DstFile.SampleToPosition(0);
	m_DstPos = m_DstStart;

	m_SrcStart = SrcStart;
	m_SrcPos = SrcStart;
	m_SrcEnd = SrcEnd;

	if (SwapBytes)
	{
		AddWaveProc(new CByteSwapConvertor);
	}
}

BOOL CDecompressContext::Init()
{
	if ( ! m_DstFile.AllocatePeakData(m_CurrentSamples))
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	if (m_Wf.FormatTag() != WAVE_FORMAT_PCM
		|| m_Wf.BitsPerSample() != 16)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}

		CWaveFormat wf;
		wf.InitFormat(WAVE_FORMAT_PCM, m_Wf.SampleRate(), m_Wf.NumChannels());

		if (! pAcmConvertor->InitConversion(m_Wf, wf))
		{
			delete pAcmConvertor;
			MessageBoxSync(IDS_STRING_UNABLE_TO_CONVERT);
			return FALSE;
		}
		AddWaveProc(pAcmConvertor);
	}

	return TRUE;
}

void CDecompressContext::PostRetire()
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
		m_DstFile.SavePeakInfo(pDocument->m_OriginalWavFile);
	}
	BaseClass::PostRetire();
}

////////// CSoundPlayContext
CSoundPlayContext::CSoundPlayContext(CWaveSoapFrontDoc * pDoc, CWaveFile & WavFile,
									SAMPLE_INDEX PlaybackStart, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK Channel,
									int PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize)
	: BaseClass(pDoc, _T("Playing"),
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

	m_PercentCompleted = -1;  // no percents
	m_PlayFile = WavFile;
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
	// if mono playback requested, open it as mono

	m_Wf.InitFormat(WAVE_FORMAT_PCM, m_PlayFile.SampleRate(),
					m_PlayFile.NumChannelsFromMask(m_Chan));
}

BOOL CSoundPlayContext::Init()
{

	m_OldThreadPriority = GetThreadPriority(GetCurrentThread());

	MMRESULT mmres = m_WaveOut.Open(m_PlaybackDevice, m_Wf, 0);

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

void CSoundPlayContext::PostRetire()
{
	pDocument->m_PlayingSound = false;
	if (m_bPauseRequested)
	{
		// TODO: keep selection and pause cursor inside a selection?
#if 0
		if (pDocument->m_SelectionEnd == pDocument->m_SelectionStart)
		{
			pDocument->m_SelectionEnd = m_SamplePlayed;
		}
#endif
		pDocument->SetSelection(m_SamplePlayed, m_SamplePlayed,
								pDocument->m_SelectedChannel, m_SamplePlayed, SetSelection_MoveCaretToCenter);
	}
	else
	{
		// return caret to the end of selected area
		pDocument->SetSelection(pDocument->m_SelectionStart, pDocument->m_SelectionEnd,
								pDocument->m_SelectedChannel, pDocument->m_SelectionEnd, SetSelection_MoveCaretToCenter);
	}
	BaseClass::PostRetire();
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
			m_PercentCompleted = -2;
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
			m_PercentCompleted = -2;
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
			unsigned Samples = size / m_Wf.SampleSize();
			unsigned SrcSamples =
				(m_End - m_CurrentPlaybackPos) / m_PlayFile.SampleSize();

			if (0 == SrcSamples)
			{
				m_CurrentPlaybackPos = m_End;
				break;
			}
			if (Samples > SrcSamples)
			{
				Samples = SrcSamples;
			}

			if (Samples != (unsigned)m_PlayFile.ReadSamples(m_Chan, m_CurrentPlaybackPos,
															Samples, pBuf))
			{
				m_CurrentPlaybackPos = m_End;
				break;
			}

			m_WaveOut.Play(nBuf, Samples * m_Wf.SampleSize(), 0);

			m_CurrentPlaybackPos += Samples * m_PlayFile.SampleSize();
		}
	}
	while(0);
	// notify the document
	m_SamplePlayed = m_WaveOut.GetPosition() + m_FirstSamplePlayed;

	pDocument->PlaybackPositionNotify(m_SamplePlayed, m_Chan);
	return TRUE;
}

CString CSoundPlayContext::GetPlaybackTimeString(int TimeFormat) const
{
	if ((TimeFormat & SampleToString_Mask) == SampleToString_Sample)
	{
		return SampleToString(m_SamplePlayed, m_PlayFile.SampleRate(), TimeFormat);
	}
	else
	{
		ULONG TimeMs = ULONG(double(m_SamplePlayed)
							/ m_PlayFile.SampleRate() * 1000.);

		TimeMs -= TimeMs % 100;
		ULONG BeginTimeMs = ULONG(double(m_FirstSamplePlayed)
								/ m_PlayFile.SampleRate() * 1000.);

		if (TimeMs < BeginTimeMs)
		{
			TimeMs = BeginTimeMs;
		}
		return TimeToHhMmSs(TimeMs, TimeFormat);
	}
}

void CUndoRedoContext::PostRetire()
{
	// check if the operation completed
	if (m_Flags & OperationContextFinished)
	{
		// everything is done, can do it normal way
		BaseClass::PostRetire();
		return;
	}

	CUndoRedoContext * pUndo = GetUndo();
	if (pUndo)
	{
		pDocument->AddUndoRedo(pUndo);
	}

	// TODO: if the last context is not done
	while( ! m_DoneList.IsEmpty())
	{
		COperationContext * pContext = m_DoneList.RemoveHead();
		pContext->PostRetire();
	}

	// delete all UNDO created when this context was executed
	DeleteUndo();
	// modify modification number, depending on operation
	if (IsUndoOperation())
	{
		if (pDocument->m_ModificationSequenceNumber >= 0)
		{
			pDocument->IncrementModified(FALSE, TRUE);    // bDeleteRedo=FALSE
		}
	}
	else
	{
		if (pDocument->m_ModificationSequenceNumber <= 0)
		{
			pDocument->DecrementModified();
		}
	}

	UnprepareUndo(); //to release file references
	// put the context back to undo/redo list
	pDocument->AddUndoRedo(this);
}

CUndoRedoContext * CUndoRedoContext::GetUndo()
{
	CUndoRedoContext * pUndo = BaseClass::GetUndo();

	if (NULL != pUndo)
	{
		// copy inverse value of Undoing flag
		pUndo->m_Flags &= ~OperationContextUndoing;
		pUndo->m_Flags |= OperationContextUndoing & ~m_Flags;
	}

	return pUndo;
}

CString CUndoRedoContext::GetStatusString()
{
	if (IsUndoOperation())
	{
		return _T("Undoing ") + m_OperationName + _T("...");
	}
	else
	{
		return _T("Redoing ") + m_OperationName + _T("...");
	}
}

///////////////////  CVolumeChangeContext
CVolumeChangeContext::CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
											LPCTSTR StatusString, LPCTSTR OperationName,
											double const * VolumeArray, int VolumeArraySize)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName)
{
	m_GetBufferFlags = 0;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;

	ASSERT(VolumeArraySize <= countof (m_Volume));

	for (int ch = 0; ch < VolumeArraySize; ch++)
	{
		m_Volume[ch] = float(VolumeArray[ch]);
	}
}

CVolumeChangeContext::CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
											LPCTSTR StatusString, LPCTSTR OperationName, float Volume)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName)
{
	m_GetBufferFlags = 0;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;

	for (int ch = 0; ch < countof (m_Volume); ch++)
	{
		m_Volume[ch] = Volume;
	}
}

BOOL CVolumeChangeContext::ProcessBuffer(void * buf, size_t BufferLength,
										SAMPLE_POSITION /*offset*/, BOOL /*bBackward*/)
{
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	int nChannels = m_DstFile.Channels();
	unsigned nSamples = BufferLength / sizeof pDst[0];

	if (nChannels == 1)
	{
		float volume = m_Volume[0];
		// special code for mute and inverse
		if (0 == volume)
		{
			memset(buf, 0, BufferLength);
		}
		else if (-1. == volume)
		{
			for (unsigned i = 0; i < nSamples; i++)
			{
				pDst[i] = LongToShort(-long(pDst[i]));
			}
		}
		else
		{
			for (unsigned i = 0; i < nSamples; i++)
			{
				pDst[i] = DoubleToShort(pDst[i] * volume);
			}
		}
		return TRUE;
	}
	else if (nChannels == 2)
	{
		// special code for 2 channels
		// special code for mute
		if (0. == m_Volume[0] && 0. == m_Volume[1])
		{
			if (m_DstFile.AllChannels(m_DstChan))
			{
				memset(pDst, 0, BufferLength);
			}
			else
			{
				if (SPEAKER_FRONT_RIGHT & m_DstChan)
				{
					pDst ++;
				}

				for (unsigned i = 0; i < nSamples; i += 2)
				{
					pDst[i] = 0;
				}
			}
			return TRUE;
		}
		// process both channels
		// special code for mute and inverse

		if ((-1. == m_Volume[0] || 0 == (SPEAKER_FRONT_LEFT & m_DstChan))
			&& (-1. == m_Volume[1] || 0 == (SPEAKER_FRONT_RIGHT & m_DstChan)))
		{
			for (unsigned i = 0; i < nSamples; i += 2)
			{
				if (SPEAKER_FRONT_LEFT & m_DstChan)
				{
					pDst[i] = LongToShort(-long(pDst[i]));
				}

				if (SPEAKER_FRONT_RIGHT & m_DstChan)
				{
					pDst[i + 1] = LongToShort(-long(pDst[i + 1]));
				}
			}
			return TRUE;
		}
	}

	for (unsigned i = 0; i < nSamples; i += nChannels)
	{
		int ch;
		CHANNEL_MASK mask;

		for (ch = 0, mask = 1; ch < nChannels; ch++, mask <<= 1)
		{
			if (mask & m_DstChan)
			{
				pDst[i + ch] = DoubleToShort(pDst[i + ch] * m_Volume[ch]);
			}
		}
	}
	return TRUE;
}

CDcScanContext::CDcScanContext(CWaveSoapFrontDoc * pDoc,
								LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName)
{
	for (unsigned i = 0; i < countof (m_Sum); i++)
	{
		m_Sum[i] = 0;
	}
}

int CDcScanContext::GetDc(int channel)
{
	if (channel >= countof (m_Sum))
	{
		return 0;
	}

	NUMBER_OF_SAMPLES nSamples = (m_DstPos - m_DstStart) / m_DstFile.SampleSize();

	if (0 == nSamples)
	{
		return 0;
	}

	return int(m_Sum[channel] / nSamples);
}

BOOL CDcScanContext::ProcessBuffer(void * buf, size_t BufferLength,
									SAMPLE_POSITION /*offset*/, BOOL /*bBackward*/)
{
	WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) buf;

	if (m_DstFile.Channels() == 1)
	{
		for (unsigned i = 0; i < BufferLength / sizeof pSrc[0]; i++)
		{
			m_Sum[0] += pSrc[i];
		}
	}
	else
	{
		ASSERT(2 == m_DstFile.Channels());

		for (unsigned i = 0; i < BufferLength / (2 * sizeof pSrc[0]); i++)
		{
			m_Sum[0] += pSrc[i * 2];
			m_Sum[1] += pSrc[i * 2 + 1];
		}
	}
	return TRUE;
}

CDcOffsetContext::CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
									LPCTSTR StatusString, LPCTSTR OperationName,
									CDcScanContext * pScanContext)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName),
	m_pScanContext(pScanContext)
{
	for (unsigned i = 0; i < countof (m_Offset); i++)
	{
		m_Offset[i] = 0;
	}

	m_GetBufferFlags = 0;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
}

CDcOffsetContext::CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
									LPCTSTR StatusString, LPCTSTR OperationName,
									int offset[], unsigned OffsetArraySize)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName),
	m_pScanContext(NULL)
{
	ASSERT(OffsetArraySize <= countof (m_Offset));

	for (unsigned i = 0; i < OffsetArraySize; i++)
	{
		m_Offset[i] = offset[i];
	}

	m_GetBufferFlags = 0;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
}

BOOL CDcOffsetContext::Init()
{
	if (NULL != m_pScanContext)
	{
		bool GotJob = false;

		for (NUMBER_OF_CHANNELS ch = 0; ch < m_DstFile.Channels(); ch ++)
		{
			m_Offset[ch] = -m_pScanContext->GetDc(ch);
			if (0 != (m_DstChan & (1 << ch))
				&& 0 != m_Offset[ch])
			{
				GotJob = true;
			}
		}

		if ( ! GotJob)
		{
			// all offsets are zero, nothing to change
			m_Flags |= OperationContextFinished;
		}
	}

	return BaseClass::Init();
}

BOOL CDcOffsetContext::ProcessBuffer(void * buf, size_t BufferLength,
									SAMPLE_POSITION /*offset*/, BOOL /*bBackward*/)
{
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;

	NUMBER_OF_CHANNELS nChannels = m_DstFile.Channels();
	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pDst[0];

	ASSERT(0 == (BufferLength % m_DstFile.SampleSize()));
	ASSERT(nChannels > 0);

	if (nChannels == 1)
	{
		int DcOffset = m_Offset[0];
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i++)
		{
			pDst[i] = LongToShort(pDst[i] + DcOffset);
		}
	}
	else if (2 == nChannels)
	{
		// process two channels
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += 2)
		{
			if (SPEAKER_FRONT_LEFT & m_DstChan)
			{
				pDst[i] = LongToShort(pDst[i] + m_Offset[0]);
			}

			if (SPEAKER_FRONT_RIGHT & m_DstChan)
			{
				pDst[i + 1] = LongToShort(pDst[i + 1] + m_Offset[1]);
			}
		}
	}
	else
	{
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += nChannels)
		{
			for (int ch = 0, mask = 1; ch < nChannels; ch ++, mask <<= 1)
			{
				if (mask & m_DstChan)
				{
					pDst[i + ch] = LongToShort(pDst[i + ch] + m_Offset[ch]);
				}
			}
		}
	}

	return TRUE;
}

CStatisticsContext::CStatisticsContext(CWaveSoapFrontDoc * pDoc,
										LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName),
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

BOOL CStatisticsContext::ProcessBuffer(void * buf, size_t const BufferLength,
										SAMPLE_POSITION offset, BOOL /*bBackward*/)
{
	WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) buf;
	int nChannels = m_DstFile.Channels();
	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pSrc[0];

	ASSERT(0 == (BufferLength % m_DstFile.SampleSize()));

	if (nChannels == 1)
	{
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i++)
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
			m_CurrentLeftCrc = CalcCrc32(CalcCrc32(m_CurrentLeftCrc,
													UCHAR(sample & 0xFF)), UCHAR((sample >> 8) & 0xFF));

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
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += nChannels)
		{
			int sample = pSrc[i];

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
			m_CurrentLeftCrc = CalcCrc32(CalcCrc32(m_CurrentLeftCrc,
													UCHAR(sample & 0xFF)), UCHAR((sample >> 8) & 0xFF));

			m_CurrentCommonCRC = CalcCrc32(CalcCrc32(m_CurrentCommonCRC,
													UCHAR(sample & 0xFF)), UCHAR((sample >> 8) & 0xFF));

			if (sample != 0)
			{
				m_CRC32Left = m_CurrentLeftCrc;
				m_CRC32Common = m_CurrentCommonCRC;
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

			sample = pSrc[i + 1];

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
			m_CurrentRightCrc = CalcCrc32(CalcCrc32(m_CurrentRightCrc,
													UCHAR(sample & 0xFF)), UCHAR((sample >> 8) & 0xFF));
			m_CurrentCommonCRC = CalcCrc32(CalcCrc32(m_CurrentCommonCRC,
													UCHAR(sample & 0xFF)), UCHAR((sample >> 8) & 0xFF));

			if (sample != 0)
			{
				m_CRC32Right = m_CurrentRightCrc;
				m_CRC32Common = m_CurrentCommonCRC;
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

void CStatisticsContext::PostRetire()
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

	{
		CDocumentPopup pop(pDocument);
		dlg.DoModal();
	}
	BaseClass::PostRetire();
}

CMaxScanContext::CMaxScanContext(CWaveSoapFrontDoc * pDoc,
								LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName)
{
	for (int i = 0; i < countof (m_Max); i++)
	{
		m_Max[i] = 0;
	}
}

int CMaxScanContext::GetMax(int channel)
{
	if (channel >= countof (m_Max))
	{
		return 0;
	}

	return m_Max[channel];
}

BOOL CMaxScanContext::ProcessBuffer(void * buf, size_t BufferLength,
									SAMPLE_POSITION /*offset*/, BOOL /*bBackward*/)
{
	WAVE_SAMPLE * pSrc = (WAVE_SAMPLE *) buf;

	int nChannels = m_DstFile.Channels();
	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pSrc[0];

	ASSERT(0 == (BufferLength % m_DstFile.SampleSize()));

	if (nChannels == 1)
	{
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i++)
		{
			int sample = pSrc[i];

			if (m_Max[0] < sample)
			{
				m_Max[0] = sample;
			}
			else if (m_Max[0] < -sample)
			{
				m_Max[0] = -sample;
			}
		}
	}
	else
	{
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += nChannels)
		{
			for (int ch = 0; ch < nChannels; ch++)
			{
				long sample = pSrc[i + ch];

				if (m_Max[ch] < sample)
				{
					m_Max[ch] = sample;
				}
				else if (m_Max[ch] < -sample)
				{
					m_Max[ch] = -sample;
				}
			}
		}
	}
	return TRUE;
}

BOOL CNormalizeContext::Init()
{
	// calculate normalization
	int nChannels = m_DstFile.Channels();

	float MaxVolume = 0.;
	bool GotJob = false;

	for (int ch = 0; ch < nChannels; ch++)
	{
		int Max = m_pScanContext->GetMax(ch);

		if (Max != 0)
		{
			m_Volume[ch] = float(32767. * m_LimitLevel / Max);

			if ((m_DstChan & (1 << ch))
				&& MaxVolume < m_Volume[ch])
			{
				MaxVolume = m_Volume[ch];
			}
		}
		else
		{
			m_Volume[ch] = 0.;
		}
	}

	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_DstChan & (1 << ch))
		{
			if (m_Volume[ch] != 0.)
			{
				if (m_bEqualChannels)
				{
					m_Volume[ch] = MaxVolume;
				}

				if (1. != m_Volume[ch])
				{
					GotJob = true;
				}
			}
		}
	}

	if ( ! GotJob)
	{
		// nothing to change
		m_Flags |= OperationContextFinished;
	}
	return TRUE;
}

void CFileSaveContext::PostRetire()
{
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

			m_SrcFile.Close();
			pDocument->PostFileSave(m_DstFile, m_NewName,
									0 != (m_Flags & FileSaveContext_SameName));
		}
	}
	else
	{
		// Operation canceled, discard the file
		pDocument->m_bClosePending = false;
		m_DstFile.DeleteOnClose();
		m_DstFile.Close();
	}
	BaseClass::PostRetire();
}

CConversionContext::CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, 0, OperationName),
	m_CurrentSamples(0)
{
	// delete the procs in the destructor
	m_Flags &= ~OperationContextDiskIntensive;
	m_ProcBatch.m_bAutoDeleteProcs = TRUE;
}

CConversionContext::CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
										LPCTSTR OperationName,
										CWaveFile & SrcFile,
										CWaveFile & DstFile)
	: BaseClass(pDoc, StatusString, 0, OperationName)
	, m_CurrentSamples(0)
{
	// delete the procs in the destructor
	m_SrcFile = SrcFile;
	m_DstFile = DstFile;

	m_SrcStart = m_SrcFile.SampleToPosition(0);
	m_DstStart = m_DstFile.SampleToPosition(0);
	m_SrcPos = m_SrcStart;
	m_DstPos = m_DstStart;
	m_SrcEnd = m_SrcFile.SampleToPosition(LAST_SAMPLE);
	m_DstEnd = m_DstStart;

	m_SrcChan = ALL_CHANNELS;
	m_DstChan = ALL_CHANNELS;

	m_Flags &= ~OperationContextDiskIntensive;
	m_ProcBatch.m_bAutoDeleteProcs = TRUE;
}

BOOL CConversionContext::InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
										CHANNEL_MASK chan, BOOL NeedUndo)
{
	if ( ! BaseClass::InitDestination(DstFile, StartSample, EndSample,
									chan, NeedUndo))
	{
		return FALSE;
	}

	m_SrcFile = DstFile;
	m_SrcStart = m_DstStart;
	m_SrcPos = m_SrcStart;
	m_SrcEnd = m_DstEnd;

	m_SrcChan = chan;

	return TRUE;
}

BOOL CConversionContext::OperationProc()
{
	SAMPLE_POSITION dwOperationBegin = m_DstPos;
	DWORD dwStartTime = GetTickCount();
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	long LeftToRead = 0;
	long LeftToWrite = 0;
	long WasRead = 0;
	long WasLockedToWrite = 0;

	void * pOriginalSrcBuf = NULL;
	char const * pSrcBuf = NULL;
	void * pOriginalDstBuf = NULL;
	char * pDstBuf = NULL;

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
				pSrcBuf = (char const *) pOriginalSrcBuf;
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
				m_DstEnd = (m_DstPos + CDirectFile::CacheBufferSize()) & -CDirectFile::CacheBufferSize();
				SizeToWrite = m_DstEnd - m_DstPos;
			}

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstPos, m_GetBufferFlags);

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
#if 0
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, WasLockedToWrite,
											m_DstPos, m_DstFile.Channels());
			}
#endif
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
	while (m_SrcPos < m_SrcEnd
			&& GetTickCount() - dwStartTime < 500
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	if (WAVE_FORMAT_PCM == m_DstFile.GetWaveFormat()->wFormatTag)
	{
		SAMPLE_INDEX nFirstSample = m_DstFile.PositionToSample(dwOperationBegin);
		SAMPLE_INDEX nLastSample = m_DstFile.PositionToSample(m_DstPos);
		// check if we need to change file size
		NUMBER_OF_SAMPLES TotalSamples = -1;

		if (nLastSample > m_CurrentSamples)
		{
			// calculate new length
			NUMBER_OF_SAMPLES MaxNumberOfSamples = 0x7FFFFFFF / m_DstFile.SampleSize();

			TotalSamples = MulDiv(nLastSample, m_SrcEnd - m_SrcStart, m_SrcPos - m_SrcStart);

			if (TotalSamples > MaxNumberOfSamples)
			{
				TotalSamples = MaxNumberOfSamples;
			}

			m_CurrentSamples = TotalSamples;

			m_DstFile.SetFileLengthSamples(TotalSamples);
		}

		TRACE("Decompress: sound changed from %d to %d, length=%d\n",
			nFirstSample, nLastSample, TotalSamples);
		pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, TotalSamples);
	}
	else
	{
		// notify the view
		pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);
	}

	UpdateCompletedPercent();

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

	DWORD dwStartTime = GetTickCount();

	do
	{
		m_Decoder.DeliverNextSample(200);
	}
	while (m_Decoder.IsStarted()
			&& GetTickCount() - dwStartTime < 500);

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
		m_PercentCompleted = MulDiv(100, m_DstCopySample, m_CurrentSamples);
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

	pDocument->SoundChanged(pDocument->WaveFileID(), 0, m_CurrentSamples, m_CurrentSamples, UpdateSoundDontRescanPeaks);

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
	m_DstPos = m_DstStart;
	m_DstCopySample = 0;

	m_Decoder.SetDstFile(file);
}

void CWmaDecodeContext::PostRetire()
{
	if (0
		//&& m_MmResult != MMSYSERR_NOERROR  // TODO
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
	BaseClass::PostRetire();
}

BOOL CWmaSaveContext::Init()
{
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
	m_DstPos = m_Enc.GetWrittenLength();
}

BOOL CWmaSaveContext::ProcessBuffer(void * buf, size_t len,
									SAMPLE_POSITION /*offset*/, BOOL /*bBackward*/)
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
	if (m_SrcPos >= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD dwStartTime = GetTickCount();
	//SAMPLE_POSITION dwOperationBegin = m_DstPos;
	int SampleSize = m_SrcFile.SampleSize();

	LONG SizeToProcess = 0;
	LONG WasLockedToWrite = 0;
	void * pSrcBuf;

	WAVE_SAMPLE TempBuf[4];
	do
	{
		// make sure ProcessBuffer gets integer number of complete samples, from sample boundary

		LONGLONG SizeToWrite = m_SrcEnd - m_SrcPos;
		WasLockedToWrite = m_SrcFile.GetDataBuffer( & pSrcBuf,
													SizeToWrite, m_SrcPos, m_GetBufferFlags);

		if (0 == WasLockedToWrite)
		{
			return FALSE;
		}

		if (WasLockedToWrite < SampleSize)
		{
			m_SrcFile.ReturnDataBuffer(pSrcBuf, WasLockedToWrite, 0);
			if (SampleSize != m_SrcFile.ReadAt(TempBuf, SampleSize, m_SrcPos))
			{
				return FALSE;
			}
			ProcessBuffer(TempBuf, SampleSize, m_SrcPos - m_SrcStart, FALSE);
			m_SrcPos += SampleSize;
			continue;
		}
		SizeToProcess = WasLockedToWrite - WasLockedToWrite % SampleSize;
		// save the data to be changed to undo buffer, but only on the first forward pass
		// virtual function which modifies the actual data:
		ProcessBuffer(pSrcBuf, SizeToProcess, m_SrcPos - m_SrcStart, FALSE);

		m_SrcFile.ReturnDataBuffer(pSrcBuf, WasLockedToWrite, 0);
		m_SrcPos += SizeToProcess;
	}
	while (m_SrcPos < m_SrcEnd
			&& GetTickCount() - dwStartTime < 1000);

	UpdateCompletedPercent();

	return TRUE;
}
