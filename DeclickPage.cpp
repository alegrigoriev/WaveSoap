// DeclickPage.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoap.h"
#include "DeclickPage.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeclickPage property page

IMPLEMENT_DYNCREATE(CDeclickPage, CPropertyPage)

CDeclickPage::CDeclickPage() : CPropertyPage(CDeclickPage::IDD)
{
	//{{AFX_DATA_INIT(CDeclickPage)
	m_ClickLogFilename = _T("");
	m_MaxClickLength = 24;
	m_MinClickAmplitude = 200;
	m_bLogClicks = FALSE;
	m_bLogClicksOnly = FALSE;
	m_bImportClicks = FALSE;
	m_ClickImportFilename = _T("");
	//}}AFX_DATA_INIT
	m_dAttackRate = .06;
	m_dClickToNoise = 5.;
	m_dEnvelopDecayRate = 0.02;
}

CDeclickPage::~CDeclickPage()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	pApp->Profile.RemoveSection(_T("Declicker"));
}

void CDeclickPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeclickPage)
	DDX_Control(pDX, IDC_EDIT_DECAY_RATE, m_EnvelopDecayRate);
	DDX_Control(pDX, IDC_EDIT_CLICK_TO_NOISE, m_ClickToNoise);
	DDX_Control(pDX, IDC_EDIT_ATTACK_RATE, m_AttackRate);
	DDX_Text(pDX, IDC_EDIT_CLICK_LOG_FILENAME, m_ClickLogFilename);
	DDX_Text(pDX, IDC_EDIT_MAX_CLICK_LENGTH, m_MaxClickLength);
	DDV_MinMaxInt(pDX, m_MaxClickLength, 6, 48);
	DDX_Text(pDX, IDC_EDIT_MIN_CLICK_AMPLITUDE, m_MinClickAmplitude);
	DDV_MinMaxInt(pDX, m_MinClickAmplitude, 50, 5000);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS, m_bLogClicks);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS_ONLY, m_bLogClicksOnly);
	DDX_Check(pDX, IDC_CHECK_IMPORT_CLICKS, m_bImportClicks);
	DDX_Text(pDX, IDC_EDIT_CLICK_IMPORT_FILENAME, m_ClickImportFilename);
	//}}AFX_DATA_MAP
	m_AttackRate.ExchangeData(pDX, m_dAttackRate,
							"Attack rate", "", 0.01, 0.9);
	m_ClickToNoise.ExchangeData(pDX, m_dClickToNoise,
								"Click to noise rate", "", 1., 10.);
	m_EnvelopDecayRate.ExchangeData(pDX, m_dEnvelopDecayRate,
									"Envelop decay rate", "", 0.01, 0.99);
}


BEGIN_MESSAGE_MAP(CDeclickPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDeclickPage)
	ON_BN_CLICKED(IDC_CHECK_LOG_CLICKS, OnCheckLogClicks)
	ON_BN_CLICKED(IDC_CHECK_IMPORT_CLICKS, OnCheckImportClicks)
	ON_BN_CLICKED(IDC_CLICK_LOG_BROWSE_BUTTON, OnClickLogBrowseButton)
	ON_BN_CLICKED(IDC_CLICK_IMPORT_BROWSE_BUTTON, OnClickImportBrowseButton)
	ON_BN_CLICKED(IDC_BUTTON_MORE_SETTINGS, OnButtonMoreSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeclickPage message handlers
void CDeclickPage::LoadValuesFromRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	pApp->Profile.AddBoolItem(_T("Declicker"), _T("LogClicks"), m_bLogClicks, FALSE);
	pApp->Profile.AddBoolItem(_T("Declicker"), _T("ImportClicks"), m_bImportClicks, FALSE);
	pApp->Profile.AddBoolItem(_T("Declicker"), _T("LogClicksOnly"), m_bLogClicksOnly, FALSE);
	pApp->Profile.AddItem(_T("Declicker"), _T("ClickLogFilename"), m_ClickLogFilename, _T(""));
	pApp->Profile.AddItem(_T("Declicker"), _T("ClickImportFilename"), m_ClickImportFilename, _T(""));
	pApp->Profile.AddItem(_T("Declicker"), _T("MaxClickLength"), m_MaxClickLength, 32, 6, 48);
	pApp->Profile.AddItem(_T("Declicker"), _T("MinClickAmplitude"), m_MinClickAmplitude, 200, 50, 5000);
	pApp->Profile.AddItem(_T("Declicker"), _T("AttackRate"), m_dAttackRate, 0.5, 0.001, 0.99);
	pApp->Profile.AddItem(_T("Declicker"), _T("DecayRate"), m_dEnvelopDecayRate, 0.01, 0.001, 0.99);
	pApp->Profile.AddItem(_T("Declicker"), _T("ClickToNoise"), m_dClickToNoise, 4., 1.5, 20);

}

void CDeclickPage::StoreValuesToRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	pApp->Profile.FlushSection(_T("Declicker"));
}

void CDeclickPage::OnOK()
{
	UpdateData();
	//CPropertyPage::OnOK();
}

BOOL CDeclickPage::OnSetActive()
{
	return TRUE;
	//return CPropertyPage::OnSetActive();
}

void CDeclickPage::SetWaveprocData(CClickRemoval * pCr)
{
	pCr->m_MeanPowerDecayRate = m_dEnvelopDecayRate;
	pCr->m_PowerToDeriv3RatioThreshold = m_dClickToNoise * m_dClickToNoise;
	pCr->m_MeanPowerAttackRate = m_dAttackRate;
	pCr->m_nMaxClickLength = m_MaxClickLength;
	pCr->m_MinDeriv3Threshold = m_MinClickAmplitude * m_MinClickAmplitude;
	if (m_bImportClicks)
	{
		pCr->SetClickSourceFile(m_ClickImportFilename);
	}
	if (m_bLogClicks)
	{
		pCr->SetClickLogFile(m_ClickLogFilename);
	}
	pCr->m_PassTrough = m_bLogClicksOnly;
}

void CDeclickPage::OnCheckLogClicks()
{
	BOOL Enable = IsDlgButtonChecked(IDC_CHECK_LOG_CLICKS);
	GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->EnableWindow(Enable);
	GetDlgItem(IDC_CLICK_LOG_BROWSE_BUTTON)->EnableWindow(Enable);
	GetDlgItem(IDC_CHECK_LOG_CLICKS_ONLY)->EnableWindow(Enable);
}

void CDeclickPage::OnCheckImportClicks()
{
	BOOL Enable = IsDlgButtonChecked(IDC_CHECK_IMPORT_CLICKS);
	GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->EnableWindow(Enable);
	GetDlgItem(IDC_CLICK_IMPORT_BROWSE_BUTTON)->EnableWindow(Enable);
}

BOOL CDeclickPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->EnableWindow(m_bLogClicks);
	GetDlgItem(IDC_CLICK_LOG_BROWSE_BUTTON)->EnableWindow(m_bLogClicks);
	GetDlgItem(IDC_CHECK_LOG_CLICKS_ONLY)->EnableWindow(m_bLogClicks);

	GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->EnableWindow(m_bImportClicks);
	GetDlgItem(IDC_CLICK_IMPORT_BROWSE_BUTTON)->EnableWindow(m_bImportClicks);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDeclickPage::OnClickLogBrowseButton()
{
	CString filter("Text files (*.txt)|*.txt|All Files (*.*)|*.*||");

	GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->GetWindowText(m_ClickLogFilename);

	CFileDialog fdlg(TRUE, "txt", m_ClickLogFilename,
					OFN_EXPLORER
					//| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_ClickLogFilename = fdlg.GetPathName();
		GetDlgItem(IDC_EDIT_CLICK_LOG_FILENAME)->SetWindowText(m_ClickLogFilename);
	}

}

void CDeclickPage::OnClickImportBrowseButton()
{
	CString filter("Text files (*.txt)|*.txt|All Files (*.*)|*.*||");

	GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->GetWindowText(m_ClickImportFilename);

	CFileDialog fdlg(TRUE, "txt", m_ClickImportFilename,
					OFN_EXPLORER
					| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_ClickImportFilename = fdlg.GetPathName();
		GetDlgItem(IDC_EDIT_CLICK_IMPORT_FILENAME)->SetWindowText(m_ClickImportFilename);
	}

}

void CDeclickPage::OnButtonMoreSettings()
{
	// TODO: Add your control notification handler code here
	CMoreDeclickDialog dlg;
	// set the data to dlg
	if (IDOK == dlg.DoModal())
	{
		// return the data from dlg
	}
}
/////////////////////////////////////////////////////////////////////////////
// CMoreDeclickDialog dialog


CMoreDeclickDialog::CMoreDeclickDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMoreDeclickDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMoreDeclickDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMoreDeclickDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoreDeclickDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMoreDeclickDialog, CDialog)
	//{{AFX_MSG_MAP(CMoreDeclickDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoreDeclickDialog message handlers
