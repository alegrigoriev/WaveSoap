#include "StdAfx.h"
#include "BladeMp3EncDll.h"

BladeMp3Encoder::BladeMp3Encoder()
	: m_pStream(NULL),
	m_DllModule(NULL),
	m_InBufferSize(0),
	m_OutBufferSize(0),
	m_bFlushStreamCalled(FALSE),
	beInitStream(InitStreamStub),
	beEncodeChunk(EncodeChunkStub),
	beDeinitStream(DeinitStreamStub),
	beCloseStream(CloseStreamStub),
	beGetVersion(GetVersionStub),
	beWriteVBRHeader(WriteVBRHeaderStub)
{
}

void BladeMp3Encoder::Close()
{
	if (m_pStream)
	{
		CloseStream();
	}
	beInitStream = InitStreamStub;
	beEncodeChunk = EncodeChunkStub;
	beDeinitStream = DeinitStreamStub;
	beCloseStream = CloseStreamStub;
	beGetVersion = GetVersionStub;
	beWriteVBRHeader = WriteVBRHeaderStub;
	if (m_DllModule)
	{
		FreeLibrary(m_DllModule);
		m_DllModule = NULL;
	}
}
BOOL BladeMp3Encoder::OpenStream(PBE_CONFIG pConfig)
{
	int OpenCount = InterlockedIncrement( & m_StreamCount);
	ASSERT (1 == OpenCount);

	if (OpenCount != 1)
	{
		InterlockedDecrement( & m_StreamCount);
		return FALSE;
	}

	if (NULL != m_pStream)
	{
		return FALSE;
	}

	DWORD dwSamples;
	if (BE_ERR_SUCCESSFUL != beInitStream(pConfig, & dwSamples, & m_OutBufferSize, & m_pStream))
	{
		return FALSE;
	}

	m_bFlushStreamCalled = false;

	m_InBufferSize = dwSamples * 2;
	if (pConfig->format.LHV1.nMode != BE_MP3_MODE_MONO)
	{
		m_InBufferSize *= 2;
	}
	return TRUE;
}

void BladeMp3Encoder::CloseStream()
{
	if (NULL != m_pStream)
	{
		beCloseStream(m_pStream);
		m_pStream = NULL;

		ASSERT(1 == m_StreamCount);

		InterlockedDecrement( & m_StreamCount);
	}
}

BOOL BladeMp3Encoder::Open(LPCTSTR DllName)
{
	if (m_DllModule != NULL)
	{
		// cannot open twice
		return FALSE;
	}
	m_DllModule = LoadLibrary(DllName);
	if (NULL == m_DllModule)
	{
		return FALSE;
	}

	beInitStream = (BEINITSTREAM)GetProcAddress(m_DllModule, TEXT_BEINITSTREAM);
	beEncodeChunk = (BEENCODECHUNK)GetProcAddress(m_DllModule, TEXT_BEENCODECHUNK);
	beDeinitStream = (BEDEINITSTREAM)GetProcAddress(m_DllModule, TEXT_BEDEINITSTREAM);
	beCloseStream = (BECLOSESTREAM)GetProcAddress(m_DllModule, TEXT_BECLOSESTREAM);
	beGetVersion = (BEVERSION)GetProcAddress(m_DllModule, TEXT_BEVERSION);
	beWriteVBRHeader = (BEWRITEVBRHEADER)GetProcAddress(m_DllModule, TEXT_BEWRITEVBRHEADER);

	if (NULL == beInitStream
		|| NULL == beEncodeChunk
		|| NULL == beDeinitStream
		|| NULL == beCloseStream
		|| NULL == beGetVersion
		|| NULL == beWriteVBRHeader)
	{
		Close();
		return FALSE;
	}
	return TRUE;
}

BOOL BladeMp3Encoder::EncodeChunk(short const * pSrc, int nSamples, BYTE * pDst, DWORD * pBytesEncoded)
{
	return BE_ERR_SUCCESSFUL == beEncodeChunk(m_pStream, nSamples, pSrc, pDst, pBytesEncoded);
}

BOOL BladeMp3Encoder::FlushStream(BYTE * pDst, DWORD * pBytesEncoded)
{
	if (m_bFlushStreamCalled)
	{
		*pBytesEncoded = 0;
		return TRUE;
	}
	m_bFlushStreamCalled = TRUE;
	return BE_ERR_SUCCESSFUL == beDeinitStream(m_pStream, pDst, pBytesEncoded);
}

CString BladeMp3Encoder::GetVersionString()
{
	BE_VERSION ver;
	memzero(ver);
	GetVersion( & ver);

	SYSTEMTIME time;
	memzero(time);
	time.wYear = ver.wYear;
	time.wDay = ver.byDay;
	time.wMonth = ver.byMonth;

	int const TimeBufSize = 256;
	TCHAR str[TimeBufSize] = {0};

	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, & time, NULL, str, TimeBufSize - 1);

	CString s;
	s.Format(_T("LameEnc DLL Version %d.%02d, (%s) Engine %d.%02d"),
			ver.byDLLMajorVersion, ver.byDLLMinorVersion,
			str, ver.byMajorVersion, ver.byMinorVersion);
	return s;
}

// LAME encoder supports only 1 stream! Truly lame
LONG BladeMp3Encoder::m_StreamCount = 0;
