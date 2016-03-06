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
#include "ContextWorkerThread.h"

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
void COperationContext::SetBeginTime(HANDLE hThread)
{
	FILETIME tmp;
	GetThreadTimes(GetCurrentThread(),
					& tmp, & tmp, & tmp, & m_pDocument->m_ThreadUserTime);
	GetThreadTimes(hThread,
					& tmp, & tmp, & tmp, & m_ThreadUserTime);
	m_BeginSystemTime = GetTickCount();
}

void COperationContext::PrintElapsedTime(HANDLE hThread)
{
	FILETIME tmp, UserTime;
	GetThreadTimes(hThread,
					& tmp, & tmp, & tmp, & UserTime);
	DWORD TickCount = GetTickCount();
	if (1) TRACE("Elapsed worker thread time : %.f s, elapsed real time=%.3f s\n",
				(UserTime.dwLowDateTime - m_ThreadUserTime.dwLowDateTime) / 10000000.,
				(TickCount - m_BeginSystemTime) / 1000.);
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
	PrintElapsedTime(GetCurrentThread());

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

	if (DUMP_ON_EXECUTE) TRACE("CreateUndo:\n"), m_pUndoContext->Dump();

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

		if (DUMP_ON_EXECUTE) TRACE("\nGetUndoChain insert:\n"), m_pUndoContext->Dump(0);

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

void CTwoFilesOperation::PostRetire()
{
	if (m_Flags & PostRetireSavePeakFile)
	{
		m_DstFile.SavePeakInfo(m_pDocument->m_OriginalWavFile);
	}
	BaseClass::PostRetire();
}

MEDIA_FILE_SIZE CTwoFilesOperation::GetTotalOperationSize() const
{
	MEDIA_FILE_SIZE SrcOpSize = 0;
	if (m_SrcStart <= m_SrcEnd)
	{
		SrcOpSize = m_SrcEnd - m_SrcStart;
	}
	else
	{
		SrcOpSize = m_SrcStart - m_SrcEnd;
	}

	if (m_DstStart <= m_DstEnd)
	{
		return SrcOpSize + m_DstEnd - m_DstStart;
	}
	else
	{
		return SrcOpSize + m_DstStart - m_DstEnd;
	}
}

MEDIA_FILE_SIZE CTwoFilesOperation::GetCompletedOperationSize() const
{
	if (m_SrcStart != m_SrcEnd)
	{
		return BaseClass::GetCompletedOperationSize();
	}

	MEDIA_FILE_SIZE SrcCompleted = 0;
	if (m_SrcStart <= m_SrcEnd)
	{
		SrcCompleted = m_SrcPos - m_SrcStart;
	}
	else
	{
		SrcCompleted = m_SrcStart - m_SrcPos;
	}

	if (m_DstStart <= m_DstEnd)
	{
		return SrcCompleted + m_DstPos - m_DstStart;
	}
	else
	{
		return SrcCompleted + m_DstStart - m_DstPos;
	}
}

/////////// CThroughProcessOperation
CThroughProcessOperation::CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc,
													ULONG Flags, LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, Flags, StatusString, OperationName)
	, m_NumberOfForwardPasses(1)
	, m_NumberOfBackwardPasses(0)
	, m_CurrentPass(1)
	, m_InputBuffer(new char[ThroughProcessBufferSize])
	, m_OutputBuffer(NULL)
	, m_UndoBuffer(NULL)
{
	try
	{
		m_OutputBuffer = new char[ThroughProcessBufferSize];
	}
	catch (std::bad_alloc &)
	{
		delete[] m_InputBuffer;
		throw;
	}
}

CThroughProcessOperation::CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc,
													ULONG Flags, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, Flags, StatusStringId, OperationNameId)
	, m_NumberOfForwardPasses(1)
	, m_NumberOfBackwardPasses(0)
	, m_CurrentPass(1)
	, m_InputBuffer(new char[ThroughProcessBufferSize])
	, m_OutputBuffer(NULL)
	, m_UndoBuffer(NULL)
{
	try
	{
		m_OutputBuffer = new char[ThroughProcessBufferSize];
	}
	catch (std::bad_alloc &)
	{
		delete[] m_InputBuffer;
		throw;
	}
}

CThroughProcessOperation::~CThroughProcessOperation()
{
	delete[] m_InputBuffer;
	delete[] m_OutputBuffer;
	delete[] m_UndoBuffer;
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

	int DstSampleSize = 1;
	int SrcSampleSize = 1;

	if (m_DstFile.IsOpen())
	{
		DstSampleSize = m_DstFile.SampleSize();
	}

	if (m_SrcFile.IsOpen())
	{
		SrcSampleSize = m_SrcFile.SampleSize();
	}

	if (m_CurrentPass > 0)
	{
		if (m_Flags & OperationContextPassFinished)
		{
			// re-intialize the pass position
			m_CurrentPass++;
			if (m_CurrentPass <= m_NumberOfForwardPasses)
			{
				if ( ! InitPass(m_CurrentPass))
				{
					return FALSE;
				}
			}
			m_Flags &= ~OperationContextPassFinished;
		}

		if (m_CurrentPass > m_NumberOfForwardPasses)
		{
			// operation with forward passes ended. Either run backward passes, or finish
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

	if (m_CurrentPass > 0)
	{
		//ASSERT(0 == ((m_DstEnd - m_DstPos) % DstSampleSize));
		SAMPLE_POSITION OperationBeginPos = m_DstPos;
		do
		{
			// if there is not enough data in the input buffer, read more
			const char * inbuf = NULL;
			unsigned BytesToProcess = 0;

			if (m_SrcFile.IsOpen())
			{
				if (m_InputBufferPutIndex - m_InputBufferGetIndex < (unsigned)SrcSampleSize)
				{
					memmove(m_InputBuffer, m_InputBuffer + m_InputBufferGetIndex, m_InputBufferPutIndex - m_InputBufferGetIndex);
					m_InputBufferPutIndex -= m_InputBufferGetIndex;
					m_InputBufferGetIndex = 0;
				}

				unsigned ToRead = ThroughProcessBufferSize - m_InputBufferPutIndex;
				if (ToRead > ThroughProcessBufferSize / 2)
				{
					if (ToRead > m_SrcEnd - m_SrcPos)
					{
						ToRead = unsigned(m_SrcEnd - m_SrcPos);
					}

					if (ToRead != 0)
					{
						long WasRead = m_SrcFile.ReadAt(m_InputBuffer + m_InputBufferPutIndex, ToRead, m_SrcPos);
						m_SrcPos += WasRead;
						m_InputBufferPutIndex += WasRead;

						if (WasRead == 0)
						{
							res = FALSE;
							break;
						}
					}
				}
				inbuf = m_InputBuffer + m_InputBufferGetIndex;
				BytesToProcess = m_InputBufferPutIndex - m_InputBufferGetIndex;
			}

			char * outbuf = NULL;
			unsigned BytesToStore = 0;
			unsigned OutputBufferFilled = m_OutputBufferPutIndex - m_OutputBufferGetIndex;

			if (m_DstFile.IsOpen())
			{
				outbuf = m_OutputBuffer + m_OutputBufferPutIndex;
				BytesToStore = ThroughProcessBufferSize - m_OutputBufferPutIndex;

				if (BytesToStore > m_DstEnd - m_DstPos - OutputBufferFilled)
				{
					BytesToStore = unsigned(m_DstEnd - m_DstPos - OutputBufferFilled);
				}
			}

			unsigned UsedBytes = 0;
			// virtual function which modifies the actual data:
			unsigned BytesStored = ProcessBuffer(inbuf, outbuf,
												BytesToProcess,
												BytesToStore, &UsedBytes,
												m_SrcPos - BytesToProcess - m_SrcStart,
												m_DstPos + OutputBufferFilled - m_DstStart,
												m_CurrentPass);

			m_OutputBufferPutIndex += BytesStored;
			m_InputBufferGetIndex += UsedBytes;
			OutputBufferFilled += BytesStored;

			if (BytesStored == 0 && UsedBytes == 0)
			{
				m_Flags |= OperationContextPassFinished;
			}
			// write data if it's more than half of the buffer
			if (m_DstFile.IsOpen()
				&& ((m_Flags & OperationContextPassFinished)    // flush in any case
					|| OutputBufferFilled >= ThroughProcessBufferSize/2))
			{
				if (1 == m_CurrentPass
					&& NULL != m_pUndoContext)
				{
					// save the data to be changed to undo buffer, but only on the first forward pass
					// Read previous data
					m_DstFile.ReadAt(m_UndoBuffer, OutputBufferFilled, m_DstPos);

					m_pUndoContext->SaveUndoData(m_UndoBuffer, OutputBufferFilled, m_DstPos);
				}

				if (OutputBufferFilled / DstSampleSize != (unsigned)m_DstFile.WriteSamples(m_DstChan, m_DstPos, OutputBufferFilled / DstSampleSize,
						m_OutputBuffer + m_OutputBufferGetIndex, m_DstChan, m_DstFile.Channels(), m_DstFile.GetSampleType()))
				{
					res = FALSE;
					break;
				}

				m_DstPos += OutputBufferFilled;

				m_OutputBufferPutIndex = 0;
				m_OutputBufferGetIndex = 0;

				if (m_DstPos >= m_DstEnd)
				{
					m_Flags |= OperationContextPassFinished;
				}
			}

		}
		while (res && 0 == (m_Flags & OperationContextPassFinished)
				&& GetTickCount() - dwStartTime < 1000);

		if (m_DstFile.IsOpen() && OperationBeginPos != m_DstPos)
		{
			// notify the view
			m_pDocument->FileChanged(m_DstFile, OperationBeginPos, m_DstPos);
		}
	}
	else
	{
		// backward pass
		if (m_Flags & OperationContextPassFinished)
		{
			m_CurrentPass--;
			if (m_CurrentPass >= -m_NumberOfBackwardPasses)
			{
				if ( ! InitPass(m_CurrentPass))
				{
					return FALSE;
				}
			}
			m_Flags &= ~OperationContextPassFinished;
		}

		if (m_CurrentPass < -m_NumberOfBackwardPasses)
		{
			m_Flags |= OperationContextFinished;
			return TRUE;
		}

		SAMPLE_POSITION OperationBeginPos = m_DstPos;
		do
		{
			// if there is not enough data in the input buffer, read more
			const char * inbuf = NULL;
			unsigned BytesToProcess = 0;

			if (m_SrcFile.IsOpen())
			{
				// the data is read and processed from the end toward begin of the buffer
				if (m_InputBufferGetIndex - m_InputBufferPutIndex < (unsigned)SrcSampleSize)
				{
					memmove(m_InputBuffer + ThroughProcessBufferSize - (m_InputBufferGetIndex - m_InputBufferPutIndex),
							m_InputBuffer + m_InputBufferPutIndex, m_InputBufferGetIndex - m_InputBufferPutIndex);

					m_InputBufferPutIndex += ThroughProcessBufferSize - m_InputBufferGetIndex;
					m_InputBufferGetIndex = ThroughProcessBufferSize;
				}

				unsigned ToRead = m_InputBufferPutIndex;
				if (ToRead > ThroughProcessBufferSize / 2)
				{
					if (ToRead > m_SrcPos - m_SrcStart)
					{
						ToRead = unsigned(m_SrcPos - m_SrcStart);
					}

					if (ToRead != 0)
					{
						long WasRead = m_SrcFile.ReadAt(m_InputBuffer + m_InputBufferPutIndex - ToRead, ToRead, m_SrcPos - ToRead);
						m_SrcPos -= WasRead;
						m_InputBufferPutIndex -= WasRead;

						if (WasRead != (long)ToRead)
						{
							res = FALSE;
							break;
						}
					}
				}
				inbuf = m_InputBuffer + m_InputBufferGetIndex;
				BytesToProcess = m_InputBufferGetIndex - m_InputBufferPutIndex;
			}

			char * outbuf = NULL;
			unsigned BytesToStore = 0;
			unsigned OutputBufferFilled = m_OutputBufferGetIndex - m_OutputBufferPutIndex;

			if (m_DstFile.IsOpen())
			{
				outbuf = m_OutputBuffer + m_OutputBufferPutIndex;
				BytesToStore = m_OutputBufferPutIndex;

				if (BytesToStore > m_DstPos - m_DstStart - OutputBufferFilled)
				{
					BytesToStore = unsigned(m_DstPos - m_DstStart - OutputBufferFilled);
				}
			}
			unsigned UsedBytes = 0;
			// virtual function which modifies the actual data:
			unsigned BytesStored = ProcessBuffer(inbuf, outbuf,
												BytesToProcess,
												BytesToStore, &UsedBytes,
												m_SrcPos + BytesToProcess - m_SrcStart,
												m_DstPos - OutputBufferFilled - m_DstStart,
												m_CurrentPass);

			m_OutputBufferPutIndex -= BytesStored;
			m_InputBufferGetIndex -= UsedBytes;
			OutputBufferFilled += BytesStored;

			if (BytesStored == 0 && UsedBytes == 0)
			{
				m_Flags |= OperationContextPassFinished;
			}
			// write data if it's more than half of the buffer
			if (m_DstFile.IsOpen()
				&& ((m_Flags & OperationContextPassFinished)    // flush in any case
					|| OutputBufferFilled >= ThroughProcessBufferSize/2))
			{
				m_DstPos -= OutputBufferFilled;
				if (OutputBufferFilled / DstSampleSize != (unsigned)m_DstFile.WriteSamples(m_DstChan, m_DstPos, OutputBufferFilled / DstSampleSize,
						m_OutputBuffer + m_OutputBufferPutIndex, m_DstChan, m_DstFile.Channels(), m_DstFile.GetSampleType()))
				{
					res = FALSE;
					break;
				}

				m_OutputBufferPutIndex = ThroughProcessBufferSize;
				m_OutputBufferGetIndex = ThroughProcessBufferSize;

				if (m_DstPos <= m_DstStart)
				{
					m_Flags |= OperationContextPassFinished;
				}
			}

		}
		while (res && 0 == (m_Flags & OperationContextPassFinished)
				&& GetTickCount() - dwStartTime < 1000);

		if (m_DstFile.IsOpen() && OperationBeginPos != m_DstPos)
		{
			// notify the view
			m_pDocument->FileChanged(m_DstFile, m_DstPos, OperationBeginPos);
		}
	}
	return res;
}

MEDIA_FILE_SIZE CThroughProcessOperation::GetTotalOperationSize() const
{
	return (m_NumberOfForwardPasses + m_NumberOfBackwardPasses)
		* (ULONGLONG(m_SrcEnd) - m_SrcStart + m_DstEnd - m_DstStart);
}

MEDIA_FILE_SIZE CThroughProcessOperation::GetCompletedOperationSize() const
{
	if (m_CurrentPass < 0)
	{
		// backward pass
		return (m_NumberOfForwardPasses - m_CurrentPass) * (ULONGLONG(m_SrcEnd) - m_SrcStart + m_DstEnd - m_DstStart)
			- (ULONGLONG(m_SrcPos) - m_SrcStart + m_DstPos - m_DstStart);
	}
	else
	{
		return (m_CurrentPass - 1) * (ULONGLONG(m_SrcEnd) - m_SrcStart + m_DstEnd - m_DstStart)
			+ (ULONGLONG(m_SrcPos) - m_SrcStart + m_DstPos - m_DstStart);
	}
}

BOOL CThroughProcessOperation::Init()
{
	return InitPass(1);
}

BOOL CThroughProcessOperation::InitPass(int nPass)
{
	if (nPass >= 0)
	{
		m_DstPos = m_DstStart;
		m_SrcPos = m_SrcStart;
		m_InputBufferGetIndex = 0;
		m_InputBufferPutIndex = 0;
		m_OutputBufferGetIndex = 0;
		m_OutputBufferPutIndex = 0;
	}
	else
	{
		m_DstPos = m_DstEnd;
		m_SrcPos = m_SrcEnd;
		m_InputBufferGetIndex = ThroughProcessBufferSize;
		m_InputBufferPutIndex = ThroughProcessBufferSize;
		m_OutputBufferGetIndex = ThroughProcessBufferSize;
		m_OutputBufferPutIndex = ThroughProcessBufferSize;
	}
	return TRUE;
}

BOOL CThroughProcessOperation::CreateUndo()
{
	m_UndoBuffer = new char[ThroughProcessBufferSize];
	return BaseClass::CreateUndo();
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
		AddContext(new CInitChannels(m_pDocument, File, NewSamples, NumberOfSamples, Channel));
	}
	else
	{
		// 3. If all channels moved: Move all markers and regions
		// TODO

		// 4. If all channels moved: Change number of samples
		AddContext(new CWaveSamplesChangeOperation(m_pDocument, File, NewSamples));
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
	, m_bSavePeakFile(bSavePeaks)
	, m_PosSample(0)
	, m_EndSample(0)
{
	m_OriginalFile = OriginalFile;
	m_SrcFile = WavFile;
}

BOOL CScanPeaksContext::Init()
{
	if (!BaseClass::Init())
	{
		return FALSE;
	}

	m_PosSample = 0;
	m_EndSample = m_SrcFile.NumberOfSamples();

	if (m_SrcFile.AllocatePeakData(m_EndSample))
	{
		return TRUE;
	}
	NotEnoughMemoryMessageBox();
	return FALSE;
}

BOOL CScanPeaksContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_PosSample >= m_EndSample)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD dwStartTime = GetTickCount();
	unsigned Granularity = m_SrcFile.GetPeakGranularity();
	SAMPLE_INDEX Current = m_PosSample & (0 - Granularity);

	do
	{
		SAMPLE_INDEX end = Current + Granularity * 16;
		if (end > m_EndSample)
		{
			end = m_EndSample;
		}
		m_SrcFile.RescanPeaks(Current, end);
		Current = end;
	}
	while (Current < m_EndSample
			&& GetTickCount() - dwStartTime < 1000);

	if (0) TRACE("CScanPeaksContext current sample=%d\n", Current);

	// notify the view
	m_pDocument->FileChanged(m_SrcFile, m_SrcFile.SampleToPosition(m_PosSample),
							m_SrcFile.SampleToPosition(Current), -1, UpdateSoundDontRescanPeaks);

	m_PosSample = Current;
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

	LONG32 tmp[MAX_NUMBER_OF_CHANNELS];
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
				m_pUndoContext->SaveUndoData(pDstBuf, Samples * DstSampleSize, m_DstPos);
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

				m_pUndoContext->SaveUndoData(tmp, DstSampleSize, m_DstPos);
			}

			if (1 != m_SrcFile.ReadSamples(ALL_CHANNELS,
											m_SrcPos, 1, tmp, m_DstFile.GetSampleType())
				|| 1 != m_DstFile.WriteSamples(m_DstChan, m_DstPos, 1,
												tmp, m_SrcChan, NumSrcChannels, m_DstFile.GetSampleType()))
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
			length = (long)Position;
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
BOOL CCopyUndoContext::InitUndoCopy(CWaveFile & SrcFile,     // the file with the data to be saved
									SAMPLE_POSITION SaveStartPos, // source file position of data needed to save and restore
									SAMPLE_POSITION SaveEndPos,
									CHANNEL_MASK SaveChannel,     // 1 or 2 - one channel, 3 - two channels, etc
									SAMPLE_POSITION RedoStartPos, // source file position of data needed to redo
									SAMPLE_POSITION RedoEndPos)
{
	ASSERT(SrcFile.IsOpen());

	m_DstStart = SaveStartPos;
	m_DstPos = SaveStartPos;
	m_DstEnd = SaveEndPos;
	m_SrcSampleSize = SrcFile.SampleSize();
	m_SrcNumberOfChannels = SrcFile.Channels();

	if (SaveEndPos >= SaveStartPos)
	{
		if ( ! m_SrcFile.CreateWaveFile( & SrcFile, NULL,
										SaveChannel,
										(SaveEndPos - SaveStartPos) / SrcFile.SampleSize(),
										CreateWaveFileTempDir
										| CreateWaveFileDeleteAfterClose
										| CreateWaveFileDontCopyInfo
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
BOOL CCopyUndoContext::SaveUndoData(void const * pBuf, long BufSize, SAMPLE_POSITION Position)
{
	ASSERT(0 == BufSize % m_SrcSampleSize);

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
			BufSize += long(Position - m_DstPos);
			pSrcBuf -= Position - m_DstPos;
			Position = m_DstPos;
		}
		else
		{
			ASSERT(Position == m_DstPos);
		}
		ASSERT(0 == (m_DstStart - Position) % m_SrcSampleSize);
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
			BufSize = long(m_DstEnd - Position);
		}
		if (Position < m_DstPos)
		{
			BufSize -= long(m_DstPos - Position);
			pSrcBuf += m_DstPos - Position;
			Position = m_DstPos;
		}
		else
		{
			ASSERT(Position == m_DstPos);
		}
		ASSERT(0 == (Position - m_DstStart) % m_SrcSampleSize);
	}

	ASSERT(0 == (BufSize % m_SrcSampleSize));

	NUMBER_OF_SAMPLES const Samples = BufSize / m_SrcSampleSize;

	long SamplesWritten = m_SrcFile.WriteSamples(ALL_CHANNELS,
												m_SrcPos, Samples, pSrcBuf, m_DstChan, m_SrcNumberOfChannels,
												m_SrcFile.GetSampleType());

	m_DstPos += SamplesWritten * m_SrcSampleSize;
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
					m_SrcFile.GetWaveFormat().FormatTag(), m_MmResult);
			m_pDocument->m_bCloseThisDocumentNow = true;
		}
		else
		{
			s.Format(IDS_ERROR_WHILE_DECOMPRESSING_FILE,
					LPCTSTR(m_SrcFile.GetName()), m_MmResult);
		}
		AfxMessageBox(s, MB_ICONSTOP);
		m_Flags &= ~PostRetireSavePeakFile;
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

	if (m_DstFile.GetWaveFormat().IsPcm())
	{
		SAMPLE_INDEX nFirstSample = m_DstFile.PositionToSample(dwOperationBegin);
		SAMPLE_INDEX nLastSample = m_DstFile.PositionToSample(m_DstPos);
		// check if we need to change file size
		NUMBER_OF_SAMPLES TotalSamples = -1;

		if (nLastSample > m_CurrentSamples)
		{
			// calculate new length
			NUMBER_OF_SAMPLES MaxNumberOfSamples = 0x7FFFFFFF / m_DstFile.SampleSize();

			TotalSamples = (nLastSample * (m_SrcEnd - m_SrcStart)) / (m_SrcPos - m_SrcStart);

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
									INT PlaybackDevice, int PlaybackBuffers, unsigned PlaybackBufferSize)
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
	// TODO: Try different formats to play
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
	TRACE(L"CSoundPlayContext::Init m_WaveOut.Open()\n");
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
	TRACE(L"CSoundPlayContext::DeInit m_WaveOut.Reset()\n");

	m_WaveOut.Close();
	TRACE(L"CSoundPlayContext::DeInit m_WaveOut.Close()\n");

	BaseClass::DeInit();
}

void CSoundPlayContext::PostRetire()
{
	m_pDocument->m_PlayingSound = false;
	m_pDocument->m_pSoundPlayContext = NULL;

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
		unsigned size;
		int nBuf = m_WaveOut.GetBuffer( & pBuf, & size, FALSE);

		if (nBuf <= 0)
		{
			// buffer not available yet
			//m_Flags |= OperationContextYield;
			Sleep(50);
			break;
		}
		else
		{
			NUMBER_OF_SAMPLES Samples = NUMBER_OF_SAMPLES(size / m_Wf.SampleSize());
			NUMBER_OF_SAMPLES SrcSamples =
				NUMBER_OF_SAMPLES((m_End - m_CurrentPlaybackPos) / m_PlayFile.SampleSize());

			if (0 == SrcSamples)
			{
				m_CurrentPlaybackPos = m_End;
				break;
			}
			if (Samples > SrcSamples)
			{
				Samples = SrcSamples;
			}

			if (Samples != (NUMBER_OF_SAMPLES)m_PlayFile.ReadSamples(m_Chan, m_CurrentPlaybackPos,
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
CWaveProcContext * CreateVolumeChangeOperation(CWaveSoapFrontDoc * pDoc,
												UINT StatusStringId, UINT OperationNameId,
												double const * VolumeArray, int VolumeArraySize)
{
	CWaveProcContext::auto_ptr pContext(new CWaveProcContext(pDoc, StatusStringId, OperationNameId));

	pContext->AddWaveProc(new CVolumeChangeProc(VolumeArray, VolumeArraySize));
	return pContext.release();
}

CWaveProcContext * CreateVolumeChangeOperation(CWaveSoapFrontDoc * pDoc,
												UINT StatusStringId, UINT OperationNameId,
												double Volume)
{
	return CreateVolumeChangeOperation(pDoc, StatusStringId, OperationNameId, &Volume, 1);
}

CScanContext::CScanContext(CWaveSoapFrontDoc * pDoc,
							UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
	m_ProcBatch.m_bAutoDeleteProcs = FALSE;
	m_ProcBatch.AddWaveProc(&m_Proc);
}

CScanContext::CScanProc::CScanProc()
{
	m_InputSampleType = SampleTypeFloat32;

	for (unsigned i = 0; i != countof(m_Sum); i++)
	{
		m_Sum[i] = 0;
		m_MinSample[i] = 1000000.;
		m_MaxSample[i] = -1000000.;
	}
}

void CScanContext::CScanProc::ProcessSampleValue(void const * pInSample, void * /*pOutSample*/, unsigned channel)
{
	double sample = *(float const*)pInSample;
	m_Sum[channel] += sample;

	if (m_MinSample[channel] > sample)
	{
		m_MinSample[channel] = sample;
	}
	if (m_MaxSample[channel] < sample)
	{
		m_MaxSample[channel] = sample;
	}
}

double CScanContext::CScanProc::GetMax(unsigned channel) const
{
	if (m_CurrentSample == 0)
	{
		return 0;
	}
	double max = 0;
	if (m_MaxSample[channel] > 0)
	{
		if (max < m_MaxSample[channel])
		{
			max = m_MaxSample[channel];
		}
	}
	else
	{
		if (max < -m_MaxSample[channel])
		{
			max = -m_MaxSample[channel];
		}
	}
	if (m_MinSample[channel] > 0)
	{
		if (max < m_MinSample[channel])
		{
			max = m_MinSample[channel];
		}
	}
	else
	{
		if (max < -m_MinSample[channel])
		{
			max = -m_MinSample[channel];
		}
	}
	return max;
}

double CScanContext::CScanProc::GetAverage(unsigned channel) const
{
	if (m_CurrentSample == 0)
	{
		return 0;
	}
	return m_Sum[channel] / m_CurrentSample;
}

CDcOffsetContext::CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									CScanContext * pScanContext)
	: BaseClass(pDoc, StatusStringId, OperationNameId),
	m_Proc(pScanContext)
{
	AddWaveProc(&m_Proc);
	m_ProcBatch.m_bAutoDeleteProcs = false;
}

CDcOffsetContext::CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									float const offset[], unsigned OffsetArraySize)
	: BaseClass(pDoc, StatusStringId, OperationNameId),
	m_Proc(NULL)
{
	ASSERT(OffsetArraySize <= countof (m_Proc.m_Offset));
	ASSERT(OffsetArraySize != 0);

	unsigned i;
	for (i = 0; i < OffsetArraySize; i++)
	{
		m_Proc.m_Offset[i] = offset[i];
	}
	for (; i < countof(m_Proc.m_Offset); i++)
	{
		m_Proc.m_Offset[i] = m_Proc.m_Offset[i-1];
	}

	AddWaveProc(&m_Proc);
	m_ProcBatch.m_bAutoDeleteProcs = false;
}

CDcOffsetContext::CDcOffsetProc::CDcOffsetProc(CScanContext * pScanContext)
	:m_pScanContext(pScanContext)
{
	m_InputSampleType = SampleTypeFloat32;
}

BOOL CDcOffsetContext::CDcOffsetProc::Init()
{
	if ( ! CWaveProc::Init())
	{
		return FALSE;
	}

	if (NULL != m_pScanContext)
	{
		for (NUMBER_OF_CHANNELS ch = 0; ch < MAX_NUMBER_OF_CHANNELS; ch ++)
		{
			m_Offset[ch] = (float)-m_pScanContext->GetAverage(ch);
		}
	}

	return TRUE;
}

void CDcOffsetContext::CDcOffsetProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel)
{
	*(float*)pOutSample = *(float const *)pInSample + m_Offset[channel];
}

CStatisticsContext::CStatisticsContext(CWaveSoapFrontDoc * pDoc,
										UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
{
	m_SrcChan = ALL_CHANNELS;

	AddWaveProc(&m_Proc);
	m_ProcBatch.m_bAutoDeleteProcs = false;
}

CStatisticsContext::CStatisticsProc::CStatisticsProc()
{
	for (unsigned i = 0; i < MAX_NUMBER_OF_CHANNELS; i++)
	{
		m_Stats[i].m_Energy = 0.;

		m_Stats[i].m_ZeroCrossing = 0;

		m_Stats[i].m_Min = INT_MAX;
		m_Stats[i].m_Max = INT_MIN;

		m_Stats[i].m_PrevSample = 0.;
		m_Stats[i].m_PrevSampleInt = 0;

		m_Stats[i].m_PosMin = 0;
		m_Stats[i].m_PosMax = 0;

		m_Stats[i].m_CurrentCrc = 0;
		m_Stats[i].m_CRC32 = 0;

		m_Stats[i].m_Sum = 0.;
	}
	m_TotalPosMin = 0;
	m_TotalPosMax = 0;
	m_CurrentCommonCRC = 0;
	m_CRC32Common = 0;

	m_Checksum = 0;
	m_ChecksumSampleNumber = 0;
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

void CStatisticsContext::CStatisticsProc::ProcessSampleValue(void const * pInSample, void * /*pOutSample*/, unsigned channel)
{
	ChannelStats* stats = &m_Stats[channel];
	UCHAR const * pUchar = (UCHAR const *)pInSample;
	switch (m_InputSampleType)
	{
	case SampleType16bit:
	{
		short sample = *(short const*)pInSample;
		stats->m_Sum += sample;
		stats->m_Energy += sample * sample;
		if (stats->m_Min > sample)
		{
			stats->m_Min = sample;
			stats->m_PosMin = m_CurrentSample;
		}
		if (stats->m_Max < sample)
		{
			stats->m_PosMax = m_CurrentSample;
			stats->m_Max = sample;
		}
		if (0x8000 & (sample ^ stats->m_PrevSampleInt))
		{
			stats->m_ZeroCrossing++;
		}
		stats->m_PrevSampleInt = sample;
		stats->m_CurrentCrc = CalcCrc32(stats->m_CurrentCrc, pUchar[0]);
		stats->m_CurrentCrc = CalcCrc32(stats->m_CurrentCrc, pUchar[1]);

		if (sample != 0)
		{
			stats->m_CRC32 = stats->m_CurrentCrc;
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
		break;
	case SampleType32bit:
	{
		long sample = *(long const*)pInSample;
		stats->m_Sum += sample;
		stats->m_Energy += double(sample) * sample;
		if (stats->m_Min > sample)
		{
			stats->m_Min = sample;
			stats->m_PosMin = m_CurrentSample;
		}
		if (stats->m_Max < sample)
		{
			stats->m_PosMax = m_CurrentSample;
			stats->m_Max = sample;
		}
		if (0x8000 & (sample ^ stats->m_PrevSampleInt))
		{
			stats->m_ZeroCrossing++;
		}
		stats->m_PrevSampleInt = sample;
		stats->m_CurrentCrc = CalcCrc32(stats->m_CurrentCrc, pUchar[0]);
		stats->m_CurrentCrc = CalcCrc32(stats->m_CurrentCrc, pUchar[1]);
		stats->m_CurrentCrc = CalcCrc32(stats->m_CurrentCrc, pUchar[2]);
		stats->m_CurrentCrc = CalcCrc32(stats->m_CurrentCrc, pUchar[3]);

		if (sample != 0)
		{
			stats->m_CRC32 = stats->m_CurrentCrc;
		}
	}
		break;
	case SampleTypeFloat32:
	{
		float sample = *(float const*)pInSample;
		if (_isnan(sample))
		{
			break;
		}
		stats->m_Sum += sample;
		stats->m_Energy += sample * sample;
		if (stats->m_Min > sample)
		{
			stats->m_Min = sample;
			stats->m_PosMin = m_CurrentSample;
		}
		if (stats->m_Max < sample)
		{
			stats->m_PosMax = m_CurrentSample;
			stats->m_Max = sample;
		}
		if ((sample < 0.) != (stats->m_PrevSample < 0.))
		{
			stats->m_ZeroCrossing++;
		}
		stats->m_PrevSample = sample;
	}
		break;
	}
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
			unsigned Channel;
			SAMPLE_INDEX Sample = dlg.GetMaxSamplePosition( & Channel);

			m_pDocument->SetSelection(Sample, Sample, 1 << Channel, Sample, SetSelection_MoveCaretToCenter);
		}
	}
	BaseClass::PostRetire();
}

SAMPLE_POSITION CStatisticsContext::CStatisticsProc::GetMaxSamplePosition(unsigned * pChannel) const
{
	SAMPLE_POSITION SamplePos = 0;
	unsigned Channel = 0;

	double value = fabs(m_Stats[0].m_Max);
	SamplePos = m_Stats[0].m_PosMax;

	for (unsigned i = 0; i < m_InputFormat.NumChannels(); i++)
	{
		ChannelStats const* stats = &m_Stats[i];

		SamplePos = stats->m_PosMax;

		if (fabs(stats->m_Min) > value)
		{
			value = fabs(fabs(stats->m_Min));
			SamplePos = stats->m_PosMin;
			Channel = i;
		}
		if (fabs(stats->m_Max) > value)
		{
			value = fabs(stats->m_Max);
			SamplePos = stats->m_PosMax;
			Channel = i;
		}
	}

	if (NULL != pChannel)
	{
		*pChannel = Channel;
	}

	return SamplePos;
}

BOOL CNormalizeContext::Init()
{
	// calculate normalization
	double Volume[MAX_NUMBER_OF_CHANNELS] = {1., 1.};
	int nChannels = m_DstFile.Channels();

	double MaxVolume = 1.;
	bool GotJob = false;
	double MaxLevel = 0;

	for (int ch = 0; ch < nChannels; ch++)
	{
		double Max = m_pScanContext->GetMax(ch);

		if (Max != 0)
		{
			Volume[ch] = double(32767. * m_LimitLevel / Max);

			if ((m_DstChan & (1 << ch))
				&& MaxLevel < Max)
			{
				MaxLevel = Max;
			}
		}
		else
		{
			Volume[ch] = 0.;
		}
	}

	if (0 != MaxLevel)
	{
		MaxVolume = 32767. * m_LimitLevel / MaxLevel;
	}

	if (MaxLevel == DoubleToShort(MaxVolume * MaxLevel))
	{
		MaxVolume = 1.;
	}

	for (int ch = 0; ch < nChannels; ch++)
	{
		if (m_DstChan & (1 << ch))
		{
			if (Volume[ch] != 0.)
			{
				if (m_bEqualChannels)
				{
					Volume[ch] = MaxVolume;
				}

				if (1. != Volume[ch])
				{
					GotJob = true;
				}
			}
		}
	}

	if (GotJob)
	{
		CVolumeChangeProc::auto_ptr VolumeChange(new CVolumeChangeProc(Volume, countof(Volume)));
		AddWaveProc(VolumeChange.release());
	}

	if ( ! BaseClass::Init())
	{
		return FALSE;
	}

	if ( ! GotJob)
	{
		// nothing to change
		m_Flags |= OperationContextFinished;
		DeleteUndo();
	}

	return TRUE;
}

CFileSaveContext::CFileSaveContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId,
									LPCTSTR NewName,
									CWaveFile & DstFile, CWaveFile & SrcFile, DWORD ContextFlags, DWORD NewFileTypeFlags)
	: BaseClass(pDoc, ContextFlags, StatusStringId, OperationNameId)
	, m_NewFileTypeFlags(NewFileTypeFlags)
	, m_NewName(NewName)
	, m_SrcFile(SrcFile)
	, m_DstFile(DstFile)
{
}

CFileSaveContext::CFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName,
									LPCTSTR NewName,
									CWaveFile & DstFile, CWaveFile & SrcFile, DWORD ContextFlags, DWORD NewFileTypeFlags)
	: BaseClass(pDoc, ContextFlags, StatusString, OperationName)
	, m_NewFileTypeFlags(NewFileTypeFlags)
	, m_NewName(NewName)
	, m_SrcFile(SrcFile)
	, m_DstFile(DstFile)
{
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
				WaveSampleType DstSampleType = m_DstFile.GetWaveFormat().GetSampleType();
				if (m_NewFileTypeFlags != 0
					|| DstSampleType != SampleType16bit
					|| DstSampleType != SampleType32bit
					|| DstSampleType != SampleTypeFloat32)
				{
					// only those types can be opened direct
					fmt = IDS_OPEN_SAVED_FILE_COPY_NONDIRECT;
					dlg.m_bDisableDirect = TRUE;
				}

				dlg.m_Prompt.Format(fmt, LPCTSTR(m_pDocument->GetTitle()),
									LPCTSTR(m_NewName));

				INT_PTR res;
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
	m_Flags &= ~OperationContextDiskIntensive;
	// delete the procs in the destructor
	m_ProcBatch.m_bAutoDeleteProcs = true;
}

// When the function creates UNDO, it also calls SetSaveForUndo to initialize range to save.
// If UNDO should be created later, the UNDO range should be initialized by calling SetSaveForUndo otherwise.
BOOL CWaveProcContext::InitInPlaceProcessing(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
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
		AddWaveProc(new CChannelConvertor(NewChannels));
		OldChannels = NewChannels;
	}

	if (pDstWf->nSamplesPerSec != pSrcWf->nSamplesPerSec)
	{
		AddWaveProc(new CResampleFilter(pDstWf->nSamplesPerSec, CResampleFilter::DefaultFilterLength, FALSE));

		m_DstFile.GetInstanceData()->RescaleMarkers(pSrcWf->nSamplesPerSec, pDstWf->nSamplesPerSec);
	}

	return TRUE;
}

unsigned CWaveProcContext::ProcessBuffer(char const * pInBuf, // if BACKWARD pass, points to the end of buffer
										char * pOutBuf,    // if BACKWARD pass, points to the end of buffer
										unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes,
										SAMPLE_POSITION /*SrcOffset*/,  // if BACKWARD pass, offset of the end of source buffer
										SAMPLE_POSITION /*DstOffset*/,  // if BACKWARD pass, offset of the end of destination buffer
										signed pass)
{
	m_ProcBatch.SetBackwardPass(pass < 0);
	ASSERT(m_Flags & OperationContextInitialized);
	// The data arrives exactly as read from file without conversion and without channel collapse
	return m_ProcBatch.ProcessSound(pInBuf, pOutBuf, nInBytes, nOutBytes, pUsedBytes);
	// The output buffer should be filled with the same number of channels as the destination file and with the target channel mask
}

BOOL CWaveProcContext::Init()
{
	if (m_SrcFile.IsOpen())
	{
		if ( ! m_ProcBatch.SetInputWaveformat(m_SrcFile.GetWaveFormat(), m_SrcChan))
		{
			return FALSE;
		}
	}
	if (m_DstFile.IsOpen())
	{
		if ( ! m_ProcBatch.SetOutputWaveformat(m_DstFile.GetWaveFormat(), m_DstChan))
		{
			return FALSE;
		}
	}
	if ( ! BaseClass::Init())
	{
		return FALSE;
	}
	return m_ProcBatch.Init();
}

BOOL CWaveProcContext::InitPass(int nPass)
{
	if ( ! BaseClass::InitPass(nPass))
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

void CConversionContext::DeInit()
{
	// For non-RIFF file, don't do post-processing
	MEDIA_FILE_POSITION DataOffset = m_DstFile.GetDataChunk()->dwDataOffset;
	if (DataOffset != 0)
	{
		// update data chunk and number of samples
		m_DstFile.SetFactNumberOfSamples(m_ProcBatch.GetOutputNumberOfSamples());

		m_DstFile.SetDatachunkLength(DWORD(m_DstPos - DataOffset));
	}

	BaseClass::DeInit();
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
	}
	while (m_Decoder.DeliverNextSample()
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
		if (0) TRACE("Changed from %d to %d, length=%d\n", nFirstSample, nLastSample, NewSampleCount);

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
	HRESULT hr;
	hr = m_CoInit.InitializeCom(COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		return FALSE;
	}

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

	// create the file in the main thread context
	class CreateWaveFile : public MainThreadCall
	{
	public:
		CreateWaveFile(CWmaDecodeContext * pContext)
			: m_pContext(pContext)
		{
		}
	protected:
		virtual LRESULT Exec()
		{
			CWaveSoapFrontDoc *pDoc = m_pContext->m_pDocument;
			if (!pDoc->m_WavFile.CreateWaveFile(NULL, m_pContext->m_Decoder.GetDstFormat(),
												ALL_CHANNELS, m_pContext->m_CurrentSamples,  // initial sample count
												CreateWaveFileTempDir
												| CreateWaveFileDeleteAfterClose
												| CreateWaveFileTemp, NULL))
			{
				AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
				//pDoc->m_bCloseThisDocumentNow = true;
				// will be done for InitFailed
				return FALSE;
			}

			pDoc->m_OriginalWaveFormat = m_pContext->m_Decoder.GetSrcFormat();
			pDoc->m_OriginalWavFile.SetWaveFormat(pDoc->m_OriginalWaveFormat);

			if (!pDoc->m_WavFile.LoadPeaksForOriginalFile(pDoc->m_OriginalWavFile, m_pContext->m_CurrentSamples))
			{
				// peak data will be created during conversion
				m_pContext->m_Flags |= PostRetireSavePeakFile;
			}

			pDoc->SoundChanged(pDoc->WaveFileID(), 0, 0, m_pContext->m_CurrentSamples, UpdateSoundDontRescanPeaks);
			pDoc->QueueSoundUpdate(pDoc->UpdateWholeFileChanged,
									pDoc->WaveFileID(), 0, 0, m_pContext->m_CurrentSamples);

			m_pContext->SetDstFile(pDoc->m_WavFile);
			return TRUE;
		}
		CWmaDecodeContext * const m_pContext;
	} call(this);

	if (FALSE == call.Call())
	{
		return FALSE;
	}


	if (S_OK == m_Decoder.Start())
	{
		return TRUE;
	}

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
		if (m_Flags & (OperationContextInitFailed | OperationContextStop))
		{
			m_pDocument->m_bCloseThisDocumentNow = true;
		}
		else
		{
			s.Format(IDS_ERROR_WHILE_DECOMPRESSING_FILE,
					LPCTSTR(m_pDocument->m_OriginalWavFile.GetName()), 0);
			AfxMessageBox(s, MB_ICONSTOP);
		}
		m_Flags &= ~PostRetireSavePeakFile;
	}
	else
	{
		// set the file length, according to the actual number of samples decompressed

		m_DstFile.SetFileLengthSamples(m_DstCopySample);

		m_pDocument->SoundChanged(m_DstFile.GetFileID(), 0, 0, m_DstCopySample, UpdateSoundDontRescanPeaks);
	}
	BaseClass::PostRetire();
}

//////////////// CWmaDecodeContext
CDirectShowDecodeContext::CDirectShowDecodeContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
													CDirectFile & SrcFile)
	: BaseClass(pDoc,
				// operation can be terminated by Close
				/*OperationContextDiskIntensive |*/ OperationContextNonCritical, StatusStringId)
	, m_OriginalFile(SrcFile)
	, m_CurrentLengthSamples(0)
	, m_DstWrittenSample(0)
	, m_LastSamplesUpdate(0)
	, m_LastTickCountUpdate(0)
	, m_LastLengthSamplesUpdate(0)
	, m_DshowDecoder(this)
	, m_StopRequested(false)
	, m_EndOfStream(false)
{
	m_UnknownDelegate = static_cast<IPin*>(&m_DshowDecoder);
}

CDirectShowDecodeContext::~CDirectShowDecodeContext()
{

}

HRESULT STDMETHODCALLTYPE CDirectShowDecodeContext::Receive(
															/* [in] */ IMediaSample *pSample)
{
	// write the data immediately
	FILTER_STATE filter_state = m_DshowDecoder.GetFilterState();
	if (filter_state != State_Paused
		&& filter_state != State_Running)
	{
		return VFW_E_WRONG_STATE;
	}

	LPBYTE pData = NULL;
	LONG Length = 0;
	HRESULT hr;
	hr = pSample->GetPointer(&pData);
	Length = pSample->GetActualDataLength();

	long Written = m_DstFile.WriteAt(pData, Length, m_DstPos);

	m_DstPos += Length;
	m_DstWrittenSample = m_DstFile.PositionToSample(m_DstPos);

	if (Written == Length)
	{
		m_pDocument->KickDocumentThread();
		return S_OK;
	}
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecodeContext::EndOfStream(void)
{
	m_EndOfStream = true;
	m_pDocument->KickDocumentThread();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecodeContext::Stop(void)
{
	m_pDocument->KickDocumentThread();
	return S_OK;
}

BOOL CDirectShowDecodeContext::OperationProc()
{
	if (m_EndOfStream
		|| (m_Flags & OperationContextStopRequested))
	{
		TRACE("CWmaDecodeContext::OperationProc: Stop Requested\n");
		if (!m_StopRequested)
		{
			m_StopRequested = true;
			m_DshowDecoder.StopDecode();
		}
		if (m_DshowDecoder.GetDecoderState() != m_DshowDecoder.DecoderStateStopped)
		{
			m_Flags |= OperationContextYield;
			return TRUE;
		}
		else
		{
			// Go forward - handle the decoder stop
		}
	}

	DWORD dwTime = GetTickCount();
	if (dwTime - m_LastTickCountUpdate < 200
		// force file update when the decoder is stopped
		&& m_DshowDecoder.GetDecoderState() != m_DshowDecoder.DecoderStateStopped)
	{
		m_Flags |= OperationContextYield;
		return TRUE;
	}
	m_LastTickCountUpdate = dwTime;
	// notify the view

	SAMPLE_INDEX nFirstSample = m_LastSamplesUpdate;
	SAMPLE_INDEX nLastSample = m_DstWrittenSample;
	m_LastSamplesUpdate = m_DstWrittenSample;

	NUMBER_OF_SAMPLES NewSampleCount = -1;

	if (m_DshowDecoder.GetDecoderState() == m_DshowDecoder.DecoderStateStopped)
	{
		if (m_Flags & OperationContextStopRequested)
		{
			m_Flags |= OperationContextStop;
		}
		else
		{
			m_Flags |= OperationContextFinished;
		}
		if (m_CurrentLengthSamples != m_DstWrittenSample)
		{
			NewSampleCount = m_DstWrittenSample;
			m_CurrentLengthSamples = m_DstWrittenSample;
		}
	}
	else if (m_DstWrittenSample > m_CurrentLengthSamples)
	{
		NewSampleCount = m_DstWrittenSample + 44100 * 30;	// add 30 seconds more
		m_CurrentLengthSamples = NewSampleCount;
		m_Flags |= OperationContextYield;
	}
	else
	{
		m_Flags |= OperationContextYield;
	}


	if (nFirstSample != nLastSample
		|| -1 != NewSampleCount)
	{
		if (0) TRACE("Changed from %d to %d, length=%d\n", nFirstSample, nLastSample, NewSampleCount);

		m_pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, NewSampleCount);
	}

	return TRUE;
}

BOOL CDirectShowDecodeContext::Init()
{
	if (!BaseClass::Init())
	{
		return FALSE;
	}
	HRESULT hr;
	hr = m_CoInit.InitializeCom(COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		return FALSE;
	}

	if (!m_DshowDecoder.Init())
	{
		return FALSE;
	}
	hr = m_DshowDecoder.Open(m_OriginalFile.GetName());
	if (FAILED(hr))
	{

		CString s;
		s.Format(IDS_CANT_OPEN_WMA_DECODER, m_OriginalFile.GetName());
		MessageBoxSync(s, MB_ICONEXCLAMATION | MB_OK);

		return FALSE;
	}

	m_CurrentLengthSamples = m_DshowDecoder.GetTotalSamples();

	// create the file in the main thread context
	class CreateWaveFile : public MainThreadCall
	{
	public:
		CreateWaveFile(CDirectShowDecodeContext * pContext)
			: m_pContext(pContext)
		{
		}
	protected:
		virtual LRESULT Exec()
		{
			CWaveSoapFrontDoc *pDoc = m_pContext->m_pDocument;
			if (!pDoc->m_WavFile.CreateWaveFile(NULL, m_pContext->m_DshowDecoder.GetDstFormat(),
												ALL_CHANNELS, m_pContext->m_CurrentLengthSamples,  // initial sample count
												CreateWaveFileTempDir
												| CreateWaveFileDeleteAfterClose
												| CreateWaveFileTemp, NULL))
			{
				AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
				//pDoc->m_bCloseThisDocumentNow = true;
				// will be done for InitFailed
				return FALSE;
			}

			m_pContext->SetDstFile(pDoc->m_WavFile);
			// the decoded data format will be used to write the peak file
			pDoc->m_OriginalWaveFormat = m_pContext->m_DshowDecoder.GetDstFormat();
			pDoc->m_OriginalWavFile.SetWaveFormat(pDoc->m_OriginalWaveFormat);
			if (!pDoc->m_WavFile.LoadPeaksForOriginalFile(pDoc->m_OriginalWavFile, m_pContext->m_CurrentLengthSamples))
			{
				// peak data will be created during conversion
				m_pContext->m_Flags |= PostRetireSavePeakFile;
			}
			pDoc->SoundChanged(pDoc->WaveFileID(), 0, 0, m_pContext->m_CurrentLengthSamples, UpdateSoundDontRescanPeaks);
			pDoc->QueueSoundUpdate(pDoc->UpdateWholeFileChanged,
									pDoc->WaveFileID(), 0, 0, m_pContext->m_CurrentLengthSamples);
			return TRUE;
		}
		CDirectShowDecodeContext * const m_pContext;
	} call(this);

	if (FALSE == call.Call())
	{
		return FALSE;
	}

	m_LastTickCountUpdate = GetTickCount();

	if (SUCCEEDED(m_DshowDecoder.StartDecode()))
	{
		return TRUE;
	}
	return FALSE;
}

void CDirectShowDecodeContext::DeInit()
{
	m_DshowDecoder.DeInit();
	m_CoInit.UninitializeCom();

	BaseClass::DeInit();
}

void CDirectShowDecodeContext::SetDstFile(CWaveFile & file)
{
	m_DstFile = file;
	m_DstWrittenSample = 0;
	m_DstStart = m_DstFile.SampleToPosition(0);
	m_DstPos = m_DstStart;
}

void CDirectShowDecodeContext::PostRetire()
{
	if (0 == (m_Flags & OperationContextFinished))
	{
		CString s;
		if (m_Flags & (OperationContextInitFailed | OperationContextStop))
		{
			m_pDocument->m_bCloseThisDocumentNow = true;
		}
		else
		{
			s.Format(IDS_ERROR_WHILE_DECOMPRESSING_FILE,
					LPCTSTR(m_pDocument->m_OriginalWavFile.GetName()), 0);
			AfxMessageBox(s, MB_ICONSTOP);
		}
		m_Flags &= ~PostRetireSavePeakFile;
	}
	else
	{
		// set the file length, according to the actual number of samples decompressed

		m_DstFile.SetFileLengthSamples(m_DstWrittenSample);

		m_pDocument->SoundChanged(m_DstFile.GetFileID(), m_LastSamplesUpdate, m_DstWrittenSample, m_DstWrittenSample, UpdateSoundDontRescanPeaks);
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
	ASSERT(m_SrcFile.GetSampleType() == SampleType16bit);

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

		unsigned SrcBufUsed = 0;

		unsigned DstBufUsed = m_ProcBatch.ProcessSound(pSrc, m_TmpBuffer + m_TmpBufferFilled, LeftToRead,
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

///////////////////////////////////////////////////////////////////////////////////////////////////
///////// CContextWorkerThread
////////////////////////////////////////////////////////

CContextWorkerThread::CContextWorkerThread(CThisApp * pApp, int Priority)
	: m_pApp(pApp)
	, m_pForegroundDocument(NULL)
	, m_RunThread(false)
	, m_Priority(Priority)
	, CWinThread(ThreadProc, this)
	, m_hThreadEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
{
	m_bAutoDelete = FALSE;
}

CContextWorkerThread::~CContextWorkerThread()
{
	CloseHandle(m_hThreadEvent);
}

void CContextWorkerThread::Kick()
{
	// set the event
	::SetEvent(m_hThreadEvent);
}

bool CContextWorkerThread::Start()
{
	m_RunThread = true;
	return FALSE != CreateThread(0, 0x10000);
}

bool CContextWorkerThread::Stop()
{
	if (m_hThread)
	{
		m_RunThread = false;

		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObjectAcceptSends(m_hThread, 20000))
		{
			TRACE("Terminating App Thread\n");
			TerminateThread(m_hThread, ~0UL);
			return false;
		}
	}
	return true;
}

void CContextWorkerThread::SetForegroundDocument(CDocument* pDoc)
{
	m_pForegroundDocument = pDoc;
}

void CContextWorkerThread::QueueOperation(COperationContext * pContext)
{
	m_OpList.InsertTail(pContext);
	pContext->SetBeginTime(m_hThread);
	SetEvent(m_hThreadEvent);
}

unsigned CContextWorkerThread::_ThreadProc()
{
	SetThreadPriority(m_Priority);

	bool NeedKickIdle = false;
	COperationContext * pLastContext = NULL;
	COperationContext * pYieldingContext = NULL;

	while (m_RunThread)
	{
		if (NeedKickIdle)
		{
			if (m_pApp->m_pMainWnd)
			{
				::SendMessage(m_pApp->m_pMainWnd->m_hWnd, UWM_RESET_LAST_STATUS_MESSAGE, 0, 0);
				if (::PostMessage(m_pApp->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0))
				{
					NeedKickIdle = false;    // otherwise keep bugging
				}
			}
		}
		COperationContext * pContext = NULL;
		if (!m_OpList.IsEmpty())
		{
			CSimpleCriticalSectionLock lock(m_OpList);
			// find if stop requested for any document

			for (pContext = m_OpList.First();
				m_OpList.NotEnd(pContext); pContext = m_OpList.Next(pContext))
			{
				if ((pContext->m_Flags & OperationContextStopRequested)
					|| pContext->m_pDocument->m_StopOperation)
				{
					break;
				}
			}

			if (m_OpList.IsEnd(pContext))
			{
				// Find if there is an operation for the active document, unless it's yielding
				for (pContext = m_OpList.First(); m_OpList.NotEnd(pContext); pContext = m_OpList.Next(pContext))
				{
					if (pContext != pYieldingContext
						&& pContext->m_pDocument == m_pForegroundDocument)
					{
						break;
					}
				}
				// But if it is clipboard operation,
				// the first clipboard op will be executed instead
				if (m_OpList.NotEnd(pContext)
					&& (pContext->m_Flags & OperationContextClipboard))
				{
					for (pContext = m_OpList.First(); m_OpList.NotEnd(pContext); pContext = m_OpList.Next(pContext))
					{
						if (pContext->m_Flags & OperationContextClipboard)
						{
							break;
						}
					}
				}
				if (m_OpList.IsEnd(pContext))
				{
					pContext = m_OpList.First();
				}
				if (pContext == pYieldingContext)
				{
					pContext = NULL;
				}
			}
		}

		if (pContext != pLastContext)
		{
			pLastContext = pContext;
			NeedKickIdle = true;
		}

		if (pContext == pYieldingContext)
		{
			pContext = NULL;
		}
		pYieldingContext = NULL;

		if (pContext != NULL)
		{
			// execute one step
			if (pContext->m_Flags & OperationContextSynchronous)
			{
				pContext->ExecuteSynch();
				pContext->m_Flags |= OperationContextFinished;
			}
			else if (0 == (pContext->m_Flags & OperationContextInitialized))
			{
				if (!pContext->Init())
				{
					pContext->m_Flags |= OperationContextInitFailed | OperationContextStop;
				}
				//SetCurrentStatusString(pContext->GetStatusString());
				pContext->m_Flags |= OperationContextInitialized;
				NeedKickIdle = true;
			}

			if (pContext->m_pDocument->m_StopOperation)
			{
				pContext->m_Flags |= OperationContextStopRequested;
			}

			int LastPercent = pContext->PercentCompleted();
			if (0 == (pContext->m_Flags & (OperationContextStop | OperationContextFinished)))
			{
				if (!pContext->OperationProc())
				{
					pContext->m_Flags |= OperationContextStop;
				}
			}

			int NewPercent = pContext->PercentCompleted();
			// signal for status update
			if (LastPercent != NewPercent)
			{
				NeedKickIdle = true;
			}

			if (pContext->m_Flags & (OperationContextStop | OperationContextFinished))
			{
				// remove the context from the list and delete the context
				m_OpList.RemoveEntry(pContext);

				bool ClipboardCreationAborted = (pContext->m_Flags & OperationContextWriteToClipboard)
												&& 0 == (pContext->m_Flags & OperationContextFinished);

				// send a signal to the document, that the operation completed
				m_pApp->SetStatusStringAndDoc(pContext->GetCompletedStatusString(),
											pContext->m_pDocument);

				pContext->DeInit();

				pContext->Retire();     // puts it in the document queue
				// send a signal to the document, that the operation completed
				NeedKickIdle = true;    // this will reenable all commands

				if (ClipboardCreationAborted)
				{
					// remove all operations that use the clipboard, to the next clipboard create operation
					for (pContext = m_OpList.First();
						m_OpList.NotEnd(pContext); )
					{
						COperationContext * pNext = m_OpList.Next(pContext);
						if (pContext->m_Flags & OperationContextWriteToClipboard)
						{
							break;
						}

						if (pContext->m_Flags & OperationContextClipboard)
						{
							m_OpList.RemoveEntry(pContext);
							pContext->Retire();
						}

						pContext = pNext;
					}
				}
				continue;
			}

			if (NeedKickIdle)
			{
				if (NewPercent >= 0)
				{
					CString s;
					s.Format(_T("%s %d%%"),
							(LPCTSTR)pContext->GetStatusString(), NewPercent);
					m_pApp->SetStatusStringAndDoc(s, pContext->m_pDocument);
				}
				else
				{
					m_pApp->SetStatusStringAndDoc(pContext->GetStatusString(), pContext->m_pDocument);
				}
				CString s;
			}
			if (pContext->m_Flags & OperationContextYield)
			{
				pContext->m_Flags &= ~OperationContextYield;
				pYieldingContext = pContext;
			}
			continue;
		}
		WaitForSingleObjectAcceptSends(m_hThreadEvent, INFINITE);
	}
	return 0;
}

