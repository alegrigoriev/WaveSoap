// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext2.cpp
#include "stdafx.h"
#include "OperationContext2.h"
#include "OperationDialogs.h"
#include "WaveSoapFrontDoc.h"
#include "BladeMP3EncDLL.h"
#include "TimeToStr.h"
#include "resource.h"       // main symbols

static int _fastcall fround(double d)
{
	return (int)floor(d + 0.5);
	if (d >= 0.)
	{
		return int(d + 0.5);
	}
	else
	{
		return int(d - 0.5);
	}
}

void SkipWhitespace(LPCTSTR * ppStr)
{
	LPCTSTR str = *ppStr;
	while (' ' == *str
			|| '\n' == *str
			|| '\t' == *str
			|| '\r' == *str)
	{
		str++;
	}
	*ppStr = str;
}

CExpressionEvaluationContext::CExpressionEvaluationContext(CWaveSoapFrontDoc * pDoc,
															UINT StatusStringId, UINT OperationNameId)
	:BaseClass(pDoc, 0, StatusStringId, OperationNameId)
{
}

BOOL CExpressionEvaluationContext::Init()
{
	m_nSamplingRate = m_DstFile.SampleRate();
	m_SamplePeriod = 1. / m_nSamplingRate;

	m_NumberOfFileSamples = m_DstFile.NumberOfSamples();
	m_dFileLengthTime = m_NumberOfFileSamples / double(m_nSamplingRate);

	m_NumberOfSelectionSamples = (m_DstEnd - m_DstStart) / m_DstFile.SampleSize();
	m_dSelectionLengthTime = m_NumberOfSelectionSamples / double(m_nSamplingRate);

	m_nSelectionSampleArgument = 0;
	m_dSelectionTimeArgument = 0.;
	m_nFileSampleArgument = m_DstFile.PositionToSample(m_DstStart);
	m_dFileTimeArgument = m_nFileSampleArgument / double(m_nSamplingRate);
	return TRUE;
}

BOOL CExpressionEvaluationContext::ProcessBuffer(void * buf,
												size_t BufferLength, SAMPLE_POSITION /*offset*/, BOOL /*bBackward*/)
{
	// calculate number of sample, and time
	int nChannels = m_DstFile.Channels();

	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	NUMBER_OF_SAMPLES NumSamples = BufferLength / sizeof pDst[0];

	try
	{
		for (NUMBER_OF_SAMPLES i = 0; i < NumSamples; i += nChannels)
		{
			for (int ch = 0; ch < nChannels; ch++)
			{
				if (m_DstChan & (1 << ch))
				{
					m_dCurrentSample = pDst[i + ch] * 0.00003051850947599719229;
					Evaluate();
					pDst[i + ch] = DoubleToShort(* m_pResultAddress);
				}
			}

			m_nSelectionSampleArgument++;
			m_nFileSampleArgument++;
			m_dSelectionTimeArgument += m_SamplePeriod;
			m_dFileTimeArgument += m_SamplePeriod;
		}
	}
	catch (const char * pError)
	{
		m_ErrorString = pError;
		return FALSE;
	}
	return TRUE;
}

CString CExpressionEvaluationContext::GetToken(LPCTSTR * ppStr, TokenType * pType)
{
	// get either delimiter, operand or operator
	static struct TokenTable
	{
		LPCTSTR token;
		TokenType type;
	}
	Table[] =
	{
		_T("("), eLeftParenthesis,
		_T(")"), eRightParenthesis,
		_T("-"), eMinusOp,
		_T("+"), ePlusOp,
		_T("/"), eDivideOp,
		_T("*"), eMultiplyOp,
		_T("%"), eModuloOp,
		_T("&"), eBinaryAndOp,
		_T("|"), eBinaryOrOp,
		_T("^"), eBinaryXorOp,
		_T("~"), eBinaryNotOp,
		_T("int"), eInt,
		_T("float"), eDouble,
		_T("double"), eDouble,
		_T("sinh"), eSinusHFunc,
		_T("sin"), eSinusFunc,
		_T("cosh"), eCosinusHFunc,
		_T("cos"), eCosinusFunc,
		_T("tanh"), eTangensHFunc,
		_T("tan"), eTangensFunc,
		_T("exp10"), eExp10Func,
		_T("exp"), eExpFunc,
		_T("log10"), eLog10Func,
		_T("log"), eLogFunc,
		_T("sqrt"), eSqrtFunc,
		_T("abs"), eAbsFunc,
		_T("noise"), eNoiseFunc,
		_T("pi"), ePiConstant,
		_T("T"), eAbsoluteTime,
		_T("t"), eSelectionTime,
		_T("dt"), eSelectionLengthTime,
		_T("DT"), eFileLengthTime,
		_T("F"), eSamplingRate,
		_T("dn"), eSelectionLengthSamples,
		_T("DN"), eFileLengthSamples,
		//_T("T", eSamplePeriod,
		_T("f1"), eCurrentFrequencyArgument1,
		_T("f2"), eCurrentFrequencyArgument2,
		_T("f3"), eCurrentFrequencyArgument3,
		_T("f"), eCurrentFrequencyArgument,
		_T("wave"), eCurrentSampleValue,
	};
	SkipWhitespace(ppStr);
	LPCTSTR str = *ppStr;
	if ('\0' == *str)
	{
		*pType = eEndOfExpression;
		return _T("");
	}
	for (int i = 0; i < countof(Table);i++)
	{
		int j;
		for (j = 0; Table[i].token[j] != '\0' && Table[i].token[j] == str[j]; j++)
		{}
		if ('\0' == Table[i].token[j])
		{
			// check that the alphanum token is not longer that the table entry
			if ((isalnum(str[0] & 0xFF) || '_' == str[0])
				&& (isalnum(str[j] & 0xFF) || '_' == str[j]))
			{
				// the string not fully compared
				* pType = eUnknownToken;
				return str;
			}
			*ppStr = str + j;
			* pType = Table[i].type;
			return str;
		}
	}
	// couldn't find predefined token,
	// try to parse for a number
	* pType = eDoubleConstant;
	return str;
}

CExpressionEvaluationContext::TokenType
CExpressionEvaluationContext::CompileParenthesedExpression(LPCTSTR * ppStr)
{
	TokenType type;
	//LPCTSTR prevStr = *ppStr;
	CString token = GetToken( ppStr, & type);
	if (type != eLeftParenthesis
		|| eRightParenthesis != CompileExpression(ppStr))
	{
		throw "Right parenthesis expected";
	}
	return eRightParenthesis;
}

void CExpressionEvaluationContext::CompileFunctionOfDouble(void (_fastcall * Function)(Operation * t), LPCTSTR * ppStr)
{
	CompileParenthesedExpression( ppStr);
	// can't put those right to the function call, because
	// order of evaluation would be unpredicted
	double * pArg = PopDouble();
	double * pDst = PushDouble();

	AddOperation(Function, pDst, pArg, NULL);
}

CExpressionEvaluationContext::TokenType
	CExpressionEvaluationContext::CompileTerm(LPCTSTR * ppStr)
{
	// term may be either expression in parentheses, or function call, or unary operation
	// and a term
	TokenType type;
	int IntConstant;
	double DoubleConstant;
	TCHAR * endptr;

	LPCTSTR prevStr = *ppStr;
	CString token = GetToken( ppStr, & type);
	switch (type)
	{
	case eEndOfExpression:
		throw "Syntax error";
		break;

	case ePlusOp:
		CompileTerm(ppStr);
		break;
	case eMinusOp:
		type = CompileTerm(ppStr);
		type = GetTopOfStackType();
		if (eIntConstant == type)
		{
			PushConstant( - *PopInt());
		}
		else if (eIntExpression == type)
		{
			int * pSrc = PopInt();
			int * pDst = PushInt();
			AddOperation(NegateInt, pDst, pSrc, NULL);
		}
		else if (eDoubleConstant == type)
		{
			PushConstant( - *PopDouble());
		}
		else if (eDoubleExpression == type)
		{
			double * pSrc = PopDouble();
			double * pDst = PushDouble();
			AddOperation(NegateDouble, pDst, pSrc, NULL);
		}
		else
		{
			throw "Internal Error";
		}
		break;

	case eBinaryNotOp:
		type = CompileTerm(ppStr);
		type = GetTopOfStackType();
		if (eIntConstant == type)
		{
			PushConstant( ~ *PopInt());
		}
		else if (eIntExpression == type)
		{
			int * pSrc = PopInt();
			int * pDst = PushInt();
			AddOperation(ComplementInt, pDst, pSrc, NULL);
		}
		else
		{
			throw "Binary complement operation arguments must be integer, use int() function";
		}
		break;

	case eLeftParenthesis:
		*ppStr = prevStr;
		return CompileParenthesedExpression( ppStr);
		break;

	case eSinusFunc:
		CompileFunctionOfDouble(Sin, ppStr);
		break;

	case eCosinusFunc:
		CompileFunctionOfDouble(Cos, ppStr);
		break;

	case eTangensFunc:
		CompileFunctionOfDouble(Tan, ppStr);
		break;

	case eSinusHFunc:
		CompileFunctionOfDouble(SinH, ppStr);
		break;

	case eCosinusHFunc:
		CompileFunctionOfDouble(CosH, ppStr);
		break;

	case eTangensHFunc:
		CompileFunctionOfDouble(TanH, ppStr);
		break;

	case eExpFunc:
		CompileFunctionOfDouble(Exp, ppStr);
		break;

	case eExp10Func:
		CompileFunctionOfDouble(Exp10, ppStr);
		break;

	case eLogFunc:
		CompileFunctionOfDouble(Log, ppStr);
		break;

	case eLog10Func:
		CompileFunctionOfDouble(Log10, ppStr);
		break;

	case eSqrtFunc:
		CompileFunctionOfDouble(Sqrt, ppStr);
		break;

	case eAbsFunc:
		CompileFunctionOfDouble(Abs, ppStr);
		break;

	case eInt:
	{
		CompileParenthesedExpression( ppStr);
		// can't put those right to the function call, because
		// order of evaluation would be unpredicted
		TokenType type = GetTopOfStackType();
		if (type != eIntConstant
			&& type != eIntExpression
			&& type != eIntVariable)
		{
			PopInt();
			PushInt();
		}
	}
		break;

	case eDouble:
	{
		CompileParenthesedExpression( ppStr);
		// can't put those right to the function call, because
		// order of evaluation would be unpredicted
		TokenType type = GetTopOfStackType();
		if (type != eDoubleConstant
			&& type != eDoubleExpression
			&& type != eDoubleVariable)
		{
			PopDouble();
			PushDouble();
		}
	}
		break;

	case eNoiseFunc:
		// either no arguments, or empty arg: noise()
		prevStr = *ppStr;
		token = GetToken( ppStr, & type);
		if (eLeftParenthesis == type)
		{
			token = GetToken( ppStr, & type);
			if (eRightParenthesis != type)
			{
				throw "Invalud argument of \"noise\" function";
			}
		}
		else
		{
			*ppStr = prevStr;
		}
		{
			double * pDst = PushDouble();
			AddOperation(Noise, pDst, NULL, NULL);
		}
		return eNoiseFunc;
		break;

	case eIntConstant:
		IntConstant = _tcstol(prevStr, & endptr, 0);
		*ppStr = endptr;
		PushConstant(IntConstant);
		break;
	case eDoubleConstant:
		DoubleConstant = _tcstod(prevStr, & endptr);
		*ppStr = endptr;
		IntConstant = int(DoubleConstant);
		if (IntConstant == DoubleConstant)
		{
			PushConstant(IntConstant);
		}
		else
		{
			PushConstant(DoubleConstant);
		}
		break;
	case ePiConstant:
		PushConstant(3.14159265358979323846);
		break;
	case eSelectionSampleNumber:
		PushVariable( & m_nSelectionSampleArgument);
		break;
	case eAbsoluteSampleNumber:
		PushVariable( & m_nFileSampleArgument);
		break;

	case eSelectionTime:
		PushVariable( & m_dSelectionTimeArgument);
		break;
	case eSelectionLengthTime:
		PushVariable( & m_dSelectionLengthTime);
		break;
	case eAbsoluteTime:
		PushVariable( & m_dFileTimeArgument);
		break;
	case eCurrentFrequencyArgument:
		PushVariable( & m_dFrequencyArgument);
		break;
	case eCurrentFrequencyArgument1:
		PushVariable( & m_dFrequencyArgument1);
		break;
	case eCurrentFrequencyArgument2:
		PushVariable( & m_dFrequencyArgument2);
		break;
	case eCurrentFrequencyArgument3:
		PushVariable( & m_dFrequencyArgument3);
		break;
	case eCurrentSampleValue:
		PushVariable( & m_dCurrentSample);
		break;
	case eSamplingRate:
		PushVariable( & m_nSamplingRate);
		break;
	default:
		*ppStr = prevStr;
		throw "Unrecognized syntax";
		break;
	}
	return type;
}

void CExpressionEvaluationContext::CompileAnd()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(AndInt, pDst, pSrc1, pSrc2);
			return;
		}
	}
	throw "Bitwise ""and"" operation arguments must be integer, use int() function";
}

void CExpressionEvaluationContext::CompileOr()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(OrInt, pDst, pSrc1, pSrc2);
			return;
		}
	}
	throw "Bitwise ""or"" operation arguments must be integer, use int() function";
}

void CExpressionEvaluationContext::CompileXor()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(XorInt, pDst, pSrc1, pSrc2);
			return;
		}
	}
	throw "Bitwise ""xor"" operation arguments must be integer, use int() function";
}

void CExpressionEvaluationContext::CompileAdd()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(AddInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(AddDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else if (eDoubleConstant == type
			|| eDoubleExpression == type)
	{
		double * pSrc2 = PopDouble();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			double * pDst = PushDouble();
			AddOperation(AddDoubleInt, pDst, pSrc2, pSrc1);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(AddDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else
	{
		throw "Internal Error";
	}
}

void CExpressionEvaluationContext::CompileSubtract()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(SubtractInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(SubtractDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else if (eDoubleConstant == type
			|| eDoubleExpression == type)
	{
		double * pSrc2 = PopDouble();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			double * pDst = PushDouble();
			AddOperation(SubtractIntDouble, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(SubtractDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else
	{
		throw "Internal Error";
	}
}

void _fastcall CExpressionEvaluationContext::DivideDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw "Divide by zero";
	}
	*t->dDst = *t->dSrc1 / *t->dSrc2;
}

void _fastcall CExpressionEvaluationContext::DivideInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw "Divide by zero";
	}
	*t->nDst = *t->nSrc1 / *t->nSrc2;
}

void _fastcall CExpressionEvaluationContext::DivideDoubleInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw "Divide by zero";
	}
	*t->dDst = *t->dSrc1 / *t->nSrc2;
}

void _fastcall CExpressionEvaluationContext::DivideIntDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw "Divide by zero";
	}
	*t->dDst = *t->nSrc1 / *t->dSrc2;
}

void _fastcall CExpressionEvaluationContext::ModuloDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw "Divide by zero";
	}
	*t->dDst = fmod(*t->dSrc1, *t->dSrc2);
}

void _fastcall CExpressionEvaluationContext::ModuloInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw "Divide by zero";
	}
	*t->nDst = *t->nSrc1 % *t->nSrc2;
}

void _fastcall CExpressionEvaluationContext::ModuloDoubleInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw "Divide by zero";
	}
	*t->dDst = fmod(*t->dSrc1, *t->nSrc2);
}

void _fastcall CExpressionEvaluationContext::ModuloIntDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw "Divide by zero";
	}
	*t->dDst = fmod(*t->nSrc1, *t->dSrc2);
}

void _fastcall CExpressionEvaluationContext::Log(Operation *t)
{
	if (*t->dSrc1 <= 0)
	{
		throw "Logarithm argument <= 0";
	}
	*t->dDst = log(*t->dSrc1);
}

void _fastcall CExpressionEvaluationContext::Log10(Operation *t)
{
	if (*t->dSrc1 <= 0)
	{
		throw "Logarithm argument <= 0";
	}
	*t->dDst = log(*t->dSrc1) * 0.434294481903251827651;
}

void _fastcall CExpressionEvaluationContext::Sqrt(Operation *t)
{
	if (*t->dSrc1 < 0)
	{
		throw "Square root argument < 0";
	}
	*t->dDst = sqrt(*t->dSrc1);
}

void _fastcall CExpressionEvaluationContext::Noise(Operation *t)
{
	C_ASSERT(RAND_MAX == 0x7FFF);
	//long r = (rand() ^ (rand() << 9)) - 0x800000;    // 24 bits
	*t->dDst = ((rand() ^ (rand() << 9)) - 0x800000) / double(0x800000);
}

void CExpressionEvaluationContext::CompileMultiply()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(MultiplyInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(MultiplyDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else if (eDoubleConstant == type
			|| eDoubleExpression == type)
	{
		double * pSrc2 = PopDouble();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			double * pDst = PushDouble();
			AddOperation(MultiplyDoubleInt, pDst, pSrc2, pSrc1);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(MultiplyDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else
	{
		throw "Internal Error";
	}
}

void CExpressionEvaluationContext::CompileModulo()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(ModuloInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(ModuloDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else if (eDoubleConstant == type
			|| eDoubleExpression == type)
	{
		double * pSrc2 = PopDouble();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			double * pDst = PushDouble();
			AddOperation(ModuloIntDouble, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(ModuloDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else
	{
		throw "Internal Error";
	}
}

void CExpressionEvaluationContext::CompileDivide()
{
	TokenType type = GetTopOfStackType();
	if (eIntConstant == type
		|| eIntExpression == type)
	{
		int * pSrc2 = PopInt();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			int * pDst = PushInt();
			AddOperation(DivideInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(DivideDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else if (eDoubleConstant == type
			|| eDoubleExpression == type)
	{
		double * pSrc2 = PopDouble();
		type = GetTopOfStackType();
		if (eIntConstant == type
			|| eIntExpression == type)
		{
			int * pSrc1 = PopInt();
			double * pDst = PushDouble();
			AddOperation(DivideIntDouble, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(DivideDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw "Internal Error";
		}
	}
	else
	{
		throw "Internal Error";
	}
}

CExpressionEvaluationContext::TokenType
	CExpressionEvaluationContext::CompileExpression(LPCTSTR * ppStr)
{
	TokenType type, type1;
	//void (_fastcall * Function)(Operation * t);
	type = CompileTerm(ppStr);
	if (eEndOfExpression == type)
	{
		throw "Expression syntax error";
	}
	LPCTSTR str = *ppStr;

	while (1)
	{
		LPCTSTR prevStr = *ppStr;
		CString token = GetToken( ppStr, & type);
		CString token1;
		switch (type)
		{
		case eEndOfExpression:
			return eEndOfExpression;
			break;

		case ePlusOp:
		case eMinusOp:
		case eBinaryOrOp:
			type1 = CompileTerm(ppStr);
			if (eEndOfExpression == type1)
			{
				throw "Right operand missing";
			}
			// check if the next operator has higher precedence
			while (1)
			{
				str = *ppStr;
				token1 = GetToken( & str, & type1);
				if (eDivideOp == type1)
				{
					*ppStr = str;
					CompileTerm(ppStr);
					CompileDivide();
				}
				else if (eMultiplyOp == type1)
				{
					*ppStr = str;
					CompileTerm(ppStr);
					CompileMultiply();
				}
				else if (eModuloOp == type1)
				{
					*ppStr = str;
					CompileTerm(ppStr);
					CompileModulo();
				}
				else if (eBinaryAndOp == type1)
				{
					*ppStr = str;
					CompileTerm(ppStr);
					CompileAnd();
				}
				else if (eBinaryXorOp == type1)
				{
					*ppStr = str;
					CompileTerm(ppStr);
					CompileXor();
				}
				else
				{
					if (ePlusOp == type)
					{
						CompileAdd();
					}
					else  if (eBinaryOrOp == type)
					{
						CompileOr();
					}
					else
					{
						CompileSubtract();
					}
					break;
				}
			}

			break;

		case eDivideOp:
			CompileTerm(ppStr);
			CompileDivide();
			break;

		case eMultiplyOp:
			CompileTerm(ppStr);
			CompileMultiply();
			break;

		case eModuloOp:
			CompileTerm(ppStr);
			CompileModulo();
			break;

		case eBinaryAndOp:
			CompileTerm(ppStr);
			CompileAnd();
			break;

		case eBinaryXorOp:
			CompileTerm(ppStr);
			CompileXor();
			break;

		case eRightParenthesis:
			//*ppStr = prevStr;
			return eRightParenthesis;
			break;
		default:
		case eUnknownToken:
			*ppStr = prevStr;
			throw "Unrecognized syntax";
			break;
		}
	}
	return eEndOfExpression;
}

void CExpressionEvaluationContext::AddOperation(void (_fastcall * Function)(Operation * t),
												void * pDst, void * pSrc1, void * pSrc2)
{
	Operation t;
	t.Function = Function;
	t.pSrc1 = pSrc1;
	t.pSrc2 = pSrc2;
	t.pDst = pDst;
	m_OperationArray.push_back(t);
}

CExpressionEvaluationContext::TokenType CExpressionEvaluationContext::GetTopOfStackType()
{
	if (m_DataTypeStackIndex <= 0)
	{
		throw "Expression Evaluation Stack Underflow";
	}
	TokenType type = m_DataTypeStack[m_DataTypeStackIndex - 1];
	if (eIntConstant == type)
	{
		return eIntConstant;
	}
	else if (eIntExpression == type
			|| eIntVariable == type)
	{
		return eIntExpression;
	}
	else if (eDoubleConstant == type)
	{
		return eDoubleConstant;
	}
	else if (eDoubleVariable == type
			|| eDoubleExpression == type)
	{
		return eDoubleExpression;
	}
	throw "Internal Error";
}

void CExpressionEvaluationContext::PushConstant(int data)
{
	if (m_ConstantBufferIndex >= NumberOfIntConstants)
	{
		throw "Expression too complex: too many constants";
	}
	if (m_DataStackIndex >= ExpressionStackSize * 2
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw "Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntConstant;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex] = int(m_ConstantBuffer + m_ConstantBufferIndex);  // store pointer to data
	m_DataStackIndex++;

	m_ConstantBuffer[m_ConstantBufferIndex] = data;
	m_ConstantBufferIndex++;

	TRACE("Push int constant %d, constant index = %d, type index = %d\n", data,
		m_ConstantBufferIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationContext::PushConstant(double data)
{
	if (m_ConstantBufferIndex + 1 >= NumberOfIntConstants)
	{
		throw "Expression too complex: too many constants";
	}

	if (m_DataStackIndex >= ExpressionStackSize * 2
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw "Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eDoubleConstant;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex] = int(m_ConstantBuffer + m_ConstantBufferIndex);  // store pointer to data
	m_DataStackIndex++;

	*(double*)(m_ConstantBuffer + m_ConstantBufferIndex) = data;
	m_ConstantBufferIndex += sizeof (double) / sizeof m_ConstantBuffer[0];

	TRACE("Push Double constant %f, constant index = %d, type index = %d\n", data,
		m_ConstantBufferIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationContext::PushVariable(int * pData)
{
	if (m_DataStackIndex >= ExpressionStackSize * 2
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw "Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntVariable;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex] = int(pData);  // store pointer to data
	m_DataStackIndex++;

	TRACE("Push int variable, data index = %d, type index = %d\n",
		m_DataStackIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationContext::PushVariable(double * pData)
{
	if (m_DataStackIndex >= ExpressionStackSize * 2
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw "Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eDoubleVariable;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex] = int(pData);  // store pointer to data
	m_DataStackIndex++;

	TRACE("Push Double variable, data index = %d, type index = %d\n",
		m_DataStackIndex, m_DataTypeStackIndex);
}

int * CExpressionEvaluationContext::PushInt()
{
	if (m_DataStackIndex >= ExpressionStackSize * 2
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw "Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntExpression;
	m_DataTypeStackIndex++;
	m_DataStackIndex++;

	TRACE("Push int, data index = %d, type index = %d\n",
		m_DataStackIndex, m_DataTypeStackIndex);
	return (int*)& m_DataStack[m_DataStackIndex - 1];
}

double * CExpressionEvaluationContext::PushDouble()
{
	if (m_DataStackIndex + 1 >= ExpressionStackSize * 2
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw "Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eDoubleExpression;
	m_DataTypeStackIndex++;

	m_DataStackIndex += sizeof (double) / sizeof m_ConstantBuffer[0];
	TRACE("Push Double, data index = %d, type index = %d\n",
		m_DataStackIndex, m_DataTypeStackIndex);
	return (double*)(m_DataStack + m_DataStackIndex -
					sizeof (double) / sizeof m_DataStack[0]);
}

int * CExpressionEvaluationContext::PopInt()
{
	if (m_DataTypeStackIndex <= 0)
	{
		throw "Expression Evaluation Stack Underflow";
	}
	TokenType type = m_DataTypeStack[m_DataTypeStackIndex - 1];
	if (eIntConstant == type
		|| eIntVariable == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex--;
		TRACE("Pop int constant, data index = %d, type index = %d\n",
			m_DataStackIndex, m_DataTypeStackIndex);
		return (int *) m_DataStack[m_DataStackIndex];
	}
	else if (eIntExpression == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex--;
		TRACE("Pop int expression, data index = %d, type index = %d\n",
			m_DataStackIndex, m_DataTypeStackIndex);
		return (int *) & m_DataStack[m_DataStackIndex];
	}
	else if (eDoubleConstant == type)
	{
		double * pSrc = PopDouble();
		PushConstant(int(*pSrc));
		return PopInt();
	}
	else if (eDoubleVariable == type
			|| eDoubleExpression == type)
	{
		// convert to int
		double * pSrc = PopDouble();
		int * pDst = PushInt();
		AddOperation(DoubleToInt, pDst, pSrc, NULL);
		return PopInt();
	}
	throw "Internal Error";
}

double * CExpressionEvaluationContext::PopDouble()
{
	if (m_DataTypeStackIndex <= 0)
	{
		throw "Expression Evaluation Stack Underflow";
	}
	TokenType type = m_DataTypeStack[m_DataTypeStackIndex - 1];
	if (eDoubleConstant == type
		|| eDoubleVariable == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex--;
		TRACE("Pop double constant, data index = %d, type index = %d\n",
			m_DataStackIndex, m_DataTypeStackIndex);
		return (double *) m_DataStack[m_DataStackIndex];
	}
	else if (eDoubleExpression == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex -= 2;
		TRACE("Pop double expression, data index = %d, type index = %d\n",
			m_DataStackIndex, m_DataTypeStackIndex);
		return (double *) & m_DataStack[m_DataStackIndex];
	}
	else if (eIntConstant == type
			|| eIntVariable == type
			|| eIntExpression == type)
	{
		// convert to double
		int * pSrc = PopInt();
		double * pDst = PushDouble();
		AddOperation(IntToDouble, pDst, pSrc, NULL);
		return PopDouble();
	}
	throw "Internal Error";
}

BOOL CExpressionEvaluationContext::SetExpression(LPCTSTR * ppszExpression)
{
	// parse the string
	m_ErrorString.Empty();
	try {
		m_DataStackIndex = 0;
		m_DataTypeStackIndex = 0;
		m_ConstantBufferIndex = 0;
		m_OperationArray.clear();

		if (eEndOfExpression != CompileExpression(ppszExpression))
		{
			throw "Expression syntax error";
		}
		// norm the sample
		PushConstant(32767);
		CompileMultiply();
		m_pResultAddress = PopInt();
	}
	catch (char * Error)
	{
		m_ErrorString = Error;
		return FALSE;
	}
	return TRUE;
}

void CExpressionEvaluationContext::Evaluate()
{
	for (unsigned i = 0; i < m_OperationArray.size(); i++)
	{
		m_OperationArray[i].Function( & m_OperationArray[i]);
	}
}

/////////////////  CCommitFileSaveContext
CCommitFileSaveContext::CCommitFileSaveContext(CWaveSoapFrontDoc * pDoc,
												UINT StatusStringId, CWaveFile & WavFile, int flags, LPCTSTR TargetName)
	: COperationContext(pDoc, OperationContextDiskIntensive, StatusStringId)
	, m_FileSaveFlags(flags)
	, m_TargetName(TargetName)
	, m_File(WavFile)
	, m_TotalCommitted(0)
{
}

BOOL CCommitFileSaveContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_File.InitializeTheRestOfFile(500, & m_TotalCommitted))
	{
		m_Flags |= OperationContextFinished;
	}
	return TRUE;
}

void CCommitFileSaveContext::PostRetire()
{
	m_File.Close();
	if ((m_Flags & OperationContextFinished)
		&& pDocument->PostCommitFileSave(m_FileSaveFlags, m_TargetName))
	{
		if (pDocument->m_bClosePending)
		{
			pDocument->m_bCloseThisDocumentNow = true;
		}
	}
	else
	{
		pDocument->m_bClosePending = false;
	}
	BaseClass::PostRetire();
}

void CConversionContext::PostRetire()
{
	if (m_DstFile.GetDataChunk()->dwDataOffset != 0)
	{
		// update data chunk and number of samples
		m_DstFile.SetFactNumberOfSamples(
										(m_SrcPos - m_SrcStart) / m_SrcFile.SampleSize());

		// BUGBUG For non-RIFF file, don't add an extra byte
		m_DstPos = (m_DstPos + 1) & ~1;
		m_DstFile.SetDatachunkLength(m_DstPos - m_DstStart);
	}

	BaseClass::PostRetire();
}

CEqualizerContext::CEqualizerContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId),
	m_bZeroPhase(FALSE)
{
}

CEqualizerContext::~CEqualizerContext()
{
}

BOOL CEqualizerContext::Init()
{
	if (m_bZeroPhase)
	{
		m_NumberOfBackwardPasses = 1;
	}
	return InitPass(1);
}

BOOL CEqualizerContext::InitPass(int /*nPass*/)
{
	//TRACE("CEqualizerContext::InitPass %d\n", nPass);

	for (int ch = 0; ch < countof (m_PrevSamples); ch++)
	{
		for (int i = 0; i < MaxNumberOfEqualizerBands; i++)
		{
			m_PrevSamples[ch][i][0] = 0.;
			m_PrevSamples[ch][i][1] = 0.;
			m_PrevSamples[ch][i][2] = 0.;
			m_PrevSamples[ch][i][3] = 0.;
		}
	}
	return TRUE;
}

double CEqualizerContext::CalculateResult(int ch, int Input)
{
	double tmp = Input;
	for (int i = 0; i < m_NumOfBands; i++)
	{
		double tmp1 = tmp * m_BandCoefficients[i][0]
					+ m_PrevSamples[ch][i][0] * m_BandCoefficients[i][1]
					+ m_PrevSamples[ch][i][1] * m_BandCoefficients[i][2]
					- m_PrevSamples[ch][i][2] * m_BandCoefficients[i][4]
					- m_PrevSamples[ch][i][3] * m_BandCoefficients[i][5];
		m_PrevSamples[ch][i][1] = m_PrevSamples[ch][i][0];
		m_PrevSamples[ch][i][0] = tmp;
		m_PrevSamples[ch][i][3] = m_PrevSamples[ch][i][2];
		m_PrevSamples[ch][i][2] = tmp1;
		tmp = tmp1;
	}
	return tmp;
}

BOOL CEqualizerContext::ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION /*offset*/, BOOL bBackward)
{
	// calculate number of sample, and time
	int nChannels = m_DstFile.Channels();
	//int nSample = (offset - m_DstStart) / nSampleSize;
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pDst[0];

	//ASSERT(0 == (offset % m_DstFile.SampleSize()));
	ASSERT(0 == (BufferLength % nChannels));

	if ( ! bBackward)
	{
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += nChannels)
		{
			for (int ch = 0; ch < nChannels; ch++)
			{
				if (m_DstChan & (1 << ch))
				{
					pDst[i + ch] = DoubleToShort(CalculateResult(ch, pDst[i + ch]));
				}
			}
		}
	}
	else
	{
		for (NUMBER_OF_SAMPLES i = nSamples - nChannels; i >=0; i -= nChannels)
		{
			for (int ch = 0; ch < nChannels; ch++)
			{
				if (m_DstChan & (1 << ch))
				{
					pDst[i + ch] = DoubleToShort(CalculateResult(ch, pDst[i + ch]));
				}
			}
		}
	}

	return TRUE;
}

BOOL CSwapChannelsContext::ProcessBuffer(void * buf, size_t BufferLength,
										SAMPLE_POSITION /*offset*/,
										BOOL /*bBackward*/)
{
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pDst[0];

	ASSERT(2 == m_DstFile.Channels());

	for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += 2)
	{
		WAVE_SAMPLE tmp = pDst[i];
		pDst[i] = pDst[i + 1];
		pDst[i + 1] = tmp;
	}
	return TRUE;
}

CFilterContext::CFilterContext(CWaveSoapFrontDoc * pDoc,
								UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId),
	m_bZeroPhase(FALSE)
{
}

CFilterContext::~CFilterContext()
{
}

BOOL CFilterContext::Init()
{
	if (m_bZeroPhase)
	{
		m_NumberOfBackwardPasses = 1;
	}
	return InitPass(1);
}

BOOL CFilterContext::InitPass(int /*nPass*/)
{
	//TRACE("CFilterContext::InitPass %d\n", nPass);

	for (int ch = 0; ch < countof (m_PrevLpfSamples); ch++)
	{
		for (int i = 0; i < MaxFilterOrder; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				m_PrevLpfSamples[ch][i][j] = 0.;

				m_PrevHpfSamples[ch][i][j] = 0.;

				m_PrevNotchSamples[ch][i][j] = 0.;
			}
		}
	}
	return TRUE;
}

double CFilterContext::CalculateResult(int ch, int Input)
{
	double in = Input;
	if (0 != m_nLpfOrder)
	{
		double out = 0;
		for (int i = 0; i < m_nLpfOrder; i++)
		{
			double tmp = in * m_LpfCoeffs[i][0]
						+ m_PrevLpfSamples[ch][i][0] * m_LpfCoeffs[i][1]
						+ m_PrevLpfSamples[ch][i][1] * m_LpfCoeffs[i][2]
						- m_PrevLpfSamples[ch][i][2] * m_LpfCoeffs[i][4]
						- m_PrevLpfSamples[ch][i][3] * m_LpfCoeffs[i][5];

			// protect against underflow (it slows the calculations tremendously)
			if (fabs(tmp) < 1E-32)
			{
				tmp = 0;
			}

			m_PrevLpfSamples[ch][i][1] = m_PrevLpfSamples[ch][i][0];
			m_PrevLpfSamples[ch][i][0] = in;
			m_PrevLpfSamples[ch][i][3] = m_PrevLpfSamples[ch][i][2];
			m_PrevLpfSamples[ch][i][2] = tmp;
			out += tmp;
		}
		in = out;
	}
	if (0 != m_nHpfOrder)
	{
		double out = 0;
		for (int i = 0; i < m_nHpfOrder; i++)
		{
			double tmp = in * m_HpfCoeffs[i][0]
						+ m_PrevHpfSamples[ch][i][0] * m_HpfCoeffs[i][1]
						+ m_PrevHpfSamples[ch][i][1] * m_HpfCoeffs[i][2]
						- m_PrevHpfSamples[ch][i][2] * m_HpfCoeffs[i][4]
						- m_PrevHpfSamples[ch][i][3] * m_HpfCoeffs[i][5];

			// protect against underflow (it slows the calculations tremendously)
			if (fabs(tmp) < 1E-32)
			{
				tmp = 0;
			}

			m_PrevHpfSamples[ch][i][1] = m_PrevHpfSamples[ch][i][0];
			m_PrevHpfSamples[ch][i][0] = in;
			m_PrevHpfSamples[ch][i][3] = m_PrevHpfSamples[ch][i][2];
			m_PrevHpfSamples[ch][i][2] = tmp;
			out += tmp;
		}
		in = out;
	}
	if (0 != m_nNotchOrder)
	{
		for (int i = 0; i < m_nNotchOrder; i++)
		{
			double tmp = in * m_NotchCoeffs[i][0]
						+ m_PrevNotchSamples[ch][i][0] * m_NotchCoeffs[i][1]
						+ m_PrevNotchSamples[ch][i][1] * m_NotchCoeffs[i][2]
						- m_PrevNotchSamples[ch][i][2] * m_NotchCoeffs[i][4]
						- m_PrevNotchSamples[ch][i][3] * m_NotchCoeffs[i][5];

			// protect against underflow (it slows the calculations tremendously)
			if (fabs(tmp) < 1E-32)
			{
				tmp = 0;
			}

			m_PrevNotchSamples[ch][i][1] = m_PrevNotchSamples[ch][i][0];
			m_PrevNotchSamples[ch][i][0] = in;
			m_PrevNotchSamples[ch][i][3] = m_PrevNotchSamples[ch][i][2];
			m_PrevNotchSamples[ch][i][2] = tmp;
			in = tmp;
		}
	}
	return in;
}

BOOL CFilterContext::ProcessBuffer(void * buf, size_t BufferLength,
									SAMPLE_POSITION /*offset*/, BOOL bBackward)
{
	// calculate number of sample, and time
	int nChannels = m_DstFile.Channels();

	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pDst[0];

	if ( ! bBackward)
	{
		for (NUMBER_OF_SAMPLES i = 0; i < nSamples; i += nChannels)
		{
			for (int ch = 0; ch < nChannels; ch++)
			{
				if (m_DstChan & (1 << ch))
				{
					pDst[i + ch] = DoubleToShort(CalculateResult(ch, pDst[i + ch]));
				}
			}
		}
	}
	else
	{
		for (NUMBER_OF_SAMPLES i = nSamples - nChannels; i >=0; i -= nChannels)
		{
			for (int ch = 0; ch < nChannels; ch++)
			{
				if (m_DstChan & (1 << ch))
				{
					pDst[i + ch] = DoubleToShort(CalculateResult(ch, pDst[i + ch]));
				}
			}
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////
CCdReadingContext::CCdReadingContext(CWaveSoapFrontDoc * pDoc,
									LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, OperationContextDiskIntensive | OperationContextSerialized,
				StatusString, OperationName),
	m_pCdBuffer(NULL),
	m_CdBufferFilled(0),
	m_hEvent(NULL),
	m_CdDataOffset(0),
	m_pNextTrackContext(NULL),
	m_TargetFileType(0),
	m_bSaveImmediately(FALSE),
	m_CdBufferSize(0)
{
	m_GetBufferFlags = CDirectFile::GetBufferWriteOnly | CDirectFile::GetBufferNoPrefetch;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty | CDirectFile::ReturnBufferFlush;
}

BOOL CCdReadingContext::InitTrackInformation(CCdDrive const & Drive,
											CdTrackInfo * pTrack,
											DWORD TargetFileType,
											WAVEFORMATEX const * pTargetFormat)
{
	CWaveFormat wfx;
	wfx.InitCdAudioFormat();

	m_Drive = Drive;
	m_CdAddress = pTrack->TrackBegin;
	m_NumberOfSectors = pTrack->NumSectors;

	m_TrackName = pTrack->Track;
	m_TrackFileName = pTrack->TrackFileName;
	m_TrackAlbum = pTrack->Album;
	m_TrackArtist = pTrack->Artist;
	m_TargetFormat = pTargetFormat;
	m_TargetFileType = TargetFileType;

	CWaveFile WaveFile;

	ULONG flags = CreateWaveFileTempDir
				| CreateWaveFileDeleteAfterClose
				| CreateWaveFilePcmFormat
				| CreateWaveFileTemp;

	LPCTSTR FileName = NULL;
	if (WAVE_FORMAT_PCM == pTargetFormat->wFormatTag)
	{
		flags = CreateWaveFileDeleteAfterClose
				| CreateWaveFilePcmFormat
				| CreateWaveFileTemp;
		FileName = m_TrackFileName;
	}

	ULONG nSamples = m_NumberOfSectors * CDDASectorSize / 4;
	if ( ! CanAllocateWaveFileSamplesDlg(wfx, nSamples))
	{
		return FALSE;
	}

	if (FALSE == WaveFile.CreateWaveFile(NULL, wfx, ALL_CHANNELS,
										nSamples, flags, FileName))
	{
		AfxMessageBox(IDS_UNABLE_TO_CREATE_NEW_FILE, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	InitDestination(WaveFile, 0, nSamples, 2, FALSE);

	m_Drive.DisableMediaChangeDetection();
	m_Drive.LockDoor();
	return TRUE;
}

BOOL CCdReadingContext::ProcessBuffer(void * buf, size_t len,
									SAMPLE_POSITION /*offset*/,
									BOOL /*bBackward*/)
{
	char * pBuf = (char*) buf;
	while (len != 0)
	{
		if (0 == m_CdBufferFilled)
		{
			size_t SectorsToRead = m_CdBufferSize / CDDASectorSize;
			if (SectorsToRead > m_NumberOfSectors)
			{
				SectorsToRead = m_NumberOfSectors;
			}
			if (0 == SectorsToRead)
			{
				memset(pBuf, 0, len);
				return TRUE;
			}

			DWORD ReadBeginTime = timeGetTime();
			if ( ! m_Drive.ReadCdData(m_pCdBuffer, m_CdAddress, SectorsToRead))
			{
				return FALSE;
			}

			DWORD ElapsedReadTime = timeGetTime() - ReadBeginTime;
			// pace the reading process
			DWORD delay;
			if (0 && ElapsedReadTime < 500)
			{
				timeSetEvent(delay, 2, LPTIMECALLBACK(m_hEvent), NULL, TIME_CALLBACK_EVENT_SET);
				WaitForSingleObject(m_hEvent, INFINITE);
			}

			m_NumberOfSectors -= SectorsToRead;
			m_CdAddress = LONG(m_CdAddress) + SectorsToRead;
			m_CdDataOffset = 0;
			m_CdBufferFilled = SectorsToRead * CDDASectorSize;
		}

		size_t ToCopy = m_CdBufferFilled;
		if (ToCopy > len)
		{
			ToCopy = len;
		}
		memcpy(pBuf, m_CdDataOffset + (char*)m_pCdBuffer, ToCopy);

		len -= ToCopy;
		m_CdBufferFilled -= ToCopy;

		pBuf += ToCopy;
		m_CdDataOffset += ToCopy;
	}
	return TRUE;
}

BOOL CCdReadingContext::Init()
{
	// allocate buffer. Round to sector size multiple
	m_Drive.SetDriveBusy();
	m_CdBufferFilled = 0;
	m_CdDataOffset = 0;
	m_CdBufferSize = 0x10000 - 0x10000 % CDDASectorSize;

	timeBeginPeriod(2);

	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pCdBuffer = VirtualAlloc(NULL, m_CdBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (NULL == m_pCdBuffer)
	{
		return FALSE;
	}

	m_Drive.SetReadSpeed(m_RequiredReadSpeed, m_CdAddress - 150, m_NumberOfSectors);
	m_Drive.ReadCdData(m_pCdBuffer, m_CdAddress, m_CdBufferSize / CDDASectorSize);
	return TRUE;
}

void CCdReadingContext::DeInit()
{
	if (NULL != m_pCdBuffer)
	{
		VirtualFree(m_pCdBuffer, 0, MEM_RELEASE);
		m_pCdBuffer = NULL;
	}
	if (NULL != m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	timeEndPeriod(2);
	if (0 == (m_Flags & OperationContextFinished)
		|| NULL == m_pNextTrackContext)
	{
		TRACE("CD drive speed reset to original %d\n", m_OriginalReadSpeed);
		m_Drive.SetReadSpeed(m_OriginalReadSpeed);
		// stop drive:
		m_Drive.StopDrive();
	}
	m_Drive.SetDriveBusy(false);
}

void CCdReadingContext::Execute()
{
	// create new document, assign a file to it
	NewFileParameters Params(m_TargetFormat,
							m_NumberOfSectors * (CDDASectorSize / 4));

	Params.m_pInitialName = m_TrackFileName;
	Params.m_FileTypeFlags = m_TargetFileType | OpenDocumentCreateNewFromCWaveFile;
	Params.m_pFile = & m_DstFile;

	CThisApp * pApp = GetApp();
	POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
	CDocTemplate* pTemplate = pApp->m_pDocManager->GetNextDocTemplate(pos);

	if (NULL == pTemplate)
	{
		delete this;
		return;
	}

	pDocument = (CWaveSoapFrontDoc *)pTemplate->OpenDocumentFile(
																(LPCTSTR) & Params,
																OpenDocumentCreateNewWithParameters);
	if (NULL == pDocument)
	{
		delete this;
		return;
	}

	pDocument->SetModifiedFlag();
	BaseClass::Execute();
}

void CCdReadingContext::PostRetire()
{
	// if the operation was not successfully completed, truncate the file.

	if (0 == (m_Flags & OperationContextFinished))
	{
		if (0 == (m_Flags & OperationContextStop))
		{
			CString s;
			s.Format(IDS_CD_READING_ERROR_FORMAT,
					m_CdAddress.Minute, m_CdAddress.Second);
			AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		}

		// if no data has been read, delete the document
		if (m_DstPos == m_DstStart)
		{
			pDocument->m_bCloseThisDocumentNow = true;
		}
		else if (m_DstFile.SetDatachunkLength(m_DstPos - m_DstStart))
		{
			pDocument->SoundChanged(m_DstFile.GetFileID(), 0, 0, m_DstFile.NumberOfSamples());
		}
	}
	else
	{
		if (m_bSaveImmediately)
		{
			pDocument->m_bClosing = true;
			pDocument->DoFileSave();
		}
		if (NULL != m_pNextTrackContext)
		{
			CCdReadingContext * pContext = m_pNextTrackContext;
			m_pNextTrackContext = NULL;
			pContext->Execute();
		}
	}
	BaseClass::PostRetire();
}

CCdReadingContext::~CCdReadingContext()
{
	delete m_pNextTrackContext;
}

/////////////// CReplaceFileContext ////////////////
CReplaceFileContext::CReplaceFileContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName,
										CWaveFile & NewFile, bool bNewDirectMode)
	: BaseClass(pDoc, OperationContextSynchronous, OperationName, OperationName)
	, m_File(NewFile)
	, m_bNewDirectMode(bNewDirectMode)
{
}

BOOL CReplaceFileContext::CreateUndo()
{
	CReplaceFileContext * pUndo =
		new CReplaceFileContext(pDocument, m_OperationName, pDocument->m_WavFile,
								pDocument->m_bDirectMode);
	if (NULL != pUndo)
	{
		m_UndoChain.InsertHead(pUndo);
		return TRUE;
	}
	return FALSE;
}

bool CReplaceFileContext::KeepsPermanentFileReference() const
{
	return m_File.IsOpen()
			&& ! m_File.IsTemporaryFile();
}

BOOL CReplaceFileContext::OperationProc()
{
	pDocument->m_WavFile = m_File;

	NUMBER_OF_SAMPLES nSamples = pDocument->WaveFileSamples();

	pDocument->SoundChanged(pDocument->WaveFileID(),
							0, nSamples, nSamples, UpdateSoundDontRescanPeaks);
	// save/restore "Direct" flag and update title
	pDocument->m_bDirectMode = m_bNewDirectMode;

	pDocument->UpdateFrameTitles();        // will cause name change in views
	pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateWholeFileChanged);

	return TRUE;
}

///////////// CReplaceFormatContext //////////////////
CReplaceFormatContext::CReplaceFormatContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName,
											WAVEFORMATEX const * pNewFormat)
	: BaseClass(pDoc, OperationContextSynchronous, OperationName, OperationName),
	m_NewWaveFormat(pNewFormat)
{
}

CReplaceFormatContext::CReplaceFormatContext(CWaveSoapFrontDoc * pDoc, UINT OperationNameId,
											WAVEFORMATEX const * pNewFormat)
	: BaseClass(pDoc, OperationContextSynchronous, OperationNameId, OperationNameId),
	m_NewWaveFormat(pNewFormat)
{
}

BOOL CReplaceFormatContext::CreateUndo()
{
	CReplaceFormatContext * pUndo =
		new CReplaceFormatContext(pDocument, m_OperationName, pDocument->WaveFormat());

	m_UndoChain.InsertHead(pUndo);
	return TRUE;
}

BOOL CReplaceFormatContext::OperationProc()
{
	// replace format to the one in m_NewWaveFormat
	pDocument->SetWaveFormat(m_NewWaveFormat);

//    pDocument->UpdateFrameTitles();        // will cause name change in views
	pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateSampleRateChanged);
	return TRUE;
}

///////////////// CLengthChangeOperation
CLengthChangeOperation::CLengthChangeOperation(CWaveSoapFrontDoc * pDoc,
												CWaveFile & File, MEDIA_FILE_SIZE NewLength)
	: BaseClass(pDoc, OperationContextSynchronous,
				IDS_STATUS_PROMPT_CHANGE_LENGTH, IDS_OPERATION_NAME_CHANGE_LENGTH)
	, m_NewLength(NewLength)
	, m_File(File)
{
}

BOOL CLengthChangeOperation::CreateUndo()
{
	if ( ! m_File.IsOpen()
		|| m_File.GetFileID() != pDocument->WaveFileID())
	{
		return TRUE;
	}

	CLengthChangeOperation * pUndo = new CLengthChangeOperation(pDocument,
																m_File, m_File.GetLength());

	m_UndoChain.InsertHead(pUndo);
	return TRUE;
}

BOOL CLengthChangeOperation::OperationProc()
{
	if ( ! m_File.SetFileLength(m_NewLength))
	{
		NotEnoughDiskSpaceMessageBox();
		return FALSE;
	}
	return TRUE;
}

BOOL CLengthChangeOperation::PrepareUndo()
{
	m_File = pDocument->m_WavFile;
	return TRUE;
}

void CLengthChangeOperation::UnprepareUndo()
{
	m_File.Close();
}

//////////////// CWaveSamplesChangeOperation /////////
CWaveSamplesChangeOperation::CWaveSamplesChangeOperation(CWaveSoapFrontDoc * pDoc,
														CWaveFile & File, NUMBER_OF_SAMPLES NewSamples)
	: BaseClass(pDoc, OperationContextSynchronous, IDS_STATUS_PROMPT_CHANGE_LENGTH,
				IDS_OPERATION_NAME_CHANGE_LENGTH)
	, m_NewSamples(NewSamples)
	, m_File(File)
{
}

BOOL CWaveSamplesChangeOperation::CreateUndo()
{
	if ( ! m_File.IsOpen()
		|| m_File.GetFileID() != pDocument->WaveFileID())
	{
		return TRUE;
	}

	CWaveSamplesChangeOperation * pUndo = new CWaveSamplesChangeOperation(pDocument,
											m_File, m_File.NumberOfSamples());

	m_UndoChain.InsertHead(pUndo);
	return TRUE;
}

BOOL CWaveSamplesChangeOperation::PrepareUndo()
{
	m_File = pDocument->m_WavFile;
	return TRUE;
}

void CWaveSamplesChangeOperation::UnprepareUndo()
{
	m_File.Close();
}

BOOL CWaveSamplesChangeOperation::OperationProc()
{
	DWORD Flags = UpdateSoundDontRescanPeaks;
	SAMPLE_INDEX UpdateBegin = 0;
	SAMPLE_INDEX UpdateEnd = 0;

	if (m_NewSamples < m_File.NumberOfSamples()
		&& m_NewSamples != 0)
	{
		Flags = 0;
		// rescan last peak granule
		UpdateBegin = m_NewSamples - 1;
		UpdateEnd = m_NewSamples;
	}

	if ( ! m_File.SetFileLengthSamples(m_NewSamples))
	{
		NotEnoughDiskSpaceMessageBox();
		return FALSE;
	}

	pDocument->SoundChanged(m_File.GetFileID(), UpdateBegin, UpdateEnd, m_NewSamples, Flags);

	return TRUE;
}

////////////// CMoveOperation ///////////////
CMoveOperation::CMoveOperation(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
	: BaseClass(pDoc, StatusString, OperationName)
	, m_pUndoMove(NULL)
{
}

CMoveOperation::CMoveOperation(CWaveSoapFrontDoc * pDoc, UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
	, m_pUndoMove(NULL)
{
}

CMoveOperation::~CMoveOperation()
{
	delete m_pUndoMove;
}

void CMoveOperation::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);

	if (NULL != m_pUndoMove)
	{
		TRACE(" %*.sUNDO MOVE:\n", indent, "");
		m_pUndoMove->Dump(indent+2);
	}
}

BOOL CMoveOperation::InitMove(CWaveFile & File,
							SAMPLE_INDEX SrcStartSample,
							SAMPLE_INDEX DstStartSample,
							NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	//ASSERT(SrcStartSample + Length == File.NumberOfSamples());

	if (SrcStartSample > DstStartSample)
	{
		// shrinking the file
		InitSource(File, SrcStartSample, SrcStartSample + Length, Channel);

		SAMPLE_INDEX UndoEnd = min(SrcStartSample, DstStartSample + Length);
		InitDestination(File, DstStartSample, DstStartSample + Length, Channel, FALSE, DstStartSample, UndoEnd);
	}
	else
	{
		// expanding the file, start the operation from end
		InitSource(File, SrcStartSample + Length, SrcStartSample, Channel);
		InitDestination(File, DstStartSample + Length, DstStartSample, Channel, FALSE, 0, 0);
	}
	return TRUE;
}

BOOL CMoveOperation::CreateUndo()
{
	if ( ! m_DstFile.IsOpen()
		|| m_DstFile.GetFileID() != pDocument->WaveFileID()
		|| NULL != m_pUndoMove)
	{
		return TRUE;
	}

	CMoveOperation * pUndo = new CMoveOperation(pDocument);

	pUndo->m_SrcPos = m_DstPos;
	pUndo->m_SrcEnd = m_DstStart;
	pUndo->m_SrcStart = m_DstEnd;
	pUndo->m_SrcChan = m_DstChan;

	pUndo->m_DstPos = m_SrcPos;
	pUndo->m_DstEnd = m_SrcStart;
	pUndo->m_DstStart = m_SrcEnd;
	pUndo->m_DstChan = m_SrcChan;

	m_pUndoMove = pUndo;

	if ( ! BaseClass::CreateUndo())
	{
		return FALSE;
	}

	if (NULL != m_pUndoContext)
	{
		// don't allow REDO for that
		m_pUndoContext->m_UndoStartPos = 0;
		m_pUndoContext->m_UndoEndPos = 0;
	}

	return TRUE;
}

ListHead<COperationContext> * CMoveOperation::GetUndoChain()
{
	BaseClass::GetUndoChain();

	if (NULL != m_pUndoMove)
	{
		// adjust positions!
		m_pUndoMove->m_SrcStart = m_DstPos;
		m_pUndoMove->m_SrcPos = m_DstPos;

		m_pUndoMove->m_DstStart = m_SrcPos;
		m_pUndoMove->m_DstPos = m_SrcPos;

		if (m_pUndoMove->m_DstStart < m_pUndoMove->m_SrcStart)
		{
			// it will need REDO creation
			m_pUndoMove->m_UndoStartPos = m_pUndoMove->m_DstStart;
			m_pUndoMove->m_UndoEndPos = min(m_pUndoMove->m_SrcStart, m_pUndoMove->m_DstEnd);
		}

		// inserted to the head: will be executed first
		m_UndoChain.InsertHead(m_pUndoMove);

		m_pUndoMove = NULL;
	}

	return COperationContext::GetUndoChain();
}

void CMoveOperation::DeInit()
{
	// Copy context does it
	//m_SrcStart = m_SrcPos;
	//m_DstStart = m_DstPos;

	BaseClass::DeInit();
}

void CMoveOperation::DeleteUndo()
{
	delete m_pUndoMove;
	m_pUndoMove = NULL;

	BaseClass::DeleteUndo();
}

BOOL CMoveOperation::PrepareUndo()
{
	m_Flags &= ~(OperationContextStop | OperationContextFinished);
	m_SrcFile = pDocument->m_WavFile;
	m_DstFile = pDocument->m_WavFile;
	return TRUE;
}

void CMoveOperation::UnprepareUndo()
{
	m_SrcFile.Close();
	m_DstFile.Close();
}

BOOL CMoveOperation::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	if (m_SrcStart <= m_SrcEnd)
	{
		// normal operation in forward direction
		return CCopyContext::OperationProc();
	}

	// move data from end to begin (reverse direction)
	if (m_SrcPos <= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	if (m_DstPos <= m_DstEnd)
	{
		return FALSE;
	}

	DWORD dwStartTime = GetTickCount();
	SAMPLE_POSITION dwOperationBegin = m_DstPos;

	long LeftToRead = 0;
	long LeftToWrite = 0;
	long WasRead = 0;
	long WasLockedToWrite = 0;
	void * pOriginalSrcBuf = NULL;
	char * pSrcBuf = NULL;
	void * pOriginalDstBuf = NULL;
	char * pDstBuf = NULL;

	DWORD DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;
	WAVE_SAMPLE tmp[MAX_NUMBER_OF_CHANNELS];
	int const SrcSampleSize = m_SrcFile.SampleSize();
	int const DstSampleSize = m_DstFile.SampleSize();
	NUMBER_OF_CHANNELS const NumSrcChannels = m_SrcFile.Channels();
	NUMBER_OF_CHANNELS const NumDstChannels = m_DstFile.Channels();

	do
	{
		if (0 == LeftToRead)
		{
			MEDIA_FILE_SIZE SizeToRead = MEDIA_FILE_SIZE(m_SrcEnd) - MEDIA_FILE_SIZE(m_SrcPos);
			if (SizeToRead < -CDirectFile::CacheBufferSize())
			{
				SizeToRead = -CDirectFile::CacheBufferSize();
			}
			WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
												SizeToRead, m_SrcPos, CDirectFile::GetBufferAndPrefetchNext);

			if (0 == WasRead)
			{
				m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);

				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}

		if (0 == LeftToWrite)
		{
			DstFileFlags = CDirectFile::GetBufferAndPrefetchNext;

			MEDIA_FILE_SIZE SizeToWrite = MEDIA_FILE_SIZE(m_DstEnd) - MEDIA_FILE_SIZE(m_DstPos);

			if (SizeToWrite < -CDirectFile::CacheBufferSize())
			{
				SizeToWrite = -CDirectFile::CacheBufferSize();
			}

			// if source and destination overlap less than BufferSize,
			// then we cannot use WriteOnly flag
			if (m_DstFile.AllChannels(m_DstChan)
				&& (m_SrcFile.GetFileID() != m_DstFile.GetFileID()
					|| m_SrcPos + CDirectFile::CacheBufferSize() < m_DstPos
					|| m_DstPos + CDirectFile::CacheBufferSize() < m_SrcPos)
				&& (NULL == m_pUndoContext
					|| ! m_pUndoContext->NeedToSaveUndo(m_DstPos, long(SizeToWrite))))
			{
				DstFileFlags = CDirectFile::GetBufferWriteOnly;
			}

			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstPos, DstFileFlags);

			if (0 == WasLockedToWrite)
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
				return FALSE;
			}

			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;

		}

		unsigned SrcSamples = -LeftToRead / SrcSampleSize;
		unsigned DstSamples = -LeftToWrite / DstSampleSize;

		if (SrcSamples != 0
			&& DstSamples != 0)
		{
			unsigned Samples = std::min(SrcSamples, DstSamples);
			// save the changed data to undo buffer
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->SaveUndoData(pDstBuf,
											-signed(Samples) * DstSampleSize,
											m_DstPos, NumDstChannels);
			}
			// CopyWaveSamples doesn't work backwards
			for (unsigned i = 0; i < Samples; i++)
			{
				pDstBuf -= DstSampleSize;
				pSrcBuf -= SrcSampleSize;
				// copy one by one sample
				CopyWaveSamples(pDstBuf, m_DstChan, NumDstChannels,
								pSrcBuf, m_SrcChan, NumSrcChannels,
								1, m_DstFile.GetSampleType(), m_SrcFile.GetSampleType());
			}

			unsigned DstCopied = Samples * DstSampleSize;
			LeftToWrite += DstCopied;
			m_DstPos -= DstCopied;

			unsigned SrcCopied = Samples * SrcSampleSize;
			LeftToRead += SrcCopied;
			m_SrcPos -= SrcCopied;
		}
		else
		{
			// save the changed data to undo buffer
			if (NULL != m_pUndoContext
				&& m_pUndoContext->NeedToSaveUndo(m_DstPos, -DstSampleSize))
			{
				m_DstFile.ReadSamples(ALL_CHANNELS,
									m_DstPos, -1, tmp + NumSrcChannels,
									m_DstFile.GetSampleType());

				m_pUndoContext->SaveUndoData(tmp + NumSrcChannels,
											-DstSampleSize, m_DstPos, NumDstChannels);
			}
			// read one sample directly
			if (-1 != m_SrcFile.ReadSamples(ALL_CHANNELS,
											m_SrcPos, -1, tmp + NumSrcChannels, m_SrcFile.GetSampleType())
				|| -1 != m_DstFile.WriteSamples(m_DstChan, m_DstPos, -1,
												tmp + NumSrcChannels, m_SrcChan, NumSrcChannels, m_SrcFile.GetSampleType()))
			{
				// error
				TRACE("Transfering a split sample was unsuccessful!\n");
				m_Flags |= OperationContextFinished;
				break;
			}

			pSrcBuf -= SrcSampleSize;
			m_SrcPos += SrcSampleSize;

			if (LeftToRead < -SrcSampleSize)
			{
				LeftToRead += SrcSampleSize;
			}
			else
			{
				LeftToRead = 0;
			}

			pDstBuf -= DstSampleSize;
			m_DstPos -= DstSampleSize;
			if (LeftToWrite < -DstSampleSize)
			{
				LeftToWrite += DstSampleSize;
			}
			else
			{
				LeftToWrite = 0;
			}
		}

		if (0 == LeftToRead)
		{
			m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
	}
	while (((DstFileFlags & CDirectFile::GetBufferWriteOnly)
				// cannot exit while write-only buffer is incomplete
				&& 0 != WasLockedToWrite)
			|| (m_SrcPos > m_SrcEnd
				&& GetTickCount() - dwStartTime < 200)
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	// notify the view
	pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);

	return TRUE;
}

////////////// CSaveTrimmedOperation
// This operation context saves the data being later trimmed, for
// later UNDO.

CSaveTrimmedOperation::CSaveTrimmedOperation(CWaveSoapFrontDoc * pDoc,
											CWaveFile & SrcFile,
											SAMPLE_INDEX SrcStartSample,
											SAMPLE_INDEX SrcEndSample,
											CHANNEL_MASK Channels)
	: BaseClass(pDoc)
	, m_pRestoreOperation(NULL)
{
	m_SrcFile = SrcFile;

	m_UndoStartPos = SrcFile.SampleToPosition(SrcStartSample);
	m_DstStart = m_UndoStartPos;
	m_SrcStart = m_UndoStartPos;
	m_DstPos = m_UndoStartPos;
	m_SrcPos = m_UndoStartPos;

	m_UndoEndPos = SrcFile.SampleToPosition(SrcEndSample);
	m_DstEnd = m_UndoEndPos;
	m_SrcEnd = m_UndoEndPos;

	m_SrcChan = Channels;
	m_DstChan = ALL_CHANNELS;
}

CSaveTrimmedOperation::~CSaveTrimmedOperation()
{
	delete m_pRestoreOperation;
}

void CSaveTrimmedOperation::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);

	if (NULL != m_pRestoreOperation)
	{
		TRACE("%*.sRestore UNDO:\n", indent, "");
		m_pRestoreOperation->Dump(indent + 2);
	}
}

BOOL CSaveTrimmedOperation::CreateUndo()
{
	// Only create undo, if the source file is the main document file
	if (NULL != m_pRestoreOperation
		|| ! m_SrcFile.IsOpen()
		|| m_SrcFile.GetFileID() != pDocument->WaveFileID()
		|| m_UndoStartPos == m_UndoEndPos)
	{
		return TRUE;
	}

	CRestoreTrimmedOperation::auto_ptr
	pUndo(new CRestoreTrimmedOperation(pDocument));

	// this operation doesn't require saving REDO data during UNDO
	if ( ! pUndo->InitUndoCopy(m_SrcFile, m_UndoStartPos, m_UndoEndPos, m_SrcChan, 0, 0))
	{
		return FALSE;
	}

	m_pRestoreOperation = pUndo.release();

	m_DstFile = m_pRestoreOperation->m_SrcFile;

	m_DstStart = m_pRestoreOperation->m_SrcStart;
	m_DstPos = m_DstStart;
	m_DstEnd = m_pRestoreOperation->m_SrcEnd;

	m_SrcStart = m_pRestoreOperation->m_DstStart;
	m_SrcPos = m_SrcStart;
	m_SrcEnd = m_pRestoreOperation->m_DstEnd;

	return TRUE;
}

ListHead<COperationContext> * CSaveTrimmedOperation::GetUndoChain()
{
	if (NULL != m_pRestoreOperation)
	{
		m_pRestoreOperation->m_SrcEnd = m_DstPos;
		m_pRestoreOperation->m_DstEnd = m_SrcPos;
		m_UndoStartPos = m_SrcPos;

		m_UndoChain.InsertHead(m_pRestoreOperation);
		m_pRestoreOperation = NULL;
	}
	return BaseClass::GetUndoChain();
}

void CSaveTrimmedOperation::DeleteUndo()
{
	delete m_pRestoreOperation;
	m_pRestoreOperation = NULL;

	BaseClass::DeleteUndo();
}

BOOL CSaveTrimmedOperation::OperationProc()
{
	if (NULL == m_pRestoreOperation)
	{
		// UNDO was not requested for this
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	return BaseClass::OperationProc();
}

BOOL CSaveTrimmedOperation::PrepareUndo()
{
	m_Flags &= ~(OperationContextStop | OperationContextFinished);
	m_SrcFile = pDocument->m_WavFile;
	return TRUE;
}

void CSaveTrimmedOperation::UnprepareUndo()
{
	m_SrcFile.Close();
}

/////////////// CRestoreTrimmedOperation
CRestoreTrimmedOperation::CRestoreTrimmedOperation(CWaveSoapFrontDoc * pDoc)
	: BaseClass(pDoc)
	, m_pSaveOperation(NULL)
{
}

CRestoreTrimmedOperation::~CRestoreTrimmedOperation()
{
	delete m_pSaveOperation;
}

void CRestoreTrimmedOperation::Dump(unsigned indent) const
{
	BaseClass::Dump(indent);

	if (NULL != m_pSaveOperation)
	{
		TRACE(" %*.sSave UNDO:\n", indent, "");
		m_pSaveOperation->Dump(indent + 2);
	}
}

BOOL CRestoreTrimmedOperation::CreateUndo()
{
	// Only create undo, if the source file is the main document file
	if (NULL != m_pSaveOperation
		|| ! m_DstFile.IsOpen()
		|| m_DstFile.GetFileID() != pDocument->WaveFileID()
		|| m_DstStart == m_DstEnd)
	{
		return TRUE;
	}

	// a reference on the main document file will be closed in UnprepareUndo
	m_pSaveOperation = new CSaveTrimmedOperation(pDocument,
												m_DstFile,
												m_DstFile.PositionToSample(m_DstStart),
												m_DstFile.PositionToSample(m_DstEnd),
												m_DstChan);

	return TRUE;
}

ListHead<COperationContext> * CRestoreTrimmedOperation::GetUndoChain()
{
	if (NULL != m_pSaveOperation)
	{
		m_pSaveOperation->m_UndoEndPos = m_DstPos;
		m_pSaveOperation->m_SrcEnd = m_DstPos;
		m_pSaveOperation->m_DstEnd = m_SrcPos;
		m_DstStart = m_DstPos;

		m_UndoChain.InsertHead(m_pSaveOperation);
		m_pSaveOperation = NULL;
	}
	return BaseClass::GetUndoChain();
}

void CRestoreTrimmedOperation::DeleteUndo()
{
	delete m_pSaveOperation;
	m_pSaveOperation = NULL;

	BaseClass::DeleteUndo();
}

////////////// CInitChannels
// This operation fills the specified channels with zeros, from start to end
// The target file
// It doesn't save the erased information for UNDO. Instead, CInitChannelsUndo
// is used to create such operation for subsequent REDO
CInitChannels::CInitChannels(CWaveSoapFrontDoc * pDoc,
							CWaveFile & File, SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channels)
	: BaseClass(pDoc, 0)
{
	InitDestination(File, Start, End, Channels, FALSE, 0, 0);
}

// if this is used for UNDO:
// set its destination to the document file
BOOL CInitChannels::PrepareUndo()
{
	m_Flags &= ~(OperationContextStop | OperationContextFinished);
	m_DstFile = pDocument->m_WavFile;
	m_SrcPos = m_SrcStart;
	m_DstPos = m_DstStart;
	return TRUE;
}

void CInitChannels::DeInit()
{
	BaseClass::DeInit();
}

void CInitChannels::UnprepareUndo()
{
	m_DstFile.Close();
}

BOOL CInitChannels::CreateUndo()
{
	if ( ! m_DstFile.IsOpen()
		|| m_DstFile.GetFileID() != pDocument->WaveFileID())
	{
		return TRUE;
	}

	m_UndoChain.InsertHead(new CInitChannelsUndo(pDocument,
												m_DstStart, m_DstEnd, m_DstChan));
	return TRUE;
}

BOOL CInitChannels::ProcessBuffer(void * buf, size_t BufferLength,
								SAMPLE_POSITION /*offset*/,
								BOOL /*bBackward*/)
{
	ASSERT(m_DstFile.GetSampleType() == SampleType16bit);
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;

	int nChannels = m_DstFile.Channels();

	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pDst[0];

	//ASSERT(0 == (offset % m_DstFile.SampleSize()));
	ASSERT(0 == (BufferLength % m_DstFile.SampleSize()));

	NUMBER_OF_SAMPLES i;
	if (m_DstFile.AllChannels(m_DstChan))
	{
		for (i = 0; i < nSamples; i++)
		{
			pDst[i] = 0;
		}
	}
	else
	{
		for (i = 0; i < nSamples; i += nChannels)
		{
			for (long ch = 0, mask = 1; ch < nChannels; ch++, mask <<= 1)
			{
				if (m_DstChan & mask)
				{
					pDst[i + ch] = 0;
				}
			}
		}
	}

	return TRUE;
}

////////////////// CInitChannelsUndo
// This operation is a placeholder for subsequent CInitChannels REDO operation
CInitChannelsUndo::CInitChannelsUndo(CWaveSoapFrontDoc * pDoc,
									SAMPLE_POSITION Start, SAMPLE_POSITION End, CHANNEL_MASK Channels)
	: BaseClass(pDoc, 0)
{
	m_SrcStart = Start;
	m_SrcEnd = End;
	m_SrcChan = Channels;
}

BOOL CInitChannelsUndo::CreateUndo()
{
	m_UndoChain.InsertHead(new CInitChannels(pDocument,
											pDocument->m_WavFile,
											m_SrcStart, m_SrcEnd, m_SrcChan));
	return TRUE;
}

BOOL CInitChannelsUndo::OperationProc()
{
	m_Flags |= OperationContextFinished;
	return TRUE;
}

/////////// CSelectionChangeOperation
CSelectionChangeOperation::CSelectionChangeOperation(CWaveSoapFrontDoc * pDoc,
													SAMPLE_INDEX Start, SAMPLE_INDEX End, SAMPLE_INDEX Caret,
													CHANNEL_MASK Channels)
	: BaseClass(pDoc, OperationContextSynchronous)
	, m_Start(Start)
	, m_End(End)
	, m_Caret(Caret)
	, m_Channels(Channels)
{
}

BOOL CSelectionChangeOperation::CreateUndo()
{
	m_UndoChain.InsertHead(new CSelectionChangeOperation(pDocument,
														pDocument->m_SelectionStart, pDocument->m_SelectionEnd,
														pDocument->m_CaretPosition, pDocument->m_SelectedChannel));

	return TRUE;
}

BOOL CSelectionChangeOperation::OperationProc()
{
	pDocument->SetSelection(m_Start, m_End, m_Channels, m_Caret);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
CReverseOperation::CReverseOperation(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, OperationContextDiskIntensive, StatusStringId, OperationNameId)
	, m_pUndoLow(NULL)
	, m_pUndoHigh(NULL)
{
}

CReverseOperation::~CReverseOperation()
{
	delete m_pUndoLow;
	delete m_pUndoHigh;
}

BOOL CReverseOperation::InitDestination(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
										CHANNEL_MASK chan, BOOL NeedUndo)
{
	if ( ! CTwoFilesOperation::InitDestination(DstFile, StartSample, StartSample + (EndSample - StartSample) / 2,
												chan, FALSE))
	{
		return FALSE;
	}

	InitSource(DstFile, EndSample, EndSample - (EndSample - StartSample) / 2,
				chan);

	if (NeedUndo)
	{
		return CreateUndo();
	}
	return TRUE;
}

BOOL CReverseOperation::CreateUndo()
{
	if (NULL != m_pUndoLow
		|| ! m_DstFile.IsOpen()
		|| m_DstFile.GetFileID() != pDocument->WaveFileID())
	{
		return TRUE;
	}

	CCopyUndoContext::auto_ptr pUndo1(new CCopyUndoContext(pDocument));
	CCopyUndoContext::auto_ptr pUndo2(new CCopyUndoContext(pDocument));

	if ( ! pUndo1->InitUndoCopy(m_DstFile, m_DstStart, m_DstEnd, m_DstChan))
	{
		return FALSE;
	}

	if ( ! pUndo2->InitUndoCopy(m_DstFile, m_SrcStart, m_SrcEnd, m_DstChan))
	{
		return FALSE;
	}

	m_pUndoLow = pUndo1.release();
	m_pUndoHigh = pUndo2.release();

	return TRUE;
}

ListHead<COperationContext> * CReverseOperation::GetUndoChain()
{
	if (NULL != m_pUndoLow)
	{
		m_pUndoLow->m_DstEnd = m_pUndoLow->m_DstPos;
		m_pUndoLow->m_SrcEnd = m_pUndoLow->m_SrcPos;

		// save for REDO all undone
		m_pUndoLow->m_UndoStartPos = m_pUndoLow->m_DstStart;
		m_pUndoLow->m_UndoEndPos = m_pUndoLow->m_DstEnd;

		m_UndoChain.InsertTail(m_pUndoLow);
		m_pUndoLow = NULL;
	}

	if (NULL != m_pUndoHigh)
	{
		// UNDO only works from low to high
		m_pUndoHigh->m_DstEnd = m_pUndoHigh->m_DstStart;
		m_pUndoHigh->m_DstStart = m_pUndoHigh->m_DstPos;

		m_pUndoHigh->m_UndoStartPos = m_pUndoHigh->m_DstStart;
		m_pUndoHigh->m_UndoEndPos = m_pUndoHigh->m_DstEnd;

		m_pUndoHigh->m_SrcEnd = m_pUndoHigh->m_SrcStart;
		m_pUndoHigh->m_SrcStart = m_pUndoHigh->m_SrcPos;

		m_UndoChain.InsertTail(m_pUndoHigh);
		m_pUndoHigh = NULL;
	}

	return BaseClass::GetUndoChain();
}

void CReverseOperation::DeleteUndo()
{
	delete m_pUndoLow;
	m_pUndoLow = NULL;

	delete m_pUndoHigh;
	m_pUndoHigh = NULL;

	BaseClass::DeleteUndo();
}

BOOL CReverseOperation::OperationProc()
{
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}

	if (m_DstPos >= m_DstEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	// m_DstPos goes from begin up.
	// m_SrcPos goes from end down
	DWORD dwStartTime = GetTickCount();
	SAMPLE_POSITION dwOperationBeginTop = m_SrcPos;
	SAMPLE_POSITION dwOperationBeginBottom = m_DstPos;

	int const TempBufCount = 1024;
	ASSERT(m_DstFile.GetSampleType() == SampleType16bit);
	WAVE_SAMPLE BufBottom[TempBufCount];
	WAVE_SAMPLE BufTop[TempBufCount];

	int const DstSampleSize = m_DstFile.SampleSize();

	NUMBER_OF_CHANNELS const NumDstChannels = m_DstFile.Channels();

	int const TempBufSamples = TempBufCount / NumDstChannels;

	do
	{

		NUMBER_OF_SAMPLES CanReadSamples = (m_DstEnd - m_DstPos) / DstSampleSize;
		if (CanReadSamples > TempBufSamples)
		{
			CanReadSamples = TempBufSamples;
		}

		WAVE_SAMPLE * p1 = BufBottom;
		WAVE_SAMPLE * p2 = BufTop + TempBufCount;

		if (-CanReadSamples != m_DstFile.ReadSamples(ALL_CHANNELS,
													m_SrcPos, -CanReadSamples, p2, m_DstFile.GetSampleType())
			|| CanReadSamples != m_DstFile.ReadSamples(ALL_CHANNELS,
														m_DstPos, CanReadSamples, p1, m_DstFile.GetSampleType()))
		{
			// error
			TRACE("Reading samples was unsuccessful!\n");
			m_Flags |= OperationContextFinished;
			break;
		}

		long const DataSize = CanReadSamples * DstSampleSize;

		// save the old data to undo buffer
		if (NULL != m_pUndoLow)
		{
			m_pUndoLow->SaveUndoData(p1,
									DataSize, m_DstPos, NumDstChannels);
		}

		if (NULL != m_pUndoHigh)
		{
			m_pUndoHigh->SaveUndoData(p2,
									-DataSize, m_SrcPos, NumDstChannels);
		}


		for (NUMBER_OF_SAMPLES i = 0; i < CanReadSamples; i++)
		{
			p2 -= NumDstChannels;
			for (NUMBER_OF_CHANNELS ch = 0; ch < NumDstChannels; ch ++)
			{
				std::swap(p1[ch], p2[ch]);
				//WAVE_SAMPLE tmp = p1[ch];
				//p1[ch] = p2[ch];
				//p2[ch] = tmp;
			}
			p1 += NumDstChannels;
		}

		// save the data back
		if (-CanReadSamples != m_DstFile.WriteSamples(m_DstChan,
													m_SrcPos, -CanReadSamples, BufTop + TempBufCount,
													m_DstChan, NumDstChannels, m_DstFile.GetSampleType())
			|| CanReadSamples != m_DstFile.WriteSamples(m_DstChan,
														m_DstPos, CanReadSamples, BufBottom,
														m_DstChan, NumDstChannels, m_DstFile.GetSampleType()))
		{
			// error
			TRACE("Writing samples was unsuccessful!\n");
			m_Flags |= OperationContextFinished;
			break;
		}

		m_SrcPos -= CanReadSamples * DstSampleSize;
		m_DstPos += CanReadSamples * DstSampleSize;
	}
	while ((m_DstPos < m_DstEnd
				&& GetTickCount() - dwStartTime < 200)
			);

	// notify the view
	pDocument->FileChanged(m_DstFile, dwOperationBeginBottom, m_DstPos);
	pDocument->FileChanged(m_DstFile, m_SrcPos, dwOperationBeginTop);

	return TRUE;
}
//////////////////////// CMetadataChangeOperation

CMetadataChangeOperation::CMetadataChangeOperation(CWaveSoapFrontDoc * pDoc, CWaveFile & WaveFile,
													unsigned MetadataChangeFlags)
	: BaseClass(pDoc, OperationContextSynchronous)
	, m_pMetadata(NULL)
	, m_pUndoData(NULL)
	, m_WaveFile(WaveFile)
	, m_MetadataCopyFlags(MetadataChangeFlags)
{
}

CMetadataChangeOperation::~CMetadataChangeOperation()
{
	delete m_pMetadata;
	delete m_pUndoData;
}

BOOL CMetadataChangeOperation::CreateUndo()
{
	if (pDocument->WaveFileID() == m_WaveFile.GetFileID())
	{
		m_pUndoData = new ThisClass(pDocument, m_WaveFile);
	}

//    m_pUndoData->SaveUndoMetadata(m_MetadataCopyFlags);

	return TRUE;
}

BOOL CMetadataChangeOperation::OperationProc()
{
	if (m_MetadataCopyFlags & CWaveFile::InstanceDataWav::MetadataCopyAllCueData)
	{
		pDocument->UpdateAllMarkers();
	}

	m_WaveFile.SwapMetadata(m_pMetadata, m_MetadataCopyFlags);

	if (NULL != m_pUndoData)
	{
		m_pUndoData->m_pMetadata = m_pMetadata;
		m_pUndoData->m_MetadataCopyFlags = m_MetadataCopyFlags;
		m_pMetadata = NULL;
		m_UndoChain.InsertHead(m_pUndoData);
		m_pUndoData = NULL;
	}

	if (m_MetadataCopyFlags & CWaveFile::InstanceDataWav::MetadataCopyAllCueData)
	{
		pDocument->UpdateAllMarkers();
	}

	return TRUE;
}

void CMetadataChangeOperation::SaveUndoMetadata(unsigned ChangeFlags)
{
	if (NULL == m_pMetadata)
	{
		m_pMetadata = new CWaveFile::InstanceDataWav;
	}
	m_MetadataCopyFlags |= ChangeFlags;
	m_pMetadata->CopyMetadata(pDocument->m_WavFile.GetInstanceData(), ChangeFlags);
}

BOOL CMetadataChangeOperation::PrepareUndo()
{
	m_WaveFile = pDocument->m_WavFile;
	return TRUE;
}

void CMetadataChangeOperation::UnprepareUndo()
{
	m_WaveFile.Close();
}

/////////////  CCueEditOperation      //////////////////////////////////////////////

// move markers
CCueEditOperation::CCueEditOperation(CWaveSoapFrontDoc * pDoc, CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
									NUMBER_OF_SAMPLES LengthToReplace, NUMBER_OF_SAMPLES SamplesToInsert)
	: BaseClass(pDoc, DstFile, CWaveFile::InstanceDataWav::MetadataCopyAllCueData)
	, m_StartDstSample(StartDstSample)
	, m_LengthToReplace(LengthToReplace)
	, m_StartSrcSample(0)
	, m_SamplesToInsert(SamplesToInsert)
{
}

// copy markers
CCueEditOperation::CCueEditOperation(CWaveSoapFrontDoc * pDoc, CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
									NUMBER_OF_SAMPLES LengthToReplace,
									CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
									NUMBER_OF_SAMPLES SamplesToInsert)
	: BaseClass(pDoc, DstFile, CWaveFile::InstanceDataWav::MetadataCopyAllCueData)
	, m_StartDstSample(StartDstSample)
	, m_LengthToReplace(LengthToReplace)
	, m_StartSrcSample(StartSrcSample)
	, m_SamplesToInsert(SamplesToInsert)
{
	m_SrcFile = SrcFile;
}

CCueEditOperation::~CCueEditOperation()
{
}

BOOL CCueEditOperation::OperationProc()
{
	if (NULL == m_pMetadata)
	{
		m_pMetadata = new CWaveFile::InstanceDataWav;
	}

	m_pMetadata->CopyMetadata(m_WaveFile.GetInstanceData(), m_pMetadata->MetadataCopyAllCueData);

	BOOL HasChanged = m_pMetadata->MoveMarkers(m_StartDstSample, m_LengthToReplace, m_SamplesToInsert);

	if (m_SrcFile.IsOpen()
		&& m_pMetadata->CopyMarkers(m_SrcFile.GetInstanceData(), m_StartSrcSample,
									m_StartDstSample, m_SamplesToInsert))
	{
		HasChanged = TRUE;
	}

	if ( ! HasChanged)
	{
		return TRUE;
	}

	return BaseClass::OperationProc();
}

/////////////  CInsertSilenceContext      //////////////////////////////////////////////
BOOL CInsertSilenceContext::InitExpand(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX length,
										CHANNEL_MASK chan, BOOL NeedUndo)
{
	if (DstFile.NumberOfSamples() == StartSample)
	{
		chan = ALL_CHANNELS;
	}

	if ( ! InitExpandOperation(this, DstFile, StartSample,
								length, chan))
	{
		return FALSE;
	}

	AddContext(new CInitChannels(pDocument, DstFile, StartSample,
								StartSample + length, chan));

	if (NeedUndo
		&& ! CreateUndo())
	{
		return FALSE;
	}
	return TRUE;
}
/////////////   CWaveMixOperation  /////////////////////////////////////////////
CWaveMixOperation::CWaveMixOperation(class CWaveSoapFrontDoc * pDoc, ULONG Flags,
									UINT StatusStringId, UINT OperationNameId)
	: BaseClass(pDoc, Flags, StatusStringId, OperationNameId)
{
}

BOOL CWaveMixOperation::ProcessBuffer(void * buf, size_t BufferLength, SAMPLE_POSITION offset, BOOL /*bBackward*/)
{
	int const nChannels = m_DstFile.Channels();
	ASSERT(m_DstFile.GetSampleType() == SampleType16bit);

	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) buf;
	unsigned const SampleSize = m_DstFile.SampleSize();

	NUMBER_OF_SAMPLES nSamples = BufferLength / sizeof pDst[0];
	SAMPLE_INDEX SampleIndex = offset / SampleSize;

	ASSERT(0 == (offset % m_DstFile.SampleSize()));
	ASSERT(0 == (BufferLength % m_DstFile.SampleSize()));

	NUMBER_OF_SAMPLES i;
	bool UseSrc = (m_SrcStart != 0
					&& m_SrcEnd > m_SrcStart
					&& m_SrcFile.IsOpen());

	for (i = 0; i < nSamples; i += nChannels, SampleIndex++)
	{
		WAVE_SAMPLE tmp[MAX_NUMBER_OF_CHANNELS];

		if (UseSrc)
		{
			if (1 != m_SrcFile.ReadSamples(ALL_CHANNELS, offset + m_SrcStart, 1, tmp))
			{
				UseSrc = false;
			}
		}

		for (long ch = 0, mask = 1; ch < nChannels; ch++, mask <<= 1)
		{
			if (m_DstChan & mask)
			{
				if (UseSrc)
				{
					pDst[i + ch] = DoubleToShort(
												pDst[i + ch] * GetDstMixCoefficient(SampleIndex, ch)
												+ tmp[ch] * GetSrcMixCoefficient(SampleIndex, ch));
				}
				else
				{
					pDst[i + ch] = DoubleToShort(pDst[i + ch] * GetDstMixCoefficient(SampleIndex, ch));
				}
			}
		}
		offset += SampleSize;
	}

	return TRUE;
}

///////////// CFadeInOutOperation /////////////////////////////////////////////

CFadeInOutOperation::CFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType)
	: BaseClass(pDoc)
	, m_FadeCurveType(FadeCurveType)
{
}

// init cross fade
// When the function creates UNDO, it also calls SetSaveForUndo to initialize range to save.
// If UNDO should be created later, the UNDO range should be initialized by calling SetSaveForUndo otherwise.
CFadeInOutOperation::CFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
										CWaveFile & SrcFile, SAMPLE_INDEX SrcBegin, CHANNEL_MASK SrcChannel,
										CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
										NUMBER_OF_SAMPLES Length, BOOL UndoEnabled)
	: BaseClass(pDoc)
	, m_FadeCurveType(FadeCurveType)
{
	InitDestination(DstFile, DstBegin, DstBegin + Length, DstChannel, UndoEnabled);
	InitSource(SrcFile, SrcBegin, SrcBegin + Length, SrcChannel);
}

// init fade in/out
// When the function creates UNDO, it also calls SetSaveForUndo to initialize range to save.
// If UNDO should be created later, the UNDO range should be initialized by calling SetSaveForUndo otherwise.
CFadeInOutOperation::CFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
										CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
										NUMBER_OF_SAMPLES Length, BOOL UndoEnabled)
	: BaseClass(pDoc)
	, m_FadeCurveType(FadeCurveType)
{
	InitDestination(DstFile, DstBegin, DstBegin + Length, DstChannel, UndoEnabled);
}

double CFadeInOutOperation::GetSrcMixCoefficient(SAMPLE_INDEX Sample, int /*Channel*/) const
{
	double const Fraction = (Sample + 0.5) * m_DstFile.SampleSize() / (m_DstEnd - m_DstStart);

	switch (m_FadeCurveType)
	{
	case FadeOutLinear:
		return Fraction;
		break;
	case FadeInLinear:
		return 1. - Fraction;
		break;
	case FadeOutSinSquared:
		return 0.5 * (1. - cos(M_PI * Fraction));
		break;
	case FadeInSinSquared:
		return 0.5 * (1. + cos(M_PI * Fraction));
		break;
	case FadeOutCosine:
		return sin(M_PI * 0.5 * Fraction);
		break;
	case FadeInSine:
		return cos(M_PI * 0.5 * Fraction);
		break;
	}
	return 0.;
}

double CFadeInOutOperation::GetDstMixCoefficient(SAMPLE_INDEX Sample, int /*Channel*/) const
{
	double const Fraction = (Sample + 0.5) * m_DstFile.SampleSize() / (m_DstEnd - m_DstStart);

	switch (m_FadeCurveType)
	{
	case FadeInLinear:
		return Fraction;
		break;
	case FadeOutLinear:
		return 1. - Fraction;
		break;
	case FadeInSinSquared:
		return 0.5 * (1. - cos(M_PI * Fraction));
		break;
	case FadeOutSinSquared:
		return 0.5 * (1. + cos(M_PI * Fraction));
		break;
	case FadeInSine:
		return sin(M_PI * 0.5 * Fraction);
		break;
	case FadeOutCosine:
		return cos(M_PI * 0.5 * Fraction);
		break;
	}
	return 1.;
}

///////////////////////////////////////////////////////////////////////
BOOL InitExpandOperation(CStagedContext * pContext,
						CWaveFile & File, SAMPLE_INDEX StartSample,
						NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	NUMBER_OF_SAMPLES NumberOfSamples = File.NumberOfSamples();

	NUMBER_OF_SAMPLES NewSamples = NumberOfSamples + Length;

	// 1. expand the file
	pContext->AddContext(new
						CWaveSamplesChangeOperation(pContext->pDocument, File, NewSamples));

	// 2. if not all channels moved, zero the expanded part
	CHANNEL_MASK ChannelsToZero = File.ChannelsMask() & ~Channel;

	if (0 != ChannelsToZero)
	{
		// special zero context used, with empty undo
		pContext->AddContext(new CInitChannels(pContext->pDocument, File, NumberOfSamples,
												NewSamples, ChannelsToZero));
	}
	else
	{
		// 3. Move all markers and regions (TODO)
	}
	// 4. CMoveContext moves all wave data toward the end
	if (NumberOfSamples > StartSample)
	{
		CMoveOperation::auto_ptr pMove(new
										CMoveOperation(pContext->pDocument, IDS_STATUS_PROMPT_EXPANDING_FILE));
		pMove->InitMove(File, StartSample, StartSample + Length, NumberOfSamples - StartSample,
						Channel);
		pContext->AddContext(pMove.release());
	}
	return TRUE;
}

// StartSample - from where the removed part starts
// Length - how much to remove
BOOL InitShrinkOperation(CStagedContext * pContext,
						CWaveFile & File,
						SAMPLE_INDEX StartSample, NUMBER_OF_SAMPLES Length, CHANNEL_MASK Channel)
{
	NUMBER_OF_SAMPLES NumberOfSamples = File.NumberOfSamples();

	NUMBER_OF_SAMPLES NewSamples = NumberOfSamples - Length;

	// 1. Move all wave data
	if (NewSamples > StartSample)
	{
		CMoveOperation::auto_ptr pMove(new
										CMoveOperation(pContext->pDocument,
														IDS_STATUS_PROMPT_SHRINKING_FILE));

		pMove->InitMove(File, StartSample + Length, StartSample,
						NumberOfSamples - StartSample - Length,
						Channel);

		pContext->AddContext(pMove.release());
	}

	// 2. If partial channels moved: add special operation, to zero the area on undo

	if (NewSamples < StartSample + Length)
	{
		// because some data (not moved by CMoveOperation)
		// will be discarded by WaveSampleChange or by InitChannels,
		// we need to save it
		pContext->AddContext(new
							CSaveTrimmedOperation(pContext->pDocument, File,
												NewSamples, StartSample + Length, Channel));
	}

	if ( ! File.AllChannels(Channel))
	{
		// not all channels are moved
		// special zero context used, with empty undo
		pContext->AddContext(new CInitChannels(pContext->pDocument, File, NewSamples, NumberOfSamples,
												Channel));
	}
	else
	{
		// 3. If all channels moved: Move all markers and regions
		// TODO

		// 4. If all channels moved: Change number of samples
		pContext->AddContext(new
							CWaveSamplesChangeOperation(pContext->pDocument, File, NewSamples));
	}
	return TRUE;
}

void InitCopyMarkers(CStagedContext * pContext,
					CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace,
					CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
					NUMBER_OF_SAMPLES SamplesToInsert)
{
	pContext->AddContext(new CCueEditOperation(pContext->pDocument, DstFile, StartDstSample,
												LengthToReplace, SrcFile, StartSrcSample, SamplesToInsert));
}

void InitMoveMarkers(CStagedContext * pContext,
					CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace,
					NUMBER_OF_SAMPLES SamplesToInsert)
{
	pContext->AddContext(new CCueEditOperation(pContext->pDocument, DstFile, StartDstSample,
												LengthToReplace, SamplesToInsert));
}

BOOL InitInsertCopy(CStagedContext * pContext,
					CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
					NUMBER_OF_SAMPLES LengthToReplace, CHANNEL_MASK DstChannel,
					CWaveFile & SrcFile, SAMPLE_INDEX StartSrcSample,
					NUMBER_OF_SAMPLES SamplesToInsert, CHANNEL_MASK SrcChannel)
{
	if (LengthToReplace < SamplesToInsert)
	{
		// Expanding the file
		if ( ! InitExpandOperation(pContext, DstFile, StartDstSample + LengthToReplace,
									SamplesToInsert - LengthToReplace, DstChannel))
		{
			return FALSE;
		}

		// only copy markers if all channels are modified
		if (DstFile.AllChannels(DstChannel))
		{
			InitCopyMarkers(pContext, DstFile, StartDstSample, LengthToReplace,
							SrcFile, StartSrcSample, SamplesToInsert);
		}
		// the copied data outside of old data chunk length should be handled by
		// CRestoreTrimmedOperation
		NUMBER_OF_SAMPLES NumberOfSamples = DstFile.NumberOfSamples();
		if (StartDstSample + SamplesToInsert > NumberOfSamples)
		{
			CRestoreTrimmedOperation * pCopy = new CRestoreTrimmedOperation
												(pContext->pDocument);

			ASSERT(StartDstSample <= NumberOfSamples);
			TRACE(_T("Copying some samples with CRestoreTrimmedOperation:\n")
				_T("SRC: start=%s, end=%s, DST: start=%s, end=%s\n"),
				LPCTSTR(SampleToString(StartSrcSample + (NumberOfSamples - StartDstSample), 44100)),
				LPCTSTR(SampleToString(StartSrcSample + SamplesToInsert, 44100)),
				LPCTSTR(SampleToString(NumberOfSamples, 44100)),
				LPCTSTR(SampleToString(StartDstSample + SamplesToInsert, 44100)));

			pCopy->InitCopy(DstFile, NumberOfSamples, DstChannel, SrcFile,
							StartSrcSample + (NumberOfSamples - StartDstSample), SrcChannel,
							StartDstSample + SamplesToInsert - NumberOfSamples);

			pContext->AddContext(pCopy);

			SamplesToInsert = NumberOfSamples - StartDstSample;
		}
	}
	else if (LengthToReplace > SamplesToInsert)
	{
		// only copy markers if all channels are modified
		if (DstFile.AllChannels(DstChannel))
		{
			InitCopyMarkers(pContext, DstFile, StartDstSample, LengthToReplace,
							SrcFile, StartSrcSample, SamplesToInsert);
		}

		if ( ! InitShrinkOperation(pContext, DstFile, StartDstSample + SamplesToInsert,
									LengthToReplace - SamplesToInsert, DstChannel))
		{
			return FALSE;
		}
	}
	else if (DstFile.AllChannels(DstChannel))
	{
		// only copy markers if all channels are modified
		InitCopyMarkers(pContext, DstFile, StartDstSample, LengthToReplace,
						SrcFile, StartSrcSample, SamplesToInsert);
	}

	if (0 != SamplesToInsert)
	{
		// now copy data
		CCopyContext::auto_ptr pCopy(new CCopyContext(pContext->pDocument,
													IDS_STATUS_PROMPT_INSERTING_DATA));

		if ( ! pCopy->InitCopy(DstFile, StartDstSample, DstChannel,
								SrcFile, StartSrcSample, SrcChannel, SamplesToInsert))
		{
			return FALSE;
		}

		pCopy->SetSaveForUndo(StartDstSample, StartDstSample + LengthToReplace);

		pContext->AddContext(pCopy.release());
	}

	return TRUE;
}

