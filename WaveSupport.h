// WaveSupport.h

#ifndef WAVESUPPORT_H__
#define WAVESUPPORT_H__

enum {
	WaveFormatMatchBytesPerSec = 1,
	WaveFormatMatchCnahhels = 2,
	WaveFormatMatchSampleRate = 4,
	WaveFormatMatch16Bits = 8,
	WaveFormatMatchFormatTag = 0x20,
	WaveFormatExactMatch = 0x100,
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
	void Allocate(int ExtraSize, bool bCopy = false);
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

	WORD FormatTag() const
	{
		return m_pWf->wFormatTag;
	}
	DWORD & SampleRate()
	{
		return m_pWf->nSamplesPerSec;
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
	void InitFormat(WORD wFormatTag, DWORD nSampleRate,
					WORD nNumChannels, WORD nBitsPerSample = 16)
	{
		m_pWf->wFormatTag = wFormatTag;
		m_pWf->nSamplesPerSec = nSampleRate;
		m_pWf->nChannels = nNumChannels;
		m_pWf->wBitsPerSample = nBitsPerSample;
		m_pWf->nBlockAlign = nBitsPerSample * nNumChannels / 8;
		m_pWf->nAvgBytesPerSec = nSampleRate * nNumChannels * nBitsPerSample / 8;
	}
	int MatchFormat(WAVEFORMATEX const * pWf);
};

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
	WAVEFORMATEX * m_pwfe;
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
WAVEFORMATEX * CopyWaveformat(const WAVEFORMATEX * src);
#endif // #ifndef WAVESUPPORT_H__
