// NumEdit.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"       // main symbols
#include "NumEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNumEdit
IMPLEMENT_DYNAMIC(CNumEdit, CEdit)

CNumEdit::CNumEdit()
//:iSelStart(0), iSelEnd(0)
{
}

CNumEdit::~CNumEdit()
{
}


BEGIN_MESSAGE_MAP(CNumEdit, CEdit)
	//{{AFX_MSG_MAP(CNumEdit)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// a function from dlgfloat.cpp
BOOL CNumEdit::SimpleFloatParse(LPCTSTR lpszText,
								double & d)
{
	ASSERT(lpszText != NULL);
	while (*lpszText == ' ' || *lpszText == '\t')
		lpszText++;

	TCHAR chFirst = lpszText[0];
	if (chFirst == '\0')    // empty string
	{
		d = DBL_MIN;
		return TRUE;
	}
	LPTSTR pLastChar;
	d = _tcstod(lpszText, & pLastChar);
	//if (d == 0.0 && chFirst != '0')
	if (d == 0.0 && chFirst != '0' && chFirst != '.')
		return FALSE;   // could not convert
	while (*pLastChar == ' ' || *pLastChar == '\t')
		pLastChar++;

	if (*pLastChar != '\0')
		return FALSE;   // not terminated properly

	return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CNumEdit message handlers
#if 0
void CNumEdit::OnUpdate()
{
	// TODO: Add your control notification handler code here
	// check if it contents a valid numeric string
	CString str;
	GetWindowText(str);
	double d;
	if(str != "." && str != "-" && str != "-."
		&&SimpleFloatParse(str, d) == FALSE)
	{
		CString str1(sLastValid);
		sLastValid = "";    // to prevent recursion
		SetWindowText(str1);
		SetSel(iSelStart, iSelEnd);
	}
	else
	{
		sLastValid = str;
	}
}
#endif

void CNumEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	// if it is alfanumeric char, filter all non-numeric characters
//    GetSel(iSelStart, iSelEnd);
	if (nChar >= ' ' && nChar <= 0xff)
	{
		if ((nChar < '0' || nChar > '9')
			&& nChar != '.'
			&& nChar != '-'
			&& nChar != 'e'
			&& nChar != 'E')
		{
//MessageBeep(MB_ICONEXCLAMATION); // don't annoy the user!
			return;
		}
	}
	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void DDX_Number(CDataExchange * pDX, UINT nID, double & num,
				LPCTSTR szDataName, LPCTSTR szUnits,
				double dLowLimit, double dHighLimit)
{
	CWnd * pWnd = CWnd::FromHandle(pDX->PrepareCtrl(nID));
	if ( ! pWnd->IsKindOf(RUNTIME_CLASS(CNumEdit)))
	{
		ASSERT(FALSE);
		return;
	}
	CNumEdit * pNumEdit = (CNumEdit *) pWnd;
	pNumEdit->ExchangeData(pDX, num, szDataName, szUnits,
							dLowLimit, dHighLimit);
}

void DDX_Number(CDataExchange* pDX, UINT nID, double & num,
				UINT uIdDataName, UINT uIdUnits,
				double dLowLimit, double dHighLimit)
{
	CWnd * pWnd = CWnd::FromHandle(pDX->PrepareCtrl(nID));
	if ( ! pWnd->IsKindOf(RUNTIME_CLASS(CNumEdit)))
	{
		ASSERT(FALSE);
		return;
	}
	CNumEdit * pNumEdit = (CNumEdit *) pWnd;
	pNumEdit->ExchangeData(pDX, num, uIdDataName, uIdUnits,
							dLowLimit, dHighLimit);
}

void CNumEdit::ExchangeData(CDataExchange* pDX, double & num,
							LPCTSTR szDataName, LPCTSTR szUnits,
							double dLowLimit, double dHighLimit)
{
	if (pDX->m_bSaveAndValidate)
	{
		UINT ID = GetDlgCtrlID( );
		(void)pDX->PrepareEditCtrl(ID);
		GetData(pDX, num, szDataName, szUnits, dLowLimit, dHighLimit);
		return;
	}
	else
	{
		SetData(num);
		return;
	}
}

void CNumEdit::ExchangeData(CDataExchange* pDX, double & num,
							UINT uIdDataName, UINT uIdUnits,
							double dLowLimit, double dHighLimit)
{
	if (pDX->m_bSaveAndValidate)
	{
		UINT ID = GetDlgCtrlID( );
		(void)pDX->PrepareEditCtrl(ID);
		LPCTSTR pszDataName = NULL;
		LPCTSTR pszUnits = NULL;
		CString str;
		if (uIdDataName != 0)
		{
			str.LoadString(uIdDataName);
			pszDataName = str;
		}
		if (uIdUnits != 0)
		{
			str.LoadString(uIdUnits);
			pszUnits = str;
		}
		GetData(pDX, num, pszDataName, pszUnits, dLowLimit, dHighLimit);
		return;
	}
	else
	{
		SetData(num);
		return;
	}
}

BOOL CNumEdit::GetData(CDataExchange * pDX, double & num,
						LPCTSTR szDataName, LPCTSTR szUnits,
						double dLowLimit, double dHighLimit)
{
	// if previous control is a static text,
	// get it
	CString WndText;
	if (szDataName != 0)
	{
		WndText = szDataName;
	}
	else
	{
		CWnd * pWnd = GetWindow(GW_HWNDPREV);
		if(pWnd)
		{
			pWnd->GetWindowText(WndText);
			// remove '&' characters
			int index;
			while ((index = WndText.Find('&')) != -1)
			{
				WndText = WndText.Left(index) + WndText.Mid(index+1);
			}
		}
	}
	CString str;
	CString ErrStr;
	GetWindowText(str);
	if (SimpleFloatParse(str, num) == FALSE)
	{
		AfxFormatString1(ErrStr, IDS_BAD_FLOAT_VALUE, WndText);
		AfxMessageBox(ErrStr, MB_OK | MB_ICONEXCLAMATION);
		if (NULL != pDX)
		{
			pDX->Fail();
		}
		return FALSE;
	}
	if (dHighLimit > dLowLimit
		&& (num < dLowLimit
			|| num > dHighLimit))
	{
		if(IS_MIN_DOUBLE(num))
		{
			AfxFormatString1(ErrStr, IDS_VALUE_REQUIRED, WndText);
		}
		else
		{
			TCHAR szBuffer[32];
			if (num < dLowLimit)
			{
				_stprintf(szBuffer, _T("%f"), dLowLimit);
				CString sBuf(szBuffer);
				if (szUnits != NULL)
				{
					sBuf = sBuf + " " + szUnits;
				}
				AfxFormatString2(ErrStr, IDS_VALUE_TOO_LOW, WndText, sBuf);
			}
			else
			{
				_stprintf(szBuffer, _T("%f"), dHighLimit);
				CString sBuf(szBuffer);
				if (szUnits != NULL)
				{
					sBuf = sBuf + " " + szUnits;
				}
				AfxFormatString2(ErrStr, IDS_VALUE_TOO_HIGH, WndText, sBuf);
			}
		}
		AfxMessageBox(ErrStr, MB_OK | MB_ICONEXCLAMATION);
		if (NULL != pDX)
		{
			pDX->Fail();
		}
		return FALSE;
	}
	return TRUE;
}

void CNumEdit::SetData(double num)
{
	if (IS_MIN_DOUBLE(num))
	{
		SetWindowText(_T(""));
	}
	else
	{
		TCHAR szBuffer[32];
		_stprintf(szBuffer, _T("%.10g"), num);
		SetWindowText(szBuffer);
	}
}

BOOL CNumEdit::IsValid()
{
	CString str;
	CString ErrStr;
	GetWindowText(str);
	double num;
	if (SimpleFloatParse(str, num) == FALSE
		|| IS_MIN_DOUBLE(num))
	{
		return FALSE;
	}
	return TRUE;
}
