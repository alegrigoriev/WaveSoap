// OperationContext.h
#if !defined(AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <mmreg.h>
#include <msacm.h>
#include "WaveSupport.h"

class COperationContext;
typedef BOOL (_stdcall * WAVE_OPERATION_PROC)(COperationContext *);
class COperationContext
{
public:
	COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName, DWORD Flags)
		: pDocument(pDoc),
		m_Flags(Flags),
		pNextChain(NULL),
		pNext(NULL),
		pPrev(NULL),
		m_pUndoContext(NULL),
		m_OperationName(OperationName),
		PercentCompleted(0)
	{
	}
	virtual ~COperationContext()
	{
		if (pNextChain)
		{
			COperationContext * pContext = pNextChain;
			pNextChain = NULL;
			delete pContext;
		}
	}
	virtual BOOL OperationProc() = 0;
	virtual BOOL Init() { return TRUE; }
	virtual BOOL DeInit() { return TRUE; }
	virtual void Retire();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString() = 0;
	CString m_OperationName;
	COperationContext * pNext;
	COperationContext * pPrev;
	COperationContext * pNextChain;
	class CWaveSoapFrontDoc * pDocument;
	DWORD m_Flags;
	int PercentCompleted;
	class CUndoRedoContext * m_pUndoContext;
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
	OperationContextDontKeepAfterRetire = 0x200,
	OperationContextDontAdjustPriority = 0x400,
};

// additional custom flags for the contexts
enum {
	CopyExpandFile = 0x80000000,
	CopyShrinkFile = 0x40000000,
	CopyCreatingUndo = 0x20000000,
	RedoContext = 0x10000000,
	UndoContextReplaceWholeFile = 0x08000000,
};

class CScanPeaksContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
public:
	DWORD m_Position;
	DWORD m_Start;
	DWORD m_End;
	int m_GranuleSize;
	CString sOp;
	CScanPeaksContext(CWaveSoapFrontDoc * pDoc)
		: COperationContext(pDoc, "Peak Scan", OperationContextDiskIntensive),
		sOp("Scanning the file for peaks..."),
		m_Start(0), m_End(0), m_Position(0)
	{

	}
	~CScanPeaksContext() {}
	virtual BOOL OperationProc();
	virtual CString GetStatusString() { return sOp; }
};

class CResizeContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	CWaveFile m_DstFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	int m_DstChan;
	DWORD m_SrcCopyPos;

	DWORD m_DstStart;
	DWORD m_DstCopyPos;
	DWORD m_DstEnd;
	BOOL ExpandProc();
	BOOL ShrinkProc();
	BOOL InitUndoRedo(CString UndoStatusString);
public:

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
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString();
};

class CCopyContext : public COperationContext
{
public:
	CWaveFile m_SrcFile;
	CWaveFile m_DstFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	int m_SrcChan;
	DWORD m_SrcCopyPos;

	DWORD m_DstStart;
	DWORD m_DstCopyPos;
	DWORD m_DstEnd;
	int m_DstChan;
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
	BOOL InitExpand(LONG StartSample, LONG Length, int Channel);
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
	CUndoRedoContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName)
		: CCopyContext(pDoc, "", OperationName)
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
	~CUndoRedoContext() {}
};

class CCutContext : public CCopyContext
{
	friend class CWaveSoapFrontDoc;
public:
	CCutContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: CCopyContext(pDoc, StatusString, "")
	{

	}
	~CCutContext() {}
	virtual BOOL OperationProc();
	virtual CString GetStatusString() { return sOp; }
};

class CDecompressContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	CWaveFile m_SrcFile;
	CWaveFile m_DstFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	DWORD m_SrcPos;

	DWORD m_DstStart;
	DWORD m_DstPos;
	DWORD m_DstEnd;

	DWORD m_CurrentSamples;

	HACMSTREAM m_acmStr;
	size_t m_SrcBufSize;
	size_t m_DstBufSize;
	ACMSTREAMHEADER m_ash;
	DWORD m_ConvertFlags;


public:
	CString sOp;
	CDecompressContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, "",
							// operation can be terminated by Close
							OperationContextDiskIntensive | OperationContextNonCritical),
		sOp(StatusString),
		m_SrcBufSize(0),
		m_DstBufSize(0),
		m_acmStr(NULL)
	{
		memset( & m_ash, 0, sizeof m_ash);
	}
	~CDecompressContext() {}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual BOOL DeInit();
	virtual CString GetStatusString()
	{
		return sOp;
	}
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
	CString m_ss;

public:
	CSoundPlayContext(CWaveSoapFrontDoc * pDoc)
		: COperationContext(pDoc, "Play",
							OperationContextDontAdjustPriority),
		m_ss(_T("Playing")),
		m_bPauseRequested(false)
	{
		PercentCompleted = -1;  // no percents
	}
	virtual ~CSoundPlayContext() {}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual BOOL DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString() { return m_ss; }
};

class CVolumeChangeContext : public COperationContext
{
public:
	CVolumeChangeContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName);
	~CVolumeChangeContext();
	CWaveFile m_DstFile;
	float m_VolumeLeft;
	float m_VolumeRight;
	int m_DstChan;
	DWORD m_DstStart;
	DWORD m_DstEnd;
	DWORD m_DstCopyPos;
	BOOL m_bClipped;

	CString m_ss;
	virtual BOOL OperationProc();
//    virtual BOOL Init();
//    virtual BOOL DeInit();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual CString GetStatusString() { return m_ss; }

};
#endif // AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
