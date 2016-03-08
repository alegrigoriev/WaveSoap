// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
typedef class CWaveSoapFrontApp CWaveSoapApp;
typedef class CWaveSoapFrontApp CThisApp;

#include <afxwinappex.h>
#include "ApplicationProfile.h"
#include "WaveFile.h"
#include "LocaleUtilities.h"
#include "WaveSupport.h"
#include "ContextWorkerThread.h"
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp:
// See WaveSoapFront.cpp for the implementation of this class
//
class CWaveSoapFrontStatusBar : public CStatusBar
{
public:
	CWaveSoapFrontStatusBar() {}
	~CWaveSoapFrontStatusBar() {}
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	//{{AFX_MSG(CWaveSoapFrontStatusBar)
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};

enum
{
	OpenDocumentDirectMode = 2,
	OpenDocumentReadOnly = 4,
	OpenDocumentDefaultMode = OpenDocumentDirectMode | OpenDocumentReadOnly,
	OpenDocumentModeFlagsMask = OpenDocumentDirectMode | OpenDocumentReadOnly,
	OpenDocumentCreateNewWithParameters = 8, // NAME is actually NewFileParameters *
	OpenDocumentCreateNewFromCWaveFile = 0x10,// use m_pFile from NewFileParameters
	DocumentFlagOpenOnly = 0x20, // this template is to open files only
	OpenDocumentMp3File = 0x1000,
	OpenDocumentWmaFile = 0x2000,
	OpenDocumentRawFile = 0x4000,
	OpenDocumentAviFile = 0x8000,
	OpenRawFileMsbFirst = 0x10000,
	OpenDocumentDShowFile = 0x20000,
	OpenDocumentNonWavFile = OpenDocumentMp3File | OpenRawFileMsbFirst
							| OpenDocumentWmaFile | OpenDocumentRawFile | OpenDocumentAviFile | OpenDocumentDShowFile,
};

struct NewFileParameters
{
	CWaveFormat pWf;
	LPCTSTR m_pInitialName;
	NUMBER_OF_SAMPLES m_InitialSamples;
	DWORD m_FileTypeFlags;
	CWaveFile * m_pFile;

	NewFileParameters(WAVEFORMATEX const * pf,
					NUMBER_OF_SAMPLES nSamples = 0)
		: m_pInitialName(NULL)
		, m_InitialSamples(nSamples)
		, m_FileTypeFlags(0)
		, m_pFile(NULL)
		, pWf(pf)
	{
	}
};

class CWaveSoapDocTemplate : public CMultiDocTemplate
{
	typedef CMultiDocTemplate BaseClass;
public:
	CWaveSoapDocTemplate( UINT nIDResource, UINT nIDStringResource,
						CRuntimeClass* pDocClass,
						CRuntimeClass* pFrameClass,
						CRuntimeClass* pViewClass,
						DWORD OpenFlags)
		:BaseClass(nIDResource, pDocClass, pFrameClass, pViewClass),
		m_OpenDocumentFlags(OpenFlags)
	{
		if ( ! m_strDocStrings.LoadString(nIDStringResource))
		{
			m_strDocStrings.LoadString(m_nIDResource);
		}
	}
	~CWaveSoapDocTemplate() {}
	virtual CDocument* OpenDocumentFile( LPCTSTR lpszPathName, int flags = 1
											//BOOL bMakeVisible = TRUE
										);

	DWORD GetDocumentTypeFlags() const
	{
		return m_OpenDocumentFlags;
	}

	virtual void OnIdle();
	void BroadcastUpdate(UINT lHint);
	BOOL IsAnyDocumentModified();
	BOOL CanSaveAnyDocument();
	void SaveAll();

	BOOL OnIdleForDocument(CDocument * pDoc);	// returns TRUE if pDoc was found

protected:
	Confidence MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch);

	DWORD m_OpenDocumentFlags;
};

class COperationContext;

class CWaveSoapFrontApp : public CWinAppEx,
	public DirectFileParameters,
	public LocaleParameters
{
	typedef CWinAppEx BaseClass;
public:
	CWaveSoapFrontApp();
	void QueueOperation(COperationContext * pContext)
	{
		m_Thread.QueueOperation(pContext);
	}
	void LoadStdProfileSettings(UINT nMaxMRU);
	void OnActivateDocument(class CWaveSoapFrontDoc * pDocument, BOOL bActivate);

	BOOL CanOpenWindowsMedia() const { return NULL != m_hWMVCORE_DLL_Handle; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontApp)
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL IsIdleMessage(MSG* pMsg);  // checks for special messages
	//}}AFX_VIRTUAL

// Implementation
	CApplicationProfile Profile;
	CString m_CurrentDir;
	int m_OpenFileDialogFilter;
	int m_DefaultOpenMode;
	enum
	{
		DefaultOpenReadOnly = 0,
		DefaultOpenDirect = 1,
		DefaultOpenBuffered = 2, };

	bool m_bOpenChildMaximized;
	bool m_bShowToolbar;
	bool m_bShowStatusBar;
	bool m_bShowNewFormatDialogWhenShiftOnly;
	bool m_bAllow4GbWavFile;      // File proppage
	// display colors:
	union
	{
		struct
		{
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
			DWORD m_MarkerColor;
			DWORD m_SelectedMarkerColor;
			DWORD m_RegionColor;
			DWORD m_SelectedRegionColor;
		};
		DWORD AppColors[16];
	};

	int m_SoundTimeFormat;

	CDirectFileCacheProxy * m_FileCache;
	CWaveSoapFrontDoc * m_pActiveDocument;

	CString m_CurrentStatusString;
	CWaveSoapFrontDoc * m_pLastStatusDocument;
	CSimpleCriticalSection m_StatusStringLock;

	void GetStatusStringAndDoc(CString & str, CWaveSoapFrontDoc ** ppDoc);
	void SetStatusStringAndDoc(const CString & str, CWaveSoapFrontDoc * pDoc);

	CWaveFile m_ClipboardFile;
	unsigned m_NewFileSamplesPerSecond;
	NUMBER_OF_CHANNELS m_NewFileChannels;
	int m_NewFileSampleType;
	LONG m_NewFileLength;   // seconds

	class CWaveSoapDocTemplate * m_pAllTypesTemplate;
	class CWaveSoapDocTemplate * m_pMP3TypeTemplate;
	class CWaveSoapDocTemplate * m_pWavTypeTemplate;
	class CWaveSoapDocTemplate * m_pWmaTypeTemplate;
	class CWaveSoapDocTemplate * m_pDShowTypeTemplate;
	class CWaveSoapDocTemplate * m_pAviTypeTemplate;
	class CWaveSoapDocTemplate * m_pAllWmTypeTemplate;
	class CWaveSoapDocTemplate * m_pRawTypeTemplate;

	INT m_DefaultPlaybackDevice;
	int m_NumPlaybackBuffers;
	ULONG m_SizePlaybackBuffers;

	INT m_DefaultRecordDevice;
	int m_NumRecordBuffers;
	ULONG m_SizeRecordBuffers;

	BOOL m_bReadOnly;
	BOOL m_bDirectMode;

	BOOL m_bUseCountrySpecificNumberAndTime;

	BOOL m_DontShowMediaPlayerWarning;

	BOOL m_bSnapMouseSelectionToMax;
	int m_LastPrefsPropertyPageSelected;

	int m_SpectrumSectionWidth;
	int m_FftBandsOrder;
	int m_FftWindowType;

	int m_PasteResampleMode; // Edit/Paste to different rate: 0 - resample, 1- paste as is
	int m_DefaultPasteMode; // 0 - replace selection, 1 - paste just selection length, 2 - insert

	BOOL m_UseFadeInOut;
	int m_FadeInOutLengthMs;
	int m_FadeInEnvelope;
	int m_FadeOutEnvelope;

	CString m_UserKey;

	CDocument* OpenDocumentFile(LPCTSTR lpszPathName, int flags);
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName);

	CPalette * GetPalette();
	CPalette   m_Palette;
	void CreatePalette();
	BOOL GetMessageString(UINT nID, CString& rMessage);

	void BroadcastUpdate(UINT lHint = 0);
	BOOL IsAnyDocumentModified();
	BOOL CanSaveAnyDocument();

	CString m_NotEnoughMemoryMsg;

	//{{AFX_MSG(CWaveSoapFrontApp)
	afx_msg void OnFileNew();
	afx_msg void OnAppAbout();
	afx_msg void OnEditPasteNew();
	afx_msg void OnUpdateEditPasteNew(CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	afx_msg void OnToolsCdgrab();
	afx_msg void OnFileSaveAll();
	afx_msg void OnUpdateFileSaveAll(CCmdUI* pCmdUI);
	afx_msg void OnToolsOptions();
	afx_msg void OnFileBatchconversion();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CContextWorkerThread m_Thread;

	HINSTANCE m_hWMVCORE_DLL_Handle;
public:
};

// this class brings a document frame temporary to the top,
// then restores it back
class CDocumentPopup
{
public:
	CDocumentPopup(CDocument * pDoc);
	~CDocumentPopup();
protected:
	CFrameWnd * m_pPopupFrame;
	CFrameWnd * m_pFrameAbove;
};

inline CThisApp * GetApp()
{
	return static_cast<CThisApp *>(AfxGetApp());
}

enum FftWindowType
{
	WindowTypeSquaredSine = 0,
	WindowTypeHalfSine = 1,
	WindowTypeHamming = 2,
	WindowTypeNuttall = 3,
};

enum SiblingNotifyCode
{
	ChannelHeightsChanged,
	FftBandsChanged,
	FftWindowChanged,

	HorizontalScaleChanged,
	HorizontalOriginChanged,     // NotifyViewsData.HorizontalScroll
	HorizontalExtentChanged,     // NotifyViewsData.HorizontalScroll
	HorizontalScrollPixels,   // lParam is pointer to int pixels (signed)
	VerticalScaleAndOffsetChanged,
	VerticalScaleIndexChanged,
	AmplitudeOffsetChanged,
	AmplitudeScrollTo,

	FftScrollTo,
	FftOffsetChanged,
	FftVerticalScaleChanged,

	SpectrumSectionDbOriginChange,	// set current origin, sent by the view
	SpectrumSectionDbScaleChange,	// set current scale in dB per pixel, double, is sent by the view to itself and to the ruler
	SpectrumSectionScaleChange,		// change current scale, double 1 to 256
	SpectrumSectionHorScrollTo,		// scroll to position, unclipped, sent by the ruler

	ShowChannelPopupMenu,		// finds out channel number and whether it's minimized, and modifies the maximize/minimize command
};

struct NotifyViewsData
{
	int code;
	union {
		struct
		{
			double HorizontalScale;
			double FirstSampleInView;
			double TotalSamplesInView;
			double TotalSamplesInExtent;    // adjusted for the reserved empty space after the end
		} HorizontalScroll;
		struct
		{
			double NewScale;
			double NewOffset;
			double MaxRange;
		} Amplitude;
		struct
		{
		} Fft;
		struct
		{
			UINT	NormalMenuId;
			UINT	MinimizedMenuId;
			POINT p;		// menu position in screen coordinates
		} PopupMenu;
	};
};

struct ChannelHeight
{
	// bottom, clip_bottom excludes the separator line.
	int top;
	int bottom;               // top+NominalChannelHeight or +MinimizedChannelHeight
	int clip_top;             // == top
	int clip_bottom;          // top+NominalChannelHeight+OddPixel or +MinimizedChannelHeight+OddPixel
	bool minimized;
};
struct NotifyChannelHeightsData
{
	int NumChannels;
	int NominalChannelHeight;      // non-minimized channel height, not including the separator line. This is used for scroll calculation
	ChannelHeight ch[MAX_NUMBER_OF_CHANNELS];
};

#define NotifySiblingViews(NotifyCode, data) NotifySiblingViews_(this, NotifyCode, data)

inline LRESULT NotifySiblingViews_(CWnd *wnd, int NotifyCode, PVOID data)
{
	ASSERT(wnd->GetParent() != NULL);
	return wnd->GetParent()->SendMessage(UWM_NOTIFY_VIEWS, NotifyCode, (LPARAM)data);
}

// long to string, thousands separated by commas
void SetStatusString(CCmdUI* pCmdUI, const CString & string,
					LPCTSTR MaxString = NULL, BOOL bForceSize = FALSE);
void SetStatusString(CCmdUI* pCmdUI, UINT id, // string resource ID
					LPCTSTR MaxString = NULL, BOOL bForceSize = FALSE);

BOOL VerifyCreateDirectory(LPCTSTR pszPath);
void NotEnoughMemoryMessageBox();
void NotEnoughDiskSpaceMessageBox();
void NotEnoughUndoSpaceMessageBox();
void FileCreationErrorMessageBox(LPCTSTR name);

BOOL CanExpandWaveFile(const CWaveFile & WaveFile, NUMBER_OF_SAMPLES NumOfSamplesToAdd);
BOOL CanExpandWaveFileDlg(const CWaveFile & WaveFile, NUMBER_OF_SAMPLES NumOfSamplesToAdd);
BOOL CanAllocateWaveFileSamples(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples);
BOOL CanAllocateWaveFileSamplesDlg(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples);
CString GetSelectionText(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Chan,
						NUMBER_OF_CHANNELS nChannels, BOOL bLockChannels,
						int nSamplesPerSec, int TimeFormat);

CString LoadCString(UINT id);
#define _AfxAppendFilterSuffix w_AfxAppendFilterSuffix
void _AfxAppendFilterSuffix(CString& filter, OPENFILENAME& ofn,
							CDocTemplate* pTemplate, CString* pstrDefaultExt);

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
