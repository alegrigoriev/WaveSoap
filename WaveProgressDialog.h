#if !defined(AFX_WAVEPROGRESSDIALOG_H__B28B2582_2DC0_11D2_BE02_444553540000__INCLUDED_)
#define AFX_WAVEPROGRESSDIALOG_H__B28B2582_2DC0_11D2_BE02_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WaveProgressDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaveProgressDialog dialog

class CWaveProgressDialog : public CDialog
{
// Construction
public:
	CWaveProgressDialog(CWnd* pParent = NULL);   // standard constructor
	~CWaveProgressDialog();

// Dialog Data
	//{{AFX_DATA(CWaveProgressDialog)
	enum { IDD = IDD_DIALOG_WAVE_PROGRESS };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CWaveProc * pProc;
	class CWaveSoapSheet * pDlg;
	HANDLE hThread;
	int m_nPercent;
	int StartTickCount;
	BOOL m_bContinueProcess;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveProgressDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaveProgressDialog)
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVEPROGRESSDIALOG_H__B28B2582_2DC0_11D2_BE02_444553540000__INCLUDED_)
