// CustomSampleRateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "CustomSampleRateDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCustomSampleRateDlg dialog


CCustomSampleRateDlg::CCustomSampleRateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCustomSampleRateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCustomSampleRateDlg)
	m_SampleRate = 0;
	//}}AFX_DATA_INIT
}


void CCustomSampleRateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCustomSampleRateDlg)
	DDX_Text(pDX, IDC_EDIT_SAMPLE_RATE, m_SampleRate);
	DDV_MinMaxInt(pDX, m_SampleRate, 1, 1000000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCustomSampleRateDlg, CDialog)
	//{{AFX_MSG_MAP(CCustomSampleRateDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustomSampleRateDlg message handlers
