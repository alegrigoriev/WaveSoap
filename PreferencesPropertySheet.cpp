// PreferencesPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "PreferencesPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet

IMPLEMENT_DYNAMIC(CPreferencesPropertySheet, CPropertySheet)

CPreferencesPropertySheet::CPreferencesPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CPreferencesPropertySheet::CPreferencesPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CPreferencesPropertySheet::~CPreferencesPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CPreferencesPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPreferencesPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet message handlers
