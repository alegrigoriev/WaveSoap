// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSupport.h

#ifndef WAVESUPPORT_H__
#define WAVESUPPORT_H__
#include <vector>
#include <mmreg.h>
#include <msacm.h>

enum {
	WaveFormatMatchBitsPerSample = 1,
	WaveFormatMatchBytesPerSec = 2,
	WaveFormatMatchCnannels = 4,
	WaveFormatMatchSampleRate = 8,
	WaveFormatMatch16Bits = 0x10,
	WaveFormatMatchFormatTag = 0x40,
	WaveFormatExactMatch = 0x100,
	WaveFormatAllFieldsMatch =
		WaveFormatExactMatch
		| WaveFormatMatchBytesPerSec
		| WaveFormatMatchCnannels
		| WaveFormatMatchSampleRate
		| WaveFormatMatchBitsPerSample
		| WaveFormatMatchFormatTag,
	WaveFormatMatchCompatibleFormats = 0x10000,
	WaveFormatExcludeFormats = 0x20000,
};

struct WaveFormatTagEx
{
	WaveFormatTagEx() {}
	WaveFormatTagEx(int tag, GUID const & guid)
		: Tag(tag), SubFormat(guid)
	{}
	WaveFormatTagEx(int tag)
		: Tag(tag)
	{
		ASSERT(tag != WAVE_FORMAT_EXTENSIBLE);
		memzero(SubFormat);
	}

	WaveFormatTagEx(WAVEFORMATEX const * pwf)
		: Tag(pwf->wFormatTag)
	{
		if (Tag != WAVE_FORMAT_EXTENSIBLE)
		{
			memzero(SubFormat);
		}
		else
		{
			SubFormat = ((PWAVEFORMATEXTENSIBLE)pwf)->SubFormat;
		}
	}

	int Tag;
	GUID SubFormat;
	bool operator ==(WaveFormatTagEx const & wfx) const
	{
		return Tag == wfx.Tag
				&& (Tag != WAVE_FORMAT_EXTENSIBLE || SubFormat == wfx.SubFormat);
	}

	bool operator ==(WAVEFORMATEX const * wfx) const
	{
		return Tag == wfx->wFormatTag
				&& (Tag != WAVE_FORMAT_EXTENSIBLE
					|| SubFormat == ((PWAVEFORMATEXTENSIBLE)wfx)->SubFormat);
	}

	WaveFormatTagEx & operator =(WAVEFORMATEX const * pwf)
	{
		Tag = pwf->wFormatTag;
		if (Tag != WAVE_FORMAT_EXTENSIBLE)
		{
			memzero(SubFormat);
		}
		else
		{
			SubFormat = ((PWAVEFORMATEXTENSIBLE)pwf)->SubFormat;
		}
		return *this;
	}
};

struct CWaveFormat
{
	WAVEFORMATEX * m_pWf;
	int m_AllocatedSize;
	CWaveFormat()
		: m_pWf(NULL),
		m_AllocatedSize(0)
	{
	}
	~CWaveFormat();
	WAVEFORMATEX * Allocate(unsigned ExtraSize, bool bCopy = false);
	CWaveFormat & operator =(WAVEFORMATEX const * pWf);
	CWaveFormat & operator =(CWaveFormat const & src)
	{
		return operator=(src.m_pWf);
	}
	CWaveFormat(CWaveFormat const & src)
		: m_pWf(NULL),
		m_AllocatedSize(0)
	{
		*this = src;
	}
	CWaveFormat(WAVEFORMATEX const * pWf)
		: m_pWf(NULL),
		m_AllocatedSize(0)
	{
		*this = pWf;
	}
	operator WAVEFORMATEX *() const
	{
		return m_pWf;
	}
	void InitCdAudioFormat();
	WAVEFORMATEX * Detach()
	{
		WAVEFORMATEX * pwf = m_pWf;
		m_pWf = NULL;
		return pwf;
	}

	WORD & FormatTag()
	{
		return m_pWf->wFormatTag;
	}

	void FormatTag(WaveFormatTagEx const & tag)
	{
		m_pWf->wFormatTag = tag.Tag;
		if (WAVE_FORMAT_EXTENSIBLE == tag.Tag
			&& m_AllocatedSize >= sizeof (WAVEFORMATEXTENSIBLE))
		{
			((PWAVEFORMATEXTENSIBLE)m_pWf)->SubFormat = tag.SubFormat;
		}
	}

	WORD FormatTag() const
	{
		return m_pWf->wFormatTag;
	}
	DWORD & SampleRate()
	{
		return m_pWf->nSamplesPerSec;
	}

	WORD BlockAlign() const
	{
		return m_pWf->nBlockAlign;
	}

	DWORD SampleRate() const
	{
		return m_pWf->nSamplesPerSec;
	}
	DWORD & BytesPerSec()
	{
		return m_pWf->nAvgBytesPerSec;
	}
	DWORD BytesPerSec() const
	{
		return m_pWf->nAvgBytesPerSec;
	}
	WORD & NumChannels()
	{
		return m_pWf->nChannels;
	}
	WORD NumChannels() const
	{
		return m_pWf->nChannels;
	}
	WORD & BitsPerSample()
	{
		return m_pWf->wBitsPerSample;
	}

	WORD BitsPerSample() const
	{
		return m_pWf->wBitsPerSample;
	}
	WORD & SampleSize()
	{
		return m_pWf->nBlockAlign;
	}

	WORD SampleSize() const
	{
		return m_pWf->nBlockAlign;
	}
	void * FormatExtension() const
	{
		return m_pWf + 1;
	}

	unsigned FormatSize() const
	{
		if (NULL == m_pWf)
		{
			return 0;
		}
		if (WAVE_FORMAT_PCM == m_pWf->wFormatTag)
		{
			return sizeof (PCMWAVEFORMAT);
		}
		return sizeof (WAVEFORMATEX) + m_pWf->cbSize;
	}

	void InitFormat(WORD wFormatTag, DWORD nSampleRate,
					WORD nNumChannels, WORD nBitsPerSample = 16, WORD Size = 0)
	{
		Allocate(Size, true);
		m_pWf->cbSize = Size;
		m_pWf->wFormatTag = wFormatTag;
		m_pWf->nSamplesPerSec = nSampleRate;
		m_pWf->nChannels = nNumChannels;
		m_pWf->wBitsPerSample = nBitsPerSample;
		m_pWf->nBlockAlign = nBitsPerSample * nNumChannels / 8;
		m_pWf->nAvgBytesPerSec = nSampleRate * nNumChannels * nBitsPerSample / 8;
	}
	int MatchFormat(WAVEFORMATEX const * pWf);
	//comparision operator is used for sorting
	bool operator <(CWaveFormat const & cmp) const
	{
		return SampleRate() < cmp.SampleRate()
				|| (SampleRate() == cmp.SampleRate()
					&& BytesPerSec() < cmp.BytesPerSec());
	}
	bool operator >(CWaveFormat const & cmp) const
	{
		return SampleRate() > cmp.SampleRate()
				|| (SampleRate() == cmp.SampleRate()
					&& BytesPerSec() > cmp.BytesPerSec());
	}
	bool operator ==(CWaveFormat const & cmp) const
	{
		return m_pWf->cbSize == cmp.m_pWf->cbSize
				&& 0 == memcmp(m_pWf, cmp.m_pWf,
								(WAVE_FORMAT_PCM == m_pWf->wFormatTag) ?
									sizeof (PCMWAVEFORMAT) : sizeof (WAVEFORMATEX) + m_pWf->cbSize);
	}
	static WAVEFORMATEX const CdAudioFormat;
};

inline bool operator ==(WAVEFORMATEX const & wf1, WAVEFORMATEX const & wf2)
{
	return wf1.cbSize == wf2.cbSize
			&& 0 == memcmp( & wf1, & wf2,
							(WAVE_FORMAT_PCM == wf1.wFormatTag) ?
								sizeof (PCMWAVEFORMAT) : sizeof (WAVEFORMATEX) + wf1.cbSize);
}

class CWaveDevice
{
public:
	CWaveDevice();
	virtual ~CWaveDevice();

	virtual BOOL IsOpen() = 0;
	BOOL AllocateBuffers(size_t size = 8192, int count = 4);
	int GetBuffer(char ** ppbuf, size_t * pSize, BOOL bWait = TRUE);
	BOOL ReturnBuffer(UINT hBuffer);    // return unused buffer
	BOOL ResetBuffers();
	BOOL WaitForQueueEmpty(DWORD timeout);
	void DeallocateBuffers();

	virtual MMRESULT Reset() = 0;

protected:
	virtual MMRESULT Unprepare(UINT index)= 0;
	struct BUFFER_STRUCT
	{
		WAVEHDR whd;
		char * pBuf;
		size_t size;
		DWORD dwFlags;
	};
	enum {BUF_USED = 1 };

	UINT m_id;
	CWaveFormat m_wfe;
	CRITICAL_SECTION cs;
	HANDLE hEvent;
	BUFFER_STRUCT * m_pBufs;
	UINT nBuffers;
};

class CWaveOut : public CWaveDevice
{
public:
	CWaveOut();
	virtual ~CWaveOut();

	virtual BOOL IsOpen() { return m_hwo != NULL; }
	MMRESULT Open(UINT id, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags = 0u);
	MMRESULT Open(LPCSTR szName, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags = 0u);
	MMRESULT Close();
	MMRESULT Reset();
	MMRESULT Pause();
	MMRESULT Resume();
	MMRESULT BreakLoop();
	DWORD GetPosition(UINT type=TIME_SAMPLES);

	MMRESULT Play(UINT hBuffer, size_t UsedSize, DWORD AuxFlags = 0);

private:
	HWAVEOUT m_hwo;
	virtual MMRESULT Unprepare(UINT index);
	static void CALLBACK waveOutProc(HWAVEOUT hwo,
									UINT uMsg,	DWORD dwInstance, DWORD dwParam1,
									DWORD dwParam2	);

	// the class doesn't allow assignment and copy
	CWaveOut(const CWaveOut &) {ASSERT(FALSE);}
	CWaveOut & operator=(const CWaveOut&) {ASSERT(FALSE); return *this;}
};

class CWaveIn : public CWaveDevice
{
public:
	CWaveIn();
	virtual ~CWaveIn();

	virtual BOOL IsOpen() { return m_hwi != NULL; }
	MMRESULT Open(UINT id, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags = 0u);
	MMRESULT Open(LPCSTR szName, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags = 0u);
	MMRESULT Close();
	MMRESULT Reset();
	MMRESULT Start();
	MMRESULT Stop();

	MMRESULT Record(UINT hBuffer, size_t UsedSize);

private:
	virtual MMRESULT Unprepare(UINT index);
	HWAVEIN m_hwi;
	DWORD GetPosition(UINT type=TIME_SAMPLES);
	static void CALLBACK waveInProc(HWAVEIN hwi,
									UINT uMsg,	DWORD dwInstance, DWORD dwParam1,
									DWORD dwParam2	);
	// the class doesn't allow assignment and copy
	CWaveIn(const CWaveIn &) {ASSERT(FALSE);}
	CWaveIn & operator=(const CWaveIn&) {ASSERT(FALSE); return *this;}
};

class CAudioMixer
{
	HMIXER m_hmix;
};

class CAudioCompressionManager
{
public:
	CAudioCompressionManager(CWaveFormat const & wf)
		: m_Wf(wf)
	{
	}

	struct FormatTagItem
	{
		FormatTagItem() : m_hadid(NULL) {}

		void SetData(WaveFormatTagEx const & pwf, CString const & name, HACMDRIVERID hadid)
		{
			Tag = pwf;
			Name = name;
			m_hadid = hadid;
		}

		WaveFormatTagEx Tag;
		HACMDRIVERID m_hadid;
		CString Name;
	};

	struct FormatItem
	{
		FormatItem() {}
		FormatItem(WAVEFORMATEX const * pwf, LPCTSTR name, int index)
			: Wf(pwf), Name(name), TagIndex(index) {}
		CWaveFormat Wf;
		CString Name;
		int TagIndex;
		bool operator <(FormatItem const & cmp) const
		{
			return Wf < cmp.Wf;
		}
		bool operator >(FormatItem const & cmp) const
		{
			return Wf > cmp.Wf;
		}
		bool operator ==(FormatItem const & cmp) const
		{
			return Wf == cmp.Wf;
		}
	};

	std::vector<FormatTagItem> m_FormatTags;
	std::vector<FormatItem> m_Formats;

	bool FillMultiFormatArray(unsigned nSelFrom, unsigned nSelTo, int Flags);
	bool FillFormatArray(unsigned nSel, int Flags)
	{
		return FillMultiFormatArray(nSel, nSel, Flags);
	}

	void FillFormatTagArray(WAVEFORMATEX const * pwf,
							WaveFormatTagEx const ListOfTags[],
							int NumTags, DWORD flags = 0);
	void FillWmaFormatTags();
	void FillMp3EncoderTags(DWORD Flags);
	void FillMp3FormatArray(DWORD Flags);
	void FillLameEncoderFormats();

	static CString GetFormatName(HACMDRIVER had, WAVEFORMATEX const * pWf);
	int FillFormatsCombo(CComboBox * pCombo, CWaveFormat & Wf,
						WaveFormatTagEx SelectedTag, int SelectedBitrate);
protected:
	static BOOL _stdcall FormatTestEnumCallback(
												HACMDRIVERID hadid, LPACMFORMATDETAILS pafd,
												DWORD dwInstance, DWORD fdwSupport);
	static BOOL _stdcall FormatTagEnumCallback(
												HACMDRIVERID hadid, LPACMFORMATTAGDETAILS paftd,
												DWORD dwInstance, DWORD fdwSupport);
	static BOOL _stdcall FormatEnumCallback(
											HACMDRIVERID hadid, LPACMFORMATDETAILS pafd,
											DWORD dwInstance, DWORD fdwSupport);
	// source wave format for comparing:
	CWaveFormat m_Wf;
};

class AudioStreamConvertor
{
	typedef void (CALLBACK * AcmStreamCallback)(HACMSTREAM , UINT uMsg,
												DWORD_PTR dwInstance, LPARAM , LPARAM );
	typedef void (CALLBACK * AcmStreamCallbackPtr)(HACMSTREAM , UINT uMsg,
													PVOID pInstance, LPARAM , LPARAM );
public:
	AudioStreamConvertor(HACMDRIVER drv = NULL);
	~AudioStreamConvertor();

	BOOL SuggestFormat(WAVEFORMATEX const * pWf1,
						WAVEFORMATEX * pWf2, size_t MaxFormat2Size, DWORD flags);

	BOOL QueryOpen(WAVEFORMATEX const * pWfSrc,
					WAVEFORMATEX const * pWfDst, DWORD flags = ACM_STREAMOPENF_NONREALTIME);

	BOOL Open(WAVEFORMATEX const * pWfSrc,
			WAVEFORMATEX const * pWfDst, DWORD flags = ACM_STREAMOPENF_NONREALTIME);

	BOOL Open(WAVEFORMATEX const * pWfSrc,
			WAVEFORMATEX const * pWfDst, HWND hCallbackWnd, DWORD_PTR dwInstance,
			DWORD flags = ACM_STREAMOPENF_NONREALTIME);

	BOOL Open(WAVEFORMATEX const * pWfSrc,
			WAVEFORMATEX const * pWfDst, HWND hCallbackWnd, PVOID pInstance,
			DWORD flags = ACM_STREAMOPENF_NONREALTIME)
	{
		return Open(pWfSrc, pWfDst, hCallbackWnd,
					reinterpret_cast<DWORD_PTR>(pInstance), flags);
	}

	BOOL Open(WAVEFORMATEX const * pWfSrc,
			WAVEFORMATEX const * pWfDst, HANDLE hEvent,
			DWORD flags = ACM_STREAMOPENF_NONREALTIME);

	BOOL Open(WAVEFORMATEX const * pWfSrc,
			WAVEFORMATEX const * pWfDst,
			AcmStreamCallback Callback,
			DWORD_PTR dwInstance, DWORD flags = ACM_STREAMOPENF_NONREALTIME);
	BOOL Open(WAVEFORMATEX const * pWfSrc,
			WAVEFORMATEX const * pWfDst,
			AcmStreamCallbackPtr Callback,
			PVOID pInstance, DWORD flags = ACM_STREAMOPENF_NONREALTIME)
	{
		return Open(pWfSrc, pWfDst,
					reinterpret_cast<AcmStreamCallback>(Callback),
					reinterpret_cast<DWORD_PTR>(pInstance), flags);
	}

	void Close();
	BOOL Reset(DWORD flags = 0);

	BOOL AllocateBuffers(size_t PreferredInBufSize = 0x10000,
						size_t PreferredOutBufSize = 0x10000);

	BOOL Convert(void const * pSrcBuf, size_t SrcBufSize, size_t * pSrcBufUsed,
				void* * ppDstBuf, size_t * pDstBufFilled,
				DWORD flags = ACM_STREAMCONVERTF_BLOCKALIGN);

protected:
	ACMSTREAMHEADER m_ash;
	HACMSTREAM m_acmStr;
	HACMDRIVER m_acmDrv;
	MMRESULT m_MmResult;

	DWORD m_SrcBufSize;
	DWORD m_DstBufSize;
};

WAVEFORMATEX * CopyWaveformat(const WAVEFORMATEX * src);
#endif // #ifndef WAVESUPPORT_H__
