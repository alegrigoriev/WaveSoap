// WaveSoapFrontDoc.h : interface of the CWaveSoapFrontDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct WavePeak
{
	__int16 low;
	__int16 high;
};

// Active document have highest priority for disk-intensive operations.
// while it is executing a disk-intensive command,
// such operations with non-active documents are suspended.
// To implement it, App object starts a thread for such operations.
//
class CWaveSoapFrontDoc : public CDocument
{
protected: // create from serialization only
	CWaveSoapFrontDoc();
	DECLARE_DYNCREATE(CWaveSoapFrontDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontDoc)
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
	CString szWaveFilename;
	CString szWaveTitle;
	CWaveFile m_WavFile;


	//WAVEFORMATEX WavFileFormat;
	CString szPeakFilename;
	BY_HANDLE_FILE_INFORMATION WavFileInfo;
	WavePeak * m_pPeaks;
	size_t m_WavePeakSize;
	DWORD m_SizeOfWaveData;
	int m_PeakDataGranularity;
	__int16 * m_pWaveBuffer;
	size_t m_WaveBufferSize;
	BOOL AllocateWaveBuffer(size_t size = 0x100000);    // 1M default
	void FreeWaveBuffer();

	LONG m_CaretPosition;
	LONG m_SelectionStart;
	LONG m_SelectionEnd;
	int m_SelectedChannel; // 0, 1, 2
	bool m_TimeSelectionMode;
	void SetSelection(int begin, int end, int channel, int caret);
	void SoundChanged(int begin, int end);
	enum {UpdateSelectionChanged = 1,
		UpdateSelectionModeChanged,
		UpdateSoundChanged,
	};

// Implementation
public:
	virtual ~CWaveSoapFrontDoc();
	void LoadPeakFile();
	void BuildPeakInfo();
	BOOL AllocatePeakInfo();
	int CalculatePeakInfoSize() const
	{
		size_t WavGranule = m_PeakDataGranularity * m_WavFile.m_pWf->nChannels * sizeof(__int16);
		size_t Granules = (m_SizeOfWaveData + WavGranule - 1) / WavGranule;
		return Granules * m_WavFile.m_pWf->nChannels;
	}
	int WaveFileSamples() const
	{
		if (m_WavFile.m_pWf != NULL)
		{
			return m_SizeOfWaveData /
					(m_WavFile.m_pWf->nChannels * m_WavFile.m_pWf->wBitsPerSample / 8);
		}
		else
		{
			return 0;
		}
	}
	int WaveChannels() const
	{
		return m_WavFile.m_pWf->nChannels;
	}

	void SavePeakInfo();
	BOOL OpenWaveFile();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	bool volatile m_OperationInProgress;
	bool volatile m_StopOperation;
	bool m_bReadOnly;
	bool m_bUndoAvailable;
	COperationContext * m_pCurrentContext;
	COperationContext * m_pQueuedOperation;
	COperationContext * m_pUndoList;
	COperationContext * m_pRedoList;
protected:
	HANDLE m_hThreadEvent;
	bool volatile m_bRunThread;
	CWinThread m_Thread;
	UINT _ThreadProc();
	static UINT AFX_CDECL ThreadProc(LPVOID arg)
	{
		CWaveSoapFrontDoc * pDoc = DYNAMIC_DOWNCAST(CWaveSoapFrontDoc, (CObject *)arg);
		if (NULL == pDoc)
		{
			return -1;
		}
		return pDoc->_ThreadProc();
	}


// Generated message map functions
protected:
	BOOL QueueOperation(COperationContext * pContext);
	// save the selected area to the permanent or temporary file
	void DoCopy(LONG Start, LONG End, LONG Channel, LPCTSTR FileName);
	void DoPaste(LONG Start, LONG End, LONG Channel, LPCTSTR FileName);
	void DoCut(LONG Start, LONG End, LONG Channel, LPCTSTR FileName);
	void DoDelete(LONG Start, LONG End, LONG Channel);

	//{{AFX_MSG(CWaveSoapFrontDoc)
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditStop();
	afx_msg void OnUpdateEditStop(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

class CSelectionUpdateInfo : public CObject
{
public:
	CSelectionUpdateInfo() {}
	~CSelectionUpdateInfo() {}
	int SelBegin;
	int SelEnd;
	int SelChannel;
	int CaretPos;
};

class CSoundUpdateInfo : public CObject
{
public:
	CSoundUpdateInfo() {}
	~CSoundUpdateInfo() {}
	int Begin;
	int End;
	int Length;
};

#pragma pack(push, 2)
struct PeakFileHeader
{
	DWORD dwSize;
	DWORD dwSignature;
	enum { pfhSignature = 'KPSW', pfhMaxVersion=1};
	DWORD dwVersion;
	FILETIME WaveFileTime;
	DWORD dwWaveFileSize;   // WAV file is less than 4G
	DWORD Granularity;      // number of WAV samples for each PeakFile value
	DWORD PeakInfoSize;
	WAVEFORMATEX wfFormat;
};
#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
