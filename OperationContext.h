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

class COperationContext;
typedef BOOL (_stdcall * WAVE_OPERATION_PROC)(COperationContext *);
class COperationContext
{
public:
	COperationContext(class CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName, DWORD Flags);
	virtual ~COperationContext();

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset) { return TRUE; }

	virtual BOOL Init() { return TRUE; }
	virtual BOOL DeInit() { return TRUE; }
	virtual void Retire();
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual void Execute();

	virtual CString GetStatusString();
	CString m_OperationName;
	CString m_OperationString;
	COperationContext * pNext;
	COperationContext * pPrev;
	COperationContext * m_pSecondaryContext;
	class CWaveSoapFrontDoc * pDocument;
	DWORD m_Flags;
	int PercentCompleted;
	class CUndoRedoContext * m_pUndoContext;

	int m_GetBufferFlags;
	int m_ReturnBufferFlags;

	int m_DstChan;
	CWaveFile m_DstFile;
	DWORD m_DstStart;
	DWORD m_DstEnd;
	DWORD m_DstCopyPos;
	BOOL InitDestination(CWaveFile & DstFile, long StartSample, long EndSample,
						int chan, BOOL NeedUndo);
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
	ContextScanning = 0x04000000,
	StatisticsContext_DcOnly = 0x02000000,
	StatisticsContext_MinMaxOnly = 0x01000000,
	UndoContextReplaceFormat = 0x00800000,
	FileSaveContext_SavingCopy    = 0x00400000,
	FileSaveContext_SameName    =   0x00200000,
	ScanPeaksSavePeakFile = 0x00200000,
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
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	DWORD m_SrcPos;

	DWORD m_CurrentSamples;

	size_t m_SrcBufSize;
	size_t m_DstBufSize;
	ACMSTREAMHEADER m_ash;
	DWORD m_ConvertFlags;


public:
	HACMSTREAM m_acmStr;
	HACMDRIVER m_acmDrv;
	CDecompressContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, "",
							// operation can be terminated by Close
							OperationContextDiskIntensive | OperationContextNonCritical),
		m_SrcBufSize(0),
		m_DstBufSize(0),
		m_acmDrv(NULL),
		m_acmStr(NULL)
	{
		m_OperationString = StatusString;
		memset( & m_ash, 0, sizeof m_ash);
	}
	~CDecompressContext()
	{
		DeInit();
	}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual BOOL DeInit();
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
	virtual BOOL DeInit();
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
	BOOL m_bClipped;

	//virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset);
	virtual void PostRetire(BOOL bChildContext = FALSE);

};

class CDcOffsetContext : public COperationContext
{
public:
	CDcOffsetContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName);
	~CDcOffsetContext();
	int m_OffsetLeft;
	int m_OffsetRight;
	BOOL m_bClipped;

	class CStatisticsContext * m_pScanContext;

	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset);
	virtual void PostRetire(BOOL bChildContext = FALSE);
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

	//virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, DWORD offset);

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
		m_ProcBatch.m_bAutoDeleteProcs = TRUE;
	}
	~CConversionContext()
	{
		delete[] (char*) m_pWf;
	}
	//virtual BOOL Init();
	//virtual BOOL DeInit();
	CBatchProcessing m_ProcBatch;
	WAVEFORMATEX * m_pWf;
	virtual BOOL OperationProc();
	BOOL SetTargetFormat(WAVEFORMATEX * pwf);
};

class CFileSaveContext : public CCopyContext
{
public:
	CFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: CCopyContext(pDoc, StatusString, OperationName),
		m_pConvert(NULL)
	{
	}
	CConversionContext * m_pConvert;
	virtual ~CFileSaveContext()
	{
		delete m_pConvert;
	}
	CString m_NewName;
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CConvertedFileSaveContext : public CFileSaveContext
{
public:
	CConvertedFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: CFileSaveContext(pDoc, StatusString, OperationName),
		m_pWf(NULL)
	{
	}
	WAVEFORMATEX * m_pWf;
	CBatchProcessing m_ProcBatch;

	virtual ~CConvertedFileSaveContext()
	{
		delete[] (char*) m_pWf;
	}
	//virtual BOOL OperationProc();
	//virtual void PostRetire(BOOL bChildContext = FALSE);
	//virtual BOOL Init();
	//virtual BOOL DeInit();
	BOOL SetTargetFormat(WAVEFORMATEX * pwf);

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
	virtual BOOL DeInit();
};

class CExpressionEvaluationContext : public COperationContext
{
public:
	CExpressionEvaluationContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName);
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset);
	BOOL SetExpression(LPCSTR * ppszExpression);
	CString m_ErrorString;

private:
	enum TokenType
	{
		eEndOfExpression,
		ePushDoubleConstant,
		ePushIntConstant,
		ePlusOp,
		eMinusOp,
		eDivideOp,
		eMultiplyOp,
		eLeftParenthesis,
		eRightParenthesis,
		eSinusFunc,
		eCosinusFunc,
		eTangensFunc,
		eSinusHFunc,
		eCosinusHFunc,
		eTangensHFunc,
		eExpFunc,
		eExp10Func,
		eLogFunc,
		eLog10Func,
		eSqrtFunc,
		eAbsFunc,
		eNoiseFunc,
		eInt,
		eIntConstant,
		eIntExpression,
		eIntVariable,
		eDouble,
		eDoubleConstant,
		eDoubleExpression,
		eDoubleVariable,
		ePiConstant,
		eSelectionSampleNumber,
		eAbsoluteSampleNumber,
		eSelectionTime,
		eAbsoluteTime,
		eCurrentFrequencyArgument,
		eCurrentSampleValue,
		eConvertDoubleToLong,
		eConvertLongToDouble,
		eUnknownToken,
	};
	struct Operation
	{
		//Operation * Next;
		//TokenType Type;
		void (_fastcall * Function)(Operation * t);
		union {
			double * dSrc1;
			int * nSrc1;
			void * pSrc1;
		};
		union {
			double * dSrc2;
			int * nSrc2;
			void * pSrc2;
		};
		union {
			double * dDst;
			int * nDst;
			void * pDst;
		};
	};
	static CString GetToken(LPCSTR * ppStr, TokenType * pType);
	TokenType CompileTerm(LPCSTR * ppStr);
	TokenType CompileExpression(LPCSTR * ppStr);
	TokenType CompileParenthesedExpression(LPCSTR * ppStr);
	void AddOperation(void (_fastcall * Function)(Operation * t),
					void * pDst, void * pSrc1, void * pSrc2);
	TokenType GetTopOfStackType();
	void PushConstant(int data);
	void PushConstant(double data);
	void PushVariable(int * pData);
	void PushVariable(double * pData);

	int * PushInt();
	int * PopInt();
	double * PushDouble();
	double * PopDouble();
	void CompileAdd();
	void CompileSubtract();
	void CompileMultiply();
	void CompileDivide();
	enum { ExpressionStackSize = 64,
		NumberOfIntConstants = 128,
		NumberOfDoubleConstants = 64,
	};
	DWORD m_DataStack[ExpressionStackSize * 2];
	int m_DataStackIndex;
	TokenType m_DataTypeStack[ExpressionStackSize];
	int m_DataTypeStackIndex;
	int m_ConstantBuffer[NumberOfIntConstants];
	int m_ConstantBufferIndex;
	int * m_pResultAddress;

	CArray<Operation, Operation&> m_OperationArray;
	int m_nSelectionSampleArgument;
	int m_nFileSampleArgument;
	double m_dSelectionTimeArgument;
	double m_dFileTimeArgument;
	double m_dFrequencyArgument;
	double m_dSelectionLengthTime;
	double m_dFileLengthTime;
	// op functions:
	static void _fastcall AddDouble(Operation *t) { *t->dDst = *t->dSrc1 + *t->dSrc2; }
	static void _fastcall AddInt(Operation *t) { *t->nDst = *t->nSrc1 + *t->nSrc2; }
	static void _fastcall AddDoubleInt(Operation *t) { *t->dDst = *t->dSrc1 + *t->nSrc2; }
	static void _fastcall SubtractDouble(Operation *t) { *t->dDst = *t->dSrc1 - *t->dSrc2; }
	static void _fastcall SubtractInt(Operation *t) { *t->dDst = *t->dSrc1 - *t->dSrc2; }
	static void _fastcall SubtractDoubleInt(Operation *t) { *t->dDst = *t->dSrc1 - *t->nSrc2; }
	static void _fastcall SubtractIntDouble(Operation *t) { *t->dDst = *t->nSrc1 - *t->dSrc2; }
	static void _fastcall MultiplyDouble(Operation *t) { *t->dDst = *t->dSrc1 * *t->dSrc2; }
	static void _fastcall MultiplyInt(Operation *t) { *t->nDst = *t->nSrc1 * *t->nSrc2; }
	static void _fastcall MultiplyDoubleInt(Operation *t)  { *t->dDst = *t->dSrc1 * *t->nSrc2; }
	static void _fastcall DivideDouble(Operation *t)  { *t->dDst = *t->dSrc1 / *t->dSrc2; }
	static void _fastcall DivideInt(Operation *t)  { *t->nDst = *t->nSrc1 / *t->nSrc2; }
	static void _fastcall DivideDoubleInt(Operation *t)  { *t->dDst = *t->dSrc1 / *t->nSrc2; }
	static void _fastcall DivideIntDouble(Operation *t)  { *t->dDst = *t->nSrc1 / *t->dSrc2; }
	static void _fastcall NegateInt(Operation *t)  { *t->nDst = - *t->nSrc1; }
	static void _fastcall NegateDouble(Operation *t)  { *t->dDst = - *t->dSrc1; }
	static void _fastcall Sin(Operation *t)  { *t->dDst = sin(*t->dSrc1); }
	static void _fastcall Cos(Operation *t)  { *t->dDst = cos(*t->dSrc1); }
	static void _fastcall Tan(Operation *t)  { *t->dDst = tan(*t->dSrc1); }
	static void _fastcall SinH(Operation *t)  { *t->dDst = sinh(*t->dSrc1); }
	static void _fastcall CosH(Operation *t)  { *t->dDst = cosh(*t->dSrc1); }
	static void _fastcall TanH(Operation *t)  { *t->dDst = tanh(*t->dSrc1); }
	static void _fastcall Exp(Operation *t)  { *t->dDst = exp(*t->dSrc1); }
	static void _fastcall Exp10(Operation *t)  { *t->dDst = exp(*t->dSrc1 * 2.3025850929940456840); }
	static void _fastcall Log(Operation *t)  { *t->dDst = log(*t->dSrc1); }
	static void _fastcall Log10(Operation *t)  { *t->dDst = log(*t->dSrc1) * 0.434294481903251827651; }
	static void _fastcall Sqrt(Operation *t)  { *t->dDst = sqrt(*t->dSrc1); }
	static void _fastcall Noise(Operation *t)  { *t->dDst = (rand() - RAND_MAX / 2) / double(RAND_MAX / 2); }
	static void _fastcall Abs(Operation *t)  { *t->dDst = abs(*t->dSrc1); }
	static void _fastcall DoubleToInt(Operation *t)  { *t->nDst = *t->dSrc1; }
	static void _fastcall IntToDouble(Operation *t)  { *t->dDst = *t->nSrc1; }
	//static void _fastcall (Operation *t)  { *t->Dst = *t->Src1  *t->Src2; }
	//static void _fastcall (Operation *t)  { *t->Dst = *t->Src1  *t->Src2; }
};
#endif // AFX_OPERATIONCONTEXT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
