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
#include "DirectFile.h"
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp:
// See WaveSoapFront.cpp for the implementation of this class
//
class COperationContext;
typedef BOOL (_stdcall * WAVE_OPERATION_PROC)(COperationContext *);
class COperationContext
{
public:
	COperationContext(class CWaveSoapFrontDoc * pDoc, DWORD Flags)
		: pDocument(pDoc),
		Flags(Flags),
		pNextChain(NULL),
		PercentCompleted(0)
	{
	}
	virtual ~COperationContext() {}
	virtual BOOL OperationProc() = 0;
	virtual BOOL Init() { return TRUE; }
	virtual BOOL DeInit() { return TRUE; }
	virtual CString GetStatusString() = 0;
	COperationContext * pNext;
	COperationContext * pPrev;
	COperationContext * pNextChain;
	class CWaveSoapFrontDoc * pDocument;
	DWORD Flags;
	int PercentCompleted;
};
enum {
	OperationContextClipboard = 1,  // clipboard operations are serialized
	OperationContextDiskIntensive = 2,  // only one disk intensive can be active
	OperationContextStopRequested = 4,    // cancel button pressed (set by the main thread)
	OperationContextStop = 8,  // the procedure is canceling
	OperationContextFinished = 0x10,    // the operation finished
	OperationContextInterventionRequired = 0x20,    // need to run a modal dialog
	OperationContextInitialized = 0x40,
	OperationContextCreatingUndo = 0x80,
	OperationContextNonCritical = 0x100,
};

class CWaveSoapFrontApp : public CWinApp
{
public:
	CWaveSoapFrontApp();
	BOOL QueueOperation(COperationContext * pContext);

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
	CString sTempDir;
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

	CDirectFile::CDirectFileCache * m_FileCache;
	CWaveSoapFrontDoc * m_pActiveDocument;
	COperationContext * m_pFirstOp;
	COperationContext * m_pLastOp;
	//CString m_CurrentStatusString;
	CWaveFile m_ClipboardFile;

	int m_DefaultPlaybackDevice;
	int m_NumPlaybackBuffers;
	size_t m_SizePlaybackBuffers;

	//{{AFX_MSG(CWaveSoapFrontApp)
	afx_msg void OnAppAbout();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	CWinThread m_Thread;
	unsigned _ThreadProc();
	bool m_RunThread;
	HANDLE m_hThreadEvent;
	CSimpleCriticalSection m_cs;
	static UINT AFX_CDECL ThreadProc(PVOID arg)
	{
		return ((CWaveSoapFrontApp *) arg)->_ThreadProc();
	}
};

inline CWaveSoapFrontApp * GetApp()
{
	return (CWaveSoapFrontApp *) AfxGetApp();
}

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
