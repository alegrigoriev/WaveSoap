// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// TimeEdit.cpp : implementation file
//

#include "stdafx.h"
//#include "WaveSoapFront.h"
#include "TimeEdit.h"
#include "TimeToStr.h"
#include "resource.h"
#include "LocaleUtilities.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeEdit

CTimeEdit::CTimeEdit(int format)
	:m_nSamplesPerSec(1),
	m_TimeFormat(format),
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
void CTimeEdit::ExchangeData(CDataExchange* pDX, SAMPLE_INDEX & sample)
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

void CTimeEdit::SetTimeSample(SAMPLE_INDEX sample)
{
	m_Sample = sample;
	UpdateEditControl();
}

void CTimeEdit::UpdateEditControl()
{
	m_OriginalString = SampleToString(m_Sample, m_nSamplesPerSec, m_TimeFormat);
	SetWindowText(m_OriginalString);
}

SAMPLE_INDEX CTimeEdit::ChangeTimeFormat(int format)
{
	SAMPLE_INDEX sample = GetTimeSample();
	SetTimeFormat(format);
	SetTimeSample(sample);
	return sample;
}

SAMPLE_INDEX CTimeEdit::UpdateTimeSample()
{
	SAMPLE_INDEX tmp = GetTimeSample();
	SetTimeSample(tmp);
	return tmp;
}

SAMPLE_INDEX CTimeEdit::GetTimeSample()
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

		TCHAR const DecimalSeparator = GetApp()->m_DecimalPoint;
		TCHAR const TimeSeparator = GetApp()->m_TimeSeparator;

		int idx = s.GetLength() - 1;
		BOOL UseFrames = SampleToString_HhMmSsFf == (m_TimeFormat & SampleToString_Mask);

		if (idx >= 0)
		{
			if (TCHAR('f') == s[idx])
			{
				UseFrames = TRUE;
				idx--;
			}
		}

		for ( ; idx >= 0; idx--)
		{
			if (s[idx] == DecimalSeparator)
			{
				if (SepFound)
				{
					break;
				}
				SepFound = true;

				if (UseFrames)
				{
					mult = 1000;
					time /= 1000;
				}
				else
				{
					time /= mult / 1000;
					mult = 1000;
				}
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

		if (UseFrames)
		{
			m_Sample = SAMPLE_INDEX(floor(time / 1000.) * m_nSamplesPerSec + fmod(time, 1000.) * m_nSamplesPerSec / 75);
		}
		else
		{
			m_Sample = SAMPLE_INDEX(time * m_nSamplesPerSec / 1000.);
		}
	}
	return m_Sample;
}

void CTimeEdit::OnVScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar* /*pScrollBar*/)
{
	if (0) TRACE("CTimeEdit::OnVScroll, nPos=%d\n", nPos);
	// nPos is actually an increment from CTimeSpinCtrl
	SAMPLE_INDEX nSample = GetTimeSample();
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

	case SampleToString_HhMmSsFf:
		switch (step)
		{
		case 1:
			increment = m_nSamplesPerSec / 75;
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
		pBuddy->SendMessage(WM_VSCROLL, SbCode + (pNMUpDown->iDelta << 16), LPARAM(m_hWnd));

		NMHDR nm;
		nm.hwndFrom = m_hWnd;
		nm.idFrom = GetDlgCtrlID();
		nm.code = TSC_BUDDY_CHANGE;

		GetParent()->SendMessage(WM_NOTIFY, nm.idFrom, LPARAM( & nm));
	}
	*pResult = 0;
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
	unsigned sel = GetComboBox().GetCurSel();

	if (sel < m_Positions.size())
	{
		// repost it to itself
		// need to use posted message, because edit control is modified
		// after the function exits
		PostMessage(WM_COMMAND, CBN_SELCHANGE, NULL);
	}
}

void CTimeEditCombo::OnComboSelectionChanged()
{
	unsigned sel = GetComboBox().GetCurSel();

	if (sel < m_Positions.size())
	{
		SetTimeSample(m_Positions[sel]);
	}
}

void CTimeEditCombo::AddPosition(LPCTSTR name, SAMPLE_INDEX time)
{
	GetComboBox().AddString(name);
	m_Positions.push_back(time);
}

void CTimeEditCombo::AddPosition(UINT id, SAMPLE_INDEX time)
{
	CString s;
	s.LoadString(id);
	AddPosition(s, time);
}

CFileTimesCombo::CFileTimesCombo(SAMPLE_INDEX caret,
								CWaveFile & WaveFile, int TimeFormat)
	: BaseClass(TimeFormat)
	, m_WaveFile(WaveFile)
	, m_CaretPosition(caret)
{
	SetSamplingRate(WaveFile.SampleRate());
}

void CFileTimesCombo::FillFileTimes()
{
	AddPosition(IDS_BEGIN_OF_SAMPLE, 0);

	NUMBER_OF_SAMPLES FileLength = m_WaveFile.NumberOfSamples();
	AddPosition(IDS_END_OF_SAMPLE, FileLength);

	if (0 != m_CaretPosition
		&& FileLength != m_CaretPosition)
	{
		AddPosition(IDS_CURSOR, m_CaretPosition);
	}
	CWaveFile::InstanceDataWav * pInst = m_WaveFile.GetInstanceData();

	CString s;

	for (CuePointVectorIterator i = pInst->m_CuePoints.begin();
		i < pInst->m_CuePoints.end(); i++)
	{
		// TODO: include positions in HH:mm:ss and the tooltips
		LPCTSTR pNote = pInst->GetCueComment(i->CuePointID);
		LPCTSTR pLabel = pInst->GetCueLabel(i->CuePointID);
		WaveRegionMarker * pMarker = pInst->GetRegionMarker(i->CuePointID);

		if (NULL == pLabel
			|| 0 == pLabel[0])
		{
			// use comment text instead
			pLabel = pNote;
		}

		if (NULL != pMarker
			&& 0 == pMarker->SampleLength)
		{
			SAMPLE_INDEX end = i->dwSampleOffset + pMarker->SampleLength;

			if (i->dwSampleOffset <= unsigned(FileLength))
			{
				LPCTSTR pTitle = pLabel;

				if (NULL == pTitle
					|| 0 == pTitle[0])
				{
					pTitle = pMarker->Name;
				}

				s.Format(_T("Begin of %s"), pTitle);
				AddPosition(s, i->dwSampleOffset);
			}

			if (end <= FileLength
				&& unsigned(end) > i->dwSampleOffset)
			{
				LPCTSTR pTitle = pLabel;

				if (NULL == pTitle
					|| 0 == pTitle[0])
				{
					pTitle = pMarker->Name;
				}

				s.Format(_T("End of %s"), pTitle);
				AddPosition(s, end);
			}
		}
		else
		{
			if (i->dwSampleOffset <= unsigned(FileLength))
			{
				AddPosition(pLabel, i->dwSampleOffset);
			}
		}
	}
}

