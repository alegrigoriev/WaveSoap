// Copyright Alexander Grigoriev, 2003, All Rights Reserved
// MessageBoxSynch.cpp: message box which can be called from an arbitrary thread
// It then runs in the context of the main thread
// Make sure not to keep any critical sections entered when calling any of those
#include "stdafx.h"

#include "MessageBoxSynch.h"
#include "SimpleCriticalSection.h"

static CSimpleCriticalSection DialogProxyCriticalSection;

INT_PTR RunModalDialogSync(CDialog & dlg)
{
	CWnd * pWnd = AfxGetApp()->m_pMainWnd;
	if (NULL == pWnd
		|| NULL == pWnd->m_hWnd)
	{
		return -1;
	}
	if (GetCurrentThreadId() == ::GetWindowThreadProcessId(pWnd->m_hWnd, NULL))
	{
		return dlg.DoModal();
	}

	CSimpleCriticalSectionLock lock(DialogProxyCriticalSection);

	return ::SendMessage(pWnd->m_hWnd, WM_USER_RUN_MODAL_SYNC, 0,
						reinterpret_cast<LPARAM>( & dlg));
}

INT_PTR MessageBoxSync(LPCTSTR lpszText, UINT nType, UINT nIDHelp)
{
	CWnd * pWnd = AfxGetApp()->m_pMainWnd;
	if (NULL == pWnd
		|| NULL == pWnd->m_hWnd)
	{
		return -1;
	}
	if (GetCurrentThreadId() == ::GetWindowThreadProcessId(pWnd->m_hWnd, NULL))
	{
		return AfxMessageBox(lpszText, nType, nIDHelp);
	}

	CSimpleCriticalSectionLock lock(DialogProxyCriticalSection);

	MSGBOXPARAMS Params;
	Params.dwStyle = nType;
	Params.lpszText = lpszText;
	Params.dwContextHelpId = nIDHelp;

	return ::SendMessage(pWnd->m_hWnd, WM_USER_RUN_MSGBOX_SYNC, 0,
						reinterpret_cast<LPARAM>( & Params));
}

INT_PTR MessageBoxSync(UINT nIDPrompt, UINT nType, UINT nIDHelp)
{
	CWnd * pWnd = AfxGetApp()->m_pMainWnd;
	if (NULL == pWnd
		|| NULL == pWnd->m_hWnd)
	{
		return -1;
	}
	if (GetCurrentThreadId() == ::GetWindowThreadProcessId(pWnd->m_hWnd, NULL))
	{
		return AfxMessageBox(nIDPrompt, nType, nIDHelp);
	}

	CSimpleCriticalSectionLock lock(DialogProxyCriticalSection);

	MSGBOXPARAMS Params;
	Params.dwStyle = nType;
	Params.lpszText = 0;
	Params.dwContextHelpId = nIDHelp;

	return ::SendMessage(pWnd->m_hWnd, WM_USER_RUN_MSGBOX_SYNC, nIDPrompt,
						reinterpret_cast<LPARAM>( & Params));
}

