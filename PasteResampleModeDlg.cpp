// PasteResampleModeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoapfront.h"
#include "PasteResampleModeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPasteResampleModeDlg dialog


CPasteResampleModeDlg::CPasteResampleModeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasteResampleModeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasteResampleModeDlg)
	m_ModeSelect = -1;
	//}}AFX_DATA_INIT
}


void CPasteResampleModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasteResampleModeDlg)
	DDX_Control(pDX, IDC_STATIC_TITLE, m_Static);
	DDX_Radio(pDX, IDC_RADIO_SELECT, m_ModeSelect);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasteResampleModeDlg, CDialog)
	//{{AFX_MSG_MAP(CPasteResampleModeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasteResampleModeDlg message handlers

BOOL CPasteResampleModeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString format, s;
	m_Static.GetWindowText(format);

	s.Format(format, m_TargetSampleRate, m_SrcSampleRate);

	m_Static.SetWindowText(s);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
