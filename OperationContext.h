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

class COperationContext : public KListEntry<COperationContext>
{
public:
	COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName, DWORD Flags);
	virtual ~COperationContext();

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE) { return TRUE; }

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

	int m_DstChan;
	CWaveFile m_DstFile;
	DWORD m_DstStart;
	DWORD m_DstEnd;
	DWORD m_DstCopyPos;

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

	BOOL InitDestination(CWaveFile & DstFile, long StartSample, long EndSample,
						int chan, BOOL NeedUndo);
#ifdef _DEBUG
	FILETIME m_ThreadUserTime;
	DWORD m_SystemTime;
	void SetBeginTime();
	void PrintElapsedTime();
#else
	void SetBeginTime() {}
	void PrintElapsedTime() {}
#endif
};

enum {
	OperationContextClipboard = 1,  // clipboard operations are serialized
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
	ScanPeaksSavePeakFile = 0x00200000,
	DecompressSavePeakFile = 0x00200000,
};

class CScanPeaksContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
public:
	DWORD m_Position;
	DWORD m_Start;
	DWORD m_End;
	int m_GranuleSize;
	CScanPeaksContext(CWaveSoapFrontDoc * pDoc)
		: COperationContext(pDoc, "Peak Scan", OperationContextDiskIntensive),
		m_Start(0), m_End(0), m_Position(0)
	{
		m_OperationString = "Scanning the file for peaks...";
	}
	~CScanPeaksContext() {}
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CResizeContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	DWORD m_SrcCopyPos;

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
	BOOL InitExpand(CWaveFile & File, LONG StartSample, LONG Length, int Channel);
	BOOL InitShrink(CWaveFile & File, LONG StartSample, LONG Length, int Channel);
	virtual BOOL OperationProc();
	virtual CString GetStatusString();
};

class CCopyContext : public COperationContext
{
public:
	CWaveFile m_SrcFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	int m_SrcChan;
	DWORD m_SrcCopyPos;

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
				LONG DstStartSample, LONG DstLength, LONG DstChannel,
				CWaveFile & SrcFile,
				LONG SrcStartSample, LONG SrcLength, LONG SrcChannel
				);
	void InitSource(CWaveFile & SrcFile, long StartSample,
					long EndSample, int chan);
	//BOOL InitExpand(LONG StartSample, LONG Length, int Channel);
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
};

class CUndoRedoContext : public CCopyContext
{
public:
	DWORD m_SrcSaveStart;
	DWORD m_SrcSaveEnd;
	DWORD m_SrcSavePos;
	DWORD m_DstSavePos;
	int m_SaveChan;
	size_t m_RestoredLength;

	struct WavePeak * m_pOldPeaks;
	size_t m_OldWavePeakSize;
	size_t m_OldAllocatedWavePeakSize;
	int m_OldPeakDataGranularity;
	bool m_bOldDirectMode;
	WAVEFORMATEX m_OldWaveFormat;

	CUndoRedoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName)
		: CCopyContext(pDoc, "", OperationName),
		m_pOldPeaks(NULL)
	{
	}

	BOOL InitUndoCopy(CWaveFile & SrcFile,
					DWORD SaveStartPos, // source file position of data needed to save and restore
					DWORD SaveEndPos,
					int SaveChannel);
	BOOL SaveUndoData(void * pBuf, long BufSize, DWORD Position, int Channel);
	BOOL NeedToSave(DWORD Position, size_t length);
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
	virtual void Execute();

	~CUndoRedoContext();
};

class CDecompressContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	CWaveFile m_SrcFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	DWORD m_SrcPos;

	DWORD m_CurrentSamples;

	size_t m_SrcBufSize;
	size_t m_DstBufSize;
	ACMSTREAMHEADER m_ash;
	DWORD m_ConvertFlags;
	WAVEFORMATEX * m_pWf;

public:
	HACMSTREAM m_acmStr;
	HACMDRIVER m_acmDrv;
	MMRESULT m_MmResult;
	BOOL m_bSwapBytes;
	CDecompressContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, WAVEFORMATEX * pWf)
		: COperationContext(pDoc, "",
							// operation can be terminated by Close
							OperationContextDiskIntensive | OperationContextNonCritical),
		m_SrcBufSize(0),
		m_DstBufSize(0),
		m_acmDrv(NULL),
		m_acmStr(NULL),
		m_bSwapBytes(FALSE),
		m_pWf(CopyWaveformat(pWf)),
		m_MmResult(MMSYSERR_NOERROR)
	{
		m_OperationString = StatusString;
		memset( & m_ash, 0, sizeof m_ash);
	}
	~CDecompressContext()
	{
		DeInit();
		delete m_pWf;
	}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CSoundPlayContext : public COperationContext
{
public:
	CWaveOut m_WaveOut;
	long m_Begin;
	long m_End;
	long m_CurrentPlaybackPos;
	int m_Chan;
	int m_SamplePlayed;
	int m_FirstSamplePlayed;
	bool m_bPauseRequested;
	int m_PlaybackDevice;
	int m_OldThreadPriority;

public:
	CSoundPlayContext(CWaveSoapFrontDoc * pDoc)
		: COperationContext(pDoc, "Play",
							OperationContextDontAdjustPriority),
		m_bPauseRequested(false)
	{
		m_OperationString = _T("Playing");
		PercentCompleted = -1;  // no percents
	}
	virtual ~CSoundPlayContext() {}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CVolumeChangeContext : public COperationContext
{
public:
	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName);
	~CVolumeChangeContext();
	float m_VolumeLeft;
	float m_VolumeRight;

	//virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);

};

class CDcOffsetContext : public COperationContext
{
public:
	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName);
	~CDcOffsetContext();
	int m_OffsetLeft;
	int m_OffsetRight;

	class CStatisticsContext * m_pScanContext;

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);
	virtual CString GetStatusString();

};

class CStatisticsContext : public COperationContext
{
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
	DWORD m_PosMinLeft;
	DWORD m_PosMaxLeft;
	DWORD m_PosMinRight;
	DWORD m_PosMaxRight;
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
	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, DWORD offset, BOOL bBackward = FALSE);

	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CNormalizeContext : public CVolumeChangeContext
{
public:
	CNormalizeContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName)
		: CVolumeChangeContext(pDoc, StatusString, OperationName),
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
public:
	CConversionContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: CCopyContext(pDoc, StatusString, OperationName),
		m_pWf(NULL)
	{
		// delete the procs in the destructor
		m_Flags &= ~OperationContextDiskIntensive;
		m_ProcBatch.m_bAutoDeleteProcs = TRUE;
	}
	~CConversionContext()
	{
		delete[] (char*) m_pWf;
	}
	//virtual BOOL Init();
	//virtual BOOL DeInit();
	virtual BOOL WasClipped() const
	{
		return m_ProcBatch.WasClipped();
	}
	virtual double GetMaxClipped() const
	{
		return m_ProcBatch.GetMaxClipped();
	}
	CBatchProcessing m_ProcBatch;
	WAVEFORMATEX * m_pWf;
	virtual BOOL OperationProc();
	//BOOL SetTargetFormat(WAVEFORMATEX * pwf);
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CFileSaveContext : public CCopyContext
{
public:
	CFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: CCopyContext(pDoc, StatusString, OperationName),
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
public:
	CWmaDecodeContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, "",
							// operation can be terminated by Close
							OperationContextDiskIntensive | OperationContextNonCritical)
	{
		m_OperationString = StatusString;
	}
	~CWmaDecodeContext()
	{
		DeInit();
	}
	CWmaDecoder m_Decoder;
	// opens m_Decoder, loads wave format to its SrcFile
	BOOL Open(CDirectFile & file);
	void SetDstFile(CWaveFile & file);
	LONG m_CurrentSamples;
	long m_DstCopySample;
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CWmaSaveContext : public CConversionContext
{
public:
	CWmaSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: CConversionContext(pDoc, StatusString, OperationName)
	{
	}
	~CWmaSaveContext()
	{
	}

	WmaEncoder m_Enc;

	virtual BOOL Init();
	virtual void DeInit();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);
	virtual BOOL OperationProc();
	//BOOL SetTargetFormat(WAVEFORMATEX * pwf);
	virtual void PostRetire(BOOL bChildContext = FALSE);
};
#endif // AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
