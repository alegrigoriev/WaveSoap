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
		m_Flags(Flags),
		pNextChain(NULL),
		PercentCompleted(0)
	{
	}
	virtual ~COperationContext()
	{
		if (pNextChain)
		{
			COperationContext * pContext = pNextChain;
			pNextChain = NULL;
			delete pContext;
		}
	}
	virtual BOOL OperationProc() = 0;
	virtual BOOL Init() { return TRUE; }
	virtual BOOL DeInit() { return TRUE; }
	virtual void Retire();
	virtual void PostRetire();
	virtual CString GetStatusString() = 0;
	COperationContext * pNext;
	COperationContext * pPrev;
	COperationContext * pNextChain;
	class CWaveSoapFrontDoc * pDocument;
	DWORD m_Flags;
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
	OperationContextDontKeepAfterRetire = 0x200,
	OperationContextDontAdjustPriority = 0x400,
};

class CWaveSoapFrontStatusBar : public CStatusBar
{
public:
	CWaveSoapFrontStatusBar() {}
	~CWaveSoapFrontStatusBar() {}
	virtual int OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
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
	BOOL m_bReadOnly;
	BOOL m_bDirectMode;

	BOOL m_bUseCountrySpecificNumberAndTime;
	TCHAR m_TimeSeparator;
	TCHAR m_DecimalPoint;
	TCHAR m_ThousandSeparator;

	//{{AFX_MSG(CWaveSoapFrontApp)
	afx_msg void OnAppAbout();
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
// long to string, thousands separated by commas
CString LtoaCS(long num);
enum
{
	TimeToHhMmSs_NeedsMs = 1,
	TimeToHhMmSs_NeedsHhMm = 2,
};
CString TimeToHhMmSs(unsigned TimeMs, int Flags = TimeToHhMmSs_NeedsMs);
void SetStatusString(CCmdUI* pCmdUI, const CString & string,
					LPCTSTR MaxString = NULL, BOOL bForceSize = FALSE);
/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
