// NoiseReductionPage.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoap.h"
#include "NoiseReductionPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionPage property page

IMPLEMENT_DYNCREATE(CNoiseReductionPage, CPropertyPage)

CNoiseReductionPage::CNoiseReductionPage() : CPropertyPage(CNoiseReductionPage::IDD)
{
	//{{AFX_DATA_INIT(CNoiseReductionPage)
	m_nFftOrderExp = -1;
	//}}AFX_DATA_INIT
	m_dTransientThreshold = 2;
	m_dNoiseReduction = 10.;
	m_dNoiseCriterion = 0.25;
	m_dNoiseThreshold = -70.;
	m_dContinuousThreshold = -80.;
	m_dLowerFrequency = 4000.;
	m_FftOrder = 128;
}

CNoiseReductionPage::~CNoiseReductionPage()
{
}

void CNoiseReductionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNoiseReductionPage)
	DDX_Control(pDX, IDC_EDIT_AGGRESSIVNESS, m_EditAggressivness);
	DDX_Control(pDX, IDC_EDIT_NOISE_REDUCTION, m_eNoiseReduction);
	DDX_Control(pDX, IDC_EDIT_NOISE_CRITERION, m_eNoiseCriterion);
	DDX_Control(pDX, IDC_EDIT_NOISE_AREA_THRESHOLD, m_eNoiseThreshold);
	DDX_Control(pDX, IDC_EDIT_LOWER_FREQUENCY, m_eLowerFrequency);
	DDX_Control(pDX, IDC_EDIT_CONT_AREA_THRESHOLD, m_eContinuousThreshold);
	DDX_CBIndex(pDX, IDC_COMBO_FFT_ORDER, m_nFftOrderExp);
	//}}AFX_DATA_MAP
	m_FftOrder = 256 << m_nFftOrderExp;
#if 0
	m_eTransientThreshold.ExchangeData(pDX, m_dTransientThreshold,
										"Transient threshold", "", 0.3, 2);
#endif
	m_eNoiseReduction.ExchangeData(pDX, m_dNoiseReduction,
									"Noise reduction", "dB", 0., 100.);
	m_eNoiseCriterion.ExchangeData(pDX, m_dNoiseCriterion,
									"Noise/continuous criterion", "", 0.0, 1.);
	m_eNoiseThreshold.ExchangeData(pDX, m_dNoiseThreshold,
									"Noise floor for noise", "dB", -100., -10.);
	m_eLowerFrequency.ExchangeData(pDX, m_dLowerFrequency,
									"Frequency", "Hz", 100., 48000.);
	m_eContinuousThreshold.ExchangeData(pDX, m_dContinuousThreshold,
										"Noise floor for continuous tone", "dB", -100., -10.);

}


BEGIN_MESSAGE_MAP(CNoiseReductionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNoiseReductionPage)
	ON_BN_CLICKED(IDC_BUTTON_MORE, OnButtonMore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNoiseReductionPage message handlers
void CNoiseReductionPage::LoadValuesFromRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	//m_bPhaseFilter = (FALSE != pApp->GetProfileInt(_T("NoiseReduction"), _T("PhaseFilter"), FALSE));

	pApp->Profile.AddItem(_T("NoiseReduction"), _T("FftOrder"), m_FftOrder, 2048, 256, 16384);

	if (m_FftOrder >= 16384)
	{
		m_nFftOrderExp = 6;
	}
	else if (m_FftOrder >= 8192)
	{
		m_nFftOrderExp = 5;
	}
	else if (m_FftOrder >= 4096)
	{
		m_nFftOrderExp = 4;
	}
	else if (m_FftOrder >= 2048)
	{
		m_nFftOrderExp = 3;
	}
	else if (m_FftOrder >= 1024)
	{
		m_nFftOrderExp = 2;
	}
	else if (m_FftOrder >= 512)
	{
		m_nFftOrderExp = 1;
	}
	else
	{
		m_nFftOrderExp = 0;
	}

	pApp->Profile.AddItem(_T("NoiseReduction"), _T("TransientThreshold"), m_dTransientThreshold, 1., 0.3, 2);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseReduction"), m_dNoiseReduction, 10., 0., 100.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseCriterion"), m_dNoiseCriterion, 0.25, 0., 1.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("NoiseThreshold"), m_dNoiseThreshold, -70., -100., -10.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("LowerFrequency"), m_dLowerFrequency, 4000., 100., 48000.);
	pApp->Profile.AddItem(_T("NoiseReduction"), _T("ContinuousThreshold"), m_dContinuousThreshold, -80., -100., -10.);
}

void CNoiseReductionPage::StoreValuesToRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	m_FftOrder = 256 << m_nFftOrderExp;
	pApp->Profile.FlushSection(_T("NoiseReduction"));
}

#define DB_TO_NEPER 0.115129254
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616

void CNoiseReductionPage::SetWaveprocData(CNoiseReduction * pNr)
{
	//pNr->m_bApplyPhaseFilter = m_bPhaseFilter;
	pNr->m_MinFrequencyToProcess = m_dLowerFrequency;
	pNr->m_ThresholdOfTransient = m_dTransientThreshold;
	pNr->m_FreqThresholdOfNoiselike = M_PI_2 * M_PI_2 * m_dNoiseCriterion * m_dNoiseCriterion;
	pNr->m_MaxNoiseSuppression = DB_TO_NEPER * m_dNoiseReduction;
	pNr->m_LevelThresholdForNoise = DB_TO_NEPER * m_dNoiseThreshold + 22.;
	pNr->m_LevelThresholdForStationary = DB_TO_NEPER * m_dContinuousThreshold + 22.;
}

void CNoiseReductionPage::OnOK()
{
	UpdateData();
}

BOOL CNoiseReductionPage::OnSetActive()
{
	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog dialog


CMoreNoiseDialog::CMoreNoiseDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMoreNoiseDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMoreNoiseDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMoreNoiseDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoreNoiseDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMoreNoiseDialog, CDialog)
	//{{AFX_MSG_MAP(CMoreNoiseDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoreNoiseDialog message handlers

void CNoiseReductionPage::OnButtonMore()
{
	CMoreNoiseDialog dlg;
	// set the data to dlg
	if (IDOK == dlg.DoModal())
	{
		// return the data from dlg
	}

}
