// OperationContext.h
#if !defined(AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OperationContext.h"
#include "EqualizerDialog.h"

class CExpressionEvaluationContext : public COperationContext
{
public:
	CExpressionEvaluationContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName);
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);
	BOOL SetExpression(LPCSTR * ppszExpression);
	CString m_ErrorString;
	void Evaluate();
	bool m_bClipped;
	double m_MaxClipped;

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
	static CString GetToken(LPCSTR * ppStr, TokenType * pType);
	TokenType CompileTerm(LPCSTR * ppStr);
	TokenType CompileExpression(LPCSTR * ppStr);
	TokenType CompileParenthesedExpression(LPCSTR * ppStr);
	void AddOperation(void (_fastcall * Function)(Operation * t),
					void * pDst, void * pSrc1, void * pSrc2);
	void CompileFunctionOfDouble(void (_fastcall * Function)(Operation * t), LPCSTR * ppStr);
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

	CArray<Operation, Operation&> m_OperationArray;
	int m_nSelectionSampleArgument;
	int m_nFileSampleArgument;
	double m_dSelectionTimeArgument;
	double m_dFileTimeArgument;
public:
	virtual void PostRetire(BOOL bChildContext = FALSE);
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
	static void _fastcall Abs(Operation *t)  { *t->dDst = abs(*t->dSrc1); }
	static void _fastcall DoubleToInt(Operation *t)
	{
		*t->nDst = *t->dSrc1;
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

class CInsertSilenceContext: public CCopyContext
{
public:
	CInsertSilenceContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
		: CCopyContext(pDoc, StatusString, OperationName)
	{
		m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
		m_GetBufferFlags = CDirectFile::GetBufferWriteOnly;
	}
	virtual BOOL OperationProc();
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);
	BOOL InitExpand(CWaveFile & DstFile, long StartSample, long length,
					int chan, BOOL NeedUndo);
};

class CCommitFileSaveContext :public COperationContext
{
public:
	CCommitFileSaveContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, StatusString, OperationContextDiskIntensive)
	{
		m_OperationString = StatusString;
	}
	int m_FileSaveFlags;
	CString m_TargetName;
	~CCommitFileSaveContext() { }
	virtual BOOL OperationProc();
	virtual void PostRetire(BOOL bChildContext = FALSE);
};

class CEqualizerContext: public COperationContext
{
public:
	CEqualizerContext(CWaveSoapFrontDoc * pDoc,
					LPCTSTR StatusString, LPCTSTR OperationName);
	~CEqualizerContext();

	BOOL m_bClipped;
	BOOL m_bZeroPhase;
	BOOL m_bSecondPass;
	double m_MaxClipped;

	// the coefficients are: 3 numerator's coeffs and 3 denominator's coeffs
	double m_BandCoefficients[MaxNumberOfEqualizerBands][6];
	int m_NumOfBands;    // 2-MaxNumberOfEqualizerBands
	// 2 channels, 2 prev input samples for each filter
	// and 2 prev output samples
	double m_PrevSamples[2][MaxNumberOfEqualizerBands][4];
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);
	virtual void PostRetire(BOOL bChildContext = FALSE);
	virtual BOOL Init();
	virtual BOOL InitPass(int nPass);
private:
	double CalculateResult(int ch, int Input);
};
class CSwapChannelsContext : public COperationContext
{
public:
	CSwapChannelsContext(CWaveSoapFrontDoc * pDoc,
						LPCTSTR StatusString, LPCTSTR OperationName)
		: COperationContext(pDoc, OperationName, OperationContextDiskIntensive)
	{
		m_OperationString = StatusString;
		m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
	}
	~CSwapChannelsContext() {}
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward = FALSE);
};

#endif // AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
