// WaveSoapFront.h : main header file for the WAVESOAPFRONT application
//

#if !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "ApplicationProfile.h"
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp:
// See WaveSoapFront.cpp for the implementation of this class
//

class CWaveSoapFrontApp : public CWinApp
{
public:
	CWaveSoapFrontApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontApp)
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	double GetProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
							double Default, double MinVal, double MaxVal);
	void WriteProfileDouble(LPCTSTR Section, LPCTSTR ValueName, double value);
	CApplicationProfile Profile;
	CString m_CurrentDir;
	// display colors:
	DWORD m_WaveBackground;
	DWORD m_SelectedWaveBackground;
	DWORD m_WaveColor;
	DWORD m_6dBLineColor;
	DWORD m_ZeroLineColor;
	DWORD m_ChannelSeparatorColor;
	DWORD m_InterpolatedColor;
	DWORD m_SelectedWaveColor;
	DWORD m_Selected6dBLineColor;
	DWORD m_SelectedZeroLineColor;
	DWORD m_SelectedChannelSeparatorColor;
	DWORD m_SelectedInterpolatedColor;


	//{{AFX_MSG(CWaveSoapFrontApp)
	afx_msg void OnAppAbout();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef CWaveSoapFrontApp CWaveSoapApp;
/////////////////////////////////////////////////////////////////////////////
class CWaveSoapDocTemplate : public CMultiDocTemplate
{
public:
	CWaveSoapDocTemplate( UINT nIDResource, CRuntimeClass* pDocClass,
						CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass )
		:CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
	{
	}
	~CWaveSoapDocTemplate() {}
	virtual CDocument* OpenDocumentFile( LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE );

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
