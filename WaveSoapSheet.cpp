// WaveSoapSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoap.h"
#include "WaveSoapSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapSheet

IMPLEMENT_DYNAMIC(CWaveSoapSheet, CPropertySheet)

CWaveSoapSheet::CWaveSoapSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CWaveSoapSheet::CWaveSoapSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPage( & m_MainPage);
	AddPage( & m_DeclickPage);
	AddPage( & m_NoisePage);
	//m_psh.dwFlags |= PSH_USEPAGELANG;
}

CWaveSoapSheet::~CWaveSoapSheet()
{
}


BEGIN_MESSAGE_MAP(CWaveSoapSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CWaveSoapSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapSheet message handlers

BOOL CWaveSoapSheet::OnInitDialog()
{
	int Modeless = m_bModeless;
	m_bModeless = FALSE;
	BOOL bResult = CPropertySheet::OnInitDialog();
	m_bModeless = Modeless;
	CWnd * pBtn = GetDlgItem(IDOK);
	if (NULL != pBtn)
	{
		pBtn->EnableWindow(! m_MainPage.m_InFile.IsEmpty());
		pBtn->SetWindowText("Run");
	}

	pBtn = GetDlgItem(IDCANCEL);
	if (NULL != pBtn)
	{
		//pOkBtn->EnableWindow(FALSE);
		pBtn->SetWindowText("Exit");
	}

	HICON m_hIcon = (HICON) LoadImage(AfxFindResourceHandle
									(MAKEINTRESOURCE(IDR_MAINFRAME), RT_GROUP_ICON),
									MAKEINTRESOURCE(IDR_MAINFRAME),
									IMAGE_ICON,
									0, 0, LR_DEFAULTSIZE
									);
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);			// Set small icon

	return bResult;
}

void CWaveSoapSheet::LoadValuesFromRegistry()
{
	m_MainPage.LoadValuesFromRegistry();
	m_DeclickPage.LoadValuesFromRegistry();
	m_NoisePage.LoadValuesFromRegistry();
}

void CWaveSoapSheet::StoreValuesToRegistry()
{
	m_MainPage.StoreValuesToRegistry();
	m_DeclickPage.StoreValuesToRegistry();
	m_NoisePage.StoreValuesToRegistry();
}
