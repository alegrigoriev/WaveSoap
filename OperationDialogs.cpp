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

void CVolumeChangeDialog::UpdateSelectionStatic()
{
	CString s;
	if (m_pWf->nChannels > 1)
	{
		LPCTSTR sChans = _T("Stereo");
		if (0 == m_Chan)
		{
			sChans = _T("Left");
		}
		else if (1 == m_Chan)
		{
			sChans = _T("Right");
		}
		s.Format("Selection : %s to %s (%s)\n"
				"Channels: %s",
				SampleToString(m_Start, m_pWf, m_TimeFormat),
				SampleToString(m_End, m_pWf, m_TimeFormat),
				SampleToString(m_End - m_Start, m_pWf, m_TimeFormat),
				sChans);
	}
	else
	{
		s.Format("Selection : %s to %s (%s)",
				SampleToString(m_Start, m_pWf, m_TimeFormat),
				SampleToString(m_End, m_pWf, m_TimeFormat),
				SampleToString(m_End - m_Start, m_pWf, m_TimeFormat));
	}
	m_SelectionStatic.SetWindowText(s);
}

void CVolumeChangeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
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
		DDX_Control(pDX, IDC_EDIT_VOLUME_RIGHT, m_eVolumeRight);
		DDX_Control(pDX, IDC_SLIDER_VOLUME_RIGHT, m_SliderVolumeRight);
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog message handlers

void CVolumeChangeDialog::OnChecklockChannels()
{
	m_bLockChannels = IsDlgButtonChecked(IDC_CHECKLOCK_CHANNELS);
	m_eVolumeRight.EnableWindow( ! m_bLockChannels);
	m_SliderVolumeRight.EnableWindow( ! m_bLockChannels);
	GetDlgItem(IDC_STATIC_RIGHT_CHANNEL)->EnableWindow( ! m_bLockChannels);
}

void CVolumeChangeDialog::OnButtonSelection()
{
	CSelectionDialog dlg;
	if (IDOK != dlg.DoModal())
	{
		return;
	}
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
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSelectionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectionDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectionDialog, CDialog)
//{{AFX_MSG_MAP(CSelectionDialog)
// NOTE: the ClassWizard will add message map macros here
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectionDialog message handlers

BOOL CVolumeChangeDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_pWf->nChannels > 1)
	{
		if (m_bLockChannels)
		{
			m_eVolumeRight.EnableWindow(FALSE);
			m_SliderVolumeRight.EnableWindow(FALSE);
			GetDlgItem(IDC_STATIC_RIGHT_CHANNEL)->EnableWindow(FALSE);
		}
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
