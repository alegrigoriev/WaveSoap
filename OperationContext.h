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
	OperationContextNonCritical = 0x100,

	OperationContextDontAdjustPriority = 0x400,
	OperationContextInitFailed = 0x800,
	OperationContextUndoing = 0x1000,
};

class COperationContext : public ListItem<COperationContext>
{
	typedef COperationContext ThisClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, DWORD Flags, LPCTSTR OperationName = _T(""));
	virtual ~COperationContext();

	virtual BOOL OperationProc() = 0;

	virtual BOOL Init() { return TRUE; }

	virtual void DeInit() { }
	virtual void Retire();
	virtual void PostRetire(BOOL bChildContext = FALSE);
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
	virtual BOOL CreateUndo(BOOL IsRedo = FALSE)
	{
		return TRUE;
	}

	// the function prepares the context which is part of UNDO
	// before doing the UNDO/REDO operation
	virtual BOOL PrepareUndo()
	{
		return TRUE;
	}
	// the function undoes any PrepareUndo, if the operation didn't go on
	virtual void UnprepareUndo() {}

	virtual void DeleteUndo();

	bool IsUndoOperation() const
	{
		return 0 != (m_Flags & OperationContextUndoing);
	}

	virtual bool KeepsPermanentFileReference() const
	{
		return false;
	}

	virtual CString GetStatusString();

	CString m_OperationName;
	CString m_OperationString;
	CString sOp;

	class CWaveSoapFrontDoc * pDocument;
	DWORD m_Flags;

	int m_PercentCompleted;
	ListHead<COperationContext> m_UndoChain;

	int m_GetBufferFlags;
	int m_ReturnBufferFlags;

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

	void UpdateCompletedPercent(SAMPLE_INDEX CurrentSample,
								SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample);
	void UpdateCompletedPercent(SAMPLE_POSITION CurrentPos,
								SAMPLE_POSITION StartPos, SAMPLE_POSITION EndPos);

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const
	{
		return 0;
	}

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const
	{
		return 0;
	}

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
	COneFileOperation(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
					ULONG Flags, LPCTSTR OperationName = _T(""));

	virtual bool KeepsPermanentFileReference() const;
	virtual LONGLONG GetTempDataSize() const;

	void InitSource(CWaveFile & SrcFile, SAMPLE_INDEX StartSample,
					SAMPLE_INDEX EndSample, CHANNEL_MASK chan);

	using BaseClass::UpdateCompletedPercent;
	// add another overload
	void UpdateCompletedPercent();

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
	CTwoFilesOperation(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
						ULONG Flags, LPCTSTR OperationName = _T(""));

	virtual LONGLONG GetTempDataSize() const;
	BOOL InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
						CHANNEL_MASK chan, BOOL NeedUndo);

//    protected:
	class CCopyUndoContext * m_pUndoContext;

	CWaveFile m_DstFile;
	CHANNEL_MASK m_DstChan;
	SAMPLE_POSITION m_DstStart;
	SAMPLE_POSITION m_DstEnd;
	SAMPLE_POSITION m_DstPos;

};

class CThroughProcessOperation : public CTwoFilesOperation
{
	typedef CTwoFilesOperation BaseClass;
	typedef CThroughProcessOperation ThisClass;
public:
	CThroughProcessOperation(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
							ULONG Flags, LPCTSTR OperationName = _T(""));

	typedef std::auto_ptr<ThisClass> auto_ptr;

	virtual BOOL Init() { return InitPass(1); }
	virtual BOOL InitPass(int nPass) { return TRUE; }
	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE) = 0;

	int m_NumberOfForwardPasses;
	int m_NumberOfBackwardPasses;
	int m_CurrentPass;

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;
};

// additional custom flags for the contexts
enum {
	FileSaveContext_SavingCopy    = 0x00400000,
	FileSaveContext_SameName    =   0x00200000,
	DecompressSavePeakFile = 0x00200000,
};

class CStagedContext : public COperationContext
{
	typedef COperationContext BaseClass;
	typedef CStagedContext ThisClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CStagedContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, DWORD Flags, LPCTSTR OperationName = _T(""));
	virtual ~CStagedContext();

	virtual BOOL OperationProc();
	virtual ListHead<COperationContext> * GetUndoChain();

	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
	virtual void Execute();
	virtual BOOL CreateUndo(BOOL IsRedo = FALSE);
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();
	virtual void DeleteUndo();

	void AddContext(COperationContext * pContext);
	void AddContextInFront(COperationContext * pContext);
	virtual LONGLONG GetTempDataSize() const;
	virtual bool KeepsPermanentFileReference() const;

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const;

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const;

protected:
	ListHead<COperationContext> m_ContextList;
	ListHead<COperationContext> m_DoneList;
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
	//~CScanPeaksContext() {}
protected:
	CWaveFile m_OriginalFile;
	int m_GranuleSize;
	BOOL m_bSavePeakFile;
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual BOOL Init();
};

class CCopyContext : public CTwoFilesOperation
{
	typedef CCopyContext ThisClass;
	typedef CTwoFilesOperation BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CCopyContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, OperationContextDiskIntensive, OperationName)
	{
	}

	BOOL InitCopy(CWaveFile & DstFile,
				SAMPLE_INDEX DstStartSample, CHANNEL_MASK DstChannel,
				CWaveFile & SrcFile,
				SAMPLE_INDEX SrcStartSample, CHANNEL_MASK SrcChannel,
				NUMBER_OF_SAMPLES SrcDstLength);

	virtual BOOL CreateUndo(BOOL IsRedo = FALSE);
	//virtual void DeleteUndo();

	virtual BOOL OperationProc();
};

class CCopyUndoContext : public CCopyContext
{
	typedef CCopyUndoContext ThisClass;
	typedef CCopyContext BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CCopyUndoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, OperationName)
	{
	}

	BOOL InitUndoCopy(CWaveFile & SrcFile,
					SAMPLE_POSITION SaveStartPos, // source file position of data needed to save and restore
					SAMPLE_POSITION SaveEndPos,
					CHANNEL_MASK SaveChannel);

	//virtual BOOL CreateUndo(BOOL IsRedo = FALSE);
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

	CUndoRedoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName)
		: BaseClass(pDoc, _T(""), 0, OperationName)
	{
		m_Flags |= OperationContextUndoing;
	}

	virtual class CUndoRedoContext * GetUndo();

	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
};

class CSoundPlayContext : public COperationContext
{
	typedef CSoundPlayContext ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CSoundPlayContext(CWaveSoapFrontDoc * pDoc, CWaveFile & WavFile,
					SAMPLE_INDEX PlaybackStart, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK Channel,
					int PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize);
	CString GetPlaybackTimeString(int TimeFormat) const;

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
	int m_PlaybackDevice;
	int m_OldThreadPriority;
	int m_PlaybackBuffers;
	size_t m_PlaybackBufferSize;

	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CVolumeChangeContext : public CThroughProcessOperation
{
	typedef CVolumeChangeContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName,
						double const * VolumeArray, int VolumeArraySize = 2);
	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName,
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
						LPCTSTR StatusString, LPCTSTR OperationName);

	int m_ZeroCrossingLeft;
	int m_ZeroCrossingRight;
	int m_PrevSampleLeft;
	int m_PrevSampleRight;
	int m_MinLeft;
	int m_MaxLeft;
	int m_MinRight;
	int m_MaxRight;
	SAMPLE_INDEX m_PosMinLeft;
	SAMPLE_INDEX m_PosMaxLeft;
	SAMPLE_INDEX m_PosMinRight;
	SAMPLE_INDEX m_PosMaxRight;
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

	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CDcScanContext : public CThroughProcessOperation
{
	typedef CDcScanContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CDcScanContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName);

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
					LPCTSTR StatusString, LPCTSTR OperationName,
					CDcScanContext * pScanContext);

	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName,
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
					LPCTSTR StatusString, LPCTSTR OperationName);

	int GetMax(int channel);

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
					LPCTSTR StatusString, LPCTSTR OperationName,
					double LimitLevel, BOOL EqualChannels,
					CMaxScanContext * pScanContext)
		: BaseClass(pDoc, StatusString, OperationName)
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

class CConversionContext : public CCopyContext
{
	typedef CConversionContext ThisClass;
	typedef CCopyContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName);

	CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
						LPCTSTR OperationName,
						CWaveFile & SrcFile,
						CWaveFile & DstFile);

	void AddWaveProc(CWaveProc * pProc, int index = -1)
	{
		m_ProcBatch.AddWaveProc(pProc, index);
	}

	BOOL InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
						CHANNEL_MASK chan, BOOL NeedUndo);
	virtual BOOL WasClipped() const
	{
		return m_ProcBatch.WasClipped();
	}
	virtual double GetMaxClipped() const
	{
		return m_ProcBatch.GetMaxClipped();
	}
	virtual BOOL OperationProc();

	virtual void PostRetire(BOOL bChildContext = FALSE);
protected:
	CBatchProcessing m_ProcBatch;
	NUMBER_OF_SAMPLES m_CurrentSamples;
};

class CDecompressContext : public CConversionContext
{
	typedef CDecompressContext ThisClass;
	typedef CConversionContext BaseClass;
	//friend class CWaveSoapFrontDoc;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CDecompressContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
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
//    virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void PostRetire(BOOL bChildContext = FALSE);

protected:

	MMRESULT m_MmResult;
	CWaveFormat m_Wf;
};

class CFileSaveContext : public CStagedContext
{
	typedef CFileSaveContext ThisClass;
	typedef CStagedContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, 0, OperationName),
		m_NewFileTypeFlags(0)
	{
	}

	CString m_NewName;
	CWaveFile m_DstFile;
	CWaveFile m_SrcFile;
	DWORD m_NewFileTypeFlags;

	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CWmaDecodeContext : public CTwoFilesOperation
{
	typedef CWmaDecodeContext ThisClass;
	typedef CTwoFilesOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CWmaDecodeContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
					CDirectFile & rWmaFile)
		: BaseClass(pDoc, StatusString,
					// operation can be terminated by Close
					OperationContextDiskIntensive | OperationContextNonCritical),
		m_WmaFile(rWmaFile)
	{
	}
	~CWmaDecodeContext()
	{
		m_Decoder.Stop();
	}
protected:
	CWmaDecoder m_Decoder;
	// opens m_Decoder, loads wave format to its SrcFile
	void SetDstFile(CWaveFile & file);
	NUMBER_OF_SAMPLES m_CurrentSamples;
	SAMPLE_INDEX m_DstCopySample;
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);

private:
	CDirectFile & m_WmaFile;
	CoInitHelper m_CoInit;
};

class CWmaSaveContext : public CConversionContext
{
	typedef CWmaSaveContext ThisClass;
	typedef CConversionContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CWmaSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, OperationName)
	{
	}

	WmaEncoder m_Enc;

	virtual BOOL Init();
	virtual void DeInit();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
	virtual BOOL OperationProc();
	//BOOL SetTargetFormat(WAVEFORMATEX * pwf);
	virtual void PostRetire(BOOL bChildContext = FALSE);
private:
	CoInitHelper m_CoInit;
};
#endif // AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
