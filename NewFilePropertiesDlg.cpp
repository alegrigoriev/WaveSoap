// NewFilePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "NewFilePropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg dialog


CNewFilePropertiesDlg::CNewFilePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewFilePropertiesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewFilePropertiesDlg)
	m_bShowOnlyWhenShift = FALSE;
	m_MonoStereo = -1;
	m_nSamplingRate = -1;
	//}}AFX_DATA_INIT
}


void CNewFilePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewFilePropertiesDlg)
	DDX_Control(pDX, IDC_SPIN_LENGTH, m_SpinLength);
	DDX_Text(pDX, IDC_COMBO_SAMPLING_RATE, m_nSamplingRate);
	DDV_MinMaxInt(pDX, m_nSamplingRate, 1, 1000000);
	DDX_Check(pDX, IDC_CHECK_SHOW_ONLY_WHEN_SHIFT, m_bShowOnlyWhenShift);
	DDX_Radio(pDX, IDC_RADIO_MONO, m_MonoStereo);
	DDX_Control(pDX, IDC_EDIT_LENGTH, m_eLength);
	//}}AFX_DATA_MAP
	m_eLength.ExchangeData(pDX, m_Length);
	DDV_MinMaxInt(pDX, m_Length, 0, 4800);
}


BEGIN_MESSAGE_MAP(CNewFilePropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CNewFilePropertiesDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg message handlers

BOOL CNewFilePropertiesDlg::OnInitDialog()
{
	m_eLength.SetTimeFormat(SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm);
	m_eLength.SetSamplingRate(1);
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
