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

IMPLEMENT_DYNAMIC(CResizableFileDialog, CFileDialog)

BEGIN_MESSAGE_MAP(CResizableFileDialog, CFileDialog)
	//{{AFX_MSG_MAP(CResizableFileDialog)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileDialogWithHistory

IMPLEMENT_DYNAMIC(CFileDialogWithHistory, CResizableFileDialog)

BEGIN_MESSAGE_MAP(CFileDialogWithHistory, CResizableFileDialog)
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
	CResizableFileDialog::OnInitDone();
	CComboBox * pCb = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_RECENT));
	if (NULL != pCb)
	{
		pCb->SetExtendedUI();

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
		&& (dir[dir.GetLength() - 1] == '\\'
			|| dir[dir.GetLength() - 1] == '/'))
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

CString CResizableFileDialog::GetNextPathName(POSITION& pos) const
{
	if (0 == (m_ofn.Flags & OFN_EXPLORER)
		|| (m_ofn.Flags & OFN_ALLOWMULTISELECT) == 0)
	{
		return CFileDialog::GetNextPathName(pos);
	}
	TCHAR const chDelimiter = '\0';

	LPTSTR lpsz = (LPTSTR)pos;
	if (lpsz == m_ofn.lpstrFile) // first time
	{
		// find char pos after first Delimiter
		while( *lpsz != '\0')
			lpsz = _tcsinc(lpsz);
		lpsz = _tcsinc(lpsz);

		// if single selection then return only selection
		if (*lpsz == 0)
		{
			pos = NULL;
			return m_ofn.lpstrFile;
		}
	}

	CString strPath = m_ofn.lpstrFile;

	LPTSTR lpszFileName = lpsz;
	CString strFileName = lpsz;

	// find char pos at next Delimiter
	while( *lpsz != '\0')
		lpsz = _tcsinc(lpsz);

	lpsz = _tcsinc(lpsz);
	if (*lpsz == '\0') // if double terminated then done
		pos = NULL;
	else
		pos = (POSITION)lpsz;

	// check if the filename is already absolute
	if (strFileName[0] == '/' || strFileName[0] == '\\'
		|| (strFileName.GetLength() > 1 && strFileName[1] == ':'))
	{
		TCHAR * pTitle;
		GetFullPathName(strFileName,MAX_PATH,strPath.GetBuffer(MAX_PATH), & pTitle);
		strPath.ReleaseBuffer();
		return strPath;
	}
	// only add '\\' if it is needed
	if (!strPath.IsEmpty())
	{
		// check for last back-slash or forward slash (handles DBCS)
		LPCTSTR lpsz = _tcsrchr(strPath, '\\');
		if (lpsz == NULL)
			lpsz = _tcsrchr(strPath, '/');
		// if it is also the last character, then we don't need an extra
		if (lpsz != NULL &&
			(lpsz - (LPCTSTR)strPath) == strPath.GetLength()-1)
		{
			ASSERT(*lpsz == '\\' || *lpsz == '/');
			return strPath + strFileName;
		}
	}
	return strPath + '\\' + strFileName;
}

size_t CResizableFileDialog::OpenfilenameSize()
{
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize = sizeof vi;
	GetVersionEx( & vi);

	if ((vi.dwPlatformId == VER_PLATFORM_WIN32_NT
			&& vi.dwMajorVersion >= 5)
		|| (vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
			&& (vi.dwMajorVersion > 4
				|| (vi.dwMajorVersion == 4
					&& vi.dwMinorVersion >= 90))))
	{
		return sizeof (OPENFILENAME);
	}
	else
	{
		return OPENFILENAME_SIZE_VERSION_400;
	}
}

void CResizableFileDialog::OnSize(UINT nType, int cx, int cy)
{
	TRACE("CResizableFileDialog::OnSize %d %d, hwdn=%x\n", cx, cy, m_hWnd);
	CFileDialog::OnSize(nType, cx, cy);
	// move dialog items
	// use _parent_ dialog size
	CRect r;
	GetParent()->GetClientRect( & r);
	if (-1 == m_PrevSize.cx)
	{
		m_PrevSize.cx = r.Width();
		m_PrevSize.cy = r.Height();
		return;
	}
	// only X coordinate or width needs change
	int dx = r.Width() - m_PrevSize.cx;
	m_PrevSize.cx = r.Width();
	m_PrevSize.cy = r.Height();
	if (0 == dx)
	{
		return;
	}

	if (0 != m_pResizeItemsCount)
	{
		HDWP hdwp = ::BeginDeferWindowPos(m_pResizeItemsCount);
		for (int i = 0; i < m_pResizeItemsCount && NULL != hdwp; i++)
		{
			HWND hWnd = ::GetDlgItem(GetSafeHwnd(), m_pResizeItems[i].Id);
			if (NULL == hWnd) continue;

			CRect cr;
			::GetWindowRect(hWnd, cr);
			ScreenToClient(cr);

			if (m_pResizeItems[i].flags & CenterHorizontally)
			{
				cr.right += (dx + (cx & 1)) >> 1;
				cr.left += (dx + (cx & 1)) >> 1;
			}
			else
			{
				if (m_pResizeItems[i].flags & (ExpandRight | MoveRight))
				{
					cr.right += dx;
				}
				if (m_pResizeItems[i].flags & MoveRight)
				{
					cr.left += dx;
				}
			}


			hdwp = ::DeferWindowPos(hdwp, hWnd, NULL, cr.left, cr.top,
									cr.Width(), cr.Height(),
									SWP_NOZORDER | SWP_NOOWNERZORDER// | SWP_NOACTIVATE | SWP_NOSENDCHANGING
									);
			if (0) TRACE("DeferWindowPos hwnd=%x dw=%d x=%d, y=%d returned %X\n",
						hWnd, dx, cr.left, cr.top, hdwp);
		}

		if (NULL != hdwp)
		{
			::EndDeferWindowPos(hdwp);
		}

	}

}

void CResizableFileDialog::OnInitDone()
{
	TRACE("CResizableFileDialog::OnInitDone\n");
	CFileDialog::OnInitDone();
}
