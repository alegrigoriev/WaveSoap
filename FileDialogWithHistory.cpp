// FileDialogWithHistory.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FileDialogWithHistory.h"
#include <Dlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileDialogWithHistory

IMPLEMENT_DYNAMIC(CFileDialogWithHistory, CFileDialog)

BEGIN_MESSAGE_MAP(CFileDialogWithHistory, CFileDialog)
	//{{AFX_MSG_MAP(CFileDialogWithHistory)
	ON_CBN_SELENDOK(IDC_COMBO_RECENT, OnComboSelendOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CFileDialogWithHistory::OnComboSelendOK()
{
	TRACE("CFileDialogWithHistory::OnComboSelendOK()\n");
	CString str;
	CComboBox * pCb = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_RECENT));
	if (NULL != pCb)
	{
		int sel = pCb->GetCurSel();
		if (-1 == sel
			|| sel >= pCb->GetCount())
		{
			return;
		}
		pCb->GetLBText(sel, str);
		TRACE("CFileDialogWithHistory::OnComboSelendOK: %s selected\n", str);
		if (str.IsEmpty())
		{
			return;
		}
		// check if the selected text is a folder
		// make sure we can find a file in the folder
		CString dir(str);
		TCHAR c = dir[dir.GetLength() - 1];
		if (c != ':'
			&& c != '\\'
			&& c != '/')
		{
			dir += '\\';
		}
		dir += '*';

		WIN32_FIND_DATA wfd;
		HANDLE hFind = FindFirstFile(dir, & wfd);
		if (INVALID_HANDLE_VALUE == hFind)
		{
			DWORD error = GetLastError();
			TRACE("FindFirstFile failed, last error = %d\n", error);
			CString s;
			if (ERROR_ACCESS_DENIED == error)
			{
				s.Format(IDS_DIRECTORY_ACCESS_DENIED, LPCTSTR(str));
			}
			else if (1 || ERROR_DIRECTORY == error
					|| ERROR_PATH_NOT_FOUND == error
					|| ERROR_INVALID_NAME == error
					|| ERROR_BAD_NETPATH)
			{
				s.Format(IDS_DIRECTORY_NOT_FOUND, LPCTSTR(str));
			}
			AfxMessageBox(s);
			// delete the string from combobox
			// delete also from the application list
			if (sel >= 0
				&& sel < sizeof m_RecentFolders / sizeof m_RecentFolders[0])
			{
				for (int i = sel; i < sizeof m_RecentFolders / sizeof m_RecentFolders[0] - 1; i++)
				{
					m_RecentFolders[i] = m_RecentFolders[i + 1];
				}
				m_RecentFolders[i].Empty();
			}
			pCb->DeleteString(sel);
			pCb->SetCurSel(-1); // no selection
			return;
		}
		else
		{
			TRACE("FindFirstFile success\n");
			FindClose(hFind);
			CWnd * pParent = GetParent();
			CWnd * pTmp = pParent->GetDlgItem(edt1);
			CString name;
			if (NULL == pTmp)
			{
				// new style dialog
				pTmp = pParent->GetDlgItem(cmb13);
			}
			if (NULL != pTmp
				&& ! m_bOpenFileDialog)
			{
				pTmp->GetWindowText(name);
			}

			pParent->SendMessage(CDM_SETCONTROLTEXT, edt1, LPARAM(LPCTSTR(str)));
			pParent->SendMessage(WM_COMMAND, IDOK, 0);

			if (m_bOpenFileDialog)
			{
				pParent->SendMessage(CDM_SETCONTROLTEXT, edt1, LPARAM(LPCTSTR("")));
			}
			else
			{
				pParent->SendMessage(CDM_SETCONTROLTEXT, edt1, LPARAM(LPCTSTR(name)));
			}
			if (NULL != pTmp)
			{
				pTmp->SetFocus();
			}
		}

	}
}

void CFileDialogWithHistory::OnInitDone()
{
	CComboBox * pCb = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_RECENT));
	if (NULL != pCb)
	{
		pCb->SetExtendedUI();
		CThisApp * pApp = GetApp();
		for (int i = 0; i < sizeof m_RecentFolders / sizeof m_RecentFolders[0]; i++)
		{
			CString s;
			s.Format("Dir%d", i);
			m_Profile.AddItem(_T("RecentOpenDirs"), s, m_RecentFolders[i]);
			m_RecentFolders[i].TrimLeft();
			m_RecentFolders[i].TrimRight();
			if ( ! m_RecentFolders[i].IsEmpty())
			{
				pCb->AddString(m_RecentFolders[i]);
			}
		}
	}
}

BOOL CFileDialogWithHistory::OnFileNameOK()
{
	CString sCurrDir;
	GetParent()->SendMessage(CDM_GETFOLDERPATH, MAX_PATH, LPARAM(sCurrDir.GetBuffer(MAX_PATH)));
	sCurrDir.ReleaseBuffer();
	TRACE("CFileDialogWithHistory::OnFileNameOK Folder Path=%s\n", sCurrDir);

	AddStringToHistory(sCurrDir, m_RecentFolders,
						sizeof m_RecentFolders / sizeof m_RecentFolders[0], false);

	m_Profile.FlushSection(_T("RecentOpenDirs"));
	return 0;
}

void CFileDialogWithHistory::OnFolderChange()
{
	CThisApp * pApp = GetApp();
	CString dir = GetFolderPath();
	if (dir.GetLength() > 1
		&& dir[dir.GetLength() - 1] == '\\')
	{
		dir.SetAt(dir.GetLength() - 1, 0);
	}
	CComboBox * pCb = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_RECENT));
	if (NULL != pCb)
	{
		for (int i = 0; i < sizeof m_RecentFolders / sizeof m_RecentFolders[0]; i++)
		{
			if ( ! m_RecentFolders[i].IsEmpty())
			{
				if (0 == m_RecentFolders[i].CompareNoCase(dir))
				{
					pCb->SetCurSel(i);
					break;
				}
			}
		}
	}
}

