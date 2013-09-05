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
	OperationContextPassFinished = 0x8000,
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


// base class for CScanPeaksContext, CInitChannelsUndo, CTwoFilesOperation. Only has the input file. Doesn't provide OperationProc.
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

// base class for CCopyContext, CReverseOperation, CThroughProcessOperation, CWaveProcContext, CWmaDecodeContext
// it supports creating UNDO
// It doesn't have its own OperationProc, thus is leaves that for the derived classes
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

// this class operates on a buffer of data from one file to another, and also supports UNDO. The data could be processed in forward and backward directons
class CThroughProcessOperation : public CTwoFilesOperation
{
	typedef CTwoFilesOperation BaseClass;
	typedef CThroughProcessOperation ThisClass;
public:
	CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags, LPCTSTR StatusString,
							LPCTSTR OperationName = _T(""));
	CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
							UINT StatusStringId = 0, UINT OperationNameId = 0);
	~CThroughProcessOperation();
	typedef std::auto_ptr<ThisClass> auto_ptr;

	virtual BOOL Init();
	virtual BOOL InitPass(int nPass);

	virtual BOOL OperationProc();
	virtual unsigned ProcessBuffer(char const * pInBuf, // if BACKWARD pass, points to the end of buffer
									char * pOutBuf,    // if BACKWARD pass, points to the end of buffer
									unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes,
									SAMPLE_POSITION SrcOffset,  // if BACKWARD pass, offset of the end of source buffer
									SAMPLE_POSITION DstOffset,  // if BACKWARD pass, offset of the end of destination buffer
									signed pass) = 0;

	int m_NumberOfForwardPasses;
	int m_NumberOfBackwardPasses;
	int m_CurrentPass;
	int m_GetBufferFlags;
	int m_ReturnBufferFlags;

	enum { ThroughProcessBufferSize = 0x10000}; // 64K

	char * m_InputBuffer;
	char * m_OutputBuffer;
	char * m_UndoBuffer;
	unsigned m_InputBufferGetIndex;
	unsigned m_InputBufferPutIndex;
	unsigned m_OutputBufferGetIndex;
	unsigned m_OutputBufferPutIndex;

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;
};

// This is used to handle WaveProc operations in a batch
class CWaveProcContext : public CThroughProcessOperation
{
	typedef CWaveProcContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
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

	virtual unsigned ProcessBuffer(char const * pInBuf, // if BACKWARD pass, points to the end of buffer
									char * pOutBuf,    // if BACKWARD pass, points to the end of buffer
									unsigned nInBytes, unsigned nOutBytes, unsigned * pUsedBytes,
									SAMPLE_POSITION SrcOffset,  // if BACKWARD pass, offset of the end of source buffer
									SAMPLE_POSITION DstOffset,  // if BACKWARD pass, offset of the end of destination buffer
									signed pass);

	CBatchProcessing m_ProcBatch;
	virtual BOOL Init();
	virtual BOOL InitPass(int nPass);
	virtual void DeInit();
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
		: BaseClass(pDoc), m_SrcSampleSize(4), m_SrcNumberOfChannels(2)
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

	unsigned m_SrcSampleSize;
	NUMBER_OF_CHANNELS m_SrcNumberOfChannels;

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
					INT PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize);

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
	INT m_PlaybackDevice;
	int m_OldThreadPriority;
	int m_PlaybackBuffers;
	size_t m_PlaybackBufferSize;

	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire();
};

CWaveProcContext * CreateVolumeChangeOperation(CWaveSoapFrontDoc * pDoc,
												UINT StatusStringId, UINT OperationNameId,
												double const * VolumeArray, int VolumeArraySize);
CWaveProcContext * CreateVolumeChangeOperation(CWaveSoapFrontDoc * pDoc,
												UINT StatusStringId, UINT OperationNameId,
												double Volume);

CWaveProcContext * CreateFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
											CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
											NUMBER_OF_SAMPLES Length, BOOL UndoEnabled);

CWaveProcContext * CreateFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
											CWaveFile & SrcFile, SAMPLE_INDEX SrcBegin, CHANNEL_MASK SrcChannel,
											CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
											NUMBER_OF_SAMPLES Length, BOOL UndoEnabled);

class CStatisticsContext : public CWaveProcContext
{
	typedef CStatisticsContext ThisClass;
	typedef CWaveProcContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CStatisticsContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId = 0);

	SAMPLE_INDEX GetMaxSamplePosition(CHANNEL_MASK * pChannel = NULL) const
	{
		return m_DstFile.PositionToSample(m_Proc.GetMaxSamplePosition(pChannel));
	}

	virtual void PostRetire();

	class CStatisticsProc: public CWaveProc
	{
	public:
		CStatisticsProc();

		int m_ZeroCrossing[2];

		int m_PrevSample[2];

		int m_Min[2];
		int m_Max[2];

		SAMPLE_INDEX m_PosMin[2];
		SAMPLE_INDEX m_PosMax[2];
		LONGLONG m_Energy[2];
		LONGLONG m_Sum[2];
		DWORD m_CurrentCrc[2];
		DWORD m_CRC32[2];
		DWORD m_CurrentCommonCRC;
		DWORD m_CRC32Common;

		// sample number is limited at 256. If 0, checksum not started yet
		int m_ChecksumSampleNumber;
		DWORD m_Checksum;

		virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);
		SAMPLE_POSITION GetMaxSamplePosition(CHANNEL_MASK * pChannel = NULL) const;

	} m_Proc;

};

class CScanContext : public CWaveProcContext
{
	typedef CScanContext ThisClass;
	typedef CWaveProcContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CScanContext(CWaveSoapFrontDoc * pDoc,
				UINT StatusStringId = 0, UINT OperationNameId = 0);

	double GetMax(unsigned channel) const
	{
		return m_Proc.GetMax(channel);
	}
	double GetAverage(unsigned channel) const
	{
		return m_Proc.GetAverage(channel);
	}

protected:
	class CScanProc : public CWaveProc
	{
		typedef CScanProc ThisClass;
		typedef CWaveProc BaseClass;

	public:
		typedef std::auto_ptr<ThisClass> auto_ptr;

		CScanProc();

		virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);

		double GetMax(unsigned channel) const;
		double GetAverage(unsigned channel) const;

		double m_Sum[MAX_NUMBER_OF_CHANNELS];
		double m_MinSample[MAX_NUMBER_OF_CHANNELS];
		double m_MaxSample[MAX_NUMBER_OF_CHANNELS];
	} m_Proc;

};

class CDcOffsetContext : public CWaveProcContext
{
	typedef CDcOffsetContext ThisClass;
	typedef CWaveProcContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					CScanContext * pScanContext);

	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					float const offset[], unsigned OffsetArraySize = 2);

protected:

	class CDcOffsetProc: public CWaveProc
	{
	public:
		CDcOffsetProc(CScanContext * pScanContext);

		float m_Offset[MAX_NUMBER_OF_CHANNELS];
		CScanContext * m_pScanContext;
		virtual void ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel);
		virtual BOOL Init();
	} m_Proc;


};

class CNormalizeContext : public CWaveProcContext
{
	typedef CNormalizeContext ThisClass;
	typedef CWaveProcContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CNormalizeContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId,
					double LimitLevel, BOOL EqualChannels,
					CScanContext * pScanContext)
		: BaseClass(pDoc, StatusStringId, OperationNameId)
		, m_pScanContext(pScanContext)
		, m_LimitLevel(LimitLevel)
		, m_bEqualChannels(EqualChannels)
	{
	}

	double m_LimitLevel;
	BOOL m_bEqualChannels;
	CScanContext * m_pScanContext;

	virtual BOOL Init();
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
