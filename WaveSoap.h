// WaveSoap.h : main header file for the WAVESOAP application
//

#if !defined(AFX_WAVESOAP_H__5799DAA4_2CB1_11D2_BE02_444553540000__INCLUDED_)
#define AFX_WAVESOAP_H__5799DAA4_2CB1_11D2_BE02_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "WaveSoapSheet.h"
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapApp:
// See WaveSoap.cpp for the implementation of this class
//

class CWaveSoapApp : public CWinApp
{
public:
	CWaveSoapApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapApp)
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL
	CWaveSoapSheet * pDlg;
// Implementation
	void ProcessSoundFile();
	double GetProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
							double Default, double MinVal, double MaxVal);
	void WriteProfileDouble(LPCTSTR Section, LPCTSTR ValueName, double value);

	//{{AFX_MSG(CWaveSoapApp)
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAP_H__5799DAA4_2CB1_11D2_BE02_444553540000__INCLUDED_)
