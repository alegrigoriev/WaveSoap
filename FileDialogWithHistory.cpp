// FileDialogWithHistory.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FileDialogWithHistory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileDialogWithHistory

IMPLEMENT_DYNAMIC(CFileDialogWithHistory, CFileDialog)

CFileDialogWithHistory::CFileDialogWithHistory(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
												DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
}


BEGIN_MESSAGE_MAP(CFileDialogWithHistory, CFileDialog)
	//{{AFX_MSG_MAP(CFileDialogWithHistory)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

