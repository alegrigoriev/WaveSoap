// OperationDialogs.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "resource.h"
#include "OperationDialogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg dialog


CCopyChannelsSelectDlg::CCopyChannelsSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCopyChannelsSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCopyChannelsSelectDlg)
	m_ChannelToCopy = -1;
	//}}AFX_DATA_INIT
}


void CCopyChannelsSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyChannelsSelectDlg)
	DDX_Radio(pDX, IDC_RADIO_LEFT, m_ChannelToCopy);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCopyChannelsSelectDlg, CDialog)
//{{AFX_MSG_MAP(CCopyChannelsSelectDlg)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog dialog


CPasteModeDialog::CPasteModeDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPasteModeDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasteModeDialog)
	m_PasteMode = -1;
	//}}AFX_DATA_INIT
}


void CPasteModeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasteModeDialog)
	DDX_Radio(pDX, IDC_RADIO_SELECT, m_PasteMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasteModeDialog, CDialog)
//{{AFX_MSG_MAP(CPasteModeDialog)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog message handlers

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog dialog


CVolumeChangeDialog::CVolumeChangeDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CVolumeChangeDialog::IDD, pParent),
	m_TimeFormat(SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs),
	m_pWf(NULL),
	m_Chan(2)
{
	//{{AFX_DATA_INIT(CVolumeChangeDialog)
	m_bUndo = FALSE;
	m_bLockChannels = FALSE;
	m_DbPercent = 0;
	//}}AFX_DATA_INIT
	// m_DbPercent: 0 = Decibel, 1 - Percent
	m_dVolumeLeftDb = 0.;
	m_dVolumeRightDb = 0.;
	m_dVolumeLeftPercent = 100.;
	m_dVolumeRightPercent = 100.;

}

void CVolumeChangeDialog::UpdateVolumeData(CDataExchange* pDX, BOOL InPercents)
{
	if (0 == InPercents)
	{
		// decibels
		m_eVolumeLeft.ExchangeData(pDX, m_dVolumeLeftDb,
									"Volume change", "dB", -40., 40.);
		if (m_pWf->nChannels > 1)
		{
			m_eVolumeRight.ExchangeData(pDX, m_dVolumeRightDb,
										"Volume change", "dB", -40., 40.);
		}
	}
	else
	{
		// percents
		m_eVolumeLeft.ExchangeData(pDX, m_dVolumeLeftPercent,
									"Volume change", "%", 1., 10000.);
		if (m_pWf->nChannels > 1)
		{
			m_eVolumeRight.ExchangeData(pDX, m_dVolumeRightPercent,
										"Volume change", "%", 1., 10000.);
		}
	}
}
static CString GetSelectionText(long Start, long End, int Chan,
								int nChannels, BOOL bLockChannels,
								long nSamplesPerSec, int TimeFormat)
{
	CString s;
	if (nChannels > 1)
	{
		LPCTSTR sChans = _T("Stereo");
		if (! bLockChannels)
		{
			if (0 == Chan)
			{
				sChans = _T("Left");
			}
			else if (1 == Chan)
			{
				sChans = _T("Right");
			}
		}
		s.Format("Selection : %s to %s (%s)\n"
				"Channels: %s",
				SampleToString(Start, nSamplesPerSec, TimeFormat),
				SampleToString(End, nSamplesPerSec, TimeFormat),
				SampleToString(End - Start, nSamplesPerSec, TimeFormat),
				sChans);
	}
	else
	{
		s.Format("Selection : %s to %s (%s)",
				SampleToString(Start, nSamplesPerSec, TimeFormat),
				SampleToString(End, nSamplesPerSec, TimeFormat),
				SampleToString(End - Start, nSamplesPerSec, TimeFormat));
	}
	return s;
}

void CVolumeChangeDialog::UpdateSelectionStatic()
{
	m_SelectionStatic.SetWindowText(GetSelectionText(m_Start, m_End, m_Chan,
													m_pWf->nChannels, m_bLockChannels,
													m_pWf->nSamplesPerSec, m_TimeFormat));
}

void CVolumeChangeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	// initialize slider control dirst, then edit control
	//{{AFX_DATA_MAP(CVolumeChangeDialog)
	DDX_Control(pDX, IDC_SLIDER_VOLUME_LEFT, m_SliderVolumeLeft);
	DDX_Control(pDX, IDC_EDIT_VOLUME_LEFT, m_eVolumeLeft);
	DDX_Control(pDX, IDC_STATIC_SELECTION, m_SelectionStatic);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	//}}AFX_DATA_MAP
	// get data first, then get new selection of the combobox
	if (m_pWf->nChannels > 1)
	{
		DDX_Check(pDX, IDC_CHECKLOCK_CHANNELS, m_bLockChannels);
		DDX_Control(pDX, IDC_SLIDER_VOLUME_RIGHT, m_SliderVolumeRight);
		DDX_Control(pDX, IDC_EDIT_VOLUME_RIGHT, m_eVolumeRight);
	}

	UpdateVolumeData(pDX, m_DbPercent);
	DDX_CBIndex(pDX, IDC_COMBODB_PERCENT, m_DbPercent);
	if ( ! pDX->m_bSaveAndValidate)
	{
		UpdateSelectionStatic();
	}
}


BEGIN_MESSAGE_MAP(CVolumeChangeDialog, CDialog)
//{{AFX_MSG_MAP(CVolumeChangeDialog)
ON_BN_CLICKED(IDC_CHECKLOCK_CHANNELS, OnChecklockChannels)
ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
ON_CBN_SELCHANGE(IDC_COMBODB_PERCENT, OnSelchangeCombodbPercent)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT_VOLUME_LEFT, OnKillfocusEditVolumeLeft)
	ON_EN_KILLFOCUS(IDC_EDIT_VOLUME_RIGHT, OnKillfocusEditVolumeRight)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog message handlers

void CVolumeChangeDialog::OnChecklockChannels()
{
	m_bLockChannels = IsDlgButtonChecked(IDC_CHECKLOCK_CHANNELS);
	UpdateEnables();
	UpdateSelectionStatic();
}

void CVolumeChangeDialog::UpdateEnables()
{
	BOOL EnableLeft = m_bLockChannels || 1 != m_Chan;
	BOOL EnableRight = ! m_bLockChannels && 0 != m_Chan;

	m_eVolumeLeft.EnableWindow(EnableLeft);
	m_SliderVolumeLeft.EnableWindow(EnableLeft);
	GetDlgItem(IDC_STATIC_LEFT_CHANNEL)->EnableWindow(EnableLeft);

	m_eVolumeRight.EnableWindow(EnableRight);
	m_SliderVolumeRight.EnableWindow(EnableRight);
	GetDlgItem(IDC_STATIC_RIGHT_CHANNEL)->EnableWindow(EnableRight);
}

void CVolumeChangeDialog::OnButtonSelection()
{
	CSelectionDialog dlg;
	dlg.m_Start = m_Start;
	dlg.m_End = m_End;
	dlg.m_Length = m_End - m_Start;
	dlg.m_FileLength = m_FileLength;
	dlg.m_Chan = m_Chan;
	dlg.m_pWf = m_pWf;
	dlg.m_TimeFormat = m_TimeFormat;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.m_Start;
	m_End = dlg.m_End;
	m_Chan = dlg.m_Chan;
	UpdateSelectionStatic();
}

void CVolumeChangeDialog::OnSelchangeCombodbPercent()
{
	int sel = ((CComboBox *)GetDlgItem(IDC_COMBODB_PERCENT))->GetCurSel();
	TRACE("OnSelchangeCombodbPercent() sel=%d\n", sel);
	if (sel == m_DbPercent)
	{
		return;
	}
	UpdateData();
	if (0 == sel)
	{
		// get percent values and convert to dB values
		if (m_dVolumeLeftPercent > 0)
		{
			m_dVolumeLeftDb = 20 * log10(0.01 * m_dVolumeLeftPercent);
			if (m_dVolumeLeftDb < -40.)
			{
				m_dVolumeLeftDb = -40.;
			}
			if (m_dVolumeLeftDb > 40.)
			{
				m_dVolumeLeftDb = 40.;
			}
		}
		else
		{
			m_dVolumeLeftDb = 0;
		}

		if (m_dVolumeRightPercent > 0)
		{
			m_dVolumeRightDb = 20 * log10(m_dVolumeRightPercent / 100.);
			if (m_dVolumeRightDb < -40.)
			{
				m_dVolumeRightDb = -40.;
			}
			else if (m_dVolumeRightDb > 40.)
			{
				m_dVolumeRightDb = 40.;
			}
		}
		else
		{
			m_dVolumeRightDb = 0;
		}
	}
	else
	{
		m_dVolumeLeftPercent = 100. * pow(10., m_dVolumeLeftDb / 20.);
		if (m_dVolumeLeftPercent < 1.)
		{
			m_dVolumeLeftPercent = 1.;
		}
		else if (m_dVolumeLeftPercent > 10000.)
		{
			m_dVolumeLeftPercent = 10000.;
		}
		m_dVolumeRightPercent = 100. * pow(10., m_dVolumeRightDb / 20.);
		if (m_dVolumeRightPercent < 1.)
		{
			m_dVolumeRightPercent = 1.;
		}
		else if (m_dVolumeRightPercent > 10000.)
		{
			m_dVolumeRightPercent = 10000.;
		}
	}
	UpdateData(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog dialog


CSelectionDialog::CSelectionDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectionDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectionDialog)
	m_Chan = -1;
	m_TimeFormatIndex = 0;
	m_SelectionNumber = 0;
	//}}AFX_DATA_INIT
}


void CSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectionDialog)
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	DDX_Control(pDX, IDC_EDIT_START, m_eStart);
	DDX_Control(pDX, IDC_EDIT_END, m_eEnd);
	DDX_Radio(pDX, IDC_RADIO_CHANNEL, m_Chan);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	DDX_CBIndex(pDX, IDC_COMBO_SELECTION, m_SelectionNumber);
	//}}AFX_DATA_MAP
	m_eStart.ExchangeData(pDX, m_Start);
	m_eEnd.ExchangeData(pDX, m_End);
	m_eLength.ExchangeData(pDX, m_Length);
}


BEGIN_MESSAGE_MAP(CSelectionDialog, CDialog)
//{{AFX_MSG_MAP(CSelectionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	ON_EN_KILLFOCUS(IDC_EDIT_END, OnKillfocusEditEnd)
	ON_EN_KILLFOCUS(IDC_EDIT_LENGTH, OnKillfocusEditLength)
	ON_EN_KILLFOCUS(IDC_EDIT_START, OnKillfocusEditStart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog message handlers

BOOL CVolumeChangeDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_SliderVolumeLeft.SetRange(-400, 400);
	m_SliderVolumeLeft.SetTicFreq(100);
	m_SliderVolumeLeft.SetLineSize(10);
	m_SliderVolumeLeft.SetPageSize(50);
	if (0 == m_DbPercent)
	{
		// decibel
		m_SliderVolumeLeft.SetPos(int(m_dVolumeLeftDb * 10.));
	}
	else
	{
		m_SliderVolumeLeft.SetPos(int(200. * log10(m_dVolumeLeftPercent / 100.)));
	}

	if (m_pWf->nChannels > 1)
	{
		m_SliderVolumeRight.SetRange(-400, 400);
		m_SliderVolumeRight.SetTicFreq(100);
		m_SliderVolumeRight.SetLineSize(10);
		m_SliderVolumeRight.SetPageSize(50);
		if (0 == m_DbPercent)
		{
			// decibel
			m_SliderVolumeRight.SetPos(int(m_dVolumeLeftDb * 10.));
		}
		else
		{
			m_SliderVolumeRight.SetPos(int(200. * log10(m_dVolumeLeftPercent / 100.)));
		}
		UpdateEnables();
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CVolumeChangeDialog::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	CSliderCtrl * pSlider = dynamic_cast<CSliderCtrl *>(pScrollBar);
	if (NULL != pSlider)
	{
		int pos = pSlider->GetPos();
		CString s;
		if (0 == m_DbPercent)
		{
			s.Format("%.2f", pos / 10.);
		}
		else
		{
			s.Format("%.0f", 100. * pow(10., pos / 200.));
		}
		int id = pSlider->GetDlgCtrlID();
		if (IDC_SLIDER_VOLUME_LEFT == id)
		{
			SetDlgItemText(IDC_EDIT_VOLUME_LEFT, s);
			if (2 == m_pWf->nChannels
				&& m_bLockChannels)
			{
				SetDlgItemText(IDC_EDIT_VOLUME_RIGHT, s);
				m_SliderVolumeRight.SetPos(pos);
			}
		}
		else if (IDC_SLIDER_VOLUME_RIGHT == id)
		{
			SetDlgItemText(IDC_EDIT_VOLUME_RIGHT, s);
		}
	}
}

void CVolumeChangeDialog::OnKillfocusEditVolumeLeft()
{
	double num;
	CString s;
	m_eVolumeLeft.GetWindowText(s);
	int SliderPos = 0;
	if (! s.IsEmpty()
		&& m_eVolumeLeft.SimpleFloatParse(s, num))
	{
		if (0 == m_DbPercent)
		{
			// decibel
			if (num < -40. || num > 40.)
			{
				return;
			}
			SliderPos = int(num * 10.);
		}
		else
		{
			// percent
			if (num < 1.
				|| num > 10000.)
			{
				return;
			}
			SliderPos = int(200. * log10(num / 100.));
		}
		m_SliderVolumeLeft.SetPos(SliderPos);
	}
}

void CVolumeChangeDialog::OnKillfocusEditVolumeRight()
{
	double num;
	CString s;
	m_eVolumeRight.GetWindowText(s);
	int SliderPos = 0;
	if (! s.IsEmpty()
		&& m_eVolumeRight.SimpleFloatParse(s, num))
	{
		if (0 == m_DbPercent)
		{
			// decibel
			if (num < -40. || num > 40.)
			{
				return;
			}
			SliderPos = int(num * 10.);
		}
		else
		{
			// percent
			if (num < 1.
				|| num > 10000.)
			{
				return;
			}
			SliderPos = int(200. * log10(num / 100.));
		}
		m_SliderVolumeRight.SetPos(SliderPos);
	}
}

BOOL CSelectionDialog::OnInitDialog()
{
	m_eStart.SetTimeFormat(m_TimeFormat);
	m_eEnd.SetTimeFormat(m_TimeFormat);
	m_eLength.SetTimeFormat(m_TimeFormat);
	switch (m_TimeFormat & SampleToString_Mask)
	{
	case SampleToString_Sample:
		m_TimeFormatIndex = 0;
		break;
	case SampleToString_HhMmSs:
		m_TimeFormatIndex = 1;
		break;
	case SampleToString_Seconds: default:
		m_TimeFormatIndex = 2;
		break;
	}
	if (NULL != m_pWf)
	{
		m_eStart.SetSamplingRate(m_pWf->nSamplesPerSec);
		m_eEnd.SetSamplingRate(m_pWf->nSamplesPerSec);
		m_eLength.SetSamplingRate(m_pWf->nSamplesPerSec);
	}
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectionDialog::OnSelchangeComboTimeFormat()
{
	int sel = ((CComboBox *)GetDlgItem(IDC_COMBO_TIME_FORMAT))->GetCurSel();
	int Format;
	switch (sel)
	{
	case 0:
		Format = SampleToString_Sample;
		break;
	case 1:
		Format = SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm;
		break;
	case 2:
	default:
		Format = SampleToString_Seconds | TimeToHhMmSs_NeedsMs;
		break;
	}
	if (Format == m_TimeFormat)
	{
		return;
	}
	m_TimeFormat = Format;
	m_Start = m_eStart.GetTimeSample();
	m_eStart.SetTimeFormat(Format);
	m_eStart.SetTimeSample(m_Start);
	m_End = m_eEnd.GetTimeSample();
	m_eEnd.SetTimeFormat(Format);
	m_eEnd.SetTimeSample(m_End);
	m_Length = m_End - m_Start;
	m_eLength.SetTimeFormat(Format);
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionDialog::OnKillfocusEditEnd()
{
	m_End = m_eEnd.GetTimeSample();
	m_eEnd.SetTimeSample(m_End);
	m_Length = m_End - m_Start;
	if (m_Length < 0) m_Length = 0;
	m_eLength.SetTimeSample(m_Length);
}

void CSelectionDialog::OnKillfocusEditLength()
{
	m_Length = m_eLength.GetTimeSample();
	m_eLength.SetTimeSample(m_Length);
	m_End = m_Start + m_Length;
	m_eEnd.SetTimeSample(m_End);
}

void CSelectionDialog::OnKillfocusEditStart()
{
	m_Start = m_eStart.GetTimeSample();
	m_eStart.SetTimeSample(m_Start);
	m_Length = m_End - m_Start;
	if (m_Length < 0) m_Length = 0;
	m_eLength.SetTimeSample(m_Length);
}
/////////////////////////////////////////////////////////////////////////////
// CGotoDialog dialog


CGotoDialog::CGotoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CGotoDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGotoDialog)
	m_TimeFormatIndex = -1;
	//}}AFX_DATA_INIT
}


void CGotoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGotoDialog)
	DDX_Control(pDX, IDC_EDIT_START, m_eStart);
	DDX_CBIndex(pDX, IDC_COMBO_TIME_FORMAT, m_TimeFormatIndex);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGotoDialog, CDialog)
	//{{AFX_MSG_MAP(CGotoDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGotoDialog message handlers
/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog dialog


CDcOffsetDialog::CDcOffsetDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CDcOffsetDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDcOffsetDialog)
	m_b5SecondsDC = FALSE;
	m_bUndo = FALSE;
	m_nDcOffset = 0;
	m_DcSelectMode = -1;
	//}}AFX_DATA_INIT
}


void CDcOffsetDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDcOffsetDialog)
	DDX_Check(pDX, IDC_CHECK_5SECONDS, m_b5SecondsDC);
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Text(pDX, IDC_EDIT_DC_OFFSET, m_nDcOffset);
	DDX_Radio(pDX, IDC_RADIO_DC_SELECT, m_DcSelectMode);
	//}}AFX_DATA_MAP
	if (0 == m_DcSelectMode)
	{
		GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(FALSE);
		//GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(FALSE);
	}
	GetDlgItem(IDC_STATIC_SELECTION)->SetWindowText(
													GetSelectionText(m_Start, m_End, m_Chan,
														m_pWf->nChannels, FALSE,
														m_pWf->nSamplesPerSec, m_TimeFormat));
}


BEGIN_MESSAGE_MAP(CDcOffsetDialog, CDialog)
	//{{AFX_MSG_MAP(CDcOffsetDialog)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_BN_CLICKED(IDC_RADIO_DC_SELECT, OnRadioDcSelect)
	ON_BN_CLICKED(IDC_RADIO2, OnRadioAdjustSelectEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDcOffsetDialog message handlers

void CDcOffsetDialog::OnButtonSelection()
{
	CSelectionDialog dlg;
	dlg.m_Start = m_Start;
	dlg.m_End = m_End;
	dlg.m_Length = m_End - m_Start;
	dlg.m_FileLength = m_FileLength;
	dlg.m_Chan = m_Chan;
	dlg.m_pWf = m_pWf;
	dlg.m_TimeFormat = m_TimeFormat;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	m_Start = dlg.m_Start;
	m_End = dlg.m_End;
	m_Chan = dlg.m_Chan;
	GetDlgItem(IDC_STATIC_SELECTION)->SetWindowText(
													GetSelectionText(m_Start, m_End, m_Chan,
														m_pWf->nChannels, FALSE,
														m_pWf->nSamplesPerSec, m_TimeFormat));
}

void CDcOffsetDialog::OnRadioDcSelect()
{
	GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(FALSE);
}

void CDcOffsetDialog::OnRadioAdjustSelectEdit()
{
	GetDlgItem(IDC_EDIT_DC_OFFSET)->EnableWindow(TRUE);
}
