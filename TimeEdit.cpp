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
	ON_WM_VSCROLL()
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
		m_Sample = GetTimeSample();
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

long CTimeEdit::GetTimeSample()
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

void CTimeEdit::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	TRACE("CTimeEdit::OnVScroll, nPos=%d\n", nPos);
	// nPos is actually an increment from CTimeSpinCtrl
	long nSample = GetTimeSample();
	int step = abs(int(nPos));
	int increment = 0;
	switch (m_TimeFormat & SampleToString_Mask)
	{
	default:
	case SampleToString_Sample:
		switch (step)
		{
		case 1:
			increment = 1;
			break;
		case 5:
			increment = 10;
			break;
		case 20:
		default:
			increment = 100;
			break;
		}
		break;
	case SampleToString_HhMmSs:
		switch (step)
		{
		case 1:
			if (m_TimeFormat & TimeToHhMmSs_NeedsMs)
			{
				increment = m_nSamplesPerSec / 10;
			}
			else
			{
				increment = m_nSamplesPerSec;
			}
			break;
		case 5:
			increment = m_nSamplesPerSec;
			break;
		case 20:
		default:
			increment = m_nSamplesPerSec * 60;
			break;
		}
		break;
	case SampleToString_Seconds:
		switch (step)
		{
		case 1:
			if (m_TimeFormat & TimeToHhMmSs_NeedsMs)
			{
				increment = m_nSamplesPerSec / 10;
			}
			else
			{
				increment = m_nSamplesPerSec;
			}
			break;
		case 5:
			increment = m_nSamplesPerSec;
			break;
		case 20:
		default:
			increment = m_nSamplesPerSec * 10;
			break;
		}
		break;
	}

	if (int(nPos) < 0)
	{
		nSample += increment - nSample % increment;
	}
	else
	{
		nSample -= increment + nSample % -increment;
	}
	if (nSample < 0)
	{
		nSample = 0;
	}
	SetTimeSample(nSample);
}
/////////////////////////////////////////////////////////////////////////////
// CTimeSpinCtrl

CTimeSpinCtrl::CTimeSpinCtrl()
{
}

CTimeSpinCtrl::~CTimeSpinCtrl()
{
}


BEGIN_MESSAGE_MAP(CTimeSpinCtrl, CSpinButtonCtrl)
	//{{AFX_MSG_MAP(CTimeSpinCtrl)
	ON_NOTIFY_REFLECT(UDN_DELTAPOS, OnDeltapos)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimeSpinCtrl message handlers

void CTimeSpinCtrl::OnDeltapos(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	//TRACE("CTimeSpinCtrl::OnDeltapos Delta=%d\n", pNMUpDown->iDelta);
	CTimeEdit * pBuddy = dynamic_cast<CTimeEdit *>(GetBuddy());
	if (pBuddy)
	{
		int SbCode = -1;
		if (pNMUpDown->iDelta < 0)
		{
			SbCode = SB_LINEUP;
		}
		else
		{
			SbCode = SB_LINEDOWN;
		}
		pBuddy->SendMessage(WM_VSCROLL, SbCode + (pNMUpDown->iDelta << 16), long(m_hWnd));
	}
	*pResult = 0;
}

CTimeEditCombo::CTimeEditCombo()
{
}

CTimeEditCombo::~CTimeEditCombo()
{
}

BEGIN_MESSAGE_MAP(CTimeEditCombo, CTimeEdit)
	//{{AFX_MSG_MAP(CTimeEditCombo)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnReflectComboSelectionChanged)
	ON_COMMAND(CBN_SELCHANGE, OnComboSelectionChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CTimeEditCombo::OnReflectComboSelectionChanged()
{
	CComboBox * pCB = (CComboBox *) this;
	int sel = pCB->GetCurSel();
	if (sel >= 0 && sel < m_Positions.size())
	{
		// repost it to itself
		// need to use posted message, because edit control is modified
		// after the function exits
		PostMessage(WM_COMMAND, CBN_SELCHANGE, NULL);
	}
}

void CTimeEditCombo::OnComboSelectionChanged()
{
	CComboBox * pCB = (CComboBox *) this;
	int sel = pCB->GetCurSel();
	if (sel >= 0 && sel < m_Positions.size())
	{
		SetTimeSample(m_Positions[sel]);
	}
}

void CTimeEditCombo::AddPosition(LPCTSTR name, long time)
{
	CComboBox * pCB = (CComboBox *) this;
	pCB->AddString(name);
	m_Positions.push_back(time);
}
