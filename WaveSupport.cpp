// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSupport.cpp

#include "stdafx.h"
#include <mmsystem.h>
#include "WaveSupport.h"
#include "BladeMP3EncDLL.h"
#include <algorithm>
#include <functional>
#include "resource.h"
#include <ks.h>
#include <ksmedia.h>
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

	MMRESULT err = waveOutOpen( & m_hwo, id, pwfe, DWORD_PTR(waveOutProc), (DWORD_PTR)this,
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

MMRESULT CWaveOut::Play(UINT hBuffer, unsigned UsedSize, DWORD AuxFlags)
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
	m_pBufs[hBuffer - 1].whd.dwUser = (DWORD_PTR) & m_pBufs[hBuffer - 1];
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
									UINT uMsg,	DWORD_PTR dwInstance, DWORD_PTR dwParam1,
									DWORD /*dwParam2*/)
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

	MMRESULT err = waveInOpen( & m_hwi, id, m_wfe, DWORD_PTR(waveInProc), DWORD_PTR(this),
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

MMRESULT CWaveIn::Record(UINT hBuffer, unsigned UsedSize)
{
	ASSERT(this != NULL);
	ASSERT(m_hwi != NULL);

	ASSERT(hBuffer >= 1 && hBuffer <= nBuffers);
	ASSERT(m_pBufs != NULL);
	ASSERT(UsedSize > 0 && UsedSize <= m_pBufs[hBuffer - 1].size);

	m_pBufs[hBuffer - 1].whd.lpData = m_pBufs[hBuffer - 1].pBuf;
	m_pBufs[hBuffer - 1].whd.dwBufferLength = UsedSize;
	m_pBufs[hBuffer - 1].whd.dwBytesRecorded = UsedSize;
	m_pBufs[hBuffer - 1].whd.dwUser = (DWORD_PTR) & m_pBufs[hBuffer - 1];
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

void CALLBACK CWaveIn::waveInProc(HWAVEIN /*hwi*/,
								UINT /*uMsg*/,	DWORD_PTR /*dwInstance*/, DWORD /*dwParam1*/,
								DWORD /*dwParam2*/	)
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

WAVEFORMATEX * CWaveFormat::Allocate(unsigned ExtraSize, bool bCopy)
{
	if (ExtraSize > 0xFFF0)
	{
		ExtraSize = 0;
	}

	int SizeToAllocate = ExtraSize + sizeof (WAVEFORMATEX);
	if (m_AllocatedSize >= SizeToAllocate)
	{
		return m_pWf;
	}
	void * NewBuf = new char[SizeToAllocate];
	if (NULL == NewBuf)
	{
		return NULL;
	}
	if (m_pWf)
	{
		if (bCopy) memcpy(NewBuf, m_pWf, m_AllocatedSize);
		delete[] (char*) m_pWf;
	}
	m_pWf = (WAVEFORMATEX *)NewBuf;
	m_AllocatedSize = SizeToAllocate;
	return m_pWf;
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
	if (NULL == pWf)
	{
		delete [] (char*)Detach();
	}
	else if (pWf != m_pWf)
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
			memcpy(m_pWf, pWf, pWf->cbSize + sizeof (WAVEFORMATEX));
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

bool CWaveFormat::IsCompressed() const
{
	if (NULL == m_pWf)
	{
		return false;
	}
	if (WAVE_FORMAT_PCM == m_pWf->wFormatTag
		|| WAVE_FORMAT_IEEE_FLOAT == m_pWf->wFormatTag)
	{
		// PCM integer or float format
		return false;
	}

	if (WAVE_FORMAT_EXTENSIBLE == m_pWf->wFormatTag
		|| FormatSize() >= sizeof (WAVEFORMATEXTENSIBLE))
	{
		WAVEFORMATEXTENSIBLE * pWfe = (WAVEFORMATEXTENSIBLE *) m_pWf;
		if (pWfe->SubFormat == KSDATAFORMAT_SUBTYPE_PCM
			|| pWfe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			return false;
		}
	}

	return true;
}

NUMBER_OF_CHANNELS CWaveFormat::NumChannelsFromMask(CHANNEL_MASK ChannelMask) const
{
	NUMBER_OF_CHANNELS Channels = 0;
	for (int ch = 0; ch < 32 && Channels < NumChannels(); ch++)
	{
		if (ChannelMask & (1 << ch))
		{
			Channels++;
		}
	}
	return Channels;
}

// return channels disregarding their position
CHANNEL_MASK CWaveFormat::ChannelsMask() const
{
	if (NULL == m_pWf)
	{
		return 0;
	}

	return ~((~0) << m_pWf->nChannels);
}

ULONG CWaveFormat::ValidateFormat() const
{
	if (NULL == m_pWf)
	{
		return WaveformatNoFormatLoaded;
	}
	if (0 == m_pWf->nChannels
		|| m_pWf->nChannels > MAX_NUMBER_OF_CHANNELS)
	{
		return WaveformatInvalidNumberChannels;
	}

	ULONG Mask = 0;

	if (m_pWf->nChannels > 2)
	{
		Mask |= WaveFormatMultichannel;
	}

	if (WAVE_FORMAT_PCM == m_pWf->wFormatTag)
	{
		// Number of bits: 8, >16, <= 32
		if (m_pWf->wBitsPerSample > 32
			|| (m_pWf->wBitsPerSample != 8
				&& m_pWf->wBitsPerSample < 16))
		{
			return WaveformatInvalidNumberOfBits;
		}

		if (m_pWf->wBitsPerSample == 8
			|| m_pWf->wBitsPerSample == 16)
		{
			if (m_pWf->nBlockAlign != m_pWf->wBitsPerSample * m_pWf->nChannels / 8)
			{
				// nBlockAlign is not exact
				return WaveformatInvalidBlockAlign;
			}
		}
		else if (m_pWf->wBitsPerSample > 16)
		{
			if (m_pWf->nBlockAlign * 8 < m_pWf->wBitsPerSample * m_pWf->nChannels
				|| m_pWf->nBlockAlign > 4 * m_pWf->nChannels)
			{
				// nBlockAlign is too big
				// or less than minimum
				return WaveformatInvalidBlockAlign;
			}
			Mask |= WaveformatExtendedNumBits;
		}
		// nBlockAlign should be at least sample size times number of channels
		// if nBlockAlign is divisible by nChannels, each sample is byte aligned
		if (m_pWf->nBlockAlign / m_pWf->nChannels > (m_pWf->wBitsPerSample + 7) / 8)
		{
			Mask |= WaveformatDataPadded;
		}

		if (m_pWf->nSamplesPerSec > 1000000
			|| m_pWf->nSamplesPerSec * m_pWf->nBlockAlign != m_pWf->nAvgBytesPerSec)
		{
			return WaveformatInvalidBytesPerSec;
		}
	}
	else if (WAVE_FORMAT_IEEE_FLOAT == m_pWf->wFormatTag)
	{
		// Number of bits: 32, 64
		if (m_pWf->wBitsPerSample != 32
			|| m_pWf->wBitsPerSample != 64)
		{
			return WaveformatInvalidNumberOfBits;
		}

		// nBlockAlign should be exact sample size times number of channels
		// each sample should be byte-aligned
		if (m_pWf->nBlockAlign != m_pWf->wBitsPerSample * m_pWf->nChannels / 8)
		{
			// nBlockAlign is invalid
			return WaveformatInvalidBlockAlign;
		}

		if (m_pWf->nSamplesPerSec > 1000000
			|| m_pWf->nSamplesPerSec * m_pWf->nBlockAlign != m_pWf->nAvgBytesPerSec)
		{
			return WaveformatInvalidBytesPerSec;
		}
	}
	else if (WAVE_FORMAT_EXTENSIBLE == m_pWf->wFormatTag)
	{
		if (FormatSize() < sizeof (WAVEFORMATEXTENSIBLE))
		{
			return WaveformatInvalidSize;
		}
		WAVEFORMATEXTENSIBLE * pWfe = (WAVEFORMATEXTENSIBLE *) m_pWf;
		if (pWfe->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
		{
			// Number of bits: 8, 16, 24, 32
			if (m_pWf->wBitsPerSample != 8
				&& m_pWf->wBitsPerSample != 16
				&& m_pWf->wBitsPerSample != 24
				&& m_pWf->wBitsPerSample != 32)
			{
				return WaveformatInvalidNumberOfBits;
			}

			if (m_pWf->nBlockAlign != m_pWf->wBitsPerSample * m_pWf->nChannels / 8)
			{
				// nBlockAlign is not exact
				return WaveformatInvalidBlockAlign;
			}

			if (m_pWf->wBitsPerSample == 8
				|| m_pWf->wBitsPerSample == 16)
			{
			}
			if (m_pWf->wBitsPerSample > 16)
			{
				Mask |= WaveformatExtendedNumBits;
			}
			// nBlockAlign should be at least sample size times number of channels
			// if nBlockAlign is divisible by nChannels, each sample is byte aligned
			if (m_pWf->nBlockAlign / m_pWf->nChannels > (m_pWf->wBitsPerSample + 7) / 8)
			{
				Mask |= WaveformatDataPadded;
			}

			if (m_pWf->nSamplesPerSec > 1000000
				|| m_pWf->nSamplesPerSec * m_pWf->nBlockAlign != m_pWf->nAvgBytesPerSec)
			{
				return WaveformatInvalidBytesPerSec;
			}
		}
		else if (pWfe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			// TODO
		}
	}
	return Mask;
}

struct FormatTagEnumStruct
{
	CAudioCompressionManager * pAcm;
	WAVEFORMATEX const * pWf;
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
	int TagIndex;
	BOOL FormatFound;
};

// if can convert to any format of the tag, add the tag to the table
BOOL _stdcall CAudioCompressionManager::FormatTestEnumCallback(
																HACMDRIVERID /*hadid*/, LPACMFORMATDETAILS pafd,
																DWORD_PTR dwInstance, DWORD /*fdwSupport*/)
{
	FormatEnumCallbackStruct * pFcs = (FormatEnumCallbackStruct *) dwInstance;

	TRACE(_T("FormatTestEnumCallback: format=%s, tag=%d, \n"), pafd->szFormat, pafd->dwFormatTag);

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
															DWORD_PTR dwInstance, DWORD /*fdwSupport*/)
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
	fcs.m_Tag.Tag = WORD(paftd->dwFormatTag & 0xFFFF);
	fcs.m_pTagsToCompare = pfts->pListOfTags;
	fcs.m_NumTagsToCompare = pfts->NumTags;
	fcs.flags = pfts->flags;
	fcs.FormatToMatch = pfts->pWf;
	fcs.TagIndex = 0;

	CWaveFormat pwfx;
	pwfx.Allocate(0xFFF0);
	ACMFORMATDETAILS afd;

	TRACE(_T("FormatTagEnum: name=%s, driverID=%x, tag=%d, formats=%d, max size=%d\n"), paftd->szFormatTag,
		hadid, paftd->dwFormatTag, paftd->cStandardFormats, paftd->cbFormatSize);

	pwfx.InitFormat(WORD(paftd->dwFormatTag),
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

			int res = acmFormatEnum(had, & afd, FormatTestEnumCallback, DWORD_PTR(& fcs), flags);
			TRACE("acmFormatEnum returned %x\n", res);
			if ( ! fcs.FormatFound)
			{
				pwfx.InitFormat(WAVE_FORMAT_PCM,
								pfts->pWf->nSamplesPerSec, pfts->pWf->nChannels);
				res = acmFormatEnum(had, & afd, FormatTestEnumCallback,
									DWORD_PTR(& fcs), ACM_FORMATENUMF_CONVERT);
				TRACE("acmFormatEnum returned %x\n", res);
			}
			if ( ! fcs.FormatFound
				&& (pfts->flags & WaveFormatMatchCompatibleFormats))
			{
				// try acmFormatSuggest
				pwfx.InitFormat(WAVE_FORMAT_PCM,
								pfts->pWf->nSamplesPerSec, pfts->pWf->nChannels);
				res = acmFormatSuggest(had, const_cast<LPWAVEFORMATEX>(pfts->pWf),
										pwfx, pwfx.m_AllocatedSize, ACM_FORMATSUGGESTF_WFORMATTAG);
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
		unsigned nIndex = (unsigned)pAcm->m_FormatTags.size();
		pAcm->m_FormatTags.resize(nIndex + 1);
		pAcm->m_FormatTags[nIndex].SetData(fcs.m_Tag, paftd->szFormatTag, hadid);
	}
	return TRUE;
}

void CAudioCompressionManager::FillFormatTagArray
	(WAVEFORMATEX const * pwf, WaveFormatTagEx const ListOfTags[],
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

	acmFormatTagEnum(NULL, & atd, FormatTagEnumCallback, DWORD_PTR( & fts), 0);

}

// enumerates all formats for the tag
BOOL _stdcall CAudioCompressionManager::FormatEnumCallback(
															HACMDRIVERID /*hadid*/, LPACMFORMATDETAILS pafd,
															DWORD_PTR dwInstance, DWORD /*fdwSupport*/)
{
	FormatEnumCallbackStruct * pfcs = (FormatEnumCallbackStruct *) dwInstance;

	CAudioCompressionManager * pAcm = pfcs->pAcm;
	TRACE(_T("FormatEnum: format=%s, tag=%d\n"), pafd->szFormat, pafd->dwFormatTag);

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
		pAcm->m_Formats.insert(pAcm->m_Formats.end(),
								FormatItem(pafd->pwfx, pafd->szFormat, pfcs->TagIndex));
		pfcs->FormatFound = TRUE;
	}
	return TRUE;
}

bool CAudioCompressionManager::FillMultiFormatArray(unsigned nSelFrom, unsigned nSelTo, int Flags)
{
	m_Formats.clear();

	CWaveFormat pwfx;
	pwfx.Allocate(0xFFF0, true);
	for (unsigned sel = nSelFrom; sel <= nSelTo && sel < m_FormatTags.size(); sel++)
	{

		if (BladeMp3Encoder::GetTag() == m_FormatTags[sel].Tag)
		{
			// todo: handle "Compatible formats" flag
			FillLameEncoderFormats();
			continue;
		}

		WORD wFormatTag = m_FormatTags[sel].Tag.Tag;

		ACMFORMATDETAILS afd;

		pwfx.InitFormat(wFormatTag, m_Wf.SampleRate(), m_Wf.NumChannels(),
						m_Wf.BitsPerSample());

		memzero(afd);
		afd.cbStruct = sizeof afd;
		afd.cbwfx = pwfx.m_AllocatedSize;
		afd.dwFormatTag = wFormatTag;
		afd.pwfx = pwfx;

		CWaveFormat FormatToMatch(m_Wf);
		FormatToMatch.FormatTag() = wFormatTag;

		HACMDRIVER had = NULL;
		if (NULL != m_FormatTags[sel].m_hadid
			&& MMSYSERR_NOERROR == acmDriverOpen(&had, m_FormatTags[sel].m_hadid, 0))
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
			fcs.m_Tag = m_FormatTags[sel].Tag;
			fcs.m_NumTagsToCompare = 0;
			fcs.m_pTagsToCompare = NULL;
			fcs.FormatFound = FALSE;
			fcs.TagIndex = sel;

			int res = acmFormatEnum(had, & afd, FormatEnumCallback, DWORD_PTR( & fcs), EnumFlags);
			TRACE("acmFormatEnum returned %x\n", res);

			for (int i = 0, ch = m_Wf.NumChannels()
								// if compatible format or match channels, check only exact num channels
				; i <= 0 + (0 == (Flags & (WaveFormatMatchCompatibleFormats | WaveFormatMatchCnannels)))
				; i++, ch ^= 3)
			{
				pwfx.InitFormat(WAVE_FORMAT_PCM, m_Wf.SampleRate(), WORD(ch),
								m_Wf.BitsPerSample());
				if (WAVE_FORMAT_PCM == wFormatTag)
				{
					// if PCM format, add exact format to the list, and its
					// mono/stereo counterpart (if compatible not selected).
					for (int j = 0; j != 2; j++)
					{
						ACMFORMATDETAILS afd;
						afd.cbStruct = sizeof afd;
						afd.dwFormatIndex = 0;
						afd.dwFormatTag = WAVE_FORMAT_PCM;
						afd.fdwSupport = 0;
						afd.pwfx = pwfx;
						afd.cbwfx = sizeof (PCMWAVEFORMAT);

						res = acmFormatDetails(had, & afd, ACM_FORMATDETAILSF_FORMAT);
						if (MMSYSERR_NOERROR == res)
						{
							m_Formats.insert(m_Formats.end(),
											FormatItem(afd.pwfx, afd.szFormat, sel));
						}
						if (16 == pwfx.BitsPerSample())
						{
							break;
						}

						// make sure 16 bits format is on the list
						pwfx.InitFormat(WAVE_FORMAT_PCM, m_Wf.SampleRate(),
										WORD(ch), 16);
					}
				}
				else
				{
					memzero(afd);
					afd.cbStruct = sizeof afd;
					afd.cbwfx = pwfx.m_AllocatedSize;
					afd.dwFormatTag = wFormatTag;
					afd.pwfx = pwfx;
					fcs.FormatFound = FALSE;
					res = acmFormatEnum(had, & afd, FormatEnumCallback,
										DWORD_PTR(& fcs), ACM_FORMATENUMF_CONVERT);

					if ( ! fcs.FormatFound
						&& (Flags & WaveFormatMatchCompatibleFormats))
					{
						// try acmFormatSuggest
						res = 0;
						if (WAVE_FORMAT_PCM == wFormatTag
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
								m_Formats.insert(m_Formats.end(),
												FormatItem(afd.pwfx, afd.szFormat, sel));
							}
						}
						TRACE("acmFormatEnum SUGGEST returned %x\n", res);
					}
				}
			}

			acmDriverClose(had, 0);

		}
	}
	std::sort(m_Formats.begin(), m_Formats.end(), std::greater<FormatItem>());
	m_Formats.erase(std::unique(m_Formats.begin(), m_Formats.end()), m_Formats.end());
	return ! m_Formats.empty();
}

void CAudioCompressionManager::FillWmaFormatTags()
{
	//WAVE_FORMAT_MSAUDIO1+1 - WMA V2
	static WaveFormatTagEx const format(WAVE_FORMAT_MSAUDIO1 + 1);
	// fill format tag array with V2 format
	FillFormatTagArray(m_Wf, & format, 1);
}

void CAudioCompressionManager::FillMp3EncoderTags(DWORD Flags)
{

	BladeMp3Encoder Mp3Enc;
	// check if MP3 ACM encoder presents
	WaveFormatTagEx const Mp3Tag(WAVE_FORMAT_MPEGLAYER3);

	FillFormatTagArray(m_Wf, & Mp3Tag, 1, Flags);

	// check if LAME encoder is available
	if ((0 == (Flags
				& (WaveFormatMatchCompatibleFormats
					| WaveFormatMatchCnannels
					| WaveFormatMatchSampleRate))
			|| (m_Wf.SampleRate() == 44100 && m_Wf.NumChannels() == 2))
		&& Mp3Enc.Open())
	{
		FormatTagItem TagItem;
		TagItem.Name = Mp3Enc.GetVersionString();
		TagItem.Tag = Mp3Enc.GetTag();

		m_FormatTags.insert(m_FormatTags.begin(), TagItem);

		Mp3Enc.Close();
	}

}

void CAudioCompressionManager::FillLameEncoderFormats()
{
	static int const Mp3Bitrates[] = { 64, 96, 128, 160, 192, 256, 320 };
	CString ms;
	if (1 == m_Wf.NumChannels())
	{
		ms.LoadString(IDS_MONO);
	}
	else
	{
		ms.LoadString(IDS_STEREO);
	}

	CString f;
	f.LoadString(IDS_LAMEENC_FORMAT);
	FormatItem Item;

	m_Formats.resize(sizeof Mp3Bitrates / sizeof Mp3Bitrates[0]);

	for (unsigned i = 0; i < m_Formats.size(); i++)
	{
		m_Formats[i].Name.Format(f, Mp3Bitrates[i], LPCTSTR(ms));
		m_Formats[i].Wf.InitFormat(WAVE_FORMAT_EXTENSIBLE, 44100,
									2, 16,
									sizeof (WAVEFORMATEXTENSIBLE) - sizeof (WAVEFORMATEX));
		m_Formats[i].Wf.m_pWf->nAvgBytesPerSec = Mp3Bitrates[i] * 125;
		((WAVEFORMATEXTENSIBLE*)(m_Formats[i].Wf.m_pWf))->SubFormat =
			BladeMp3Encoder::GetTag().SubFormat;
		m_Formats[i].TagIndex = 0;
	}
}

int CAudioCompressionManager::FillFormatsCombo(CComboBox * pCombo,
												CWaveFormat & Wf,
												WaveFormatTagEx SelectedTag,
												int SelectedBitrate)
{
	pCombo->ResetContent();
	pCombo->LockWindowUpdate();

	unsigned i;

	for (i = 0; i < m_Formats.size(); i++)
	{
		pCombo->AddString(m_Formats[i].Name);
	}

	unsigned sel = ~0U;
	int BestMatch = 0;
	for (i = 0; i < m_Formats.size(); i++)
	{
		WAVEFORMATEX * pwf = m_Formats[i].Wf;

		if (SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1
			|| SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1 + 1)
		{
			if (::abs(long(SelectedBitrate - pwf->nAvgBytesPerSec * 8)) < 1000)
			{
				sel = i;
			}
			continue;
		}
		else if (SelectedTag.Tag == WAVE_FORMAT_MPEGLAYER3
				|| SelectedTag == BladeMp3Encoder::GetTag())
		{
			if (::abs(long(SelectedBitrate - pwf->nAvgBytesPerSec * 8)) < 1000)
			{
				sel = i;
			}
			continue;
		}

		int match = Wf.MatchFormat(pwf);
		if (WaveFormatExactMatch & match)
		{
			// exact match found
			sel = i;
			break;
		}
		// select the best match
		// Sample rate must match, then number of channels might match
		// If original format is non PCM and the queried format is the same format

		if (match > BestMatch)
		{
			BestMatch = match;
			sel = i;
			continue;
		}
		if (0 == match
			|| match < BestMatch)
		{
			continue;
		}
		if (m_Formats[sel].Wf.FormatTag() == WAVE_FORMAT_PCM)
		{
			if (-1 == sel
				|| pwf->wBitsPerSample >= m_Formats[sel].Wf.BitsPerSample())
			{
				sel = i;
			}
		}
		else
		{
			if (-1 == sel
				|| pwf->nAvgBytesPerSec >= m_Formats[sel].Wf.BytesPerSec())
			{
				sel = i;
			}
		}
	}

	if (-1 == sel)
	{
		sel = 0;
	}
	pCombo->SetCurSel(sel);
	pCombo->UnlockWindowUpdate();
	return sel;
}

CString CAudioCompressionManager::GetFormatName(HACMDRIVER had, WAVEFORMATEX const * pWf)
{
	ACMFORMATDETAILS afd;
	afd.cbStruct = sizeof afd;
	afd.dwFormatIndex = 0;
	afd.dwFormatTag = pWf->wFormatTag;
	afd.fdwSupport = 0;
	afd.pwfx = const_cast<WAVEFORMATEX*>(pWf);
	afd.cbwfx = sizeof (WAVEFORMATEX) + pWf->cbSize;
	if (WAVE_FORMAT_PCM == pWf->wFormatTag)
	{
		afd.cbwfx = sizeof (PCMWAVEFORMAT);
	}

	MMRESULT res = acmFormatDetails(had, & afd, ACM_FORMATDETAILSF_FORMAT);
	if (MMSYSERR_NOERROR == res)
	{
		return afd.szFormat;
	}
	return CString();
}

CString CAudioCompressionManager::GetFormatTagName(HACMDRIVER had, DWORD Tag)
{
	ACMFORMATTAGDETAILS afd = {0};

	afd.cbStruct = sizeof afd;
	afd.dwFormatTag = Tag;

	MMRESULT res = acmFormatTagDetails(had, & afd, ACM_FORMATTAGDETAILSF_FORMATTAG);
	if (MMSYSERR_NOERROR == res)
	{
		return afd.szFormatTag;
	}
	return CString();
}

AudioStreamConvertor::AudioStreamConvertor(HACMDRIVER drv)
	: m_SrcBufSize(0),
	m_DstBufSize(0),
	m_acmDrv(drv),
	m_acmStr(NULL)
{
	memzero(m_ash);
	m_ash.cbStruct = sizeof m_ash;
}

AudioStreamConvertor::~AudioStreamConvertor()
{
	Close();
}

BOOL AudioStreamConvertor::SuggestFormat(WAVEFORMATEX const * pWf1,
										WAVEFORMATEX * pWf2, unsigned MaxFormat2Size, DWORD flags)
{
	m_MmResult = acmFormatSuggest(m_acmDrv,
								const_cast<LPWAVEFORMATEX>(pWf1),
								pWf2, MaxFormat2Size, flags);
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::QueryOpen(WAVEFORMATEX const * pWfSrc,
									WAVEFORMATEX const * pWfDst, DWORD flags)
{
	ASSERT(0 == (flags & (CALLBACK_EVENT | CALLBACK_FUNCTION | CALLBACK_WINDOW)));
	HACMSTREAM has;

	m_MmResult = acmStreamOpen( & has, m_acmDrv,
								const_cast<LPWAVEFORMATEX>(pWfSrc),
								const_cast<LPWAVEFORMATEX>(pWfDst), NULL, NULL, NULL,
								ACM_STREAMOPENF_QUERY | flags);
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::Open(WAVEFORMATEX const * pWfSrc,
								WAVEFORMATEX const * pWfDst, DWORD flags)
{
	ASSERT(NULL == m_acmStr);
	ASSERT(0 == (flags & (ACM_STREAMOPENF_QUERY | CALLBACK_EVENT | CALLBACK_FUNCTION | CALLBACK_WINDOW)));

	memzero(m_ash);
	m_ash.cbStruct = sizeof m_ash;

	m_MmResult = acmStreamOpen( & m_acmStr, m_acmDrv,
								const_cast<LPWAVEFORMATEX>(pWfSrc),
								const_cast<LPWAVEFORMATEX>(pWfDst), NULL, NULL, NULL,
								flags);
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::Open(WAVEFORMATEX const * pWfSrc,
								WAVEFORMATEX const * pWfDst, HWND hCallbackWnd, DWORD_PTR dwInstance,
								DWORD flags)
{
	ASSERT(NULL == m_acmStr);
	ASSERT(0 == (flags & (ACM_STREAMOPENF_QUERY | CALLBACK_EVENT | CALLBACK_FUNCTION)));

	memzero(m_ash);
	m_ash.cbStruct = sizeof m_ash;

	m_MmResult = acmStreamOpen( & m_acmStr, m_acmDrv,
								const_cast<LPWAVEFORMATEX>(pWfSrc),
								const_cast<LPWAVEFORMATEX>(pWfDst), NULL,
								reinterpret_cast<DWORD_PTR>(hCallbackWnd), dwInstance,
								flags | CALLBACK_WINDOW);
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::Open(WAVEFORMATEX const * pWfSrc,
								WAVEFORMATEX const * pWfDst, HANDLE hEvent,
								DWORD flags)
{
	ASSERT(NULL == m_acmStr);
	ASSERT(0 == (flags & (ACM_STREAMOPENF_QUERY | CALLBACK_WINDOW | CALLBACK_FUNCTION)));

	memzero(m_ash);
	m_ash.cbStruct = sizeof m_ash;

	m_MmResult = acmStreamOpen( & m_acmStr, m_acmDrv,
								const_cast<LPWAVEFORMATEX>(pWfSrc),
								const_cast<LPWAVEFORMATEX>(pWfDst), NULL,
								reinterpret_cast<DWORD_PTR>(hEvent), NULL,
								flags | CALLBACK_EVENT);
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::Open(WAVEFORMATEX const * pWfSrc,
								WAVEFORMATEX const * pWfDst, AcmStreamCallback Callback, DWORD_PTR dwInstance,
								DWORD flags)
{
	ASSERT(NULL == m_acmStr);
	ASSERT(0 == (flags & (ACM_STREAMOPENF_QUERY | CALLBACK_WINDOW | CALLBACK_EVENT)));

	memzero(m_ash);
	m_ash.cbStruct = sizeof m_ash;

	m_MmResult = acmStreamOpen( & m_acmStr, m_acmDrv,
								const_cast<LPWAVEFORMATEX>(pWfSrc),
								const_cast<LPWAVEFORMATEX>(pWfDst), NULL,
								reinterpret_cast<DWORD_PTR>(Callback), dwInstance,
								flags | CALLBACK_FUNCTION);
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::AllocateBuffers(size_t PreferredInBufSize,
											size_t PreferredOutBufSize)
{
	ASSERT(NULL != m_acmStr);
	ASSERT(NULL == m_ash.pbSrc);
	ASSERT(NULL == m_ash.pbDst);

	m_SrcBufSize = DWORD(PreferredInBufSize);
	m_DstBufSize = DWORD(PreferredOutBufSize);

	if (0 == PreferredInBufSize)
	{
		if (MMSYSERR_NOERROR != (m_MmResult = acmStreamSize(m_acmStr, m_DstBufSize, & m_SrcBufSize,
															ACM_STREAMSIZEF_DESTINATION))
			|| MMSYSERR_NOERROR != (m_MmResult = acmStreamSize(m_acmStr, m_SrcBufSize, & m_DstBufSize,
																ACM_STREAMSIZEF_SOURCE)))
		{
			return FALSE;
		}
	}
	else
	{
		if (MMSYSERR_NOERROR != (m_MmResult = acmStreamSize(m_acmStr, m_SrcBufSize, & m_DstBufSize,
															ACM_STREAMSIZEF_SOURCE))
			|| MMSYSERR_NOERROR != (m_MmResult = acmStreamSize(m_acmStr, m_DstBufSize, & m_SrcBufSize,
																ACM_STREAMSIZEF_DESTINATION)))
		{
			return FALSE;
		}
	}
	// allocate buffers
	m_ash.cbSrcLength = m_SrcBufSize;
	m_ash.cbDstLength = m_DstBufSize;
	m_ash.pbSrc = new BYTE[m_SrcBufSize];
	m_ash.pbDst = new BYTE[m_DstBufSize];

	if (NULL == m_ash.pbSrc
		|| NULL == m_ash.pbDst)
	{
		return FALSE;
	}

	m_MmResult = acmStreamPrepareHeader(m_acmStr, & m_ash, 0);
	m_ash.cbSrcLength = 0;
	m_ash.cbDstLength = 0;
	return MMSYSERR_NOERROR == m_MmResult;
}

BOOL AudioStreamConvertor::Convert(void const * pSrc, size_t SrcSize, size_t * pSrcBufUsed,
									void* * ppDstBuf, size_t * pDstBufFilled,
									DWORD flags)
{
	ASSERT(NULL != m_acmStr);
	// fill input buffer
	unsigned ToCopy = m_SrcBufSize - m_ash.cbSrcLength;

	if (ToCopy > SrcSize)
	{
		ToCopy = unsigned(SrcSize);
	}

	* pDstBufFilled = 0;
	* ppDstBuf = m_ash.pbDst;

	memcpy(m_ash.pbSrc + m_ash.cbSrcLength, pSrc, ToCopy);
	m_ash.cbSrcLength += ToCopy;

	*pSrcBufUsed = ToCopy;

	if (m_ash.cbSrcLength < m_SrcBufSize
		&& 0 == (flags & ACM_STREAMCONVERTF_END))
	{
		return TRUE;
	}
	m_ash.cbSrcLengthUsed = 0;
	m_ash.cbDstLength = m_DstBufSize;
	m_ash.cbDstLengthUsed = 0;

	m_MmResult = acmStreamConvert(m_acmStr, & m_ash, flags);

	* pDstBufFilled = m_ash.cbDstLengthUsed;

	if (MMSYSERR_NOERROR != m_MmResult
		|| (0 == m_ash.cbDstLengthUsed && 0 == m_ash.cbSrcLengthUsed
			&& 0 == (flags & ACM_STREAMCONVERTF_END)))
	{
		return FALSE;
	}

	memmove(m_ash.pbSrc, m_ash.pbSrc + m_ash.cbSrcLengthUsed,
			m_ash.cbSrcLength - m_ash.cbSrcLengthUsed);
	m_ash.cbSrcLength -= m_ash.cbSrcLengthUsed;

	return TRUE;
}

BOOL AudioStreamConvertor::Reset(DWORD flags)
{
	ASSERT(NULL != m_acmStr);
	return NULL != m_acmStr
			&& MMSYSERR_NOERROR == acmStreamReset(m_acmStr, flags);
}

void AudioStreamConvertor::Close()
{
	m_ash.cbDstLength = m_DstBufSize;
	m_ash.cbSrcLength = m_SrcBufSize;

	if (NULL != m_acmStr)
	{
		if (m_ash.fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED)
		{
			acmStreamUnprepareHeader(m_acmStr, & m_ash, 0);
		}

		m_ash.fdwStatus = 0;

		delete[] m_ash.pbDst;
		m_ash.pbDst = NULL;

		delete[] m_ash.pbSrc;
		m_ash.pbSrc = NULL;

		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
	}
}

void CopyWaveSamples(void * pDstBuf, CHANNEL_MASK DstChannels,
					NUMBER_OF_CHANNELS const NumDstChannels,
					void const * pSrcBuf, CHANNEL_MASK SrcChannels,
					NUMBER_OF_CHANNELS const NumSrcChannels,
					unsigned Samples,
					WaveSampleType DstType, WaveSampleType SrcType)
{
	ASSERT(DstType == SampleType16bit);
	ASSERT(SrcType == SampleType16bit);

	CHANNEL_MASK const DstChannelsMask = ~((~0UL) << NumDstChannels);

	DstChannels &= DstChannelsMask;
	ASSERT(0 != DstChannels);

	CHANNEL_MASK const SrcChannelsMask = ~((~0UL) << NumSrcChannels);

	SrcChannels &= SrcChannelsMask;
	ASSERT(0 != SrcChannels);

	ASSERT(NumSrcChannels <= 2 && NumSrcChannels > 0);
	int const SrcSampleSize = sizeof (WAVE_SAMPLE) * NumSrcChannels;

	ASSERT(NumDstChannels <= 2 && NumDstChannels > 0);
	//int const DstSampleSize = sizeof (WAVE_SAMPLE) * NumDstChannels;

	if (DstType == SrcType
		&& DstChannelsMask == DstChannels
		&& SrcChannelsMask == SrcChannels
		&& DstChannels == SrcChannels)
	{
		// simply copy the data
		memmove(pDstBuf, pSrcBuf, Samples * SrcSampleSize);
		return;
	}

	// the following variants are possible:
	// copying one channel to one channel
	// copying one channel to two channels
	// copying two channels to one channel
	WAVE_SAMPLE const * pSrc = (WAVE_SAMPLE const *) pSrcBuf;
	WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) pDstBuf;

	if ((SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT) == DstChannels)
	{
		ASSERT(SPEAKER_FRONT_RIGHT == SrcChannels
				|| SPEAKER_FRONT_LEFT == SrcChannels);
		// copy one src channel to two dst channels
		if (SPEAKER_FRONT_RIGHT == SrcChannels)
		{
			// channel #1
			pSrc++;
		}

		for (unsigned i = 0; i < Samples;
			i++, pSrc += NumSrcChannels, pDst += NumDstChannels)
		{
			WAVE_SAMPLE temp = *pSrc;
			pDst[0] = temp;
			pDst[1] = temp;
		}
	}
	else if ((SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT) == SrcChannels)
	{
		// copy two src channels to one dst channel
		ASSERT(SPEAKER_FRONT_RIGHT == DstChannels
				|| SPEAKER_FRONT_LEFT == DstChannels);

		if (SPEAKER_FRONT_RIGHT == DstChannels)
		{
			// channel #1
			pDst++;
		}

		for (unsigned i = 0; i < Samples;
			i++, pSrc += NumSrcChannels, pDst += NumDstChannels)
		{
			pDst[0] = (pSrc[0] + pSrc[1]) / 2;
		}
	}
	else
	{
		// copying one channel to one channel
		if (SPEAKER_FRONT_RIGHT == SrcChannels)
		{
			// channel #1
			pSrc++;
		}
		if (SPEAKER_FRONT_RIGHT == DstChannels)
		{
			// channel #1
			pDst++;
		}

		for (unsigned i = 0; i < Samples;
			i++, pSrc += NumSrcChannels, pDst += NumDstChannels)
		{
			pDst[0] = pSrc[0];
		}
	}
}


WAVEFORMATEX const CWaveFormat::CdAudioFormat =
{
	WAVE_FORMAT_PCM,
	2,
	44100,  // nSamplesPerSec
	44100*4,  // nAvgBytesPerSec
	4, // nBlockAlign
	16, // bits per sample
	0   // cbSize
};
