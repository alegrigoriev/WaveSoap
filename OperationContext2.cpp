// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// OperationContext2.cpp
#include "stdafx.h"
#define _USE_MATH_DEFINES   // for M_PI definition
#include <math.h>
#include "OperationContext2.h"
#include "OperationDialogs.h"
#include "WaveSoapFrontDoc.h"
#include "BladeMP3EncDLL.h"
#include "TimeToStr.h"
#include "resource.h"       // main symbols

#define TRACE_EXPRESSION 0

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

CExpressionEvaluationProc::CExpressionEvaluationProc()
	:m_CurrentChannel(0)
	, m_DataStackIndex(0)
	, m_DataTypeStackIndex(0)
	, m_ConstantBufferIndex(0)
{
	m_InputSampleType = SampleTypeFloat32;
}

BOOL CExpressionEvaluationProc::Init()
{
	m_SamplePeriod = 1. / m_nSamplingRate;
	m_dFileLengthTime = m_NumberOfFileSamples / double(m_nSamplingRate);
	m_dSelectionLengthTime = m_NumberOfSelectionSamples / double(m_nSamplingRate);

	m_nSelectionSampleArgument = 0;
	m_dSelectionTimeArgument = 0.;
	m_dFileTimeArgument = m_nFileSampleArgument / double(m_nSamplingRate);
	for (unsigned i = 0; i < countof(m_DitherState); i++)
	{
		for (unsigned j = 0; j < countof(m_DitherState[i]); j++)
		{
			m_DitherState[i][j] = 0.;
		}
	}
	return TRUE;
}

void CExpressionEvaluationProc::ProcessSoundSample(char const * pInSample, char * pOutSample, unsigned NumChannels)
{
	try
	{
		BaseClass::ProcessSoundSample(pInSample, pOutSample, NumChannels);
		m_nSelectionSampleArgument++;
		m_nFileSampleArgument++;
		m_dSelectionTimeArgument += m_SamplePeriod;
		m_dFileTimeArgument += m_SamplePeriod;
	}
	catch (wchar_t const * pError)
	{
		m_ErrorString = pError;
		//return FALSE;
	}
}

void CExpressionEvaluationProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel)
{
	m_CurrentChannel = channel;
	m_dCurrentSample = *(float const*)pInSample;
	Evaluate();
	*(float*)pOutSample = (float)* m_pResultAddress;
}

CString CExpressionEvaluationProc::GetToken(LPCTSTR * ppStr, TokenType * pType)
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
		_T("dither"), eDitherFunc,
		_T("pi"), ePiConstant,
		_T("n"), eSelectionSampleNumber,
		_T("N"), eAbsoluteSampleNumber,
		_T("t"), eSelectionTime,
		_T("dt"), eSelectionLengthTime,
		_T("T"), eAbsoluteTime,
		_T("DT"), eFileLengthTime,
		_T("F"), eSamplingRate,
		_T("dn"), eSelectionLengthSamples,
		_T("DN"), eFileLengthSamples,
		_T("f1"), eCurrentFrequencyArgument1,
		_T("f2"), eCurrentFrequencyArgument2,
		_T("f3"), eCurrentFrequencyArgument3,
		_T("f"), eCurrentFrequencyArgument,
		_T("wave"), eCurrentSampleValue,	// TODO: make it a function
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

CExpressionEvaluationProc::TokenType
CExpressionEvaluationProc::CompileParenthesedExpression(LPCTSTR * ppStr)
{
	TokenType type;
	//LPCTSTR prevStr = *ppStr;
	CString token = GetToken( ppStr, & type);
	if (type != eLeftParenthesis
		|| eRightParenthesis != CompileExpression(ppStr))
	{
		throw L"Right parenthesis expected";
	}
	return eRightParenthesis;
}

void CExpressionEvaluationProc::CompileFunctionOfDouble(void (ThisClass::* Function)(Operation * t), LPCTSTR * ppStr)
{
	CompileParenthesedExpression( ppStr);
	// can't put those right to the function call, because
	// order of evaluation would be unpredicted
	double * pArg = PopDouble();
	double * pDst = PushDouble();

	AddOperation(Function, pDst, pArg, NULL);
}

CExpressionEvaluationProc::TokenType
	CExpressionEvaluationProc::CompileTerm(LPCTSTR * ppStr)
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
		throw L"Syntax error";
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
			AddOperation(&ThisClass::NegateInt, pDst, pSrc, NULL);
		}
		else if (eDoubleConstant == type)
		{
			PushConstant( - *PopDouble());
		}
		else if (eDoubleExpression == type)
		{
			double * pSrc = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::NegateDouble, pDst, pSrc, NULL);
		}
		else
		{
			throw L"Internal Error";
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
			AddOperation(&ThisClass::ComplementInt, pDst, pSrc, NULL);
		}
		else
		{
			throw L"Binary complement operation arguments must be integer, use int() function";
		}
		break;

	case eLeftParenthesis:
		*ppStr = prevStr;
		return CompileParenthesedExpression( ppStr);
		break;

	case eSinusFunc:
		CompileFunctionOfDouble(&ThisClass::Sin, ppStr);
		break;

	case eCosinusFunc:
		CompileFunctionOfDouble(&ThisClass::Cos, ppStr);
		break;

	case eTangensFunc:
		CompileFunctionOfDouble(&ThisClass::Tan, ppStr);
		break;

	case eSinusHFunc:
		CompileFunctionOfDouble(&ThisClass::SinH, ppStr);
		break;

	case eCosinusHFunc:
		CompileFunctionOfDouble(&ThisClass::CosH, ppStr);
		break;

	case eTangensHFunc:
		CompileFunctionOfDouble(&ThisClass::TanH, ppStr);
		break;

	case eExpFunc:
		CompileFunctionOfDouble(&ThisClass::Exp, ppStr);
		break;

	case eExp10Func:
		CompileFunctionOfDouble(&ThisClass::Exp10, ppStr);
		break;

	case eLogFunc:
		CompileFunctionOfDouble(&ThisClass::Log, ppStr);
		break;

	case eLog10Func:
		CompileFunctionOfDouble(&ThisClass::Log10, ppStr);
		break;

	case eSqrtFunc:
		CompileFunctionOfDouble(&ThisClass::Sqrt, ppStr);
		break;

	case eAbsFunc:
		CompileFunctionOfDouble(&ThisClass::Abs, ppStr);
		break;

	case eInt:
	{
		CompileParenthesedExpression( ppStr);
		// can't put those right to the function call, because
		// order of evaluation would be unpredicted
		TokenType top_type = GetTopOfStackType();
		if (top_type != eIntConstant
			&& top_type != eIntExpression
			&& top_type != eIntVariable)
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
		TokenType top_type = GetTopOfStackType();
		if (top_type != eDoubleConstant
			&& top_type != eDoubleExpression
			&& top_type != eDoubleVariable)
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
				throw L"Invalud argument of \"noise\" function";
			}
		}
		else
		{
			*ppStr = prevStr;
		}
		{
			double * pDst = PushDouble();
			AddOperation(&ThisClass::Noise, pDst, NULL, NULL);
		}
		break;

	case eDitherFunc:
		CompileFunctionOfDouble(&ThisClass::Dither, ppStr);
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
		PushConstant(M_PI);
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
	case eFileLengthTime:
		PushVariable( & m_dFileLengthTime);
		break;
	case eSelectionLengthSamples:
		PushVariable(&m_NumberOfSelectionSamples);
		break;
	case eFileLengthSamples:
		PushVariable(&m_NumberOfFileSamples);
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
		throw L"Unrecognized syntax";
		break;
	}
	return type;
}

void CExpressionEvaluationProc::CompileAnd()
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
			AddOperation(&ThisClass::AndInt, pDst, pSrc1, pSrc2);
			return;
		}
	}
	throw L"Bitwise ""and"" operation arguments must be integer, use int() function";
}

void CExpressionEvaluationProc::CompileOr()
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
			AddOperation(&ThisClass::OrInt, pDst, pSrc1, pSrc2);
			return;
		}
	}
	throw L"Bitwise ""or"" operation arguments must be integer, use int() function";
}

void CExpressionEvaluationProc::CompileXor()
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
			AddOperation(&ThisClass::XorInt, pDst, pSrc1, pSrc2);
			return;
		}
	}
	throw L"Bitwise ""xor"" operation arguments must be integer, use int() function";
}

void CExpressionEvaluationProc::CompileAdd()
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
			AddOperation(&ThisClass::AddInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::AddDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
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
			AddOperation(&ThisClass::AddDoubleInt, pDst, pSrc2, pSrc1);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::AddDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
		}
	}
	else
	{
		throw L"Internal Error";
	}
}

void CExpressionEvaluationProc::CompileSubtract()
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
			AddOperation(&ThisClass::SubtractInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::SubtractDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
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
			AddOperation(&ThisClass::SubtractIntDouble, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::SubtractDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
		}
	}
	else
	{
		throw L"Internal Error";
	}
}

void CExpressionEvaluationProc::DivideDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw L"Divide by zero";
	}
	*t->dDst = *t->dSrc1 / *t->dSrc2;
}

void CExpressionEvaluationProc::DivideInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw L"Divide by zero";
	}
	*t->nDst = *t->nSrc1 / *t->nSrc2;
}

void CExpressionEvaluationProc::DivideDoubleInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw L"Divide by zero";
	}
	*t->dDst = *t->dSrc1 / *t->nSrc2;
}

void CExpressionEvaluationProc::DivideIntDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw L"Divide by zero";
	}
	*t->dDst = *t->nSrc1 / *t->dSrc2;
}

void CExpressionEvaluationProc::ModuloDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw L"Divide by zero";
	}
	*t->dDst = fmod(*t->dSrc1, *t->dSrc2);
}

void CExpressionEvaluationProc::ModuloInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw L"Divide by zero";
	}
	*t->nDst = *t->nSrc1 % *t->nSrc2;
}

void CExpressionEvaluationProc::ModuloDoubleInt(Operation *t)
{
	if (0 == *t->nSrc2)
	{
		throw L"Divide by zero";
	}
	*t->dDst = fmod(*t->dSrc1, *t->nSrc2);
}

void CExpressionEvaluationProc::ModuloIntDouble(Operation *t)
{
	if (0 == *t->dSrc2)
	{
		throw L"Divide by zero";
	}
	*t->dDst = fmod(*t->nSrc1, *t->dSrc2);
}

void CExpressionEvaluationProc::Log(Operation *t)
{
	if (*t->dSrc1 <= 0)
	{
		throw L"Logarithm argument <= 0";
	}
	*t->dDst = log(*t->dSrc1);
}

void CExpressionEvaluationProc::Log10(Operation *t)
{
	if (*t->dSrc1 <= 0)
	{
		throw L"Logarithm argument <= 0";
	}
	*t->dDst = log(*t->dSrc1) * 0.434294481903251827651;
}

void CExpressionEvaluationProc::Sqrt(Operation *t)
{
	if (*t->dSrc1 < 0)
	{
		throw L"Square root argument < 0";
	}
	*t->dDst = sqrt(*t->dSrc1);
}

void CExpressionEvaluationProc::Noise(Operation *t)
{
	C_ASSERT(RAND_MAX == 0x7FFF);
	//long r = (rand() ^ (rand() << 9)) - 0x800000;    // 24 bits
	*t->dDst = ((rand() ^ (rand() << 9)) - 0x800000) / double(0x800000);
}

void CExpressionEvaluationProc::Dither(Operation *t)
{
	// The double argument is a number of LSB units of amplitude of noise to the dither filter
	C_ASSERT(RAND_MAX == 0x7FFF);
	//
	static double FilterCoeffs[] =
	{
		// triangular filter
		7./32,
		-5./32,
		3./32,
		-1./32,
	};
	static bool coeffs_calculated = false;
	if (!coeffs_calculated)
	{
		for (unsigned i = 0; i < countof(FilterCoeffs); i++)
		{
		}
		coeffs_calculated = true;
	}
	double * DitherState = m_DitherState[m_CurrentChannel];
	memmove(DitherState + 1, DitherState,
			sizeof DitherState[0] * (countof(FilterCoeffs)*2-1));
	DitherState[0] = *t->dSrc1 * (rand() - 0x4000) / (0x4000LL * 0x8000);

	double result = 0.;
	for (unsigned i = 0; i < countof(FilterCoeffs); i++)
	{
		result += FilterCoeffs[i] * (DitherState[i + countof(FilterCoeffs)] - DitherState[countof(FilterCoeffs) - 1 - i]);
	}
	result -= 0.95*DitherState[countof(FilterCoeffs)*2];
	DitherState[countof(FilterCoeffs) * 2] = result;
	*t->dDst = result * 0.05;
}

void CExpressionEvaluationProc::CompileMultiply()
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
			AddOperation(&ThisClass::MultiplyInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::MultiplyDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
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
			AddOperation(&ThisClass::MultiplyDoubleInt, pDst, pSrc2, pSrc1);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::MultiplyDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
		}
	}
	else
	{
		throw L"Internal Error";
	}
}

void CExpressionEvaluationProc::CompileModulo()
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
			AddOperation(&ThisClass::ModuloInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::ModuloDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
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
			AddOperation(&ThisClass::ModuloIntDouble, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::ModuloDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
		}
	}
	else
	{
		throw L"Internal Error";
	}
}

void CExpressionEvaluationProc::CompileDivide()
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
			AddOperation(&ThisClass::DivideInt, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::DivideDoubleInt, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
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
			AddOperation(&ThisClass::DivideIntDouble, pDst, pSrc1, pSrc2);
		}
		else if (eDoubleConstant == type
				|| eDoubleExpression == type)
		{
			double * pSrc1 = PopDouble();
			double * pDst = PushDouble();
			AddOperation(&ThisClass::DivideDouble, pDst, pSrc1, pSrc2);
		}
		else
		{
			throw L"Internal Error";
		}
	}
	else
	{
		throw L"Internal Error";
	}
}

CExpressionEvaluationProc::TokenType
	CExpressionEvaluationProc::CompileExpression(LPCTSTR * ppStr)
{
	TokenType type, type1;
	//void (* Function)(Operation * t);
	type = CompileTerm(ppStr);
	if (eEndOfExpression == type)
	{
		throw L"Expression syntax error";
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
				throw L"Right operand missing";
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
			throw L"Unrecognized syntax";
			break;
		}
	}
	return eEndOfExpression;
}

void CExpressionEvaluationProc::AddOperation(void (ThisClass::* Function)(Operation * t),
											void * pDst, void * pSrc1, void * pSrc2)
{
	Operation t;
	t.Function = Function;
	t.pSrc1 = pSrc1;
	t.pSrc2 = pSrc2;
	t.pDst = pDst;
	m_OperationArray.push_back(t);
}

CExpressionEvaluationProc::TokenType CExpressionEvaluationProc::GetTopOfStackType()
{
	if (m_DataTypeStackIndex <= 0)
	{
		throw L"Expression Evaluation Stack Underflow";
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
	throw L"Internal Error";
}

void CExpressionEvaluationProc::PushConstant(int data)
{
	if (m_ConstantBufferIndex >= NumberOfIntConstants)
	{
		throw L"Expression too complex: too many constants";
	}
	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntConstant;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex].pInt = & m_ConstantBuffer[m_ConstantBufferIndex].Int;
	m_DataStackIndex++;

	m_ConstantBuffer[m_ConstantBufferIndex].Int = data;
	m_ConstantBufferIndex++;

	if (TRACE_EXPRESSION) TRACE("Push int constant %d, constant index = %d, type index = %d\n", data,
								m_ConstantBufferIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationProc::PushConstant(double data)
{
	if (m_ConstantBufferIndex >= NumberOfIntConstants)
	{
		throw L"Expression too complex: too many constants";
	}

	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eDoubleConstant;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex].pDouble = &m_ConstantBuffer[m_ConstantBufferIndex].Double;
	m_DataStackIndex++;

	m_ConstantBuffer[m_ConstantBufferIndex].Double = data;
	m_ConstantBufferIndex++;

	if (TRACE_EXPRESSION) TRACE("Push Double constant %f, constant index = %d, type index = %d\n", data,
								m_ConstantBufferIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationProc::PushVariable(int * pData)
{
	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntVariable;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex].pInt = pData;
	m_DataStackIndex++;

	if (TRACE_EXPRESSION) TRACE("Push int variable, data index = %d, type index = %d\n",
								m_DataStackIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationProc::PushVariable(long * pData)
{
	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntVariable;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex].pInt = (int*)pData;
	m_DataStackIndex++;

	if (TRACE_EXPRESSION) TRACE("Push long (same as int) variable, data index = %d, type index = %d\n",
								m_DataStackIndex, m_DataTypeStackIndex);
}

void CExpressionEvaluationProc::PushVariable(double * pData)
{
	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eDoubleVariable;
	m_DataTypeStackIndex++;

	m_DataStack[m_DataStackIndex].pDouble = pData;
	m_DataStackIndex++;

	if (TRACE_EXPRESSION) TRACE("Push Double variable, data index = %d, type index = %d\n",
								m_DataStackIndex, m_DataTypeStackIndex);
}

int * CExpressionEvaluationProc::PushInt()
{
	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eIntExpression;
	m_DataTypeStackIndex++;
	m_DataStackIndex++;

	if (TRACE_EXPRESSION) TRACE("Push int, data index = %d, type index = %d\n",
								m_DataStackIndex, m_DataTypeStackIndex);
	return & m_DataStack[m_DataStackIndex - 1].Int;
}

double * CExpressionEvaluationProc::PushDouble()
{
	if (m_DataStackIndex >= DataStackSize
		|| m_DataTypeStackIndex >= ExpressionStackSize)
	{
		throw L"Expression Evaluation Stack Overflow";
	}
	m_DataTypeStack[m_DataTypeStackIndex] = eDoubleExpression;
	m_DataTypeStackIndex++;

	m_DataStackIndex ++;
	if (TRACE_EXPRESSION) TRACE("Push Double, data index = %d, type index = %d\n",
								m_DataStackIndex, m_DataTypeStackIndex);
	return &m_DataStack[m_DataStackIndex - 1].Double;
}

int * CExpressionEvaluationProc::PopInt()
{
	if (m_DataTypeStackIndex <= 0)
	{
		throw L"Expression Evaluation Stack Underflow";
	}
	TokenType type = m_DataTypeStack[m_DataTypeStackIndex - 1];
	if (eIntConstant == type
		|| eIntVariable == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex--;
		if (TRACE_EXPRESSION) TRACE("Pop int constant, data index = %d, type index = %d\n",
									m_DataStackIndex, m_DataTypeStackIndex);
		return m_DataStack[m_DataStackIndex].pInt;
	}
	else if (eIntExpression == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex--;
		if (TRACE_EXPRESSION) TRACE("Pop int expression, data index = %d, type index = %d\n",
									m_DataStackIndex, m_DataTypeStackIndex);
		return & m_DataStack[m_DataStackIndex].Int;
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
		AddOperation(&ThisClass::DoubleToInt, pDst, pSrc, NULL);
		return PopInt();
	}
	throw L"Internal Error";
}

double * CExpressionEvaluationProc::PopDouble()
{
	if (m_DataTypeStackIndex <= 0)
	{
		throw L"Expression Evaluation Stack Underflow";
	}
	TokenType type = m_DataTypeStack[m_DataTypeStackIndex - 1];
	if (eDoubleConstant == type
		|| eDoubleVariable == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex--;
		if (TRACE_EXPRESSION) TRACE("Pop double constant, data index = %d, type index = %d\n",
									m_DataStackIndex, m_DataTypeStackIndex);
		return m_DataStack[m_DataStackIndex].pDouble;
	}
	else if (eDoubleExpression == type)
	{
		m_DataTypeStackIndex--;
		m_DataStackIndex --;
		if (TRACE_EXPRESSION) TRACE("Pop double expression, data index = %d, type index = %d\n",
									m_DataStackIndex, m_DataTypeStackIndex);
		return & m_DataStack[m_DataStackIndex].Double;
	}
	else if (eIntConstant == type
			|| eIntVariable == type
			|| eIntExpression == type)
	{
		// convert to double
		int * pSrc = PopInt();
		double * pDst = PushDouble();
		AddOperation(&ThisClass::IntToDouble, pDst, pSrc, NULL);
		return PopDouble();
	}
	throw L"Internal Error";
}

BOOL CExpressionEvaluationProc::SetExpression(LPCTSTR * ppszExpression)
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
			throw L"Expression syntax error";
		}
		// norm the sample
//        PushConstant(32767);
//        CompileMultiply();
		m_pResultAddress = PopDouble();
	}
	catch (wchar_t const * Error)
	{
		m_ErrorString = Error;
		return FALSE;
	}
	return TRUE;
}

void CExpressionEvaluationProc::Evaluate()
{
	for (unsigned i = 0; i < m_OperationArray.size(); i++)
	{
		(this->*(m_OperationArray[i].Function))( & m_OperationArray[i]);
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
		&& m_pDocument->PostCommitFileSave(m_FileSaveFlags, m_TargetName))
	{
		if (m_pDocument->m_bClosePending)
		{
			m_pDocument->m_bCloseThisDocumentNow = true;
		}
	}
	else
	{
		m_pDocument->m_bClosePending = false;
	}
	BaseClass::PostRetire();
}

CEqualizerContext::CEqualizerContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									double const BandCoefficients[MaxNumberOfEqualizerBands][6],
									int NumberOfBands, BOOL ZeroPhase)
	: BaseClass(pDoc, StatusStringId, OperationNameId)
	, m_Proc(BandCoefficients, NumberOfBands, ZeroPhase)
{
	if (ZeroPhase)
	{
		m_NumberOfBackwardPasses = 1;
	}
	AddWaveProc( & m_Proc);
	m_ProcBatch.m_bAutoDeleteProcs = false;
}

CEqualizerContext::~CEqualizerContext()
{
}

CEqualizerContext::EqualizerProc::EqualizerProc(double const BandCoefficients[MaxNumberOfEqualizerBands][6], int NumberOfBands, BOOL ZeroPhase)
	: m_bZeroPhase(ZeroPhase)
	, m_NumOfBands(NumberOfBands)
{
	m_InputSampleType = SampleTypeFloat32;

	for (int i = 0; i < NumberOfBands; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			m_BandCoefficients[i][j] = BandCoefficients[i][j];
		}
	}
}

BOOL CEqualizerContext::EqualizerProc::Init()
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

double CEqualizerContext::EqualizerProc::CalculateResult(int ch, float Input)
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

void CEqualizerContext::EqualizerProc::ProcessSampleValue(void const * pInSample, void * pOutSample, unsigned channel)
{
	*(float*)pOutSample = (float)CalculateResult(channel, *(float const*)pInSample);
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
	m_CdBufferSize(0),
	m_Drive(NULL)
{
}

BOOL CCdReadingContext::InitTrackInformation(class ICdDrive const * Drive,
											CdTrackInfo * pTrack,
											DWORD TargetFileType,
											WAVEFORMATEX const * pTargetFormat)
{
	CWaveFormat wfx;
	wfx.InitCdAudioFormat();

	m_Drive = Drive->Clone();
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

	InitDestination(WaveFile, 0, nSamples, 3, FALSE);

	m_Drive->DisableMediaChangeDetection();
	m_Drive->LockDoor();
	return TRUE;
}

unsigned CCdReadingContext::ProcessBuffer(char const * /*pInBuf*/, // if BACKWARD pass, points to the end of buffer
										char * pOutBuf,    // if BACKWARD pass, points to the end of buffer
										unsigned /*nInBytes*/, unsigned nOutBytes, unsigned * /*pUsedBytes*/,
										SAMPLE_POSITION /*SrcOffset*/,  // if BACKWARD pass, offset of the end of source buffer
										SAMPLE_POSITION /*DstOffset*/,  // if BACKWARD pass, offset of the end of destination buffer
										signed /*pass*/)
{
	unsigned OutBufFilled = 0;
	while (nOutBytes != 0)
	{
		if (0 == m_CdBufferFilled)
		{
			unsigned SectorsToRead = m_CdBufferSize / CDDASectorSize;
			if (SectorsToRead > m_NumberOfSectors)
			{
				SectorsToRead = m_NumberOfSectors;
			}
			if (0 == SectorsToRead)
			{
				break;
			}

#if CD_READ_PACE_ENABLE
			DWORD ReadBeginTime = timeGetTime();
#endif
			if ( ! m_Drive->ReadCdData(m_pCdBuffer, m_CdAddress, SectorsToRead))
			{
				break;     // TODO: Set an error string
			}

#if CD_READ_PACE_ENABLE
			DWORD ElapsedReadTime = timeGetTime() - ReadBeginTime;
			// pace the reading process
			DWORD delay;
			if (0 && ElapsedReadTime < 500)
			{
				timeSetEvent(delay, 2, LPTIMECALLBACK(m_hEvent), NULL, TIME_CALLBACK_EVENT_SET);
				WaitForSingleObject(m_hEvent, INFINITE);
			}
#endif
			m_NumberOfSectors -= SectorsToRead;
			m_CdAddress = LONG(m_CdAddress) + SectorsToRead;
			m_CdDataOffset = 0;
			m_CdBufferFilled = SectorsToRead * CDDASectorSize;
		}

		unsigned ToCopy = m_CdBufferFilled;
		if (ToCopy > nOutBytes)
		{
			ToCopy = nOutBytes;
		}
		memcpy(pOutBuf, m_CdDataOffset + (char*)m_pCdBuffer, ToCopy);

		nOutBytes -= ToCopy;
		m_CdBufferFilled -= ToCopy;

		pOutBuf += ToCopy;
		OutBufFilled += ToCopy;
		m_CdDataOffset += ToCopy;
	}
	return OutBufFilled;
}

BOOL CCdReadingContext::Init()
{
	if (!BaseClass::Init())
	{
		return FALSE;
	}
	// allocate buffer. Round to sector size multiple
	m_Drive->SetDriveBusy();
	m_CdBufferFilled = 0;
	m_CdDataOffset = 0;
	m_CdBufferSize = 0x10000 - 0x10000 % CDDASectorSize;

#if CD_READ_PACE_ENABLE
	timeBeginPeriod(2);
#endif
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pCdBuffer = VirtualAlloc(NULL, m_CdBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (NULL == m_pCdBuffer)
	{
		return FALSE;
	}

	m_Drive->SetReadSpeed(m_RequiredReadSpeed, m_CdAddress - 150, m_NumberOfSectors);
	m_Drive->ReadCdData(m_pCdBuffer, m_CdAddress, m_CdBufferSize / CDDASectorSize);
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

#if CD_READ_PACE_ENABLE
	timeEndPeriod(2);
#endif

	if (0 == (m_Flags & OperationContextFinished)
		|| NULL == m_pNextTrackContext)
	{
		TRACE("CD drive speed reset to original %d\n", m_OriginalReadSpeed);
		m_Drive->SetReadSpeed(m_OriginalReadSpeed);
		// stop drive:
		m_Drive->StopDrive();
	}
	m_Drive->SetDriveBusy(false);
	BaseClass::DeInit();
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

	m_pDocument = (CWaveSoapFrontDoc *)pTemplate->OpenDocumentFile(
					(LPCTSTR) & Params,
					OpenDocumentCreateNewWithParameters|1);
	if (NULL == m_pDocument)
	{
		delete this;
		return;
	}

	m_pDocument->SetModifiedFlag();
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
		ASSERT(m_DstPos - m_DstStart < 0xFFFFFFFFULL - 44);
		if (m_DstPos == m_DstStart)
		{
			m_pDocument->m_bCloseThisDocumentNow = true;
		}
		else if (m_DstFile.SetDatachunkLength(WAV_FILE_SIZE(m_DstPos - m_DstStart)))
		{
			m_pDocument->SoundChanged(m_DstFile.GetFileID(), 0, 0, m_DstFile.NumberOfSamples());
		}
	}
	else
	{
		if (m_bSaveImmediately)
		{
			m_SrcFile.Close();
			m_DstFile.Close();
			m_pDocument->m_bClosing = true;
			m_pDocument->DoFileSave();
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
	delete m_Drive;
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
		new CReplaceFileContext(m_pDocument, m_OperationName, m_pDocument->m_WavFile,
								m_pDocument->m_bDirectMode);
	if (NULL != pUndo)
	{
		m_UndoChain.InsertHead(pUndo);
		return TRUE;
	}
	return FALSE;
}

bool CReplaceFileContext::KeepsPermanentFileReference() const
{
	return m_File.RefersPermanentFile();
}

BOOL CReplaceFileContext::OperationProc()
{
	m_pDocument->m_WavFile = m_File;

	NUMBER_OF_SAMPLES nSamples = m_pDocument->WaveFileSamples();

	m_pDocument->SoundChanged(m_pDocument->WaveFileID(),
							0, nSamples, nSamples, UpdateSoundDontRescanPeaks);
	// save/restore "Direct" flag and update title
	m_pDocument->m_bDirectMode = m_bNewDirectMode;

	m_pDocument->UpdateFrameTitles();        // will cause name change in views
	m_pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateWholeFileChanged);

	return TRUE;
}

///////////// CReplaceFormatContext //////////////////
CReplaceFormatContext::CReplaceFormatContext(CWaveSoapFrontDoc * pDoc, LPCTSTR OperationName,
											CWaveFormat const & NewFormat)
	: BaseClass(pDoc, OperationContextSynchronous, OperationName, OperationName),
	m_NewWaveFormat(NewFormat)
{
}

CReplaceFormatContext::CReplaceFormatContext(CWaveSoapFrontDoc * pDoc, UINT OperationNameId,
											CWaveFormat const & NewFormat)
	: BaseClass(pDoc, OperationContextSynchronous, OperationNameId, OperationNameId),
	m_NewWaveFormat(NewFormat)
{
}

BOOL CReplaceFormatContext::CreateUndo()
{
	CReplaceFormatContext * pUndo =
		new CReplaceFormatContext(m_pDocument, m_OperationName, m_pDocument->WaveFormat());

	m_UndoChain.InsertHead(pUndo);
	return TRUE;
}

BOOL CReplaceFormatContext::OperationProc()
{
	// replace format to the one in m_NewWaveFormat
	m_pDocument->SetWaveFormat(m_NewWaveFormat);

	m_pDocument->UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateSampleRateChanged);
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
		|| m_File.GetFileID() != m_pDocument->WaveFileID())
	{
		return TRUE;
	}

	CLengthChangeOperation * pUndo = new CLengthChangeOperation(m_pDocument,
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
	m_File = m_pDocument->m_WavFile;
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
		|| m_File.GetFileID() != m_pDocument->WaveFileID())
	{
		return TRUE;
	}

	CWaveSamplesChangeOperation * pUndo = new CWaveSamplesChangeOperation(m_pDocument,
											m_File, m_File.NumberOfSamples());

	m_UndoChain.InsertHead(pUndo);
	return TRUE;
}

BOOL CWaveSamplesChangeOperation::PrepareUndo()
{
	m_File = m_pDocument->m_WavFile;
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

	m_pDocument->SoundChanged(m_File.GetFileID(), UpdateBegin, UpdateEnd, m_NewSamples, Flags);

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
		|| m_DstFile.GetFileID() != m_pDocument->WaveFileID()
		|| NULL != m_pUndoMove)
	{
		return TRUE;
	}

	CMoveOperation * pUndo = new CMoveOperation(m_pDocument);

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
	m_SrcFile = m_pDocument->m_WavFile;
	m_DstFile = m_pDocument->m_WavFile;
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
	UCHAR tmp[4*MAX_NUMBER_OF_CHANNELS];
	int const SrcSampleSize = m_SrcFile.SampleSize();
	int const DstSampleSize = m_DstFile.SampleSize();
	WaveSampleType const SrcSampleType = m_SrcFile.GetSampleType();
	WaveSampleType const DstSampleType = m_DstFile.GetSampleType();
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
											-signed(Samples) * DstSampleSize, m_DstPos);
			}
			// CopyWaveSamples doesn't work backwards
			for (unsigned i = 0; i < Samples; )
			{
				unsigned SamplesToCopy = Samples - i;
				// see if the buffers overlap
				if (pSrcBuf - SrcSampleSize*SamplesToCopy < pDstBuf - DstSampleSize*SamplesToCopy
					&& pDstBuf - DstSampleSize*SamplesToCopy < pSrcBuf)
				{
					// there is overlap, copy by parts
					SamplesToCopy = unsigned((pDstBuf - pSrcBuf + (SrcSampleSize - DstSampleSize) * SamplesToCopy) / SrcSampleSize);
					if (SamplesToCopy == 0)
					{
						SamplesToCopy = 1;
					}
					ASSERT(SamplesToCopy <= Samples - i);
				}
				pDstBuf -= DstSampleSize * SamplesToCopy;
				pSrcBuf -= SrcSampleSize * SamplesToCopy;

				CopyWaveSamples(pDstBuf, m_DstChan, NumDstChannels,
								pSrcBuf, m_SrcChan, NumSrcChannels,
								SamplesToCopy, DstSampleType, SrcSampleType);

				i += SamplesToCopy;
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
									m_DstPos, -1, tmp + DstSampleSize,
									m_DstFile.GetSampleType());

				m_pUndoContext->SaveUndoData(tmp + DstSampleSize,
											-DstSampleSize, m_DstPos);
			}
			// read one sample directly
			if (1 != m_SrcFile.ReadSamples(ALL_CHANNELS,
											m_SrcPos - SrcSampleSize, 1, tmp, SrcSampleType)
				|| 1 != m_DstFile.WriteSamples(m_DstChan, m_DstPos - DstSampleSize, 1,
												tmp, m_SrcChan, NumSrcChannels, SrcSampleType))
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
				&& GetTickCount() - dwStartTime < 1000)
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	// notify the view
	m_pDocument->FileChanged(m_DstFile, dwOperationBegin, m_DstPos);

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
		|| m_SrcFile.GetFileID() != m_pDocument->WaveFileID()
		|| m_UndoStartPos == m_UndoEndPos)
	{
		return TRUE;
	}

	CRestoreTrimmedOperation::auto_ptr
	pUndo(new CRestoreTrimmedOperation(m_pDocument));

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
	m_SrcFile = m_pDocument->m_WavFile;
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
		|| m_DstFile.GetFileID() != m_pDocument->WaveFileID()
		|| m_DstStart == m_DstEnd)
	{
		return TRUE;
	}

	// a reference on the main document file will be closed in UnprepareUndo
	m_pSaveOperation = new CSaveTrimmedOperation(m_pDocument,
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
	m_DstFile = m_pDocument->m_WavFile;
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
		|| m_DstFile.GetFileID() != m_pDocument->WaveFileID())
	{
		return TRUE;
	}

	m_UndoChain.InsertHead(new CInitChannelsUndo(m_pDocument,
												m_DstStart, m_DstEnd, m_DstChan));
	return TRUE;
}

unsigned CInitChannels::ProcessBuffer(char const * /*pInBuf*/, // if BACKWARD pass, points to the end of buffer
									char * pOutBuf,    // if BACKWARD pass, points to the end of buffer
									unsigned /*nInBytes*/, unsigned nOutBytes, unsigned * /*pUsedBytes*/,
									SAMPLE_POSITION /*SrcOffset*/,  // if BACKWARD pass, offset of the end of source buffer
									SAMPLE_POSITION /*DstOffset*/,  // if BACKWARD pass, offset of the end of destination buffer
									signed
#ifdef _DEBUG
									pass
#endif
									)
{
	ASSERT(pass == 1);
	memset(pOutBuf, 0, nOutBytes);
	return nOutBytes;
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
	m_UndoChain.InsertHead(new CInitChannels(m_pDocument, m_pDocument->m_WavFile,
											m_pDocument->m_WavFile.PositionToSample(m_SrcStart),
											m_pDocument->m_WavFile.PositionToSample(m_SrcEnd), m_SrcChan));

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
	if (m_UndoChain.IsEmpty())
	{
		m_UndoChain.InsertHead(new CSelectionChangeOperation(m_pDocument,
															m_pDocument->m_SelectionStart, m_pDocument->m_SelectionEnd,
															m_pDocument->m_CaretPosition, m_pDocument->m_SelectedChannel));
	}
	return TRUE;
}

BOOL CSelectionChangeOperation::OperationProc()
{
	m_pDocument->SetSelection(m_Start, m_End, m_Channels, m_Caret);
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

BOOL CReverseOperation::InitInPlaceProcessing(CWaveFile & DstFile, SAMPLE_INDEX StartSample, SAMPLE_INDEX EndSample,
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
		|| m_DstFile.GetFileID() != m_pDocument->WaveFileID())
	{
		return TRUE;
	}

	CCopyUndoContext::auto_ptr pUndo1(new CCopyUndoContext(m_pDocument));
	CCopyUndoContext::auto_ptr pUndo2(new CCopyUndoContext(m_pDocument));

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

	int const TempBufCount = 4096;

	UCHAR BufBottom[TempBufCount];
	UCHAR BufTop[TempBufCount];

	int const DstSampleSize = m_DstFile.SampleSize();

	NUMBER_OF_CHANNELS const NumDstChannels = m_DstFile.Channels();

	int const TempBufSamples = sizeof BufTop / DstSampleSize;

	do
	{

		NUMBER_OF_SAMPLES CanReadSamples = NUMBER_OF_SAMPLES((m_DstEnd - m_DstPos) / DstSampleSize);
		if (CanReadSamples > TempBufSamples)
		{
			CanReadSamples = TempBufSamples;
		}

		long const DataSize = CanReadSamples * DstSampleSize;
		UCHAR * p1 = BufBottom;
		UCHAR * p2 = BufTop + DataSize;

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


		// save the old data to undo buffer
		if (NULL != m_pUndoLow)
		{
			m_pUndoLow->SaveUndoData(p1, DataSize, m_DstPos);
		}

		if (NULL != m_pUndoHigh)
		{
			m_pUndoHigh->SaveUndoData(p2, -DataSize, m_SrcPos);
		}


		for (NUMBER_OF_SAMPLES i = 0; i < CanReadSamples; i++)
		{
			LONG tmp[MAX_NUMBER_OF_CHANNELS];
			ASSERT(DstSampleSize <= sizeof tmp);

			p2 -= DstSampleSize;
			memcpy(tmp, p2, DstSampleSize);
			memcpy(p2, p1, DstSampleSize);
			memcpy(p1, tmp, DstSampleSize);
			p1 += DstSampleSize;
		}

		// save the data back
		if (-CanReadSamples != m_DstFile.WriteSamples(m_DstChan,
													m_SrcPos, -CanReadSamples, BufTop + DataSize,
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

		m_SrcPos -= DataSize;
		m_DstPos += DataSize;
	}
	while ((m_DstPos < m_DstEnd
				&& GetTickCount() - dwStartTime < 1000)
			);

	// notify the view
	m_pDocument->FileChanged(m_DstFile, dwOperationBeginBottom, m_DstPos);
	m_pDocument->FileChanged(m_DstFile, m_SrcPos, dwOperationBeginTop);

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
	if (m_pDocument->WaveFileID() == m_WaveFile.GetFileID())
	{
		m_pUndoData = new ThisClass(m_pDocument, m_WaveFile);
	}


	return TRUE;
}

BOOL CMetadataChangeOperation::OperationProc()
{
	if (m_MetadataCopyFlags & CWaveFile::InstanceDataWav::MetadataCopyAllCueData)
	{
		m_pDocument->UpdateAllMarkers();
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
		m_pDocument->UpdateAllMarkers();
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
	m_pMetadata->CopyMetadata(m_pDocument->m_WavFile.GetInstanceData(), ChangeFlags);
}

BOOL CMetadataChangeOperation::PrepareUndo()
{
	m_WaveFile = m_pDocument->m_WavFile;
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

/////////////  CCueReverseOperation      //////////////////////////////////////////////

CCueReverseOperation::CCueReverseOperation(CWaveSoapFrontDoc * pDoc,
											CWaveFile & DstFile, SAMPLE_INDEX StartDstSample,
											NUMBER_OF_SAMPLES LengthToReverse)
	: BaseClass(pDoc, DstFile, CWaveFile::InstanceDataWav::MetadataCopyAllCueData)
	, m_StartDstSample(StartDstSample)
	, m_LengthToReverse(LengthToReverse)
{
}

CCueReverseOperation::~CCueReverseOperation()
{
}

BOOL CCueReverseOperation::OperationProc()
{
	if (NULL == m_pMetadata)
	{
		m_pMetadata = new CWaveFile::InstanceDataWav;
	}

	m_pMetadata->CopyMetadata(m_WaveFile.GetInstanceData(), m_pMetadata->MetadataCopyAllCueData);

	BOOL HasChanged = m_pMetadata->ReverseMarkers(m_StartDstSample, m_LengthToReverse);

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

	if ( ! InitExpandOperation(DstFile, StartSample, length, chan))
	{
		return FALSE;
	}

	AddContext(new CInitChannels(m_pDocument, DstFile, StartSample,
								StartSample + length, chan));

	if (NeedUndo
		&& ! CreateUndo())
	{
		return FALSE;
	}
	return TRUE;
}

CWaveProcContext * CreateFadeInOutOperation(class CWaveSoapFrontDoc * pDoc, int FadeCurveType,
											CWaveFile & DstFile, SAMPLE_INDEX DstBegin, CHANNEL_MASK DstChannel,
											NUMBER_OF_SAMPLES Length, BOOL UndoEnabled)
{
	CWaveProcContext::auto_ptr pContext(new CWaveProcContext(pDoc));

	CFadeInOutProc::auto_ptr pProc(new CFadeInOutProc(FadeCurveType, Length));

	if ( ! pContext->InitInPlaceProcessing(DstFile, DstBegin, DstBegin+Length, DstChannel, UndoEnabled))
	{
		return NULL;
	}

	pContext->AddWaveProc(pProc.release());
	return pContext.release();
}
