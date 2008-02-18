// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext.h
#if !defined(AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <mmreg.h>
#include <msacm.h>
#include "WaveSupport.h"
#include "WmaFile.h"
#include "KListEntry.h"
#include "CoInitHelper.h"
#include "Waveproc.h"

enum {
	OperationContextClipboard = 1,  // clipboard operations are serialized
	OperationContextSerialized = 1,  // clipboard operations are serialized
	OperationContextDiskIntensive = 2,  // only one disk intensive can be active
	OperationContextStopRequested = 4,    // cancel button pressed (set by the main thread)
	OperationContextStop = 8,  // the procedure is canceling
	OperationContextFinished = 0x10,    // the operation finished
	OperationContextSynchronous = 0x20,    // need to run in a main thread
	OperationContextInitialized = 0x40,
	OperationContextCommitFile = 0x80,
	OperationContextNonCritical = 0x100,
	OperationContextWriteToClipboard = 0x200,  // the operation write to clipboard

	OperationContextDontAdjustPriority = 0x400,
	OperationContextInitFailed = 0x800,
	OperationContextUndoing = 0x1000,

	// CreateUndo was called
	OperationContextUndoCreated = 0x2000,
	// when this operation was queued, modify count was incremented
	OperationContextModifyCountIncremented = 0x4000,
};

class COperationContext : public ListItem<COperationContext>
{
	typedef COperationContext ThisClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	COperationContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags, LPCTSTR StatusString, LPCTSTR OperationName = _T(""));
	COperationContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags, UINT StatusStringId = 0, UINT OperationNameId = 0);

	virtual ~COperationContext();

	virtual void Dump(unsigned indent=0) const;

	virtual BOOL OperationProc() = 0;

	virtual BOOL Init() { return TRUE; }

	virtual void DeInit() { }
	virtual void Retire();
	virtual void PostRetire();
	virtual void Execute();
	virtual void ExecuteSynch();

	virtual LONGLONG GetTempDataSize() const
	{
		return 0;
	}

	// The function returns UNDO context to store in the document list after
	// the main operation is completed
	virtual class CUndoRedoContext * GetUndo();
	// the function may prepare Undo/redo:
	virtual ListHead<COperationContext> * GetUndoChain()
	{
		return & m_UndoChain;
	}

	// the function is called to create UNDO context before starting the main operation
	virtual BOOL CreateUndo();
	// this function adds CSelectionChangeOperation UNDO context
	void AddSelectionUndo(SAMPLE_INDEX Start, SAMPLE_INDEX End, SAMPLE_INDEX Caret,
						CHANNEL_MASK Channels);
	// the function prepares the context which is part of UNDO
	// before doing the UNDO/REDO operation
	virtual BOOL PrepareUndo()
	{
		return TRUE;
	}
	// the function undoes any PrepareUndo, if the operation didn't go on
	virtual void UnprepareUndo() {}

	// reverses CreateUndo
	virtual void DeleteUndo();

	bool IsUndoOperation() const
	{
		return 0 != (m_Flags & OperationContextUndoing);
	}

	virtual bool KeepsPermanentFileReference() const
	{
		return false;
	}

	virtual CString GetStatusString() const;
	virtual CString GetOperationName() const;
	virtual CString GetCompletedStatusString() const;

	void SetOperationName(LPCTSTR str);
	void SetOperationName(UINT id);

	void SetStatusPrompt(LPCTSTR str);
	void SetStatusPrompt(UINT id);

	// this is name for the operation. It will be shown in Undo and Redo prompt
	CString m_OperationName;
	// it is status prompt. It is shown during operation execution
	CString m_StatusPrompt;

	class CWaveSoapFrontDoc * m_pDocument;
	DWORD m_Flags;

	// This list keeps all UNDO operations for this operation context.
	// they are created in advance, before the operation is executed
	// First item in the list is to be executed first
	// this means the latest item executed during operation,
	// gives the first UNDO item.
	ListHead<COperationContext> m_UndoChain;

	bool m_bClipped;
	double m_MaxClipped;

	__int16 DoubleToShort(double x)
	{
		long tmp = (long) floor(x + 0.5);
		if (tmp < -0x8000)
		{
			if (m_MaxClipped < -x)
			{
				m_MaxClipped = -x;
			}
			m_bClipped = TRUE;
			return -0x8000;
		}
		else if (tmp > 0x7FFF)
		{
			if (m_MaxClipped < x)
			{
				m_MaxClipped = x;
			}
			m_bClipped = TRUE;
			return 0x7FFF;
		}
		else
		{
			return __int16(tmp);
		}
	}

	__int16 LongToShort(long x)
	{
		if (x < -0x8000)
		{
			if (m_MaxClipped < -x)
			{
				m_MaxClipped = -x;
			}
			m_bClipped = TRUE;
			return -0x8000;
		}
		else if (x > 0x7FFF)
		{
			if (m_MaxClipped < x)
			{
				m_MaxClipped = x;
			}
			m_bClipped = TRUE;
			return 0x7FFF;
		}
		else
		{
			return __int16(x);
		}
	}

	virtual BOOL WasClipped() const
	{
		return m_bClipped;
	}
	virtual double GetMaxClipped() const
	{
		return m_MaxClipped;
	}

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const
	{
		return 0;
	}

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const
	{
		return 0;
	}

	virtual int PercentCompleted() const;

#ifdef _DEBUG
	FILETIME m_ThreadUserTime;
	DWORD m_BeginSystemTime;
	void SetBeginTime();
	void PrintElapsedTime();
#else
	void SetBeginTime() {}
	void PrintElapsedTime() {}
#endif
	// utility functions

};

class COneFileOperation : public COperationContext
{
	typedef COperationContext BaseClass;
	typedef COneFileOperation ThisClass;
public:
	COneFileOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags, LPCTSTR StatusString,
					LPCTSTR OperationName = _T(""));
	COneFileOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
					UINT StatusStringId = 0, UINT OperationNameId = 0);

	virtual void Dump(unsigned indent=0) const;

	virtual bool KeepsPermanentFileReference() const;
	virtual LONGLONG GetTempDataSize() const;

	void InitSource(CWaveFile & SrcFile, SAMPLE_INDEX StartSample,
					SAMPLE_INDEX EndSample, CHANNEL_MASK chan);

	CWaveFile m_SrcFile;
	CHANNEL_MASK m_SrcChan;
	SAMPLE_POSITION m_SrcStart;
	SAMPLE_POSITION m_SrcEnd;
	SAMPLE_POSITION m_SrcPos;

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;
};

class CTwoFilesOperation : public COneFileOperation
{
	typedef COneFileOperation BaseClass;
	typedef CTwoFilesOperation ThisClass;
public:
	CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags, LPCTSTR StatusString,
						LPCTSTR OperationName = _T(""));
	CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
						UINT StatusStringId, UINT OperationNameId = 0);

	CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags);

	~CTwoFilesOperation();

	virtual void Dump(unsigned indent=0) const;

	virtual LONGLONG GetTempDataSize() const;
	BOOL InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
						CHANNEL_MASK chan, BOOL NeedUndo,
						SAMPLE_INDEX StartUndoSample = 0,
						SAMPLE_INDEX EndUndoSample = LAST_SAMPLE);

	void SetSaveForUndo(SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample);

//    protected:
	CWaveFile m_DstFile;
	CHANNEL_MASK m_DstChan;
	SAMPLE_POSITION m_DstStart;
	SAMPLE_POSITION m_DstEnd;
	SAMPLE_POSITION m_DstPos;

	SAMPLE_POSITION m_UndoStartPos;
	SAMPLE_POSITION m_UndoEndPos;
protected:
	class CCopyUndoContext * m_pUndoContext;

	virtual BOOL CreateUndo();
	virtual ListHead<COperationContext> * GetUndoChain();
	virtual void DeleteUndo();
	virtual void DeInit();

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;
	virtual bool KeepsPermanentFileReference() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;
};

class CThroughProcessOperation : public CTwoFilesOperation
{
	typedef CTwoFilesOperation BaseClass;
	typedef CThroughProcessOperation ThisClass;
public:
	CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags, LPCTSTR StatusString,
							LPCTSTR OperationName = _T(""));
	CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
							UINT StatusStringId = 0, UINT OperationNameId = 0);

	typedef std::auto_ptr<ThisClass> auto_ptr;

	virtual BOOL Init();
	virtual BOOL InitPass(int nPass);

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE) = 0;

	int m_NumberOfForwardPasses;
	int m_NumberOfBackwardPasses;
	int m_CurrentPass;
	int m_GetBufferFlags;
	int m_ReturnBufferFlags;

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;
};

// additional custom flags for the contexts
enum {
	FileSaveContext_SavingCopy    = 0x40000000,
	FileSaveContext_SameName    =   0x20000000,
	FileSaveContext_DontPromptReopen =   0x10000000,
	FileSaveContext_DontSavePeakInfo =   0x08000000,
	FileSaveContext_SavingPartial    = 0x04000000,
	DecompressSavePeakFile = 0x20000000,
};

class CStagedContext : public COperationContext
{
	typedef COperationContext BaseClass;
	typedef CStagedContext ThisClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CStagedContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags, LPCTSTR StatusString,
					LPCTSTR OperationName = _T(""));
	CStagedContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags,
					UINT StatusStringId = 0, UINT OperationNameId = 0);

	virtual ~CStagedContext();

	virtual BOOL OperationProc();
	virtual ListHead<COperationContext> * GetUndoChain();

	virtual void PostRetire();
	virtual CString GetStatusString() const;
	virtual void Execute();
	virtual BOOL CreateUndo();
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();
	virtual void DeleteUndo();

	virtual void Dump(unsigned indent=0) const;

	void AddContext(COperationContext * pContext);
	void AddContextInFront(COperationContext * pContext);
	virtual LONGLONG GetTempDataSize() const;
	virtual bool KeepsPermanentFileReference() const;

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;

	BOOL InitInsertCopy(CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
						NUMBER_OF_SAMPLES LengthToReplace, CHANNEL_MASK DstChannel,
						CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
						NUMBER_OF_SAMPLES SamplesToInsert, CHANNEL_MASK SrcChannel);

	BOOL InitExpandOperation(CWaveFile & File, SAMPLE_INDEX StartSample,
							NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);
	// delete area from StartSample to StartSample+Length
	BOOL InitShrinkOperation(CWaveFile & File, SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);

	void InitCopyMarkers(CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
						NUMBER_OF_SAMPLES LengthToReplace,
						CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
						NUMBER_OF_SAMPLES SamplesToInsert);

	void InitMoveMarkers(CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
						NUMBER_OF_SAMPLES LengthToReplace,
						NUMBER_OF_SAMPLES SamplesToInsert);

protected:
	void RetireAllChildren();
	// This list keeps items to execute. First() is executed first
	ListHead<COperationContext> m_ContextList;
	// This list keeps items done. First() was executed first, Last was executed last
	ListHead<COperationContext> m_DoneList;
	// this is total size of all completed operations
	MEDIA_FILE_SIZE m_DoneSize;
};

class CScanPeaksContext : public COneFileOperation
{
	typedef CScanPeaksContext ThisClass;
	typedef COneFileOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CScanPeaksContext(CWaveSoapFrontDoc * pDoc,
					CWaveFile & WavFile,
					CWaveFile & OriginalFile,
					BOOL bSavePeaks);
	//~CScanPeaksContext() {}
protected:
	CWaveFile m_OriginalFile;
	int m_GranuleSize;
	BOOL m_bSavePeakFile;
	virtual BOOL OperationProc();
	virtual void PostRetire();
	virtual BOOL Init();
};

class CCopyContext : public CTwoFilesOperation
{
	typedef CCopyContext ThisClass;
	typedef CTwoFilesOperation BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CCopyContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName = _T(""));
	CCopyContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId = 0, UINT OperationNameId = 0);

	BOOL InitCopy(CWaveFile & DstFile,
				SAMPLE_INDEX DstStartSample, CHANNEL_MASK DstChannel,
				CWaveFile & SrcFile,
				SAMPLE_INDEX SrcStartSample, CHANNEL_MASK SrcChannel,
				NUMBER_OF_SAMPLES SrcDstLength);

protected:
	virtual void DeInit();
	virtual void PostRetire();

	virtual BOOL OperationProc();
};

class CCopyUndoContext : public CCopyContext
{
	typedef CCopyUndoContext ThisClass;
	typedef CCopyContext BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CCopyUndoContext(CWaveSoapFrontDoc * pDoc)
		: BaseClass(pDoc)
	{
	}

	BOOL InitUndoCopy(CWaveFile & SrcFile,
					SAMPLE_POSITION SaveStartPos, // source file position of data needed to save and restore
					SAMPLE_POSITION SaveEndPos,
					CHANNEL_MASK SaveChannel,
					SAMPLE_POSITION RedoStartPos = 0, // source file position of data needed to redo
					SAMPLE_POSITION RedoEndPos = LAST_SAMPLE_POSITION);

	//virtual BOOL CreateUndo();
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();
	//virtual void DeleteUndo();

	virtual BOOL SaveUndoData(void const * pBuf, long BufSize,
							SAMPLE_POSITION Position,
							NUMBER_OF_CHANNELS NumSrcChannels);

	virtual BOOL NeedToSaveUndo(SAMPLE_POSITION Position, long length);
};

class CUndoRedoContext : public CStagedContext
{
	typedef CUndoRedoContext ThisClass;
	typedef CStagedContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	bool IsUndoOperation() const
	{
		return 0 != (m_Flags & OperationContextUndoing);
	}

	CUndoRedoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName);
	CUndoRedoContext(CWaveSoapFrontDoc * pDoc, UINT OperationNameId);

	virtual class CUndoRedoContext * GetUndo();

	virtual void PostRetire();
	virtual CString GetStatusString() const;
	virtual CString GetOperationName() const;
};

class CSoundPlayContext : public COperationContext
{
	typedef CSoundPlayContext ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CSoundPlayContext(CWaveSoapFrontDoc * pDoc, CWaveFile & WavFile,
					SAMPLE_INDEX PlaybackStart, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK Channel,
					INT_PTR PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize);

	CString GetPlaybackTimeString(int TimeFormat) const;

	//virtual CString GetStatusString() const;
	virtual CString GetCompletedStatusString() const;

	void Pause()
	{
		m_bPauseRequested = true;
	}
protected:
	CWaveOut m_WaveOut;
	CWaveFile m_PlayFile;
	CWaveFormat m_Wf;

	SAMPLE_POSITION m_Begin;
	SAMPLE_POSITION m_End;
	SAMPLE_POSITION m_CurrentPlaybackPos;

	CHANNEL_MASK m_Chan;

	SAMPLE_INDEX m_SamplePlayed;
	SAMPLE_INDEX m_FirstSamplePlayed;
	SAMPLE_INDEX m_LastSamplePlayed;

	bool m_bPauseRequested;
	INT_PTR m_PlaybackDevice;
	int m_OldThreadPriority;
	int m_PlaybackBuffers;
	size_t m_PlaybackBufferSize;

	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire();
};

class CVolumeChangeContext : public CThroughProcessOperation
{
	typedef CVolumeChangeContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						UINT StatusStringId, UINT OperationNameId,
						double const * VolumeArray, int VolumeArraySize = 2);
	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						UINT StatusStringId, UINT OperationNameId,
						float Volume = 1.f);

protected:
	float m_Volume[MAX_NUMBER_OF_CHANNELS];

	//virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);

};

class CStatisticsContext : public CThroughProcessOperation
{
	typedef CStatisticsContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CStatisticsContext(CWaveSoapFrontDoc * pDoc,
						UINT StatusStringId, UINT OperationNameId = 0);

	int m_ZeroCrossingLeft;
	int m_ZeroCrossingRight;
	int m_PrevSampleLeft;
	int m_PrevSampleRight;
	int m_MinLeft;
	int m_MaxLeft;
	int m_MinRight;
	int m_MaxRight;
	SAMPLE_POSITION m_PosMinLeft;
	SAMPLE_POSITION m_PosMaxLeft;
	SAMPLE_POSITION m_PosMinRight;
	SAMPLE_POSITION m_PosMaxRight;
	LONGLONG m_EnergyLeft;
	LONGLONG m_EnergyRight;
	LONGLONG m_SumLeft;
	LONGLONG m_SumRight;
	DWORD m_CurrentLeftCrc;
	DWORD m_CRC32Left;
	DWORD m_CurrentRightCrc;
	DWORD m_CRC32Right;
	DWORD m_CurrentCommonCRC;
	DWORD m_CRC32Common;

	// sample number is limited at 256. If 0, checksum not started yet
	int m_ChecksumSampleNumber;
	DWORD m_Checksum;

	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward = FALSE);

	SAMPLE_INDEX GetMaxSamplePosition(CHANNEL_MASK * pChannel = NULL) const;

	virtual void PostRetire();
};

class CDcScanContext : public CThroughProcessOperation
{
	typedef CDcScanContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CDcScanContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId = 0, UINT OperationNameId = 0);

	int GetDc(int channel);

protected:
	LONGLONG m_Sum[MAX_NUMBER_OF_CHANNELS];

	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
};

class CDcOffsetContext : public CThroughProcessOperation
{
	typedef CDcOffsetContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					CDcScanContext * pScanContext);

	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					int offset[], unsigned OffsetArraySize = 2);

protected:
	int m_Offset[MAX_NUMBER_OF_CHANNELS];

	CDcScanContext * m_pScanContext;

	virtual BOOL Init();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);

};

class CMaxScanContext : public CThroughProcessOperation
{
	typedef CMaxScanContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CMaxScanContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId = 0);

	int GetMax(unsigned channel);

protected:
	int m_Max[MAX_NUMBER_OF_CHANNELS];

	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
};

class CNormalizeContext : public CVolumeChangeContext
{
	typedef CNormalizeContext ThisClass;
	typedef CVolumeChangeContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CNormalizeContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					double LimitLevel, BOOL EqualChannels,
					CMaxScanContext * pScanContext)
		: BaseClass(pDoc, StatusStringId, OperationNameId)
		, m_pScanContext(pScanContext)
		, m_LimitLevel(LimitLevel)
		, m_bEqualChannels(EqualChannels)
	{
	}

	double m_LimitLevel;
	BOOL m_bEqualChannels;
	CMaxScanContext * m_pScanContext;

	virtual BOOL Init();
};

class CWaveProcContext : public CTwoFilesOperation
{
	typedef CWaveProcContext ThisClass;
	typedef CTwoFilesOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CWaveProcContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId = 0, UINT OperationNameId = 0);

	virtual void Dump(unsigned indent) const
	{
		BaseClass::Dump(indent);
		m_ProcBatch.Dump(indent+1);
	}

	void AddWaveProc(CWaveProc * pProc, int index = -1)
	{
		m_ProcBatch.AddWaveProc(pProc, index);
	}

	BOOL InitInPlaceProcessing(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
								CHANNEL_MASK chan, BOOL NeedUndo);

	BOOL MakeCompatibleFormat(WAVEFORMATEX const * pSrcWf, WAVEFORMATEX const * pDstWf,
							CHANNEL_MASK ChannelsToUse);

	virtual BOOL WasClipped() const
	{
		return m_ProcBatch.WasClipped();
	}
	virtual double GetMaxClipped() const
	{
		return m_ProcBatch.GetMaxClipped();
	}
protected:
	virtual BOOL OperationProc();

	CBatchProcessing m_ProcBatch;
	virtual BOOL Init();
	virtual void DeInit();
};

// is used to convert the file to save it
class CConversionContext : public CWaveProcContext
{
	typedef CConversionContext ThisClass;
	typedef CWaveProcContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CConversionContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId = 0, UINT OperationNameId = 0);

	CConversionContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
						UINT OperationNameId,
						CWaveFile & SrcFile,
						CWaveFile & DstFile, BOOL RawDstFile, SAMPLE_INDEX SrcBegin = 0, SAMPLE_INDEX SrcEnd = LAST_SAMPLE);

	virtual void PostRetire();
};

class CDecompressContext : public CWaveProcContext
{
	typedef CDecompressContext ThisClass;
	typedef CWaveProcContext BaseClass;
	//friend class CWaveSoapFrontDoc;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CDecompressContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
						CWaveFile & SrcFile,
						CWaveFile & DstFile,
						SAMPLE_POSITION SrcStart,
						SAMPLE_POSITION SrcEnd,
						NUMBER_OF_SAMPLES NumSamples,
						WAVEFORMATEX const * pWf,
						BOOL SwapBytes = FALSE);

	~CDecompressContext()
	{
		DeInit();
	}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void PostRetire();

protected:

	MMRESULT m_MmResult;
	CWaveFormat m_Wf;
	NUMBER_OF_SAMPLES m_CurrentSamples;
};

class CFileSaveContext : public CStagedContext
{
	typedef CFileSaveContext ThisClass;
	typedef CStagedContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CFileSaveContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId = 0, UINT OperationNameId = 0)
		: BaseClass(pDoc, 0, StatusStringId, OperationNameId),
		m_NewFileTypeFlags(0)
	{
	}

	CFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, 0, StatusString, OperationName),
		m_NewFileTypeFlags(0)
	{
	}

	~CFileSaveContext()
	{
	}

	CString m_NewName;
	CWaveFile m_DstFile;
	CWaveFile m_SrcFile;
	DWORD m_NewFileTypeFlags;

	virtual void PostRetire();
};

class CWmaDecodeContext : public CTwoFilesOperation
{
	typedef CWmaDecodeContext ThisClass;
	typedef CTwoFilesOperation BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CWmaDecodeContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId,
					CDirectFile & rWmaFile);
	~CWmaDecodeContext();

protected:
	// opens m_Decoder, loads wave format to its SrcFile
	void SetDstFile(CWaveFile & file);
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire();

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const
	{
		return m_CurrentSamples;
	}

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const
	{
		return m_DstCopySample;
	}

private:
	NUMBER_OF_SAMPLES m_CurrentSamples;
	SAMPLE_INDEX m_DstCopySample;
	CWmaDecoder m_Decoder;
	CDirectFile & m_WmaFile;
	CoInitHelper m_CoInit;
};

class CWmaSaveContext : public CWaveProcContext
{
	typedef CWmaSaveContext ThisClass;
	typedef CWaveProcContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CWmaSaveContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId,
					CWaveFile & SrcFile, CWaveFile & DstFile,
					SAMPLE_INDEX SrcBegin, SAMPLE_INDEX SrcEnd);

	virtual BOOL Init();
	virtual void DeInit();
	virtual BOOL OperationProc();
	//BOOL SetTargetFormat(WAVEFORMATEX * pwf);
private:
	WmaEncoder m_Enc;
	static unsigned const m_TmpBufferSize = 0x10000;    // 64K
	char m_TmpBuffer[m_TmpBufferSize];
	unsigned m_TmpBufferFilled;

	CoInitHelper m_CoInit;
};
#endif // AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
