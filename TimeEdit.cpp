// TimeEdit.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "TimeEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeEdit

CTimeEdit::CTimeEdit()
	:m_nSamplesPerSec(1),
	m_TimeFormat(0),
	m_Sample(0)
{
}

CTimeEdit::~CTimeEdit()
{
}


BEGIN_MESSAGE_MAP(CTimeEdit, CEdit)
	//{{AFX_MSG_MAP(CTimeEdit)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeEdit message handlers
void CTimeEdit::ExchangeData(CDataExchange* pDX, long & sample)
{
	if (pDX->m_bSaveAndValidate)
	{
		UINT ID = GetDlgCtrlID( );
		(void)pDX->PrepareEditCtrl(ID);
		m_Sample = GetTimeSample(NULL);
		sample = m_Sample;
		return;
	}
	else
	{
		SetTimeSample(sample);
		return;
	}
}

void CTimeEdit::SetTimeFormat(int format)
{
	m_TimeFormat = format;
	//UpdateEditControl();
}

void CTimeEdit::SetTimeSample(long sample)
{
	m_Sample = sample;
	UpdateEditControl();
}

void CTimeEdit::UpdateEditControl()
{
	m_OriginalString = SampleToString(m_Sample, m_nSamplesPerSec, m_TimeFormat);
	SetWindowText(m_OriginalString);
}

long CTimeEdit::GetTimeSample(LPCTSTR str)
{
	CString s;
	GetWindowText(s);
	s.TrimLeft();
	s.TrimRight();
	if (s == m_OriginalString)
	{
		return m_Sample;
	}
	if (SampleToString_Sample == (m_TimeFormat & SampleToString_Mask))
	{
		// remove thousand separators
		m_OriginalString = s;
		s.Remove(GetApp()->m_ThousandSeparator);
		TCHAR * endptr;
		long num = _tcstoul(s, & endptr, 10);
		if (num < 0 || *endptr != 0)
		{
			num = 0;
		}
		m_Sample = num;
	}
	else
	{
		m_OriginalString = s;
		s.Remove(GetApp()->m_ThousandSeparator);
		int mult = 1000;
		int mult1 = 1000;
		double time = 0;    // in miliseconds
		BOOL SepFound = false;
		TCHAR DecimalSeparator = GetApp()->m_DecimalPoint;
		TCHAR TimeSeparator = GetApp()->m_TimeSeparator;

		for (int idx = s.GetLength() - 1; idx >= 0; idx--)
		{
			if (s[idx] == DecimalSeparator)
			{
				if (SepFound)
				{
					break;
				}
				SepFound = true;
				time /= mult / 1000;
				mult = 1000;
			}
			else if (s[idx] == TimeSeparator)
			{
				SepFound = true;
				mult1 *= 60;
				mult = mult1;
			}
			else if (s[idx] >= '0' && s[idx] <= '9')
			{
				time += mult * (s[idx] - '0');
				mult *= 10;
			}
			else
			{
				break;
			}
		}
		m_Sample = long(time * m_nSamplesPerSec / 1000.);
	}
	return m_Sample;
}
