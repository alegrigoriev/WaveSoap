// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WmaFile.cpp
#include "stdafx.h"
#include "WmaFile.h"
#include <wmsysprf.h>
#include "KInterlocked.h"
#define TRACE_WMA_DECODER 0

HRESULT STDMETHODCALLTYPE CDirectFileStream::Read(
												/* [length_is][size_is][out] */ void __RPC_FAR *pv,
												/* [in] */ ULONG cb,
												/* [out] */ ULONG __RPC_FAR *pcbRead)
{
	if (TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CDirectFileStream::Read %d bytes at pos %d, length=%d\n"), GetCurrentThreadId(),
								cb, (DWORD)m_File.Seek(0, FILE_CURRENT), (DWORD)m_File.GetLength());

	LONG lRead = m_File.Read(pv, cb);

	if (lRead != -1)
	{
		if (NULL != pcbRead)
		{
			*pcbRead = lRead;
		}
		//TRACE(_T("Read %d bytes, buffer CRC = %X\n"), lRead, CalcCrc32(PUCHAR(pv), lRead));
		return S_OK;
	}
	else
	{
		TRACE(_T("Thread:%08X CDirectFileStream::Read failed, read %d bytes, requested %d\n"), GetCurrentThreadId(),
			lRead, cb);
		return S_FALSE;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Write(
													/* [size_is][in] */ const void __RPC_FAR *pv,
													/* [in] */ ULONG cb,
													/* [out] */ ULONG __RPC_FAR *pcbWritten)
{
	if (TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CDirectFileStream::Write %d bytes\n"), GetCurrentThreadId(), cb);
	LONG lWritten = m_File.Write(pv, cb);
	if (-1 != lWritten)
	{
		if (NULL != pcbWritten)
		{
			*pcbWritten = lWritten;
		}
		return S_OK;
	}
	else
	{
		return STG_E_MEDIUMFULL;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Seek(
												/* [in] */ LARGE_INTEGER dlibMove,
												/* [in] */ DWORD dwOrigin,
												/* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
	if (0 || TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CDirectFileStream::Seek to %08X%08X, %d\n"), GetCurrentThreadId(),
									dlibMove.HighPart, dlibMove.LowPart, dwOrigin);
	int origin;
	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
		origin = FILE_BEGIN;
		break;
	case STREAM_SEEK_CUR:
		origin = FILE_CURRENT;
		break;
	case STREAM_SEEK_END:
		origin = FILE_END;
		break;
	default:
		return STG_E_INVALIDFUNCTION;
	}
	LONGLONG pos = m_File.Seek(dlibMove.QuadPart, origin);
	if (-1i64 == pos)
	{
		return STG_E_INVALIDPOINTER;
	}
	if (NULL != plibNewPosition)
	{
		plibNewPosition->QuadPart = pos;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::SetSize(
													/* [in] */ ULARGE_INTEGER libNewSize)
{
	if (m_File.SetFileLength(libNewSize.QuadPart))
	{
		return S_OK;
	}
	else
	{
		return STG_E_MEDIUMFULL;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::CopyTo(
													/* [unique][in] */ IStream __RPC_FAR *pstm,
													/* [in] */ ULARGE_INTEGER cb,
													/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
													/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
	// copy cb bytes from this stream to *pstrm
	if (TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CDirectFileStream::CopyTo %d bytes\n"), GetCurrentThreadId(), cb);
	LONGLONG FilePos = m_File.Seek(0, FILE_CURRENT);
	LONGLONG FileLength = m_File.GetLength();
	if (NULL != pcbRead)
	{
		pcbRead->QuadPart = 0;
	}
	if (NULL != pcbWritten)
	{
		pcbWritten->QuadPart = 0;
	}
	while (cb.QuadPart > 0)
	{
		LONGLONG ToLock = cb.QuadPart;
		LONGLONG RestOfTheFile = FileLength - FilePos;
		if (ToLock > RestOfTheFile)
		{
			ToLock = RestOfTheFile;
		}
		if (0 == ToLock)
		{
			break;
		}
		void * pBuf;
		long Locked = m_File.GetDataBuffer(& pBuf, ToLock, FilePos, 0);
		if (0 == Locked)
		{
			break;
		}
		if (Locked < 0)
		{
			return STG_E_MEDIUMFULL;
		}
		FilePos += Locked;
		m_File.Seek(Locked, FILE_CURRENT);

		if (NULL != pcbRead)
		{
			pcbRead->QuadPart += Locked;
		}
		ULONG Written = 0;
		HRESULT hr = pstm->Write(pBuf, Locked, & Written);
		m_File.ReturnDataBuffer(pBuf, Locked, CDirectFile::ReturnBufferDiscard);

		if( FAILED( hr ) )
		{
			return hr ;
		}
		if (NULL != pcbWritten)
		{
			pcbWritten->QuadPart += Written;
		}
		if (Written != ULONG(Locked))
		{
			break;
		}
		cb.QuadPart -= Locked;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Commit(
													/* [in] */ DWORD /*grfCommitFlags*/)
{
	m_File.Commit();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Revert( void)
{
	// not really supported
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::LockRegion(
														/* [in] */ ULARGE_INTEGER /*libOffset*/,
														/* [in] */ ULARGE_INTEGER /*cb*/,
														/* [in] */ DWORD /*dwLockType*/)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::LockRegion\n"));
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::UnlockRegion(
														/* [in] */ ULARGE_INTEGER /*libOffset*/,
														/* [in] */ ULARGE_INTEGER /*cb*/,
														/* [in] */ DWORD /*dwLockType*/)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::UnlockRegion\n"));
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Stat(
												/* [out] */ STATSTG __RPC_FAR *pstatstg,
												/* [in] */ DWORD grfStatFlag)
{
	if (TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CDirectFileStream::Stat flag=%x\n"), GetCurrentThreadId(), grfStatFlag);
	if (! m_File.IsOpen())
	{
		return STG_E_ACCESSDENIED;
	}
	switch (grfStatFlag)
	{
	case STATFLAG_NONAME:
		break;
	case STATFLAG_DEFAULT:
		break;
	default:
		return STG_E_INVALIDFLAG;
	}
	if (NULL == pstatstg)
	{
		return STG_E_INVALIDPOINTER;
	}
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = m_File.GetLength();
	pstatstg->mtime = m_File.GetFileInformation().ftLastWriteTime;
	pstatstg->ctime = m_File.GetFileInformation().ftCreationTime;
	pstatstg->atime = m_File.GetFileInformation().ftLastAccessTime;
	pstatstg->grfLocksSupported = 0;
	pstatstg->grfMode = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	pstatstg->reserved = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Clone(
													/* [out] */ IStream __RPC_FAR *__RPC_FAR * /*ppstm*/)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::Clone\n"));
	return STG_E_INVALIDFUNCTION;
}

void CDirectFileStream::Close()
{
	m_File.Close(0);
}


CWmaDecoderSync::CWmaDecoderSync()
	: //ReaderStatus(WMT_ERROR),
	m_CurrentStreamTime(0),
	m_bNeedNextSample(false),
	m_BufferLengthTime(0),
	m_Bitrate(1),
	m_bOpened(false),
	m_bStarted(false),
	m_OpenedEvent(FALSE, FALSE),
	m_StartedEvent(FALSE, FALSE),
	m_SampleEvent(FALSE, FALSE),
	m_dwAudioOutputNum(0)
{
}

CWmaDecoderSync::~CWmaDecoderSync()
{
}

void CWmaDecoderSync::DeInit()
{
	m_Reader.Release();
}

bool CWmaDecoderSync::DeliverNextSample()
{
	INSSBuffer* pSample = NULL;
	QWORD cnsSampleTime = 0;
	QWORD cnsSampleDuration = 0;
	DWORD dwFlags = 0;
	DWORD dwOutputNum = 0;
	WORD wStreamNum = 0;

	HRESULT hr = m_Reader->GetNextSample( 0, &pSample,
										&cnsSampleTime,
										&cnsSampleDuration,
										&dwFlags,
										&dwOutputNum,
										&wStreamNum );

	if( FAILED( hr ) )
	{
		if( NS_E_NO_MORE_SAMPLES == hr )
		{
			Stop();
			return false;
		}
	}
	if (m_dwAudioOutputNum != dwOutputNum)
	{
		if (TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CWmaDecoder::OnSample m_dwAudioOutputNum != dwOutputNum\n"), GetCurrentThreadId());
		if (pSample != NULL)
		{
			pSample->Release();
		}
		return true;
	}

	BYTE *pData = NULL;
	DWORD cbData = 0;

	hr = pSample->GetBufferAndLength( &pData, &cbData);

	if (TRACE_WMA_DECODER) TRACE(_T("Thread:%08X CWmaDecoder::OnSample, sample time=%d ms, next time=%d ms, %d bytes, hr=%X\n"), GetCurrentThreadId(),
								DWORD(cnsSampleTime/10000), DWORD((cnsSampleTime+cnsSampleDuration)/10000), cbData, hr);

	if( FAILED( hr ) )
	{
		pSample->Release();
		Stop();
		return false;
	}
	m_DstFile.CDirectFile::Write(pData, cbData);

	// update current number of samples
	SAMPLE_POSITION DstCopyPos = (SAMPLE_POSITION)m_DstFile.CDirectFile::Seek(0, FILE_CURRENT);
	SAMPLE_INDEX DstCopySample = m_DstFile.PositionToSample(DstCopyPos);

	if (DstCopySample > m_CurrentSamples)
	{
		// calculate new length
		NUMBER_OF_SAMPLES MaxNumberOfSamples = 0x7FFFFFFF / m_DstFile.SampleSize();
		LONGLONG TotalSamplesEstimated = ULONG(double(DstCopySample) * SrcLength() / SrcPos());

		if (TotalSamplesEstimated > MaxNumberOfSamples)
		{
			TotalSamplesEstimated = MaxNumberOfSamples;
		}
		if (NUMBER_OF_SAMPLES(TotalSamplesEstimated) < m_CurrentSamples)
		{
			TotalSamplesEstimated = m_CurrentSamples;
		}

		m_CurrentSamples = NUMBER_OF_SAMPLES(TotalSamplesEstimated);

		m_DstFile.SetFileLengthSamples(m_CurrentSamples);
	}
	// modify positions after file length modified,
	m_DstPos = DstCopyPos;  // to avoid race condition
	m_DstCopySample = DstCopySample;

	// ask for next buffer
	m_CurrentStreamTime = cnsSampleTime + cnsSampleDuration;

	pSample->Release();
	return IsStarted();
}

BOOL CWmaDecoderSync::Init()
{
	HRESULT hr;
	hr = WMCreateSyncReader( NULL, 0 , &m_Reader );
	if( FAILED( hr ) )
	{
		return FALSE;
	}

	m_CurrentStreamTime = 0;
	m_bNeedNextSample = true;
	return TRUE;
}

HRESULT CWmaDecoderSync::Open(LPCTSTR szFilename)
{
	CDirectFile file;
	if ( ! file.Open(szFilename, CDirectFile::OpenReadOnly))
	{
		return S_FALSE;
	}
	return Open(file);
}

#if 1
#define USE_READER_CALLBACK_ADVANCED 1
class CWmaDecoderAsync : public IWMReaderCallback
#if USE_READER_CALLBACK_ADVANCED
	, public IWMReaderCallbackAdvanced
#endif
{
public:

	CWmaDecoderAsync();
	~CWmaDecoderAsync();
	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if (riid == IID_IWMReaderCallback)
		{
			*ppvObject = static_cast<IWMReaderCallback*>(this);
		}
#if USE_READER_CALLBACK_ADVANCED
		else if (riid == IID_IWMReaderCallbackAdvanced)
		{
			*ppvObject = static_cast<IWMReaderCallbackAdvanced*>(this);
		}
#endif
		else if (riid == IID_IWMStatusCallback)
		{
			*ppvObject = static_cast<IWMStatusCallback*>(this);
		}
		else
		{
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) { return 1; }

	ULONG STDMETHODCALLTYPE Release(void) { return 1; }
	//
	//Methods of IWMReaderCallback
	//
	HRESULT STDMETHODCALLTYPE OnSample( /* [in] */ DWORD dwOutputNum,
													/* [in] */ QWORD cnsSampleTime,
													/* [in] */ QWORD cnsSampleDuration,
													/* [in] */ DWORD dwFlags,
													/* [in] */ INSSBuffer __RPC_FAR *pSample,
													/* [in] */ void __RPC_FAR *pvContext);

	HRESULT STDMETHODCALLTYPE OnStatus( /* [in] */ WMT_STATUS Status,
													/* [in] */ HRESULT hr,
													/* [in] */ WMT_ATTR_DATATYPE dwType,
													/* [in] */ BYTE __RPC_FAR *pValue,
													/* [in] */ void __RPC_FAR *pvContext);

#if USE_READER_CALLBACK_ADVANCED
	//
	//Methods of IWMReaderCallbackAdvanced
	//
	virtual HRESULT STDMETHODCALLTYPE OnStreamSample(
													/* [in] */ WORD /*wStreamNum*/,
													/* [in] */ QWORD /*cnsSampleTime*/,
													/* [in] */ QWORD /*cnsSampleDuration*/,
													/* [in] */ DWORD /*dwFlags*/,
													/* [in] */ INSSBuffer __RPC_FAR * /*pSample*/,
													/* [in] */ void __RPC_FAR * /*pvContext*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnTime(
											/* [in] */ QWORD cnsCurrentTime,
											/* [in] */ void __RPC_FAR *pvContext);

	virtual HRESULT STDMETHODCALLTYPE OnStreamSelection(
														/* [in] */ WORD /*wStreamCount*/,
														/* [in] */ WORD __RPC_FAR * /*pStreamNumbers*/,
														/* [in] */ WMT_STREAM_SELECTION __RPC_FAR * /*pSelections*/,
														/* [in] */ void __RPC_FAR * /*pvContext*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OnOutputPropsChanged(
															/* [in] */ DWORD /*dwOutputNum*/,
															/* [in] */ WM_MEDIA_TYPE __RPC_FAR * /*pMediaType*/,
															/* [in] */ void __RPC_FAR * /*pvContext*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AllocateForStream(
														/* [in] */ WORD wStreamNum,
														/* [in] */ DWORD cbBuffer,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
														/* [in] */ void __RPC_FAR *pvContext);

	virtual HRESULT STDMETHODCALLTYPE AllocateForOutput(
														/* [in] */ DWORD dwOutputNum,
														/* [in] */ DWORD cbBuffer,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
														/* [in] */ void __RPC_FAR *pvContext);
#endif

	HRESULT Open(LPCTSTR szFilename);
	HRESULT Open(CDirectFile & file);

	BOOL Init();
	void DeInit();

	HRESULT Start();
	HRESULT Stop();

	MEDIA_FILE_SIZE SrcLength()
	{
		return m_InputStream.GetLength();
	}
	MEDIA_FILE_POSITION SrcPos()
	{
		return m_InputStream.GetPos();
	}

	bool IsOpened() const
	{
		return m_bOpened;
	}
	bool IsStarted() const
	{
		return m_bStarted;
	}
	void DeliverNextSample(DWORD timeout);
	void SetDstFile(CWaveFile & file);
	CWaveFormat const & GetSrcFormat() const
	{
		return m_SrcWf;
	}
	CWaveFormat const & GetDstFormat() const
	{
		return m_DstWf;
	}
	DWORD GetBitRate() const
	{
		return m_Bitrate;
	}
	long GetCurrentSample() const
	{
		return m_DstCopySample;
	}
	ULONG GetTotalSamples() const
	{
		return m_CurrentSamples;
	}

protected:
	BOOL volatile m_bNeedNextSample;
	CWaveFile m_DstFile;

	SAMPLE_POSITION m_DstPos;
	SAMPLE_INDEX m_DstCopySample;

	CComPtr<IWMReader> m_Reader;
	CComQIPtr<IWMReaderAdvanced2> m_pAdvReader;

	CDirectFileStream m_InputStream;

	bool m_bOpened;
	bool m_bStarted;

	ATL::CEvent m_OpenedEvent;
	ATL::CEvent m_StartedEvent;
	ATL::CEvent m_SampleEvent;

	//WMT_STATUS ReaderStatus;
	QWORD m_CurrentStreamTime;
	QWORD m_BufferLengthTime; //32kbytes in 100ns units
	QWORD m_StreamDuration;

	NUMBER_OF_SAMPLES m_CurrentSamples;
	DWORD m_dwAudioOutputNum;
	CWaveFormat m_DstWf;
	CWaveFormat m_SrcWf;
	DWORD m_Bitrate;
};



#endif

HRESULT CWmaDecoderSync::Open(CDirectFile & file)
{
	HRESULT hr;
	if ( ! file.IsOpen())
	{
		return S_FALSE;
	}
#if 0
	m_InputStream.SetFile(file);

	hr = m_Reader->OpenStream( & m_InputStream);
#else
	hr = m_Reader->Open(file.GetName());
#endif
	if (FAILED(hr))
	{
#if 1
		CWmaDecoderAsync decoder1;
		CComPtr<IWMReader> reader;
		hr = WMCreateReader(NULL, 0, &reader);
		if (SUCCEEDED(hr))
		{
			hr = reader->Open(file.GetName(), &decoder1, NULL);

		}
#endif
		m_InputStream.Close();
		return hr;
	}
	// select an audio stream
	DWORD cOutputs = 0 ;
	hr = m_Reader->GetOutputCount( & cOutputs);
	if (FAILED(hr) || cOutputs > 63)
	{
		m_Reader->Close();
		m_InputStream.Close();
		return hr;
	}
	// query profile (stream properties)

	WM_MEDIA_TYPE* pMedia = NULL ;
	ULONG cbType = 0 ;
	DWORD i;

	for (i = 0 ; i < cOutputs ; i++ )
	{
		CComPtr<IWMOutputMediaProps> pProps;

		hr = m_Reader->GetOutputProps( i, &pProps );

		if (FAILED(hr))
		{
			break;
		}

		hr = pProps->GetMediaType(NULL, &cbType) ;

		if (FAILED(hr))
		{
			break;
		}

		pMedia = (WM_MEDIA_TYPE*) new BYTE[cbType];

		if (pMedia == NULL)
		{
			hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

			break;
		}

		hr = pProps->GetMediaType(pMedia, &cbType);

		if (FAILED(hr))
		{
			break;
		}

		if (pMedia->majortype == WMMEDIATYPE_Audio)
		{
			break;
		}

		delete[] pMedia;
		pMedia = NULL;
		cbType = 0;
	}

	if (i >= cOutputs || NULL == pMedia)
	{
		//
		// Couldnt find any Audio output number in the file
		//
		hr = E_UNEXPECTED;

	}
	else
	{
#if 0
		WCHAR str1[100];
		WCHAR str2[100];
		StringFromGUID2(pMedia->majortype, str1, sizeof str1 / sizeof str1[0]);
		StringFromGUID2(pMedia->subtype, str2, sizeof str2 / sizeof str2[0]);
		if (TRACE_WMA_DECODER) TRACE(_T("Media type = %S, subtype = %S\n"), str1, str2);
// WMA: Media type = {73647561-0000-0010-8000-00AA00389B71}, sybtype = {00000001-0000-0010-8000-00AA00389B71}
// MP3: Media type = {73647561-0000-0010-8000-00AA00389B71}, sybtype = {00000001-0000-0010-8000-00AA00389B71}
#endif
		m_dwAudioOutputNum = i;

		m_DstWf = ( WAVEFORMATEX * )pMedia->pbFormat;

		DWORD MaxSampleSize = 32768;

		hr = m_Reader->GetMaxOutputSampleSize(m_dwAudioOutputNum, & MaxSampleSize);

		if (TRACE_WMA_DECODER) TRACE(_T("m_pAdvReader->GetMaxOutputSampleSize=%d\n"), MaxSampleSize);
		if (FAILED(hr))
		{
			MaxSampleSize = 32768;
		}
		m_BufferLengthTime = MulDiv(MaxSampleSize, 10000000, m_DstWf.BytesPerSec());
		m_BufferLengthTime -= m_BufferLengthTime % 10000;   // round to miliseconds

		if (m_DstWf.FormatTag() != WAVE_FORMAT_PCM)
		{
			hr = E_UNEXPECTED;
		}
	}

	delete[] ( BYTE* )pMedia;
	pMedia = NULL;
	if (FAILED(hr))
	{
		return hr;
	}

	if (CComQIPtr<IWMHeaderInfo> pHeaderInfo = m_Reader)
	{
		WORD stream = WORD(m_dwAudioOutputNum);
		if (TRACE_WMA_DECODER) TRACE(_T("IWMHeaderInfo interface acquired\n"));

		QWORD StreamLength = 0;
		WORD SizeofStreamLength = sizeof StreamLength;
		WMT_ATTR_DATATYPE type = WMT_TYPE_QWORD;

#ifdef _DEBUG
		WORD AttributeCount = 0;

		pHeaderInfo->GetAttributeCount(stream, & AttributeCount);
		TRACE("HeaderInfo::stream=%d, %d attributes\n", stream, AttributeCount);
		for (WORD ii = 0; ii < AttributeCount; ii++)
		{
			WCHAR name[1024];
			name[0] = 0;
			WORD NameLen = 1024;
			WMT_ATTR_DATATYPE Type;
			BYTE value[1024];
			WORD Length=1024;
			if (SUCCEEDED(pHeaderInfo->GetAttributeByIndex(ii,
															&stream, name, & NameLen, & Type, value, & Length)))
			{
				TRACE(L"Attribute %d name=%s\n", ii, name);
			}
		}
#endif

		hr = pHeaderInfo->GetAttributeByName( & stream, g_wszWMDuration,
											& type, (BYTE *) & StreamLength, & SizeofStreamLength);

		if (SUCCEEDED(hr))
		{
			if (TRACE_WMA_DECODER) TRACE(_T("Stream Length = %08X%08X (%d seconds), size=%d\n"),
				DWORD(StreamLength >> 32), DWORD(StreamLength & 0xFFFFFFFF), DWORD(StreamLength / 10000000), SizeofStreamLength);
			m_StreamDuration = StreamLength;
		}
	}
	else
	{
		m_StreamDuration = 10000 * 1000 * 60i64;    // 1 minute
	}

	m_CurrentSamples = ULONG(m_StreamDuration * m_DstWf.SampleRate() / 10000000);
	if (TRACE_WMA_DECODER) TRACE(_T("m_CurrentSamples = %d (%d seconds)\n"), m_CurrentSamples,
								m_CurrentSamples / m_DstWf.SampleRate());

	if (CComQIPtr<IWMProfile> pProfile = m_Reader)
	{
		DWORD nStreamCount = 0;
		pProfile->GetStreamCount( & nStreamCount);

		for (unsigned iStream = 0; iStream < nStreamCount; iStream++)
		{
			CComPtr<IWMStreamConfig> pStreamConfig;
			hr = pProfile->GetStream(iStream, & pStreamConfig);

			if (SUCCEEDED(hr))
			{
				WORD iStreamNumber;
				pStreamConfig->GetStreamNumber( & iStreamNumber);

				if (CComQIPtr<IWMMediaProps> pStreamProps = pStreamConfig)
				{
					if (FAILED(pStreamConfig->GetBitrate( & m_Bitrate)))
					{
						m_Bitrate = 1;
					}
					DWORD size = 0;
					pStreamProps->GetMediaType(NULL, & size);

					WM_MEDIA_TYPE * pType = (WM_MEDIA_TYPE *)new  char[size];
					if (pType)
					{
						pStreamProps->GetMediaType(pType, & size);
						WMT_STREAM_SELECTION StreamSelection = WMT_ON;
						if (pType->majortype != WMMEDIATYPE_Audio)
						{
							StreamSelection = WMT_OFF;
						}

						m_Reader->SetStreamsSelected(1, & iStreamNumber, & StreamSelection);
						m_SrcWf = (WAVEFORMATEX *) pType->pbFormat;
						delete[] (char*) pType;
					}
				}
			}
		}
	}

	return S_OK;
}

HRESULT CWmaDecoderSync::Start()
{
	m_bStarted = true;
	return S_OK;
}

HRESULT CWmaDecoderSync::Stop()
{
	m_bStarted = false;
	return S_OK;
}

void CWmaDecoderSync::SetDstFile(CWaveFile & file)
{
	m_DstFile = file;
	m_DstPos = file.SampleToPosition(0);
	m_DstCopySample = 0;
	m_DstFile.CDirectFile::Seek(m_DstPos, FILE_BEGIN);
}

////////////////////////////////////////////////////////////////////////////////
//////// WmaEncoder
WmaEncoder::WmaEncoder()
	:m_SampleTime100ns(0)
{
}

void WmaEncoder::DeInit()
{
	TRACE("WmaEncoder::DeInit\n");
	if (m_pWriter)
	{
		Write(NULL, 0); // make sure buffer gets written
		m_pWriter->Flush();

		m_pWriter->EndWriting();

		if (m_pHeaderInfo)
		{
#if 0
			// WM bug: Duration property is set incorrect, seems rounded to multiple of 32 KB wave buffers.
			//m_pHeaderInfo->SetAttribute();    // Writer doesn't support after the file is written?
			QWORD StreamLength = 0;
			WORD SizeofStreamLength = sizeof StreamLength;
			WMT_ATTR_DATATYPE type = WMT_TYPE_QWORD;
			WORD stream = 0;
			HRESULT hr = m_pHeaderInfo->GetAttributeByName( & stream, g_wszWMDuration,
															& type, (BYTE *) & StreamLength, & SizeofStreamLength);
			if (SUCCEEDED(hr))
			{
				if (TRACE_WMA_DECODER) TRACE(_T("AFter write Stream Length = %08X%08X (%d seconds)\n"),
					DWORD(StreamLength >> 32), DWORD(StreamLength & 0xFFFFFFFF), DWORD(StreamLength / 10000000));
			}
#endif
		}
	}
	m_pWriter.Release();
	m_pWriterAdvanced.Release();
	m_pProfileManager.Release();
	m_pHeaderInfo.Release();
	m_pBuffer.Release();
}

WmaEncoder::~WmaEncoder()
{
}

BOOL WmaEncoder::OpenWrite(CDirectFile & File)
{
	m_FileWriter.Open(File);
	HRESULT hr;

	hr = m_pWriterAdvanced->AddSink( & m_FileWriter);

	m_pHeaderInfo = m_pWriter;
#if 0

	if ( ! m_pHeaderInfo)
	{
		return FALSE;
	}

	m_pHeaderInfo->SetAttribute(0, g_wszWMTitle, WMT_TYPE_STRING, (BYTE const *)L"Title", sizeof L"Title");
	m_pHeaderInfo->SetAttribute(0, g_wszWMAuthor, WMT_TYPE_STRING, (BYTE const *)L"Author", sizeof L"Author");
	m_pHeaderInfo.Release();
#endif
	// open input properties
	CComPtr<IWMInputMediaProps> pMediaProps;
	hr = m_pWriter->GetInputProps(0, & pMediaProps);

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	char * buf = NULL;
	DWORD size = 0;
	pMediaProps->GetMediaType(NULL, & size);
	buf = new char[size];

	WM_MEDIA_TYPE * pType = (WM_MEDIA_TYPE *) buf;
	hr = pMediaProps->GetMediaType(pType, & size);

	if (SUCCEEDED(hr))
	{
		//WAVEFORMATEX * pwfx = (WAVEFORMATEX *)pType->pbFormat;

		pType->pbFormat = (PBYTE) & m_SrcWfx;
		pType->cbFormat = sizeof m_SrcWfx;
		hr = m_pWriter->SetInputProps(0, pMediaProps);
	}

	delete[] buf;

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	m_SampleTime100ns = 0;

	hr = m_pWriter->BeginWriting();

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

#ifdef _DEBUG
	DWORD NumSinks;
	m_pWriterAdvanced->GetSinkCount( & NumSinks);
	if (TRACE_WMA_DECODER) TRACE(_T("Writer sink count=%d\n"), NumSinks);
#endif
	return TRUE;
}

#ifdef _DEBUG
void PrintProfile(IWMProfileManager * pProfileManager, REFGUID guid)
{
	CComPtr<IWMProfile> pProfile;

	HRESULT hr;
	hr = pProfileManager->LoadProfileByID(guid, & pProfile);
	if ( ! SUCCEEDED(hr))
	{
		return;
	}

	CComPtr<IWMStreamConfig> pStreamConfig;
	hr = pProfile->GetStreamByNumber(1, & pStreamConfig);

	if (SUCCEEDED(hr))
	{
		CComQIPtr<IWMMediaProps> pProps = pStreamConfig;

		DWORD bitrate;
		pStreamConfig->GetBitrate( & bitrate);

		if (pProps)
		{
			DWORD cbType;
			//   Make the first call to establish the size of buffer needed.
			pProps -> GetMediaType(NULL, &cbType);

			//   Now create a buffer of the appropriate size
			BYTE *pBuf = new BYTE[cbType];
			//   Create an appropriate structure pointer to the buffer.
			WM_MEDIA_TYPE *pType = (WM_MEDIA_TYPE*) pBuf;

			//   Call the method again to extract the information.
			hr = pProps -> GetMediaType(pType, & cbType);

			WAVEFORMATEX * pWfx = (WAVEFORMATEX *) pType->pbFormat;
			DWORD * pExt = (DWORD *) (pType->pbFormat + sizeof WAVEFORMATEX);
			if (TRACE_WMA_DECODER) TRACE(_T("Format: %d, BytesPerSec: %d, BlockAlign: %d, cbSize: %d, stream bitrate=%d\n"),
										pWfx->wFormatTag, pWfx->nAvgBytesPerSec,
										pWfx->nBlockAlign, pWfx->cbSize, bitrate);
			if (TRACE_WMA_DECODER) TRACE(_T("FormatExtension: %08X, %08X, %08X\n"), pExt[0], pExt[1], pExt[2]);

			delete[] pBuf;

		}
	}

}
#endif

BOOL WmaEncoder::Init()
{
	HRESULT hr;
	if (S_OK != WMCreateWriter(NULL, & m_pWriter))
	{
		return FALSE;
	}

	m_pWriterAdvanced = m_pWriter;

	if ( ! m_pWriterAdvanced)
	{
		return FALSE;
	}

	hr = WMCreateProfileManager( & m_pProfileManager);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

#ifdef _DEBUG
	PrintProfile(m_pProfileManager, WMProfile_V40_64Audio);
	PrintProfile(m_pProfileManager, WMProfile_V40_96Audio);
	PrintProfile(m_pProfileManager, WMProfile_V40_128Audio);
	PrintProfile(m_pProfileManager, WMProfile_V70_64AudioISDN);
	PrintProfile(m_pProfileManager, WMProfile_V70_64Audio);
	PrintProfile(m_pProfileManager, WMProfile_V70_96Audio);
	PrintProfile(m_pProfileManager, WMProfile_V70_128Audio);
#endif

	return TRUE;
}

void WmaEncoder::SetArtist(LPCTSTR szArtist)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMAuthor, WMT_TYPE_STRING,
								(LPBYTE)szArtist, WORD((wcslen(szArtist) + 1) * sizeof(TCHAR)));
#else
	WCHAR Artist[MAX_PATH+1] = {0};
	int nChars = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szArtist, -1,
										Artist, MAX_PATH);
	if (0 == nChars)
	{
		return;
	}
	m_pHeaderInfo->SetAttribute(1, g_wszWMAuthor, WMT_TYPE_STRING,
								(LPBYTE)Artist, (nChars + 1) * sizeof(TCHAR));
#endif
}

void WmaEncoder::SetAlbum(LPCTSTR szAlbum)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMAlbumTitle, WMT_TYPE_STRING,
								(LPBYTE)szAlbum, WORD((wcslen(szAlbum) + 1) * sizeof(TCHAR)));
#else
	WCHAR Album[MAX_PATH+1] = {0};
	int nChars = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAlbum, -1,
										Album, MAX_PATH);
	if (0 == nChars)
	{
		return;
	}
	m_pHeaderInfo->SetAttribute(1, g_wszWMAlbumTitle, WMT_TYPE_STRING,
								(LPBYTE)Album, (nChars + 1) * sizeof(TCHAR));
#endif
}

void WmaEncoder::SetGenre(LPCTSTR szGenre)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMGenre, WMT_TYPE_STRING,
								(LPBYTE)szGenre, WORD((wcslen(szGenre) + 1) * sizeof(TCHAR)));
#else
	WCHAR Genre[MAX_PATH+1] = {0};
	int nChars = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szGenre, -1,
										Genre, MAX_PATH);
	if (0 == nChars)
	{
		return;
	}
	m_pHeaderInfo->SetAttribute(1, g_wszWMGenre, WMT_TYPE_STRING,
								(LPBYTE)Genre, (nChars + 1) * sizeof(TCHAR));
#endif
}

void WmaEncoder::SetSourceWaveFormat(WAVEFORMATEX const * pSrcWfx)
{
	m_SrcWfx = pSrcWfx;
}

BOOL WmaEncoder::SetDestinationFormat(WAVEFORMATEX const * pDstWfx)
{
// pDstWfx points to the destination format (but with wFormatTag = WAVE_FORMAT_PCM
// the function must replace wFormatTag
	CComPtr<IWMProfile> pProfile;

	HRESULT hr = m_pProfileManager->LoadProfileByID(WMProfile_V70_128Audio, & pProfile);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	CComPtr<IWMStreamConfig> pStreamConfig;
	hr = pProfile->GetStreamByNumber(1, & pStreamConfig);

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	CComQIPtr<IWMMediaProps> pProps = pStreamConfig;

	if ( ! pProps)
	{
		return FALSE;
	}

	DWORD cbType;
	//   Make the first call to establish the size of buffer needed.
	pProps -> GetMediaType(NULL, &cbType);

	//   Now create a buffer of the appropriate size
	BYTE *pBuf = new BYTE[cbType];
	//   Create an appropriate structure pointer to the buffer.
	WM_MEDIA_TYPE *pType = (WM_MEDIA_TYPE*) pBuf;

	//   Call the method again to extract the information.
	hr = pProps -> GetMediaType(pType, & cbType);

#ifdef _DEBUG
	WAVEFORMATEX * pWfx = (WAVEFORMATEX *) pType->pbFormat;
	DWORD * pExt = (DWORD *) (pType->pbFormat + sizeof WAVEFORMATEX);
	if (TRACE_WMA_DECODER) TRACE(_T("Format: %d, BytesPerSec: %d, BlockAlign: %d, cbSize: %d\n"),
								pWfx->wFormatTag, pWfx->nAvgBytesPerSec,
								pWfx->nBlockAlign, pWfx->cbSize);
	if (TRACE_WMA_DECODER) TRACE(_T("FormatExtension: %08X, %08X, %08X\n"), pExt[0], pExt[1], pExt[2]);

#endif
	pType->pbFormat = PBYTE(pDstWfx);
	pType->cbFormat = sizeof (WAVEFORMATEX) + pDstWfx->cbSize;

	if (TRACE_WMA_DECODER) TRACE(_T("MediaType: wFormatTag=%d, BytesPerSec = %d\n"),
								pDstWfx->wFormatTag, pDstWfx->nAvgBytesPerSec);

	hr = pStreamConfig->SetBitrate(pDstWfx->nAvgBytesPerSec * 8);
	hr = pProps->SetMediaType(pType);

	hr = pProfile->ReconfigStream(pStreamConfig);

	if (TRACE_WMA_DECODER) TRACE(_T("ReconfigStream returned %X\n"), hr);

	hr = m_pWriter->SetProfile(pProfile);
	delete[] pBuf;

	return SUCCEEDED(hr);
}

BOOL WmaEncoder::Write(void const * Buf, unsigned size)
{
	UCHAR const * pSrcBuf = static_cast<UCHAR const *>(Buf);

	if (0 == size)
	{
		pSrcBuf = NULL;
		if ( ! m_pBuffer)
		{
			return TRUE;
		}
	}
	do
	{

		if ( ! m_pBuffer)
		{
			HRESULT hr = m_pWriter->AllocateSample(m_SrcWfx.BytesPerSec(), & m_pBuffer);
			if (! SUCCEEDED(hr))
			{
				return FALSE;
			}
			m_pBuffer->SetLength(0);
		}

		DWORD BufLength = 0;
		DWORD MaxLength = 0;
		BYTE * pBuf;
		m_pBuffer->GetBufferAndLength( & pBuf, & BufLength);
		m_pBuffer->GetMaxLength( & MaxLength);

		if (pSrcBuf != NULL)
		{
			DWORD ToCopy = MaxLength - BufLength;
			if (ToCopy > size)
			{
				ToCopy = (DWORD)size;
			}
			memcpy(pBuf + BufLength, pSrcBuf, ToCopy);

			BufLength += ToCopy;
			size -= ToCopy;
			pSrcBuf += ToCopy;

			m_pBuffer->SetLength(BufLength);
		}

		if ((NULL == pSrcBuf && 0 != BufLength)
			|| BufLength == MaxLength
			)
		{
#ifdef _DEBUG
			QWORD ActualWriterTime;
			m_pWriterAdvanced->GetWriterTime( & ActualWriterTime);

			if (TRACE_WMA_DECODER) TRACE(_T("Writing src buf %p, time=%d ms, ActualWriterTime=%d ms\n"),
										m_pBuffer, DWORD(m_SampleTime100ns / 10000), DWORD(ActualWriterTime / 10000));
#endif
			if (! SUCCEEDED(m_pWriter->WriteSample(0, m_SampleTime100ns, 0, m_pBuffer)))
			{
				return FALSE;
			}
			//Sleep(50);
			m_SampleTime100ns += (1000*1000*10Ui64) * BufLength / m_SrcWfx.BytesPerSec();

			m_pBuffer.Release();
		}
	} while(size != 0);

	return TRUE;
}

HRESULT STDMETHODCALLTYPE FileWriter::OnHeader(
												/* [in] */ INSSBuffer __RPC_FAR *pHeader)
{
	if (TRACE_WMA_DECODER) TRACE(_T("FileWriter::OnHeader\n"));
	m_DstFile.Seek(0, FILE_BEGIN);
	return OnDataUnit(pHeader);
}

HRESULT STDMETHODCALLTYPE FileWriter::IsRealTime(
												/* [out] */ BOOL __RPC_FAR *pfRealTime)
{
	if (NULL != pfRealTime)
	{
		* pfRealTime = FALSE;
		return S_OK;
	}
	else
	{
		return E_POINTER;
	}
}

namespace
{
class NSSBuffer : public INSSBuffer
{

	LONG_volatile RefCount;
	DWORD BufLength;
	DWORD MaxLength;
	BYTE * pBuf;

public:
	NSSBuffer(DWORD Length)
		: RefCount(1),
		BufLength(Length),
		MaxLength(Length),
		pBuf(new BYTE[Length])
	{
	}
private:
	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == IID_IUnknown)
		{
			*ppvObject = static_cast<IUnknown*>(this);
		}
		else if ( riid == IID_INSSBuffer )
		{
			*ppvObject = static_cast<INSSBuffer*>(this);
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void )
	{
		return ++RefCount;
	}

	ULONG STDMETHODCALLTYPE Release( void )
	{
		LONG Ref = --RefCount;
		if (0 == Ref)
		{
			delete this;
		}
		return Ref;
	}
	virtual HRESULT STDMETHODCALLTYPE GetLength(
												/* [out] */ DWORD __RPC_FAR *pdwLength)
	{
		if (NULL == pdwLength)
		{
			return E_POINTER;
		}
		* pdwLength = BufLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetLength(
												/* [in] */ DWORD dwLength)
	{
		if (dwLength > MaxLength)
		{
			return E_INVALIDARG;
		}
		BufLength = dwLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMaxLength(
													/* [out] */ DWORD __RPC_FAR *pdwLength)
	{
		if (NULL == pdwLength)
		{
			return E_POINTER;
		}
		* pdwLength = MaxLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(
												/* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppdwBuffer)
	{
		if (NULL == ppdwBuffer)
		{
			return E_POINTER;
		}
		* ppdwBuffer = pBuf;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBufferAndLength(
														/* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppdwBuffer,
														/* [out] */ DWORD __RPC_FAR *pdwLength)
	{
		if (NULL == ppdwBuffer
			|| NULL == pdwLength)
		{
			return E_POINTER;
		}
		* pdwLength = BufLength;
		* ppdwBuffer = pBuf;
		return S_OK;
	}
	~NSSBuffer()
	{
		delete[] pBuf;
	}
};
}

HRESULT STDMETHODCALLTYPE FileWriter::AllocateDataUnit(
														/* [in] */ DWORD cbDataUnit,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit)
{
	if (NULL == ppDataUnit)
	{
		return E_POINTER;
	}
	* ppDataUnit = new NSSBuffer(cbDataUnit);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileWriter::OnDataUnit(
												/* [in] */ INSSBuffer __RPC_FAR *pDataUnit)
{
	PBYTE pData;
	DWORD Length;
	if (NULL == pDataUnit)
	{
		return E_POINTER;
	}

	pDataUnit->GetBufferAndLength( & pData, & Length);
	if (TRACE_WMA_DECODER) TRACE(_T("FileWriter::OnDataUnit buf = %p, Length=%d\n"),
								pData, Length);
	if (NULL == pData)
	{
		return E_POINTER;
	}

	if (Length == (DWORD) m_DstFile.Write(pData, Length))
	{
		ULONGLONG CurPos = m_DstFile.Seek(0, FILE_CURRENT);
		if (CurPos > m_WrittenLength)
		{
			m_WrittenLength = CurPos;
		}
		//pDataUnit->SetLength(0); //???

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT STDMETHODCALLTYPE FileWriter::OnEndWriting( void)
{
	if (TRACE_WMA_DECODER) TRACE(_T("OnEndWriting\n"));
	return S_OK;
}

#if 1
HRESULT STDMETHODCALLTYPE CWmaDecoderAsync::OnStatus( /* [in] */ WMT_STATUS Status,
																/* [in] */ HRESULT hr,
																/* [in] */ WMT_ATTR_DATATYPE dwType,
																/* [in] */ BYTE __RPC_FAR *pValue,
																/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	// The following values are used:
	// WMT_OPENED
	// WMT_STOPPED
	// WMT_STARTED
	// WMT_END_OF_FILE
	// WMT_ERROR
	switch (Status)
	{
	case WMT_OPENED:
		TRACE(_T("CWmaDecoder::OnStatus WMT_OPENED, HRESULT=%X\n"), hr);
		if (SUCCEEDED(hr))
		{
			m_bOpened = true;
		}
		m_OpenedEvent.Set();
		break;
	case WMT_STARTED:
		TRACE(_T("CWmaDecoder::OnStatus WMT_STARTED, HRESULT=%X\n"), hr);
		m_bStarted = true;
		m_StartedEvent.Set();
		break;

	case WMT_STOPPED:
		TRACE(_T("CWmaDecoder::OnStatus WMT_STOPPED, HRESULT=%X\n"), hr);
		m_bStarted = false;
		m_StartedEvent.Set();
		break;

	case WMT_ERROR:
		TRACE(_T("CWmaDecoder::OnStatus WMT_ERROR, HRESULT=%X\n"), hr);
		break;
		m_bStarted = false;
		m_StartedEvent.Set();
		break;

	case WMT_END_OF_FILE:
		TRACE(_T("CWmaDecoder::OnStatus WMT_END_OF_FILE, HRESULT=%X\n"), hr);
		//m_bStarted = false;
		m_StartedEvent.Set();
		m_OpenedEvent.Set();
		if (m_Reader)
		{
			m_Reader->Stop();
		}
		break;

	case WMT_END_OF_STREAMING:
		TRACE(_T("CWmaDecoder::OnStatus WMT_END_OF_STREAMING, HRESULT=%X\n"), hr);
		//            m_bStarted = false;
		m_StartedEvent.Set();
		m_OpenedEvent.Set();
		if (m_Reader)
		{
			m_Reader->Stop();
		}
		break;

	default:
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnStatus Status=%X, hr=%08X, DataType=%X, pValue=%X\n"),
									Status, hr, dwType, pValue);
		;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoderAsync::OnSample( /* [in] */ DWORD dwOutputNum,
																/* [in] */ QWORD cnsSampleTime,
																/* [in] */ QWORD cnsSampleDuration,
																/* [in] */ DWORD /*dwFlags*/,
																/* [in] */ INSSBuffer __RPC_FAR *pSample,
																/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	//
	// Make sure its Audio sample
	//
	if (m_dwAudioOutputNum != dwOutputNum)
	{
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnSample m_dwAudioOutputNum != dwOutputNum\n"));
		return S_OK;
	}

	if (!IsStarted())
	{
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnSample: ! IsStarted()\n"));
		return S_OK;
	}

	HRESULT hr = S_OK;
	BYTE *pData = NULL;
	DWORD cbData = 0;

	hr = pSample->GetBufferAndLength(&pData, &cbData);

	if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnSample, time=%d ms, %d bytes, hr=%X\n"),
								DWORD(cnsSampleTime / 10000), cbData, hr);

	if (FAILED(hr))
	{
		return hr;
	}
	m_DstFile.CDirectFile::Write(pData, cbData);

	// update current number of samples
	SAMPLE_POSITION DstCopyPos = (SAMPLE_POSITION)m_DstFile.CDirectFile::Seek(0, FILE_CURRENT);
	SAMPLE_INDEX DstCopySample = m_DstFile.PositionToSample(DstCopyPos);

	if (DstCopySample > m_CurrentSamples)
	{
		// calculate new length
		NUMBER_OF_SAMPLES MaxNumberOfSamples = 0x7FFFFFFF / m_DstFile.SampleSize();
		LONGLONG TotalSamplesEstimated = ULONG(double(DstCopySample) * SrcLength() / SrcPos());

		if (TotalSamplesEstimated > MaxNumberOfSamples)
		{
			TotalSamplesEstimated = MaxNumberOfSamples;
		}
		if (NUMBER_OF_SAMPLES(TotalSamplesEstimated) < m_CurrentSamples)
		{
			TotalSamplesEstimated = m_CurrentSamples;
		}

		m_CurrentSamples = NUMBER_OF_SAMPLES(TotalSamplesEstimated);

		m_DstFile.SetFileLengthSamples(m_CurrentSamples);
	}
	// modify positions after file length modified,
	m_DstPos = DstCopyPos;  // to avoid race condition
	m_DstCopySample = DstCopySample;

	// ask for next buffer
	m_CurrentStreamTime = cnsSampleTime + cnsSampleDuration;

	m_bNeedNextSample = true;
	m_SampleEvent.Set();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoderAsync::OnTime(
													/* [in] */ QWORD cnsCurrentTime,
													/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	if (0 || TRACE_WMA_DECODER) TRACE(_T("IWMReaderCallbackAdvancedOnTime(%I64d)\n"), cnsCurrentTime);
	// ask for next buffer
	m_CurrentStreamTime = cnsCurrentTime;

	m_bNeedNextSample = true;
	m_SampleEvent.Set();
	return S_OK;
}

void CWmaDecoderAsync::DeliverNextSample(DWORD timeout)
{
	if (m_bNeedNextSample)
	{
		m_bNeedNextSample = false;
		if (m_pAdvReader)
		{
			QWORD NextSampleTime = m_CurrentStreamTime + m_BufferLengthTime;
			if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::DeliverNextSample:  m_CurrentStreamTime=%I64d, NextTime=%I64d\n"),
										m_CurrentStreamTime, NextSampleTime);

			m_pAdvReader->DeliverTime(NextSampleTime);
		}
		else
		{
			if (TRACE_WMA_DECODER) TRACE(_T("NULL == m_pAdvReader, m_Reader = %X\n"), m_Reader);
		}
	}
	else
	{
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::DeliverNextSample:  ! m_bNeedNextSample\n"));
	}
	if (0 != timeout)
	{
		WaitForSingleObject(m_SampleEvent, timeout);
	}
}

#if USE_READER_CALLBACK_ADVANCED
CWmaDecoderAsync::CWmaDecoderAsync()
	: //ReaderStatus(WMT_ERROR),
	m_CurrentStreamTime(0),
	m_bNeedNextSample(false),
	m_BufferLengthTime(0),
	m_Bitrate(1),
	m_bOpened(false),
	m_bStarted(false),
	m_OpenedEvent(FALSE, FALSE),
	m_StartedEvent(FALSE, FALSE),
	m_SampleEvent(FALSE, FALSE),
	m_dwAudioOutputNum(0)
{
}

CWmaDecoderAsync::~CWmaDecoderAsync()
{
}

void CWmaDecoderAsync::DeInit()
{
	m_pAdvReader.Release();
	m_Reader.Release();
}

HRESULT STDMETHODCALLTYPE CWmaDecoderAsync::AllocateForStream(
															/* [in] */ WORD /*wStreamNum*/,
															/* [in] */ DWORD cbBuffer,
															/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
															/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	if (NULL == ppBuffer)
	{
		return E_POINTER;
	}
	*ppBuffer = new NSSBuffer(cbBuffer);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoderAsync::AllocateForOutput(
															/* [in] */ DWORD /*dwOutputNum*/,
															/* [in] */ DWORD cbBuffer,
															/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
															/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	if (NULL == ppBuffer)
	{
		return E_POINTER;
	}
	*ppBuffer = new NSSBuffer(cbBuffer);
	return S_OK;
}

#endif
#endif
CDirectShowDecoder::CDirectShowDecoder(CDirectShowDecoderDataSink * DataSink)
	: m_FilterState(State_Stopped)
	, m_DecoderState(DecoderStateUninitialized)
	, m_DataSink(DataSink)
{

}
CDirectShowDecoder::~CDirectShowDecoder()
{

}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryInterface(
															/* [in] */ REFIID riid,
															/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (riid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown*>(static_cast<IBaseFilter*>(this));
	}
	else if (riid == IID_IPin)
	{
		*ppvObject = static_cast<IPin*>(this);
	}
	else if (riid == IID_IBaseFilter)
	{
		*ppvObject = static_cast<IBaseFilter*>(this);
	}
	else if (riid == IID_IMediaFilter)
	{
		*ppvObject = static_cast<IMediaFilter*>(this);
	}
	else if (riid == IID_IPersist)
	{
		*ppvObject = static_cast<IPersist*>(this);
	}
	else if (riid == IID_IMemInputPin)
	{
		*ppvObject = static_cast<IMemInputPin*>(this);
	}
	else
	{
		return E_NOINTERFACE;
	}
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CDirectShowDecoder::AddRef(void)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CDirectShowDecoder::Release(void)
{
	return 1;
}
// IPin overrides:
HRESULT STDMETHODCALLTYPE CDirectShowDecoder::Connect(
													/* [in] */ IPin *pReceivePin,
	/* [annotation][in] */
													_In_opt_  const AM_MEDIA_TYPE *pmt)
{
	if (m_ConnectedPin)
	{
		return VFW_E_ALREADY_CONNECTED;
	}
	m_ConnectedPin = pReceivePin;

	m_AsyncReader = pReceivePin;

	if (!m_AsyncReader)
	{
		m_ConnectedPin.Release();
		return E_NOINTERFACE;
	}
	// FIXME
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::ReceiveConnection(
																/* [in] */ IPin *pConnector,
																/* [in] */ const AM_MEDIA_TYPE *pmt)
{
	if (NULL == pConnector
		|| NULL == pmt)
	{
		return E_POINTER;
	}
	if (m_ConnectedPin)
	{
		return VFW_E_ALREADY_CONNECTED;
	}
	// this is called on an input pin
	if (pmt->majortype == MEDIATYPE_Audio
		&& pmt->subtype == MEDIASUBTYPE_PCM
		&& pmt->formattype == FORMAT_WaveFormatEx)
	{
		// verify other format qualifiers
		m_ConnectedPin = pConnector;
		//m_AsyncReader = pConnector;

		m_ConnectedMediaType.Set(*pmt);
		m_DstWf = (PWAVEFORMATEX)m_ConnectedMediaType.Format();
		return S_OK;
	}
	else
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::Disconnect(void)
{
	if (!m_ConnectedPin)
	{
		return S_FALSE;
	}
	m_ConnectedPin.Release();
	m_ConnectedMediaType.ResetFormatBuffer();
	m_ConnectedMediaType.InitMediaType();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::ConnectedTo(
	/* [annotation][out] */
														_Out_  IPin **pPin)
{
	if (pPin == NULL)
	{
		return E_POINTER;
	}
	if (!m_ConnectedPin)
	{
		return VFW_E_NOT_CONNECTED;
	}
	*pPin = m_ConnectedPin;
	(*pPin)->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::ConnectionMediaType(
	/* [annotation][out] */
																_Out_  AM_MEDIA_TYPE *pmt)
{
	if (!m_ConnectedPin)
	{
		return VFW_E_NOT_CONNECTED;
	}
	if (pmt == NULL)
	{
		return E_POINTER;
	}
	return CopyMediaType(pmt, &m_ConnectedMediaType);
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryPinInfo(
	/* [annotation][out] */
															_Out_  PIN_INFO *pInfo)
{
	if (NULL == pInfo)
	{
		return E_POINTER;
	}
	wcsncpy(pInfo->achName, L"WaveSoap consumer pin", countof(pInfo->achName));
	pInfo->dir = PINDIR_INPUT;
	pInfo->pFilter = this;
	AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryDirection(
	/* [annotation][out] */
															_Out_  PIN_DIRECTION *pPinDir)
{
	if (NULL == pPinDir)
	{
		return E_POINTER;
	}
	*pPinDir = PINDIR_INPUT;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryId(
	/* [annotation][out] */
													_Out_  LPWSTR *Id)
{
	if (NULL == Id)
	{
		return E_POINTER;
	}
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof L"WaveSoap_pin");
	if (NULL == *Id)
	{
		return E_OUTOFMEMORY;
	}
	memcpy(*Id, L"WaveSoap_pin", sizeof L"WaveSoap_pin");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryAccept(
														/* [in] */ const AM_MEDIA_TYPE *pmt)
{
	if (pmt->majortype != MEDIATYPE_Audio
		|| pmt->formattype != FORMAT_WaveFormatEx)
	{
		return S_FALSE;
	}

	PWAVEFORMATEX pwf = (PWAVEFORMATEX)pmt->pbFormat;
	PWAVEFORMATEXTENSIBLE pwfx = (PWAVEFORMATEXTENSIBLE)pwf;

	if (pmt->subtype == MEDIASUBTYPE_PCM)
	{
		// accept 16, 24 32 bit, from 1 to 32 channels, from 1 to 100000 samples per second
		if (NULL == pwf
			|| pmt->cbFormat < sizeof(WAVEFORMATEX))
		{
			return S_FALSE;
		}

		if (pwf->wFormatTag != WAVE_FORMAT_PCM
			&& pwf->wFormatTag != WAVE_FORMAT_EXTENSIBLE
			&& pwfx->SubFormat != KSDATAFORMAT_SUBTYPE_PCM)
		{
			return S_FALSE;
		}

		if (pwf->wBitsPerSample != 16
			&& pwf->wBitsPerSample != 24
			&& pwf->wBitsPerSample != 32)
		{
			return S_FALSE;
		}

		if (pwf->nChannels < 1 || pwf->nChannels > 32)
		{
			return S_FALSE;
		}

		if (pwf->nSamplesPerSec < 1 || pwf->nSamplesPerSec > 1000000)
		{
			return S_FALSE;
		}
		return S_OK;
	}
	else if (pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT)
	{
		if (pwf->wFormatTag != WAVE_FORMAT_IEEE_FLOAT
			&& pwf->wFormatTag != WAVE_FORMAT_EXTENSIBLE
			&& pwfx->SubFormat != KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			return S_FALSE;
		}

		if (pwf->wBitsPerSample != 32)
		{
			return S_FALSE;
		}

		if (pwf->nChannels < 1 || pwf->nChannels > 32)
		{
			return S_FALSE;
		}

		if (pwf->nSamplesPerSec < 1 || pwf->nSamplesPerSec > 1000000)
		{
			return S_FALSE;
		}
		return S_FALSE;
	}
	else
	{
		return S_FALSE;
	}
}

class SingleFormatMediaTypeEnum : public IEnumMediaTypes
{
public:
	SingleFormatMediaTypeEnum(AM_MEDIA_TYPE const & mt, int start_index =0);
private:
	virtual ~SingleFormatMediaTypeEnum() {}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
													/* [in] */ REFIID riid,
													/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

	virtual ULONG STDMETHODCALLTYPE AddRef(void);

	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE Next(
											/* [in] */ ULONG cMediaTypes,
		/* [annotation][size_is][out] */
											_Out_writes_to_(cMediaTypes, *pcFetched)  AM_MEDIA_TYPE **ppMediaTypes,
		/* [annotation][out] */
											_Out_opt_  ULONG *pcFetched);

	virtual HRESULT STDMETHODCALLTYPE Skip(
											/* [in] */ ULONG cMediaTypes);

	virtual HRESULT STDMETHODCALLTYPE Reset(void);

	virtual HRESULT STDMETHODCALLTYPE Clone(
		/* [annotation][out] */
											_Out_  IEnumMediaTypes **ppEnum);
	CMediaType type;
	LONG RefCount;
public:
	int enum_index;
};

class SinglePinEnum : public IEnumPins
{
public:
	SinglePinEnum(IPin * p, int start_index = 0);
private:
	virtual ~SinglePinEnum() {}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
													/* [in] */ REFIID riid,
													/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

	virtual ULONG STDMETHODCALLTYPE AddRef(void);

	virtual ULONG STDMETHODCALLTYPE Release(void);

	virtual HRESULT STDMETHODCALLTYPE Next(
											/* [in] */ ULONG cPins,
		/* [annotation][size_is][out] */
											_Out_writes_to_(cPins, *pcFetched)  IPin **ppPins,
		/* [annotation][out] */
											_Out_opt_  ULONG *pcFetched);

	virtual HRESULT STDMETHODCALLTYPE Skip(
											/* [in] */ ULONG cPins);

	virtual HRESULT STDMETHODCALLTYPE Reset(void);

	virtual HRESULT STDMETHODCALLTYPE Clone(
		/* [annotation][out] */
											_Out_  IEnumPins **ppEnum);

	CComPtr<IPin> pin;
	LONG RefCount;
public:
	int enum_index;
};


HRESULT STDMETHODCALLTYPE CDirectShowDecoder::EnumMediaTypes(
	/* [annotation][out] */
															_Out_  IEnumMediaTypes **ppEnum)
{
	if (NULL == ppEnum)
	{
		return E_POINTER;
	}
	if (!m_ConnectedPin)
	{
		return VFW_E_NOT_CONNECTED;
	}
	try
	{
		*ppEnum = new SingleFormatMediaTypeEnum(m_ConnectedMediaType);
		return S_OK;
	}
	catch (std::bad_alloc&)
	{
		return E_OUTOFMEMORY;
	}
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryInternalConnections(
	/* [annotation][out] */
	_Out_writes_to_opt_(*nPin, *nPin)  IPin **apPin,
	/* [out][in] */ ULONG *nPin)
{
	*nPin = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::EndOfStream(void)
{
	TRACE(L"CDirectShowDecoder::EndOfStream(void)\n");
	if (m_DataSink)
	{
		m_DataSink->EndOfStream();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::BeginFlush(void)
{
	TRACE(L"CDirectShowDecoder::BeginFlush(void)\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::EndFlush(void)
{
	TRACE(L"CDirectShowDecoder::EndFlush(void)\n");
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::NewSegment(
														/* [in] */ REFERENCE_TIME tStart,
														/* [in] */ REFERENCE_TIME tStop,
														/* [in] */ double dRate)
{
	return S_OK;
}

// IBaseFilter overrides
HRESULT STDMETHODCALLTYPE CDirectShowDecoder::EnumPins(
	/* [annotation][out] */
														_Out_  IEnumPins **ppEnum)
{
	if (NULL == ppEnum)
	{
		return E_POINTER;
	}
	try
	{
		*ppEnum = new SinglePinEnum(this);
		return S_OK;
	}
	catch (std::bad_alloc&)
	{
		return E_OUTOFMEMORY;
	}
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::FindPin(
													/* [string][in] */ LPCWSTR Id,
	/* [annotation][out] */
													_Out_  IPin **ppPin)
{
	if (NULL == Id
		|| NULL == ppPin)
	{
		return E_POINTER;
	}

	if (0 == wcscmp(Id, L"WaveSoap_pin"))
	{
		*ppPin = this;
		AddRef();
		return S_OK;
	}
	return VFW_E_NOT_FOUND;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryFilterInfo(
	/* [annotation][out] */
															_Out_  FILTER_INFO *pInfo)
{
	if (NULL == pInfo)
	{
		return E_POINTER;
	}
	wcsncpy(pInfo->achName, m_FilterName, countof(pInfo->achName));
	pInfo->pGraph = m_FilterGraph;
	if (pInfo->pGraph)
	{
		pInfo->pGraph->AddRef();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::JoinFilterGraph(
	/* [annotation][in] */
															_In_opt_  IFilterGraph *pGraph,
	/* [annotation][string][in] */
															_In_opt_  LPCWSTR pName)
{
	if (pGraph)
	{
		m_FilterGraph = pGraph;
		m_FilterName = pName;
	}
	else
	{
		m_FilterGraph.Release();
		m_FilterName.Empty();
	}
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CDirectShowDecoder::QueryVendorInfo(
	/* [annotation][string][out] */
															_Out_  LPWSTR *pVendorInfo)
{
	*pVendorInfo = (LPWSTR)CoTaskMemAlloc(sizeof L"Private");

	if (*pVendorInfo)
	{
		memcpy(*pVendorInfo, L"Private", sizeof L"Private");
		return S_OK;
	}
	return E_OUTOFMEMORY;
}

////////////////////////////////////////////////////////////////
// IMediaFilter overrides
HRESULT STDMETHODCALLTYPE CDirectShowDecoder::Stop(void)
{
	TRACE(L"CDirectShowDecoder::Stop(void)\n");
	m_FilterState = State_Stopped;
	m_DecoderState = DecoderStateStopped;
	if (m_DataSink)
	{
		m_DataSink->Stop();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::Pause(void)
{
	TRACE(L"CDirectShowDecoder::Pause(void)\n");
	m_FilterState = State_Paused;
	m_DecoderState = DecoderStatePaused;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::Run(
												REFERENCE_TIME tStart)
{
	TRACE(L"CDirectShowDecoder::Run(REFERENCE_TIME tStart)\n");
	m_FilterState = State_Running;
	m_DecoderState = DecoderStateRunning;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::GetState(
														/* [in] */ DWORD dwMilliSecsTimeout,
	/* [annotation][out] */
														_Out_  FILTER_STATE *State)
{
	*State = m_FilterState;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::SetSyncSource(
	/* [annotation][in] */
															_In_opt_  IReferenceClock *pClock)
{
	if (pClock)
	{
		m_RefClock = pClock;
	}
	else
	{
		m_RefClock.Release();
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::GetSyncSource(
	/* [annotation][out] */
															_Outptr_result_maybenull_  IReferenceClock **pClock)
{
	if (NULL == pClock)
	{
		return E_POINTER;
	}
	*pClock = m_RefClock;
	if (*pClock)
	{
		(*pClock)->AddRef();
	}
	return S_OK;
}
///////////////////////////////////////////////////////////////////////
// IPersist overrides
HRESULT STDMETHODCALLTYPE CDirectShowDecoder::GetClassID(
														/* [out] */ __RPC__out CLSID *pClassID)
{
	return E_FAIL;
}

/////////////////////////////////////////////////////////////////////
// IMemInputPin overrides
HRESULT STDMETHODCALLTYPE CDirectShowDecoder::GetAllocator(
	/* [annotation][out] */
															_Out_  IMemAllocator **ppAllocator)
{
	return VFW_E_NO_ALLOCATOR;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::NotifyAllocator(
															/* [in] */ IMemAllocator *pAllocator,
															/* [in] */ BOOL bReadOnly)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::GetAllocatorRequirements(
	/* [annotation][out] */
	_Out_  ALLOCATOR_PROPERTIES *pProps)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::Receive(
													/* [in] */ IMediaSample *pSample)
{
	// write the data immediately
	// need to do AddRef/Release to work around buggy sources which don't do it properly when calling Receive
	HRESULT hr = S_OK;
	pSample->AddRef();
	if (m_DataSink)
	{
		hr = m_DataSink->Receive(pSample);
	}
	pSample->Release();

	return hr;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::ReceiveMultiple(
	/* [annotation][size_is][in] */
															_In_reads_(nSamples)  IMediaSample **pSamples,
															/* [in] */ long nSamples,
	/* [annotation][out] */
															_Out_  long *nSamplesProcessed)
{
	HRESULT hr = S_OK;
	for (long i = 0; i < nSamples; i++)
	{
		HRESULT hr1 = Receive(pSamples[i]);
		if (hr1 != S_OK)
		{
			hr = hr1;
		}
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CDirectShowDecoder::ReceiveCanBlock(void)
{
	return S_OK;
}


BOOL CDirectShowDecoder::Init()
{
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL,
								CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&m_GraphBuilder);
	if (FAILED(hr))
	{
		return FALSE;
	}
	m_DecoderState = DecoderStateInitialized;
	return TRUE;
}
// Release the format block for a media type.

void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// pUnk should not be used.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

// Delete a media type structure that was allocated on the heap.
void _DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL)
	{
		_FreeMediaType(*pmt);
		CoTaskMemFree(pmt);
	}
}

HRESULT CDirectShowDecoder::Open(LPCWSTR szFilename)
{
	CComPtr<IBaseFilter> SourceFilter;

	HRESULT hr = m_GraphBuilder->AddSourceFilter(szFilename, L"SourceFilter", &SourceFilter);
	if (FAILED(hr))
	{
		return hr;
	}

	hr = m_GraphBuilder->AddFilter(this, L"WaveSoapSink");
	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IEnumPins> PinEnum;
	hr = SourceFilter->EnumPins(&PinEnum);

	if (FAILED(hr))
	{
		return hr;
	}

	CComPtr<IPin> Pin, PinToConnect;

	CMediaType ChosenMediaType;

	for (bool bBreakPinEnum = false; !bBreakPinEnum && PinEnum->Next(1, &Pin, 0) == S_OK; Pin.Release())
	{
		PIN_DIRECTION PinDirThis;
		hr = Pin->QueryDirection(&PinDirThis);
		if (FAILED(hr))
		{
			return hr;
		}
		PIN_INFO pin_info = { 0 };
		if (SUCCEEDED(Pin->QueryPinInfo(&pin_info)))
		{
			TRACE(L"Found pin with direction %d, name %s\n", pin_info.dir, pin_info.achName);
			if (NULL != pin_info.pFilter)
			{
				pin_info.pFilter->Release();
			}
		}
		if (PinDirThis != PINDIR_OUTPUT)
		{
			continue;
		}
		CComPtr<IEnumMediaTypes> EnumTypes;
		hr = Pin->EnumMediaTypes(&EnumTypes);

		if (!SUCCEEDED(hr))
		{
			continue;
		}
		AM_MEDIA_TYPE * pMediaType;

		for (bool bBreakMediaTypeEnum = false; !bBreakMediaTypeEnum && EnumTypes->Next(1, &pMediaType, NULL) == S_OK; _DeleteMediaType(pMediaType))
		{
			if (S_OK != QueryAccept(pMediaType))
			{
				continue;
			}

			PWAVEFORMATEX wf = (PWAVEFORMATEX)pMediaType->pbFormat;
			//PWAVEFORMATEXTENSIBLE wfx = (PWAVEFORMATEXTENSIBLE)pMediaType->pbFormat;
			PWAVEFORMATEX prev_wf = (PWAVEFORMATEX)ChosenMediaType.Format();
			//PWAVEFORMATEXTENSIBLE prev_wfx = (PWAVEFORMATEXTENSIBLE)ChosenMediaType.Format();

			if (pMediaType->subtype == MEDIASUBTYPE_IEEE_FLOAT)
			{
				TRACE(L"Media type audio, subtype FLOAT, FormatTag = %04X, Sample rate=%d, bits per sample=%d,channels=%d, block align=%d\n",
					wf->wFormatTag, wf->nSamplesPerSec, wf->wBitsPerSample, wf->nChannels, wf->nBlockAlign);
			}
			else if (pMediaType->subtype == MEDIASUBTYPE_PCM)
			{
				TRACE(L"Media type audio, subtype PCM, FormatTag = %04X, Sample rate=%d, bits per sample=%d,channels=%d, block align=%d\n",
					wf->wFormatTag, wf->nSamplesPerSec, wf->wBitsPerSample, wf->nChannels, wf->nBlockAlign);
			}
			// choosing preferred format.
			// higher number of bits, sample rate and number of channels preferred
			// Float is preferred over integer, WAVEFORMATEXTENSIBLE is preferred over WAVEFORMATEX,
			if (ChosenMediaType.majortype == MEDIATYPE_Audio)
			{

				if (wf->nSamplesPerSec < prev_wf->nSamplesPerSec)
				{
					continue;
				}
				else if (wf->nSamplesPerSec > prev_wf->nSamplesPerSec)
				{
					// prefer this format, fall through
				}
				else if (wf->nChannels < prev_wf->nChannels)
				{
					continue;
				}
				else if (wf->nChannels > prev_wf->nChannels)
				{
					// prefer this format, fall through
				}
				else if (pMediaType->subtype == MEDIASUBTYPE_PCM
						&& ChosenMediaType.subtype == MEDIASUBTYPE_IEEE_FLOAT)
				{
					// float is preferred over integer
					continue;
				}
				else if (pMediaType->subtype == MEDIASUBTYPE_IEEE_FLOAT
						&& ChosenMediaType.subtype == MEDIASUBTYPE_PCM)
				{
					// prefer this format, fall through
				}
				else if (wf->wBitsPerSample < prev_wf->wBitsPerSample)
				{
					continue;
				}
				else if (wf->wBitsPerSample > prev_wf->wBitsPerSample)
				{
					// prefer this format, fall through
				}
				else if (wf->wFormatTag != WAVE_FORMAT_EXTENSIBLE && prev_wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
				{
					continue;
				}
				else
				{
					// the new format tag is either same as previous, or extensible
				}
			}
			ChosenMediaType.ResetFormatBuffer();
			ChosenMediaType = *pMediaType;
			PinToConnect.Release();
			PinToConnect = Pin;
		}
	}

	if (ChosenMediaType.majortype == MEDIATYPE_Audio
		&& SUCCEEDED(m_GraphBuilder->ConnectDirect(PinToConnect, this, &ChosenMediaType)))
	{
		PIN_INFO pin_info = { 0 };
		if (0 && S_OK == PinToConnect->QueryPinInfo(&pin_info))
		{
			if (pin_info.pFilter)
			{
				pin_info.pFilter->SetSyncSource(NULL);
				pin_info.pFilter->Release();
			}
		}
		TRACE(L"Connected to the pin!\n");
		m_DecoderState = DecoderStateOpened;
		return S_OK;
	}
	return E_FAIL;
}

void CDirectShowDecoder::Close()
{
	HRESULT hr;
	if (m_GraphBuilder)
	{
		if (m_ConnectedPin)
		{
			hr = m_GraphBuilder->Disconnect(m_ConnectedPin);
			hr = m_GraphBuilder->Disconnect(this);
			m_ConnectedPin.Release();	// it's probably done in Disconnect
		}
		if (m_FilterGraph)
		{
			m_FilterGraph->RemoveFilter(this);
		}
		m_GraphBuilder.Release();
		m_AsyncReader.Release();
	}
}

void CDirectShowDecoder::DeInit()
{
	Close();
}

NUMBER_OF_SAMPLES CDirectShowDecoder::GetTotalSamples() const
{
	NUMBER_OF_SAMPLES Samples = 0;
	CComQIPtr<IMediaSeeking> pSeek = m_ConnectedPin;
	if (pSeek)
	{
		LONGLONG StreamDuration = 0;
		if (SUCCEEDED(pSeek->GetDuration(&StreamDuration)))
		{
			Samples = NUMBER_OF_SAMPLES(StreamDuration * m_DstWf.SampleRate() / 10000000);
		}
	}
	return Samples;
}

HRESULT CDirectShowDecoder::StartDecode()
{
	CComQIPtr<IMediaControl> MediaControl = m_GraphBuilder;
	if (MediaControl)
	{
		return MediaControl->Run();
	}
	return E_NOINTERFACE;
}

HRESULT CDirectShowDecoder::StopDecode()
{
	CComQIPtr<IMediaControl> MediaControl = m_GraphBuilder;
	if (MediaControl)
	{
		return MediaControl->Stop();
	}
	return E_NOINTERFACE;
}

SingleFormatMediaTypeEnum::SingleFormatMediaTypeEnum(AM_MEDIA_TYPE const & mt, int start_index)
	: RefCount(1)
	, enum_index(start_index)
	, type(mt)
{

}

HRESULT STDMETHODCALLTYPE SingleFormatMediaTypeEnum::QueryInterface(
	/* [in] */ REFIID riid,
	/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (riid == IID_IEnumMediaTypes)
	{
		*ppvObject = static_cast<IEnumMediaTypes*>(this);
		AddRef();
		return S_OK;
	}
	if (riid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE SingleFormatMediaTypeEnum::AddRef(void)
{
	return InterlockedIncrement(&RefCount);
}

ULONG STDMETHODCALLTYPE SingleFormatMediaTypeEnum::Release(void)
{
	LONG ref = InterlockedDecrement(&RefCount);
	if (ref == 0)
	{
		delete this;
	}
	return ref;
}

HRESULT STDMETHODCALLTYPE SingleFormatMediaTypeEnum::Next(
														/* [in] */ ULONG cMediaTypes,
	/* [annotation][size_is][out] */
														_Out_writes_to_(cMediaTypes, *pcFetched)  AM_MEDIA_TYPE **ppMediaTypes,
	/* [annotation][out] */
														_Out_opt_  ULONG *pcFetched)
{
	if (0 == cMediaTypes)
	{
		return E_INVALIDARG;
	}
	if (1 != cMediaTypes
		&& pcFetched == NULL)
	{
		return E_INVALIDARG;
	}
	if (enum_index >= 1)
	{
		if (pcFetched)
		{
			*pcFetched = 0;
		}
		return S_FALSE;
	}
	ppMediaTypes[0] = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
	if (ppMediaTypes[0] == NULL)
	{
		return E_OUTOFMEMORY;
	}
	CMediaType copy_type(type);
	enum_index++;

	*ppMediaTypes[0] = copy_type;
	copy_type.InitMediaType();	// resets to avoid double free
	if (pcFetched)
	{
		*pcFetched = 1;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SingleFormatMediaTypeEnum::Skip(
														/* [in] */ ULONG cMediaTypes)
{
	if (cMediaTypes > 2)
	{
		enum_index = 1;
		return S_FALSE;
	}
	enum_index += cMediaTypes;
	if (enum_index >= 1)
	{
		enum_index = 1;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SingleFormatMediaTypeEnum::Reset(void)
{
	enum_index = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SingleFormatMediaTypeEnum::Clone(
	/* [annotation][out] */
															_Out_  IEnumMediaTypes **ppEnum)
{
	if (NULL == ppEnum)
	{
		return E_POINTER;
	}
	try
	{
		*ppEnum = new SingleFormatMediaTypeEnum(type, enum_index);
		return S_OK;
	}
	catch (std::bad_alloc&)
	{
		return E_OUTOFMEMORY;
	}
}

SinglePinEnum::SinglePinEnum(IPin * p, int start_index)
	: RefCount(1)
	, enum_index(start_index)
	, pin(p)
{

}

HRESULT STDMETHODCALLTYPE SinglePinEnum::QueryInterface(
														/* [in] */ REFIID riid,
														/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (riid == IID_IEnumPins)
	{
		*ppvObject = static_cast<IEnumPins*>(this);
		AddRef();
		return S_OK;
	}
	if (riid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown*>(this);
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE SinglePinEnum::AddRef(void)
{
	return InterlockedIncrement(&RefCount);
}

ULONG STDMETHODCALLTYPE SinglePinEnum::Release(void)
{
	LONG ref = InterlockedDecrement(&RefCount);
	if (ref == 0)
	{
		delete this;
	}
	return ref;
}

HRESULT STDMETHODCALLTYPE SinglePinEnum::Next(
											/* [in] */ ULONG cPins,
	/* [annotation][size_is][out] */
											_Out_writes_to_(cPins, *pcFetched)  IPin **ppPins,
	/* [annotation][out] */
											_Out_opt_  ULONG *pcFetched)
{
	if (0 == ppPins
		|| 0 == cPins)
	{
		return E_INVALIDARG;
	}
	if (1 != cPins
		&& pcFetched == NULL)
	{
		return E_INVALIDARG;
	}
	if (enum_index >= 1)
	{
		if (pcFetched)
		{
			*pcFetched = 0;
		}
		return S_FALSE;
	}

	ppPins[0] = pin;
	ppPins[0]->AddRef();
	enum_index++;
	if (pcFetched)
	{
		*pcFetched = 1;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SinglePinEnum::Skip(
											/* [in] */ ULONG cPins)
{
	if (cPins > 2)
	{
		enum_index = 1;
		return S_FALSE;
	}
	enum_index += cPins;
	if (enum_index >= 1)
	{
		enum_index = 1;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SinglePinEnum::Reset(void)
{
	enum_index = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SinglePinEnum::Clone(
	/* [annotation][out] */
												_Out_  IEnumPins **ppEnum)
{
	if (NULL == ppEnum)
	{
		return E_POINTER;
	}
	try
	{
		*ppEnum = new SinglePinEnum(pin, enum_index);
		return S_OK;
	}
	catch (std::bad_alloc&)
	{
		return E_OUTOFMEMORY;
	}
}
