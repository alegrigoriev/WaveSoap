// WaveSupport.cpp

#include "stdafx.h"
#include <mmsystem.h>
#include "WaveSupport.h"
#include <algorithm>
#include <functional>
/////////////////////////////////
// CWaveDevice stuff
/////////////////////////////////

CWaveDevice::CWaveDevice()
	: m_pBufs(NULL), m_id(WAVE_MAPPER-1),
	nBuffers(0),
	hEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
{
	InitializeCriticalSection( & cs);
}

CWaveDevice::~CWaveDevice()
{
	if (hEvent != NULL) CloseHandle(hEvent);
	DeleteCriticalSection( & cs);
}

BOOL CWaveDevice::AllocateBuffers(size_t size, int count)
{
	ASSERT(count != 0);
	ASSERT(size != 0);
	ASSERT(this);

	// Reset(); // made in DeallocateBuffers()
	DeallocateBuffers();
	m_pBufs = new BUFFER_STRUCT[count];
	if (NULL == m_pBufs)
		return FALSE;
	memset(m_pBufs, 0, count * sizeof BUFFER_STRUCT);
	nBuffers = count;
	for (int i = 0; i < count; i++)
	{
		m_pBufs[i].size = size;
		m_pBufs[i].pBuf = new char[size];
		if (NULL == m_pBufs[i].pBuf)
		{
			DeallocateBuffers();
			return FALSE;
		}
	}
	return TRUE;
}

void CWaveDevice::DeallocateBuffers()
{
	ASSERT(this);
	Reset();
	ResetBuffers();
	for (unsigned i = 0; i < nBuffers; i++)
	{
		//VERIFY(MMSYSERR_NOERROR == Unprepare(i + 1));
		// buffers are unprepared in ResetBuffers
		delete[] m_pBufs[i].pBuf;
	}
	nBuffers = 0;
	delete m_pBufs;
	m_pBufs = NULL;
}

int CWaveDevice::GetBuffer(char ** ppbuf, size_t * pSize, BOOL bWait)
{
	ASSERT(this && ppbuf && pSize);

	if (NULL == m_pBufs)
		return 0;

	do {
		if (! IsOpen())
			return 0;
		// find any unused buffer
		::EnterCriticalSection(& cs);
		for (unsigned i = 0; i < nBuffers; i++)
		{
			if (0 == (m_pBufs[i].dwFlags & BUF_USED))
			{
				Unprepare(i + 1);
				m_pBufs[i].dwFlags |= BUF_USED;
				::LeaveCriticalSection(&cs);
				* ppbuf = m_pBufs[i].pBuf;
				* pSize = m_pBufs[i].size;
				return i + 1;
			}
		}
		::LeaveCriticalSection(&cs);
		if ( ! bWait)
		{
			return -1;
		}
		::WaitForSingleObject(hEvent, 500);
	}while(1);
}

BOOL CWaveDevice::ReturnBuffer(UINT hBuffer)
{
	ASSERT(this != NULL);
	ASSERT(hBuffer > 0 && hBuffer <= nBuffers);

	m_pBufs[hBuffer - 1].dwFlags &= ~BUF_USED;
	return TRUE;
}

BOOL CWaveDevice::ResetBuffers()
{
	ASSERT(this != NULL);
	if ( ! IsOpen())
	{
		return FALSE;
	}
	EnterCriticalSection( & cs);
	for (unsigned i = 0; i < nBuffers; i++)
	{
		if (m_pBufs[i].whd.dwFlags & WHDR_INQUEUE)
		{
			LeaveCriticalSection( & cs);
			return FALSE;
		}
		Unprepare(i + 1);
		m_pBufs[i].dwFlags = 0;
	}
	LeaveCriticalSection( & cs);
	return TRUE;
}

BOOL CWaveDevice::WaitForQueueEmpty(DWORD timeout)
{
	ASSERT(this != NULL);
	DWORD StartTime = GetTickCount();
	do {
		DWORD flag = 0;
		for (unsigned i = 0; i < nBuffers; i++)
		{
			flag |= m_pBufs[i].whd.dwFlags;
		}

		if (0 == (flag & WHDR_INQUEUE))
		{
			return TRUE;
		}
		if (0 == timeout)
		{
			return FALSE;
		}

		WaitForSingleObject(hEvent, timeout);
	} while (GetTickCount() - StartTime < timeout);
	return FALSE;
}

/////////////////////////////////
// CWaveOut stuff
/////////////////////////////////

CWaveOut::CWaveOut():
	m_hwo(NULL)
{
}

CWaveOut::~CWaveOut()
{
	Close();
	DeallocateBuffers();
}

MMRESULT CWaveOut::Open(UINT id, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags)
{
	ASSERT(pwfe != NULL);
	ASSERT((dwAuxFlags & ~(WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY | WAVE_MAPPED)) == 0);

	if (dwAuxFlags & WAVE_FORMAT_QUERY)
	{
		HWAVEOUT hwo;
		return waveOutOpen( & hwo, id, const_cast<WAVEFORMATEX*>(pwfe), NULL, NULL,
							dwAuxFlags & (WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY | WAVE_MAPPED));
	}

	ASSERT(this != NULL);
	Close();

	MMRESULT err = waveOutOpen( & m_hwo, id, pwfe, ULONG(waveOutProc), (DWORD)this,
								CALLBACK_FUNCTION | (dwAuxFlags & (WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY | WAVE_MAPPED)));

	if (MMSYSERR_NOERROR == err)
	{
		m_wfe = pwfe;
		m_id = id;
	}

	return err;
}

MMRESULT CWaveOut::Open(LPCSTR szName, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags)
{
	//ASSERT(this != NULL);
	//ASSERT(pwfe != NULL);
	// member variables are not used until internal Open call
	// find the device by name
	UINT nDevCount = waveOutGetNumDevs();
	WAVEOUTCAPSA woc;
	for (UINT id = 0; id < nDevCount; id++)
	{
		if (MMSYSERR_NOERROR == waveOutGetDevCapsA(id, & woc, sizeof woc))
		{
			if (0 == _stricmp(szName, woc.szPname))
			{
				m_id = id;
				return Open(id, pwfe, dwAuxFlags);
			}
		}
	}
	return MMSYSERR_NODRIVER;
}

MMRESULT CWaveOut::Close()
{
	ASSERT(this);
	if (! IsOpen())
		return MMSYSERR_INVALHANDLE;
	Reset();
	ResetBuffers();
	MMRESULT err = waveOutClose(m_hwo);
	if (err != MMSYSERR_NOERROR)
		return err;
	m_hwo = NULL;
	m_id = WAVE_MAPPER - 1;

	return MMSYSERR_NOERROR;
}

MMRESULT CWaveOut::Play(UINT hBuffer, size_t UsedSize, DWORD AuxFlags)
{
	ASSERT(this != NULL);
	ASSERT(m_hwo != NULL);
	ASSERT(0 == (AuxFlags & ~(WHDR_BEGINLOOP | WHDR_ENDLOOP)));
	ASSERT(hBuffer >= 1 && hBuffer <= nBuffers);
	ASSERT(m_pBufs != NULL);
	ASSERT(UsedSize > 0 && UsedSize <= m_pBufs[hBuffer - 1].size);

	m_pBufs[hBuffer - 1].whd.lpData = m_pBufs[hBuffer - 1].pBuf;
	m_pBufs[hBuffer - 1].whd.dwBufferLength = UsedSize;
	m_pBufs[hBuffer - 1].whd.dwBytesRecorded = UsedSize;
	m_pBufs[hBuffer - 1].whd.dwUser = (DWORD) & m_pBufs[hBuffer - 1];
	m_pBufs[hBuffer - 1].whd.dwFlags = 0;

	waveOutPrepareHeader(m_hwo, & m_pBufs[hBuffer - 1].whd,
						sizeof m_pBufs[hBuffer - 1].whd);
	m_pBufs[hBuffer - 1].whd.dwFlags |= AuxFlags;

	return waveOutWrite(m_hwo, & m_pBufs[hBuffer - 1].whd,
						sizeof m_pBufs[hBuffer - 1].whd);
}

MMRESULT CWaveOut::Reset()
{
	ASSERT(this != NULL);

	if ( ! IsOpen())
		return MMSYSERR_NOERROR;
	return waveOutReset(m_hwo);
}

MMRESULT CWaveOut::BreakLoop()
{
	ASSERT(this != NULL);

	if ( ! IsOpen())
		return MMSYSERR_NOERROR;
	return waveOutBreakLoop(m_hwo);
}

MMRESULT CWaveOut::Pause()
{
	ASSERT(this != NULL);

	if ( ! IsOpen())
		return MMSYSERR_INVALHANDLE;
	return waveOutPause(m_hwo);
}

MMRESULT CWaveOut::Resume()
{
	ASSERT(this != NULL);

	if ( ! IsOpen())
		return MMSYSERR_INVALHANDLE;
	return waveOutRestart(m_hwo);
}

DWORD CWaveOut::GetPosition(UINT type)
{
	ASSERT(this != NULL);
	ASSERT(TIME_SAMPLES == type || TIME_BYTES == type
			|| TIME_MS == type || TIME_TICKS == type);

	MMTIME mmt;
	mmt.wType = type;
	if ( ! IsOpen()
		|| waveOutGetPosition(m_hwo, & mmt, sizeof mmt) != MMSYSERR_NOERROR)
		return DWORD(-1);
	return mmt.u.ms;
}

MMRESULT CWaveOut::Unprepare(UINT hBuffer)
{
	ASSERT(this != NULL);
	ASSERT(IsOpen());
	ASSERT(hBuffer > 0 && hBuffer <= nBuffers);

	return waveOutUnprepareHeader(m_hwo, & m_pBufs[hBuffer - 1].whd,
								sizeof m_pBufs[0].whd);
}

void CALLBACK CWaveOut::waveOutProc(HWAVEOUT hwo,
									UINT uMsg,	DWORD dwInstance, DWORD dwParam1,
									DWORD dwParam2	)
{
	CWaveOut * pWo = (CWaveOut *) dwInstance;
	// can't use ASSERT in wave out callback
#ifdef _DEBUG
	if (pWo == NULL || pWo->m_hwo != hwo)
	{
		TRACE("Wrong dwInstance in waveOutProc");
	}
#endif
	switch (uMsg)
	{
	case WOM_OPEN:
		TRACE("WOM_OPEN\r\n");
		break;
	case WOM_DONE:
	{
		WAVEHDR * pWhdr = (WAVEHDR *) dwParam1;
		BUFFER_STRUCT * pBufStruct = (BUFFER_STRUCT *)pWhdr->dwUser;
		pBufStruct->dwFlags &= ~BUF_USED;
		SetEvent(pWo->hEvent);
	}
		break;
	case WOM_CLOSE:
		TRACE("WOM_CLOSE\r\n");
		break;
	default:
		TRACE("Wrong Audio Callback Msg\r\n");
		break;
	}
}

/////////////////////////////////
// CWaveIn stuff
/////////////////////////////////

CWaveIn::CWaveIn():
	m_hwi(NULL)
{
}

CWaveIn::~CWaveIn()
{
	DeallocateBuffers();
	Close();
}

MMRESULT CWaveIn::Open(UINT id, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags)
{
	ASSERT(this != NULL);
	ASSERT(pwfe != NULL);
	ASSERT((dwAuxFlags & ~(WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY | WAVE_MAPPED)) == 0);

	if (dwAuxFlags & WAVE_FORMAT_QUERY)
	{
		HWAVEIN hwi;
		return waveInOpen( & hwi, id, const_cast<WAVEFORMATEX*>(pwfe), NULL, NULL,
							dwAuxFlags & (WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY | WAVE_MAPPED));
	}

	ASSERT(this != NULL);
	Close();

	m_wfe = pwfe;

	MMRESULT err = waveInOpen( & m_hwi, id, m_wfe, ULONG(waveInProc), (DWORD)this,
								CALLBACK_FUNCTION | (dwAuxFlags & (WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY | WAVE_MAPPED)));

	if (MMSYSERR_NOERROR == err)
	{
		m_id = id;
	}

	return err;
}

MMRESULT CWaveIn::Open(LPCSTR szName, const WAVEFORMATEX * pwfe, DWORD dwAuxFlags)
{
	// find the device by name
	UINT nDevCount = waveInGetNumDevs();
	WAVEINCAPSA wic;
	for (UINT id = 0; id < nDevCount; id++)
	{
		if (MMSYSERR_NOERROR == waveInGetDevCapsA(id, & wic, sizeof wic))
		{
			if (0 == _stricmp(szName, wic.szPname))
			{
				m_id = id;
				return Open(id, pwfe, dwAuxFlags);
			}
		}
	}
	return MMSYSERR_NODRIVER;
}

MMRESULT CWaveIn::Close()
{
	ASSERT(this);
	if (! IsOpen())
		return MMSYSERR_INVALHANDLE;
	Reset();
	ResetBuffers();
	MMRESULT err = waveInClose(m_hwi);
	if (err != MMSYSERR_NOERROR)
		return err;
	m_hwi = NULL;
	m_id = WAVE_MAPPER - 1;

	return MMSYSERR_NOERROR;
}

MMRESULT CWaveIn::Record(UINT hBuffer, size_t UsedSize)
{
	ASSERT(this != NULL);
	ASSERT(m_hwi != NULL);

	ASSERT(hBuffer >= 1 && hBuffer <= nBuffers);
	ASSERT(m_pBufs != NULL);
	ASSERT(UsedSize > 0 && UsedSize <= m_pBufs[hBuffer - 1].size);

	m_pBufs[hBuffer - 1].whd.lpData = m_pBufs[hBuffer - 1].pBuf;
	m_pBufs[hBuffer - 1].whd.dwBufferLength = UsedSize;
	m_pBufs[hBuffer - 1].whd.dwBytesRecorded = UsedSize;
	m_pBufs[hBuffer - 1].whd.dwUser = (DWORD) & m_pBufs[hBuffer - 1];
	m_pBufs[hBuffer - 1].whd.dwFlags = 0;
	return waveInAddBuffer(m_hwi, & m_pBufs[hBuffer - 1].whd,
							sizeof m_pBufs[hBuffer - 1].whd);
}

MMRESULT CWaveIn::Reset()
{
	ASSERT(this != NULL);

	if ( ! IsOpen())
		return MMSYSERR_NOERROR;
	return waveInReset(m_hwi);
}

MMRESULT CWaveIn::Stop()
{
	ASSERT(this != NULL);

	if ( ! CWaveIn::IsOpen())
		return MMSYSERR_INVALHANDLE;
	return waveInStop(m_hwi);
}

MMRESULT CWaveIn::Start()
{
	ASSERT(this != NULL);

	if ( ! IsOpen())
		return MMSYSERR_INVALHANDLE;
	return waveInStart(m_hwi);
}

DWORD CWaveIn::GetPosition(UINT type)
{
	ASSERT(this != NULL);
	ASSERT(TIME_SAMPLES == type || TIME_BYTES == type
			|| TIME_MS == type || TIME_TICKS == type);

	MMTIME mmt;
	mmt.wType = type;
	if (! IsOpen()
		|| waveInGetPosition(m_hwi, & mmt, sizeof mmt) != MMSYSERR_NOERROR)
		return DWORD(-1);
	return mmt.u.ms;
}

MMRESULT CWaveIn::Unprepare(UINT hBuffer)
{
	ASSERT(this != NULL);
	ASSERT(IsOpen());
	ASSERT(hBuffer > 0 && hBuffer <= nBuffers);

	return waveInUnprepareHeader(m_hwi, & m_pBufs[hBuffer - 1].whd,
								sizeof m_pBufs[0].whd);
}

void CALLBACK CWaveIn::waveInProc(HWAVEIN hwi,
								UINT uMsg,	DWORD dwInstance, DWORD dwParam1,
								DWORD dwParam2	)
{
}

WAVEFORMATEX * CopyWaveformat(const WAVEFORMATEX * src)
{
	int size = src->cbSize + sizeof (WAVEFORMATEX);
	if (WAVE_FORMAT_PCM == src->wFormatTag)
	{
		size = sizeof (WAVEFORMATEX);
	}
	WAVEFORMATEX * dst = ( WAVEFORMATEX *) new char[size];
	if (NULL == dst)
	{
		return NULL;
	}
	memcpy(dst, src, size);
	if (WAVE_FORMAT_PCM == src->wFormatTag)
	{
		dst->cbSize = 0;
	}
	return dst;
}

CWaveFormat::~CWaveFormat()
{
	delete[] (char*) m_pWf;
}

void CWaveFormat::Allocate(int ExtraSize, bool bCopy)
{
	int SizeToAllocate = ExtraSize + sizeof (WAVEFORMATEX);
	if (m_AllocatedSize >= SizeToAllocate)
	{
		return;
	}
	void * NewBuf = new char[SizeToAllocate];
	if (NULL == NewBuf)
	{
		return;
	}
	if (m_pWf)
	{
		if (bCopy) memcpy(NewBuf, m_pWf, m_AllocatedSize);
		delete[] (char*) m_pWf;
	}
	m_pWf = (WAVEFORMATEX *)NewBuf;
	m_AllocatedSize = SizeToAllocate;
}

void CWaveFormat::InitCdAudioFormat()
{
	Allocate(0);
	m_pWf->cbSize = 0;
	m_pWf->nSamplesPerSec = 44100;
	m_pWf->wFormatTag = WAVE_FORMAT_PCM;
	m_pWf->wBitsPerSample = 16;
	m_pWf->nChannels = 2;
	m_pWf->nBlockAlign = 4;
	m_pWf->nAvgBytesPerSec = 176400;
}

CWaveFormat & CWaveFormat::operator =(WAVEFORMATEX const * pWf)
{
	if (pWf != m_pWf)
	{
		if (WAVE_FORMAT_PCM == pWf->wFormatTag)
		{
			Allocate(0);
			memcpy(m_pWf, pWf, sizeof (PCMWAVEFORMAT));
			m_pWf->cbSize = 0;
		}
		else
		{
			Allocate(pWf->cbSize);
			memcpy(m_pWf, pWf, pWf->cbSize + sizeof WAVEFORMATEX);
		}
	}
	return *this;
}

int CWaveFormat::MatchFormat(WAVEFORMATEX const * pwf)
{
	int match = 0;
	if (pwf->wFormatTag == FormatTag())
	{
		if (WAVE_FORMAT_PCM == pwf->wFormatTag)
		{
			if (0 == memcmp(pwf, m_pWf, sizeof (PCMWAVEFORMAT)))
			{
				// exact match found
				if (16 == pwf->wBitsPerSample)
				{
					return WaveFormatAllFieldsMatch | WaveFormatMatch16Bits;
				}
				else
				{
					return WaveFormatAllFieldsMatch;
				}
			}
		}
		else
		{
			if (pwf->cbSize == m_pWf->cbSize
				&& 0 == memcmp(pwf, m_pWf, pwf->cbSize + sizeof (WAVEFORMATEX)))
			{
				// exact match found
				return WaveFormatAllFieldsMatch;
			}
		}
		match |= WaveFormatMatchFormatTag;
	}
	if (pwf->nSamplesPerSec == SampleRate())
	{
		match |= WaveFormatMatchSampleRate;
	}
	if (pwf->nChannels == NumChannels())
	{
		match |= WaveFormatMatchCnannels;
	}
	if (pwf->nAvgBytesPerSec == BytesPerSec())
	{
		match |= WaveFormatMatchBytesPerSec;
	}
	if (pwf->wBitsPerSample == BitsPerSample())
	{
		match |= WaveFormatMatchBitsPerSample;
	}
	return match;
}

struct FormatTagEnumStruct
{
	CAudioCompressionManager * pAcm;
	WAVEFORMATEX * pWf;
	WaveFormatTagEx const * pListOfTags;
	int NumTags;
	DWORD flags;
};

struct FormatEnumCallbackStruct
{
	CAudioCompressionManager * pAcm;
	CWaveFormat FormatToMatch;
	WaveFormatTagEx m_Tag;
	WaveFormatTagEx const * m_pTagsToCompare; // include or exclude
	int m_NumTagsToCompare;
	//CString m_FormatTagName;
	int flags;
	BOOL FormatFound;
};

// if can convert to any format of the tag, add the tag to the table
BOOL _stdcall CAudioCompressionManager::FormatTestEnumCallback(
																HACMDRIVERID hadid, LPACMFORMATDETAILS pafd,
																DWORD dwInstance, DWORD fdwSupport)
{
	FormatEnumCallbackStruct * pFcs = (FormatEnumCallbackStruct *) dwInstance;

	TRACE("FormatTestEnumCallback: format=%s, tag=%d, \n", pafd->szFormat, pafd->dwFormatTag);

	// check if the format should be excluded
	if (0 != pFcs->m_NumTagsToCompare
		&& NULL != pFcs->m_pTagsToCompare
		&& (pFcs->flags & WaveFormatExcludeFormats))
	{
		for (int i = 0; i < pFcs->m_NumTagsToCompare; i++)
		{
			if (pFcs->m_pTagsToCompare[i] == pafd->pwfx)
			{
				// format tag should be excluded from enumerated, but keep trying
				return TRUE;
			}
		}
	}

	int match = pFcs->FormatToMatch.MatchFormat(pafd->pwfx);
	// include only formats with the same tag.
	// If MatchCompatibleFormats selected,
	if (pFcs->m_Tag == pafd->pwfx)
	{
		if (pFcs->flags & WaveFormatMatchCompatibleFormats)
		{
			// include all non-PCM formats or
			// PCM formats with the same sample rate and number of channels,
			// and the same number of bits or 16 bits
			if (pFcs->m_Tag.Tag == WAVE_FORMAT_PCM
				&& ((match & (WaveFormatMatchSampleRate | WaveFormatMatchCnannels))
					!= (WaveFormatMatchSampleRate | WaveFormatMatchCnannels)
					|| (0 == (match & (WaveFormatMatchBitsPerSample | WaveFormatMatch16Bits)))))
			{
				return TRUE;    // enumerate more
			}
		}
		else
		{
			// check which flags are specified
			if (pFcs->flags & ~match
				& (WaveFormatMatchCnannels
					| WaveFormatMatchBitsPerSample
					| WaveFormatMatchSampleRate))
			{
				return TRUE;    // enumerate more
			}
		}
		pFcs->m_Tag = pafd->pwfx;
		pFcs->FormatFound = TRUE;
		return FALSE;   // no more enumeration required
	}
	return TRUE;    // enumerate more
}

BOOL _stdcall CAudioCompressionManager::FormatTagEnumCallback(
															HACMDRIVERID hadid, LPACMFORMATTAGDETAILS paftd,
															DWORD dwInstance, DWORD fdwSupport)
{
	FormatTagEnumStruct * pfts = (FormatTagEnumStruct *) dwInstance;
	CAudioCompressionManager * pAcm = pfts->pAcm;

	if (pfts->NumTags != 0
		&& pfts->pListOfTags != NULL)
	{
		int Include = (pfts->flags & WaveFormatExcludeFormats);

		for (int i = 0; i < pfts->NumTags; i++)
		{
			if (paftd->dwFormatTag == pfts->pListOfTags[i].Tag)
			{
				if (Include)
				{
					if (pfts->pListOfTags[i].Tag != WAVE_FORMAT_EXTENSIBLE)
					{
						// format tag should be excluded from enumerated
						return TRUE;
					}
				}
				Include = TRUE;
				break;
			}
		}
		if ( ! Include)
		{
			return TRUE;
		}
	}

	FormatEnumCallbackStruct fcs;

	fcs.FormatFound = FALSE;
	fcs.pAcm = pAcm;
	fcs.m_Tag.Tag = paftd->dwFormatTag;
	fcs.m_pTagsToCompare = pfts->pListOfTags;
	fcs.m_NumTagsToCompare = pfts->NumTags;
	fcs.flags = pfts->flags;
	fcs.FormatToMatch = pfts->pWf;

	CWaveFormat pwfx;
	pwfx.Allocate(0xFFF0);
	ACMFORMATDETAILS afd;

	TRACE("FormatTagEnum: name=%s, driverID=%x, tag=%d, formats=%d, max size=%d\n", paftd->szFormatTag,
		hadid, paftd->dwFormatTag, paftd->cStandardFormats, paftd->cbFormatSize);

	pwfx.InitFormat(paftd->dwFormatTag,
					pfts->pWf->nSamplesPerSec, pfts->pWf->nChannels);

	memzero(afd);
	afd.cbStruct = sizeof afd;
	afd.cbwfx = pwfx.m_AllocatedSize;
	afd.dwFormatTag = paftd->dwFormatTag;
	afd.pwfx = pwfx;

	HACMDRIVER had = NULL;

	if (paftd->dwFormatTag == WAVE_FORMAT_PCM)
	{
		fcs.FormatFound = TRUE;
	}
	else
	{
		if (MMSYSERR_NOERROR == acmDriverOpen(&had, hadid, 0))
		{
			DWORD flags = ACM_FORMATENUMF_WFORMATTAG;

			if (pfts->flags & WaveFormatMatchCompatibleFormats)
			{
				flags |= ACM_FORMATENUMF_NCHANNELS | ACM_FORMATENUMF_NSAMPLESPERSEC;
			}
			if (pfts->flags & WaveFormatMatchCnannels)
			{
				flags |= ACM_FORMATENUMF_NCHANNELS;
			}
			if (pfts->flags & WaveFormatMatchSampleRate)
			{
				flags |= ACM_FORMATENUMF_NSAMPLESPERSEC;
			}

			int res = acmFormatEnum(had, & afd, FormatTestEnumCallback, DWORD(& fcs), flags);
			TRACE("acmFormatEnum returned %x\n", res);
			if ( ! fcs.FormatFound)
			{
				pwfx.InitFormat(WAVE_FORMAT_PCM,
								pfts->pWf->nSamplesPerSec, pfts->pWf->nChannels);
				res = acmFormatEnum(had, & afd, FormatTestEnumCallback,
									DWORD(& fcs), ACM_FORMATENUMF_CONVERT);
				TRACE("acmFormatEnum returned %x\n", res);
			}
			if ( ! fcs.FormatFound)
			{
				// try acmFormatSuggest
				pwfx.InitFormat(WAVE_FORMAT_PCM,
								pfts->pWf->nSamplesPerSec, pfts->pWf->nChannels);
				res = acmFormatSuggest(had, pfts->pWf, pwfx, pwfx.m_AllocatedSize,
										ACM_FORMATSUGGESTF_WFORMATTAG);
				if (MMSYSERR_NOERROR == res)
				{
					TRACE("No format enumerated, but acmSuggestFormat returned one\n");
					fcs.FormatFound = TRUE;
				}
			}
			acmDriverClose(had, 0);
		}
	}

	if (fcs.FormatFound)
	{
		int nIndex = pAcm->m_FormatTags.size();
		pAcm->m_FormatTags.resize(nIndex + 1);
		pAcm->m_FormatTags[nIndex].SetData(fcs.m_Tag, paftd->szFormatTag, hadid);
	}
	return TRUE;
}

void CAudioCompressionManager::FillFormatTagArray
	(WAVEFORMATEX * pwf, WaveFormatTagEx const ListOfTags[],
		int NumTags, DWORD flags)
{
	m_FormatTags.clear();

	FormatTagEnumStruct fts;
	fts.pAcm = this;
	fts.pWf = pwf;
	fts.flags = flags;
	fts.NumTags = NumTags;
	fts.pListOfTags = ListOfTags;

	// enum all formats
	ACMFORMATTAGDETAILS atd;
	memzero(atd);
	atd.cbStruct = sizeof atd;
	atd.dwFormatTag = WAVE_FORMAT_UNKNOWN;

	acmFormatTagEnum(NULL, & atd, FormatTagEnumCallback, DWORD( & fts), 0);

}

// enumerates all formats for the tag
BOOL _stdcall CAudioCompressionManager::FormatEnumCallback(
															HACMDRIVERID hadid, LPACMFORMATDETAILS pafd,
															DWORD dwInstance, DWORD fdwSupport)
{
	FormatEnumCallbackStruct * pfcs = (FormatEnumCallbackStruct *) dwInstance;

	CAudioCompressionManager * pAcm = pfcs->pAcm;
	TRACE("FormatEnum: format=%s, tag=%d\n", pafd->szFormat, pafd->dwFormatTag);

	int match = pfcs->FormatToMatch.MatchFormat(pafd->pwfx);
	// include only formats with the same tag.
	// If MatchCompatibleFormats selected,
	if (pfcs->m_Tag == pafd->pwfx)
	{
		if (pfcs->flags & WaveFormatMatchCompatibleFormats)
		{
			// include all non-PCM formats or
			// PCM formats with the same sample rate and number of channels,
			// and the same number of bits or 16 bits
			if (pfcs->m_Tag.Tag == WAVE_FORMAT_PCM
				&& ((match & (WaveFormatMatchSampleRate | WaveFormatMatchCnannels))
					!= (WaveFormatMatchSampleRate | WaveFormatMatchCnannels)
					|| (0 == (match & (WaveFormatMatchBitsPerSample | WaveFormatMatch16Bits)))))
			{
				return TRUE;
			}
		}
		else
		{
			// check which flags are specified
			if (pfcs->flags & ~match
				& (WaveFormatMatchCnannels
					| WaveFormatMatchBitsPerSample
					| WaveFormatMatchSampleRate))
			{
				return TRUE;
			}
		}
		FormatItem * pfmt = pAcm->m_Formats.insert(pAcm->m_Formats.end());
		pfmt->Wf = pafd->pwfx;
		pfmt->Name = pafd->szFormat;
	}
	return TRUE;
}

void CAudioCompressionManager::FillFormatArray(unsigned SelFormat, int Flags)
{
	m_Formats.clear();
	if (SelFormat >= m_FormatTags.size())
	{
		return;
	}

	DWORD dwFormatTag = m_FormatTags[SelFormat].Tag.Tag;
	CWaveFormat pwfx;
	pwfx.Allocate(0xFFF0, true);

	ACMFORMATDETAILS afd;

	pwfx.InitFormat(dwFormatTag, m_Wf.SampleRate(), m_Wf.NumChannels(),
					m_Wf.BitsPerSample());

	memzero(afd);
	afd.cbStruct = sizeof afd;
	afd.cbwfx = pwfx.m_AllocatedSize;
	afd.dwFormatTag = dwFormatTag;
	afd.pwfx = pwfx;

	CWaveFormat FormatToMatch(m_Wf);
	FormatToMatch.FormatTag() = dwFormatTag;

	HACMDRIVER had = NULL;
	if (MMSYSERR_NOERROR == acmDriverOpen(&had, m_FormatTags[SelFormat].m_hadid, 0))
	{
		DWORD EnumFlags = ACM_FORMATENUMF_WFORMATTAG;

		if (Flags & WaveFormatMatchCompatibleFormats)
		{
			EnumFlags |= ACM_FORMATENUMF_NCHANNELS | ACM_FORMATENUMF_NSAMPLESPERSEC;
		}
		if (Flags & WaveFormatMatchCnannels)
		{
			EnumFlags |= ACM_FORMATENUMF_NCHANNELS;
		}
		if (Flags & WaveFormatMatchSampleRate)
		{
			EnumFlags |= ACM_FORMATENUMF_NSAMPLESPERSEC;
		}

		FormatEnumCallbackStruct fcs;

		fcs.pAcm = this;
		fcs.flags = Flags;
		fcs.FormatToMatch = FormatToMatch;
		fcs.m_Tag = m_FormatTags[SelFormat].Tag;
		fcs.m_NumTagsToCompare = 0;
		fcs.m_pTagsToCompare = NULL;

		int res = acmFormatEnum(had, & afd, FormatEnumCallback, DWORD( & fcs), EnumFlags);
		TRACE("acmFormatEnum returned %x\n", res);

		for (int i = 0, ch = m_Wf.NumChannels()
							// if compatible format or match channels, check only exact num channels
			; i <= (0 == (Flags & (WaveFormatMatchCompatibleFormats | WaveFormatMatchCnannels)))
			; i++, ch ^= 3)
		{
			pwfx.InitFormat(WAVE_FORMAT_PCM, m_Wf.SampleRate(), ch,
							m_Wf.BitsPerSample());
			if (WAVE_FORMAT_PCM == dwFormatTag)
			{
				// if PCM format, add exact format to the list, and its
				// mono/stereo counterpart (if compatible not selected).
				ACMFORMATDETAILS afd;
				afd.cbStruct = sizeof afd;
				afd.dwFormatIndex = 0;
				afd.dwFormatTag = pwfx.FormatTag();
				afd.fdwSupport = 0;
				afd.pwfx = pwfx;
				afd.cbwfx = sizeof (WAVEFORMATEX) + pwfx.m_pWf->cbSize;

				res = acmFormatDetails(had, & afd, ACM_FORMATDETAILSF_FORMAT);
				if (MMSYSERR_NOERROR == res)
				{
					FormatItem * pfmt = m_Formats.insert(m_Formats.end());
					pfmt->Wf = afd.pwfx;
					pfmt->Name = afd.szFormat;
				}
			}
			else
			{
				memzero(afd);
				afd.cbStruct = sizeof afd;
				afd.cbwfx = pwfx.m_AllocatedSize;
				afd.dwFormatTag = dwFormatTag;
				afd.pwfx = pwfx;
				res = acmFormatEnum(had, & afd, FormatEnumCallback,
									DWORD(& fcs), ACM_FORMATENUMF_CONVERT);

				if (m_Formats.empty())
				{
					// try acmFormatSuggest
					res = 0;
					if (WAVE_FORMAT_PCM == dwFormatTag
						|| MMSYSERR_NOERROR == acmFormatSuggest(had, m_Wf, pwfx, pwfx.m_AllocatedSize,
																ACM_FORMATSUGGESTF_WFORMATTAG))
					{
						ACMFORMATDETAILS afd;
						afd.cbStruct = sizeof afd;
						afd.dwFormatIndex = 0;
						afd.dwFormatTag = pwfx.FormatTag();
						afd.fdwSupport = 0;
						afd.pwfx = pwfx;
						afd.cbwfx = sizeof (WAVEFORMATEX) + pwfx.m_pWf->cbSize;

						res = acmFormatDetails(had, & afd, ACM_FORMATDETAILSF_FORMAT);
						if (MMSYSERR_NOERROR == res)
						{
							FormatItem * pfmt = m_Formats.insert(m_Formats.end());
							pfmt->Wf = afd.pwfx;
							pfmt->Name = afd.szFormat;
						}
					}
					TRACE("acmFormatEnum SUGGEST returned %x\n", res);
				}
			}
		}

		acmDriverClose(had, 0);

		std::sort(m_Formats.begin(), m_Formats.end(), std::greater<FormatItem>());
		m_Formats.erase(std::unique(m_Formats.begin(), m_Formats.end()), m_Formats.end());
	}
}

