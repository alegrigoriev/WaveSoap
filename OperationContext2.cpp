// OperationContext2.cpp
#include "stdafx.h"
#include "OperationContext2.h"
#include "OperationDialogs.h"

static int fround(double d)
{
	return floor(d + 0.5);
	if (d >= 0.)
	{
		return int(d + 0.5);
	}
	else
	{
		return int(d - 0.5);
	}
}

void SkipWhitespace(LPCSTR * ppStr)
{
	LPCSTR str = *ppStr;
	while (' ' == *str
			|| '\n' == *str
			|| '\t' == *str
			|| '\r' == *str)
	{
		str++;
	}
	*ppStr = str;
}

CExpressionEvaluationContext::CExpressionEvaluationContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString, LPCTSTR OperationName)
	:COperationContext(pDoc, StatusString, 0)

{
	m_OperationString = StatusString;
	m_OperationName = OperationName;
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
	m_bClipped = false;
	m_MaxClipped = 0;
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
	m_nFileSampleArgument = (m_DstStart - m_DstFile.GetDataChunk()->dwDataOffset) / m_DstFile.SampleSize();
	m_dFileTimeArgument = m_nFileSampleArgument / double(m_nSamplingRate);
	return TRUE;
}

BOOL CExpressionEvaluationContext::ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward)
{
	// calculate number of sample, and time
	int nSampleSize = m_DstFile.SampleSize();
	int nChannels = m_DstFile.Channels();
	int nSample = (offset - m_DstStart) / nSampleSize;
	__int16 * pDst = (__int16 *) buf;
	try
	{
		if (1 == nChannels)
		{
			while (len >= sizeof (__int16))
			{
				m_dCurrentSample = *pDst * 0.00003051850947599719229;
				Evaluate();
				int result = fround(* m_pResultAddress);
				if (result > 0x7FFF)
				{
					m_bClipped = true;
					if (m_MaxClipped < result)
					{
						m_MaxClipped = result;
					}
					result = 0x7FFF;
				}
				else if (result < -0x8000)
				{
					if (m_MaxClipped < -result)
					{
						m_MaxClipped = -result;
					}
					result = -0x8000;
					m_bClipped = true;
				}
				*pDst = result;
				pDst++;
				len -= sizeof (__int16);
				m_nSelectionSampleArgument++;
				m_nFileSampleArgument++;
				m_dSelectionTimeArgument += m_SamplePeriod;
				m_dFileTimeArgument += m_SamplePeriod;
			}
		}
		else
		{
			while (len >= sizeof (__int16))
			{
				if (0 == (offset & sizeof (__int16)))
				{
					if (m_DstChan != 1) // not right only
					{
						m_dCurrentSample = *pDst * 0.00003051850947599719229;
						Evaluate();
						int result = fround(* m_pResultAddress);
						if (result > 0x7FFF)
						{
							if (m_MaxClipped < result)
							{
								m_MaxClipped = result;
							}
							result = 0x7FFF;
							m_bClipped = true;
						}
						else if (result < -0x8000)
						{
							if (m_MaxClipped < -result)
							{
								m_MaxClipped = -result;
							}
							result = -0x8000;
							m_bClipped = true;
						}
						*pDst = result;
					}
					offset += sizeof (__int16);
					pDst++;
					len -= sizeof (__int16);
				}
				if (len >= sizeof (__int16))
				{
					if (m_DstChan != 0) // not left only
					{
						m_dCurrentSample = *pDst * 0.00003051850947599719229;
						Evaluate();
						int result = * m_pResultAddress;
						if (result > 0x7FFF)
						{
							if (m_MaxClipped < result)
							{
								m_MaxClipped = result;
							}
							result = 0x7FFF;
							m_bClipped = true;
						}
						else if (result < -0x8000)
						{
							if (m_MaxClipped < -result)
							{
								m_MaxClipped = -result;
							}
							result = -0x8000;
							m_bClipped = true;
						}
						*pDst = result;
					}
					offset += sizeof (__int16);
					pDst++;
					len -= sizeof (__int16);
				}

				m_nSelectionSampleArgument++;
				m_nFileSampleArgument++;
				m_dSelectionTimeArgument += m_SamplePeriod;
				m_dFileTimeArgument += m_SamplePeriod;
			}
		}
	}
	catch (const char * pError)
	{
		m_ErrorString = pError;
		return FALSE;
	}
	return TRUE;
}

CString CExpressionEvaluationContext::GetToken(LPCSTR * ppStr, TokenType * pType)
{
	// get either delimiter, operand or operator
	static struct TokenTable
	{
		char * token;
		TokenType type;
	}
	Table[] =
	{
		"(", eLeftParenthesis,
		")", eRightParenthesis,
		"-", eMinusOp,
		"+", ePlusOp,
		"/", eDivideOp,
		"*", eMultiplyOp,
		"%", eModuloOp,
		"&", eBinaryAndOp,
		"|", eBinaryOrOp,
		"^", eBinaryXorOp,
		"~", eBinaryNotOp,
		"int", eInt,
		"float", eDouble,
		"double", eDouble,
		"sinh", eSinusHFunc,
		"sin", eSinusFunc,
		"cosh", eCosinusHFunc,
		"cos", eCosinusFunc,
		"tanh", eTangensHFunc,
		"tan", eTangensFunc,
		"exp10", eExp10Func,
		"exp", eExpFunc,
		"log10", eLog10Func,
		"log", eLogFunc,
		"sqrt", eSqrtFunc,
		"abs", eAbsFunc,
		"noise", eNoiseFunc,
		"pi", ePiConstant,
		"T", eAbsoluteTime,
		"t", eSelectionTime,
		"dt", eSelectionLengthTime,
		"DT", eFileLengthTime,
		"F", eSamplingRate,
		"dn", eSelectionLengthSamples,
		"DN", eFileLengthSamples,
		//"T", eSamplePeriod,
		"f1", eCurrentFrequencyArgument1,
		"f2", eCurrentFrequencyArgument2,
		"f3", eCurrentFrequencyArgument3,
		"f", eCurrentFrequencyArgument,
		"wave", eCurrentSampleValue,
	};
	SkipWhitespace(ppStr);
	LPCSTR str = *ppStr;
	if ('\0' == *str)
	{
		*pType = eEndOfExpression;
		return "";
	}
	for (int i = 0; i < sizeof Table / sizeof Table[0];i++)
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
CExpressionEvaluationContext::CompileParenthesedExpression(LPCSTR * ppStr)
{
	TokenType type;
	LPCSTR prevStr = *ppStr;
	CString token = GetToken( ppStr, & type);
	if (type != eLeftParenthesis
		|| eRightParenthesis != CompileExpression(ppStr))
	{
		throw "Right parenthesis expected";
	}
	return eRightParenthesis;
}

void CExpressionEvaluationContext::CompileFunctionOfDouble(void (_fastcall * Function)(Operation * t), LPCSTR * ppStr)
{
	CompileParenthesedExpression( ppStr);
	// can't put those right to the function call, because
	// order of evaluation would be unpredicted
	double * pArg = PopDouble();
	double * pDst = PushDouble();

	AddOperation(Function, pDst, pArg, NULL);
}

CExpressionEvaluationContext::TokenType
	CExpressionEvaluationContext::CompileTerm(LPCSTR * ppStr)
{
	// term may be either expression in parentheses, or function call, or unary operation
	// and a term
	TokenType type;
	int IntConstant;
	double DoubleConstant;
	char * endptr;

	LPCSTR prevStr = *ppStr;
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
		IntConstant = strtol(prevStr, & endptr, 0);
		*ppStr = endptr;
		PushConstant(IntConstant);
		break;
	case eDoubleConstant:
		DoubleConstant = strtod(prevStr, & endptr);
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
	CExpressionEvaluationContext::CompileExpression(LPCSTR * ppStr)
{
	TokenType type, type1;
	//void (_fastcall * Function)(Operation * t);
	type = CompileTerm(ppStr);
	if (eEndOfExpression == type)
	{
		throw "Expression syntax error";
	}
	LPCSTR str = *ppStr;

	while (1)
	{
		LPCSTR prevStr = *ppStr;
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
	m_OperationArray.Add(t);
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

BOOL CExpressionEvaluationContext::SetExpression(LPCSTR * ppszExpression)
{
	// parse the string
	m_ErrorString.Empty();
	try {
		m_DataStackIndex = 0;
		m_DataTypeStackIndex = 0;
		m_ConstantBufferIndex = 0;
		m_OperationArray.RemoveAll();
		if (eEndOfExpression != CompileExpression(ppszExpression))
		{
			throw "Expression syntax error";
		}
		// todo: norm the sample
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
	for (int i = 0; i < m_OperationArray.GetSize(); i++)
	{
		m_OperationArray[i].Function( & m_OperationArray[i]);
	}
}

BOOL CInsertSilenceContext::InitExpand(CWaveFile & DstFile, long StartSample, long Length,
										int Channel, BOOL NeedUndo)
{
	m_pExpandShrinkContext = new CResizeContext(pDocument, _T("Expanding the file..."), "");
	if (NULL == m_pExpandShrinkContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	if ( ! m_pExpandShrinkContext->InitExpand(DstFile, StartSample,
											Length, Channel))
	{
		delete m_pExpandShrinkContext;
		m_pExpandShrinkContext = NULL;
		return FALSE;
	}
	InitDestination(DstFile, StartSample, StartSample + Length, Channel, FALSE);
	m_pExpandShrinkContext->InitUndoRedo("Insert Silence");
	m_Flags |= CopyExpandFile;
	return TRUE;
}

BOOL CInsertSilenceContext::OperationProc()
{
	// process ShrinkExpand context first, then proceed with clearing the area
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Flags & CopyExpandFile)
	{
		if (NULL != m_pExpandShrinkContext)
		{
			if ( 0 == (m_pExpandShrinkContext->m_Flags & (OperationContextStop | OperationContextFinished)))
			{
				if ( ! m_pExpandShrinkContext->OperationProc())
				{
					m_pExpandShrinkContext->m_Flags |= OperationContextStop;
				}
			}
			PercentCompleted = m_pExpandShrinkContext->PercentCompleted;
			if (m_pExpandShrinkContext->m_Flags &
				(OperationContextStop | OperationContextFinished))
			{
				m_Flags &= ~(CopyExpandFile | CopyShrinkFile);
			}
			return TRUE;
		}
		m_Flags &= ~(CopyExpandFile | CopyShrinkFile);
		PercentCompleted = 0;
	}

	return COperationContext::OperationProc();
}

BOOL CInsertSilenceContext::ProcessBuffer(void * buf, size_t BufferLength, DWORD offset, BOOL bBackward)
{
	__int16 * pDst = (__int16 *) buf;
	if (m_DstFile.Channels() == 1
		|| ALL_CHANNELS == m_DstChan)
	{
		memset(buf, 0, BufferLength);
		return TRUE;
	}
	// change one channel
	if ((offset & 2)
		!= m_DstChan * 2)
	{
		// skip this word
		pDst++;
		BufferLength -= 2;
	}

	for (int i = 0; i < BufferLength / (2 * sizeof pDst[0]); i++, pDst += 2)
	{
		pDst[0] = 0;
	}

	BufferLength -= i * (2 * sizeof pDst[0]);
	if (2 == BufferLength)
	{
		pDst[0] = 0;
	}
	return TRUE;
}

BOOL CCommitFileSaveContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_DstFile.InitializeTheRestOfFile(500, & PercentCompleted))
	{
		m_Flags |= OperationContextFinished;
	}
	return TRUE;
}

void CCommitFileSaveContext::PostRetire(BOOL bChildContext)
{
	m_DstFile.Close();
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
	COperationContext::PostRetire(bChildContext);
}

void CConversionContext::PostRetire(BOOL bChildContext)
{
	if (m_Flags & ConvertContextReplaceWholeFile)
	{
		if (m_Flags & OperationContextFinished)
		{
			pDocument->m_WavFile.DetachSourceFile();
			pDocument->m_WavFile = m_DstFile;
			// since we replaced the file, it's no more direct
			pDocument->SetModifiedFlag(TRUE, m_pUndoContext != NULL);
			if (NULL != m_pUndoContext)
			{
				m_pUndoContext->m_bOldDirectMode = pDocument->m_bDirectMode;
				m_pUndoContext->m_OldAllocatedWavePeakSize =
					pDocument->m_AllocatedWavePeakSize;
				m_pUndoContext->m_OldWavePeakSize = pDocument->m_WavePeakSize;
				pDocument->m_WavePeakSize = 0;
				m_pUndoContext->m_OldPeakDataGranularity =
					pDocument->m_PeakDataGranularity;
				m_pUndoContext->m_pOldPeaks = pDocument->m_pPeaks;

				// detach peaks
				pDocument->m_pPeaks = NULL;
				pDocument->m_AllocatedWavePeakSize = 0;
				pDocument->m_WavePeakSize = 0;

				pDocument->AddUndoRedo(m_pUndoContext);
				m_pUndoContext = NULL;
			}
			if (pDocument->m_bDirectMode)
			{
				pDocument->m_bDirectMode = false;
				pDocument->UpdateFrameTitles();        // will cause name change in views
			}
			long nSamples = pDocument->WaveFileSamples();
			pDocument->SoundChanged(pDocument->WaveFileID(),
									0, nSamples, nSamples, UpdateSoundDontRescanPeaks);
			pDocument->BuildPeakInfo(FALSE);    // don't save it yet
			pDocument->UpdateAllViews(NULL, pDocument->UpdateWholeFileChanged);
		}
		else
		{
			// we don't replace the file, nothing changed
			if (NULL != m_pUndoContext)
			{
				delete m_pUndoContext;
				m_pUndoContext = NULL;
			}
		}
	}
	CCopyContext::PostRetire(bChildContext);
}


void CExpressionEvaluationContext::PostRetire(BOOL bChildContext)
{
	if (m_bClipped)
	{
		CString s;
		s.Format(IDS_SOUND_CLIPPED, pDocument->GetTitle(), int(m_MaxClipped * 100. / 32678));
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
	}
	COperationContext::PostRetire(bChildContext);
}

CEqualizerContext::CEqualizerContext(CWaveSoapFrontDoc * pDoc,
									LPCTSTR StatusString, LPCTSTR OperationName)
	: COperationContext(pDoc, OperationName, OperationContextDiskIntensive),
	m_bClipped(FALSE),
	m_bZeroPhase(FALSE),
	m_bSecondPass(FALSE),
	m_MaxClipped(0.)
{
	m_OperationString = StatusString;
	//m_GetBufferFlags = 0; // leave CDirectFile::GetBufferAndPrefetchNext flag
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
}

CEqualizerContext::~CEqualizerContext()
{
}

BOOL CEqualizerContext::Init()
{
	m_ReturnBufferFlags = CDirectFile::ReturnBufferDirty;
	if (m_bZeroPhase)
	{
		m_NumberOfBackwardPasses = 1;
	}
	return InitPass(1);
}

BOOL CEqualizerContext::InitPass(int nPass)
{
	TRACE("CEqualizerContext::InitPass %d\n", nPass);
	for (int i = 0; i < MaxNumberOfEqualizerBands; i++)
	{
		m_PrevSamples[0][i][0] = 0.;
		m_PrevSamples[1][i][0] = 0.;
		m_PrevSamples[0][i][1] = 0.;
		m_PrevSamples[1][i][1] = 0.;
		m_PrevSamples[0][i][2] = 0.;
		m_PrevSamples[1][i][2] = 0.;
		m_PrevSamples[0][i][3] = 0.;
		m_PrevSamples[1][i][3] = 0.;
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

void CEqualizerContext::PostRetire(BOOL bChildContext)
{
	if (m_bClipped)
	{
		CString s;
		s.Format(IDS_SOUND_CLIPPED, pDocument->GetTitle(), int(m_MaxClipped * 100. / 32678));
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
	}
	COperationContext::PostRetire(bChildContext);
}

BOOL CEqualizerContext::ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward)
{
	// calculate number of sample, and time
	int nSampleSize = m_DstFile.SampleSize();
	int nChannels = m_DstFile.Channels();
	//int nSample = (offset - m_DstStart) / nSampleSize;
	__int16 * pDst = (__int16 *) buf;
	if (1 == nChannels)
	{
		if ( ! bBackward)
		{
			while (len >= sizeof (__int16))
			{
				double dResult = CalculateResult(0, *pDst);
				int result = fround(dResult);
				if (result > 0x7FFF)
				{
					if (m_MaxClipped < dResult)
					{
						m_MaxClipped = dResult;
					}
					m_bClipped = true;
					result = 0x7FFF;
				}
				else if (result < -0x8000)
				{
					if (m_MaxClipped < -dResult)
					{
						m_MaxClipped = -dResult;
					}
					result = -0x8000;
					m_bClipped = true;
				}
				*pDst = result;
				pDst++;
				len -= sizeof (__int16);
			}
		}
		else
		{
			pDst += len / sizeof (__int16);
			while (len >= sizeof (__int16))
			{
				pDst --;
				double dResult = CalculateResult(0, *pDst);
				int result = fround(dResult);
				if (result > 0x7FFF)
				{
					if (m_MaxClipped < dResult)
					{
						m_MaxClipped = dResult;
					}
					m_bClipped = true;
					result = 0x7FFF;
				}
				else if (result < -0x8000)
				{
					if (m_MaxClipped < -dResult)
					{
						m_MaxClipped = -dResult;
					}
					result = -0x8000;
					m_bClipped = true;
				}
				*pDst = result;
				len -= sizeof (__int16);
			}
		}
	}
	else
	{
		if ( ! bBackward)
		{
			while (len >= sizeof (__int16))
			{
				if (0 == (offset & sizeof (__int16)))
				{
					if (m_DstChan != 1) // not right only
					{
						double dResult = CalculateResult(0, *pDst);
						int result = fround(dResult);
						if (result > 0x7FFF)
						{
							if (m_MaxClipped < dResult)
							{
								m_MaxClipped = dResult;
							}
							m_bClipped = true;
							result = 0x7FFF;
						}
						else if (result < -0x8000)
						{
							if (m_MaxClipped < -dResult)
							{
								m_MaxClipped = -dResult;
							}
							result = -0x8000;
							m_bClipped = true;
						}
						*pDst = result;
					}
					offset += sizeof (__int16);
					pDst++;
					len -= sizeof (__int16);
				}
				if (len >= sizeof (__int16))
				{
					if (m_DstChan != 0) // not left only
					{
						double dResult = CalculateResult(1, *pDst);
						int result = fround(dResult);
						if (result > 0x7FFF)
						{
							if (m_MaxClipped < dResult)
							{
								m_MaxClipped = dResult;
							}
							m_bClipped = true;
							result = 0x7FFF;
						}
						else if (result < -0x8000)
						{
							if (m_MaxClipped < -dResult)
							{
								m_MaxClipped = -dResult;
							}
							result = -0x8000;
							m_bClipped = true;
						}
						*pDst = result;
					}
					offset += sizeof (__int16);
					pDst++;
					len -= sizeof (__int16);
				}

			}
		}
		else
		{
			pDst += len / sizeof (__int16);
			offset += len;
			while (len >= sizeof (__int16))
			{
				pDst--;
				offset -= sizeof (__int16);

				if (0 == (offset & sizeof (__int16)))
				{
					if (m_DstChan != 1) // not right only
					{
						double dResult = CalculateResult(0, *pDst);
						int result = fround(dResult);
						if (result > 0x7FFF)
						{
							if (m_MaxClipped < dResult)
							{
								m_MaxClipped = dResult;
							}
							m_bClipped = true;
							result = 0x7FFF;
						}
						else if (result < -0x8000)
						{
							if (m_MaxClipped < -dResult)
							{
								m_MaxClipped = -dResult;
							}
							result = -0x8000;
							m_bClipped = true;
						}
						*pDst = result;
					}
				}
				else
				{
					if (m_DstChan != 0) // not left only
					{
						double dResult = CalculateResult(1, *pDst);
						int result = fround(dResult);
						if (result > 0x7FFF)
						{
							if (m_MaxClipped < dResult)
							{
								m_MaxClipped = dResult;
							}
							m_bClipped = true;
							result = 0x7FFF;
						}
						else if (result < -0x8000)
						{
							if (m_MaxClipped < -dResult)
							{
								m_MaxClipped = -dResult;
							}
							result = -0x8000;
							m_bClipped = true;
						}
						*pDst = result;
					}
				}
				len -= sizeof (__int16);
			}
		}
	}
	return TRUE;
}

BOOL CSwapChannelsContext::ProcessBuffer(void * buf, size_t len, DWORD offset, BOOL bBackward)
{
	__int16 * pDst = (__int16 *) buf;
	// channels are always 2
	for (unsigned i = 0; i < len / sizeof pDst[0]; i += 2)
	{
		__int16 tmp = pDst[i];
		pDst[i] = pDst[i + 1];
		pDst[i + 1] = tmp;
	}
	return TRUE;
}
