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

	void SavePeakInfo();
	BOOL OpenWaveFile();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWaveSoapFrontDoc)
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

struct PeakFileHeader
{
	DWORD dwSize;
	DWORD dwSignature;
	enum { pfhSignature = 'KPSW', pfhMaxVersion=1};
	DWORD dwVersion;
	FILETIME WaveFileTime;
	DWORD dwWaveFileSize;   // WAV file is less than 4G
	WAVEFORMATEX wfFormat;
	DWORD Granularity;      // number of WAV samples for each PeakFile value
	DWORD PeakInfoSize;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
