// MainPage.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoap.h"
#include "MainPage.h"
#include <process.h>
#include "WaveProgressDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainPage property page

IMPLEMENT_DYNCREATE(CMainPage, CPropertyPage)

CMainPage::CMainPage() : CPropertyPage(CMainPage::IDD)
{
	//{{AFX_DATA_INIT(CMainPage)
	m_bDoCameraNoiseReduction = FALSE;
	m_bDoDeclick = FALSE;
	m_bDoNoiseReduction = FALSE;
	m_bDoUlfNoiseReduction = FALSE;
	m_InFile = _T("");
	m_OutFile = _T("");
	//}}AFX_DATA_INIT
}

CMainPage::~CMainPage()
{
}

void CMainPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainPage)
	DDX_Check(pDX, IDC_CHECK_CAMERA_NOISE, m_bDoCameraNoiseReduction);
	DDX_Check(pDX, IDC_CHECK_DECLICK, m_bDoDeclick);
	DDX_Check(pDX, IDC_CHECK_NOISE_REDUCTION, m_bDoNoiseReduction);
	DDX_Check(pDX, IDC_CHECK_ULF_NOISE, m_bDoUlfNoiseReduction);
	DDX_Text(pDX, IDC_EDIT_SOURCE_FILENAME, m_InFile);
	DDV_MaxChars(pDX, m_InFile, 256);
	DDX_Text(pDX, IDC_EDIT_TARGET_FILENAME, m_OutFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMainPage, CPropertyPage)
	//{{AFX_MSG_MAP(CMainPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_SOURCE, OnButtonBrowseSource)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_TARGET, OnButtonBrowseTarget)
	ON_EN_CHANGE(IDC_EDIT_SOURCE_FILENAME, OnChangeEditSourceFilename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainPage message handlers

void CMainPage::LoadValuesFromRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	m_bDoDeclick =
		(FALSE != pApp->GetProfileInt(_T("Options"), _T("Declick"), TRUE));
	m_bDoNoiseReduction =
		(FALSE != pApp->GetProfileInt(_T("Options"), _T("NoiseReduction"), TRUE));
	m_bDoUlfNoiseReduction =
		(FALSE != pApp->GetProfileInt(_T("Options"), _T("UlfNoiseReduction"), TRUE));
	m_bDoCameraNoiseReduction =
		(FALSE != pApp->GetProfileInt(_T("Options"), _T("CameraNoiseReduction"), FALSE));
}

void CMainPage::StoreValuesToRegistry()
{
	CWaveSoapApp * pApp = (CWaveSoapApp *)AfxGetApp();
	pApp->WriteProfileInt(_T("Options"), _T("Declick"), m_bDoDeclick);
	pApp->WriteProfileInt(_T("Options"), _T("NoiseReduction"), m_bDoNoiseReduction);
	pApp->WriteProfileInt(_T("Options"), _T("UlfNoiseReduction"), m_bDoUlfNoiseReduction);
	pApp->WriteProfileInt(_T("Options"), _T("CameraNoiseReduction"), m_bDoCameraNoiseReduction);
}

void CMainPage::OnOK()
{
	UpdateData();
}

BOOL CMainPage::OnSetActive()
{
	return TRUE;
}

BOOL CMainPage::OnApply()
{
	ProcessSoundFile();
	return FALSE;
}

static BOOL WaveProcCallback(CWaveProc * pProc, UINT msg,
							size_t TotalSize, size_t pos)
{
	CWaveProgressDialog * pDlg =
		(CWaveProgressDialog *) pProc->m_dwCallbackData;

	switch(msg)
	{
	case WAVEPROC_MSG_PROGRESS:
		if (pDlg != NULL
			&& pDlg->m_hWnd != NULL
			&& TotalSize >= 100
			&& FALSE != pDlg->m_bContinueProcess)
		{
			int Percent = pos / (TotalSize / 100);
			if (Percent != pDlg->m_nPercent)
			{
				pDlg->m_nPercent = Percent;
				HWND hProgress = ::GetDlgItem(pDlg->m_hWnd, IDC_PROGRESS1);
				::PostMessage(hProgress, PBM_SETPOS, Percent, 0L);
				// calculate estimated time to finish
				int TickCount = GetTickCount();
				if (pos != 0 && TickCount - pDlg->StartTickCount > 1000)
				{
					CString s;
					int TimeLeft =
						int((TickCount - pDlg->StartTickCount)
							* double(TotalSize - pos) / double(pos) / 1000.);
					if (TimeLeft >= 60)
					{
						TimeLeft = (TimeLeft + 30) / 60;
						s.Format("Estimated time left:  about %d minute%s",
								TimeLeft, (TimeLeft > 1) ? "s" : "");
					}
					else
					{
						s.Format("Estimated time left: %d seconds",
								TimeLeft);
					}
					::SetWindowText(
									::GetDlgItem(pDlg->m_hWnd, IDC_STATIC_ESTIMATED),
									s);
				}
			}
		}
		else
		{
			if (pDlg != NULL
				&& pDlg->m_hWnd != NULL
				&& FALSE == pDlg->m_bContinueProcess)
			{
				::PostMessage(pDlg->m_hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
				return FALSE;
			}
		}
		break;
	case WAVEPROC_MSG_FINISHED:
		if (pDlg != NULL
			&& pDlg->m_hWnd != NULL)
		{
			pDlg->m_bContinueProcess = FALSE;
			::PostMessage(pDlg->m_hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
		}
		break;
	}
	return TRUE;
}

unsigned __stdcall WaveProcThread(void * arg)
{
	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	CWaveProgressDialog * pWaveDlg = (CWaveProgressDialog *) arg;
	CWaveSoapSheet * pDlg = pWaveDlg->pDlg;
	CMainPage * pMain = & pDlg->m_MainPage;

	CDeclickPage * pDeclick = & pDlg->m_DeclickPage;

	CNoiseReductionPage * pNoise = & pDlg->m_NoisePage;

	CClickRemoval cr;
	CHumRemoval hr;
	CBatchProcessing bp;
	CNoiseReduction nr;
	bp.m_Callback = WaveProcCallback;
	bp.m_dwCallbackData = (DWORD)pWaveDlg;

	if (pMain->m_bDoUlfNoiseReduction)
	{
		bp.AddWaveProc( & hr);
	}

	if (pMain->m_bDoDeclick)
	{
		pDeclick->SetWaveprocData( & cr);
		bp.AddWaveProc( & cr);
	}

	if (pMain->m_bDoNoiseReduction)
	{
		pNoise->SetWaveprocData( & nr);
		bp.AddWaveProc( & nr);
	}

	pWaveDlg->pProc = & bp;
	ProcessWaveFile(pMain->m_InFile, pMain->m_OutFile, & bp);
	return 0;
}

void CMainPage::ProcessSoundFile()
{
	CWaveSoapSheet * pDlg = (CWaveSoapSheet *) GetParent();
	ASSERT(pDlg->IsKindOf(RUNTIME_CLASS(CWaveSoapSheet)));
	CMainPage * pMain = & pDlg->m_MainPage;
	if (m_InFile.IsEmpty()
		|| ! (m_bDoDeclick
			|| m_bDoCameraNoiseReduction
			|| m_bDoNoiseReduction
			|| m_bDoUlfNoiseReduction))
	{
		return;
	}

	if (m_OutFile.IsEmpty())
	{
		m_OutFile = m_InFile;
	}

	unsigned id;
	CWaveProgressDialog dlg(GetParent());
	dlg.pDlg = pDlg;
	dlg.hThread = (HANDLE)_beginthreadex(NULL, 0, WaveProcThread,
										& dlg, 0, & id);
	dlg.DoModal();
}

void CMainPage::OnButtonBrowseSource()
{
	CString filter;
	filter.LoadString(IDS_AVI_FILTER);

	GetDlgItem(IDC_EDIT_SOURCE_FILENAME)->GetWindowText(m_InFile);

	CFileDialog fdlg(TRUE, "wav", m_InFile,
					OFN_EXPLORER
					| OFN_FILEMUSTEXIST
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_InFile = fdlg.GetPathName();
		GetDlgItem(IDC_EDIT_SOURCE_FILENAME)->SetWindowText(m_InFile);
		CWnd * pTarget = GetDlgItem(IDC_EDIT_TARGET_FILENAME);
		if (NULL != pTarget
			&& 0 == pTarget->GetWindowTextLength())
		{
			m_OutFile = m_InFile;
			pTarget->SetWindowText(m_InFile);
		}
	}
}

void CMainPage::OnButtonBrowseTarget()
{
	CString filter;
	filter.LoadString(IDS_AVI_FILTER);

	GetDlgItem(IDC_EDIT_TARGET_FILENAME)->GetWindowText(m_OutFile);

	CFileDialog fdlg(FALSE, "wav", m_OutFile,
					OFN_EXPLORER
					| OFN_NOTESTFILECREATE
					| OFN_OVERWRITEPROMPT
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		m_OutFile = fdlg.GetPathName();
		GetDlgItem(IDC_EDIT_TARGET_FILENAME)->SetWindowText(m_OutFile);
	}
}

void CMainPage::OnChangeEditSourceFilename()
{
	CWnd * pEdit = GetDlgItem(IDC_EDIT_SOURCE_FILENAME);
	CWnd * pBtn = GetParent()->GetDlgItem(IDOK);
	if (pEdit != NULL && NULL != pBtn)
	{
		pBtn->EnableWindow( 0 != pEdit->GetWindowTextLength());
	}
}
