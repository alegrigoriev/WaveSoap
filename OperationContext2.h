// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext.h
#if !defined(AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OperationContext.h"
#include "EqualizerDialog.h"
#include "FilterDialog.h"
#include "CdDrive.h"

class CExpressionEvaluationContext : public CThroughProcessOperation
{
	typedef CExpressionEvaluationContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CExpressionEvaluationContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId);

	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
	BOOL SetExpression(LPCTSTR * ppszExpression);
	CString m_ErrorString;
	void Evaluate();

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
		eModuloOp,
		eBinaryAndOp,
		eBinaryOrOp,
		eBinaryXorOp,
		eBinaryNotOp,
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
		eSelectionLengthTime,
		eSelectionLengthSamples,
		eFileLengthTime,
		eFileLengthSamples,
		eSamplingRate,
		eSamplePeriod,
		eCurrentFrequencyArgument,
		eCurrentFrequencyArgument1,
		eCurrentFrequencyArgument2,
		eCurrentFrequencyArgument3,
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
	static CString GetToken(LPCTSTR * ppStr, TokenType * pType);
	TokenType CompileTerm(LPCTSTR * ppStr);
	TokenType CompileExpression(LPCTSTR * ppStr);
	TokenType CompileParenthesedExpression(LPCTSTR * ppStr);
	void AddOperation(void (_fastcall * Function)(Operation * t),
					void * pDst, void * pSrc1, void * pSrc2);
	void CompileFunctionOfDouble(void (_fastcall * Function)(Operation * t), LPCTSTR * ppStr);
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
	void CompileModulo();
	void CompileDivide();
	void CompileAnd();
	void CompileOr();
	void CompileXor();

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

	std::vector<Operation> m_OperationArray;
	int m_nSelectionSampleArgument;
	int m_nFileSampleArgument;
	double m_dSelectionTimeArgument;
	double m_dFileTimeArgument;
public:
	double m_dFrequencyArgument;
	double m_dFrequencyArgument1;
	double m_dFrequencyArgument2;
	double m_dFrequencyArgument3;
private:
	double m_dSelectionLengthTime;
	int m_NumberOfFileSamples;
	int m_NumberOfSelectionSamples;
	double m_dFileLengthTime;
	double m_dCurrentSample;
	int m_nSamplingRate;
	double m_SamplePeriod;
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
	static void _fastcall MultiplyDoubleInt(Operation *t)
	{
		*t->dDst = *t->dSrc1 * *t->nSrc2;
	}

	static void _fastcall DivideDouble(Operation *t);
	static void _fastcall DivideInt(Operation *t);
	static void _fastcall DivideDoubleInt(Operation *t);
	static void _fastcall DivideIntDouble(Operation *t);

	static void _fastcall ModuloDouble(Operation *t);
	static void _fastcall ModuloInt(Operation *t);
	static void _fastcall ModuloDoubleInt(Operation *t);
	static void _fastcall ModuloIntDouble(Operation *t);

	static void _fastcall NegateInt(Operation *t)  { *t->nDst = - *t->nSrc1; }
	static void _fastcall ComplementInt(Operation *t)  { *t->nDst = ~ *t->nSrc1; }
	static void _fastcall NegateDouble(Operation *t)  { *t->dDst = - *t->dSrc1; }
	static void _fastcall Sin(Operation *t)  { *t->dDst = sin(fmod(*t->dSrc1, 6.2831853071795864769252867)); }
	static void _fastcall Cos(Operation *t)  { *t->dDst = cos(*t->dSrc1); }
	static void _fastcall Tan(Operation *t)  { *t->dDst = tan(*t->dSrc1); }
	static void _fastcall SinH(Operation *t)  { *t->dDst = sinh(*t->dSrc1); }
	static void _fastcall CosH(Operation *t)  { *t->dDst = cosh(*t->dSrc1); }
	static void _fastcall TanH(Operation *t)  { *t->dDst = tanh(*t->dSrc1); }
	static void _fastcall Exp(Operation *t)  { *t->dDst = exp(*t->dSrc1); }
	static void _fastcall Exp10(Operation *t)  { *t->dDst = exp(*t->dSrc1 * 2.3025850929940456840); }
	static void _fastcall Log(Operation *t);
	static void _fastcall Log10(Operation *t);
	static void _fastcall Sqrt(Operation *t);
	static void _fastcall Noise(Operation *t);
	static void _fastcall Abs(Operation *t)  { *t->dDst = fabs(*t->dSrc1); }
	static void _fastcall DoubleToInt(Operation *t)
	{
		*t->nDst = int(*t->dSrc1);
	}
	static void _fastcall IntToDouble(Operation *t)  { *t->dDst = *t->nSrc1; }
	static void _fastcall AndInt(Operation *t)
	{
		*t->nDst = *t->nSrc1 & *t->nSrc2;
	}
	static void _fastcall OrInt(Operation *t) { *t->nDst = *t->nSrc1 | *t->nSrc2; }
	static void _fastcall XorInt(Operation *t) { *t->nDst = *t->nSrc1 ^ *t->nSrc2; }
	//static void _fastcall (Operation *t)  { *t->Dst = *t->Src1  *t->Src2; }
	//static void _fastcall (Operation *t)  { *t->Dst = *t->Src1  *t->Src2; }
	virtual BOOL Init();
};

class CInsertSilenceContext : public CStagedContext
{
	typedef CInsertSilenceContext ThisClass;
	typedef CStagedContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CInsertSilenceContext(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId)
		: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
	{
	}

	BOOL InitExpand(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX length,
					CHANNEL_MASK chan, BOOL NeedUndo);
};

class CCommitFileSaveContext :public COperationContext
{
	typedef CCommitFileSaveContext ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CCommitFileSaveContext(CWaveSoapFrontDoc * pDoc,
							UINT StatusStringId, CWaveFile & WavFile, int flags, LPCTSTR TargetName);

	//~CCommitFileSaveContext() { }
	virtual BOOL OperationProc();
	virtual void PostRetire();

	virtual MEDIA_FILE_SIZE GetTotalOperationSize() const
	{
		return m_File.GetLength();
	}

	virtual MEDIA_FILE_SIZE GetCompletedOperationSize() const
	{
		return m_TotalCommitted;
	}

protected:
	int m_FileSaveFlags;
	CString m_TargetName;
	CWaveFile m_File;
	LONGLONG m_TotalCommitted;
};

class CEqualizerContext: public CThroughProcessOperation
{
	typedef CEqualizerContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CEqualizerContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId);
	~CEqualizerContext();

	BOOL m_bZeroPhase;

	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs
	double m_BandCoefficients[MaxNumberOfEqualizerBands][6];
	int m_NumOfBands;    // 2-MaxNumberOfEqualizerBands
	// 2 channels, 2 prev input samples for each filter
	// and 2 prev output samples
	double m_PrevSamples[MAX_NUMBER_OF_CHANNELS][MaxNumberOfEqualizerBands][4];

	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
	virtual BOOL Init();
	virtual BOOL InitPass(int nPass);
private:
	double CalculateResult(int ch, int Input);
};

class CFilterContext: public CThroughProcessOperation
{
	typedef CFilterContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CFilterContext(CWaveSoapFrontDoc * pDoc,
					UINT StatusStringId, UINT OperationNameId);
	~CFilterContext();

	BOOL m_bZeroPhase;

	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs
	// results of the filter sections are ADDED
	// if order==0, no filter
	int     m_nLpfOrder;    // low pass filter order
	double m_LpfCoeffs[MaxFilterOrder][6];
	double m_PrevLpfSamples[MAX_NUMBER_OF_CHANNELS][MaxFilterOrder][4];

	// results of the filter sections are ADDED
	int     m_nHpfOrder;    // high pass filter order
	double m_HpfCoeffs[MaxFilterOrder][6];
	double m_PrevHpfSamples[MAX_NUMBER_OF_CHANNELS][MaxFilterOrder][4];

	// results of the filter sections are MULTIPLIED
	int     m_nNotchOrder;
	double m_NotchCoeffs[MaxFilterOrder][6];
	double m_PrevNotchSamples[MAX_NUMBER_OF_CHANNELS][MaxFilterOrder][4];

	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
	virtual BOOL Init();
	virtual BOOL InitPass(int nPass);
private:
	double CalculateResult(int ch, int Input);
};

class CSwapChannelsContext : public CThroughProcessOperation
{
	typedef CSwapChannelsContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CSwapChannelsContext(CWaveSoapFrontDoc * pDoc,
						UINT StatusStringId, UINT OperationNameId)
		: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId,
					OperationNameId)
	{
	}

	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
};

class CCdReadingContext : public CThroughProcessOperation
{
	typedef CCdReadingContext ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CCdReadingContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName);

	virtual ~CCdReadingContext();
	virtual void Execute();
	BOOL InitTrackInformation(CCdDrive const & Drive,
							struct CdTrackInfo * pTrack,
							DWORD TargetFileType,
							WAVEFORMATEX const * pTargetFormat);

	CCdReadingContext * m_pNextTrackContext;
	int m_RequiredReadSpeed;
	int m_OriginalReadSpeed;
	BOOL m_bSaveImmediately;
protected:
	CCdDrive m_Drive;
	CdAddressMSF m_CdAddress;
	ULONG m_NumberOfSectors;
	HANDLE m_hEvent;
	void * m_pCdBuffer;
	size_t m_CdBufferSize;
	size_t m_CdBufferFilled;
	size_t m_CdDataOffset;

	CString m_TrackName;
	CString m_TrackFileName;
	CString m_TrackAlbum;
	CString m_TrackArtist;
	CWaveFormat m_TargetFormat;
	DWORD m_TargetFileType;

	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
	virtual BOOL Init();
	virtual void DeInit();
	virtual void PostRetire();
};

class CReplaceFileContext : public COperationContext
{
	typedef CReplaceFileContext ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CReplaceFileContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName,
						CWaveFile & NewFile, bool bNewDirectMode = false);

	virtual BOOL CreateUndo();
	virtual bool KeepsPermanentFileReference() const;

	virtual BOOL OperationProc();
protected:
	bool m_bNewDirectMode;
	CWaveFile m_File;
};

class CReplaceFormatContext : public COperationContext
{
	typedef CReplaceFormatContext ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CReplaceFormatContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName,
						WAVEFORMATEX const * pNewFormat);
	CReplaceFormatContext(CWaveSoapFrontDoc * pDoc, UINT OperationNameId,
						WAVEFORMATEX const * pNewFormat);

	virtual BOOL CreateUndo();
	virtual BOOL OperationProc();

protected:
	CWaveFormat m_NewWaveFormat;
};

class CLengthChangeOperation : public COperationContext
{
	typedef CLengthChangeOperation ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CLengthChangeOperation(class CWaveSoapFrontDoc * pDoc,
							CWaveFile & File, MEDIA_FILE_SIZE NewLength);

protected:
	virtual BOOL CreateUndo();
	virtual BOOL OperationProc();
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();

	CWaveFile m_File;
	MEDIA_FILE_SIZE m_NewLength;
};

class CWaveSamplesChangeOperation : public COperationContext
{
	typedef CWaveSamplesChangeOperation ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CWaveSamplesChangeOperation(CWaveSoapFrontDoc * pDoc,
								CWaveFile & File, NUMBER_OF_SAMPLES NewSamples);

protected:
	virtual BOOL CreateUndo();
	virtual BOOL OperationProc();
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();

	NUMBER_OF_SAMPLES m_NewSamples;
	CWaveFile m_File;
};

class CMoveOperation : public CCopyContext
{
	// move data in the same file
	typedef CMoveOperation ThisClass;
	typedef CCopyContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CMoveOperation(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName = _T(""));
	CMoveOperation(CWaveSoapFrontDoc * pDoc, UINT StatusStringId = 0, UINT OperationNameId = 0);

	~CMoveOperation();

	BOOL InitMove(CWaveFile & File,
				SAMPLE_INDEX SrcStartSample,
				SAMPLE_INDEX DstStartSample,
				NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);
	virtual void Dump(unsigned indent=0) const;

protected:
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();
	virtual ListHead<COperationContext> * GetUndoChain();
	virtual void DeleteUndo();
	virtual void DeInit();

	virtual BOOL CreateUndo();
	virtual BOOL OperationProc();

	CMoveOperation * m_pUndoMove;
};

// this object saves the data being discarded as a result
// of trimming the file
// It uses RestoreTrimmedOperation as UNDO
class CSaveTrimmedOperation : public CCopyContext
{
	// move data in the same file
	typedef CSaveTrimmedOperation ThisClass;
	typedef CCopyContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CSaveTrimmedOperation(CWaveSoapFrontDoc * pDoc,
						CWaveFile & SrcFile,
						SAMPLE_INDEX SrcStartSample,
						SAMPLE_INDEX SrcEndSample,
						CHANNEL_MASK Channels);
	~CSaveTrimmedOperation();
	virtual void Dump(unsigned indent=0) const;

protected:
	class CRestoreTrimmedOperation * m_pRestoreOperation;

	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();

	virtual ListHead<COperationContext> * GetUndoChain();

	virtual BOOL CreateUndo();
	virtual void DeleteUndo();
	virtual BOOL OperationProc();
};

class CRestoreTrimmedOperation : public CCopyUndoContext
{
	// move data in the same file
	typedef CRestoreTrimmedOperation ThisClass;
	typedef CCopyUndoContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CRestoreTrimmedOperation(CWaveSoapFrontDoc * pDoc);
	~CRestoreTrimmedOperation();
	virtual void Dump(unsigned indent=0) const;

protected:

	class CSaveTrimmedOperation * m_pSaveOperation;

	virtual BOOL CreateUndo();
	virtual ListHead<COperationContext> * GetUndoChain();
	virtual void DeleteUndo();
};

// zero-fill a file area
class CInitChannels : public CThroughProcessOperation
{
	// Zero some channels. When undone, data is left as is (will be overwritten)
	typedef CInitChannels ThisClass;
	typedef CThroughProcessOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CInitChannels(CWaveSoapFrontDoc * pDoc,
				CWaveFile & File, SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channels);

protected:
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();
	virtual void DeInit();

	virtual BOOL CreateUndo();
	virtual BOOL ProcessBuffer(void * buf, size_t len, SAMPLE_POSITION offset, BOOL bBackward = FALSE);
};

// undo zero-fill a file area (does nothing, but produces a redo)
class CInitChannelsUndo : public COneFileOperation
{
	// Zero some channels. When undone, data is left as is (will be overwritten)
	typedef CInitChannelsUndo ThisClass;
	typedef COneFileOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CInitChannelsUndo(CWaveSoapFrontDoc * pDoc,
					SAMPLE_POSITION Start, SAMPLE_POSITION End, CHANNEL_MASK Channels);

protected:

	virtual BOOL CreateUndo();
	virtual BOOL OperationProc();
};

// This operation can be used as UNDO/REDO
class CMetadataChangeOperation : public COperationContext
{
	typedef CMetadataChangeOperation ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CMetadataChangeOperation(CWaveSoapFrontDoc * pDoc, CWaveFile & WaveFile,
							unsigned MetadataChangeFlags = 0);
	~CMetadataChangeOperation();

	virtual BOOL CreateUndo();
	virtual BOOL PrepareUndo();
	virtual void UnprepareUndo();

	void SaveUndoMetadata(unsigned ChangeFlags);

protected:
	virtual BOOL OperationProc();
	CWaveFile::InstanceDataWav * m_pMetadata;
	unsigned m_MetadataCopyFlags;
	CMetadataChangeOperation * m_pUndoData;
	CWaveFile m_WaveFile;
};

// This operation is never used as UNDO/REDO
class CCueEditOperation : public CMetadataChangeOperation
{
	typedef CCueEditOperation ThisClass;
	typedef CMetadataChangeOperation BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	// move markers
	CCueEditOperation(CWaveSoapFrontDoc * pDoc, CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace, NUMBER_OF_SAMPLES SamplesToInsert);

	// copy markers
	CCueEditOperation(CWaveSoapFrontDoc * pDoc, CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace,
					CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
					NUMBER_OF_SAMPLES SamplesToInsert);

	~CCueEditOperation();
	virtual BOOL OperationProc();

	CWaveFile m_SrcFile;
	SAMPLE_INDEX m_StartDstSample;
	NUMBER_OF_SAMPLES m_LengthToReplace;
	SAMPLE_INDEX m_StartSrcSample;
	NUMBER_OF_SAMPLES m_SamplesToInsert;
};

class CSelectionChangeOperation : public COperationContext
{
	typedef CSelectionChangeOperation ThisClass;
	typedef COperationContext BaseClass;
public:
	typedef std::auto_ptr<ThisClass> auto_ptr;

	CSelectionChangeOperation(CWaveSoapFrontDoc * pDoc,
							SAMPLE_INDEX Start, SAMPLE_INDEX End, SAMPLE_INDEX Caret,
							CHANNEL_MASK Channels);

	virtual BOOL CreateUndo();
	virtual BOOL OperationProc();

protected:
	SAMPLE_INDEX m_Start;
	SAMPLE_INDEX m_End;
	SAMPLE_INDEX m_Caret;
	CHANNEL_MASK m_Channels;
};

class CReverseOperation : public CTwoFilesOperation
{
	typedef CReverseOperation ThisClass;
	typedef CTwoFilesOperation BaseClass;

public:
	typedef std::auto_ptr<ThisClass> auto_ptr;
	CReverseOperation(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId);
	~CReverseOperation();

	BOOL InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
						CHANNEL_MASK chan, BOOL NeedUndo);

protected:

	CCopyUndoContext * m_pUndoLow;
	CCopyUndoContext * m_pUndoHigh;

	SAMPLE_POSITION m_HighDstPos;

	virtual BOOL CreateUndo();
	virtual ListHead<COperationContext> * GetUndoChain();
	virtual void DeleteUndo();
	virtual BOOL OperationProc();
};

class CWaveMixOperation : public CThroughProcessOperation
{
	typedef CThroughProcessOperation BaseClass;
	typedef CWaveMixOperation ThisClass;

public:

	typedef std::auto_ptr<ThisClass> auto_ptr;
	CWaveMixOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags = 0,
					UINT StatusStringId = 0, UINT OperationNameId = 0);

protected:
	virtual BOOL ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL bBackward);
	virtual double GetSrcMixCoefficient(SAMPLE_INDEX Sample, int Channel) const = 0;
	virtual double GetDstMixCoefficient(SAMPLE_INDEX Sample, int Channel) const = 0;
};

enum { FadeInLinear = 1,
	FadeOutLinear = -FadeInLinear,
	FadeInSinSquared = 2,
	FadeOutSinSquared = -FadeInSinSquared,
	FadeInSine = 3,
	FadeOutCosine = -FadeInSine,
};

class CFadeInOutOperation : public CWaveMixOperation
{
	typedef CWaveMixOperation BaseClass;
	typedef CFadeInOutOperation ThisClass;

public:

	typedef std::auto_ptr<ThisClass> auto_ptr;
	CFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType);
	// init cross fade
	CFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
						CWaveFile & SrcFile, SAMPLE_INDEX SrcBegin, CHANNEL_MASK SrcChannel,
						CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
						NUMBER_OF_SAMPLES Length, BOOL UndoEnabled);

	// init fade in/out
	CFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
						CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
						NUMBER_OF_SAMPLES Length, BOOL UndoEnabled);

protected:
	virtual double GetSrcMixCoefficient(SAMPLE_INDEX Sample, int Channel) const;
	virtual double GetDstMixCoefficient(SAMPLE_INDEX Sample, int Channel) const;
	int m_FadeCurveType;
};

BOOL InitInsertCopy(CStagedContext * pContext,
					CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace, CHANNEL_MASK DstChannel,
					CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
					NUMBER_OF_SAMPLES SamplesToInsert, CHANNEL_MASK SrcChannel);

BOOL InitExpandOperation(CStagedContext * pContext,
						CWaveFile & File, SAMPLE_INDEX StartSample,
						NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);
// delete area from StartSample to StartSample+Length
BOOL InitShrinkOperation(CStagedContext * pContext,
						CWaveFile & File, SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel);

void InitCopyMarkers(CStagedContext * pContext,
					CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace,
					CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
					NUMBER_OF_SAMPLES SamplesToInsert);

void InitMoveMarkers(CStagedContext * pContext,
					CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace,
					NUMBER_OF_SAMPLES SamplesToInsert);

#endif // AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
