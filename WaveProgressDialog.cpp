// WaveProgressDialog.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoap.h"
#include "WaveProgressDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveProgressDialog dialog


CWaveProgressDialog::CWaveProgressDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CWaveProgressDialog::IDD, pParent),
	pProc(NULL), pDlg(NULL), hThread(NULL), m_nPercent(-1),
	StartTickCount(GetTickCount()), m_bContinueProcess(TRUE)
{
	//{{AFX_DATA_INIT(CWaveProgressDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWaveProgressDialog::~CWaveProgressDialog()
{
	if (NULL != hThread)
	{
		m_bContinueProcess = FALSE;
		WaitForSingleObject(hThread, 10000);
		CloseHandle(hThread);
		hThread = NULL;
	}
}

void CWaveProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaveProgressDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaveProgressDialog, CDialog)
	//{{AFX_MSG_MAP(CWaveProgressDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveProgressDialog message handlers

void CWaveProgressDialog::OnCancel()
{
	// TODO: Add extra cleanup here
	if (FALSE != m_bContinueProcess)
	{
		m_bContinueProcess = FALSE;
	}
	else
		CDialog::OnCancel();
}
