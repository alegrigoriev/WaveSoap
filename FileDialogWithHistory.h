#if !defined(AFX_FILEDIALOGWITHHISTORY_H__F63C5621_0671_4E36_A5E8_E3182EB884BB__INCLUDED_)
#define AFX_FILEDIALOGWITHHISTORY_H__F63C5621_0671_4E36_A5E8_E3182EB884BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FileDialogWithHistory.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFileDialogWithHistory dialog

class CFileDialogWithHistory : public CFileDialog
{
	DECLARE_DYNAMIC(CFileDialogWithHistory)

public:
	CFileDialogWithHistory(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
							LPCTSTR lpszDefExt = NULL,
							LPCTSTR lpszFileName = NULL,
							DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							LPCTSTR lpszFilter = NULL,
							CWnd* pParentWnd = NULL);

protected:
	//{{AFX_MSG(CFileDialogWithHistory)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEDIALOGWITHHISTORY_H__F63C5621_0671_4E36_A5E8_E3182EB884BB__INCLUDED_)
