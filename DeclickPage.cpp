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
	m_MaxClickLength = 14;
	m_MinClickAmplitude = 100;
	m_bLogClicks = FALSE;
	m_bLogClicksOnly = FALSE;
	//}}AFX_DATA_INIT
	m_dAttackRate = .06;
	m_dClickToNoise = 5.;
	m_dEnvelopDecayRate = 0.02;
}

CDeclickPage::~CDeclickPage()
{
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
	DDV_MinMaxInt(pDX, m_MaxClickLength, 6, 32);
	DDX_Text(pDX, IDC_EDIT_MIN_CLICK_AMPLITUDE, m_MinClickAmplitude);
	DDV_MinMaxInt(pDX, m_MinClickAmplitude, 50, 5000);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS, m_bLogClicks);
	DDX_Check(pDX, IDC_CHECK_LOG_CLICKS_ONLY, m_bLogClicksOnly);
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
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeclickPage message handlers
void CDeclickPage::LoadValuesFromRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	m_bLogClicks =
		(FALSE != pApp->GetProfileInt(_T("Declicker"), _T("LogClicks"), FALSE));
	m_bLogClicksOnly =
		(FALSE != pApp->GetProfileInt(_T("Declicker"), _T("LogClicksOnly"), FALSE));
	m_ClickLogFilename =
		pApp->GetProfileString(_T("Declicker"), _T("ClickLogFilename"), "");

	m_MaxClickLength =
		pApp->GetProfileInt(_T("Declicker"), _T("MaxClickLength"), 14);
	m_MinClickAmplitude =
		pApp->GetProfileInt(_T("Declicker"), _T("MinClickAmplitude"), 100);

	m_dAttackRate =
		pApp->GetProfileDouble(_T("Declicker"), _T("AttackRate"), 0.06, 0.01, 0.99);
	m_dEnvelopDecayRate =
		pApp->GetProfileDouble(_T("Declicker"), _T("DecayRate"), 0.06, 0.01, 0.99);
	m_dClickToNoise =
		pApp->GetProfileDouble(_T("Declicker"), _T("ClickToNoise"), 4., 1.5, 20);
}

void CDeclickPage::StoreValuesToRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	pApp->WriteProfileInt(_T("Declicker"), _T("LogClicks"), m_bLogClicks);
	pApp->WriteProfileInt(_T("Declicker"), _T("LogClicksOnly"), m_bLogClicksOnly);
	pApp->WriteProfileString(_T("Declicker"), _T("ClickLogFilename"), m_ClickLogFilename);

	pApp->WriteProfileInt(_T("Declicker"), _T("MaxClickLength"), m_MaxClickLength);
	pApp->WriteProfileInt(_T("Declicker"), _T("MinClickAmplitude"), m_MinClickAmplitude);

	pApp->WriteProfileDouble(_T("Declicker"), _T("AttackRate"), m_dAttackRate);
	pApp->WriteProfileDouble(_T("Declicker"), _T("DecayRate"), m_dEnvelopDecayRate);
	pApp->WriteProfileDouble(_T("Declicker"), _T("ClickToNoise"), m_dClickToNoise);
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
	pCr->m_MinDeriv3Threshold = m_MinClickAmplitude;
}
