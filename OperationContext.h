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

class COperationContext : public ListItem<COperationContext>
{
public:
	COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, DWORD Flags, LPCTSTR OperationName = _T(""));
	virtual ~COperationContext();

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE) { return TRUE; }

	virtual BOOL Init() { return InitPass(1); }
	virtual BOOL InitPass(int nPass) { return TRUE; }

	virtual void DeInit() { }
	virtual void Retire();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual void Execute();

	virtual CString GetStatusString();
	CString m_OperationName;
	CString m_OperationString;

	// chained context is executed
	// after the primary context is successfully completed
	// but if the primary context is aborted, the chained one is just deleted
	COperationContext * m_pChainedContext;
	class CWaveSoapFrontDoc * pDocument;
	DWORD m_Flags;
	int PercentCompleted;
	class CUndoRedoContext * m_pUndoContext;

	int m_GetBufferFlags;
	int m_ReturnBufferFlags;
	int m_NumberOfForwardPasses;
	int m_NumberOfBackwardPasses;
	int m_CurrentPass;

	CWaveFile m_DstFile;
	CHANNEL_MASK m_DstChan;
	SAMPLE_POSITION m_DstStart;
	SAMPLE_POSITION m_DstEnd;
	SAMPLE_POSITION m_DstPos;

	CWaveFile m_SrcFile;
	CHANNEL_MASK m_SrcChan;
	SAMPLE_POSITION m_SrcStart;
	SAMPLE_POSITION m_SrcEnd;
	SAMPLE_POSITION m_SrcPos;

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

	BOOL InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
						CHANNEL_MASK chan, BOOL NeedUndo);
	void InitSource(CWaveFile & SrcFile, SAMPLE_INDEX StartSample,
					SAMPLE_INDEX EndSample, CHANNEL_MASK chan);

	void UpdateCompletedPercent();
	void UpdateCompletedPercent(SAMPLE_INDEX CurrentSample,
								SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample);
	void UpdateCompletedPercent(SAMPLE_POSITION CurrentPos,
								SAMPLE_POSITION StartPos, SAMPLE_POSITION EndPos);
#ifdef _DEBUG
	FILETIME m_ThreadUserTime;
	DWORD m_BeginSystemTime;
	void SetBeginTime();
	void PrintElapsedTime();
#else
	void SetBeginTime() {}
	void PrintElapsedTime() {}
#endif
};

enum {
	OperationContextClipboard = 1,  // clipboard operations are serialized
	OperationContextSerialized = 1,  // clipboard operations are serialized
	OperationContextDiskIntensive = 2,  // only one disk intensive can be active
	OperationContextStopRequested = 4,    // cancel button pressed (set by the main thread)
	OperationContextStop = 8,  // the procedure is canceling
	OperationContextFinished = 0x10,    // the operation finished
	OperationContextInterventionRequired = 0x20,    // need to run a modal dialog
	OperationContextInitialized = 0x40,
	OperationContextCreatingUndo = 0x80,
	OperationContextNonCritical = 0x100,
	//OperationContextDontKeepAfterRetire = 0x200,
	OperationContextDontAdjustPriority = 0x400,
	OperationContextInitFailed = 0x800,
};

// additional custom flags for the contexts
enum {
	CopyExpandFile = 0x80000000,
	CopyShrinkFile = 0x40000000,
	CopyCreatingUndo = 0x20000000,
	RedoContext = 0x10000000,
	UndoContextReplaceWholeFile = 0x08000000,
	ConvertContextReplaceWholeFile = 0x08000000,
	ContextScanning = 0x04000000,
	StatisticsContext_DcOnly = 0x02000000,
	StatisticsContext_MinMaxOnly = 0x01000000,
	UndoContextReplaceFormat = 0x00800000,
	FileSaveContext_SavingCopy    = 0x00400000,
	FileSaveContext_SameName    =   0x00200000,
	DecompressSavePeakFile = 0x00200000,
};

class CScanPeaksContext : public COperationContext
{
	typedef COperationContext BaseClass;
public:

	CScanPeaksContext(CWaveSoapFrontDoc * pDoc,
					CWaveFile & WavFile,
					CWaveFile & OriginalFile,
					BOOL bSavePeaks)
		: COperationContext(pDoc, _T("Scanning the file for peaks..."), OperationContextDiskIntensive, _T("Peak Scan"))
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

class CResizeContext : public COperationContext
{
	typedef COperationContext BaseClass;
	friend class CWaveSoapFrontDoc;
	// Start, End and position are in bytes

	BOOL ExpandProc();
	BOOL ShrinkProc();
public:
	BOOL InitUndoRedo(CString UndoStatusString);

	CString sOp;
	CResizeContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: COperationContext(pDoc, OperationName, OperationContextDiskIntensive),
		sOp(StatusString)
	{

	}
	~CResizeContext();
	BOOL InitExpand(CWaveFile & File, SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);
	BOOL InitShrink(CWaveFile & File, SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);
	virtual BOOL OperationProc();
	virtual CString GetStatusString();
};

class CCopyContext : public COperationContext
{
	typedef COperationContext BaseClass;
public:
	// Start, End and position are in bytes

	class CResizeContext * m_pExpandShrinkContext;

public:
	CString sOp;
	CCopyContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: COperationContext(pDoc, OperationName, OperationContextDiskIntensive),
		sOp(StatusString),
		m_pExpandShrinkContext(NULL)
	{

	}
	~CCopyContext();
	BOOL InitCopy(CWaveFile & DstFile,
				SAMPLE_INDEX DstStartSample, NUMBER_OF_SAMPLES DstLength, CHANNEL_MASK DstChannel,
				CWaveFile & SrcFile,
				SAMPLE_INDEX SrcStartSample, NUMBER_OF_SAMPLES SrcLength, CHANNEL_MASK SrcChannel
				);
	//BOOL InitExpand(LONG StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
};

class CUndoRedoContext : public CCopyContext
{
	typedef CCopyContext BaseClass;
public:
	SAMPLE_POSITION m_SrcSaveStart;
	SAMPLE_POSITION m_SrcSaveEnd;
	SAMPLE_POSITION m_SrcSavePos;
	SAMPLE_POSITION m_DstSavePos;
	CHANNEL_MASK m_SaveChan;
	WAV_FILE_SIZE m_RestoredLength;

	struct WavePeak * m_pOldPeaks;
	size_t m_OldWavePeakSize;
	size_t m_OldAllocatedWavePeakSize;
	int m_OldPeakDataGranularity;
	bool m_bOldDirectMode;
	WAVEFORMATEX m_OldWaveFormat;

	CUndoRedoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName)
		: CCopyContext(pDoc, _T(""), OperationName),
		m_pOldPeaks(NULL)
	{
	}

	BOOL InitUndoCopy(CWaveFile & SrcFile,
					SAMPLE_POSITION SaveStartPos, // source file position of data needed to save and restore
					SAMPLE_POSITION SaveEndPos,
					CHANNEL_MASK SaveChannel);
	BOOL SaveUndoData(void * pBuf, long BufSize, SAMPLE_POSITION Position, CHANNEL_MASK Channel);
	BOOL NeedToSave(SAMPLE_POSITION Position, size_t length);
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
	virtual void Execute();

	~CUndoRedoContext();
};

class CSoundPlayContext : public COperationContext
{
	typedef COperationContext BaseClass;
public:
	CWaveOut m_WaveOut;
	CWaveFile m_PlayFile;
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

public:
	CSoundPlayContext(CWaveSoapFrontDoc * pDoc, CWaveFile & WavFile,
					SAMPLE_INDEX PlaybackStart, SAMPLE_INDEX PlaybackEnd, CHANNEL_MASK Channel,
					int PlaybackDevice, int PlaybackBuffers, size_t PlaybackBufferSize);

	virtual ~CSoundPlayContext() {}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CVolumeChangeContext : public COperationContext
{
	typedef COperationContext BaseClass;
public:
	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName);
	~CVolumeChangeContext();
	float m_VolumeLeft;
	float m_VolumeRight;

	//virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);

};

class CDcOffsetContext : public COperationContext
{
	typedef COperationContext BaseClass;
public:
	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName);
	~CDcOffsetContext();
	int m_OffsetLeft;
	int m_OffsetRight;

	class CStatisticsContext * m_pScanContext;

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
	virtual CString GetStatusString();

};

class CStatisticsContext : public COperationContext
{
	typedef COperationContext BaseClass;
public:
	CStatisticsContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName);
	~CStatisticsContext() {}
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

	//virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward = FALSE);

	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CNormalizeContext : public CVolumeChangeContext
{
	typedef CVolumeChangeContext BaseClass;
public:
	CNormalizeContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, OperationName),
		m_pScanContext(NULL)
	{
	}
	virtual ~CNormalizeContext()
	{
		delete m_pScanContext;
	}
	double m_LimitLevel;
	BOOL m_bEqualChannels;
	CStatisticsContext * m_pScanContext;
	virtual BOOL OperationProc();
	virtual CString GetStatusString();
};

class CConversionContext : public CCopyContext
{
	typedef CCopyContext BaseClass;
public:
	CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName);

	CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString,
						LPCTSTR OperationName,
						CWaveFile & SrcFile,
						CWaveFile & DstFile);

	//~CConversionContext() {}

	void AddWaveProc(CWaveProc * pProc, int index = -1)
	{
		m_ProcBatch.AddWaveProc(pProc, index);
	}

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
	typedef CConversionContext BaseClass;
	friend class CWaveSoapFrontDoc;

public:
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

class CFileSaveContext : public CCopyContext
{
	typedef CCopyContext BaseClass;
public:
	CFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, OperationName),
		m_NewFileTypeFlags(0),
		m_pConvert(NULL)
	{
	}
	CConversionContext * m_pConvert;
	virtual ~CFileSaveContext()
	{
		delete m_pConvert;
	}
	CString m_NewName;
	DWORD m_NewFileTypeFlags;

	virtual BOOL Init();
	virtual void DeInit();
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual void Execute();
};

class CWmaDecodeContext: public COperationContext
{
	typedef COperationContext BaseClass;
public:
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
	typedef CConversionContext BaseClass;
public:
	CWmaSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: BaseClass(pDoc, StatusString, OperationName)
	{
	}
	~CWmaSaveContext()
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
