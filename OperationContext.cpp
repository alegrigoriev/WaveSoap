// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext.cpp
#include "stdafx.h"
#include "OperationContext.h"
#include "OperationContext2.h"
#include "OperationDialogs.h"
#include "ReopenSavedFileCopyDlg.h"
#include "MessageBoxSynch.h"
#include "DialogWithSelection.inl"
#include "WaveSoapFrontDoc.h"
#include <algorithm>
#include "TimeToStr.h"
#include "resource.h"       // main symbols
#include "ElapsedTime.h"

#define DUMP_ON_EXECUTE 1

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

COperationContext::COperationContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags, LPCTSTR StatusString, LPCTSTR OperationName)
	: m_pDocument(pDoc),
	m_Flags(Flags),
	m_OperationName(OperationName),
	m_StatusPrompt(StatusString),
	m_bClipped(false),
	m_MaxClipped(0.)
{
}

COperationContext::COperationContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags,
									UINT StatusStringId, UINT OperationNameId)
	: m_pDocument(pDoc),
	m_Flags(Flags),
	m_bClipped(false),
	m_MaxClipped(0.)
{
	if (0 != StatusStringId)
	{
		m_StatusPrompt.LoadString(StatusStringId);
	}
	if (0 != OperationNameId)
	{
		m_OperationName.LoadString(OperationNameId);
	}
}

COperationContext::~COperationContext()
{
	DeleteUndo();
}

void COperationContext::Dump(unsigned indent) const
{
	TRACE("%*.s%s\n", indent, "", typeid(*this).name());

	if ( ! m_UndoChain.IsEmpty())
	{
		TRACE(" %*.sUndo list:\n", indent, "");
		for (COperationContext * pContext = m_UndoChain.First();
			m_UndoChain.NotEnd(pContext);
			pContext = m_UndoChain.Next(pContext))
		{
			pContext->Dump(indent+2);
		}
	}
}

CString COperationContext::GetStatusString() const
{
	return m_StatusPrompt;
}

CString COperationContext::GetOperationName() const
{
	return m_OperationName;
}

CString COperationContext::GetCompletedStatusString() const
{
	CString s = GetOperationName();
	if (s.IsEmpty())
	{
		s = GetStatusString();
	}
	if (m_Flags & OperationContextFinished)
	{
		s += LoadCString(IDS_STATUS_OPERATION_PROMPT_COMPLETED);
	}
	else if (m_Flags & OperationContextStopRequested)
	{
		s += LoadCString(IDS_STATUS_OPERATION_PROMPT_STOPPED);
	}
	else
	{
		s += LoadCString(IDS_STATUS_OPERATION_PROMPT_FAILED);
	}
	return s;
}

void COperationContext::DeleteUndo()
{
	while ( ! m_UndoChain.IsEmpty())
	{
		delete m_UndoChain.RemoveTail();
	}
}

void COperationContext::Retire()
{
	// all context go to retirement list
	// queue it to the Doc
	PrintElapsedTime();

	m_pDocument->m_RetiredList.InsertTail(this);
}

void COperationContext::PostRetire()
{
	if (WasClipped())
	{
		// bring document frame to the top, then return
		CDocumentPopup pop(m_pDocument);

		CString s;
		s.Format(IDS_SOUND_CLIPPED, m_pDocument->GetTitle(), int(GetMaxClipped() * (100. / 32678)));
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
	}

	// save undo context
	CUndoRedoContext * pUndo = GetUndo();

	if (pUndo)
	{
		m_pDocument->AddUndoRedo(pUndo);
	}
	else if (0 != (m_Flags & OperationContextUndoCreated)
			&& 0 != (m_Flags & OperationContextModifyCountIncremented))
	{
		// undo was created and modify count incremented, but no changes were made (no undo returned)
		m_pDocument->DecrementModified();
	}

	delete this;
}

void COperationContext::Execute()
{
	if (DUMP_ON_EXECUTE)
	{
		TRACE("\nExecute:\n");
		Dump(); // for debug
	}

	SetBeginTime();
	m_pDocument->QueueOperation(this);
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
	ListHead<COperationContext> * pUndoChain = GetUndoChain();

	if (NULL == pUndoChain
		|| pUndoChain->IsEmpty())
	{
		return NULL;
	}

	CUndoRedoContext::auto_ptr pUndo(new CUndoRedoContext(m_pDocument,
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

int COperationContext::PercentCompleted() const
{
	MEDIA_FILE_SIZE TotalSize = GetTotalOperationSize();

	if (0 == TotalSize)
	{
		return -1;  // don't print percent
	}

	return int(GetCompletedOperationSize() * 100. / TotalSize);
}

void COperationContext::SetOperationName(LPCTSTR str)
{
	m_OperationName = str;
}

void COperationContext::SetStatusPrompt(LPCTSTR str)
{
	m_StatusPrompt = str;
}

void COperationContext::SetOperationName(UINT id)
{
	m_OperationName.LoadString(id);
}

void COperationContext::SetStatusPrompt(UINT id)
{
	m_StatusPrompt.LoadString(id);
}

void COperationContext::AddSelectionUndo(SAMPLE_INDEX Start, SAMPLE_INDEX End, SAMPLE_INDEX Caret,
										CHANNEL_MASK Channels)
{
	m_UndoChain.InsertHead(new CSelectionChangeOperation(m_pDocument, Start, End, Caret, Channels));
}

//////////// COneFileOperation
COneFileOperation::COneFileOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
									LPCTSTR StatusString,
									LPCTSTR OperationName)
	: BaseClass(pDoc, Flags, StatusString, OperationName)
	, m_SrcChan(ALL_CHANNELS)
	, m_SrcStart(0)
	, m_SrcEnd(0)
	, m_SrcPos(0)
{
}

COneFileOperation::COneFileOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
									UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, Flags, StatusStringId, OperationNameId)
	, m_SrcChan(ALL_CHANNELS)
	, m_SrcStart(0)
	, m_SrcEnd(0)
	, m_SrcPos(0)
{
}

bool COneFileOperation::KeepsPermanentFileReference() const
{
	return m_SrcFile.RefersPermanentFile();
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

	if (0) TRACE("SrcStart=%X, SrcEnd=%X, SrcPos=%X\n",
				m_SrcStart, m_SrcEnd, m_SrcPos);
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

void COneFileOperation::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);

	if (m_SrcFile.IsOpen()
		&& m_SrcStart != NULL
		&& m_SrcEnd != NULL
		&& m_SrcPos != NULL)
	{
		int SampleRate = m_SrcFile.SampleRate();
		TRACE(_T(" %*.sSRC: start=%s, end=%s, pos=%s, chan=%d\n"), indent, _T(""),
			LPCTSTR(SampleToString(m_SrcFile.PositionToSample(m_SrcStart), SampleRate)),
			LPCTSTR(SampleToString(m_SrcFile.PositionToSample(m_SrcEnd), SampleRate)),
			LPCTSTR(SampleToString(m_SrcFile.PositionToSample(m_SrcPos), SampleRate)),
			m_SrcChan);
	}
	else
	{
		TRACE(" %*.sSRC: start=%X, end=%X, pos=%X, chan=%d\n", indent, "",
			m_SrcStart, m_SrcEnd, m_SrcPos, m_SrcChan);
	}
}

//////////// CTwoFilesOperation
CTwoFilesOperation::CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc,
										ULONG Flags, LPCTSTR StatusString,
										LPCTSTR OperationName)
	: BaseClass(pDoc, Flags, StatusString, OperationName)
	, m_DstChan(ALL_CHANNELS)
	, m_DstStart(0)
	, m_DstEnd(0)
	, m_DstPos(0)
	, m_pUndoContext(NULL)
	, m_UndoStartPos(0)
	, m_UndoEndPos(0)
{
}

CTwoFilesOperation::CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc,
										ULONG Flags, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, Flags, StatusStringId, OperationNameId)
	, m_DstChan(ALL_CHANNELS)
	, m_DstStart(0)
	, m_DstEnd(0)
	, m_DstPos(0)
	, m_pUndoContext(NULL)
	, m_UndoStartPos(0)
	, m_UndoEndPos(0)
{
}

CTwoFilesOperation::CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags)
	: BaseClass(pDoc, Flags)
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

bool CTwoFilesOperation::KeepsPermanentFileReference() const
{
	return BaseClass::KeepsPermanentFileReference()
			|| m_DstFile.RefersPermanentFile();
}

// When the function creates UNDO, it also calls SetSaveForUndo to initialize range to save.
// If UNDO should be created later, the UNDO range should be initialized otherwise.
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

	if (0) TRACE("DstStart=%X, DstEnd=%X, DstPos=%X\n",
				m_DstStart, m_DstEnd, m_DstPos);

	if (0 == StartUndoSample
		&& LAST_SAMPLE == EndUndoSample)
	{
		StartUndoSample = StartSample;
		EndUndoSample = EndSample;
	}

	if (StartUndoSample != EndUndoSample)
	{
		SetSaveForUndo(StartUndoSample, EndUndoSample);
	}
	else
	{
		m_UndoStartPos = 0;
		m_UndoEndPos = 0;
	}

	// create undo
	if (NeedUndo)
	{
		return CreateUndo();
	}
	return TRUE;
}

BOOL CTwoFilesOperation::CreateUndo()
{
	if (NULL != m_pUndoContext
		|| ! m_DstFile.IsOpen()
		|| m_DstFile.GetFileID() != m_pDocument->WaveFileID()
		|| m_UndoStartPos == m_UndoEndPos)
	{
		return TRUE;
	}

	CCopyUndoContext::auto_ptr pUndo(new CCopyUndoContext(m_pDocument));

	// by default, save for REDO all undone
	if ( ! pUndo->InitUndoCopy(m_DstFile, m_UndoStartPos, m_UndoEndPos, m_DstChan))
	{
		return FALSE;
	}

	m_pUndoContext = pUndo.release();

	TRACE("CreateUndo:\n"), m_pUndoContext->Dump();

	return TRUE;
}

void CTwoFilesOperation::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);

	if (m_DstFile.IsOpen()
		&& m_DstStart != NULL
		&& m_DstEnd != NULL
		&& m_DstPos != NULL)
	{
		int SampleRate = m_DstFile.SampleRate();

		TRACE(_T(" %*.sDST: start=%s, end=%s, pos=%s, chan=%d\n"), indent, _T(""),
			LPCTSTR(SampleToString(m_DstFile.PositionToSample(m_DstStart), SampleRate)),
			LPCTSTR(SampleToString(m_DstFile.PositionToSample(m_DstEnd), SampleRate)),
			LPCTSTR(SampleToString(m_DstFile.PositionToSample(m_DstPos), SampleRate)),
			m_DstChan);

		if (NULL != m_pUndoContext
			&& m_pUndoContext->m_UndoStartPos != m_pUndoContext->m_UndoEndPos)
		{
			TRACE(_T(" %*.sSAVE to UNDO: start=%s, end=%s\n"), indent, _T(""),
				LPCTSTR(SampleToString(m_DstFile.PositionToSample(m_UndoStartPos), SampleRate)),
				LPCTSTR(SampleToString(m_DstFile.PositionToSample(m_UndoEndPos), SampleRate)));
		}
	}
	else
	{
		TRACE(" %*.sDST: start=%X, end=%X, pos=%X, chan=%d\n", indent, "",
			m_DstStart, m_DstEnd, m_DstPos, m_DstChan);

		if (NULL != m_pUndoContext
			&& m_pUndoContext->m_UndoStartPos != m_pUndoContext->m_UndoEndPos)
		{
			TRACE(" %*.sSAVE to UNDO: start=%X, end=%X\n", indent, "", m_UndoStartPos, m_UndoEndPos);
		}
	}


	if (NULL != m_pUndoContext)
	{
		TRACE(" %*.sUNDO:\n", indent, "");
		m_pUndoContext->Dump(indent+2);
	}
}

ListHead<COperationContext> * CTwoFilesOperation::GetUndoChain()
{
	if (NULL != m_pUndoContext)
	{
		m_pUndoContext->m_SrcEnd = m_pUndoContext->m_SrcPos;
		m_pUndoContext->m_DstEnd = m_pUndoContext->m_DstPos;

		if (m_pUndoContext->m_UndoStartPos != m_pUndoContext->m_UndoEndPos)
		{
			m_pUndoContext->m_UndoEndPos = m_pUndoContext->m_DstEnd;
		}

		TRACE("\nGetUndoChain insert:\n");
		m_pUndoContext->Dump(0);

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

// when the operation is completed, UNDO boundary is adjusted
// This is only used if this operation is base of CUndoCopy context and was used to save UNDO data.
void CTwoFilesOperation::DeInit()
{
	// adjust undo boundaries, in case the operation will be queued back
	if (m_UndoStartPos != m_UndoEndPos)
	{
		if (m_DstStart <= m_DstEnd)
		{
			if (m_UndoStartPos < m_DstPos)
			{
				m_UndoStartPos = std::min(m_DstPos, m_UndoEndPos);
			}
		}
		else
		{
			if (m_UndoStartPos > m_DstPos)
			{
				m_UndoStartPos = std::max(m_DstPos, m_UndoEndPos);
			}
		}
	}
	BaseClass::DeInit();
}

MEDIA_FILE_SIZE CTwoFilesOperation::GetTotalOperationSize() const
{
	if (m_SrcStart != m_SrcEnd)
	{
		return BaseClass::GetTotalOperationSize();
	}

	if (m_DstStart <= m_DstEnd)
	{
		return m_DstEnd - m_DstStart;
	}
	else
	{
		return m_DstStart - m_DstEnd;
	}
}

MEDIA_FILE_SIZE CTwoFilesOperation::GetCompletedOperationSize() const
{
	if (m_SrcStart != m_SrcEnd)
	{
		return BaseClass::GetCompletedOperationSize();
	}

	if (m_DstStart <= m_DstEnd)
	{
		return m_DstPos - m_DstStart;
	}
	else
	{
		return m_DstStart - m_DstPos;
	}
}

/////////// CThroughProcessOperation
CThroughProcessOperation::CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc,
													ULONG Flags, LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, Flags, StatusString, OperationName)
	, m_NumberOfForwardPasses(1)
	, m_NumberOfBackwardPasses(0)
	, m_CurrentPass(1)
	, m_GetBufferFlags(CDirectFile::GetBufferAndPrefetchNext)
	, m_ReturnBufferFlags(CDirectFile::ReturnBufferDirty)
{
}

CThroughProcessOperation::CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc,
													ULONG Flags, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, Flags, StatusStringId, OperationNameId)
	, m_NumberOfForwardPasses(1)
	, m_NumberOfBackwardPasses(0)
	, m_CurrentPass(1)
	, m_GetBufferFlags(CDirectFile::GetBufferAndPrefetchNext)
	, m_ReturnBufferFlags(CDirectFile::ReturnBufferDirty)
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
			m_pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);
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
			m_pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);
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
	if (m_CurrentPass < 0)
	{
		return (m_NumberOfForwardPasses - m_CurrentPass) * BaseClass::GetTotalOperationSize()
			- BaseClass::GetCompletedOperationSize();
	}
	else
	{
		return (m_CurrentPass - 1) * BaseClass::GetTotalOperationSize()
			+ BaseClass::GetCompletedOperationSize();
	}
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
								DWORD Flags, LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, Flags, StatusString, OperationName)
	, m_DoneSize(0)
{
}

CStagedContext::CStagedContext(CWaveSoapFrontDoc * pDoc,
								DWORD Flags, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, Flags, StatusStringId, OperationNameId)
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

void CStagedContext::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);  // print name

	if ( ! m_ContextList.IsEmpty())
	{
		TRACE(" %*.sPending operations:\n", indent, "");
		for (COperationContext * pContext = m_ContextList.First();
			m_ContextList.NotEnd(pContext);
			pContext = m_ContextList.Next(pContext))
		{
			pContext->Dump(indent+2);
		}
	}

	if ( ! m_DoneList.IsEmpty())
	{
		TRACE(" %*.sDone operations:\n", indent, "");
		for (COperationContext * pContext = m_DoneList.First();
			m_DoneList.NotEnd(pContext);
			pContext = m_DoneList.Next(pContext))
		{
			pContext->Dump(indent+2);
		}
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
		TRACE("Staged context: executing %s synchronously\n", typeid(*pContext).name());

		pContext->ExecuteSynch();
		pContext->m_Flags |= OperationContextFinished;

		m_DoneList.InsertTail(m_ContextList.RemoveHead());

		return TRUE;
	}
	else if (0 == (pContext->m_Flags & OperationContextInitialized))
	{
		TRACE("Staged context: %s::Init()\n", typeid(*pContext).name());
		if ( ! pContext->Init())
		{
			pContext->m_Flags |= OperationContextInitFailed | OperationContextStop;
			result = FALSE;
		}
		pContext->m_Flags |= OperationContextInitialized;
	}

	if (m_Flags & OperationContextStopRequested)
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
	}

	if (pContext->m_Flags & (OperationContextFinished | OperationContextInitFailed))
	{
		COperationContext * pDone = m_ContextList.RemoveHead();
		m_DoneSize += pDone->GetTotalOperationSize();

		TRACE("Staged context: %s::DeInit()\n", typeid(*pContext).name());
		pDone->DeInit();
		pDone->m_Flags &= ~OperationContextInitialized;

		// if DeInit set Stop flag, stop all gurther operations
		if (pDone->m_Flags & OperationContextStop)
		{
			m_Flags |= OperationContextStop;
		}

		m_DoneList.InsertTail(pDone);

	}
	else if ((m_Flags & OperationContextStopRequested)
			&& (pContext->m_Flags & OperationContextStop))
	{
		pContext->DeInit();

		pContext->m_Flags &= ~OperationContextInitialized;

		m_Flags |= OperationContextStop;
	}

	return result;
}

void CStagedContext::RetireAllChildren()
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
}

void CStagedContext::PostRetire()
{
	RetireAllChildren();

	BaseClass::PostRetire();
}

CString CStagedContext::GetStatusString() const
{
	if (m_ContextList.IsEmpty())
	{
		return BaseClass::GetStatusString();
	}

	CString s(m_ContextList.First()->GetStatusString());
	CString s1(m_StatusPrompt);

	if (m_StatusPrompt.IsEmpty())
	{
		s1 = s;
	}
	else if (s.IsEmpty())
	{
		s1 = m_StatusPrompt;
	}
	else
	{
		s1 += _T(" - ");
		s1 += s;
	}
#if 0
	if ( ! s1.IsEmpty())
	{
		TCHAR last = s1[s1.GetLength() - 1];
		if (last != ' '
			&& last != '.')
		{
			s1 += _T("...");
		}
	}
#endif
	return s1;
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

	// m_UndoChain list keeps all UNDO operations for this operation context.
	// First item in the list is to be executed first
	// this means the latest item executed during operation,
	// gives the first UNDO item.

	// m_DoneList keeps items done. First() was executed first, Last was executed last
	// UNDO from the last done item should become first in m_UndoChain list
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

///////////////////////////////////////////////////////////////////////
BOOL CStagedContext::InitExpandOperation(CWaveFile & File, SAMPLE_INDEX StartSample,
										NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	NUMBER_OF_SAMPLES NumberOfSamples = File.NumberOfSamples();

	NUMBER_OF_SAMPLES NewSamples = NumberOfSamples + Length;

	// 1. expand the file
	AddContext(new
				CWaveSamplesChangeOperation(m_pDocument, File, NewSamples));

	// 2. if not all channels moved, zero the expanded part
	CHANNEL_MASK ChannelsToZero = File.ChannelsMask() & ~Channel;

	if (0 != ChannelsToZero)
	{
		// special zero context used, with empty undo
		AddContext(new CInitChannels(m_pDocument, File, NumberOfSamples,
									NewSamples, ChannelsToZero));
	}
	else
	{
		// 3. Move all markers and regions (TODO)
	}
	// 4. CMoveContext moves all wave data toward the end
	if (NumberOfSamples > StartSample)
	{
		CMoveOperation::auto_ptr pMove(new
										CMoveOperation(m_pDocument, IDS_STATUS_PROMPT_EXPANDING_FILE));
		pMove->InitMove(File, StartSample, StartSample + Length, NumberOfSamples - StartSample,
						Channel);
		AddContext(pMove.release());
	}
	return TRUE;
}

// StartSample - from where the removed part starts
// Length - how much to remove
BOOL CStagedContext::InitShrinkOperation(CWaveFile & File,
										SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	NUMBER_OF_SAMPLES NumberOfSamples = File.NumberOfSamples();

	NUMBER_OF_SAMPLES NewSamples = NumberOfSamples - Length;

	// 1. Move all wave data
	if (NewSamples > StartSample)
	{
		CMoveOperation::auto_ptr pMove(new
										CMoveOperation(m_pDocument,
														IDS_STATUS_PROMPT_SHRINKING_FILE));

		pMove->InitMove(File, StartSample + Length, StartSample,
						NumberOfSamples - StartSample - Length,
						Channel);

		AddContext(pMove.release());
	}

	// 2. If partial channels moved: add special operation, to zero the area on undo

	if (NewSamples < StartSample + Length)
	{
		// because some data (not moved by CMoveOperation)
		// will be discarded by WaveSampleChange or by InitChannels,
		// we need to save it
		AddContext(new
					CSaveTrimmedOperation(m_pDocument, File,
										NewSamples, StartSample + Length, Channel));
	}

	if ( ! File.AllChannels(Channel))
	{
		// not all channels are moved
		// special zero context used, with empty undo
		AddContext(new CInitChannels(m_pDocument, File, NewSamples, NumberOfSamples,
									Channel));
	}
	else
	{
		// 3. If all channels moved: Move all markers and regions
		// TODO

		// 4. If all channels moved: Change number of samples
		AddContext(new
					CWaveSamplesChangeOperation(m_pDocument, File, NewSamples));
	}
	return TRUE;
}

void CStagedContext::InitCopyMarkers(CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
									NUMBER_OF_SAMPLES LengthToReplace,
									CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
									NUMBER_OF_SAMPLES SamplesToInsert)
{
	AddContext(new CCueEditOperation(m_pDocument, DstFile, StartDstSample,
									LengthToReplace, SrcFile, StartSrcSample, SamplesToInsert));
}

void CStagedContext::InitMoveMarkers(CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
									NUMBER_OF_SAMPLES LengthToReplace,
									NUMBER_OF_SAMPLES SamplesToInsert)
{
	AddContext(new CCueEditOperation(m_pDocument, DstFile, StartDstSample,
									LengthToReplace, SamplesToInsert));
}

BOOL CStagedContext::InitInsertCopy(CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
									NUMBER_OF_SAMPLES LengthToReplace, CHANNEL_MASK DstChannel,
									CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
									NUMBER_OF_SAMPLES SamplesToInsert, CHANNEL_MASK SrcChannel)
{
	if (LengthToReplace < SamplesToInsert)
	{
		// Expanding the file
		if ( ! InitExpandOperation(DstFile, StartDstSample + LengthToReplace,
									SamplesToInsert - LengthToReplace, DstChannel))
		{
			return FALSE;
		}

		// only copy markers if all channels are modified
		if (DstFile.AllChannels(DstChannel))
		{
			InitCopyMarkers(DstFile, StartDstSample, LengthToReplace,
							SrcFile, StartSrcSample, SamplesToInsert);
		}
		// the copied data outside of old data chunk length should be handled by
		// CRestoreTrimmedOperation
		NUMBER_OF_SAMPLES NumberOfSamples = DstFile.NumberOfSamples();
		if (StartDstSample + SamplesToInsert > NumberOfSamples)
		{
			CRestoreTrimmedOperation * pCopy = new CRestoreTrimmedOperation(m_pDocument);

			ASSERT(StartDstSample <= NumberOfSamples);
			TRACE(_T("Copying some samples with CRestoreTrimmedOperation:\n")
				_T("SRC: start=%s, end=%s, DST: start=%s, end=%s\n"),
				LPCTSTR(SampleToString(StartSrcSample + (NumberOfSamples - StartDstSample), 44100)),
				LPCTSTR(SampleToString(StartSrcSample + SamplesToInsert, 44100)),
				LPCTSTR(SampleToString(NumberOfSamples, 44100)),
				LPCTSTR(SampleToString(StartDstSample + SamplesToInsert, 44100)));

			pCopy->InitCopy(DstFile, NumberOfSamples, DstChannel, SrcFile,
							StartSrcSample + (NumberOfSamples - StartDstSample), SrcChannel,
							StartDstSample + SamplesToInsert - NumberOfSamples);

			AddContext(pCopy);

			SamplesToInsert = NumberOfSamples - StartDstSample;
		}
	}
	else if (LengthToReplace > SamplesToInsert)
	{
		// only copy markers if all channels are modified
		if (DstFile.AllChannels(DstChannel))
		{
			InitCopyMarkers(DstFile, StartDstSample, LengthToReplace,
							SrcFile, StartSrcSample, SamplesToInsert);
		}

		if ( ! InitShrinkOperation(DstFile, StartDstSample + SamplesToInsert,
									LengthToReplace - SamplesToInsert, DstChannel))
		{
			return FALSE;
		}
	}
	else if (DstFile.AllChannels(DstChannel))
	{
		// only copy markers if all channels are modified
		InitCopyMarkers(DstFile, StartDstSample, LengthToReplace,
						SrcFile, StartSrcSample, SamplesToInsert);
	}

	if (0 != SamplesToInsert)
	{
		// now copy data
		CCopyContext::auto_ptr pCopy(new CCopyContext(m_pDocument,
													IDS_STATUS_PROMPT_INSERTING_DATA));

		if ( ! pCopy->InitCopy(DstFile, StartDstSample, DstChannel,
								SrcFile, StartSrcSample, SrcChannel, SamplesToInsert))
		{
			return FALSE;
		}

		pCopy->SetSaveForUndo(StartDstSample, StartDstSample + LengthToReplace);

		AddContext(pCopy.release());
	}

	return TRUE;
}

///////////////// CScanPeaksContext
CScanPeaksContext::CScanPeaksContext(CWaveSoapFrontDoc * pDoc,
									CWaveFile & WavFile,
									CWaveFile & OriginalFile,
									BOOL bSavePeaks)
	: BaseClass(pDoc, OperationContextDiskIntensive,
				IDS_PEAK_SCAN_STATUS_PROMPT, IDS_PEAK_SCAN_OPERATION_NAME)
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

	// notify the view
	m_pDocument->FileChanged(m_SrcFile, dwOperationBegin,
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
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

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
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusString, OperationName)
{
}

CCopyContext::CCopyContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
{
}

// The function doesn't create UNDO. You should call SetSaveForUndo before CreateUndo
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
	if (m_Flags & OperationContextCommitFile)
	{
		m_DstFile.SetDatachunkLength(m_DstPos - m_DstFile.GetDataChunk()->dwDataOffset);
	}
	m_DstStart = m_DstPos;
}

void CCopyContext::PostRetire()
{
	if (m_Flags & OperationContextCommitFile)
	{
		m_DstFile.CommitChanges();
	}
	BaseClass::PostRetire();
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
				&& (m_SrcFile.GetFileID() != m_DstFile.GetFileID()
					|| m_SrcPos + CDirectFile::CacheBufferSize() < m_DstPos
					|| m_DstPos + CDirectFile::CacheBufferSize() < m_SrcPos)
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
	m_pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);

	return TRUE;
}

////////////// CCopyUndoContext

BOOL CCopyUndoContext::PrepareUndo()
{
	m_Flags &= ~(OperationContextStop | OperationContextFinished);
	m_DstFile = m_pDocument->m_WavFile;
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
									CHANNEL_MASK SaveChannel,
									SAMPLE_POSITION RedoStartPos, // source file position of data needed to redo
									SAMPLE_POSITION RedoEndPos)
{
	ASSERT(SrcFile.IsOpen());

	m_DstStart = SaveStartPos;
	m_DstPos = SaveStartPos;
	m_DstEnd = SaveEndPos;

	if (SaveEndPos >= SaveStartPos)
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

		m_SrcEnd = m_SrcFile.SampleToPosition(LAST_SAMPLE);

		if (0 == RedoStartPos
			&& LAST_SAMPLE_POSITION == RedoEndPos)
		{
			m_UndoStartPos = m_DstStart;
			m_UndoEndPos = m_DstEnd;
		}
		else if (RedoStartPos == RedoEndPos)
		{
			// make them equal
			m_UndoStartPos = m_DstStart;
			m_UndoEndPos = m_DstStart;
		}
		else
		{
			m_UndoStartPos = RedoStartPos;
			m_UndoEndPos = RedoEndPos;
		}
	}
	else
	{
		if ( ! m_SrcFile.CreateWaveFile( & SrcFile, NULL,
										SaveChannel,
										(SaveStartPos - SaveEndPos) / SrcFile.SampleSize(),
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

		m_SrcStart = m_SrcFile.SampleToPosition(LAST_SAMPLE);

		m_SrcEnd = m_SrcFile.SampleToPosition(0);

		if (0 == RedoStartPos
			&& LAST_SAMPLE_POSITION == RedoEndPos)
		{
			m_UndoStartPos = m_SrcStart;
			m_UndoEndPos = m_SrcEnd;
		}
		else if (RedoStartPos == RedoEndPos)
		{
			// make them equal
			m_UndoStartPos = m_SrcStart;
			m_UndoEndPos = m_SrcStart;
		}
		else
		{
			// make sure UndoStartPos > UndoEndPos
			if (RedoStartPos < RedoEndPos)
			{
				m_UndoStartPos = RedoEndPos;
				m_UndoEndPos = RedoStartPos;
			}
			else
			{
				m_UndoStartPos = RedoStartPos;
				m_UndoEndPos = RedoEndPos;
			}
		}
	}
	m_SrcPos = m_SrcStart;
	m_SrcChan = ALL_CHANNELS;
	m_DstChan = SaveChannel;

	return TRUE;
}

// save the data being overwritten by other operation
// Position is source position. It goes to DstPos of this context.
// Channels saved from buffer are specified in m_DstChan
BOOL CCopyUndoContext::SaveUndoData(void const * pBuf, long BufSize,
									SAMPLE_POSITION Position,
									NUMBER_OF_CHANNELS NumSrcChannels)
{
	int const SrcSampleSize = m_SrcFile.BitsPerSample() * NumSrcChannels / 8;

	ASSERT(0 == BufSize % SrcSampleSize);

	char const * pSrcBuf = (char const*) pBuf;

	if (m_DstEnd < m_DstStart)
	{
		ASSERT(BufSize <= 0);
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
			BufSize = -long(Position - m_DstEnd);
		}

		if (Position > m_DstPos)
		{
			BufSize += Position - m_DstPos;
			pSrcBuf -= Position - m_DstPos;
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
		ASSERT(BufSize >= 0);

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

	NUMBER_OF_SAMPLES const Samples = BufSize / SrcSampleSize;

	long SamplesWritten = m_SrcFile.WriteSamples(ALL_CHANNELS,
												m_SrcPos, Samples, pSrcBuf, m_DstChan, NumSrcChannels,
												m_DstFile.GetSampleType());

	m_DstPos += SamplesWritten * SrcSampleSize;
	m_SrcPos += SamplesWritten * m_SrcFile.SampleSize();

	return SamplesWritten == Samples;
}

///////////// CDecompressContext
CDecompressContext::CDecompressContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
										CWaveFile & SrcFile,
										CWaveFile & DstFile,
										SAMPLE_POSITION SrcStart,
										SAMPLE_POSITION SrcEnd,
										NUMBER_OF_SAMPLES NumSamples,
										WAVEFORMATEX const * pSrcWf,
										BOOL SwapBytes)
	: BaseClass(pDoc, StatusStringId)
	, m_MmResult(MMSYSERR_NOERROR)
	, m_CurrentSamples(NumSamples)
{
	// operation can be terminated by Close
	m_Flags = OperationContextDiskIntensive | OperationContextNonCritical;
	m_DstFile = DstFile;
	m_SrcFile = SrcFile;
	m_Wf = pSrcWf;

	m_DstStart = m_DstFile.SampleToPosition(0);
	m_DstPos = m_DstStart;
	m_DstEnd = m_DstStart;

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
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

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
			m_pDocument->m_bCloseThisDocumentNow = true;
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
		m_DstFile.SavePeakInfo(m_pDocument->m_OriginalWavFile);
	}
	BaseClass::PostRetire();
}

BOOL CDecompressContext::OperationProc()
{
	SAMPLE_POSITION dwOperationBegin = m_DstPos;
	if ( ! BaseClass::OperationProc())
	{
		return FALSE;
	}

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

		if (0) TRACE("Decompress: sound changed from %d to %d, length=%d\n",
					nFirstSample, nLastSample, TotalSamples);
		m_pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, TotalSamples);
	}
	return TRUE;
}

////////// CSoundPlayContext
CSoundPlayContext::CSoundPlayContext(CWaveSoapFrontDoc * pDoc, CWaveFile & WavFile,
									SAMPLE_INDEX PlaybackStart, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK Channel,
									INT_PTR PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize)
	: BaseClass(pDoc, OperationContextDontAdjustPriority,
				IDS_PLAYBACK_STATUS_PROMPT, IDS_PLAYBACK_OPERATION_NAME)
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
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

	m_OldThreadPriority = GetThreadPriority(GetCurrentThread());

	MMRESULT mmres = m_WaveOut.Open(m_PlaybackDevice, m_Wf, 0);
//    CWaveOut::EnumFormats(0, NULL, 0);

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

	m_pDocument->PlaybackPositionNotify(-1, 0);// sample=-1, channel=0

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	return TRUE;
}

void CSoundPlayContext::DeInit()
{
	m_pDocument->PlaybackPositionNotify(-1, 0);// sample=-1, channel=0
	SetThreadPriority(GetCurrentThread(), m_OldThreadPriority);
	m_WaveOut.Reset();

	BaseClass::DeInit();
}

void CSoundPlayContext::PostRetire()
{
	m_pDocument->m_PlayingSound = false;
	if (m_bPauseRequested)
	{
		// TODO: keep selection and pause cursor inside a selection?
#if 0
		if (m_pDocument->m_SelectionEnd == m_pDocument->m_SelectionStart)
		{
			m_pDocument->m_SelectionEnd = m_SamplePlayed;
		}
#endif
		m_pDocument->SetSelection(m_SamplePlayed, m_SamplePlayed,
								m_pDocument->m_SelectedChannel, m_SamplePlayed, SetSelection_MoveCaretToCenter);
	}
	else
	{
		// return caret to the end of selected area
		m_pDocument->SetSelection(m_pDocument->m_SelectionStart, m_pDocument->m_SelectionEnd,
								m_pDocument->m_SelectedChannel, m_pDocument->m_SelectionEnd, SetSelection_MoveCaretToCenter);
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

	m_pDocument->PlaybackPositionNotify(m_SamplePlayed, m_Chan);
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

//CString CSoundPlayContext::GetStatusString() const
	//{
	//}

CString CSoundPlayContext::GetCompletedStatusString() const
{
	if (m_bPauseRequested)
	{
		return LoadCString(IDS_PLAYBACK_PAUSED_STATUS);
	}
	else
	{
		return LoadCString(IDS_PLAYBACK_STOPPED_STATUS);
	}
}

//////////////// CUndoRedoContext
CUndoRedoContext::CUndoRedoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName)
	: BaseClass(pDoc, 0, _T(""), OperationName)
{
	m_Flags |= OperationContextUndoing;
}

CUndoRedoContext::CUndoRedoContext(CWaveSoapFrontDoc * pDoc, UINT OperationNameId)
	: BaseClass(pDoc, 0, 0, OperationNameId)
{
	m_Flags |= OperationContextUndoing;
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

	// remove stop and error flags
	m_Flags &= ~(OperationContextStopRequested | OperationContextStop);
	// last incomplete operation: remove stop and error flags
	if ( ! m_ContextList.IsEmpty())
	{
		m_ContextList.First()->m_Flags &= ~(OperationContextStopRequested | OperationContextStop);
	}

	CUndoRedoContext * pUndo = GetUndo();
	if (pUndo)
	{
		m_pDocument->AddUndoRedo(pUndo);
	}

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
		if (m_pDocument->m_ModificationSequenceNumber >= 0)
		{
			m_pDocument->IncrementModified(FALSE, TRUE);    // bDeleteRedo=FALSE
		}
	}
	else
	{
		if (m_pDocument->m_ModificationSequenceNumber <= 0)
		{
			m_pDocument->DecrementModified();
		}
	}

	UnprepareUndo(); //to release file references
	// put the context back to undo/redo list
	m_pDocument->AddUndoRedo(this);
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

CString CUndoRedoContext::GetOperationName() const
{
	if (IsUndoOperation())
	{
		return LoadCString(IDS_OPERATION_PREFIX_UNDO) + m_OperationName;
	}
	else
	{
		return LoadCString(IDS_OPERATION_PREFIX_REDO) + m_OperationName;
	}
}

CString CUndoRedoContext::GetStatusString() const
{
	if (IsUndoOperation())
	{
		return LoadCString(IDS_STATUS_PREFIX_UNDOING) + m_OperationName;
	}
	else
	{
		return LoadCString(IDS_STATUS_PREFIX_REDOING) + m_OperationName;
	}
}

///////////////////  CVolumeChangeContext
CVolumeChangeContext::CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
											UINT StatusStringId, UINT OperationNameId,
											double const * VolumeArray, int VolumeArraySize)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
{
	ASSERT(VolumeArraySize <= countof (m_Volume));

	for (int ch = 0; ch < VolumeArraySize; ch++)
	{
		m_Volume[ch] = float(VolumeArray[ch]);
	}
}

CVolumeChangeContext::CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
											UINT StatusStringId, UINT OperationNameId, float Volume)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
{
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
								UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
{
	m_ReturnBufferFlags = 0;    // no write back

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
									UINT StatusStringId, UINT OperationNameId,
									CDcScanContext * pScanContext)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId),
	m_pScanContext(pScanContext)
{
	for (unsigned i = 0; i < countof (m_Offset); i++)
	{
		m_Offset[i] = 0;
	}

}

CDcOffsetContext::CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									int offset[], unsigned OffsetArraySize)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId),
	m_pScanContext(NULL)
{
	ASSERT(OffsetArraySize <= countof (m_Offset));

	for (unsigned i = 0; i < OffsetArraySize; i++)
	{
		m_Offset[i] = offset[i];
	}

}

BOOL CDcOffsetContext::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

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
			DeleteUndo();
		}
	}

	return TRUE;
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
										UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId),
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
	m_ReturnBufferFlags = 0;    // no write back
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
	// offset is relative to start, but we want absolute offset in file
	offset += m_DstStart;

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
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples;
			i += nChannels, offset += nChannels * sizeof (*pSrc))
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
	// only show the dialog if the operation was all completed
	if (m_Flags & OperationContextFinished)
	{
		// show dialog
		CStatisticsDialog dlg(this,
							m_pDocument->m_WavFile, m_pDocument->m_CaretPosition,
							m_pDocument->GetPathName());

		CDocumentPopup pop(m_pDocument);

		if (IDC_BUTTON_GOTO_MAX == dlg.DoModal())
		{
			CHANNEL_MASK Channel;
			SAMPLE_INDEX Sample = dlg.GetMaxSamplePosition( & Channel);

			m_pDocument->SetSelection(Sample, Sample, Channel, Sample, SetSelection_MoveCaretToCenter);
		}
	}
	BaseClass::PostRetire();
}

SAMPLE_INDEX CStatisticsContext::GetMaxSamplePosition(CHANNEL_MASK * pChannel) const
{
	SAMPLE_POSITION SamplePos = 0;
	CHANNEL_MASK Channel = ALL_CHANNELS;

	SamplePos = m_PosMaxLeft;
	long value = m_MaxLeft;
	Channel = (1 << 0);

	if (-m_MinLeft > value)
	{
		value = -m_MinLeft;
		SamplePos = m_PosMinLeft;
	}

	if (m_DstFile.Channels() > 1)
	{
		if (m_MaxRight > value)
		{
			value = m_MaxRight;
			SamplePos = m_PosMaxRight;
			Channel = (1 << 1);
		}
		if (-m_MinRight > value)
		{
			value = -m_MinRight;
			SamplePos = m_PosMinRight;
			Channel = (1 << 1);
		}
	}

	if (NULL != pChannel)
	{
		*pChannel = Channel;
	}

	return m_DstFile.PositionToSample(SamplePos);
}

CMaxScanContext::CMaxScanContext(CWaveSoapFrontDoc * pDoc,
								UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
{
	m_ReturnBufferFlags = 0;    // no write back

	for (int i = 0; i < countof (m_Max); i++)
	{
		m_Max[i] = 0;
	}
}

int CMaxScanContext::GetMax(unsigned channel)
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
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}
	// calculate normalization
	int nChannels = m_DstFile.Channels();

	float MaxVolume = 1.;
	bool GotJob = false;
	long MaxLevel = 0;

	for (int ch = 0; ch < nChannels; ch++)
	{
		int Max = m_pScanContext->GetMax(ch);

		if (Max != 0)
		{
			m_Volume[ch] = float(32767. * m_LimitLevel / Max);

			if ((m_DstChan & (1 << ch))
				&& MaxLevel < Max)
			{
				MaxLevel = Max;
			}
		}
		else
		{
			m_Volume[ch] = 0.;
		}
	}

	if (0 != MaxLevel)
	{
		MaxVolume = float(32767. * m_LimitLevel / MaxLevel);
	}

	if (MaxLevel == DoubleToShort(MaxVolume * MaxLevel))
	{
		MaxVolume = 1.;
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
		DeleteUndo();
	}
	return TRUE;
}

void CFileSaveContext::PostRetire()
{
	RetireAllChildren();  // make sure all references in child operations are closed
	// rename files, change them
	if (m_Flags & OperationContextFinished)
	{
		m_DstFile.DeleteOnClose(false);
		// but if a copy is created, just close it
		if (m_Flags & FileSaveContext_SavingPartial)
		{
			m_DstFile.CommitChanges();
			m_DstFile.Close();
		}
		else if (m_Flags & FileSaveContext_SavingCopy)
		{
			m_DstFile.CommitChanges();

			if (0 == (m_Flags & FileSaveContext_DontSavePeakInfo))
			{
				m_pDocument->m_WavFile.SavePeakInfo(m_DstFile);
			}

			if (0 == (m_Flags & FileSaveContext_DontPromptReopen))
			{
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

				dlg.m_Prompt.Format(fmt, LPCTSTR(m_pDocument->GetTitle()),
									LPCTSTR(m_NewName));

				int res;
				{
					CDocumentPopup pop(m_pDocument);
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
				m_DstFile.Close();
			}
		}
		else
		{
			// exchange files, close and delete old ones, rename new ones
			m_pDocument->m_FileTypeFlags = m_NewFileTypeFlags;

			m_SrcFile.Close();
			m_pDocument->PostFileSave(m_DstFile, m_NewName,
									0 != (m_Flags & FileSaveContext_SameName));
		}
	}
	else
	{
		// Operation canceled, discard the file
		m_pDocument->m_bClosePending = false;
		m_DstFile.DeleteOnClose();
		m_DstFile.Close();
	}
	BaseClass::PostRetire();
}

/////////////////////////////////////////////////////////
///////////////  CWaveProcContext  //////////////////////
CWaveProcContext::CWaveProcContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, 0, StatusStringId, OperationNameId)
{
	// delete the procs in the destructor
	m_Flags &= ~OperationContextDiskIntensive;
	m_ProcBatch.m_bAutoDeleteProcs = TRUE;
}

// When the function creates UNDO, it also calls SetSaveForUndo to initialize range to save.
// If UNDO should be created later, the UNDO range should be initialized by calling SetSaveForUndo otherwise.
BOOL CWaveProcContext::InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
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

// convert number of channels and sample rate to make data compatible with the destination compressor
BOOL CWaveProcContext::MakeCompatibleFormat(WAVEFORMATEX const * pSrcWf, WAVEFORMATEX const * pDstWf,
											CHANNEL_MASK ChannelsToUse)
{
	NUMBER_OF_CHANNELS OldChannels = pSrcWf->nChannels;
	NUMBER_OF_CHANNELS const NewChannels = pDstWf->nChannels;

	if (NewChannels < OldChannels)
	{
		AddWaveProc(new CChannelConvertor(OldChannels, NewChannels, ChannelsToUse));
		OldChannels = NewChannels;
	}

	if (pDstWf->nSamplesPerSec != pSrcWf->nSamplesPerSec)
	{
		AddWaveProc(new CResampleFilter(pSrcWf->nSamplesPerSec, pDstWf->nSamplesPerSec,
										63, OldChannels));

		m_DstFile.GetInstanceData()->RescaleMarkers(pSrcWf->nSamplesPerSec, pDstWf->nSamplesPerSec);
	}

	// if target channels is more than source, convert it after resampling,
	if (NewChannels > OldChannels)
	{
		AddWaveProc(new CChannelConvertor(OldChannels, NewChannels, ALL_CHANNELS));
	}
	return TRUE;
}

BOOL CWaveProcContext::OperationProc()
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

	WAVE_SAMPLE TmpReadBuffer[MAX_NUMBER_OF_CHANNELS];
	WAVE_SAMPLE TmpWriteBuffer[MAX_NUMBER_OF_CHANNELS];

	long const InputSampleSize = m_ProcBatch.GetInputSampleSize();
	long const OutputSampleSize = m_ProcBatch.GetOutputSampleSize();

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
				//DebugTimeStamp time;

				WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcPos, CDirectFile::GetBufferAndPrefetchNext);

				//if (1) TRACE("CWaveProcContext:block 0x%X file %p, time %d/10 ms\n",
				//    DWORD(m_SrcPos >> 17), m_SrcFile.GetFileID(), time.ElapsedTimeTenthMs());

				if (0 == WasRead)
				{
					m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
												CDirectFile::ReturnBufferDirty);
					return FALSE;
				}
				pSrcBuf = (char const *) pOriginalSrcBuf;
				LeftToRead = WasRead;

				if (0 != InputSampleSize)
				{
					LeftToRead -= LeftToRead % InputSampleSize;

					if (0 == LeftToRead)
					{
						m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);

						WasRead = 0;
						if (InputSampleSize != m_SrcFile.ReadAt(TmpReadBuffer,
																InputSampleSize, m_SrcPos))
						{
							m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
														CDirectFile::ReturnBufferDirty);
							return FALSE;
						}
						pSrcBuf = (char const *) TmpReadBuffer;
						LeftToRead = InputSampleSize;
					}
				}
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
			long flags = CDirectFile::GetBufferWriteOnly;

			if (m_SrcFile.GetFileID() == m_DstFile.GetFileID()
				|| NULL != m_pUndoContext)
			{
				flags = 0;
			}

			if (0 == SizeToWrite)
			{
				m_DstEnd = (m_DstPos + CDirectFile::CacheBufferSize()) & -CDirectFile::CacheBufferSize();
				SizeToWrite = m_DstEnd - m_DstPos;

				// don't allow zeroing after the specified area
				flags = 0;
			}

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstPos, flags);

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

			if (0 != OutputSampleSize)
			{
				LeftToWrite -= LeftToWrite % OutputSampleSize;

				if (0 == LeftToWrite)
				{
					m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
												CDirectFile::ReturnBufferDiscard);

					WasLockedToWrite = 0;

					// only need to read the buffer if UNDO is necessary
					if (NULL != m_pUndoContext
						&& OutputSampleSize != m_DstFile.ReadAt(TmpWriteBuffer,
																OutputSampleSize, m_DstPos))
					{
						m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
						return FALSE;
					}
					pDstBuf = (char *) TmpWriteBuffer;
					LeftToWrite = OutputSampleSize;
				}
			}
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf, LeftToWrite,
											m_DstPos, m_DstFile.Channels());
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

		if (0 == WasLockedToWrite
			&& 0 != DstBufUsed)
		{
			// write data back
			if (OutputSampleSize != m_DstFile.WriteAt(TmpWriteBuffer,
													OutputSampleSize, m_DstPos))
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
				return FALSE;
			}
			LeftToWrite = 0;
		}
		else
		{
			LeftToWrite -= DstBufUsed;
			pDstBuf += DstBufUsed;

			if (0 == LeftToWrite)
			{
				m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
				WasLockedToWrite = 0;
			}
		}

		m_DstPos += DstBufUsed;
	} while (GetTickCount() - dwStartTime < 1000);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	// notify the view
	m_pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);

	return TRUE;
}

BOOL CWaveProcContext::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}
	return m_ProcBatch.Init();
}

void CWaveProcContext::DeInit()
{
	m_ProcBatch.DeInit();
	BaseClass::DeInit();
}

///////////////////////////////////////////////////////////
///////////////  CConversionContext  //////////////////////
CConversionContext::CConversionContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
}

CConversionContext::CConversionContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
										UINT OperationNameId,
										CWaveFile & SrcFile,
										CWaveFile & DstFile, BOOL RawDstFile, SAMPLE_INDEX SrcBegin, SAMPLE_INDEX SrcEnd)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
	m_SrcFile = SrcFile;
	m_DstFile = DstFile;

	m_SrcStart = m_SrcFile.SampleToPosition(SrcBegin);
	m_SrcPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.SampleToPosition(SrcEnd);

	if (RawDstFile)
	{
		m_DstStart = 0;
	}
	else
	{
		m_DstStart = m_DstFile.SampleToPosition(0);
		m_DstEnd = m_DstFile.SampleToPosition(LAST_SAMPLE);
	}

	m_DstPos = m_DstStart;

	m_SrcChan = ALL_CHANNELS;
	m_DstChan = ALL_CHANNELS;
}

void CConversionContext::PostRetire()
{
	// For non-RIFF file, don't do post-processing
	MEDIA_FILE_POSITION DataOffset = m_DstFile.GetDataChunk()->dwDataOffset;
	if (DataOffset != 0)
	{
		// update data chunk and number of samples
		m_DstFile.SetFactNumberOfSamples(m_ProcBatch.GetOutputNumberOfSamples());

		m_DstFile.SetDatachunkLength(DWORD(m_DstPos - DataOffset));
	}

	BaseClass::PostRetire();
}

//////////////// CWmaDecodeContext
CWmaDecodeContext::CWmaDecodeContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
									CDirectFile & rWmaFile)
	: BaseClass(pDoc,
				// operation can be terminated by Close
				OperationContextDiskIntensive | OperationContextNonCritical, StatusStringId),
	m_WmaFile(rWmaFile)
{
}

CWmaDecodeContext::~CWmaDecodeContext()
{
	//m_Decoder.Stop();
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

		m_pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, NewSampleCount);
	}

	return TRUE;
}

BOOL CWmaDecodeContext::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

	m_CoInit.InitializeCom(COINIT_APARTMENTTHREADED);

	if ( ! m_Decoder.Init()
		|| S_OK != m_Decoder.Open(m_WmaFile))
	{

		CString s;
		s.Format(IDS_CANT_OPEN_WMA_DECODER, m_WmaFile.GetName());
		MessageBoxSync(s, MB_ICONEXCLAMATION | MB_OK);

		m_pDocument->m_bCloseThisDocumentNow = true;
		return FALSE;
	}

	m_CurrentSamples = m_Decoder.GetTotalSamples();

	// create a wave file in the document
	if ( ! m_pDocument->m_WavFile.CreateWaveFile(NULL, m_Decoder.GetDstFormat(),
												ALL_CHANNELS, m_CurrentSamples,  // initial sample count
												CreateWaveFileTempDir
												| CreateWaveFileDeleteAfterClose
												| CreateWaveFilePcmFormat
												| CreateWaveFileTemp, NULL))
	{
		MessageBoxSync(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);

		m_pDocument->m_bCloseThisDocumentNow = true;
		return FALSE;
	}

	m_pDocument->m_OriginalWaveFormat = m_Decoder.GetDstFormat();
	SetDstFile(m_pDocument->m_WavFile);

	m_pDocument->SoundChanged(m_pDocument->WaveFileID(), 0, m_CurrentSamples, m_CurrentSamples, UpdateSoundDontRescanPeaks);

	m_pDocument->QueueSoundUpdate(m_pDocument->UpdateWholeFileChanged,
								m_pDocument->WaveFileID(), 0, 0, m_CurrentSamples);

	m_pDocument->m_WavFile.LoadPeaksForCompressedFile(m_pDocument->m_OriginalWavFile, m_CurrentSamples);

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

	BaseClass::DeInit();
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
	if (0 == (m_Flags & OperationContextFinished))
	{
		CString s;
		if (m_Flags & OperationContextInitFailed)
		{
			//s.Format(IDS_CANT_DECOMPRESS_FILE, LPCTSTR(m_pDocument->m_OriginalWavFile.GetName()), -1, 0);
			//AfxMessageBox(s, MB_ICONSTOP);
			m_pDocument->m_bCloseThisDocumentNow = true;
		}
		else
		{
			s.Format(IDS_ERROR_WHILE_DECOMPRESSING_FILE,
					LPCTSTR(m_pDocument->m_OriginalWavFile.GetName()), 0);
			AfxMessageBox(s, MB_ICONSTOP);
		}
	}
	else
	{
		// set the file length, according to the actual number of samples decompressed
		m_DstFile.SetFileLengthSamples(m_DstCopySample);

		m_pDocument->SoundChanged(m_DstFile.GetFileID(), 0, 0, m_DstCopySample, UpdateSoundDontRescanPeaks);

		if (m_Flags & DecompressSavePeakFile)
		{
			m_DstFile.SavePeakInfo(m_pDocument->m_OriginalWavFile);
		}
	}
	BaseClass::PostRetire();
}

//////////////////////////////////////////////////////////////////
///////////////// CWmaSaveContext
CWmaSaveContext::CWmaSaveContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId,
								CWaveFile & SrcFile, CWaveFile & DstFile,
								SAMPLE_INDEX SrcBegin, SAMPLE_INDEX SrcEnd)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
	, m_TmpBufferFilled(0)
{
	InitSource(SrcFile, SrcBegin, SrcEnd, ALL_CHANNELS);

	m_DstFile = DstFile;
	m_DstStart = 0;
	m_DstPos = 0;
	m_DstEnd = 0;
	m_DstChan = ALL_CHANNELS;
}

BOOL CWmaSaveContext::Init()
{
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

	CWaveFormat wf;
	wf.InitFormat(WAVE_FORMAT_PCM, m_DstFile.SampleRate(), m_DstFile.Channels());

	m_Enc.SetSourceWaveFormat(wf);

	m_CoInit.InitializeCom(COINIT_MULTITHREADED);

	if (! m_Enc.Init())
	{
		return FALSE;
	}

	m_Enc.SetDestinationFormat(m_DstFile.GetWaveFormat());

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

	BaseClass::DeInit();
}

BOOL CWmaSaveContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	DWORD dwStartTime = GetTickCount();

	LONG WasRead = 0;
	void * pOriginalSrcBuf = NULL;

	LONG LeftToRead = 0;
	char const * pSrc = NULL;
	long const InputSampleSize = m_SrcFile.SampleSize();

	WAVE_SAMPLE TmpReadBuffer[MAX_NUMBER_OF_CHANNELS];
	BOOL TmpReadBufferUsed = FALSE;

	do
	{
		if (0 == LeftToRead)
		{
			TmpReadBufferUsed = FALSE;

			LONGLONG SizeToRead = m_SrcEnd - m_SrcPos;
			if (SizeToRead > CDirectFile::CacheBufferSize())
			{
				SizeToRead = CDirectFile::CacheBufferSize();
			}

			if (0 != SizeToRead)
			{
				WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcPos, CDirectFile::GetBufferAndPrefetchNext);

				if (0 != SizeToRead
					&& 0 == WasRead)
				{
					return FALSE;
				}

				pSrc = (char const *) pOriginalSrcBuf;
				LeftToRead = WasRead;

				if (0 != InputSampleSize)
				{
					LeftToRead -= LeftToRead % InputSampleSize;

					if (0 == LeftToRead)
					{
						m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);

						WasRead = 0;
						if (InputSampleSize != m_SrcFile.ReadAt(TmpReadBuffer,
																InputSampleSize, m_SrcPos))
						{
							return FALSE;
						}
						pSrc = (char const *) TmpReadBuffer;
						LeftToRead = InputSampleSize;
						TmpReadBufferUsed = TRUE;
					}
				}
			}
			else
			{
				pOriginalSrcBuf = NULL;
				LeftToRead = 0;
				WasRead = 0;
			}
		}

		size_t SrcBufUsed = 0;

		size_t DstBufUsed = m_ProcBatch.ProcessSound(pSrc, m_TmpBuffer + m_TmpBufferFilled, LeftToRead,
													m_TmpBufferSize - m_TmpBufferFilled, & SrcBufUsed);

		m_TmpBufferFilled += DstBufUsed;

		if (m_TmpBufferFilled == m_TmpBufferSize
			|| (0 == WasRead && ! TmpReadBufferUsed))
		{
			m_Enc.Write(m_TmpBuffer, m_TmpBufferFilled);
			m_TmpBufferFilled = 0;
		}

		if (0 == SrcBufUsed && 0 == DstBufUsed)
		{
			m_Flags |= OperationContextFinished;
			break;
		}

		m_SrcPos += SrcBufUsed;
		pSrc += SrcBufUsed;
		LeftToRead -= SrcBufUsed;

		if (0 == LeftToRead)
		{
			m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead, 0);
			WasRead = 0;
		}
	}
	while (GetTickCount() - dwStartTime < 1000);

	if (WasRead != 0)
	{
		m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead, 0);
	}

	return TRUE;
}
