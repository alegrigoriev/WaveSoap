// OperationContext.h
#if !defined(AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OperationContext.h"

class CExpressionEvaluationContext : public COperationContext
{
public:
	CExpressionEvaluationContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName);
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset);
	BOOL SetExpression(LPCSTR * ppszExpression);
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
	virtual BOOL ProcessBuffer(void * buf, size_t len, DWORD offset);
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

#endif // AFX_OPERATIONCONTEXT2_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
