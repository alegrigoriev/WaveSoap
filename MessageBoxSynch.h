// Copyright Alexander Grigoriev, 2003, All Rights Reserved
// MessageBoxSynch.h: message box which can be called from an arbitrary thread
// It then runs in the context of the main thread
// Make sure not to keep any critical sections entered when calling any of those
#pragma once
enum
{
	WM_USER_RUN_MSGBOX_SYNC = WM_USER+0xAA,
	WM_USER_RUN_MODAL_SYNC,
};

// BaseClass is usually CFrameWnd.
// This class is used as a base for CMainFrm class
template<class BaseClass>
class DialogProxyWnd : public BaseClass
{
public:
	template<typename W>
	DialogProxyWnd(W * pParent)
		: BaseClass(pParent)
	{
	}

	template<typename W1, typename W2>
	DialogProxyWnd(W1 * pParent, W2 Context)
		: BaseClass(pParent, Context)
	{
	}

	DialogProxyWnd()
		: BaseClass()
	{
	}

protected:
	DECLARE_MESSAGE_MAP();

private:
	LRESULT OnRunModalSync(WPARAM, LPARAM lparam)
	{
		CDialog * pDlg = reinterpret_cast<CDialog *>(lparam);
		return pDlg->DoModal();
	}
	LRESULT OnRunMsgBoxSync(WPARAM id, LPARAM lparam)
	{

		MSGBOXPARAMS * pMsg = reinterpret_cast<MSGBOXPARAMS *>(lparam);

		if (0 != id)
		{
			return AfxMessageBox(UINT(id), pMsg->dwStyle, pMsg->dwContextHelpId);
		}
		else
		{
			return AfxMessageBox(pMsg->lpszText, pMsg->dwStyle, pMsg->dwContextHelpId);
		}
	}
};

template<class BaseClass>
const AFX_MSGMAP* DialogProxyWnd<BaseClass>::GetMessageMap() const
{
	return & messageMap;
}

template<class BaseClass>
AFX_COMDAT const AFX_MSGMAP DialogProxyWnd<BaseClass>::messageMap =
{
	& BaseClass::messageMap, _messageEntries
};

template<class BaseClass>
AFX_COMDAT const AFX_MSGMAP_ENTRY DialogProxyWnd<BaseClass>::_messageEntries[] =
{
	ON_MESSAGE(WM_USER_RUN_MODAL_SYNC, OnRunModalSync)
	ON_MESSAGE(WM_USER_RUN_MSGBOX_SYNC, OnRunMsgBoxSync)
{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 }
};

INT_PTR RunModalDialogSync(CDialog & dlg);

INT_PTR MessageBoxSync(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = -1);

INT_PTR MessageBoxSync(UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = 0);
